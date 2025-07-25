/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/file.h>
#include <linux/fs.h>
#ifdef CONFIG_SC2342_INTEG
#include <linux/gnss.h>
#endif
#include <linux/kthread.h>
#include <linux/printk.h>
#ifdef CONFIG_WCN_SIPC
#include <linux/sipc.h>
#endif
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/wait.h>
#include <marlin_platform.h>
#include <wcn_bus.h>

#include "gnss_common.h"
#include "gnss_dump.h"
#include "mdbg_type.h"
#include "wcn_glb.h"
#include "wcn_glb_reg.h"

#define GNSSDUMP_INFO(format, arg...) pr_info("gnss_dump: " format, ## arg)
#define GNSSDUMP_ERR(format, arg...) pr_err("gnss_dump: " format, ## arg)

static struct file *gnss_dump_file;
static	loff_t pos;
#define GNSS_MEMDUMP_PATH			"/data/vendor/gnss/gnssdump.mem"

#ifndef CONFIG_SC2342_INTEG
struct gnss_mem_dump {
	uint address;
	uint length;
};

/* dump cp firmware firstly, wait for next adding */
static struct gnss_mem_dump gnss_marlin3_dump[] = {
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
	{GNSS_CP_START_ADDR, GNSS_FIRMWARE_MAX_SIZE}, /* gnss firmware code */
#else
	{0, 0},
#endif
	{GNSS_DRAM_ADDR, GNSS_DRAM_SIZE}, /* gnss dram */
	{GNSS_TE_MEM, GNSS_TE_MEM_SIZE}, /* gnss te mem */
	{GNSS_BASE_AON_APB, GNSS_BASE_AON_APB_SIZE}, /* aon apb */
	{CTL_BASE_AON_CLOCK, CTL_BASE_AON_CLOCK_SIZE}, /* aon clock */
};
#else
struct regmap_dump {
	int regmap_type;
	uint reg;
};
/* for sharkle ap reg dump, this order format can't change, just do it */
static struct regmap_dump gnss_sharkle_ap_reg[] = {
	{REGMAP_PMU_APB, 0x00cc}, /* REG_PMU_APB_SLEEP_CTRL */
	{REGMAP_PMU_APB, 0x00d4}, /* REG_PMU_APB_SLEEP_STATUS */
	{REGMAP_AON_APB, 0x057c}, /* REG_AON_APB_WCN_SYS_CFG2 */
	{REGMAP_AON_APB, 0x0578}, /* REG_AON_APB_WCN_SYS_CFG1 */
	{REGMAP_TYPE_NR, (AON_CLK_CORE + CGM_WCN_SHARKLE_CFG)}, /* clk */
	{REGMAP_PMU_APB, 0x0100}, /* REG_PMU_APB_PD_WCN_SYS_CFG */
	{REGMAP_PMU_APB, 0x0104}, /* REG_PMU_APB_PD_WIFI_WRAP_CFG */
	{REGMAP_PMU_APB, 0x0108}, /* REG_PMU_APB_PD_GNSS_WRAP_CFG */
};

/* for pike2 ap reg dump, this order format can't change, just do it */
static struct regmap_dump gnss_pike2_ap_reg[] = {
	{REGMAP_PMU_APB, 0x00cc}, /* REG_PMU_APB_SLEEP_CTRL */
	{REGMAP_PMU_APB, 0x00d4}, /* REG_PMU_APB_SLEEP_STATUS */
	{REGMAP_PMU_APB, 0x0338}, /* REG_PMU_APB_WCN_SYS_CFG_STATUS */
	{REGMAP_AON_APB, 0x00d8}, /* REG_AON_APB_WCN_CONFIG0 */
	{REGMAP_TYPE_NR, (AON_CLK_CORE + CGM_WCN_PIKE2_CFG)}, /* clk */
	{REGMAP_PMU_APB, 0x0050}, /* REG_PMU_APB_PD_WCN_TOP_CFG */
	{REGMAP_PMU_APB, 0x0054}, /* REG_PMU_APB_PD_WCN_WIFI_CFG */
	{REGMAP_PMU_APB, 0x0058}, /* REG_PMU_APB_PD_WCN_GNSS_CFG */
};


