/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/syscalls.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/firmware.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <marlin_platform.h>
#include <wcn_bus.h>

#include "wcn_gnss.h"
#include "rf/rf.h"
#include "../sleep/sdio_int.h"
#include "../sleep/slp_mgr.h"
#include "mem_pd_mgr.h"
#include "wcn_op.h"
#include "wcn_parn_parser.h"
#include "pcie_boot.h"
#include "usb_boot.h"
#include "rdc_debug.h"
#include "wcn_dump.h"
#include "wcn_misc.h"
#include "wcn_log.h"
#include "wcn_procfs.h"
#include "mdbg_type.h"
#include "wcn_glb_reg.h"
#include "wcn_glb.h"

#ifdef CONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
#include "../fw/firmware_hex.h"
#endif

#ifdef CONFIG_AML_BOARD
#include <linux/amlogic/aml_gpio_consumer.h>

extern int wifi_irq_trigger_level(void);
#ifdef CONFIG_WCN_RESET_PIN_CONNECTED
extern void extern_bt_set_enable(int is_on);
#endif
extern void extern_wifi_set_enable(int is_on);
#endif

#ifdef CONFIG_HISI_BOARD
#include "vendor/hisilicon/hi_drv_gpio.h"

/* reset pin connect with gpio4_2 */
#define RTL_REG_RST_GPIO	(4*8 + 2)

enum hi_GPIO_DIR_E {
	HI_DIR_OUT = 0,
	HI_DIR_IN  = 1,
};
#endif

#ifdef CONFIG_AW_BOARD
#include <linux/pm_wakeirq.h>
//extern void sunxi_wlan_set_power(int on);
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);

struct gpio_config {
	u32	gpio;
	u32	mul_sel;
	u32	pull;
	u32	drv_level;
	u32	data;
};
#endif

#define WCN_FW_MAX_PATH_NUM	4
/* path of cp2 firmware. */
#ifdef CONFIG_CUSTOMIZE_UNISOC_FW_PATH
#define UNISOC_FW_PATH_DEFAULT CONFIG_CUSTOMIZE_UNISOC_FW_PATH
#else
#define UNISOC_FW_PATH_DEFAULT "/system/etc/firmware/"
#endif
static char *wcn_fw_path[WCN_FW_MAX_PATH_NUM] = {
	UNISOC_FW_PATH_DEFAULT,		/* most of projects */
	"/lib/firmware/"		/* allwinner r328... */
};
#define WCN_FW_NAME	"wcnmodem.bin"
#define GNSS_FW_NAME	"gnssmodem.bin"

#ifndef REG_PMU_APB_XTL_WAIT_CNT0
#define REG_PMU_APB_XTL_WAIT_CNT0 0xe42b00ac
#endif

static char BTWF_FIRMWARE_PATH[255];
static char GNSS_FIRMWARE_PATH[255];

struct wcn_sync_info_t {
	unsigned int init_status;
	unsigned int mem_pd_bt_start_addr;
	unsigned int mem_pd_bt_end_addr;
	unsigned int mem_pd_wifi_start_addr;
	unsigned int mem_pd_wifi_end_addr;
	unsigned int prj_type;
	unsigned int tsx_dac_data;
	unsigned int sdio_config;
	unsigned int dcache_status;
	unsigned int dcache_start_addr;
	unsigned int dcache_end_addr;
	unsigned int mem_pd_status;
	unsigned int mem_ap_cmd;
	unsigned int rsvd[3];
	unsigned int bind_verify_data[4];
};

struct firmware_backup {
	size_t size;
	u8 *data;
};

struct tsx_data {
	u32 flag; /* cali flag ref */
	u16 dac; /* AFC cali data */
	u16 reserved;
};

struct tsx_cali {
	u32 init_flag;
	struct tsx_data tsxdata;
};

/*
 * sdio config to cp side
 * bit[31:24]: reserved
 * bit[23]: wake_host_data_separation:
 *	0: if BT_WAKEUP_HOST en or WL_WAKEUP_HOST en,
 *	    wifi and bt packets can wake host;
 *	1: if BT_WAKEUP_HOST en, ONLY bt packets can wake host;
 *	    if WL_WAKEUP_HOST en, ONLY wifi packets can wake host
 * bit[22:18]: wake_host_level_duration_10ms: BT_WAKEUP_HOST or WL_WAKEUP_HOST
 *	      level dyration time per 10ms, example: 0:0ms; 3:30ms; 20:200ms
 * bit[17:16]: wl_wake_host_trigger_type:
 *	     00:WL_WAKEUP_HOST  trigger type low
 *	     01:WL_WAKEUP_HOST  trigger type rising
 *	     10:WL_WAKEUP_HOST  trigger type falling
 *	     11:WL_WAKEUP_HOST  trigger type high
 * bit[15]: wl_wake_host_en: 0: disable, 1: enable
 * bit[14:13]: sdio_irq_trigger_type:
 *	      00:pubint gpio irq trigger type low
 *	      01:pubint gpio irq trigger type rising [NOT support]
 *	      10:pubint gpio irq trigger type falling [NOT support]
 *	      11:pubint gpio irq trigger type high
 * bit[12:11]: sdio_irq_type:
 *	      00:dedicated irq, gpio1
 *	      01:inband data1 irq
 *	      10:use BT_WAKEUP_HOST(pubint) pin as gpio irq
 *	      11:use WL_WAKEUP_HOST(esmd3) pin as gpio irq
 * bit[10:9]: bt_wake_host_trigger_type:
 *	     00:BT_WAKEUP_HOST  trigger type low
 *	     01:BT_WAKEUP_HOST  trigger type rising
 *	     10:BT_WAKEUP_HOST  trigger type falling
 *	     11:BT_WAKEUP_HOST  trigger type high
 * bit[8]: bt_wake_host_en: 0: disable, 1: enable
 * bit[7:5]: sdio_blk_size: 000: blocksize 840; 001: blocksize 512
 * bit[4]: sdio_rx_mode: 0: adma; 1: sdma
 * bit[3:1]: vendor_id: 000: default id, unisoc[0x0]
 *		       001: hisilicon default version, pull chipen after resume
 *		       010: hisilicon version, keep power (NOT pull chipen) and
 *			    reset sdio after resume
 * bit[0]: sdio_config_en: 0: disable sdio config
 *		          1: enable sdio config
 */
union wcn_sdiohal_config {
	unsigned int val;
	struct {
		unsigned char sdio_config_en:1;
		unsigned char vendor_id:3;
		unsigned char sdio_rx_mode:1;
		unsigned char sdio_blk_size:3;
		unsigned char bt_wake_host_en:1;
		unsigned char bt_wake_host_trigger_type:2;
		unsigned char sdio_irq_type:2;
		unsigned char sdio_irq_trigger_type:2;
		unsigned char wl_wake_host_en:1;
		unsigned char wl_wake_host_trigger_type:2;
		unsigned char wake_host_level_duration_10ms:5;
		unsigned char wake_host_data_separation:1;
		unsigned int  reserved:8;
	} cfg;
};

struct wcn_clock_info {
	enum wcn_clock_type type;
	enum wcn_clock_mode mode;
	/*
	 * xtal-26m-clk-type-gpio config in the dts.
	 * if xtal-26m-clk-type config in the dts,this gpio unvalid.
	 */
	int gpio;
};

struct marlin_device {
	struct wcn_clock_info clk_xtal_26m;
	int coexist;
	int wakeup_ap;
	int ap_send_data;
	int reset;
	int chip_en;
	int int_ap;
	/* power sequence */
	/* VDDIO->DVDD12->chip_en->rst_N->AVDD12->AVDD33 */
	struct regulator *dvdd12;
	struct regulator *avdd12;
	/* for PCIe */
	struct regulator *avdd18;
	/* for wifi PA, BT TX RX */
	struct regulator *avdd33;
	/* for internal 26M clock */
	struct regulator *dcxo18;
	struct regulator *avdd33_usb20;
	struct clk *clk_32k;

	struct clk *clk_parent;
	struct clk *clk_enable;
	struct mutex power_lock;
	struct completion carddetect_done;
	struct completion download_done;
	struct completion gnss_download_done;
	unsigned long power_state;
	char *write_buffer;
	struct delayed_work power_wq;
	struct work_struct download_wq;
	struct work_struct gnss_dl_wq;
	bool no_power_off;
	bool wait_ge2;
	bool is_btwf_in_sysfs;
	bool is_gnss_in_sysfs;
	int wifi_need_download_ini_flag;
	int first_power_on_flag;
	unsigned char download_finish_flag;
	unsigned char bt_wl_wake_host_en;
	unsigned int bt_wake_host_int_num;
	unsigned int wl_wake_host_int_num;
	int loopcheck_status_change;
	struct wcn_sync_info_t sync_f;
	struct tsx_cali tsxcali;
	char *btwf_path;
	char *gnss_path;
	struct firmware_backup firmware;
};

struct wifi_calibration {
	struct wifi_config_t config_data;
	struct wifi_cali_t cali_data;
};

static struct wifi_calibration wifi_data;
struct completion ge2_completion;
static int first_call_flag;
marlin_reset_callback marlin_reset_func;
void *marlin_callback_para;
static struct raw_notifier_head marlin_reset_notifiers[MARLIN_ALL];

struct marlin_device *marlin_dev;
struct sprdwcn_gnss_ops *gnss_ops;

unsigned char  flag_reset;
char functionmask[8];
static unsigned int reg_val;
static unsigned int clk_wait_val;
static unsigned int cp_clk_wait_val;
static unsigned int marlin2_clk_wait_reg;

#ifdef CONFIG_WCN_PMIC
/* for PMIC control */
static struct regmap *reg_map;
#endif

#define IMG_HEAD_MAGIC "WCNM"
#define IMG_HEAD_MAGIC_COMBINE "WCNE"

#define IMG_MARLINAA_TAG "MLAA"
#define IMG_MARLINAB_TAG "MLAB"
#define IMG_MARLINAC_TAG "MLAC"
#define IMG_MARLINAD_TAG "MLAD"

#define IMG_MARLIN3_AA_TAG "30AA"
#define IMG_MARLIN3_AB_TAG "30AB"
#define IMG_MARLIN3_AC_TAG "30AC"
#define IMG_MARLIN3_AD_TAG "30AD"

#define IMG_MARLIN3L_AA_TAG "3LAA"
#define IMG_MARLIN3L_AB_TAG "3LAB"
#define IMG_MARLIN3L_AC_TAG "3LAC"
#define IMG_MARLIN3L_AD_TAG "3LAD"

#define IMG_MARLIN3E_AA_TAG "3EAA"
#define IMG_MARLIN3E_AB_TAG "3EAB"
#define IMG_MARLIN3E_AC_TAG "3EAC"
#define IMG_MARLIN3E_AD_TAG "3EAD"

#define MARLIN_MASK 0x27F
#define GNSS_MASK 0x080
#define AUTO_RUN_MASK 0X100

#define AFC_CALI_FLAG 0x54463031 /* cali flag */
#define AFC_CALI_READ_FINISH 0x12121212
#define WCN_AFC_CALI_PATH "/productinfo/wcn/tsx_bt_data.txt"

#ifdef CONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
#define POWER_WQ_DELAYED_MS 0
#else
#define POWER_WQ_DELAYED_MS 7500
#endif

/* #define E2S(x) { case x: return #x; } */

struct head {
	char magic[4];
	unsigned int version;
	unsigned int img_count;
};

struct imageinfo {
	char tag[4];
	unsigned int offset;
	unsigned int size;
};

struct combin_img_info {
	unsigned int addr;			/* image target address */
	unsigned int offset;			/* image combin img offset */
	unsigned int size;			/* image size */
};

/* get wcn module hardware interface type. */
enum wcn_hw_type wcn_get_hw_if_type(void)
{
#if defined(CONFIG_WCN_SDIO)
	return HW_TYPE_SDIO;
#elif defined(CONFIG_WCN_PCIE)
	return HW_TYPE_PCIE;
#elif defined(CONFIG_WCN_SIPC)
	return HW_TYPE_SIPC;
#elif defined(CONFIG_WCN_USB)
	return HW_TYPE_USB;
#else
	return HW_TYPE_UNKNOWN;
#endif
}
EXPORT_SYMBOL_GPL(wcn_get_hw_if_type);

unsigned long marlin_get_power_state(void)
{
	return marlin_dev->power_state;
}
EXPORT_SYMBOL_GPL(marlin_get_power_state);

unsigned char marlin_get_bt_wl_wake_host_en(void)
{
	return marlin_dev->bt_wl_wake_host_en;
}
EXPORT_SYMBOL_GPL(marlin_get_bt_wl_wake_host_en);

/* return chipid, for example:
 * 0x2355000x: Marlin3 series
 * 0x2355B00x: Marlin3 lite series
 * 0x5663000x: Marlin3E series
 * 0: read chipid fail or not unisoc module
 */
#define WCN_CHIPID_MASK (0xFFFFF000)
unsigned int marlin_get_wcn_chipid(void)
{
	int ret;
	static unsigned long int chip_id;
#ifdef CONFIG_WCN_USB
	return MARLIN3E_AA_CHIPID;
#endif
	if (likely(chip_id))
		return chip_id;

	WCN_DEBUG("%s enter.\n", __func__);

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
	ret = sprdwcn_bus_reg_read(CHIPID_REG, &chip_id, 4);
	if (ret < 0) {
		WCN_ERR("marlin read chip ID fail, ret=%d\n", ret);
		return 0;
	}

#else
	/* At first, read Marlin3E chipid register. */
	ret = sprdwcn_bus_reg_read(CHIPID_REG_M3E, &chip_id, 4);
	if (ret < 0) {
		WCN_ERR("read marlin3E chip id fail, ret=%d\n", ret);
		return 0;
	}

	/* If it is not Marlin3E, then read Marlin3 chipid register. */
	if ((chip_id & WCN_CHIPID_MASK) != MARLIN3E_AA_CHIPID) {
		ret = sprdwcn_bus_reg_read(CHIPID_REG_M3_M3L, &chip_id, 4);
		if (ret < 0) {
			WCN_ERR("read marlin3 chip id fail, ret=%d\n", ret);
			return 0;
		}
	}
#endif
	WCN_INFO("%s: chipid: 0x%lx\n", __func__, chip_id);

	return chip_id;
}
EXPORT_SYMBOL_GPL(marlin_get_wcn_chipid);

/* return chip model, for example:
 * 0: WCN_CHIP_INVALID
 * 1: WCN_CHIP_MARLIN3
 * 2: WCN_CHIP_MARLIN3L
 * 3: WCN_CHIP_MARLIN3E
 */
enum wcn_chip_model wcn_get_chip_model(void)
{
	static enum wcn_chip_model chip_model = WCN_CHIP_INVALID;
	unsigned int chip_id;
	static const char *chip_model_str[] = {
		"ERRO",
		"Marlin3",
		"Marlin3Lite",
		"Marlin3E",
	};

	if (likely(chip_model))
		return chip_model;

	/* if read chipid fail or not unisoc module, chip_id is 0. */
	chip_id = marlin_get_wcn_chipid();
	if (chip_id == 0)
		chip_model = WCN_CHIP_INVALID;
	else if ((chip_id & WCN_CHIPID_MASK) == MARLIN3_AA_CHIPID)
		chip_model = WCN_CHIP_MARLIN3;
	else if ((chip_id & WCN_CHIPID_MASK) == MARLIN3L_AA_CHIPID)
		chip_model = WCN_CHIP_MARLIN3L;
	else if ((chip_id & WCN_CHIPID_MASK) == MARLIN3E_AA_CHIPID)
		chip_model = WCN_CHIP_MARLIN3E;
	else
		chip_model = WCN_CHIP_INVALID;
	WCN_DEBUG("%s: chip_model: %s\n", __func__, chip_model_str[chip_model]);

	return chip_model;
}
EXPORT_SYMBOL_GPL(wcn_get_chip_model);

/* return chip type, for example:
 * 0: WCN_CHIP_ID_INVALID
 * 1: WCN_CHIP_ID_AA
 * 2: WCN_CHIP_ID_AB
 * 3: WCN_CHIP_ID_AC
 * 4: WCN_CHIP_ID_AD
 */
enum wcn_chip_id_type wcn_get_chip_type(void)
{
	static enum wcn_chip_id_type chip_type = WCN_CHIP_ID_INVALID;
	unsigned int chip_id;
	static const char *chip_type_str[] = {
		"ERRO",
		"AA",
		"AB",
		"AC",
		"AD",
	};

	if (likely(chip_type))
		return chip_type;

	/* if read chipid fail or not unisoc module, chip_id is 0. */
	chip_id = marlin_get_wcn_chipid();
	if (chip_id == 0)
		chip_type = WCN_CHIP_ID_INVALID;
	else
		chip_type = (chip_id & 0xF) + 1;
	WCN_DEBUG("%s: chip_type: %s\n", __func__, chip_type_str[chip_type]);

	return chip_type;
}
EXPORT_SYMBOL_GPL(wcn_get_chip_type);

#define WCN_CHIP_NAME_UNKNOWN "UNKNOWN"

