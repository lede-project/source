/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
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
#include <linux/version.h>
#include "linux/gnss.h"

#include "wcn_misc.h"
#include "wcn_glb.h"
#include "wcn_glb_reg.h"
#include "wcn_gnss.h"
#include "wcn_procfs.h"


static struct mutex marlin_lock;
static struct wifi_calibration wifi_data;

/* efuse data */
static const u32
s_wifi_efuse_id[WCN_PLATFORM_TYPE][WIFI_EFUSE_BLOCK_COUNT] = {
	{41, 42, 43},	/* SharkLE */
	{38, 39, 40},	/* PIKE2 */
	{41, 42, 43},	/* SharkL3 */
};
static const u32 s_wifi_efuse_default_value[WIFI_EFUSE_BLOCK_COUNT] = {
0x11111111, 0x22222222, 0x33333333};	/* Until now, the value is error */

static char gnss_firmware_parent_path[FIRMWARE_FILEPATHNAME_LENGTH_MAX];
static char firmware_file_name[FIRMWARE_FILEPATHNAME_LENGTH_MAX];
static  u32 s_gnss_efuse_id[GNSS_EFUSE_BLOCK_COUNT] = {40, 42, 43};

struct sprdwcn_gnss_ops *gnss_ops;

void wcn_boot_init(void)
{
	mutex_init(&marlin_lock);
}

void wcn_device_poweroff(void)
{
	u32 i;

	mutex_lock(&marlin_lock);

	for (i = 0; i < WCN_MARLIN_ALL; i++)
		stop_integrate_wcn_truely(i);

	if (marlin_reset_func != NULL)
		marlin_reset_func(marlin_callback_para);

	mutex_unlock(&marlin_lock);
	WCN_INFO("all subsys power off finish!\n");
}

static int wcn_get_firmware_path(char *firmwarename, char *firmware_path)
{
	if (firmwarename == NULL || firmware_path == NULL)
		return -EINVAL;

	memset(firmware_path, 0, FIRMWARE_FILEPATHNAME_LENGTH_MAX);
	if (strcmp(firmwarename, WCN_MARLIN_DEV_NAME) == 0) {
		if (parse_firmware_path(firmware_path))
			return -EINVAL;
	} else if (strcmp(firmwarename, WCN_GNSS_DEV_NAME) == 0) {
		int folder_path_length = 0;
		/*
		 * GNSS firmware path is the same as BTWF
		 * But the function parse_firmware_path return path
		 * includes filename of wcnmodem
		 */
		if (parse_firmware_path(firmware_path))
			return -EINVAL;
		folder_path_length = strlen(firmware_path)
			-strlen(WCN_BTWF_FILENAME);
		*(firmware_path + folder_path_length) = 0;
		strcpy(gnss_firmware_parent_path, firmware_path);

	} else
		return -EINVAL;

	WCN_INFO("wcn_dev->firmware_path:%s\n",
		firmware_path);

	return 0;
}

/*judge status of sbuf until timeout*/
static void wcn_sbuf_status(u8 dst, u8 channel)
{
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(1000);
	while (1) {
		if (!sbuf_status(dst, channel))
			break;
		else if (time_after(jiffies, timeout)) {
			WCN_INFO("channel %d-%d is not ready!\n",
				 dst, channel);
			break;
		}
		msleep(20);
	}
}

/* only wifi need it */
static void marlin_write_cali_data(void)
{
	phys_addr_t phy_addr;
	u32 cali_flag;

	/* get cali para from RF */
	get_connectivity_config_param(&wifi_data.config_data);
	get_connectivity_cali_param(&wifi_data.cali_data);

	/* copy calibration file data to target ddr address */
	phy_addr = s_wcn_device.btwf_device->base_addr +
		   (phys_addr_t)&s_wssm_phy_offset_p->wifi.calibration_data;
	wcn_write_data_to_phy_addr(phy_addr, &wifi_data, sizeof(wifi_data));

	/* notify CP to cali */
	cali_flag = WIFI_CALIBRATION_FLAG_VALUE;
	phy_addr = s_wcn_device.btwf_device->base_addr +
		   (phys_addr_t)&s_wssm_phy_offset_p->wifi.calibration_flag;
	wcn_write_data_to_phy_addr(phy_addr, &cali_flag, sizeof(cali_flag));

	WCN_INFO("finish\n");
}

/* only wifi need it: AP save the cali data to ini file */
static void marlin_save_cali_data(void)
{
	phys_addr_t phy_addr;

	if (s_wcn_device.btwf_device) {
		memset(&wifi_data.cali_data, 0x0,
					sizeof(struct wifi_cali_t));
		/* copy calibration file data to target ddr address */
		phy_addr = s_wcn_device.btwf_device->base_addr +
			   (phys_addr_t)&s_wssm_phy_offset_p->
			   wifi.calibration_data +
			   sizeof(struct wifi_config_t);
		wcn_read_data_from_phy_addr(phy_addr, &wifi_data.cali_data,
					    sizeof(struct wifi_cali_t));
		dump_cali_file(&wifi_data.cali_data);
		WCN_INFO("finish\n");
	}
}