/* for sharkl3 ap reg dump, this order format can't change, just do it */
static struct regmap_dump gnss_sharkl3_ap_reg[] = {
	{REGMAP_PMU_APB, 0x00cc}, /* REG_PMU_APB_SLEEP_CTRL */
	{REGMAP_PMU_APB, 0x00d4}, /* REG_PMU_APB_SLEEP_STATUS */
	{REGMAP_AON_APB, 0x057c}, /* REG_AON_APB_WCN_SYS_CFG2 */
	{REGMAP_AON_APB, 0x0578}, /* REG_AON_APB_WCN_SYS_CFG1 */
	{REGMAP_TYPE_NR, (AON_CLK_CORE + CGM_WCN_SHARKLE_CFG)}, /* clk */
	{REGMAP_PMU_APB, 0x0100}, /* REG_PMU_APB_PD_WCN_SYS_CFG */
	{REGMAP_PMU_APB, 0x0104}, /* REG_PMU_APB_PD_WIFI_WRAP_CFG */
	{REGMAP_PMU_APB, 0x0108}, /* REG_PMU_APB_PD_GNSS_WRAP_CFG */
};
#define GNSS_DUMP_REG_NUMBER 8
static char gnss_dump_level; /* 0: default, all, 1: only data, pmu, aon */

#endif


static int wcn_chmod(char *path, char *mode)
{
	int result = 0;
	char cmd_path[] = "/usr/bin/chmod";
	char *cmd_argv[] = {cmd_path, mode, path, NULL};
	char *cmd_envp[] = {"HOME=/", "PATH=/sbin:/bin:/usr/bin", NULL};

	result = call_usermodehelper(cmd_path, cmd_argv, cmd_envp,
		UMH_WAIT_PROC);

	return result;
}

static int gnss_creat_gnss_dump_file(void)
{
	gnss_dump_file = filp_open(GNSS_MEMDUMP_PATH,
		O_RDWR | O_CREAT | O_TRUNC, 0666);
	GNSSDUMP_INFO("gnss_creat_gnss_dump_file entry\n");
	if (IS_ERR(gnss_dump_file)) {
		GNSSDUMP_ERR("%s error is %p\n",
			__func__, gnss_dump_file);
		return -1;
	}
	if (wcn_chmod(GNSS_MEMDUMP_PATH, "0666") != 0)
		GNSSDUMP_ERR("%s chmod	error\n", __func__);

	return 0;
}

#ifdef CONFIG_SC2342_INTEG
static void gnss_write_data_to_phy_addr(phys_addr_t phy_addr,
					      void *src_data, u32 size)
{
	void *virt_addr;

	GNSSDUMP_INFO("gnss_write_data_to_phy_addr entry\n");
	virt_addr = shmem_ram_vmap_nocache(phy_addr, size);
	if (virt_addr) {
		memcpy(virt_addr, src_data, size);
		shmem_ram_unmap(virt_addr);
	} else
		GNSSDUMP_ERR("%s shmem_ram_vmap_nocache fail\n", __func__);
}

static void gnss_read_data_from_phy_addr(phys_addr_t phy_addr,
					  void *tar_data, u32 size)
{
	void *virt_addr;

	GNSSDUMP_INFO("gnss_read_data_from_phy_addr\n");
	virt_addr = shmem_ram_vmap_nocache(phy_addr, size);
	if (virt_addr) {
		memcpy(tar_data, virt_addr, size);
		shmem_ram_unmap(virt_addr);
	} else
		GNSSDUMP_ERR("%s shmem_ram_vmap_nocache fail\n", __func__);
}

static void gnss_hold_cpu(void)
{
	struct regmap *regmap;
	u32 value;
	phys_addr_t base_addr;
	int i = 0;

	GNSSDUMP_INFO("gnss_hold_cpu entry\n");
	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKL3)
		regmap = wcn_get_gnss_regmap(REGMAP_WCN_REG);
	else
		regmap = wcn_get_gnss_regmap(REGMAP_ANLG_WRAP_WCN);
	wcn_regmap_read(regmap, 0X20, &value);
	value |= 1 << 2;
	wcn_regmap_raw_write_bit(regmap, 0X20, value);

	wcn_regmap_read(regmap, 0X24, &value);
	value |= 1 << 3;
	wcn_regmap_raw_write_bit(regmap, 0X24, value);

	value = GNSS_CACHE_FLAG_VALUE;
	base_addr = wcn_get_gnss_base_addr();
	gnss_write_data_to_phy_addr(base_addr + GNSS_CACHE_FLAG_ADDR,
		(void *)&value, 4);

	value = 0;
	wcn_regmap_raw_write_bit(regmap, 0X20, value);
	wcn_regmap_raw_write_bit(regmap, 0X24, value);
	while (i < 3) {
		gnss_read_data_from_phy_addr(base_addr + GNSS_CACHE_FLAG_ADDR,
			(void *)&value, 4);
		if (value == GNSS_CACHE_END_VALUE)
			break;
		i++;
		msleep(50);
	}
	if (value != GNSS_CACHE_END_VALUE)
		GNSSDUMP_ERR("%s gnss cache failed value %d\n", __func__,
			value);

	msleep(200);
}