/* Marlin3_AD_0x23550003 */
const char *wcn_get_chip_name(void)
{
	unsigned int chip_id, pos = 0;
	enum wcn_chip_model chip_model;
	enum wcn_chip_id_type chip_type;
	static char wcn_chip_name[32] = { 0 };
	static const char *chip_model_str[] = {
		"ERRO_",
		"Marlin3_",
		"Marlin3Lite_",
		"Marlin3E_",
	};
	static const char *chip_type_str[] = {
		"ERRO_",
		"AA_",
		"AB_",
		"AC_",
		"AD_",
	};

	if (wcn_chip_name[0])
		return wcn_chip_name;

	/* if read chipid fail or not unisoc module, chip_id is 0. */
	chip_id = marlin_get_wcn_chipid();
	if (chip_id == 0) {
		memcpy((void *)wcn_chip_name, WCN_CHIP_NAME_UNKNOWN,
		       strlen(WCN_CHIP_NAME_UNKNOWN));
		goto out;
	}

	chip_model = wcn_get_chip_model();
	if (chip_model == WCN_CHIP_INVALID) {
		memcpy((void *)wcn_chip_name, WCN_CHIP_NAME_UNKNOWN,
		       strlen(WCN_CHIP_NAME_UNKNOWN));
		goto out;
	}
	sprintf((char *)wcn_chip_name, chip_model_str[chip_model]);
	pos += strlen(chip_model_str[chip_model]);

	chip_type = wcn_get_chip_type();
	if (chip_type == WCN_CHIP_ID_INVALID) {
		memcpy((void *)wcn_chip_name, WCN_CHIP_NAME_UNKNOWN,
		       strlen(WCN_CHIP_NAME_UNKNOWN));
		goto out;
	}
	sprintf((char *)(wcn_chip_name + pos), chip_type_str[chip_type]);
	pos += strlen(chip_type_str[chip_type]);
	sprintf((char *)(wcn_chip_name + pos), "0x%x", chip_id);

out:
	WCN_DEBUG("%s: chip_name: %s\n", __func__, wcn_chip_name);
	return wcn_chip_name;
}
EXPORT_SYMBOL_GPL(wcn_get_chip_name);

/*
 * Some platforms not insmod bsp ko dynamically. This function is used for
 * wifi or bt to insmod driver statically.
 * return: 0: unisoc module; -1: other module
 */
#ifdef CONFIG_WCN_CHECK_MODULE_VENDOR
static int marlin_find_sdio_device_id(unsigned char *path)
{
	int i, open_cnt = 6;
	struct file *filp;
	loff_t pos;
	unsigned char read_buf[64], sdio_id_path[64];
	char *sdio_id_pos;

	sprintf(sdio_id_path, path);
	for (i = 0; i < open_cnt; i++) {
		filp = filp_open(sdio_id_path, O_RDONLY, 0644);
		if (IS_ERR(filp))
			msleep(100);
		else
			break;
	}
	if (i >= open_cnt) {
		WCN_ERR("%s open %s fail no.%d cnt=%d\n", __func__,
			sdio_id_path, (int)IS_ERR(filp), i);
		/* other module */
		return -1;
	}
	WCN_INFO("%s open %s success cnt=%d\n", __func__,
		 sdio_id_path, i);
	pos = 0;
	kernel_read(filp, read_buf, sizeof(read_buf), &pos);
	WCN_INFO("%s read_buf: %s\n", __func__, read_buf);
	sdio_id_pos = strstr(read_buf, "SDIO_ID=0000:0000");
	if (!sdio_id_pos) {
		WCN_ERR("%s sdio id not match (0000:0000)\n", __func__);
		filp_close(filp, NULL);
		/* other module */
		return -1;
	}
	filp_close(filp, NULL);
	WCN_INFO("%s: This is unisoc module\n", __func__);

	/* unisoc module */
	return 0;
}
#endif

#define SDIO_DEVICE_ID_PATH0 ("/sys/bus/sdio/devices/mmc0:8800:1/uevent")
#define SDIO_DEVICE_ID_PATH1 ("/sys/bus/sdio/devices/mmc1:8800:1/uevent")
#define SDIO_DEVICE_ID_PATH2 ("/sys/bus/sdio/devices/mmc2:8800:1/uevent")
int marlin_get_wcn_module_vendor(void)
{
	int ret = 0;

#ifdef CONFIG_WCN_CHECK_MODULE_VENDOR
	ret = marlin_find_sdio_device_id(SDIO_DEVICE_ID_PATH1);
	if (!ret)
		return ret;
	ret = marlin_find_sdio_device_id(SDIO_DEVICE_ID_PATH2);
	if (!ret)
		return ret;
	ret = marlin_find_sdio_device_id(SDIO_DEVICE_ID_PATH0);
	return ret;
#endif

	/* unisoc module */
	return ret;
}
EXPORT_SYMBOL_GPL(marlin_get_wcn_module_vendor);

/* return number of marlin antennas
 * MARLIN_TWO_ANT; MARLIN_THREE_ANT; ...
 */
int marlin_get_ant_num(void)
{
	return get_board_ant_num();
}
EXPORT_SYMBOL_GPL(marlin_get_ant_num);

/* get the subsys string */
const char *strno(int subsys)
{
	switch (subsys) {
	case MARLIN_BLUETOOTH:
		return "MARLIN_BLUETOOTH";
	case MARLIN_FM:
		return "MARLIN_FM";
	case MARLIN_WIFI:
		return "MARLIN_WIFI";
	case MARLIN_WIFI_FLUSH:
		return "MARLIN_WIFI_FLUSH";
	case MARLIN_SDIO_TX:
		return "MARLIN_SDIO_TX";
	case MARLIN_SDIO_RX:
		return "MARLIN_SDIO_RX";
	case MARLIN_MDBG:
		return "MARLIN_MDBG";
	case MARLIN_GNSS:
		return "MARLIN_GNSS";
	case WCN_AUTO:
		return "WCN_AUTO";
	case MARLIN_ALL:
		return "MARLIN_ALL";
	default: return "MARLIN_SUBSYS_UNKNOWN";
	}
/* #undef E2S */
}

/* tsx/dac init */
int marlin_tsx_cali_data_read(struct tsx_data *p_tsx_data)
{
	u32 size = 0;
	u32 read_len = 0;
	struct file *file;
	loff_t offset = 0;
	char *pdata;

	file = filp_open(WCN_AFC_CALI_PATH, O_RDONLY, 0);
	if (IS_ERR(file)) {
		WCN_ERR("open file error\n");
		return -1;
	}
	WCN_INFO("open image "WCN_AFC_CALI_PATH" successfully\n");

	/* read file to buffer */
	size = sizeof(struct tsx_data);
	pdata = (char *)p_tsx_data;
	do {
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
		read_len = kernel_read(file, (void *)pdata, size, &offset);
#else
		read_len = kernel_read(file, offset, pdata, size);
#endif

		if (read_len > 0) {
			size -= read_len;
			pdata += read_len;
		}
	} while ((read_len > 0) && (size > 0));
	fput(file);
	WCN_INFO("After read, data =%p dac value %02x\n", pdata,
			 p_tsx_data->dac);

	return 0;
}

static u16 marlin_tsx_cali_data_get(void)
{
	int ret;

	return 0;

	WCN_INFO("tsx cali init flag %d\n", marlin_dev->tsxcali.init_flag);

	if (marlin_dev->tsxcali.init_flag == AFC_CALI_READ_FINISH)
		return marlin_dev->tsxcali.tsxdata.dac;

	ret = marlin_tsx_cali_data_read(&marlin_dev->tsxcali.tsxdata);
	marlin_dev->tsxcali.init_flag = AFC_CALI_READ_FINISH;
	if (ret != 0) {
		marlin_dev->tsxcali.tsxdata.dac = 0xffff;
		WCN_INFO("tsx cali read fail! default 0xffff\n");
		return marlin_dev->tsxcali.tsxdata.dac;
	}

	if (marlin_dev->tsxcali.tsxdata.flag != AFC_CALI_FLAG) {
		marlin_dev->tsxcali.tsxdata.dac = 0xffff;
		WCN_INFO("tsx cali flag fail! default 0xffff\n");
		return marlin_dev->tsxcali.tsxdata.dac;
	}
	WCN_INFO("dac flag %d value:0x%x\n",
			    marlin_dev->tsxcali.tsxdata.flag,
			    marlin_dev->tsxcali.tsxdata.dac);

	return marlin_dev->tsxcali.tsxdata.dac;
}

#define marlin_firmware_get_combin_info(buffer) \
		(struct combin_img_info *)(buffer + sizeof(struct head))

#define bin_magic_is(data, magic_tag) \
	!strncmp(((struct head *)data)->magic, magic_tag, strlen(magic_tag))

#define marlin_fw_get_img_count(img) (((struct head *)img)->img_count)

static const struct imageinfo *marlin_imageinfo_get_from_data(const char *tag,
		const void *data)
{
	const struct imageinfo *imageinfo;
	int imageinfo_count;
	int i;

	imageinfo = (struct imageinfo *)(data + sizeof(struct head));
	imageinfo_count = marlin_fw_get_img_count(data);

	for (i = 0; i < imageinfo_count; i++)
		if (!strncmp(imageinfo[i].tag, tag, 4))
			return &(imageinfo[i]);
	return NULL;
}

static const struct imageinfo *marlin_judge_images(const unsigned char *buffer)
{
	unsigned long chip_id;
	const struct imageinfo *image_info;

	chip_id = marlin_get_wcn_chipid();
	if (buffer == NULL)
		return NULL;

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3) {
		switch (chip_id) {
		case MARLIN_AA_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAA_TAG,
							      buffer);
		case MARLIN_AB_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAB_TAG,
							      buffer);
		case MARLIN_AC_CHIPID:
		case MARLIN_AD_CHIPID:
			/* bin of m3 AC and AD chip is the same. */
			image_info = marlin_imageinfo_get_from_data(
				IMG_MARLINAC_TAG, buffer);
			if (image_info) {
				WCN_INFO("%s find %s tag\n", __func__,
					 IMG_MARLINAC_TAG);
				return image_info;
			}
			return marlin_imageinfo_get_from_data(IMG_MARLINAD_TAG,
							      buffer);
		default:
			WCN_ERR("Marlin can't find correct imginfo!\n");
		}
	}
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
		switch (chip_id) {
		case MARLIN_AA_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAA_TAG,
							      buffer);
		case MARLIN_AB_CHIPID:
			return marlin_imageinfo_get_from_data(IMG_MARLINAB_TAG,
							      buffer);
		default:
			WCN_ERR("Marlin can't find correct imginfo!\n");
		}
	}
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E) {
		switch (chip_id) {
		case MARLIN_AA_CHIPID:
		case MARLIN_AB_CHIPID:
			/* bin of m3e AA and AB chip is the same. */
			image_info = marlin_imageinfo_get_from_data(
				IMG_MARLINAA_TAG, buffer);
			if (image_info) {
				WCN_INFO("%s find %s tag\n", __func__,
					 IMG_MARLINAA_TAG);
				return image_info;
			}
			return marlin_imageinfo_get_from_data(IMG_MARLINAB_TAG,
							      buffer);
		default:
			WCN_ERR("Marlin can't find correct imginfo!\n");
		}
	}

#else  /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

	switch (chip_id) {
	case MARLIN3_AA_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AA_TAG, buffer);
	case MARLIN3_AB_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AB_TAG, buffer);
	case MARLIN3_AC_CHIPID:
	case MARLIN3_AD_CHIPID:
		/* bin of m3 AC and AD chip is the same. */
		image_info = marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AC_TAG, buffer);
		if (image_info) {
			WCN_INFO("%s find %s tag\n", __func__,
				 IMG_MARLIN3_AC_TAG);
			return image_info;
		}
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3_AD_TAG, buffer);
	case MARLIN3L_AA_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AA_TAG, buffer);
	case MARLIN3L_AB_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AB_TAG, buffer);
	case MARLIN3L_AC_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AC_TAG, buffer);
	case MARLIN3L_AD_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3L_AD_TAG, buffer);
	case MARLIN3E_AA_CHIPID:
	case MARLIN3E_AB_CHIPID:
		/* bin of m3e AA and AB chip is the same. */
		image_info = marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AA_TAG, buffer);
		if (image_info) {
			WCN_INFO("%s find %s tag\n", __func__,
				 IMG_MARLIN3E_AA_TAG);
			return image_info;
		}
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AB_TAG, buffer);
	case MARLIN3E_AC_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AC_TAG, buffer);
	case MARLIN3E_AD_CHIPID:
		return marlin_imageinfo_get_from_data(
			IMG_MARLIN3E_AD_TAG, buffer);
	default:
		WCN_INFO("%s Cannot find Chip Firmware\n", __func__);
		break;
	}
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	return NULL;
}

static char *load_firmware_data_path(const char *path, loff_t offset,
	unsigned long int imag_size)
{
	int read_len, size, i, opn_num_max = 1;
	char *buffer = NULL;
	char *data = NULL;
	struct file *file;

	WCN_DEBUG("%s Enter\n", __func__);
	file = filp_open(path, O_RDONLY, 0);
	for (i = 1; i <= opn_num_max; i++) {
		if (IS_ERR(file)) {
			WCN_DEBUG("%s: try open file %s,count_num:%d\n",
				  __func__, path, i);
			ssleep(1);
			file = filp_open(path, O_RDONLY, 0);
		} else
			break;
	}
	if (IS_ERR(file)) {
		WCN_ERR("%s: open file %s error\n",
			__func__, path);
		return NULL;
	}
	WCN_DEBUG("marlin %s open image file sucessfully\n",
		  __func__);
	size = imag_size;
	buffer = vmalloc(size);
	if (!buffer) {
		fput(file);
		WCN_ERR("no memory for image\n");
		return NULL;
	}

	data = buffer;
	do {
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
		read_len = kernel_read(file, (void *)buffer, size, &offset);
#else
		read_len = kernel_read(file, offset, buffer, size);
#endif
		if (read_len > 0) {
			size -= read_len;
			buffer += read_len;
		}
	} while ((read_len > 0) && (size > 0));
	fput(file);
	WCN_DEBUG("%s finish read_Len:%d\n", __func__, read_len);
#if KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE
	/* kernel_read return value changed. */
	if (read_len <= 0)
		return NULL;
#endif

	return data;
}

static int sprdwcn_bus_direct_write_dispack(unsigned int addr, const void *buf,
		size_t buf_size, size_t packet_max_size)
{
	int ret = 0;
	size_t offset = 0;
	void *kbuf = marlin_dev->write_buffer;

	while (offset < buf_size) {
		size_t temp_size = min(packet_max_size, buf_size - offset);

		memcpy(kbuf, buf + offset, temp_size);
		ret = sprdwcn_bus_direct_write(addr + offset, kbuf, temp_size);
		if (ret < 0)
			goto OUT;

		offset += temp_size;
	}
OUT:
	if (ret < 0)
		WCN_ERR(" %s: dt write error:%d\n", __func__, ret);
	return ret;
}

struct marlin_firmware {
	const u8 *data;
	size_t size;
	bool is_from_fs;
	const void *priv;
};

/* this function __must__ be paired with marlin_firmware_release !*/
/* Suggest use it like Documentation/firmware_class/README:65
 *
 *	if(marlin_request_firmware(&fw) == 0)
 *		handle_this_firmware(fw);
 *	marlin_release_firmware(fw);
 */
static int marlin_request_firmware(struct marlin_firmware **mfirmware_p)
{
	struct marlin_firmware *mfirmware;
#ifndef CONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
	unsigned char load_fw_cnt = 0;
	const void *buffer;
	const struct firmware *firmware;
	int ret = 0;
#endif

	*mfirmware_p = NULL;
	mfirmware = kmalloc(sizeof(struct marlin_firmware), GFP_KERNEL);
	if (!mfirmware)
		return -ENOMEM;

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->is_btwf_in_sysfs = 1;
#endif
#ifdef CONFIG_AW_BOARD
	marlin_dev->is_btwf_in_sysfs = 1;
#endif

#ifdef CONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX

	/* Some customer (amlogic) download fw from /fw/wcnmodem.bin.hex */
	WCN_INFO("marlin %s from wcnmodem.bin.hex start!\n", __func__);
	mfirmware->data = firmware_hex_buf;
	mfirmware->size = FIRMWARE_HEX_SIZE;
	mfirmware->is_from_fs = 0;
	mfirmware->priv = firmware_hex_buf;

#else /* CONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX */

	if (marlin_dev->is_btwf_in_sysfs != 1) {
		/*
		 * If first power on, download from partition,
		 * else download from backup firmware.
		 */
		if (marlin_dev->first_power_on_flag == 1) {
			WCN_INFO("%s from %s%s start!\n", __func__,
				 wcn_fw_path[0], WCN_FW_NAME);
			ret = request_firmware(&firmware, WCN_FW_NAME, NULL);
			if (ret < 0) {
				WCN_ERR("%s not find %s errno:(%d)(ignore!!)\n",
					__func__, WCN_FW_NAME, ret);
				marlin_dev->is_btwf_in_sysfs = 1;

				return ret;
			}

			mfirmware->priv = (void *)firmware;
			mfirmware->data = firmware->data;
			mfirmware->size = firmware->size;
			mfirmware->is_from_fs = 1;
			marlin_dev->firmware.size = firmware->size;
			marlin_dev->firmware.data = vmalloc(firmware->size);
			if (!marlin_dev->firmware.data) {
				WCN_INFO("%s malloc backup error!\n", __func__);
				return -1;
			}

			memcpy(marlin_dev->firmware.data, firmware->data,
			       firmware->size);
		} else {
			WCN_INFO("marlin %s from backup start!\n", __func__);
			mfirmware->priv = (void *)(&marlin_dev->firmware);
			mfirmware->data = marlin_dev->firmware.data;
			mfirmware->size = marlin_dev->firmware.size;
			mfirmware->is_from_fs = 0;
		}
	} else {
		/* NOTE! We canot guarantee the img is complete when we read it
		 * first! The macro FIRMWARE_MAX_SIZE only guarantee AA(first in
		 * partition) img is complete. So we need read this img two
		 * times (other time in marlin_firmware_parse_image)
		 */
load_fw:
		WCN_INFO("%s from %s start!\n", __func__, BTWF_FIRMWARE_PATH);
		marlin_dev->is_btwf_in_sysfs = 1;
		buffer = load_firmware_data_path(BTWF_FIRMWARE_PATH, 0,
						 FIRMWARE_MAX_SIZE);
		if (!buffer) {
			load_fw_cnt++;
			WCN_DEBUG("%s buff is NULL\n", __func__);
			if (load_fw_cnt < WCN_FW_MAX_PATH_NUM) {
				sprintf(BTWF_FIRMWARE_PATH, "%s%s",
					wcn_fw_path[load_fw_cnt],
					WCN_FW_NAME);
				goto load_fw;
			}
			kfree(mfirmware);
			return -1;
		}

		mfirmware->data = buffer;
		mfirmware->size = FIRMWARE_MAX_SIZE;
		mfirmware->is_from_fs = 0;
		mfirmware->priv = buffer;
	}

#endif /* CONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX */

	memcpy(functionmask, mfirmware->data, 8);
	if ((functionmask[0] == 0x00) && (functionmask[1] == 0x00)) {
		mfirmware->data += 8;
		mfirmware->size -= 8;
	} else {
		functionmask[7] = 0;
	}

	*mfirmware_p = mfirmware;

	return 0;
}