/* only wifi need it */
static void marlin_write_efuse_data(void)
{
	phys_addr_t phy_addr;
	u32 iloop = 0;
	u32 tmp_value[WIFI_EFUSE_BLOCK_COUNT];
	u32 chip_type;

	chip_type = wcn_platform_chip_type();
	/* get data from Efuse */
	memset(&tmp_value, 0, sizeof(tmp_value[0])*WIFI_EFUSE_BLOCK_COUNT);
	for (iloop = 0; iloop < WIFI_EFUSE_BLOCK_COUNT; iloop++) {
		tmp_value[iloop] =
		sprd_efuse_double_read(
				s_wifi_efuse_id[chip_type][iloop], true);
		if (tmp_value[iloop] == 0)
			tmp_value[iloop] = s_wifi_efuse_default_value[iloop];
	}

	for (iloop = 0; iloop < WIFI_EFUSE_BLOCK_COUNT; iloop++)
		WCN_INFO("s_wifi_efuse_id[%d][%d]=%d\n",
				chip_type,
				iloop,
				s_wifi_efuse_id[chip_type][iloop]);
	/* copy efuse data to target ddr address */
	phy_addr = s_wcn_device.btwf_device->base_addr +
		   (phys_addr_t)&s_wssm_phy_offset_p->wifi.efuse[0];
	wcn_write_data_to_phy_addr(phy_addr, &tmp_value,
				sizeof(tmp_value[0])*WIFI_EFUSE_BLOCK_COUNT);

	WCN_INFO("finish\n");
}

#define WCN_EFUSE_TEMPERATURE_MAGIC 0x432ff678
#define WCN_EFUSE_TEMPERATURE_OFF 44
/* now just sharkle */
static void marlin_write_efuse_temperature(void)
{
	phys_addr_t phy_addr;
	u32 magic, val;

	magic = WCN_EFUSE_TEMPERATURE_MAGIC;
	val = sprd_efuse_double_read(WCN_EFUSE_TEMPERATURE_OFF, true);
	if (val == 0) {
		WCN_INFO("temperature efuse read err\n");
		magic += 1;
		goto out;
	}
	WCN_INFO("temperature efuse read 0x%x\n", val);
	phy_addr = s_wcn_device.btwf_device->base_addr +
		(phys_addr_t)&s_wssm_phy_offset_p->efuse_temper_val;
	wcn_write_data_to_phy_addr(phy_addr, &val, sizeof(val));
out:
	phy_addr = s_wcn_device.btwf_device->base_addr +
		(phys_addr_t)&s_wssm_phy_offset_p->efuse_temper_magic;
	wcn_write_data_to_phy_addr(phy_addr, &magic, sizeof(magic));
}

void wcn_marlin_write_efuse(void)
{
	marlin_write_efuse_data();
	marlin_write_efuse_temperature();
}

/* used for provide efuse data to gnss */
void gnss_write_efuse_data(void)
{
	phys_addr_t phy_addr, phy_addr1;
	u32 efuse_enable_value = GNSS_EFUSE_ENABLE_VALUE;
	u32 iloop = 0;
	u32 tmp_value[GNSS_EFUSE_BLOCK_COUNT];

	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKL3) {
		s_gnss_efuse_id[0] = 44;
		s_gnss_efuse_id[1] = 42;
		s_gnss_efuse_id[2] = 43;
	} else if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKLE) {
		s_gnss_efuse_id[0] = 40;
		s_gnss_efuse_id[1] = 42;
		s_gnss_efuse_id[2] = 43;
	} else if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_PIKE2) {
		s_gnss_efuse_id[0] = 44;
		s_gnss_efuse_id[1] = 39;
		s_gnss_efuse_id[2] = 40;
	} else {
		WCN_INFO("gnss efuse write not support this board now\n");
	}
	/* get data from Efuse */
	memset(&tmp_value, 0, sizeof(tmp_value[0]) * GNSS_EFUSE_BLOCK_COUNT);
	for (iloop = 0; iloop < GNSS_EFUSE_BLOCK_COUNT; iloop++) {
		tmp_value[iloop] =
		sprd_efuse_double_read(s_gnss_efuse_id[iloop], true);
	}

	/* copy efuse data to target ddr address */
	phy_addr = s_wcn_device.gnss_device->base_addr +
				   GNSS_EFUSE_DATA_OFFSET;
	wcn_write_data_to_phy_addr(phy_addr, &tmp_value,
				sizeof(tmp_value[0]) * GNSS_EFUSE_BLOCK_COUNT);
	/*write efuse function enable value*/
	phy_addr1 = s_wcn_device.gnss_device->base_addr +
		GNSS_EFUSE_ENABLE_ADDR;
	wcn_write_data_to_phy_addr(phy_addr1, &efuse_enable_value, 4);

	WCN_INFO("finish\n");
}

/* used for distinguish Pike2 or sharkle */
static void gnss_write_version_data(void)
{
	phys_addr_t phy_addr;
	u32 tmp_aon_id[2];

	tmp_aon_id[0] = g_platform_chip_id.aon_chip_id0;
	tmp_aon_id[1] = g_platform_chip_id.aon_chip_id1;
	phy_addr = wcn_get_gnss_base_addr() +
				   GNSS_REC_AON_CHIPID_OFFSET;
	wcn_write_data_to_phy_addr(phy_addr, &tmp_aon_id,
				GNSS_REC_AON_CHIPID_SIZE);

	WCN_INFO("finish\n");
}