static int gnss_dump_cp_register_data(u32 addr, u32 len)
{
	struct regmap *regmap;
	u32 i;
	u8 *buf = NULL;
	u8 *ptr = NULL;
	long int ret;
	void  *iram_buffer = NULL;

	GNSSDUMP_INFO(" start dump cp register!addr:%x,len:%d\n", addr, len);
	buf = kzalloc(len, GFP_KERNEL);
	if (!buf) {
		GNSSDUMP_ERR("%s kzalloc buf error\n", __func__);
		return -ENOMEM;
	}

	if (IS_ERR(gnss_dump_file)) {
		gnss_dump_file = filp_open(GNSS_MEMDUMP_PATH,
			O_RDWR | O_CREAT | O_APPEND, 0666);
		if (IS_ERR(gnss_dump_file)) {
			GNSSDUMP_ERR("%s  open file mem error\n", __func__);
			kfree(buf);
			return PTR_ERR(gnss_dump_file);
		}
	}

	iram_buffer = vmalloc(len);
	if (!iram_buffer) {
		GNSSDUMP_ERR("%s vmalloc iram_buffer error\n", __func__);
		kfree(buf);
		return -ENOMEM;
	}
	memset(iram_buffer, 0, len);

	/* can't op cp reg when level is 1, just record 0 to buffer */
	if (gnss_dump_level == 0) {
		if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKL3)
			regmap = wcn_get_gnss_regmap(REGMAP_WCN_REG);
		else
			regmap = wcn_get_gnss_regmap(REGMAP_ANLG_WRAP_WCN);
		wcn_regmap_raw_write_bit(regmap, ANLG_WCN_WRITE_ADDR, addr);
		for (i = 0; i < len / 4; i++) {
			ptr = buf + i * 4;
			wcn_regmap_read(regmap, ANLG_WCN_READ_ADDR, (u32 *)ptr);
		}
		memcpy(iram_buffer, buf, len);
	}
	pos = gnss_dump_file->f_pos;
	ret = kernel_write(gnss_dump_file, iram_buffer, len, &pos);
	gnss_dump_file->f_pos = pos;
	kfree(buf);
	vfree(iram_buffer);
	if (ret != len) {
		GNSSDUMP_ERR("gnss_dump_cp_register_data failed  size is %ld\n",
			ret);
		return -1;
	}
	GNSSDUMP_INFO("gnss_dump_cp_register_data finish  size is  %ld\n",
		ret);

	return ret;
}


static int gnss_dump_ap_register(void)
{
	struct regmap *regmap;
	u32 value[GNSS_DUMP_REG_NUMBER + 1] = {0}; /* [0]board+ [..]reg */
	u32 i = 0;
	u32 len = 0;
	u8 *ptr = NULL;
	int ret;
	void  *apreg_buffer = NULL;
	struct regmap_dump *gnss_ap_reg = NULL;

	GNSSDUMP_INFO("%s ap reg data\n", __func__);
	if (IS_ERR(gnss_dump_file)) {
		gnss_dump_file = filp_open(GNSS_MEMDUMP_PATH,
			O_RDWR | O_CREAT | O_APPEND, 0666);
		if (IS_ERR(gnss_dump_file)) {
			GNSSDUMP_ERR("%s open file mem error\n", __func__);
			return -1;
		}
	}

	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKLE) {
		gnss_ap_reg = gnss_sharkle_ap_reg;
		len = (GNSS_DUMP_REG_NUMBER + 1) * sizeof(u32);
	} else if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_PIKE2) {
		gnss_ap_reg = gnss_pike2_ap_reg;
		len = (GNSS_DUMP_REG_NUMBER + 1) * sizeof(u32);
	} else {
		gnss_ap_reg = gnss_sharkl3_ap_reg;
		len = (GNSS_DUMP_REG_NUMBER + 1) * sizeof(u32);
	}

	apreg_buffer = vmalloc(len);
	if (!apreg_buffer)
		return -2;

	ptr = (u8 *)&value[0];
	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKLE)
		value[0] = 0xF1;
	else if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_PIKE2)
		value[0] = 0xF2;
	else
		value[0] = 0xF3;

	for (i = 0; i < GNSS_DUMP_REG_NUMBER; i++) {
		if ((gnss_ap_reg + i)->regmap_type == REGMAP_TYPE_NR) {
			gnss_read_data_from_phy_addr((gnss_ap_reg + i)->reg,
				&value[i+1], 4);
		} else {
			regmap =
			wcn_get_gnss_regmap((gnss_ap_reg + i)->regmap_type);
			wcn_regmap_read(regmap, (gnss_ap_reg + i)->reg,
				&value[i+1]);
		}
	}
	memset(apreg_buffer, 0, len);
	memcpy(apreg_buffer, ptr, len);
	pos = gnss_dump_file->f_pos;
	ret = kernel_write(gnss_dump_file, apreg_buffer, len, &pos);
	gnss_dump_file->f_pos = pos;
	vfree(apreg_buffer);
	if (ret != len)
		GNSSDUMP_ERR("%s not write completely,ret is 0x%x\n", __func__,
			ret);

	return 0;
}