static int marlin_firmware_parse_image(struct marlin_firmware *mfirmware)
{
	int offset = 0;
	int size = 0;
	int old_mfirmware_size = mfirmware->size;

	if (bin_magic_is(mfirmware->data, IMG_HEAD_MAGIC)) {
		const struct imageinfo *info;

		WCN_INFO("%s imagepack is %s type,need parse it\n",
			 __func__, IMG_HEAD_MAGIC);
		info = marlin_judge_images(mfirmware->data);
		if (!info) {
			WCN_ERR("marlin:%s imginfo is NULL\n", __func__);
			return -1;
		}
		mfirmware->size = info->size;
		mfirmware->data += info->offset;
		offset = info->offset;
		size = info->size;
	} else if (bin_magic_is(mfirmware->data, IMG_HEAD_MAGIC_COMBINE)) {
		/* cal the combin size */
		int img_count;
		const struct combin_img_info *img_info;
		int img_real_size = 0;

		WCN_INFO("%s imagepack is %s type,need parse it\n",
			 __func__, IMG_HEAD_MAGIC_COMBINE);

		img_count = marlin_fw_get_img_count(mfirmware->data);
		img_info = marlin_firmware_get_combin_info(mfirmware->data);

		img_real_size =
		img_info[img_count - 1].size + img_info[img_count - 1].offset;

		mfirmware->size = img_real_size;
		size = img_real_size;
	}

#ifndef CONFIG_WCN_RESUME_POWER_DOWN
	if (!mfirmware->is_from_fs && (offset + size) > old_mfirmware_size) {
		const void *buffer;

		/* NOTE! We canot guarantee the img is complete when we read it
		 * first! The macro FIRMWARE_MAX_SIZE only guarantee AA(first in
		 * partition) img is complete. So we need read this img two
		 * times (in this)
		 */
		buffer = load_firmware_data_path(BTWF_FIRMWARE_PATH,
				offset, size);
		if (!buffer) {
			WCN_ERR("marlin:%s buffer is NULL\n", __func__);
			return -1;
		}
		/* struct data "info" is a part of mfirmware->priv,
		 * if we free mfirmware->priv, "info" will be free too!
		 * so we need free priv at here after use "info"
		 */
		vfree(mfirmware->priv);
		mfirmware->data = buffer;
		mfirmware->priv = buffer;
	}
#endif

	return 0;
}

#ifdef CONFIG_WCN_USB
int marlin_firmware_write(struct marlin_firmware *fw)
{
	int i;
	int img_count;
	const struct combin_img_info *img_info;
	int ret;

	if (!bin_magic_is(fw->data, IMG_HEAD_MAGIC_COMBINE)) {
		WCN_ERR("Marlin3 USB image must have maigc WCNE\n");
		ret = marlin_firmware_download_usb(0x40500000, fw->data,
				0x56F00, PACKET_SIZE);
		if (ret) {
			WCN_ERR("%s usb download error\n", __func__);
			return -1;
		}
		marlin_firmware_download_exec_usb(WCN_USB_FW_ADDR);
		return 0;
	}

	img_count = marlin_fw_get_img_count(fw->data);
	img_info = marlin_firmware_get_combin_info(fw->data);

	/* for every img_info */
	for (i = 0; i < img_count; i++) {
		if (img_info[i].size + img_info[i].offset > fw->size) {
			WCN_ERR("%s memory crossover\n", __func__);
			return -1;
		}
		ret = marlin_firmware_download_usb(img_info[i].addr,
				fw->data + img_info[i].offset,
				img_info[i].size, PACKET_SIZE);
		if (ret) {
			WCN_ERR("%s usb download error\n", __func__);
			return -1;
		}
	}

	marlin_firmware_download_exec_usb(WCN_USB_FW_ADDR);
	return 0;
}
#else
static int marlin_firmware_write(struct marlin_firmware *mfirmware)
{
	int i = 0;
	int combin_img_count;
	const struct combin_img_info *imginfoe;
	int err;

	if (bin_magic_is(mfirmware->data, IMG_HEAD_MAGIC_COMBINE)) {
		WCN_INFO("marlin %s imagepack is WCNE type,need parse it\n",
			__func__);

		combin_img_count = marlin_fw_get_img_count(mfirmware->data);
		imginfoe = marlin_firmware_get_combin_info(mfirmware->data);
		if (!imginfoe) {
			WCN_ERR("marlin:%s imginfo is NULL\n", __func__);
			return -1;
		}

		for (i = 0; i < combin_img_count; i++) {
			if (imginfoe[i].size + imginfoe[i].offset >
					mfirmware->size) {
				WCN_ERR("%s memory crossover\n", __func__);
				return -1;
			}
			err = sprdwcn_bus_direct_write_dispack(imginfoe[i].addr,
					mfirmware->data + imginfoe[i].offset,
					imginfoe[i].size, PACKET_SIZE);
			if (err) {
				WCN_ERR("%s download error\n", __func__);
				return -1;
			}
		}
	} else {
		err = sprdwcn_bus_direct_write_dispack(CP_START_ADDR,
				mfirmware->data, mfirmware->size, PACKET_SIZE);
		if (err) {
			WCN_ERR("%s download error\n", __func__);
			return -1;
		}
	}
	WCN_INFO("combin_img %d %s finish and successful\n", i, __func__);

	return 0;
}
#endif

static void marlin_release_firmware(struct marlin_firmware *mfirmware)
{
	if (mfirmware) {
		if (mfirmware->is_from_fs)
			release_firmware(mfirmware->priv);
		kfree(mfirmware);
	}
}

int wcn_gnss_ops_register(struct sprdwcn_gnss_ops *ops)
{
	if (gnss_ops) {
		WARN_ON(1);
		return -EBUSY;
	}

	gnss_ops = ops;

	return 0;
}

void wcn_gnss_ops_unregister(void)
{
	gnss_ops = NULL;
}
static int gnss_download_firmware(void)
{
	char *buf;
	int err;
	unsigned int load_fw_cnt = 0;

	buf = marlin_dev->write_buffer;
reload:
	WCN_DEBUG("%s %d.load from %s\n", __func__, load_fw_cnt + 1, GNSS_FIRMWARE_PATH);
	if (gnss_ops && (gnss_ops->set_file_path))
		gnss_ops->set_file_path(&GNSS_FIRMWARE_PATH[0]);
	else
		WCN_ERR("%s gnss_ops set_file_path error\n", __func__);

	buf = load_firmware_data_path(GNSS_FIRMWARE_PATH, 0, GNSS_FIRMWARE_MAX_SIZE);

	if (!buf) {
		++load_fw_cnt;
		if (load_fw_cnt < WCN_FW_MAX_PATH_NUM) {
			sprintf(GNSS_FIRMWARE_PATH, "%s%s", \
					wcn_fw_path[load_fw_cnt], GNSS_FW_NAME);
			goto reload;
		}
	}

	err = sprdwcn_bus_direct_write_dispack(GNSS_CP_START_ADDR, \
			buf, GNSS_FIRMWARE_MAX_SIZE, PACKET_SIZE);
	if (err)
		WCN_INFO("%s download error\n", __func__);

	return err;
}

/* BT WIFI FM download */
static int btwifi_download_firmware(void)
{
	int ret;
	struct marlin_firmware *mfirmware;

	ret = marlin_request_firmware(&mfirmware);
	if (ret) {
		WCN_ERR("%s request firmware error\n", __func__);
		goto OUT;
	}

#ifdef CONFIG_WCN_USB
	/**
	 * marlin3e chip romcode cause this:
	 * if you didn't send start_usb command
	 * then you can't read chip id and donwload code.
	 * so, we must sent start_usb command in here.
	 */
#endif

	ret = marlin_firmware_parse_image(mfirmware);
	if (ret) {
		WCN_ERR("%s firmware parse AA\\AB error\n", __func__);
		goto OUT;
	}

	ret = marlin_firmware_write(mfirmware);
	if (ret) {
		WCN_ERR("%s firmware write error\n", __func__);
		goto OUT;
	}

OUT:
	marlin_release_firmware(mfirmware);

	return ret;
}

#ifdef CONFIG_AW_BOARD
static void marlin_bt_wake_int_en(void)
{
	enable_irq(marlin_dev->bt_wake_host_int_num);
}

static void marlin_bt_wake_int_dis(void)
{
	disable_irq(marlin_dev->bt_wake_host_int_num);
}

static irqreturn_t marlin_bt_wake_int_isr(int irq, void *para)
{
	static int bt_wake_cnt;

	bt_wake_cnt++;
	WCN_DEBUG("bt_wake_irq_cnt %d\n", bt_wake_cnt);
	return IRQ_HANDLED;
}

static int marlin_registsr_bt_wake(struct device *dev)
{
	struct device_node *np;
	int bt_wake_host_gpio, ret = 0;

	np = of_find_compatible_node(NULL, NULL, "allwinner,sunxi-btlpm");
	if (!np) {
		WCN_ERR("dts node for bt_wake not found");
		return -EINVAL;
	}
	bt_wake_host_gpio = of_get_named_gpio(np, "bt_hostwake", 0);
	if (!gpio_is_valid(bt_wake_host_gpio)) {
		WCN_ERR("bt_hostwake irq is invalid: %d\n",
			bt_wake_host_gpio);
		return -EINVAL;
	}

	ret = devm_gpio_request(dev, bt_wake_host_gpio,
				"bt-wake-host-gpio");
	if (ret) {
		WCN_ERR("bt-wake-host-gpio request err: %d\n",
			bt_wake_host_gpio);
		return ret;
	}

	ret = gpio_direction_input(bt_wake_host_gpio);
	if (ret < 0) {
		WCN_ERR("%s, gpio-%d input set fail!\n", __func__,
			bt_wake_host_gpio);
		return ret;
	}

	marlin_dev->bt_wake_host_int_num = gpio_to_irq(bt_wake_host_gpio);

	WCN_INFO("%s bt_hostwake gpio=%d intnum=%d\n", __func__,
		 bt_wake_host_gpio, marlin_dev->bt_wake_host_int_num);

	ret = device_init_wakeup(dev, true);
	if (ret < 0) {
		WCN_ERR("device init bt wakeup failed!\n");
		return ret;
	}

	ret = dev_pm_set_wake_irq(dev,
		marlin_dev->bt_wake_host_int_num);
	if (ret < 0) {
		WCN_ERR("can't enable wakeup src for bt_hostwake %d\n",
			bt_wake_host_gpio);
		return ret;
	}

	ret = request_irq(marlin_dev->bt_wake_host_int_num,
			  marlin_bt_wake_int_isr,
			  IRQF_TRIGGER_RISING |
			  IRQF_NO_SUSPEND,
			  "bt_wake_isr",
			  NULL);
	if (ret != 0) {
		WCN_ERR("req bt_hostwake irq-%d err! ret=%d",
			marlin_dev->bt_wake_host_int_num, ret);
		return ret;
	}
	disable_irq(marlin_dev->bt_wake_host_int_num);

	return 0;
}

static void marlin_unregistsr_bt_wake(void)
{
	disable_irq(marlin_dev->bt_wake_host_int_num);
	free_irq(marlin_dev->bt_wake_host_int_num, NULL);
}
#endif