static int wcn_load_firmware_img(struct wcn_device *wcn_dev,
				 const char *path, unsigned int len)
{
	int read_len, size, i, ret;
	loff_t off = 0;
	unsigned long timeout;
	char *data = NULL;
	char *wcn_image_buffer;
	struct file *file;

	/* try to open file */
	for (i = 1; i <= WCN_OPEN_MAX_CNT; i++) {
		file = filp_open(path, O_RDONLY, 0);
		if (IS_ERR(file)) {
			WCN_ERR("try open file %s,count_num:%d, file=%p\n",
				path, i, file);
			if (i == WCN_OPEN_MAX_CNT) {
				WCN_ERR("open file %s error\n", path);
				return -EINVAL;
			}
			msleep(200);
		} else
			break;
	}

	WCN_INFO("open image file %s  successfully\n", path);
	/* read file to buffer */
	size = len;
	wcn_image_buffer = vmalloc(size);
	if (!wcn_image_buffer) {
		fput(file);
		WCN_ERR("no memory\n");
		return -ENOMEM;
	}
	WCN_INFO("wcn_image_buffer=%p will read len:%u\n",
		 wcn_image_buffer, len);

	data = wcn_image_buffer;
	timeout = jiffies + msecs_to_jiffies(4000);
	do {
read_retry:
#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
		read_len = kernel_read(file, (void *)wcn_image_buffer,
				       size, &off);
#else
		read_len = kernel_read(file, off, wcn_image_buffer, size);
#endif
		if (read_len > 0) {
			off += read_len;
			size -= read_len;
			wcn_image_buffer += read_len;
		} else if (read_len < 0) {
			WCN_INFO("image read erro:%d read:%lld\n",
				 read_len, off);
			msleep(200);
			if (time_before(jiffies, timeout)) {
				goto read_retry;
			} else {
				vfree(data);
				fput(file);
				WCN_INFO("load image fail:%d off:%lld len:%d\n",
					 read_len, off, len);
				return read_len;
			}
		}
	} while ((read_len > 0) && (size > 0));

	fput(file);
	WCN_INFO("After read, wcn_image_buffer=%p size:%d read:%lld\n",
		 wcn_image_buffer, size, off);
	if (size + off != len)
		WCN_INFO("download image may erro!!\n");

	wcn_image_buffer = data;
	if (wcn_dev_is_gnss(wcn_dev) && gnss_ops && (gnss_ops->file_judge)) {
		ret = gnss_ops->file_judge(wcn_image_buffer,
					   s_wcn_device.gnss_type);
		if (ret == 1) {
			vfree(wcn_image_buffer);
			WCN_INFO("change gnss file path\n");
			return 1;
		}
	}

#if WCN_INTEGRATE_PLATFORM_DEBUG
	if (s_wcn_debug_case == WCN_START_MARLIN_DDR_FIRMWARE_DEBUG)
		memcpy(wcn_image_buffer, marlin_firmware_bin, len);
	else if (s_wcn_debug_case == WCN_START_GNSS_DDR_FIRMWARE_DEBUG)
		memcpy(wcn_image_buffer, gnss_firmware_bin, len);
#endif

	/* copy file data to target ddr address */
	wcn_write_data_to_phy_addr(wcn_dev->base_addr, data, len);

	vfree(wcn_image_buffer);

	WCN_INFO("finish\n");

	return 0;
}

static int wcn_load_firmware_data(struct wcn_device *wcn_dev)
{
	bool is_gnss;

	WCN_INFO("entry\n");

	if (!wcn_dev)
		return -EINVAL;
	if (strlen(wcn_dev->firmware_path) == 0) {
		/* get firmware path */
		if (wcn_get_firmware_path(wcn_dev->name,
					  wcn_dev->firmware_path) < 0) {
			WCN_ERR("wcn_get_firmware path Failed!\n");
			return -EINVAL;
		}
		WCN_INFO("firmware path=%s\n", wcn_dev->firmware_path);
	}
	is_gnss = wcn_dev_is_gnss(wcn_dev);
	if (is_gnss) {
		strcpy(wcn_dev->firmware_path, gnss_firmware_parent_path);
		strcat(wcn_dev->firmware_path, wcn_dev->firmware_path_ext);
		WCN_INFO("gnss path=%s\n", wcn_dev->firmware_path);
		gnss_file_path_set(wcn_dev->firmware_path);
	}

	return wcn_load_firmware_img(wcn_dev, wcn_dev->firmware_path,
				     wcn_dev->file_length);
}

/*
 * This function is used to use the firmware subsystem
 * to load the wcn image.And at the same time support
 * for reading from the partition image.The first way
 * to use the first.
 */