static void gnss_dump_cp_register(void)
{
	u32 count;

	count = gnss_dump_cp_register_data(DUMP_REG_GNSS_APB_CTRL_ADDR,
			DUMP_REG_GNSS_APB_CTRL_LEN);
	GNSSDUMP_INFO("gnss dump gnss_apb_ctrl_reg %u ok!\n", count);

	count = gnss_dump_cp_register_data(DUMP_REG_GNSS_AHB_CTRL_ADDR,
			DUMP_REG_GNSS_AHB_CTRL_LEN);
	GNSSDUMP_INFO("gnss dump manu_clk_ctrl_reg %u ok!\n", count);

	count = gnss_dump_cp_register_data(DUMP_COM_SYS_CTRL_ADDR,
			DUMP_COM_SYS_CTRL_LEN);
	GNSSDUMP_INFO("gnss dump com_sys_ctrl_reg %u ok!\n", count);

	count = gnss_dump_cp_register_data(DUMP_WCN_CP_CLK_CORE_ADDR,
			DUMP_WCN_CP_CLK_LEN);
	GNSSDUMP_INFO("gnss dump manu_clk_ctrl_reg %u ok!\n", count);
}

static void gnss_dump_register(void)
{
	gnss_dump_ap_register();
	gnss_dump_cp_register();
	GNSSDUMP_INFO("gnss dump register ok!\n");
}

static void gnss_dump_iram(void)
{
	u32 count;

	count = gnss_dump_cp_register_data(GNSS_DUMP_IRAM_START_ADDR,
			GNSS_CP_IRAM_DATA_NUM * 4);
	GNSSDUMP_INFO("gnss dump iram %u ok!\n", count);
}

static int gnss_dump_share_memory(u32 len)
{
	void *virt_addr;
	phys_addr_t base_addr;
	long int ret;
	void  *ddr_buffer = NULL;

	if (len == 0)
		return -1;
	GNSSDUMP_INFO("gnss_dump_share_memory\n");
	base_addr = wcn_get_gnss_base_addr();
	virt_addr = shmem_ram_vmap_nocache(base_addr, len);
	if (!virt_addr) {
		GNSSDUMP_ERR(" %s shmem_ram_vmap_nocache fail\n", __func__);
		return -1;
	}

	if (IS_ERR(gnss_dump_file)) {
		gnss_dump_file = filp_open(GNSS_MEMDUMP_PATH,
			O_RDWR | O_CREAT | O_APPEND, 0666);
		if (IS_ERR(gnss_dump_file)) {
			GNSSDUMP_ERR("%s open file mem error\n", __func__);
			return PTR_ERR(gnss_dump_file);
		}
	}

	ddr_buffer = vmalloc(len);
	if (!ddr_buffer) {
		GNSSDUMP_ERR(" %s vmalloc ddr_buffer fail\n", __func__);
		return -1;
	}
	memset(ddr_buffer, 0, len);
	memcpy(ddr_buffer, virt_addr, len);
	pos = gnss_dump_file->f_pos;
	ret = kernel_write(gnss_dump_file, ddr_buffer, len, &pos);
	gnss_dump_file->f_pos = pos;
	shmem_ram_unmap(virt_addr);
	vfree(ddr_buffer);
	if (ret != len) {
		GNSSDUMP_ERR("%s dump ddr error,data len is %ld\n", __func__,
			ret);
		return -1;
	}

	GNSSDUMP_INFO("gnss dump share memory  size = %ld\n", ret);

	return 0;
}