static int marlin_parse_dt(struct platform_device *pdev)
{
#ifdef CONFIG_WCN_PARSE_DTS
	struct device_node *np = pdev->dev.of_node;
#elif defined CONFIG_AW_BOARD
	struct device_node *np = NULL;
#endif
#ifdef CONFIG_WCN_PMIC
	struct regmap *pmu_apb_gpr;
	struct wcn_clock_info *clk;
#endif
	int ret = -1;

	if (!marlin_dev)
		return ret;

#if defined(CONFIG_WCN_PARSE_DTS) && defined(CONFIG_WCN_PMIC)
	clk = &marlin_dev->clk_xtal_26m;
	clk->gpio = of_get_named_gpio(np, "xtal-26m-clk-type-gpio", 0);
	if (!gpio_is_valid(clk->gpio))
		WCN_INFO("xtal-26m-clk gpio not config\n");

	/* xtal-26m-clk-type has priority over than xtal-26m-clk-type-gpio */
	ret = of_property_read_string(np, "xtal-26m-clk-type",
					  (const char **)&buf);
	if (!ret) {
		WCN_INFO("force config xtal 26m clk %s\n", buf);
		if (!strncmp(buf, "TCXO", 4))
			clk->type = WCN_CLOCK_TYPE_TCXO;
		else if (!strncmp(buf, "TSX", 3))
			clk->type = WCN_CLOCK_TYPE_TSX;
		else
			WCN_ERR("force config xtal 26m clk %s err!\n", buf);
	} else {
		WCN_INFO("unforce config xtal 26m clk:%d", clk->type);
	}

	marlin_dev->dvdd12 = devm_regulator_get(&pdev->dev, "dvdd12");
	if (IS_ERR(marlin_dev->dvdd12)) {
		WCN_ERR("Get regulator of dvdd12 error!\n");
		WCN_ERR("Maybe share the power with mem\n");
	}

	if (of_property_read_bool(np, "bound-avdd12")) {
		WCN_INFO("forbid avdd12 power ctrl\n");
		marlin_dev->bound_avdd12 = true;
	} else {
		WCN_INFO("do avdd12 power ctrl\n");
		marlin_dev->bound_avdd12 = false;
	}

	marlin_dev->avdd12 = devm_regulator_get(&pdev->dev, "avdd12");
	if (IS_ERR(marlin_dev->avdd12)) {
		WCN_ERR("avdd12 err =%ld\n", PTR_ERR(marlin_dev->avdd12));
		if (PTR_ERR(marlin_dev->avdd12) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		WCN_ERR("Get regulator of avdd12 error!\n");
	}

	marlin_dev->avdd33 = devm_regulator_get(&pdev->dev, "avdd33");
	if (IS_ERR(marlin_dev->avdd33)) {
		if (PTR_ERR(marlin_dev->avdd33) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		WCN_ERR("Get regulator of avdd33 error!\n");
	}

	marlin_dev->dcxo18 = devm_regulator_get(&pdev->dev, "dcxo18");
	if (IS_ERR(marlin_dev->dcxo18)) {
		if (PTR_ERR(marlin_dev->dcxo18) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		WCN_ERR("Get regulator of dcxo18 error!\n");
	}

	if (of_property_read_bool(np, "bound-dcxo18")) {
		WCN_INFO("forbid dcxo18 power ctrl\n");
		marlin_dev->bound_dcxo18 = true;
	} else {
		WCN_INFO("do dcxo18 power ctrl\n");
		marlin_dev->bound_dcxo18 = false;
	}

	marlin_dev->clk_32k = devm_clk_get(&pdev->dev, "clk_32k");
	if (IS_ERR(marlin_dev->clk_32k)) {
		WCN_ERR("can't get wcn clock dts config: clk_32k\n");
		return -1;
	}

	marlin_dev->clk_parent = devm_clk_get(&pdev->dev, "source");
	if (IS_ERR(marlin_dev->clk_parent)) {
		WCN_ERR("can't get wcn clock dts config: source\n");
		return -1;
	}
	clk_set_parent(marlin_dev->clk_32k, marlin_dev->clk_parent);

	marlin_dev->clk_enable = devm_clk_get(&pdev->dev, "enable");
	if (IS_ERR(marlin_dev->clk_enable)) {
		WCN_ERR("can't get wcn clock dts config: enable\n");
		return -1;
	}
#endif /* end of CONFIG_WCN_PARSE_DTS and CONFIG_WCN_PMIC */

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->chip_en = of_get_named_gpio(np, "wl-reg-on", 0);
#else
	marlin_dev->chip_en = 0;
#endif
	if (marlin_dev->chip_en > 0) {
		WCN_INFO("%s chip_en gpio=%d\n", __func__,
			 marlin_dev->chip_en);
		if (!gpio_is_valid(marlin_dev->chip_en)) {
			WCN_ERR("chip_en gpio is invalid: %d\n",
				marlin_dev->chip_en);
			return -EINVAL;
		}
		ret = gpio_request(marlin_dev->chip_en, "chip_en");
		if (ret) {
			WCN_ERR("gpio chip_en request err: %d\n",
				marlin_dev->chip_en);
			marlin_dev->chip_en = 0;
		}
	}

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->reset = of_get_named_gpio(np, "bt-reg-on", 0);
#else
	marlin_dev->reset = 0;
#endif
	if (marlin_dev->reset > 0) {
		WCN_INFO("%s reset gpio=%d\n", __func__, marlin_dev->reset);
		if (!gpio_is_valid(marlin_dev->reset)) {
			WCN_ERR("reset gpio is invalid: %d\n",
				marlin_dev->reset);
			return -EINVAL;
		}
		ret = gpio_request(marlin_dev->reset, "reset");
		if (ret) {
			WCN_ERR("gpio reset request err: %d\n",
				marlin_dev->reset);
			marlin_dev->reset = 0;
		}
	}

#ifdef CONFIG_WCN_PARSE_DTS
	marlin_dev->int_ap = of_get_named_gpio(np, "pub-int-gpio", 0);
#else
	marlin_dev->int_ap = 0;
#endif
	if (marlin_dev->int_ap > 0) {
		WCN_INFO("%s int gpio=%d\n", __func__, marlin_dev->int_ap);
		if (!gpio_is_valid(marlin_dev->int_ap)) {
			WCN_ERR("int irq is invalid: %d\n",
				marlin_dev->int_ap);
			return -EINVAL;
		}
		ret = gpio_request(marlin_dev->int_ap, "int_ap");
		if (ret) {
			WCN_ERR("int_ap request err: %d\n",
				marlin_dev->int_ap);
			marlin_dev->int_ap = 0;
		}
	}

#ifdef CONFIG_WCN_PARSE_DTS
#ifdef CONFIG_WCN_PMIC
	if (gpio_is_valid(clk->gpio)) {
		ret = gpio_request(clk->gpio, "wcn_xtal_26m_type");
		if (ret)
			WCN_ERR("xtal 26m gpio request err: %d\n", ret);
	}
#endif

	ret = of_property_read_string(np, "unisoc,btwf-file-name",
					  (const char **)&marlin_dev->btwf_path);
	if (!ret)
		strcpy(BTWF_FIRMWARE_PATH, marlin_dev->btwf_path);
	else
		sprintf(BTWF_FIRMWARE_PATH, "%s%s", wcn_fw_path[0],
			WCN_FW_NAME);
	WCN_DEBUG("btwf firmware path is %s\n", BTWF_FIRMWARE_PATH);

	ret = of_property_read_string(np, "unisoc,gnss-file-name",
					  (const char **)&marlin_dev->gnss_path);
	if (!ret) {
		WCN_INFO("gnss firmware name:%s\n", marlin_dev->gnss_path);
		strcpy(GNSS_FIRMWARE_PATH, marlin_dev->gnss_path);
	}
#else
	sprintf(BTWF_FIRMWARE_PATH, "%s%s", wcn_fw_path[0], WCN_FW_NAME);
	WCN_DEBUG("btwf firmware path is %s\n", BTWF_FIRMWARE_PATH);
	sprintf(GNSS_FIRMWARE_PATH, "%s%s", wcn_fw_path[0], GNSS_FW_NAME);
	WCN_DEBUG("gnss firmware path is %s\n", GNSS_FIRMWARE_PATH);
#endif /* end of CONFIG_WCN_PARSE_DTS */

#ifdef CONFIG_WCN_PARSE_DTS
	if (of_property_read_bool(np, "keep-power-on")) {
		WCN_INFO("wcn config keep power on\n");
		marlin_dev->no_power_off = true;
	}
#else
#ifndef CONFIG_WCN_POWER_UP_DOWN
	WCN_INFO("wcn config keep power on\n");
	marlin_dev->no_power_off = true;
#endif
#endif

#ifdef CONFIG_WCN_PARSE_DTS
	if (of_property_read_bool(np, "bt-wake-host")) {
		int bt_wake_host_gpio;

		WCN_INFO("wcn config bt wake host\n");
		marlin_dev->bt_wl_wake_host_en |= BIT(BT_WAKE_HOST);
		bt_wake_host_gpio =
			of_get_named_gpio(np, "bt-wake-host-gpio", 0);
		WCN_INFO("%s bt-wake-host-gpio=%d\n", __func__,
			 bt_wake_host_gpio);
		if (!gpio_is_valid(bt_wake_host_gpio)) {
			WCN_ERR("int irq is invalid: %d\n",
				bt_wake_host_gpio);
			return -EINVAL;
		}
		ret = gpio_request(bt_wake_host_gpio, "bt-wake-host-gpio");
		if (ret)
			WCN_ERR("bt-wake-host-gpio request err: %d\n",
				bt_wake_host_gpio);
	}
#else
#ifdef CONFIG_BT_WAKE_HOST_EN
	WCN_INFO("wcn config bt wake host\n");
	marlin_dev->bt_wl_wake_host_en |= BIT(BT_WAKE_HOST);
#ifdef CONFIG_AW_BOARD
	ret = marlin_registsr_bt_wake(&pdev->dev);
	if (ret)
		return ret;
#endif /* end of CONFIG_AW_BOARD */
#endif /* end of CONFIG_BT_WAKE_HOST_EN */
#endif /* end of CONFIG_WCN_PARSE_DTS */

#ifdef CONFIG_WCN_PARSE_DTS
	if (of_property_read_bool(np, "wl-wake-host")) {
		int wl_wake_host_gpio;

		WCN_INFO("wcn config wifi wake host\n");
		marlin_dev->bt_wl_wake_host_en |= BIT(WL_WAKE_HOST);
		wl_wake_host_gpio =
			of_get_named_gpio(np, "wl-wake-host-gpio", 0);
		WCN_INFO("%s wl-wake-host-gpio=%d\n", __func__,
			 wl_wake_host_gpio);
		if (!gpio_is_valid(wl_wake_host_gpio)) {
			WCN_ERR("int irq is invalid: %d\n",
				wl_wake_host_gpio);
			return -EINVAL;
		}
		ret = gpio_request(wl_wake_host_gpio, "wl-wake-host-gpio");
		if (ret)
			WCN_ERR("wl-wake-host-gpio request err: %d\n",
				wl_wake_host_gpio);
	}
#else
#ifdef CONFIG_WL_WAKE_HOST_EN
	WCN_INFO("wcn config wifi wake host\n");
	marlin_dev->bt_wl_wake_host_en |= BIT(WL_WAKE_HOST);
#endif /* end of CONFIG_WL_WAKE_HOST_EN */
#endif /* end of CONFIG_WCN_PARSE_DTS */

#ifdef CONFIG_WCN_PARSE_DTS
	if (of_property_read_bool(np, "wait-ge2")) {
		WCN_INFO("wait-ge2 need wait gps ready\n");
		marlin_dev->wait_ge2 = true;
	}
#endif

#if defined(CONFIG_WCN_PARSE_DTS) && defined(CONFIG_WCN_PMIC)
	pmu_apb_gpr = syscon_regmap_lookup_by_phandle(np,
				"unisoc,syscon-pmu-apb");
	if (IS_ERR(pmu_apb_gpr)) {
		WCN_ERR("%s:failed to find pmu_apb_gpr(26M)(ignore)\n",
				__func__);
		return -EINVAL;
	}
	ret = regmap_read(pmu_apb_gpr, REG_PMU_APB_XTL_WAIT_CNT0,
					&clk_wait_val);
	WCN_INFO("marlin2 clk_wait value is 0x%x\n", clk_wait_val);

	ret = of_property_read_u32(np, "unisoc,reg-m2-apb-xtl-wait-addr",
			&marlin2_clk_wait_reg);
	if (ret) {
		WCN_ERR("Did not find reg-m2-apb-xtl-wait-addr\n");
		return -EINVAL;
	}
	WCN_INFO("marlin2 clk reg is 0x%x\n", marlin2_clk_wait_reg);
#endif

	return 0;
}

static int marlin_gpio_free(struct platform_device *pdev)
{
	if (!marlin_dev)
		return -1;

	if (marlin_dev->reset > 0)
		gpio_free(marlin_dev->reset);
	if (marlin_dev->chip_en > 0)
		gpio_free(marlin_dev->chip_en);
	if (marlin_dev->int_ap > 0)
		gpio_free(marlin_dev->int_ap);

	return 0;
}

static int marlin_clk_enable(bool enable)
{
	int ret = 0;

	return ret;

	if (enable) {
		ret = clk_prepare_enable(marlin_dev->clk_32k);
		ret = clk_prepare_enable(marlin_dev->clk_enable);
		WCN_INFO("marlin %s successfully!\n", __func__);
	} else {
		clk_disable_unprepare(marlin_dev->clk_enable);
		clk_disable_unprepare(marlin_dev->clk_32k);
	}

	return ret;
}

static int marlin_avdd18_dcxo_enable(bool enable)
{
	int ret = 0;

	return 0;

	WCN_INFO("avdd18_dcxo enable 1v8 %d\n", enable);
	if (marlin_dev->dcxo18 == NULL)
		return 0;

	if (enable) {
		if (regulator_is_enabled(marlin_dev->dcxo18))
			return 0;
		regulator_set_voltage(marlin_dev->dcxo18,
						  1800000, 1800000);
		ret = regulator_enable(marlin_dev->dcxo18);
		if (ret)
			WCN_ERR("fail to enable avdd18_dcxo\n");
	} else {
		if (regulator_is_enabled(marlin_dev->dcxo18)) {
			ret = regulator_disable(marlin_dev->dcxo18);
			if (ret)
				WCN_ERR("fail to disable avdd18_dcxo\n");
		}
	}

	return ret;
}

static int marlin_digital_power_enable(bool enable)
{
	int ret = 0;

	return ret;

	WCN_INFO("marlin_digital_power_enable D1v2 %d\n", enable);
	if (marlin_dev->dvdd12 == NULL)
		return 0;

	if (enable) {
		/* gpio_direction_output(marlin_dev->reset, 0); */

		regulator_set_voltage(marlin_dev->dvdd12,
						  1200000, 1200000);
		ret = regulator_enable(marlin_dev->dvdd12);
	} else {
		if (regulator_is_enabled(marlin_dev->dvdd12))
			ret = regulator_disable(marlin_dev->dvdd12);
	}

	return ret;
}
static int marlin_analog_power_enable(bool enable)
{
	int ret = 0;

	return ret;

	if (marlin_dev->avdd12 != NULL) {
		msleep(20);
		WCN_INFO("marlin_analog_power_enable 1v2 %d\n", enable);
		if (enable) {
			regulator_set_voltage(marlin_dev->avdd12,
			1200000, 1200000);
			ret = regulator_enable(marlin_dev->avdd12);
		} else {
			if (regulator_is_enabled(marlin_dev->avdd12))
				ret =
				regulator_disable(marlin_dev->avdd12);
		}
	}

	if (marlin_dev->avdd33_usb20 != NULL) {
		WCN_INFO(" avdd33_usb20 enable:%d\n", enable);
		if (enable) {
			regulator_set_voltage(marlin_dev->avdd33_usb20,
			3300000, 3300000);
			ret = regulator_enable(marlin_dev->avdd33_usb20);
		} else {
			if (regulator_is_enabled(marlin_dev->avdd33_usb20))
				ret =
				regulator_disable(marlin_dev->avdd33_usb20);
		}
	}

	return ret;
}

/*
 * hold cpu means cpu register is clear
 * different from reset pin gpio
 * reset gpio is all register is clear
 */
void marlin_hold_cpu(void)
{
	int ret = 0;
	unsigned int temp_reg_val;

	ret = sprdwcn_bus_reg_read(CP_RESET_REG, &temp_reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read reset reg error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s reset reg val:0x%x\n", __func__, temp_reg_val);
	temp_reg_val |= (RESET_BIT);
	ret = sprdwcn_bus_reg_write(CP_RESET_REG, &temp_reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write reset reg error:%d\n", __func__, ret);
		return;
	}
}

void marlin_read_cali_data(void)
{
	int err;

	WCN_INFO("marlin sync entry is_calibrated:%d\n",
		wifi_data.cali_data.cali_config.is_calibrated);

	if (!wifi_data.cali_data.cali_config.is_calibrated) {
		memset(&wifi_data.cali_data, 0x0,
			sizeof(struct wifi_cali_t));
		err = sprdwcn_bus_reg_read(CALI_OFSET_REG,
			&wifi_data.cali_data, sizeof(struct wifi_cali_t));
		if (err < 0) {
			WCN_ERR("marlin read cali data fail:%d\n", err);
			return;
		}
	}

	if ((marlin2_clk_wait_reg > 0) && (clk_wait_val > 0)) {
		sprdwcn_bus_reg_read(marlin2_clk_wait_reg,
					&cp_clk_wait_val, 4);
		WCN_INFO("marlin2 cp_clk_wait_val is 0x%x\n", cp_clk_wait_val);
		clk_wait_val = ((clk_wait_val & 0xFF00) >> 8);
		cp_clk_wait_val =
			((cp_clk_wait_val & 0xFFFFFC00) | clk_wait_val);
		WCN_INFO("marlin2 cp_clk_wait_val is modifyed 0x%x\n",
					cp_clk_wait_val);
		err = sprdwcn_bus_reg_write(marlin2_clk_wait_reg,
						   &cp_clk_wait_val, 4);
		if (err < 0)
			WCN_ERR("marlin2 write 26M error:%d\n", err);
	}

	/* write this flag to notify cp that ap read calibration data */
	reg_val = 0xbbbbbbbb;
	err = sprdwcn_bus_reg_write(CALI_REG, &reg_val, 4);
	if (err < 0) {
		WCN_ERR("marlin write cali finish error:%d\n", err);
		return;
	}

	sprdwcn_bus_runtime_get();

	//complete(&marlin_dev->download_done);
}

#ifdef CONFIG_WCN_SDIO
static void marlin_send_sdio_config_to_cp_vendor(void)
{
	union wcn_sdiohal_config sdio_cfg = {0};

#if (defined(CONFIG_HISI_BOARD) || defined(CONFIG_AML_BOARD) ||\
	defined(CONFIG_RK_BOARD) || defined(CONFIG_AW_BOARD))
	/* Vendor config */

	/* bit[0]: sdio_config_en:
	 * 0: disable sdio config
	 * 1: enable sdio config
	 */
	sdio_cfg.cfg.sdio_config_en = 1;

	/* bit[3:1]: vendor_id:
	 * 000: default id, unisoc[0x0]
	 * 001: hisilicon default version, pull chipen after resume
	 * 010: hisilicon version, keep power (NOT pull chipen) and
	 *      reset sdio after resume
	 */
#if defined(CONFIG_WCN_RESUME_POWER_DOWN)
	sdio_cfg.cfg.vendor_id = WCN_VENDOR_RESUME_POWER_DOWN;
	WCN_INFO("sdio_config vendor:[power down after resume]\n");
#elif defined(CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO)
	sdio_cfg.cfg.vendor_id = WCN_VENDOR_RESUME_KEEPPWR_RESETSDIO;
	WCN_INFO("sdio_config vendor:[keeppwr, reset sdio after resume]\n");
#else
	sdio_cfg.cfg.vendor_id = WCN_VENDOR_DEFAULT;
	WCN_DEBUG("sdio_config vendor:[default]\n");
#endif

	/* bit[4]: sdio_rx_mode: 0: adma; 1: sdma */
	if (sprdwcn_bus_get_rx_mode()) {
		sdio_cfg.cfg.sdio_rx_mode = 0;
		WCN_DEBUG("sdio_config rx mode:[adma]\n");
	} else {
		sdio_cfg.cfg.sdio_rx_mode = 1;
		WCN_INFO("sdio_config rx mode:[sdma]\n");
	}

	/* bit[7:5]: sdio_blk_size: 000: blocksize 840; 001: blocksize 512 */
	if (sprdwcn_bus_get_blk_size() == 512) {
		sdio_cfg.cfg.sdio_blk_size = 1;
		WCN_INFO("sdio_config blksize:[512]\n");
	} else
		WCN_DEBUG("sdio_config blksize:[840]\n");

	/*
	 * bit[8]: bt_wake_host_en: 0: disable, 1: enable
	 *
	 * When bit[8] is 1, bit[10:9] region will be parsed:
	 * bit[10:9]: bt_wake_host_trigger_type:
	 * 00:BT_WAKEUP_HOST  trigger type low
	 * 01:BT_WAKEUP_HOST  trigger type rising
	 * 10:BT_WAKEUP_HOST  trigger type falling
	 * 11:BT_WAKEUP_HOST  trigger type high
	 */
	if (marlin_get_bt_wl_wake_host_en() & BIT(BT_WAKE_HOST)) {
		sdio_cfg.cfg.bt_wake_host_en = 1;
		WCN_DEBUG("sdio_config bt_wake_host:[en]\n");
#if defined(CONFIG_HISI_BOARD)
		/*
		 * Hisi only support wakeup by:
		 * high level - low level for 200ms -high level
		 */
		sdio_cfg.cfg.bt_wake_host_trigger_type = 0;
		WCN_INFO("sdio_config bt_wake_host trigger:[low]\n");
#elif defined(CONFIG_AML_BOARD)
		if (wifi_irq_trigger_level() == GPIO_IRQ_LOW) {
			sdio_cfg.cfg.bt_wake_host_trigger_type = 0;
			WCN_INFO("sdio_config bt_wake_host trigger:[low]\n");
		} else {
			sdio_cfg.cfg.bt_wake_host_trigger_type = 3;
			WCN_INFO("sdio_config bt_wake_host trigger:[high]\n");
		}
#else
		sdio_cfg.cfg.bt_wake_host_trigger_type = 3;
		WCN_INFO("sdio_config bt_wake_host trigger:[high]\n");
#endif
	}

	/* bit[12:11]: sdio_irq_type:
	 * 00:dedicated irq, gpio1
	 * 01:inband data1 irq
	 * 10:use BT_WAKEUP_HOST(pubint) pin as gpio irq
	 * 11:use WL_WAKEUP_HOST(esmd3) pin as gpio irq
	 */
	if (sprdwcn_bus_get_irq_type() != 0) {
		sdio_cfg.cfg.sdio_irq_type = 1;
		WCN_INFO("sdio_config irq:[inband]\n");
	} else {
#ifdef CONFIG_CUSTOMIZE_SDIO_IRQ_TYPE
		sdio_cfg.cfg.sdio_irq_type = CONFIG_CUSTOMIZE_SDIO_IRQ_TYPE;
		if (sdio_cfg.cfg.sdio_irq_type == 0)
			WCN_INFO("sdio_config sdio_irq:[gpio1]\n");
		else if (sdio_cfg.cfg.sdio_irq_type == 2)
			WCN_INFO("sdio_config sdio_irq:[pubint]\n");
		else if (sdio_cfg.cfg.sdio_irq_type == 3)
			WCN_INFO("sdio_config sdio_irq:[esmd3]\n");
		else
			WCN_INFO("sdio_config sdio_irq:[error]\n");
#else
		sdio_cfg.cfg.sdio_irq_type = 0;
		WCN_INFO("sdio_config sdio_irq:[gpio1]\n");
#endif
	}

	/*
	 * When bit[12:11] is 10/11, bit[14:13] region will be parsed:
	 * bit[14:13]: sdio_irq_trigger_type:
	 * 00:pubint gpio irq trigger type low
	 * 01:pubint gpio irq trigger type rising [NOT support]
	 * 10:pubint gpio irq trigger type falling [NOT support]
	 * 11:pubint gpio irq trigger type high
	 */
	if (sprdwcn_bus_get_irq_type() == 0) {
#if defined(CONFIG_AML_BOARD)
		if (wifi_irq_trigger_level() == GPIO_IRQ_LOW) {
			sdio_cfg.cfg.sdio_irq_trigger_type = 0;
			WCN_INFO("sdio_config sdio_irq trigger:[low]\n");
		} else {
			sdio_cfg.cfg.sdio_irq_trigger_type = 3;
			WCN_INFO("sdio_config sdio_irq trigger:[high]\n");
		}
#endif
	}

	/*
	 * bit[15]: wl_wake_host_en: 0: disable, 1: enable
	 *
	 * When bit[15] is 1, bit[17:16] region will be parsed:
	 * bit[17:16]: wl_wake_host_trigger_type:
	 * 00:WL_WAKEUP_HOST  trigger type low
	 * 01:WL_WAKEUP_HOST  trigger type rising
	 * 10:WL_WAKEUP_HOST  trigger type falling
	 * 11:WL_WAKEUP_HOST  trigger type high
	 */
	if (marlin_get_bt_wl_wake_host_en() & BIT(WL_WAKE_HOST)) {
		sdio_cfg.cfg.wl_wake_host_en = 1;
		WCN_DEBUG("sdio_config wl_wake_host:[en]\n");
#if defined(CONFIG_HISI_BOARD)
		/*
		 * Hisi only support wakeup by:
		 * high level - low level for 200ms -high level
		 */
		sdio_cfg.cfg.wl_wake_host_trigger_type = 0;
		WCN_INFO("sdio_config wl_wake_host trigger:[low]\n");
#elif defined(CONFIG_AML_BOARD)
		if (wifi_irq_trigger_level() == GPIO_IRQ_LOW) {
			sdio_cfg.cfg.wl_wake_host_trigger_type = 0;
			WCN_INFO("sdio_config wl_wake_host trigger:[low]\n");
		} else {
			sdio_cfg.cfg.wl_wake_host_trigger_type = 3;
			WCN_INFO("sdio_config wl_wake_host trigger:[high]\n");
		}
#else
		sdio_cfg.cfg.wl_wake_host_trigger_type = 3;
		WCN_INFO("sdio_config wl_wake_host trigger:[high]\n");
#endif
	}

	/*
	 * bit[22:18]: wake_host_level_duration_10s: BT_WAKEUP_HOST or
	 * WL_WAKEUP_HOST level dyration time per 10ms,
	 * example: 0:0ms; 3:30ms; 20:200ms
	 */
#if defined(CONFIG_HISI_BOARD)
	/*
	 * Hisi only support wakeup by:
	 * high level - low level for 200ms -high level
	 */
	sdio_cfg.cfg.wake_host_level_duration_10ms = 20;
#elif defined(CONFIG_AML_BOARD)
	/* wakeup level for 30ms */
	sdio_cfg.cfg.wake_host_level_duration_10ms = 3;
#else
	sdio_cfg.cfg.wake_host_level_duration_10ms = 2;
#endif
	WCN_INFO("sdio_config wake_host_level_duration_time:[%dms]\n",
		 (sdio_cfg.cfg.wake_host_level_duration_10ms * 10));

	/*
	 * bit[23]: wake_host_data_separation:
	 * 0: if BT_WAKEUP_HOST en or WL_WAKEUP_HOST en,
	 *    wifi and bt packets can wake host;
	 * 1: if BT_WAKEUP_HOST en, ONLY bt packets can wake host;
	 *    if WL_WAKEUP_HOST en, ONLY wifi packets can wake host
	 */
#if (defined(CONFIG_AML_BOARD) || defined(CONFIG_RK_BOARD))
	sdio_cfg.cfg.wake_host_data_separation = 1;
	WCN_DEBUG("sdio_config wake_host_data_separation:[yes]\n");
#else
	WCN_INFO("sdio_config wake_host_data_separation:[bt/wifi reuse]\n");
#endif

#else
	/* Default config */
	sdio_cfg.val = 0;
#endif

	marlin_dev->sync_f.sdio_config = sdio_cfg.val;
}

static int marlin_send_sdio_config_to_cp(void)
{
	int sdio_config_off = 0;

	sdio_config_off = (unsigned long)(&(marlin_dev->sync_f.sdio_config)) -
		(unsigned long)(&(marlin_dev->sync_f));
	WCN_DEBUG("sdio_config_offset:0x%x\n", sdio_config_off);

	marlin_send_sdio_config_to_cp_vendor();

	WCN_INFO("%s sdio_config:0x%x (%sable config)\n",
		 __func__, marlin_dev->sync_f.sdio_config,
		 (marlin_dev->sync_f.sdio_config & BIT(0)) ? "en" : "dis");

	return sprdwcn_bus_reg_write(SYNC_ADDR + sdio_config_off,
				     &(marlin_dev->sync_f.sdio_config), 4);
}
#endif
static int marlin_write_cali_data(void)
{
	int ret = 0, init_state = 0, cali_data_offset = 0;
	int i = 0;

	//WCN_INFO("tsx_dac_data:%d\n", marlin_dev->tsxcali.tsxdata.dac);
	cali_data_offset = (unsigned long)(&(marlin_dev->sync_f.tsx_dac_data))-
		(unsigned long)(&(marlin_dev->sync_f));
	WCN_DEBUG("cali_data_offset:0x%x\n", cali_data_offset);

	do {
		i++;
		ret = sprdwcn_bus_reg_read(SYNC_ADDR, &init_state, 4);
		if (ret < 0) {
			WCN_ERR("%s marlin3 read SYNC_ADDR error:%d\n",
				__func__, ret);
			return ret;
		}
		WCN_INFO("%s sync init_state:0x%x\n", __func__, init_state);

		if (init_state != SYNC_CALI_WAITING)
			msleep(20);
		/* wait cp in the state of waiting cali data */
		else {
			/*write cali data to cp*/
#if 0 /* ott product cali data read from effuse, not ap */
			marlin_dev->sync_f.tsx_dac_data =
					marlin_dev->tsxcali.tsxdata.dac;
			ret = sprdwcn_bus_reg_write(SYNC_ADDR +
					cali_data_offset,
					&(marlin_dev->sync_f.tsx_dac_data), 4);
			if (ret < 0) {
				WCN_ERR("write cali data error:%d\n", ret);
				return ret;
			}
#endif
#ifdef CONFIG_WCN_SDIO
			/*write sdio config to cp*/
			ret = marlin_send_sdio_config_to_cp();
			if (ret < 0) {
				WCN_ERR("write sdio_config error:%d\n", ret);
				return ret;
			}
#endif
			/*tell cp2 can handle cali data*/
			init_state = SYNC_CALI_WRITE_DONE;
			ret = sprdwcn_bus_reg_write(SYNC_ADDR, &init_state, 4);
			if (ret < 0) {
				WCN_ERR("write cali_done flag error:%d\n", ret);
				return ret;
			}

			i = 0;
			WCN_INFO("marlin_write_cali_data finish\n");
			return ret;
		}

		if (i > 10)
			i = 0;
	} while (i);

	return ret;

}

enum wcn_clock_type wcn_get_xtal_26m_clk_type(void)
{
	return marlin_dev->clk_xtal_26m.type;
}
EXPORT_SYMBOL_GPL(wcn_get_xtal_26m_clk_type);

enum wcn_clock_mode wcn_get_xtal_26m_clk_mode(void)
{
	return marlin_dev->clk_xtal_26m.mode;
}
EXPORT_SYMBOL_GPL(wcn_get_xtal_26m_clk_mode);

static int spi_read_rf_reg(unsigned int addr, unsigned int *data)
{
	unsigned int reg_data = 0;
	int ret;

	reg_data = ((addr & 0x7fff) << 16) | SPI_BIT31;
	ret = sprdwcn_bus_reg_write(SPI_BASE_ADDR, &reg_data, 4);
	if (ret < 0) {
		WCN_ERR("write SPI RF reg error:%d\n", ret);
		return ret;
	}

	usleep_range(4000, 6000);

	ret = sprdwcn_bus_reg_read(SPI_BASE_ADDR, &reg_data, 4);
	if (ret < 0) {
		WCN_ERR("read SPI RF reg error:%d\n", ret);
		return ret;
	}
	*data = reg_data & 0xffff;

	return 0;
}

static int check_cp_clock_mode(void)
{
	int ret = 0;
	unsigned int temp_val;

	WCN_DEBUG("%s\n", __func__);

	ret = spi_read_rf_reg(AD_DCXO_BONDING_OPT, &temp_val);
	if (ret < 0) {
		WCN_ERR("read AD_DCXO_BONDING_OPT error:%d\n", ret);
		return ret;
	}
	WCN_DEBUG("read AD_DCXO_BONDING_OPT val:0x%x\n", temp_val);
	if ((temp_val & tsx_mode) == tsx_mode)
		WCN_INFO("clock mode: TSX\n");
	else {
		WCN_INFO("clock mode: TCXO, outside clock\n");
		//marlin_avdd18_dcxo_enable(false);
	}

	return ret;
}

/* release CPU */
static int marlin_start_run(void)
{
	int ret = 0;
	unsigned int ss_val;

	WCN_DEBUG("marlin_start_run\n");

	marlin_tsx_cali_data_get();
#ifdef CONFIG_WCN_SLP
	sdio_pub_int_btwf_en0();
	/* after chip power on, reset sleep status */
	slp_mgr_reset();
#endif

	ret = sprdwcn_bus_reg_read(CP_RESET_REG, &ss_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read reset reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s read reset reg val:0x%x\n", __func__, ss_val);
	ss_val  &= (~(RESET_BIT));
	WCN_INFO("after do %s reset reg val:0x%x\n", __func__, ss_val);
	ret = sprdwcn_bus_reg_write(CP_RESET_REG, &ss_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write reset reg error:%d\n", __func__, ret);
		return ret;
	}
	marlin_bootup_time_update();	/* update the time at once. */

	ret = sprdwcn_bus_reg_read(CP_RESET_REG, &ss_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read reset reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_DEBUG("%s reset reg val:0x%x\n", __func__, ss_val);

	return ret;
}

//#if IS_ENABLED(CONFIG_AW_BIND_VERIFY)
#include <crypto/sha2.h>

static void expand_seed(u8 *seed, u8 *out)
{
	unsigned char hash[64];
	int i;

	sha256(seed, 4, hash);

	for (i = 0; i < 4; i++) {
		memcpy(&out[i * 9], &hash[i * 8], 8);
		out[i * 9 + 8] = seed[i];
	}
}

static int wcn_bind_verify_calculate_verify_data(u8 *in, u8 *out)
{
	u8 seed[4], buf[36], a, b, c;
	int i, n;

	for (i = 0, n = 0; i < 4; i++) {
		a = in[n++];
		b = in[n++];
		c = in[n++];
		seed[i] = (a & b) ^ (a & c) ^ (b & c) ^ in[i + 12];
	}

	expand_seed(seed, buf);

	for (i = 0, n = 0; i < 12; i++) {
		a = buf[n++];
		b = buf[n++];
		c = buf[n++];
		out[i] = (a & b) ^ (a & c) ^ (b & c);
	}

	for (i = 0, n = 0; i < 4; i++) {
		a = out[n++];
		b = out[n++];
		c = out[n++];
		seed[i] = (a & b) ^ (~a & c);
	}

	expand_seed(seed, buf);

	for (i = 0, n = 0; i < 12; i++) {
		a = buf[n++];
		b = buf[n++];
		c = buf[n++];
		out[i] = (a & b) ^ (~a & c);
	}

	for (i = 0, n = 0; i < 4; i++) {
		a = out[n++];
		b = out[n++];
		c = out[n++];
		out[i + 12] = (a & b) ^ (a & c) ^ (b & c) ^ seed[i];
	}

	return 0;
}

static int marlin_bind_verify(void)
{
	unsigned char din[16], dout[16];
	int bind_verify_data_off = 0, init_state, ret = 0;

	/*transform confuse data to verify data*/
	memcpy(din, &marlin_dev->sync_f.bind_verify_data[0], 16);
	WCN_INFO("%s confuse data: 0x%02x%02x%02x%02x%02x%02x%02x%02x"
		 "%02x%02x%02x%02x%02x%02x%02x%02x\n", __func__,
		 din[0], din[1], din[2], din[3],
		 din[4], din[5], din[6], din[7],
		 din[8], din[9], din[10], din[11],
		 din[12], din[13], din[14], din[15]);
	wcn_bind_verify_calculate_verify_data(din, dout);
	WCN_INFO("%s verify data: 0x%02x%02x%02x%02x%02x%02x%02x%02x"
		 "%02x%02x%02x%02x%02x%02x%02x%02x\n", __func__,
		 dout[0], dout[1], dout[2], dout[3],
		 dout[4], dout[5], dout[6], dout[7],
		 dout[8], dout[9], dout[10], dout[11],
		 dout[12], dout[13], dout[14], dout[15]);

	/*send bind verify data to cp2*/
	memcpy(&marlin_dev->sync_f.bind_verify_data[0], dout, 16);
	bind_verify_data_off = (unsigned long)
		(&(marlin_dev->sync_f.bind_verify_data[0])) -
		(unsigned long)(&(marlin_dev->sync_f));
	ret = sprdwcn_bus_direct_write(SYNC_ADDR + bind_verify_data_off,
		&(marlin_dev->sync_f.bind_verify_data[0]), 16);
	if (ret < 0) {
		WCN_ERR("write bind verify data error:%d\n", ret);
		return ret;
	}

	/*tell cp2 can handle bind verify data*/
	init_state = SYNC_VERIFY_WRITE_DONE;
	ret = sprdwcn_bus_reg_write(SYNC_ADDR, &init_state, 4);
	if (ret < 0) {
		WCN_ERR("write bind verify flag error:%d\n", ret);
		return ret;
	}

	return ret;
}
//#endif

static int check_cp_ready(void)
{
	int ret = 0;
	int i = 0;

#ifdef CONFIG_WCN_USB
	return sprdwcn_check_cp_ready(SYNC_ADDR, 3000);
#endif

	do {
		i++;
		ret = sprdwcn_bus_direct_read(SYNC_ADDR,
			&(marlin_dev->sync_f), sizeof(struct wcn_sync_info_t));
		if (ret < 0) {
			WCN_ERR("%s marlin3 read SYNC_ADDR error:%d\n",
				__func__, ret);
			return ret;
		}
		WCN_INFO("%s sync val:0x%x, prj_type val:0x%x\n", __func__,
				marlin_dev->sync_f.init_status,
				marlin_dev->sync_f.prj_type);
		if (marlin_dev->sync_f.init_status == SYNC_ALL_FINISHED)
			i = 0;
//#ifdef CONFIG_AW_BIND_VERIFY
		else if (marlin_dev->sync_f.init_status ==
			SYNC_VERIFY_WAITING) {
			ret = marlin_bind_verify();
			if (ret != 0) {
				WCN_ERR("%s bind verify error:%d\n",
					__func__, ret);
				return ret;
			}
		}
//#endif
		else
			msleep(20);
		if (i > 10)
			return -1;
	} while (i);

	return 0;
}
static int gnss_start_run(void)
{
	int ret = 0;
	unsigned int temp;

	WCN_INFO("gnss start run enter ");
#ifdef CONFIG_WCN_SLP
	sdio_pub_int_gnss_en0();
#endif
	ret = sprdwcn_bus_reg_read(GNSS_CP_RESET_REG, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s marlin3_gnss read reset reg error:%d\n",
			__func__, ret);
		return ret;
	}
	WCN_INFO("%s reset reg val:0x%x\n", __func__, temp);
	temp &= (~0) - 1;
	ret = sprdwcn_bus_reg_write(GNSS_CP_RESET_REG, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s marlin3_gnss write reset reg error:%d\n",
				__func__, ret);
		return ret;
	}

	return ret;
}

#if defined CONFIG_UWE5623 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_WCN_SDIO
static int marlin_reset_by_128_bit(void)
{
	unsigned char reg;

	WCN_INFO("%s entry\n", __func__);
	if (sprdwcn_bus_aon_readb(REG_CP_RST_CHIP, &reg)) {
		WCN_ERR("%s line:%d\n", __func__, __LINE__);
		return -1;
	}

	reg |= 1;
	if (sprdwcn_bus_aon_writeb(REG_CP_RST_CHIP, reg)) {
		WCN_ERR("%s line:%d\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}
#endif
#endif

#ifdef CONFIG_HISI_BOARD
static unsigned int hi_gpio_set_value(unsigned int gpio, unsigned int value)
{
	int status;

	WCN_INFO("%s entry\n", __func__);

	status = HI_DRV_GPIO_SetDirBit(gpio, HI_DIR_OUT);
	if (status != HI_SUCCESS) {
		WCN_ERR("gpio(%d) HI_DRV_GPIO_SetDirBit HI_DIR_OUT failed\n",
			gpio);
		return status;
	}
	mdelay(RESET_DELAY);
	status = HI_DRV_GPIO_WriteBit(gpio, value);
	if (status != HI_SUCCESS) {
		WCN_ERR("gpio(%d) HI_DRV_GPIO_WriteBit value(%d) failed\n",
			gpio, value);
		return status;
	}

	return HI_SUCCESS;
}
#endif

static int marlin_reset(int val)
{
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5623
	#ifdef CONFIG_WCN_SDIO
	marlin_reset_by_128_bit();
	#endif
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E)
		marlin_reset_by_128_bit();
#endif

#ifdef CONFIG_HISI_BOARD
	/* As for Hisi platform, repull reset pin to reset wcn chip. */
	hi_gpio_set_value(RTL_REG_RST_GPIO, 0);
	mdelay(RESET_DELAY);
	hi_gpio_set_value(RTL_REG_RST_GPIO, 1);
#endif

	if (marlin_dev->reset > 0) {
		if (gpio_is_valid(marlin_dev->reset)) {
			gpio_direction_output(marlin_dev->reset, 0);
			mdelay(RESET_DELAY);
			gpio_direction_output(marlin_dev->reset, 1);
		}
	}

	return 0;
}

static int chip_reset_release(int val)
{
	static unsigned int reset_count;

#if (defined(CONFIG_AML_BOARD) && defined(CONFIG_WCN_RESET_PIN_CONNECTED))
	/* As for amlogic platform,
	 * 1. chipen pin connected with WL_EN pin, reset pin connected with
	 * BT_EN pin. But amlogic iptv source code won't pull BT_EN(reset) pin.
	 * Unisoc module will change reset pin NC defaultly. This means reset
	 * pin will be disconnected with BT_EN pin.
	 * 2. BT wake host will judge both BT_EN pin and BT_WAKE_HOST pin.
	 */
	if (val) {
		if (reset_count == 0)
			extern_bt_set_enable(1);
		reset_count++;
	} else {
		extern_bt_set_enable(0);
		reset_count--;
	}
#else
	if (marlin_dev->reset <= 0)
		return 0;

	if (!gpio_is_valid(marlin_dev->reset)) {
		WCN_ERR("reset gpio error\n");
		return -1;
	}
	if (val) {
		if (reset_count == 0)
			gpio_direction_output(marlin_dev->reset, 1);
		reset_count++;
	} else {
		gpio_direction_output(marlin_dev->reset, 0);
		reset_count--;
	}
#endif

	return 0;
}
void marlin_chip_en(bool enable, bool reset)
{
	static unsigned int chip_en_count;

#ifdef CONFIG_AML_BOARD
	if (enable) {
		if (chip_en_count == 0) {
			extern_wifi_set_enable(0);
			msleep(100);
			extern_wifi_set_enable(1);
			WCN_INFO("marlin chip en pull up\n");
		}
		chip_en_count++;
	} else {
		chip_en_count--;
		if (chip_en_count == 0) {
			extern_wifi_set_enable(0);
			WCN_INFO("marlin chip en pull down\n");
		}
	}
	return;
#endif

#ifdef CONFIG_AW_BOARD
	if (enable) {
		if (chip_en_count == 0) {
			msleep(100);
			WCN_INFO("marlin chip en dummy pull up -- need manually set GPIO \n");
		}
		chip_en_count++;
	} else {
		chip_en_count--;
		if (chip_en_count == 0) {
			WCN_INFO("marlin chip en dummy pull down -- need manually set GPIO \n");
		}
	}
	return;
#endif

	/*
	 * Incar board pull chipen gpio at pin control.
	 * Hisi board pull chipen gpio at hi_sdio_detect.ko.
	 */
	if (marlin_dev->chip_en <= 0)
		return;

	if (gpio_is_valid(marlin_dev->chip_en)) {
		if (reset) {
			gpio_direction_output(marlin_dev->chip_en, 0);
			WCN_INFO("marlin gnss chip en reset\n");
			msleep(100);
			gpio_direction_output(marlin_dev->chip_en, 1);
		} else if (enable) {
			if (chip_en_count == 0) {
				gpio_direction_output(marlin_dev->chip_en, 0);
				mdelay(1);
				gpio_direction_output(marlin_dev->chip_en, 1);
				mdelay(1);
				WCN_INFO("marlin chip en pull up\n");
			}
			chip_en_count++;
		} else {
			chip_en_count--;
			if (chip_en_count == 0) {
				gpio_direction_output(marlin_dev->chip_en, 0);
				WCN_INFO("marlin chip en pull down\n");
			}
		}
	}
}
EXPORT_SYMBOL_GPL(marlin_chip_en);

int set_cp_mem_status(int subsys, int val)
{
	int ret;
	unsigned int temp_val;

	return 0;

	ret = sprdwcn_bus_reg_read(REG_WIFI_MEM_CFG1, &temp_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read wifimem_cfg1 error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s read btram poweron(bit22)val:0x%x\n", __func__, temp_val);

	if ((subsys == MARLIN_BLUETOOTH) && (val == 1)) {
		temp_val = temp_val & (~FORCE_SHUTDOWN_BTRAM);
		WCN_INFO("wr btram poweron(bit22) val:0x%x\n", temp_val);
		ret = sprdwcn_bus_reg_write(REG_WIFI_MEM_CFG1, &temp_val, 4);
		if (ret < 0) {
			WCN_ERR("write wifimem_cfg1 reg error:%d\n", ret);
			return ret;
		}
		return 0;
	} else if (test_bit(MARLIN_BLUETOOTH, &marlin_dev->power_state) &&
		   (subsys != MARLIN_BLUETOOTH))
		return 0;

	temp_val = temp_val | FORCE_SHUTDOWN_BTRAM;
	WCN_INFO(" shut down btram(bit22) val:0x%x\n", temp_val);
	ret = sprdwcn_bus_reg_write(REG_WIFI_MEM_CFG1, &temp_val, 4);
	if (ret < 0) {
		WCN_ERR("write wifimem_cfg1 reg error:%d\n", ret);
		return ret;
	}

	return ret;
}

int enable_spur_remove(void)
{
	int ret;
	unsigned int temp_val;

	temp_val = FM_ENABLE_SPUR_REMOVE_FREQ2_VALUE;
	ret = sprdwcn_bus_reg_write(FM_REG_SPUR_FEQ1_ADDR, &temp_val, 4);
	if (ret < 0) {
		WCN_ERR("write FM_REG_SPUR reg error:%d\n", ret);
		return ret;
	}

	return 0;
}

int disable_spur_remove(void)
{
	int ret;
	unsigned int temp_val;

	temp_val = FM_DISABLE_SPUR_REMOVE_VALUE;
	ret = sprdwcn_bus_reg_write(FM_REG_SPUR_FEQ1_ADDR, &temp_val, 4);
	if (ret < 0) {
		WCN_ERR("write disable FM_REG_SPUR reg error:%d\n", ret);
		return ret;
	}

	return 0;
}

void set_fm_supe_freq(int subsys, int val, unsigned long sub_state)
{
	switch (subsys) {
	case MARLIN_FM:
		if (test_bit(MARLIN_GNSS, &sub_state) && (val == 1))
			enable_spur_remove();
		else
			disable_spur_remove();
		break;
	case MARLIN_GNSS:
		if (test_bit(MARLIN_FM, &sub_state) && (val == 1))
			enable_spur_remove();
		else
			disable_spur_remove();
		break;
	default:
		break;
	}
}

/*
 * MARLIN_GNSS no need loopcheck action
 * MARLIN_AUTO no need loopcheck action
 */
static void power_state_notify_or_not(int subsys, int poweron)
{
#ifndef CONFIG_WCN_LOOPCHECK
	return;
#endif

	if (poweron == 1) {
		set_cp_mem_status(subsys, poweron);
#ifdef CONFIG_WCN_FM
		set_fm_supe_freq(subsys, poweron, marlin_dev->power_state);
#endif
	}

	if ((test_bit(MARLIN_BLUETOOTH, &marlin_dev->power_state) +
	      test_bit(MARLIN_FM, &marlin_dev->power_state) +
	      test_bit(MARLIN_WIFI, &marlin_dev->power_state) +
	      test_bit(MARLIN_MDBG, &marlin_dev->power_state)) == 1) {
#ifdef CONFIG_WCN_LOOPCHECK
		WCN_DEBUG("only one module open, need to notify loopcheck\n");
		start_loopcheck();
#endif
		marlin_dev->loopcheck_status_change = 1;
		wakeup_loopcheck_int();
	}

	if (((marlin_dev->power_state) & MARLIN_MASK) == 0) {
#ifdef CONFIG_WCN_LOOPCHECK
		WCN_DEBUG("marlin close, need to notify loopcheck\n");
		stop_loopcheck();
#endif
		marlin_dev->loopcheck_status_change = 1;
		wakeup_loopcheck_int();

	}
}

static void marlin_scan_finish(void)
{
	WCN_INFO("marlin_scan_finish!\n");
	complete(&marlin_dev->carddetect_done);
}

int find_firmware_path(void)
{
	int ret;
	int pre_len;
#ifdef CONFIG_WCN_GNSS
	if ((strlen(BTWF_FIRMWARE_PATH) != 0) || (strlen(GNSS_FIRMWARE_PATH) != 0))
#else
	if (strlen(BTWF_FIRMWARE_PATH) != 0)
#endif
		return 0;

	ret = parse_firmware_path(BTWF_FIRMWARE_PATH);
	if (ret != 0) {
		WCN_ERR("can not find wcn partition\n");
		return ret;
	}
	WCN_INFO("BTWF path is %s\n", BTWF_FIRMWARE_PATH);
	pre_len = strlen(BTWF_FIRMWARE_PATH) - strlen("wcnmodem");
	memcpy(GNSS_FIRMWARE_PATH, BTWF_FIRMWARE_PATH, strlen(BTWF_FIRMWARE_PATH));
	memcpy(&GNSS_FIRMWARE_PATH[pre_len], "gnssmodem", strlen("gnssmodem"));
	GNSS_FIRMWARE_PATH[pre_len + strlen("gnssmodem")] = '\0';
	WCN_INFO("GNSS path is %s\n", GNSS_FIRMWARE_PATH);

	return 0;
}

static void pre_gnss_download_firmware(struct work_struct *work)
{
	static int cali_flag;
	int ret = 0;

	/* ./fstab.xxx is prevent for user space progress */
	if (marlin_dev->first_power_on_flag == 1)
		if (find_firmware_path() < 0)
			return;

	if (gnss_download_firmware() != 0) {
		WCN_ERR("gnss download firmware fail\n");
		return;
	}

	if (gnss_ops && (gnss_ops->write_data)) {
		if (gnss_ops->write_data() != 0)
			return;
	} else
		WCN_ERR("%s gnss_ops write_data error\n", __func__);

	if (gnss_start_run() != 0)
		WCN_ERR("gnss start run fail\n");

	if (cali_flag == 0) {
		WCN_INFO("gnss start to backup calidata\n");
		if (gnss_ops && gnss_ops->backup_data) {
			ret = gnss_ops->backup_data();
			if (ret == 0)
				cali_flag = 1;
		} else
			WCN_ERR("%s gnss_ops backup_data error\n", __func__);
	} else {
		WCN_INFO("gnss wait boot finish\n");
		if (gnss_ops && gnss_ops->wait_gnss_boot)
			gnss_ops->wait_gnss_boot();
		else
			WCN_ERR("%s gnss_ops wait boot error\n", __func__);

	}
	complete(&marlin_dev->gnss_download_done);	/* temp */

}

#if defined CONFIG_WCN_USB && defined CONFIG_SYS_REBOOT_NOT_REPOWER_USBCHIP
static unsigned char fdl_hex_buf[] = {
#include "../fw/usb_fdl.bin.hex"
};

#define FDL_HEX_SIZE sizeof(fdl_hex_buf)

static int wcn_usb_fdl_download(void)
{
	int ret;
	struct marlin_firmware *firmware;

	firmware = kmalloc(sizeof(struct marlin_firmware), GFP_KERNEL);
	if (!firmware)
		return -ENOMEM;

	WCN_INFO("marlin %s from wcnmodem.bin.hex start!\n", __func__);
	firmware->data = fdl_hex_buf;
	firmware->size = FDL_HEX_SIZE;
	firmware->is_from_fs = 0;
	firmware->priv = fdl_hex_buf;

	ret = marlin_firmware_parse_image(firmware);
	if (ret) {
		WCN_ERR("%s firmware parse AA\\AB error\n", __func__);
		goto OUT;
	}

	ret = marlin_firmware_write(firmware);
	if (ret) {
		WCN_ERR("%s firmware write error\n", __func__);
		goto OUT;
	}
OUT:
	marlin_release_firmware(firmware);

	return ret;
}

static void btwifi_download_fdl_firmware(void)
{
	int ret;

	marlin_firmware_download_start_usb();
	wcn_get_chip_name();

	if (wcn_usb_fdl_download()) {
		WCN_INFO("fdl download err\n");
		return;
	}
	msleep(100);

	init_completion(&marlin_dev->carddetect_done);
	marlin_reset(true);
	mdelay(1);

	ret = wait_for_completion_timeout(&marlin_dev->carddetect_done,
		msecs_to_jiffies(CARD_DETECT_WAIT_MS));
	if (ret == 0) {
		WCN_ERR("first wait scan error!\n");
		return;
	}
}
#endif

static void pre_btwifi_download_sdio(struct work_struct *work)
{
#ifdef CONFIG_WCN_USB
#ifdef CONFIG_SYS_REBOOT_NOT_REPOWER_USBCHIP
	/*
	 * Fix Bug 1349945.
	 * Because the usb vbus can't be controlled on some platforms,
	 * So, BT WIFI can't work after ap sys reboot, the reason is cp state is
	 * not rebooted. So, wen need pull reset pin to reset cp.
	 * But on cp init, set the reset_hold reg to keep iram for dump mem,
	 * it's lead to the chip can't reset cache and power state. So that,
	 * the chip can't work normal.
	 * To solve this problem, we need a fdl to clear the reset_hold reg
	 * before re-reset. After clear the reset_hold reg, then reset chip
	 * again and normal boot system.
	 */
	btwifi_download_fdl_firmware();
#endif
	marlin_firmware_download_start_usb();
#endif
	wcn_get_chip_name();

	if (btwifi_download_firmware() == 0 &&
		marlin_start_run() == 0) {
		check_cp_clock_mode();
		marlin_write_cali_data();
		/* check_cp_ready must be in front of mem_pd_save_bin,
		 * save bin task is scheduled after verify.
		 */
		if (check_cp_ready() != 0) {
			sprdwcn_bus_set_carddump_status(true);
			return;
		}
#ifdef CONFIG_MEM_PD
		mem_pd_save_bin();
#endif
#ifdef CONFIG_CPLOG_DEBUG
		wcn_debug_init();
#endif
		sprdwcn_bus_runtime_get();

#ifndef CONFIG_WCND
		get_cp2_version();
#ifndef CONFIG_CPLOG_DEBUG
		switch_cp2_log(false);
#endif
#endif
		complete(&marlin_dev->download_done);
	}
}

/* for example: wifipa bound XTLEN3 */
int pmic_bound_xtl_assert(unsigned int enable)
{
#if defined(CONFIG_WCN_PMIC) && !defined(CONFIG_WCN_PCIE)
	unsigned int val;

	regmap_read(reg_map, ANA_REG_GLB_LDO_XTL_EN10, &val);
	WCN_INFO("%s:%d, XTL_EN10 =0x%x\n", __func__, enable, val);
	regmap_update_bits(reg_map,
			   ANA_REG_GLB_LDO_XTL_EN10,
			   BIT_LDO_WIFIPA_EXT_XTL3_EN,
			   enable);
	regmap_read(reg_map, ANA_REG_GLB_LDO_XTL_EN10, &val);
	WCN_INFO("after XTL_EN10 =0x%x\n", val);
#endif
	return 0;
}

void wifipa_enable(int enable)
{
	int ret = -1;

	return;

	if (marlin_dev->avdd33) {
		WCN_INFO("marlin_analog_power_enable 3v3 %d\n", enable);
		msleep(20);
		if (enable) {
			if (regulator_is_enabled(marlin_dev->avdd33))
				return;
			regulator_set_voltage(marlin_dev->avdd33,
						  3300000, 3300000);
			ret = regulator_enable(marlin_dev->avdd33);
			if (ret)
				WCN_ERR("fail to enable wifipa\n");
		} else {
			if (regulator_is_enabled(marlin_dev->avdd33)) {
				ret =
				regulator_disable(marlin_dev->avdd33);
				if (ret)
					WCN_ERR("fail to disable wifipa\n");
				WCN_INFO(" wifi pa disable\n");
			}
		}
	}
}


void set_wifipa_status(int subsys, int val)
{
	return;

	if (val == 1) {
		if (((subsys == MARLIN_BLUETOOTH) || (subsys == MARLIN_WIFI)) &&
		    ((marlin_dev->power_state & 0x5) == 0)) {
			wifipa_enable(1);
			pmic_bound_xtl_assert(1);
		}

		if (((subsys != MARLIN_BLUETOOTH) && (subsys != MARLIN_WIFI)) &&
		    ((marlin_dev->power_state & 0x5) == 0)) {
			wifipa_enable(0);
			pmic_bound_xtl_assert(0);
		}

	} else {
		if (((subsys == MARLIN_BLUETOOTH) &&
		     ((marlin_dev->power_state & 0x4) == 0)) ||
		    ((subsys == MARLIN_WIFI) &&
		     ((marlin_dev->power_state & 0x1) == 0))) {
			wifipa_enable(0);
			pmic_bound_xtl_assert(0);
		}
	}
}

/*
 * RST_N (LOW)
 * VDDIO -> DVDD12/11 ->CHIP_EN ->DVDD_CORE(inner)
 * ->(>=550uS) RST_N (HIGH)
 * ->(>=100uS) ADVV12
 * ->(>=10uS)  AVDD33
 */
int chip_power_on(int subsys)
{
	WCN_DEBUG("%s\n", __func__);

#ifndef CONFIG_WCN_PCIE
	/* may be we can call reinit_completion api */
	init_completion(&marlin_dev->carddetect_done);
#endif
	marlin_avdd18_dcxo_enable(true);
	marlin_clk_enable(true);
	marlin_digital_power_enable(true);
	marlin_chip_en(true, false);
	msleep(20);
	chip_reset_release(1);
	marlin_analog_power_enable(true);
	wifipa_enable(1);
	sprdwcn_bus_driver_register();


#ifndef CONFIG_WCN_PCIE
	sprdwcn_bus_rescan();
	if (wait_for_completion_timeout(&marlin_dev->carddetect_done,
		msecs_to_jiffies(CARD_DETECT_WAIT_MS)) == 0) {
		WCN_ERR("wait SDIO rescan card time out\n");
		return -1;
	}
	loopcheck_first_boot_set();
#ifdef CONFIG_MEM_PD
	mem_pd_poweroff_deinit();
#endif
#ifdef CONFIG_WCN_SDIO
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_poweron(true);
#endif
#endif

	return 0;
}

int chip_power_off(int subsys)
{
	WCN_INFO("%s\n", __func__);

	sprdwcn_bus_driver_unregister();
	marlin_avdd18_dcxo_enable(false);
	marlin_clk_enable(false);
	marlin_chip_en(false, false);
	marlin_digital_power_enable(false);
	marlin_analog_power_enable(false);
	chip_reset_release(0);
	marlin_dev->wifi_need_download_ini_flag = 0;
	marlin_dev->power_state = 0;
#ifndef CONFIG_WCN_PCIE
#ifdef CONFIG_MEM_PD
	mem_pd_poweroff_deinit();
#endif
	sprdwcn_bus_remove_card();
#endif
	loopcheck_first_boot_clear();
#ifdef CONFIG_WCN_SDIO
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_poweron(false);
#endif

	return 0;
}

int gnss_powerdomain_open(void)
{
	/* add by this. */
	int ret = 0, retry_cnt = 0;
	unsigned int temp;

#ifndef CONFIG_WCN_GNSS
	return 0;
#endif

	WCN_INFO("%s\n", __func__);

	ret = sprdwcn_bus_reg_read(CGM_GNSS_FAKE_CFG, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s read CGM_GNSS_FAKE_CFG error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s CGM_GNSS_FAKE_CFG:0x%x\n", __func__, temp);
	temp = temp & (~(CGM_GNSS_FAKE_SEL));
	ret = sprdwcn_bus_reg_write(CGM_GNSS_FAKE_CFG, &temp, 4);
	if (ret < 0) {
		WCN_ERR("write CGM_GNSS_FAKE_CFG err:%d\n", ret);
		return ret;
	}

	ret = sprdwcn_bus_reg_read(PD_GNSS_SS_AON_CFG4, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s read PD_GNSS_SS_AON_CFG4 err:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s PD_GNSS_SS_AON_CFG4:0x%x\n", __func__, temp);
	temp = temp & (~(FORCE_DEEP_SLEEP));
	WCN_INFO("%s PD_GNSS_SS_AON_CFG4:0x%x\n", __func__, temp);
	ret = sprdwcn_bus_reg_write(PD_GNSS_SS_AON_CFG4, &temp, 4);
	if (ret < 0) {
		WCN_ERR("write PD_GNSS_SS_AON_CFG4 err:%d\n", ret);
		return ret;
	}

	/* wait gnss sys power on finish */
	do {
		usleep_range(3000, 6000);

		ret = sprdwcn_bus_reg_read(CHIP_SLP_REG, &temp, 4);
		if (ret < 0) {
			WCN_ERR("%s read CHIP_SLP_REG err:%d\n", __func__, ret);
			return ret;
		}

		WCN_INFO("%s CHIP_SLP:0x%x,bit12,13 need 1\n", __func__, temp);
		retry_cnt++;
	} while ((!(temp & GNSS_SS_PWRON_FINISH)) &&
		 (!(temp & GNSS_PWR_FINISH)) && (retry_cnt < 3));

	return 0;
}

int gnss_powerdomain_close(void)
{
	/* add by this. */
	int ret = 0;
	unsigned int temp;

#ifndef CONFIG_WCN_GNSS
	return 0;
#endif

	WCN_INFO("%s\n", __func__);

	ret = sprdwcn_bus_reg_read(PD_GNSS_SS_AON_CFG4, &temp, 4);
	if (ret < 0) {
		WCN_ERR("read PD_GNSS_SS_AON_CFG4 err:%d\n", ret);
		return ret;
	}
	WCN_INFO("%s PD_GNSS_SS_AON_CFG4:0x%x\n", __func__, temp);
	temp = (temp | FORCE_DEEP_SLEEP | PD_AUTO_EN) &
		(~(CHIP_DEEP_SLP_EN));
	WCN_INFO("%s PD_GNSS_SS_AON_CFG4:0x%x\n", __func__, temp);
	ret = sprdwcn_bus_reg_write(PD_GNSS_SS_AON_CFG4, &temp, 4);
	if (ret < 0) {
		WCN_ERR("write PD_GNSS_SS_AON_CFG4 err:%d\n", ret);
		return ret;
	}

	return 0;
}

int open_power_ctl(void)
{
	marlin_dev->no_power_off = 0;
	clear_bit(WCN_AUTO, &marlin_dev->power_state);

	return 0;
}
EXPORT_SYMBOL_GPL(open_power_ctl);

void marlin_schedule_download_wq(void)
{
	unsigned long timeleft;

	marlin_dev->wifi_need_download_ini_flag = 0;
	schedule_work(&marlin_dev->download_wq);
	timeleft = wait_for_completion_timeout(
		&marlin_dev->download_done,
		msecs_to_jiffies(POWERUP_WAIT_MS));
	if (!timeleft) {
		WCN_ERR("marlin download timeout\n");
	}

}

static int marlin_set_power(int subsys, int val)
{
	unsigned long timeleft;

	WCN_DEBUG("mutex_lock\n");
	mutex_lock(&marlin_dev->power_lock);

	if (marlin_dev->wait_ge2) {
		first_call_flag++;
		if (first_call_flag == 1) {
			WCN_INFO("(marlin2+ge2)waiting ge2 download finish\n");
			timeleft
				= wait_for_completion_timeout(
				&ge2_completion, 12*HZ);
			if (!timeleft)
				WCN_ERR("wait ge2 timeout\n");
		}
		first_call_flag = 2;
	}

	WCN_INFO("marlin power state:%lx, subsys: [%s] power %d\n",
			marlin_dev->power_state, strno(subsys), val);
	init_completion(&marlin_dev->download_done);
	init_completion(&marlin_dev->gnss_download_done);

	/*  power on */
	if (val) {
		/* 1. when the first open:
		 * `- first download gnss, and then download btwifi
		 */
		marlin_dev->first_power_on_flag++;
		if (marlin_dev->first_power_on_flag == 1) {
			WCN_INFO("the first power on start\n");
			if (chip_power_on(subsys) < 0) {
				WCN_ERR("chip power on fail\n");
				goto out;
			}
			set_wifipa_status(subsys, val);
			set_bit(subsys, &marlin_dev->power_state);
#ifdef CONFIG_WCN_GNSS
			WCN_INFO("GNSS start to auto download\n");
			schedule_work(&marlin_dev->gnss_dl_wq);
			timeleft
				= wait_for_completion_timeout(
				&marlin_dev->gnss_download_done, 10*HZ);
			if (!timeleft) {
				WCN_ERR("GNSS download timeout\n");
				goto out;
			}
			WCN_INFO("gnss auto download finished and run ok\n");
#endif
			WCN_INFO("then marlin start to download\n");
			schedule_work(&marlin_dev->download_wq);
			timeleft = wait_for_completion_timeout(
				&marlin_dev->download_done,
				msecs_to_jiffies(POWERUP_WAIT_MS));
			if (!timeleft) {
				WCN_ERR("marlin download timeout\n");
				goto out;
			}
			marlin_dev->download_finish_flag = 1;
			WCN_INFO("then marlin download finished and run ok\n");
			marlin_dev->first_power_on_flag = 2;
			WCN_DEBUG("mutex_unlock\n");
			mutex_unlock(&marlin_dev->power_lock);
			power_state_notify_or_not(subsys, val);
#ifdef CONFIG_WCN_GNSS
			if (subsys == WCN_AUTO) {
				marlin_set_power(WCN_AUTO, false);
				return 0;
			}
#endif
			if (subsys == MARLIN_GNSS) {
				marlin_set_power(MARLIN_GNSS, false);
				marlin_set_power(MARLIN_GNSS, true);
				return 0;
			}
			return 0;
		}
		/* 2. the second time, WCN_AUTO coming */
		else if (subsys == WCN_AUTO) {
			if (marlin_dev->no_power_off) {
				WCN_INFO("have power on, no action\n");
				set_wifipa_status(subsys, val);
				set_bit(subsys, &marlin_dev->power_state);
			}

			else {

				WCN_INFO("!1st,not to bkup gnss cal, no act\n");
			}
		}

		/* 3. when GNSS open,
		 *	  |- GNSS and MARLIN have power on and ready
		 */
		else if ((((marlin_dev->power_state) & AUTO_RUN_MASK) != 0)
			|| (((marlin_dev->power_state) & GNSS_MASK) != 0)) {
			WCN_INFO("GNSS and marlin have ready\n");
			if (((marlin_dev->power_state) & MARLIN_MASK) == 0)
				loopcheck_first_boot_set();
			set_wifipa_status(subsys, val);
			set_bit(subsys, &marlin_dev->power_state);

			goto check_power_state_notify;
		}
		/* 4. when GNSS close, marlin open.
		 *	  ->  subsys=gps,GNSS download
		 */
		else if (((marlin_dev->power_state) & MARLIN_MASK) != 0) {
			if ((subsys == MARLIN_GNSS) || (subsys == WCN_AUTO)) {
				WCN_INFO("BTWF ready, GPS start to download\n");
				set_wifipa_status(subsys, val);
				set_bit(subsys, &marlin_dev->power_state);
#ifdef CONFIG_WCN_GNSS
				gnss_powerdomain_open();

				schedule_work(&marlin_dev->gnss_dl_wq);
				timeleft = wait_for_completion_timeout(
					&marlin_dev->gnss_download_done, 10*HZ);
				if (!timeleft) {
					WCN_ERR("GNSS download timeout\n");
					goto out;
				}
#endif
				WCN_INFO("GNSS download finished and ok\n");

			} else {
				WCN_INFO("marlin have open, GNSS is closed\n");
				set_wifipa_status(subsys, val);
				set_bit(subsys, &marlin_dev->power_state);

				goto check_power_state_notify;
			}
		}
		/* 5. when GNSS close, marlin close.no module to power on */
		else {
			WCN_INFO("no module to power on, start to power on\n");
			if (chip_power_on(subsys) < 0) {
				WCN_ERR("chip power on fail\n");
				goto out;
			}
			set_wifipa_status(subsys, val);
			set_bit(subsys, &marlin_dev->power_state);

			/* 5.1 first download marlin, and then download gnss */
			if ((subsys == WCN_AUTO || subsys == MARLIN_GNSS)) {
				WCN_INFO("marlin start to download\n");
				schedule_work(&marlin_dev->download_wq);
				timeleft = wait_for_completion_timeout(
					&marlin_dev->download_done,
					msecs_to_jiffies(POWERUP_WAIT_MS));
				if (!timeleft) {
					WCN_ERR("marlin download timeout\n");
					goto out;
				}
				marlin_dev->download_finish_flag = 1;

				WCN_INFO("marlin dl finished and run ok\n");
#ifdef CONFIG_WCN_GNSS
				WCN_INFO("GNSS start to download\n");
				schedule_work(&marlin_dev->gnss_dl_wq);
				timeleft = wait_for_completion_timeout(
					&marlin_dev->gnss_download_done, 10*HZ);
				if (!timeleft) {
					WCN_ERR("then GNSS download timeout\n");
					goto out;
				}
				WCN_INFO("then gnss dl finished and ok\n");
#endif

			}
			/* 5.2 only download marlin, and then
			 * close gnss power domain
			 */
			else {
				WCN_INFO("only marlin start to download\n");
				schedule_work(&marlin_dev->download_wq);
				if (wait_for_completion_timeout(
					&marlin_dev->download_done,
					msecs_to_jiffies(POWERUP_WAIT_MS))
					<= 0) {

					WCN_ERR("marlin download timeout\n");
					goto out;
				}
				marlin_dev->download_finish_flag = 1;
				WCN_INFO("BTWF download finished and run ok\n");
#ifdef CONFIG_WCN_GNSS
				gnss_powerdomain_close();
#endif
			}
		}
		/* power on together's Action */
		power_state_notify_or_not(subsys, val);

		WCN_INFO("wcn chip power on and run finish: [%s]\n",
				  strno(subsys));
	/* power off */
	} else {
		if (marlin_dev->power_state == 0)
			goto check_power_state_notify;

		if (flag_reset)
			marlin_dev->power_state = 0;

		if (marlin_dev->no_power_off) {
			if (!flag_reset) {
				if (subsys != WCN_AUTO) {
					/* in order to not download again */
					set_bit(WCN_AUTO,
						&marlin_dev->power_state);
					clear_bit(subsys,
						&marlin_dev->power_state);
				}

				MDBG_LOG("marlin reset flag_reset:%d\n",
					flag_reset);

				goto check_power_state_notify;
			}
		}

		if (!marlin_dev->download_finish_flag)
			goto check_power_state_notify;

		set_wifipa_status(subsys, val);
		clear_bit(subsys, &marlin_dev->power_state);
		if (marlin_dev->power_state != 0) {
			WCN_INFO("can not power off, other module is on\n");
			if (subsys == MARLIN_GNSS)
				gnss_powerdomain_close();

			goto check_power_state_notify;
		}

		set_cp_mem_status(subsys, val);
#ifdef CONFIG_WCN_FM
		set_fm_supe_freq(subsys, val, marlin_dev->power_state);
#endif
		power_state_notify_or_not(subsys, val);

		WCN_INFO("wcn chip start power off!\n");
		sprdwcn_bus_runtime_put();
		chip_power_off(subsys);
		WCN_INFO("marlin power off!\n");
		marlin_dev->download_finish_flag = 0;
		if (flag_reset)
			flag_reset = FALSE;
	} /* power off end */

	/* power on off together's Action */
	WCN_DEBUG("mutex_unlock\n");
	mutex_unlock(&marlin_dev->power_lock);

	return 0;

out:
	sprdwcn_bus_runtime_put();
#ifdef CONFIG_MEM_PD
	mem_pd_poweroff_deinit();
#endif
	sprdwcn_bus_driver_unregister();
	marlin_clk_enable(false);
	marlin_digital_power_enable(false);
	marlin_analog_power_enable(false);
	chip_reset_release(0);
	marlin_dev->power_state = 0;
	WCN_DEBUG("mutex_unlock\n");
	mutex_unlock(&marlin_dev->power_lock);

	return -1;

check_power_state_notify:
	power_state_notify_or_not(subsys, val);
	WCN_DEBUG("mutex_unlock\n");
	mutex_unlock(&marlin_dev->power_lock);

	return 0;
}

void marlin_power_off(enum marlin_sub_sys subsys)
{
	WCN_INFO("%s all\n", __func__);

	marlin_dev->no_power_off = false;
	set_bit(subsys, &marlin_dev->power_state);
	marlin_set_power(subsys, false);
}

int marlin_get_power(enum marlin_sub_sys subsys)
{
	if (subsys == MARLIN_ALL)
		return marlin_dev->power_state != 0;
	else
		return test_bit(subsys, &marlin_dev->power_state);
}
EXPORT_SYMBOL_GPL(marlin_get_power);

bool marlin_get_download_status(void)
{
	return marlin_dev->download_finish_flag;
}
EXPORT_SYMBOL_GPL(marlin_get_download_status);

int wcn_get_module_status_changed(void)
{
	return marlin_dev->loopcheck_status_change;
}
EXPORT_SYMBOL_GPL(wcn_get_module_status_changed);

void wcn_set_module_status_changed(bool status)
{
	marlin_dev->loopcheck_status_change = status;
}
EXPORT_SYMBOL_GPL(wcn_set_module_status_changed);

int marlin_get_module_status(void)
{
	if (test_bit(MARLIN_BLUETOOTH, &marlin_dev->power_state) ||
		test_bit(MARLIN_FM, &marlin_dev->power_state) ||
		test_bit(MARLIN_WIFI, &marlin_dev->power_state) ||
		test_bit(MARLIN_MDBG, &marlin_dev->power_state) ||
		test_bit(MARLIN_GNSS, &marlin_dev->power_state) ||
		test_bit(WCN_AUTO, &marlin_dev->power_state))
		return 1;
	return 0;
}
EXPORT_SYMBOL_GPL(marlin_get_module_status);

int is_first_power_on(enum marlin_sub_sys subsys)
{
	if (marlin_dev->wifi_need_download_ini_flag == 1)
		return 1;	/* the first */
	else
		return 0;	/* not the first */
}
EXPORT_SYMBOL_GPL(is_first_power_on);

int cali_ini_need_download(enum marlin_sub_sys subsys)
{
	unsigned int pd_wifi_st = 0;

#ifdef CONFIG_AW_BOARD
	/*FIX SPCSS00757820, wifi-bt on/off frequently & quickly, ini need download but not*/
	return 1;
#endif

#ifdef CONFIG_MEM_PD
	pd_wifi_st = mem_pd_wifi_state();
#endif
	if ((marlin_dev->wifi_need_download_ini_flag == 1) || pd_wifi_st) {
		WCN_INFO("cali_ini_need_download return 1\n");
		return 1;	/* the first */
	}
	return 0;	/* not the first */
}
EXPORT_SYMBOL_GPL(cali_ini_need_download);

int marlin_set_wakeup(enum marlin_sub_sys subsys)
{
	int ret = 0;	/* temp */

	return 0;
	if (unlikely(marlin_dev->download_finish_flag != true))
		return -1;

	return ret;
}
EXPORT_SYMBOL_GPL(marlin_set_wakeup);

int marlin_set_sleep(enum marlin_sub_sys subsys, bool enable)
{
	return 0;	/* temp */

	if (unlikely(marlin_dev->download_finish_flag != true))
		return -1;

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_set_sleep);

/* Temporary modification for UWE5623:
 * cmd52 read/write timeout -110 issue.
 */
void marlin_read_test_after_reset(void)
{
	int ret;
	unsigned int reg_addr = AON_APB_TEST_READ_REG, reg_val;

	ret = sprdwcn_bus_reg_read(reg_addr, &reg_val, 4);
	if (ret < 0)
		WCN_ERR("%s read 0x%x error:%d\n", __func__, reg_addr, ret);
	else
		WCN_INFO("%s read 0x%x = 0x%x\n", __func__, reg_addr, reg_val);
}

int marlin_reset_reg(void)
{
	int ret;

	/* may be we can call reinit_completion api */
	init_completion(&marlin_dev->carddetect_done);
	marlin_reset(true);
	mdelay(1);
	sprdwcn_bus_rescan();

	ret = wait_for_completion_timeout(&marlin_dev->carddetect_done,
		msecs_to_jiffies(CARD_DETECT_WAIT_MS));
	if (ret == 0) {
		WCN_ERR("marlin reset reg wait scan error!\n");
		ret = -1;
	}

	#ifdef CONFIG_WCN_SDIO
	/* Temporary modification for UWE5623:
	 * cmd52 read/write timeout -110 issue.
	 */
	marlin_read_test_after_reset();
	#endif
	return ret;
}
EXPORT_SYMBOL_GPL(marlin_reset_reg);

int start_marlin(u32 subsys)
{
#ifdef CONFIG_WCN_PCIE
	WCN_INFO("%s [%s],power_status=%ld\n", __func__, strno(subsys),
		 marlin_dev->power_state);
	if (marlin_dev->download_finish_flag == 1) {
		WCN_INFO("firmware have download\n");
		return 0;
	}
	set_bit(subsys, &marlin_dev->power_state);
	pcie_boot(subsys);
	marlin_dev->download_finish_flag = 1;

	return 0;
#else
	WCN_INFO("%s [%s]\n", __func__, strno(subsys));
	if (sprdwcn_bus_get_carddump_status() != 0) {
		WCN_ERR("%s SDIO card dump\n", __func__);
		return -1;
	}

	if (get_loopcheck_status()) {
		WCN_ERR("%s loopcheck status is fail\n", __func__);
		return -1;
	}

	if (subsys == MARLIN_WIFI) {
		/* not need write cali */
		if (marlin_dev->wifi_need_download_ini_flag == 0)
			/* need write cali */
			marlin_dev->wifi_need_download_ini_flag = 1;
		else
			/* not need write cali */
			marlin_dev->wifi_need_download_ini_flag = 2;
	}
	marlin_set_power(subsys, true);

#ifdef CONFIG_MEM_PD
	return mem_pd_mgr(subsys, true);
#else
	return 0;
#endif
#endif
}
EXPORT_SYMBOL_GPL(start_marlin);

int stop_marlin(u32 subsys)
{
#ifndef CONFIG_WCN_PCIE
	if (sprdwcn_bus_get_carddump_status() != 0) {
		WCN_ERR("%s SDIO card dump\n", __func__);
		return -1;
	}

	if (get_loopcheck_status()) {
		WCN_ERR("%s loopcheck status is fail\n", __func__);
		return -1;
	}

#ifdef CONFIG_MEM_PD
	mem_pd_mgr(subsys, false);
#endif

	return marlin_set_power(subsys, false);
#else
	clear_bit(subsys, &marlin_dev->power_state);
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(stop_marlin);

static void marlin_power_wq(struct work_struct *work)
{
	WCN_INFO("%s start\n", __func__);

	/* WCN_AUTO is for auto backup gnss cali data */
	marlin_set_power(WCN_AUTO, true);

}
static void marlin_reset_notify_init(void);
static int marlin_probe(struct platform_device *pdev)
{
#ifdef CONFIG_WCN_PMIC
	struct device_node *regmap_np;
	struct platform_device *pdev_regmap = NULL;
#endif

	marlin_dev = devm_kzalloc(&pdev->dev,
			sizeof(struct marlin_device), GFP_KERNEL);
	if (!marlin_dev)
		return -ENOMEM;
	marlin_dev->write_buffer = devm_kzalloc(&pdev->dev,
			PACKET_SIZE, GFP_KERNEL);
	if (marlin_dev->write_buffer == NULL) {
		devm_kfree(&pdev->dev, marlin_dev);
		WCN_ERR("marlin_probe write buffer low memory\n");
		return -ENOMEM;
	}
	mutex_init(&(marlin_dev->power_lock));
	marlin_dev->power_state = 0;
	if (marlin_parse_dt(pdev) < 0)
		WCN_INFO("marlin2 parse_dt some para not config\n");
	if (marlin_dev->reset > 0) {
		if (gpio_is_valid(marlin_dev->reset))
			gpio_direction_output(marlin_dev->reset, 0);
	}
	init_completion(&ge2_completion);
	init_completion(&marlin_dev->carddetect_done);
#ifdef CONFIG_WCN_SLP
	slp_mgr_init();
#endif
	/* register ops */
	wcn_bus_init();
	sprdwcn_bus_register_rescan_cb(marlin_scan_finish);
	/* sdiom_init or pcie_init */
	sprdwcn_bus_preinit();
#ifndef CONFIG_WCN_PCIE
#ifdef CONFIG_WCN_SDIO
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_init(marlin_dev->int_ap);
#endif
#ifdef CONFIG_MEM_PD
	mem_pd_init();
#endif
	proc_fs_init();
	log_dev_init();
	mdbg_atcmd_owner_init();
	wcn_op_init();
#endif
#ifndef CONFIG_WCND
	loopcheck_init();
#endif

	flag_reset = 0;
	/*notify subsys do reset, when cp2 was dead*/
	marlin_reset_notify_init();

#ifdef CONFIG_WCN_PCIE
	chip_power_on(WCN_AUTO);
#endif

	INIT_WORK(&marlin_dev->download_wq, pre_btwifi_download_sdio);
	INIT_WORK(&marlin_dev->gnss_dl_wq, pre_gnss_download_firmware);
	INIT_DELAYED_WORK(&marlin_dev->power_wq, marlin_power_wq);
#if 0
	schedule_delayed_work(&marlin_dev->power_wq,
				  msecs_to_jiffies(POWER_WQ_DELAYED_MS));
#endif

#ifdef CONFIG_WCN_PMIC
	regmap_np = of_find_compatible_node(NULL, NULL, "sprd,pmic-glb");
	if (!regmap_np) {
		WCN_ERR("get pmic glb node fail\n");
		return -ENODEV;
	}

	pdev_regmap = of_find_device_by_node(regmap_np);
	if (!pdev_regmap) {
		of_node_put(regmap_np);
		WCN_ERR("get pmic device node fail\n");
		return -ENODEV;
	}

	reg_map = dev_get_regmap(pdev_regmap->dev.parent, NULL);
	if (!reg_map) {
		WCN_ERR("get regmap error\n");
		of_node_put(regmap_np);
		return PTR_ERR(reg_map);
	}
#endif

	WCN_INFO("marlin_probe ok!\n");

	return 0;
}

static void marlin_remove(struct platform_device *pdev)
{
#if (defined(CONFIG_BT_WAKE_HOST_EN) && defined(CONFIG_AW_BOARD))
	marlin_unregistsr_bt_wake();
#endif
	cancel_work_sync(&marlin_dev->download_wq);
	cancel_work_sync(&marlin_dev->gnss_dl_wq);
	cancel_delayed_work_sync(&marlin_dev->power_wq);
#ifndef CONFIG_WCND
	loopcheck_deinit();
#endif
	wcn_op_exit();
	mdbg_atcmd_owner_deinit();
	log_dev_exit();
	proc_fs_exit();
#ifdef CONFIG_WCN_SDIO
	if (marlin_dev->int_ap > 0)
		sdio_pub_int_deinit();
#endif
#ifdef CONFIG_MEM_PD
	mem_pd_exit();
#endif
	sprdwcn_bus_deinit();
	if ((marlin_dev->power_state != 0) && (!marlin_dev->no_power_off)) {
		WCN_INFO("marlin some subsys power is on, warning!\n");
		wifipa_enable(0);
		pmic_bound_xtl_assert(0);
		marlin_chip_en(false, false);
	}
	wcn_bus_deinit();
#ifdef CONFIG_WCN_SLP
	slp_mgr_deinit();
#endif
	marlin_gpio_free(pdev);
	mutex_destroy(&marlin_dev->power_lock);
	vfree(marlin_dev->firmware.data);
	devm_kfree(&pdev->dev, marlin_dev->write_buffer);
	devm_kfree(&pdev->dev, marlin_dev);

	WCN_INFO("marlin_remove ok!\n");

	return;
}

static void marlin_shutdown(struct platform_device *pdev)
{
	/* When the following three conditions are met at the same time,
	 * wcn chip will be powered off:
	 * 1. chip has been powered on (power_state is not 0);
	 * 2. config power up and down (not keep power on);
	 * 3. bt/wifi wake host is disabled.
	 */
	if ((marlin_dev->power_state != 0) && (!marlin_dev->no_power_off) &&
		(!marlin_get_bt_wl_wake_host_en())) {
		WCN_INFO("marlin some subsys power is on, warning!\n");
		wifipa_enable(0);
		pmic_bound_xtl_assert(0);
		marlin_chip_en(false, false);
	}

#if (defined(CONFIG_HISI_BOARD) && defined(CONFIG_WCN_USB))
	/* As for Hisi platform, repull reset pin to reset wcn chip. */
	hi_gpio_set_value(RTL_REG_RST_GPIO, 0);
	mdelay(RESET_DELAY);
	hi_gpio_set_value(RTL_REG_RST_GPIO, 1);
#endif
	WCN_INFO("marlin_shutdown end\n");
}

static int marlin_suspend(struct device *dev)
{

	WCN_INFO("[%s]enter\n", __func__);
#if (defined(CONFIG_BT_WAKE_HOST_EN) && defined(CONFIG_AW_BOARD))
	/* enable wcn wake host irq. */
	marlin_bt_wake_int_en();
#endif

	return 0;
}

int marlin_reset_register_notify(void *callback_func, void *para)
{
	marlin_reset_func = (marlin_reset_callback)callback_func;
	marlin_callback_para = para;

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_reset_register_notify);

int marlin_reset_unregister_notify(void)
{
	marlin_reset_func = NULL;
	marlin_callback_para = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(marlin_reset_unregister_notify);

static void marlin_reset_notify_init(void)
{
	int i = 0;
	for (i = 0; i < MARLIN_ALL; i++)
		 RAW_INIT_NOTIFIER_HEAD(&marlin_reset_notifiers[i]);
}

/**
 * @return: is notify callback function return value
*/
int marlin_reset_notify_call(enum marlin_cp2_status sts)
{

	int i = 0;
	for (i = 0; i < MARLIN_ALL; i++) {
		if (NULL != marlin_reset_notifiers[i].head)
			raw_notifier_call_chain(&marlin_reset_notifiers[i], sts, (void *)strno(i));
	}
	return 0;
}
EXPORT_SYMBOL_GPL(marlin_reset_notify_call);

int marlin_reset_callback_register(u32 subsys, struct notifier_block *nb)
{
	return raw_notifier_chain_register(&marlin_reset_notifiers[subsys], nb);
}
EXPORT_SYMBOL_GPL(marlin_reset_callback_register);

void marlin_reset_callback_unregister(u32 subsys, struct notifier_block *nb)
{
	int ret = 0;
	ret = raw_notifier_chain_unregister(&marlin_reset_notifiers[subsys], nb);
	if (ret)
		WCN_ERR("%s is not registered for reset notification\n", strno(subsys));
}
EXPORT_SYMBOL_GPL(marlin_reset_callback_unregister);

static int marlin_resume(struct device *dev)
{
	WCN_INFO("[%s]enter\n", __func__);
#if (defined(CONFIG_BT_WAKE_HOST_EN) && defined(CONFIG_AW_BOARD))
	/* disable wcn wake host irq. */
	marlin_bt_wake_int_dis();
#endif

	return 0;
}

static const struct dev_pm_ops marlin_pm_ops = {
	.suspend = marlin_suspend,
	.resume	= marlin_resume,
};

static const struct of_device_id marlin_match_table[] = {
	{.compatible = "unisoc,uwe_bsp",},
	{ },
};

static struct platform_driver marlin_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "uwe_bsp",
		.pm = &marlin_pm_ops,
#ifdef CONFIG_WCN_PARSE_DTS
		.of_match_table = marlin_match_table,
#endif
	},
	.probe = marlin_probe,
	.remove = marlin_remove,
	.shutdown = marlin_shutdown,
};

#ifndef CONFIG_WCN_PARSE_DTS
static void uwe_release(struct device *dev)
{
	WCN_INFO("[%s]enter\n", __func__);
}

static struct platform_device uwe_device = {
	.name = "uwe_bsp",
	.dev = {
		.release = uwe_release,
	}
};

#ifdef CONFIG_WCN_GNSS
static void gnss_common_ctl_release(struct device *dev)
{
	WCN_INFO("[%s]enter\n", __func__);
}
static struct platform_device gnss_common_ctl_device = {
	.name = "gnss_common_ctl",
	.dev = {
		.release = gnss_common_ctl_release,
	}
};
#endif
#endif

#ifdef CONFIG_WCN_GNSS
extern int __init gnss_common_ctl_init(void);
extern void __exit gnss_common_ctl_exit(void);
extern int __init gnss_pmnotify_ctl_init(void);
extern void __exit gnss_pmnotify_ctl_cleanup(void);
extern int __init gnss_module_init(void);
extern void __exit gnss_module_exit(void);
#endif
static int __init marlin_init(void)
{
	WCN_INFO("marlin_init entry!\n");

#ifndef CONFIG_WCN_PARSE_DTS
	platform_device_register(&uwe_device);
#endif
#ifdef CONFIG_WCN_GNSS
	platform_device_register(&gnss_common_ctl_device);
	gnss_common_ctl_init();
	gnss_pmnotify_ctl_init();
	gnss_module_init();
#endif
	return platform_driver_register(&marlin_driver);
}
#ifdef CONFIG_WCN_BSP_DRIVER_BUILDIN
late_initcall(marlin_init);
#else
module_init(marlin_init);
#endif

static void __exit marlin_exit(void)
{
	WCN_INFO("marlin_exit entry!\n");

#ifndef CONFIG_WCN_PARSE_DTS
	platform_device_unregister(&uwe_device);
#endif
#ifdef CONFIG_WCN_GNSS
	gnss_common_ctl_exit();
	gnss_pmnotify_ctl_cleanup();
	gnss_module_exit();
	platform_device_register(&gnss_common_ctl_device);
#endif
	platform_driver_unregister(&marlin_driver);

	WCN_INFO("marlin_exit end!\n");
}
module_exit(marlin_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Spreadtrum  WCN Marlin Driver");
MODULE_AUTHOR("Yufeng Yang <yufeng.yang@spreadtrum.com>");