static int wcn_download_image(struct wcn_device *wcn_dev)
{
	const struct firmware *firmware;
	int load_fimrware_ret;
	bool is_marlin;
	int err;

	is_marlin = wcn_dev_is_marlin(wcn_dev);
	memset(firmware_file_name, 0, FIRMWARE_FILEPATHNAME_LENGTH_MAX);

	if (!is_marlin) {
		if (s_wcn_device.gnss_type == WCN_GNSS_TYPE_GL)
			strcpy(firmware_file_name, WCN_GNSS_FILENAME);
		else if (s_wcn_device.gnss_type == WCN_GNSS_TYPE_BD)
			strcpy(firmware_file_name, WCN_GNSS_BD_FILENAME);
		else
			return -EINVAL;
	}

	if (is_marlin)
		strcpy(firmware_file_name, WCN_BTWF_FILENAME);

	strcat(firmware_file_name, ".bin");
	WCN_INFO("loading image [%s] from firmware subsystem ...\n",
			firmware_file_name);
	err = request_firmware_direct(&firmware, firmware_file_name, NULL);
	if (err < 0) {
		WCN_ERR("no find image [%s] errno:(%d)(ignore!!)\n",
				firmware_file_name, err);
		load_fimrware_ret = wcn_load_firmware_data(wcn_dev);
		if (load_fimrware_ret != 0) {
			WCN_ERR("wcn_load_firmware_data ERR!\n");
			return -EINVAL;
		}
	} else {
		WCN_INFO("image size = %d\n", (int)firmware->size);
		if (wcn_write_data_to_phy_addr(wcn_dev->base_addr,
					       (void *)firmware->data,
					       firmware->size)) {
			WCN_ERR("wcn_mem_ram_vmap_nocache fail\n");
			release_firmware(firmware);
			return -ENOMEM;
		}

		release_firmware(firmware);
		WCN_INFO("loading image [%s] successfully!\n",
				firmware_file_name);
	}

	return 0;
}

static int wcn_download_image_new(struct wcn_device *wcn_dev)
{
	char *file;
	int ret = 0;

	/* file_path used in dts */
	if (wcn_dev->file_path) {
		file = wcn_dev->file_path;
		if (wcn_dev_is_gnss(wcn_dev)) {
			if (s_wcn_device.gnss_type == WCN_GNSS_TYPE_BD)
				file = wcn_dev->file_path_ext;
			gnss_file_path_set(file);
		}
		WCN_INFO("load config file:%s\n", file);
		ret = wcn_load_firmware_img(wcn_dev, file,
					    wcn_dev->file_length);

		/* For gnss fix file path isn't fit with actual file type */
		if (wcn_dev_is_gnss(wcn_dev) && (ret == 1)) {
			if (s_wcn_device.gnss_type == WCN_GNSS_TYPE_BD)
				file = wcn_dev->file_path;
			else
				file = wcn_dev->file_path_ext;
			gnss_file_path_set(file);
			WCN_INFO("load config file:%s\n", file);
			wcn_load_firmware_img(wcn_dev, file,
					      wcn_dev->file_length);
		}
		return 0;
	}

	/* old function */
	return wcn_download_image(wcn_dev);
}

static void wcn_clean_marlin_ddr_flag(struct wcn_device *wcn_dev)
{
	phys_addr_t phy_addr;
	u32 tmp_value;

	tmp_value = MARLIN_CP_INIT_START_MAGIC;
	phy_addr = wcn_dev->base_addr +
		   (phys_addr_t)&(s_wssm_phy_offset_p->marlin.init_status);
	wcn_write_data_to_phy_addr(phy_addr, &tmp_value, sizeof(tmp_value));

	tmp_value = 0;
	phy_addr = wcn_dev->base_addr +
		   (phys_addr_t)&s_wssm_phy_offset_p->cp2_sleep_status;
	wcn_write_data_to_phy_addr(phy_addr, &tmp_value, sizeof(tmp_value));
	phy_addr = wcn_dev->base_addr +
		   (phys_addr_t)&s_wssm_phy_offset_p->sleep_flag_addr;
	wcn_write_data_to_phy_addr(phy_addr, &tmp_value, sizeof(tmp_value));
}

static int wcn_wait_marlin_boot(struct wcn_device *wcn_dev)
{
	u32 wait_count = 0;
	u32 magic_value = 0;
	phys_addr_t phy_addr;

	phy_addr = wcn_dev->base_addr +
		   (phys_addr_t)&(s_wssm_phy_offset_p->
		   marlin.init_status);
	for (wait_count = 0; wait_count < MARLIN_WAIT_CP_INIT_COUNT;
	     wait_count++) {
		wcn_read_data_from_phy_addr(phy_addr,
					    &magic_value, sizeof(u32));
		if (magic_value == MARLIN_CP_INIT_READY_MAGIC)
			break;

		msleep(MARLIN_WAIT_CP_INIT_POLL_TIME_MS);
		WCN_INFO("BTWF: magic_value=0x%x, wait_count=%d\n",
			 magic_value, wait_count);
	}

	/* get CP ready flag failed */
	if (wait_count >= MARLIN_WAIT_CP_INIT_COUNT) {
		WCN_ERR("MARLIN boot cp timeout!\n");
		magic_value = MARLIN_CP_INIT_FALIED_MAGIC;
		wcn_write_data_to_phy_addr(phy_addr, &magic_value, sizeof(u32));
		return -1;
	}

	return 0;
}

static void wcn_marlin_boot_finish(struct wcn_device *wcn_dev)
{
	phys_addr_t phy_addr;
	u32 magic_value = 0;

	/* save cali data to INI file */
	if (!s_wcn_device.btwf_calibrated) {
		u32 cali_flag;

		marlin_save_cali_data();
		/* clear notify CP calibration flag */
		cali_flag = WIFI_CALIBRATION_FLAG_CLEAR_VALUE;
		phy_addr = s_wcn_device.btwf_device->base_addr +
			   (phys_addr_t)&(s_wssm_phy_offset_p->
			   wifi.calibration_flag);
		wcn_write_data_to_phy_addr(phy_addr, &cali_flag,
					   sizeof(cali_flag));
		s_wcn_device.btwf_calibrated = true;
	}

	/* set success flag */
	phy_addr = wcn_dev->base_addr +
		   (phys_addr_t)&s_wssm_phy_offset_p->marlin.init_status;
	magic_value = MARLIN_CP_INIT_SUCCESS_MAGIC;
	wcn_write_data_to_phy_addr(phy_addr, &magic_value, sizeof(u32));
}