static int gnss_integrated_dump_mem(void)
{
	int ret = 0;

	GNSSDUMP_INFO("gnss_dump_mem entry\n");
	gnss_hold_cpu();
	ret = gnss_creat_gnss_dump_file();
	if (ret == -1) {
		GNSSDUMP_ERR("%s create mem dump file  error\n", __func__);
		return -1;
	}
	ret = gnss_dump_share_memory(GNSS_SHARE_MEMORY_SIZE);
	gnss_dump_iram();
	gnss_dump_register();
	if (gnss_dump_file != NULL)
		filp_close(gnss_dump_file, NULL);

	return ret;
}

#else
static int gnss_ext_hold_cpu(void)
{
	uint temp = 0;
	int ret = 0;

	GNSSDUMP_INFO("%s entry\n", __func__);
	temp = BIT_GNSS_APB_MCU_AP_RST_SOFT;
	ret = sprdwcn_bus_reg_write(REG_GNSS_APB_MCU_AP_RST + GNSS_SET_OFFSET,
		&temp, 4);
	if (ret < 0) {
		GNSSDUMP_ERR("%s write reset reg error:%d\n", __func__, ret);
		return ret;
	}
	temp = GNSS_ARCH_EB_REG_BYPASS;
	ret = sprdwcn_bus_reg_write(GNSS_ARCH_EB_REG + GNSS_SET_OFFSET,
					&temp, 4);
	if (ret < 0)
		GNSSDUMP_ERR("%s write bypass reg error:%d\n", __func__, ret);

	return ret;
}

static int gnss_ext_dump_data(unsigned int start_addr, int len)
{
	u8 *buf = NULL;
	int ret = 0, count = 0, trans = 0;

	GNSSDUMP_INFO("%s, addr:%x,len:%d\n", __func__, start_addr, len);
	buf = kzalloc(DUMP_PACKET_SIZE, GFP_KERNEL);
	if (!buf) {
		GNSSDUMP_ERR("%s kzalloc buf error\n", __func__);
		return -ENOMEM;
	}

	if (IS_ERR(gnss_dump_file)) {
		gnss_dump_file = filp_open(GNSS_MEMDUMP_PATH,
			O_RDWR | O_CREAT | O_APPEND, 0666);
		if (IS_ERR(gnss_dump_file)) {
			GNSSDUMP_ERR("%s  open file mem error\n", __func__);
			kfree(buf);
			return PTR_ERR(gnss_dump_file);
		}
	}
	while (count < len) {
		trans = (len - count) > DUMP_PACKET_SIZE ?
				 DUMP_PACKET_SIZE : (len - count);
		ret = sprdwcn_bus_direct_read(start_addr + count, buf, trans);
		if (ret < 0) {
			GNSSDUMP_ERR("%s read error:%d\n", __func__, ret);
			goto dump_data_done;
		}
		count += trans;
		pos = gnss_dump_file->f_pos;
		ret = kernel_write(gnss_dump_file, buf, trans, &pos);
		gnss_dump_file->f_pos = pos;
		if (ret != trans) {
			GNSSDUMP_ERR("%s failed size is %d, ret %d\n", __func__,
				      len, ret);
			goto dump_data_done;
		}
	}
	GNSSDUMP_INFO("%s finish %d\n", __func__, len);
	ret = 0;

dump_data_done:
	kfree(buf);
	return ret;
}

static int gnss_ext_dump_mem(void)
{
	int ret = 0;
	int i = 0;

	GNSSDUMP_INFO("%s entry\n", __func__);
// #ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	/*update the two address after get chip type*/
	gnss_marlin3_dump[0].address = GNSS_CP_START_ADDR;
	gnss_marlin3_dump[0].length = GNSS_FIRMWARE_MAX_SIZE;
// #endif
	gnss_ext_hold_cpu();
	ret = gnss_creat_gnss_dump_file();
	if (ret == -1) {
		GNSSDUMP_ERR("%s create file error\n", __func__);
		return -1;
	}
	for (i = 0; i < ARRAY_SIZE(gnss_marlin3_dump); i++)
		if (gnss_ext_dump_data(gnss_marlin3_dump[i].address,
			gnss_marlin3_dump[i].length)) {
			GNSSDUMP_ERR("%s dumpdata i %d error\n", __func__, i);
			break;
		}
	if (gnss_dump_file != NULL)
		filp_close(gnss_dump_file, NULL);
	return ret;
}
#endif

int gnss_dump_mem(char flag)
{
#ifdef CONFIG_SC2342_INTEG
	gnss_dump_level = flag;
	return gnss_integrated_dump_mem();
#else
	return gnss_ext_dump_mem();
#endif
}