/*  GNSS assert workaround */
#define GNSS_TEST_OFFSET 0x150050
#define GNSS_TEST_MAGIC 0x12345678
static void gnss_clear_boot_flag(void)
{
	phys_addr_t phy_addr;
	u32 magic_value;

	phy_addr = wcn_get_gnss_base_addr() + GNSS_TEST_OFFSET;
	wcn_read_data_from_phy_addr(phy_addr, &magic_value, sizeof(u32));
	WCN_INFO("magic value is 0x%x\n", magic_value);
	magic_value = 0;
	wcn_write_data_to_phy_addr(phy_addr, &magic_value, sizeof(u32));

	WCN_INFO("finish!\n");
}

/* used for distinguish Pike2 or sharkle */
static void gnss_read_boot_flag(void)
{
	phys_addr_t phy_addr;
	u32 magic_value;
	u32 wait_count;

	phy_addr = wcn_get_gnss_base_addr() + GNSS_TEST_OFFSET;
	for (wait_count = 0; wait_count < MARLIN_WAIT_CP_INIT_COUNT;
	     wait_count++) {
		wcn_read_data_from_phy_addr(phy_addr,
					    &magic_value, sizeof(u32));
		if (magic_value == GNSS_TEST_MAGIC)
			break;

		msleep(MARLIN_WAIT_CP_INIT_POLL_TIME_MS);
		WCN_INFO("gnss boot: magic_value=%d, wait_count=%d\n",
			  magic_value, wait_count);
	}

	WCN_INFO("finish!\n");
}

static int wcn_wait_gnss_boot(struct wcn_device *wcn_dev)
{
	static int cali_flag;
	u32 wait_count = 0;
	u32 magic_value = 0;
	phys_addr_t phy_addr;

	if (cali_flag) {
		gnss_read_boot_flag();
		return 0;
	}

	phy_addr = wcn_dev->base_addr +
		   GNSS_CALIBRATION_FLAG_CLEAR_ADDR;
	for (wait_count = 0; wait_count < GNSS_WAIT_CP_INIT_COUNT;
		 wait_count++) {
		wcn_read_data_from_phy_addr(phy_addr,
						&magic_value, sizeof(u32));
		WCN_INFO("gnss cali: magic_value=0x%x, wait_count=%d\n",
			 magic_value, wait_count);
		if (magic_value == GNSS_CALI_DONE_FLAG)
			break;
		msleep(GNSS_WAIT_CP_INIT_POLL_TIME_MS);
	}

	if (wait_count >= GNSS_WAIT_CP_INIT_COUNT) {
		gnss_set_boot_status(WCN_BOOT_CP2_ERR_BOOT);
		return -1;
	}

	cali_flag = 1;
	return 0;
}

static void wcn_marlin_pre_boot(struct wcn_device *wcn_dev)
{
	if (!s_wcn_device.btwf_calibrated)
		marlin_write_cali_data();
}

static void wcn_gnss_pre_boot(struct wcn_device *wcn_dev)
{
	gnss_write_version_data();
}

/* load firmware and boot up sys. */
int wcn_proc_native_start(void *arg)
{
	bool is_marlin;
	int err;
	struct wcn_device *wcn_dev = (struct wcn_device *)arg;

	if (!wcn_dev) {
		WCN_ERR("dev is NULL\n");
		return -ENODEV;
	}

	WCN_INFO("%s enter\n", wcn_dev->name);
	is_marlin = wcn_dev_is_marlin(wcn_dev);

	/* when hot restart handset, the DDR value is error */
	if (is_marlin)
		wcn_clean_marlin_ddr_flag(wcn_dev);

	wcn_dev->boot_cp_status = WCN_BOOT_CP2_OK;
	err = wcn_download_image_new(wcn_dev);
	if (err < 0) {
		WCN_ERR("wcn download image err!\n");
		wcn_dev->boot_cp_status = WCN_BOOT_CP2_ERR_DONW_IMG;
		return -1;
	}

	if (is_marlin)
		/* wifi need calibrate */
		wcn_marlin_pre_boot(wcn_dev);
	else
		/* gnss need prepare some data before bootup */
		wcn_gnss_pre_boot(wcn_dev);

	/* boot up system */
	wcn_cpu_bootup(wcn_dev);

	wcn_dev->power_state = WCN_POWER_STATUS_ON;
	WCN_INFO("device power_state:%d\n",
		 wcn_dev->power_state);

	/* wifi need polling CP ready */
	if (is_marlin) {
		if (wcn_wait_marlin_boot(wcn_dev)) {
			wcn_dev->boot_cp_status = WCN_BOOT_CP2_ERR_BOOT;
			return -1;
		}
		wcn_sbuf_status(3, 4);
		wcn_marlin_boot_finish(wcn_dev);
	} else if (wcn_wait_gnss_boot(wcn_dev))
		return -1;

	return 0;
}

int wcn_proc_native_stop(void *arg)
{
	struct wcn_device *wcn_dev = arg;
	u32 iloop_index;
	u32 reg_nr = 0;
	unsigned int val;
	u32 reg_read;
	u32 type;

	WCN_INFO("enter\n");

	if (wcn_dev == NULL)
		return -EINVAL;

	reg_nr = wcn_dev->reg_shutdown_nr < REG_SHUTDOWN_CNT_MAX ?
		wcn_dev->reg_shutdown_nr : REG_SHUTDOWN_CNT_MAX;
	for (iloop_index = 0; iloop_index < reg_nr; iloop_index++) {
		val = 0;
		type = wcn_dev->ctrl_shutdown_type[iloop_index];
		reg_read = wcn_dev->ctrl_shutdown_reg[iloop_index] -
			wcn_dev->ctrl_shutdown_rw_offset[iloop_index];
		wcn_regmap_read(wcn_dev->rmap[type],
				   reg_read,
				   &val
				   );
		WCN_INFO("ctrl_shutdown_reg[%d] = 0x%x, val=0x%x\n",
				iloop_index, reg_read, val);

		wcn_regmap_raw_write_bit(wcn_dev->rmap[type],
				wcn_dev->ctrl_shutdown_reg[iloop_index],
				wcn_dev->ctrl_shutdown_value[iloop_index]);
		udelay(wcn_dev->ctrl_shutdown_us_delay[iloop_index]);
		wcn_regmap_read(wcn_dev->rmap[type],
				   reg_read,
				   &val
				   );
		WCN_INFO("ctrl_reg[%d] = 0x%x, val=0x%x\n",
				iloop_index, reg_read, val);
	}

	return 0;
}

void wcn_power_wq(struct work_struct *pwork)
{
	bool is_marlin;
	struct wcn_device *wcn_dev;
	struct delayed_work *ppower_wq;
	int ret;

	ppower_wq = container_of(pwork, struct delayed_work, work);
	wcn_dev = container_of(ppower_wq, struct wcn_device, power_wq);

	WCN_INFO("start boot :%s\n", wcn_dev->name);
	is_marlin = wcn_dev_is_marlin(wcn_dev);
	if (!is_marlin)
		gnss_clear_boot_flag();

	wcn_power_enable_vddcon(true);
	if (is_marlin) {
		/* ASIC: enable vddcon and wifipa interval time > 1ms */
		usleep_range(VDDWIFIPA_VDDCON_MIN_INTERVAL_TIME,
			VDDWIFIPA_VDDCON_MAX_INTERVAL_TIME);
		wcn_marlin_power_enable_vddwifipa(true);
	}

	wcn_power_enable_sys_domain(true);
	ret = wcn_proc_native_start(wcn_dev);

	WCN_INFO("finish %s!\n", ret ? "ERR" : "OK");
	complete(&wcn_dev->download_done);
}

static void wcn_clear_ddr_gnss_cali_bit(void)
{
	phys_addr_t phy_addr;
	u32 value;
	struct wcn_device *wcn_dev;

	wcn_dev = s_wcn_device.btwf_device;
	if (wcn_dev) {
		value = GNSS_CALIBRATION_FLAG_CLEAR_ADDR_CP;
		phy_addr = wcn_dev->base_addr +
			   (phys_addr_t)&s_wssm_phy_offset_p->gnss_flag_addr;
		wcn_write_data_to_phy_addr(phy_addr, &value, sizeof(u32));
		WCN_INFO("set gnss flag off:0x%x\n", value);
	}
	wcn_dev = s_wcn_device.gnss_device;
	value = GNSS_CALIBRATION_FLAG_CLEAR_VALUE;
	phy_addr = wcn_dev->base_addr + GNSS_CALIBRATION_FLAG_CLEAR_ADDR;
	wcn_write_data_to_phy_addr(phy_addr, &value, sizeof(u32));
	WCN_INFO("clear gnss ddr bit\n");
}

static void wcn_set_nognss(u32 val)
{
	phys_addr_t phy_addr;
	struct wcn_device *wcn_dev;

	wcn_dev = s_wcn_device.btwf_device;
	if (wcn_dev) {
		phy_addr = wcn_dev->base_addr +
			   (phys_addr_t)&s_wssm_phy_offset_p->include_gnss;
		wcn_write_data_to_phy_addr(phy_addr, &val, sizeof(u32));
		WCN_INFO("gnss:%u\n", val);
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

static struct wcn_device *wcn_get_dev_by_type(u32 subsys_bit)
{
	if (subsys_bit & WCN_MARLIN_MASK)
		return s_wcn_device.btwf_device;
	else if ((subsys_bit & WCN_GNSS_MASK) ||
		 (subsys_bit & WCN_GNSS_BD_MASK))
		return s_wcn_device.gnss_device;

	WCN_ERR("invalid subsys:0x%x\n", subsys_bit);

	return NULL;
}

/* pre_str should't NULl */
static void wcn_show_dev_status(const char *pre_str)
{
	u32 status;

	if (s_wcn_device.btwf_device) {
		status = s_wcn_device.btwf_device->wcn_open_status;
		WCN_INFO("%s malrin status[%d] BT:%d FM:%d WIFI:%d MDBG:%d\n",
			 pre_str, status,
			 status & (1 << WCN_MARLIN_BLUETOOTH),
			 status & (1 << WCN_MARLIN_FM),
			 status & (1 << WCN_MARLIN_WIFI),
			 status & (1 << WCN_MARLIN_MDBG));
	}
	if (s_wcn_device.gnss_device) {
		status = s_wcn_device.gnss_device->wcn_open_status;
		WCN_INFO("%s gnss status[%d] GPS:%d GNSS_BD:%d\n",
			 pre_str, status, status & (1 << WCN_GNSS),
			 status & (1 << WCN_GNSS_BD));
	}
}

int start_integrate_wcn_truely(u32 subsys)
{
	bool is_marlin;
	struct wcn_device *wcn_dev;
	u32 subsys_bit = 1 << subsys;

	WCN_INFO("start subsys:%d\n", subsys);
	wcn_dev = wcn_get_dev_by_type(subsys_bit);
	if (!wcn_dev) {
		WCN_ERR("wcn dev null!\n");
		return -EINVAL;
	}

	wcn_show_dev_status("before start");
	mutex_lock(&wcn_dev->power_lock);

	/* Check whether opened already */
	if (wcn_dev->wcn_open_status) {
		WCN_INFO("%s opened already = %d, subsys=%d!\n",
			 wcn_dev->name, wcn_dev->wcn_open_status, subsys);
		wcn_dev->wcn_open_status |= subsys_bit;
		wcn_show_dev_status("after start1");
		mutex_unlock(&wcn_dev->power_lock);
		return 0;
	}

	is_marlin = wcn_dev_is_marlin(wcn_dev);
	if (!is_marlin) {
		if (subsys_bit & WCN_GNSS_MASK) {
			strcpy(&wcn_dev->firmware_path_ext[0],
			       WCN_GNSS_FILENAME);
			s_wcn_device.gnss_type = WCN_GNSS_TYPE_GL;
			WCN_INFO("wcn gnss path=%s\n",
				&wcn_dev->firmware_path_ext[0]);
		} else {
			strcpy(&wcn_dev->firmware_path_ext[0],
			       WCN_GNSS_BD_FILENAME);
			s_wcn_device.gnss_type = WCN_GNSS_TYPE_BD;
			WCN_INFO("wcn bd path=%s\n",
				&wcn_dev->firmware_path_ext[0]);
		}
	}

	/* Not opened, so first open */
	init_completion(&wcn_dev->download_done);
	schedule_delayed_work(&wcn_dev->power_wq, 0);

	if (wait_for_completion_timeout(&wcn_dev->download_done,
		msecs_to_jiffies(MARLIN_WAIT_CP_INIT_MAX_TIME)) <= 0) {
		/* marlin download fail dump memory */
		if (is_marlin)
			goto err_boot_marlin;
		mutex_unlock(&wcn_dev->power_lock);
		return -1;
	} else if (wcn_dev->boot_cp_status) {
		if (wcn_dev->boot_cp_status == WCN_BOOT_CP2_ERR_DONW_IMG) {
			mutex_unlock(&wcn_dev->power_lock);
			return -1;
		}
		if (is_marlin)
			goto err_boot_marlin;
		else if (wcn_dev->boot_cp_status == WCN_BOOT_CP2_ERR_BOOT) {
			mutex_unlock(&wcn_dev->power_lock);  /* gnss */
			return -1;
		}
	}
	wcn_dev->wcn_open_status |= subsys_bit;

	if (is_marlin) {
		wcn_set_module_state(true);
		mdbg_atcmd_clean();
		wcn_ap_notify_btwf_time();
	}
	mutex_unlock(&wcn_dev->power_lock);

	wcn_show_dev_status("after start2");

	return 0;

err_boot_marlin:
	mdbg_assert_interface("MARLIN boot cp timeout");
	/* warnning! fake status for poweroff in usr mode */
	wcn_dev->wcn_open_status |= subsys_bit;
	mutex_unlock(&wcn_dev->power_lock);

	return -1;
}

int start_integrate_wcn(u32 subsys)
{
	static u32 first_time;
	u32 btwf_subsys;
	u32 ret;

	WCN_INFO("subsys:%d\n", subsys);
	mutex_lock(&marlin_lock);
	if (unlikely(sprdwcn_bus_get_carddump_status() != 0)) {
		WCN_ERR("in dump status subsys=%d!\n", subsys);
		mutex_unlock(&marlin_lock);
		return -1;
	}
	if (unlikely(get_loopcheck_status() >= 2)) {
		WCN_ERR("loopcheck status error subsys=%d!\n", subsys);
		mutex_unlock(&marlin_lock);
		return -1;
	}

	if (unlikely(!first_time)) {
		if (s_wcn_device.gnss_device) {
			/* clear ddr gps cali bit */
			wcn_set_nognss(WCN_INTERNAL_INCLUD_GNSS_VAL);
			wcn_clear_ddr_gnss_cali_bit();
			ret = start_integrate_wcn_truely(WCN_GNSS);
			if (ret) {
				mutex_unlock(&marlin_lock);
				return ret;
			}
		} else {
			wcn_set_nognss(WCN_INTERNAL_NOINCLUD_GNSS_VAL);
			WCN_INFO("not include gnss\n");
		}

		/* after cali,gnss powerdown itself,AP sync state by stop op */
		if (s_wcn_device.gnss_device)
			stop_integrate_wcn_truely(WCN_GNSS);
		first_time = 1;

		if (s_wcn_device.btwf_device) {
			if (subsys == WCN_GNSS || subsys == WCN_GNSS_BD)
				btwf_subsys = WCN_MARLIN_MDBG;
			else
				btwf_subsys = subsys;
			ret = start_integrate_wcn_truely(btwf_subsys);
			if (ret) {
				mutex_unlock(&marlin_lock);
				return ret;
			}
		}
		WCN_INFO("first time, start gnss and btwf\n");

		if (s_wcn_device.btwf_device &&
		    (subsys == WCN_GNSS || subsys == WCN_GNSS_BD))
			stop_integrate_wcn_truely(btwf_subsys);
		else {
			mutex_unlock(&marlin_lock);
			return 0;
		}
	}
	ret = start_integrate_wcn_truely(subsys);
	mutex_unlock(&marlin_lock);

	return ret;
}

int start_marlin(u32 subsys)
{
	return start_integrate_wcn(subsys);
}
EXPORT_SYMBOL_GPL(start_marlin);

/* force_sleep: 1 for send cmd, 0 for the old way */
static int wcn_wait_wcn_deep_sleep(struct wcn_device *wcn_dev, int force_sleep)
{
	u32 wait_sleep_count = 0;

	for (wait_sleep_count = 0;
	     wait_sleep_count < WCN_WAIT_SLEEP_MAX_COUNT;
	     wait_sleep_count++) {
		if (wcn_get_sleep_status(wcn_dev, force_sleep) == 0)
			break;
		msleep(20);
		WCN_INFO("wait_sleep_count=%d!\n",
			 wait_sleep_count);
	}

	return 0;
}

int stop_integrate_wcn_truely(u32 subsys)
{
	bool is_marlin;
	struct wcn_device *wcn_dev;
	u32 subsys_bit = 1 << subsys;
	int force_sleep = 0;

	/* Check Parameter whether valid */
	wcn_dev = wcn_get_dev_by_type(subsys_bit);
	if (!wcn_dev) {
		WCN_ERR("wcn dev NULL: subsys=%d!\n", subsys);
		return -EINVAL;
	}

	wcn_show_dev_status("before stop");
	if (unlikely(!(subsys_bit & wcn_dev->wcn_open_status))) {
		/* It wants to stop not opened device */
		WCN_ERR("%s not opend, err: subsys = %d\n",
			wcn_dev->name, subsys);
		return -EINVAL;
	}

	is_marlin = wcn_dev_is_marlin(wcn_dev);

	mutex_lock(&wcn_dev->power_lock);
	wcn_dev->wcn_open_status &= ~subsys_bit;
	if (wcn_dev->wcn_open_status) {
		WCN_INFO("%s subsys(%d) close, and subsys(%d) opend\n",
			 wcn_dev->name, subsys, wcn_dev->wcn_open_status);
		wcn_show_dev_status("after stop1");
		mutex_unlock(&wcn_dev->power_lock);
		return 0;
	}

	WCN_INFO("%s do stop\n", wcn_dev->name);
	/* btwf use the send shutdown cp2 cmd way */
	if (is_marlin && !sprdwcn_bus_get_carddump_status())
		force_sleep = wcn_send_force_sleep_cmd(wcn_dev);
	/* the last module will stop,AP should wait CP2 sleep */
	wcn_wait_wcn_deep_sleep(wcn_dev, force_sleep);

	/* only one module works: stop CPU */
	wcn_proc_native_stop(wcn_dev);
	wcn_power_enable_sys_domain(false);

	if (is_marlin) {
		/* stop common resources if can disable it */
		wcn_marlin_power_enable_vddwifipa(false);
		/* ASIC: disable vddcon, wifipa interval time > 1ms */
		usleep_range(VDDWIFIPA_VDDCON_MIN_INTERVAL_TIME,
			     VDDWIFIPA_VDDCON_MAX_INTERVAL_TIME);
	}
	wcn_power_enable_vddcon(false);

	wcn_sys_soft_reset();
	wcn_sys_soft_release();
	wcn_sys_deep_sleep_en();
	wcn_dev->power_state = WCN_POWER_STATUS_OFF;

	WCN_INFO("%s open_status = %d,power_state=%d,stop subsys=%d!\n",
		 wcn_dev->name, wcn_dev->wcn_open_status,
		 wcn_dev->power_state, subsys);

	if (is_marlin)
		wcn_set_module_state(false);
	mutex_unlock(&(wcn_dev->power_lock));

	wcn_show_dev_status("after stop2");
	return 0;
}

int stop_integrate_wcn(u32 subsys)
{
	u32 ret;

	mutex_lock(&marlin_lock);
	if (unlikely(sprdwcn_bus_get_carddump_status() != 0)) {
		WCN_ERR("err:in dump status subsys=%d!\n", subsys);
		mutex_unlock(&marlin_lock);
		return -1;
	}
	if (unlikely(get_loopcheck_status() >= 2)) {
		WCN_ERR("err:loopcheck status error subsys=%d!\n", subsys);
		mutex_unlock(&marlin_lock);
		return -1;
	}

	ret = stop_integrate_wcn_truely(subsys);
	mutex_unlock(&marlin_lock);

	return ret;
}

int stop_marlin(u32 subsys)
{
	return stop_integrate_wcn(subsys);
}
EXPORT_SYMBOL_GPL(stop_marlin);

