/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/bug.h>
#include <linux/delay.h>
#ifdef CONFIG_SC2342_INTEG
#include <linux/gnss.h>
#endif
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <marlin_platform.h>
#include <wcn_bus.h>

#include "../wcn_gnss.h"
#include "gnss_common.h"
#include "gnss_dump.h"
#include "wcn_glb.h"
#include "wcn_glb_reg.h"

#define GNSSCOMM_INFO(format, arg...) pr_info("gnss_ctl: " format, ## arg)
#define GNSSCOMM_ERR(format, arg...) pr_err("gnss_ctl: " format, ## arg)

#define GNSS_DATA_BASE_TYPE_H  16
#define GNSS_MAX_STRING_LEN	10
/* gnss mem dump success return value is 3 */
#define GNSS_DUMP_DATA_SUCCESS	3
#define FIRMWARE_FILEPATHNAME_LENGTH_MAX 256

struct gnss_common_ctl {
	struct device *dev;
	unsigned long chip_ver;
	unsigned int gnss_status;
	unsigned int gnss_subsys;
	char firmware_path[FIRMWARE_FILEPATHNAME_LENGTH_MAX];
};

static struct gnss_common_ctl gnss_common_ctl_dev;

enum gnss_status_e {
	GNSS_STATUS_POWEROFF = 0,
	GNSS_STATUS_POWERON,
	GNSS_STATUS_ASSERT,
	GNSS_STATUS_POWEROFF_GOING,
	GNSS_STATUS_POWERON_GOING,
	GNSS_STATUS_MAX,
};
#ifdef CONFIG_SC2342_INTEG
enum gnss_cp_status_subtype {
	GNSS_CP_STATUS_CALI = 1,
	GNSS_CP_STATUS_INIT = 2,
	GNSS_CP_STATUS_INIT_DONE = 3,
	GNSS_CP_STATUS_IDLEOFF = 4,
	GNSS_CP_STATUS_IDLEON = 5,
	GNSS_CP_STATUS_SLEEP = 6,
	GNSS_CP_STATUS__MAX,
};
static struct completion gnss_dump_complete;
#endif

static const int gnss_version = 0x22;
#ifdef CONFIG_WCN_PARSE_DTS
static const struct of_device_id gnss_common_ctl_of_match[] = {
	{.compatible = "sprd,gnss_common_ctl", .data = (void *)&gnss_version},
	{},
};
#endif
#ifndef CONFIG_SC2342_INTEG
struct gnss_cali {
	bool cali_done;
	u32 *cali_data;
};
static struct gnss_cali gnss_cali_data;
static u32 *gnss_efuse_data;


#ifdef GNSSDEBUG
static void gnss_cali_done_isr(void)
{
	complete(&marlin_dev->gnss_cali_done);
	GNSSCOMM_INFO("gnss cali done");
}
#endif
static int gnss_cali_init(void)
{
	gnss_cali_data.cali_done = false;

	gnss_cali_data.cali_data = kzalloc(GNSS_CALI_DATA_SIZE, GFP_KERNEL);
	if (gnss_cali_data.cali_data == NULL) {
		GNSSCOMM_ERR("%s malloc fail.\n", __func__);
		return -ENOMEM;
	}

#ifdef GNSSDEBUG
	init_completion(&marlin_dev.gnss_cali_done);
	sdio_pub_int_RegCb(GNSS_CALI_DONE, (PUB_INT_ISR)gnss_cali_done_isr);
#endif
	gnss_efuse_data = kzalloc(GNSS_EFUSE_DATA_SIZE, GFP_KERNEL);
	if (gnss_efuse_data == NULL) {
		GNSSCOMM_ERR("%s malloc efuse data fail.\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void gnss_cali_deinit(void)
{
	gnss_cali_data.cali_done = false;
	if (gnss_cali_data.cali_data)
		kfree(gnss_cali_data.cali_data);
	if (gnss_efuse_data)
		kfree(gnss_efuse_data);
}

int gnss_write_cali_data(void)
{
	GNSSCOMM_INFO("gnss write calidata, flag %d\n",
			gnss_cali_data.cali_done);
	if (gnss_cali_data.cali_done) {
		sprdwcn_bus_direct_write(GNSS_CALI_ADDRESS,
			gnss_cali_data.cali_data, GNSS_CALI_DATA_SIZE);
	}
	return 0;
}

int gnss_write_efuse_data(void)
{
	GNSSCOMM_INFO("%s flag %d\n", __func__,	gnss_cali_data.cali_done);
	if (gnss_cali_data.cali_done)
		sprdwcn_bus_direct_write(GNSS_EFUSE_ADDRESS,
					 &gnss_efuse_data[0],
					 GNSS_EFUSE_DATA_SIZE);

	return 0;
}

int gnss_write_data(void)
{
	int ret = 0;

	gnss_write_cali_data();
	ret = gnss_write_efuse_data();

	return ret;
}

int gnss_backup_cali(void)
{
	int i = 10;
	int tempvalue = 0;

	if (!gnss_cali_data.cali_done) {
		GNSSCOMM_INFO("%s begin\n", __func__);
		if (gnss_cali_data.cali_data != NULL) {
			while (i--) {
				sprdwcn_bus_direct_read(GNSS_CALI_ADDRESS,
					gnss_cali_data.cali_data, GNSS_CALI_DATA_SIZE);
				tempvalue = *(gnss_cali_data.cali_data);
				GNSSCOMM_INFO(" cali %d time, value is 0x%x\n", i, tempvalue);
				if (tempvalue != GNSS_CALI_DONE_FLAG) {
					msleep(100);
					continue;
				}
				GNSSCOMM_INFO("-------------->cali success\n");
				gnss_cali_data.cali_done = true;
				break;
			}
		}
	} else
		GNSSCOMM_INFO(" no need back again\n");

	return 0;
}

int gnss_backup_efuse(void)
{
	int ret = 1;

	if (gnss_cali_data.cali_done) { /* efuse data is ok when cali done */
		sprdwcn_bus_direct_read(GNSS_EFUSE_ADDRESS,
					&gnss_efuse_data[0],
					GNSS_EFUSE_DATA_SIZE);
		ret = 0;
		GNSSCOMM_ERR("%s 0x%x\n", __func__, gnss_efuse_data[0]);
	} else
		GNSSCOMM_INFO("%s no need back again\n", __func__);

	return ret;
}

int gnss_backup_data(void)
{
	int ret;

	gnss_backup_cali();
	ret = gnss_backup_efuse();

	return ret;
}

int gnss_boot_wait(void)
{
	int ret = -1;
	u32 *magic_value;
	int i = 125;

	magic_value = kzalloc(GNSS_BOOTSTATUS_SIZE, GFP_KERNEL);
	if (magic_value == NULL) {
		GNSSCOMM_ERR("%s, malloc fail\n", __func__);
		return -1;
	}
	while (i--) {
		sprdwcn_bus_direct_read(GNSS_BOOTSTATUS_ADDRESS, magic_value,
					GNSS_BOOTSTATUS_SIZE);
		GNSSCOMM_ERR("boot read %d time, value is 0x%x\n",
					i, *magic_value);
		if (*magic_value != GNSS_BOOTSTATUS_MAGIC) {
			msleep(20);
			continue;
		}
		ret = 0;
		GNSSCOMM_INFO("boot read success\n");
		break;
	}
	kfree(magic_value);
	return ret;
}
#else
int gnss_file_judge(char *buff, int gnss_type)
{
	unsigned int gnssfile_flag;
	int file_type, ret = 0;

	if (buff == NULL) {
		GNSSCOMM_ERR("%s: buff null\n", __func__);
		return -1;
	}
	gnssfile_flag = *(unsigned int *)(buff + GNSS_FLAG_ADDRESS_INFILE);
	if (gnssfile_flag == GNSS_FLAG_GLO)
		file_type = WCN_GNSS_TYPE_GL;
	else if (gnssfile_flag == GNSS_FLAG_BD)
		file_type = WCN_GNSS_TYPE_BD;
	else {
		GNSSCOMM_INFO("%s: no need to change\n", __func__);
		return 0;
	}
	if (file_type != gnss_type)
		ret = 1;

	return ret;
}



#endif

static void gnss_power_on(bool enable)
{
	int ret;

	GNSSCOMM_INFO("%s:enable=%d,current gnss_status=%d\n", __func__,
			enable, gnss_common_ctl_dev.gnss_status);
	if (enable && gnss_common_ctl_dev.gnss_status == GNSS_STATUS_POWEROFF) {
		gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWERON_GOING;

		/* special for cali fail flow */
#ifdef CONFIG_SC2342_INTEG
		if (gnss_get_boot_status() == WCN_BOOT_CP2_ERR_BOOT) {
			GNSSCOMM_ERR("%s: last start failed\n", __func__);
			gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWERON;
			return;
		}
#endif
		ret = start_marlin(gnss_common_ctl_dev.gnss_subsys);
		if (ret != 0)
			GNSSCOMM_ERR("%s: start marlin failed ret=%d\n",
					__func__, ret);
		else
			gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWERON;
	} else if (!enable && gnss_common_ctl_dev.gnss_status
			== GNSS_STATUS_POWERON) {
		gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWEROFF_GOING;
#ifdef CONFIG_SC2342_INTEG
		if (gnss_get_boot_status() == WCN_BOOT_CP2_ERR_BOOT)
			gnss_set_boot_status(WCN_BOOT_CP2_OK); /* init val */
#endif
		ret = stop_marlin(gnss_common_ctl_dev.gnss_subsys);
		if (ret != 0)
			GNSSCOMM_INFO("%s: stop marlin failed ret=%d\n",
				 __func__, ret);
		else
			gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWEROFF;
	} else {
		GNSSCOMM_INFO("%s: status is not match\n", __func__);
	}
}

static ssize_t gnss_power_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long set_value;

	if (kstrtoul(buf, GNSS_MAX_STRING_LEN, &set_value)) {
		GNSSCOMM_ERR("%s, Maybe store string is too long\n", __func__);
		return -EINVAL;
	}
	GNSSCOMM_INFO("%s,%lu\n", __func__, set_value);
	if (set_value == 1)
		gnss_power_on(1);
	else if (set_value == 0)
		gnss_power_on(0);
	else {
		count = -EINVAL;
		GNSSCOMM_INFO("%s,unknown control\n", __func__);
	}

	return count;
}

static DEVICE_ATTR_WO(gnss_power_enable);

static ssize_t gnss_subsys_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long set_value;

	if (kstrtoul(buf, GNSS_MAX_STRING_LEN, &set_value))
		return -EINVAL;

	GNSSCOMM_INFO("%s,%lu\n", __func__, set_value);
#ifndef CONFIG_SC2342_INTEG
	gnss_common_ctl_dev.gnss_subsys = MARLIN_GNSS;
#else
	if (set_value == WCN_GNSS)
		gnss_common_ctl_dev.gnss_subsys = WCN_GNSS;
	else if (set_value == WCN_GNSS_BD)
		gnss_common_ctl_dev.gnss_subsys  = WCN_GNSS_BD;
	else
		count = -EINVAL;
#endif
	return count;
}

void gnss_file_path_set(char *buf)
{
	strcpy(&gnss_common_ctl_dev.firmware_path[0], buf);
}

static ssize_t gnss_subsys_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int i = 0;

	GNSSCOMM_INFO("%s\n", __func__);
	if (gnss_common_ctl_dev.gnss_status == GNSS_STATUS_POWERON) {
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d:%s\n",
				gnss_common_ctl_dev.gnss_subsys,
				&gnss_common_ctl_dev.firmware_path[0]);
	} else {
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				gnss_common_ctl_dev.gnss_subsys);
	}

	return i;
}

static DEVICE_ATTR_RW(gnss_subsys);

#ifdef CONFIG_SC2342_INTEG
static int gnss_status_get(void)
{
	phys_addr_t phy_addr;
	u32 magic_value;

	phy_addr = wcn_get_gnss_base_addr() + GNSS_STATUS_OFFSET;
	wcn_read_data_from_phy_addr(phy_addr, &magic_value, sizeof(u32));
	GNSSCOMM_INFO("[%s] magic_value=%d\n", __func__, magic_value);

	return magic_value;
}

void gnss_dump_mem_ctrl_co(void)
{
	char flag = 0; /* 0: default, all, 1: only data, pmu, aon */
	unsigned int temp_status = 0;
	static char dump_flag;

	GNSSCOMM_INFO("[%s], flag is %d\n", __func__, dump_flag);
	if (dump_flag == 1)
		return;
	dump_flag = 1;

	if (gnss_get_boot_status() == WCN_BOOT_CP2_ERR_BOOT)
		gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWERON;

	temp_status = gnss_common_ctl_dev.gnss_status;
	if ((temp_status == GNSS_STATUS_POWERON_GOING) ||
		((temp_status == GNSS_STATUS_POWERON) &&
		(gnss_status_get() != GNSS_CP_STATUS_SLEEP))) {
		flag = (temp_status == GNSS_STATUS_POWERON) ? 0 : 1;
		gnss_dump_mem(flag);
		gnss_common_ctl_dev.gnss_status = GNSS_STATUS_ASSERT;
	}
	complete(&gnss_dump_complete);
}
#else
int gnss_dump_mem_ctrl(void)
{
	int ret = -1;
	static char dump_flag;

	GNSSCOMM_INFO("[%s], flag is %d\n", __func__, dump_flag);
	if (dump_flag == 1)
		return 0;
	dump_flag = 1;
	if (gnss_common_ctl_dev.gnss_status == GNSS_STATUS_POWERON) {
		ret = gnss_dump_mem(0);
		gnss_common_ctl_dev.gnss_status = GNSS_STATUS_ASSERT;
	}

	return ret;
}
#endif
static ssize_t gnss_dump_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long set_value;
	int ret = -1;
	int temp = 0;

	if (kstrtoul(buf, GNSS_MAX_STRING_LEN, &set_value)) {
		GNSSCOMM_ERR("%s, store string is too long\n", __func__);
		return -EINVAL;
	}
	GNSSCOMM_INFO("%s,%lu\n", __func__, set_value);
	if (set_value == 1) {
#ifdef CONFIG_SC2342_INTEG
		temp = wait_for_completion_timeout(&gnss_dump_complete,
						   msecs_to_jiffies(6000));
		GNSSCOMM_INFO("%s exit %d\n", __func__,
				  jiffies_to_msecs(temp));
		if (temp > 0)
			ret = GNSS_DUMP_DATA_SUCCESS;
		else
			gnss_dump_mem_ctrl_co();
#else
		temp = gnss_dump_mem_ctrl();
		GNSSCOMM_INFO("%s exit temp %d\n", __func__, temp);
		if (temp == 0)
			ret = GNSS_DUMP_DATA_SUCCESS;
#endif
	} else
		count = -EINVAL;

	return ret;
}

static DEVICE_ATTR_WO(gnss_dump);

static ssize_t gnss_status_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int i = 0;

	GNSSCOMM_INFO("%s\n", __func__);

	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			gnss_common_ctl_dev.gnss_status);

	return i;
}
static DEVICE_ATTR_RO(gnss_status);
#ifndef CONFIG_SC2342_INTEG
static uint gnss_op_reg;
static uint gnss_indirect_reg_offset;
static ssize_t gnss_regr_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int i = 0;
	unsigned int op_reg = gnss_op_reg;
	unsigned int buffer;

	GNSSCOMM_INFO("%s, register is 0x%x\n", __func__, gnss_op_reg);
	if (op_reg == GNSS_INDIRECT_OP_REG) {
		int set_value;

		set_value = gnss_indirect_reg_offset + 0x80000000;
		sprdwcn_bus_direct_write(op_reg, &set_value, 4);
	}
	sprdwcn_bus_direct_read(op_reg, &buffer, 4);
	GNSSCOMM_INFO("%s,temp value is 0x%x\n", __func__, buffer);

	i += scnprintf(buf + i, PAGE_SIZE - i, "show: 0x%x\n", buffer);

	return i;
}
static DEVICE_ATTR_RO(gnss_regr);

static ssize_t gnss_regaddr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long set_addr;

	if (kstrtoul(buf, GNSS_DATA_BASE_TYPE_H, &set_addr)) {
		GNSSCOMM_ERR("%s, input error\n", __func__);
		return -EINVAL;
	}
	GNSSCOMM_INFO("%s,0x%lx\n", __func__, set_addr);
	gnss_op_reg = (uint)set_addr;

	return count;
}
static DEVICE_ATTR_WO(gnss_regaddr);

static ssize_t gnss_regspaddr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long set_addr;

	if (kstrtoul(buf, GNSS_DATA_BASE_TYPE_H, &set_addr)) {
		GNSSCOMM_ERR("%s, input error\n", __func__);
		return -EINVAL;
	}
	GNSSCOMM_INFO("%s,0x%lx\n", __func__, set_addr);
	gnss_op_reg = GNSS_INDIRECT_OP_REG;
	gnss_indirect_reg_offset = (uint)set_addr;
	return count;
}
static DEVICE_ATTR_WO(gnss_regspaddr);

static ssize_t gnss_regw_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long set_value;
	unsigned int op_reg = gnss_op_reg;

	if (kstrtoul(buf, GNSS_DATA_BASE_TYPE_H, &set_value)) {
		GNSSCOMM_ERR("%s, input error\n", __func__);
		return -EINVAL;
	}
	if (op_reg == GNSS_INDIRECT_OP_REG)
		set_value = gnss_indirect_reg_offset + set_value;
	GNSSCOMM_INFO("%s,0x%lx\n", __func__, set_value);
	sprdwcn_bus_direct_write(op_reg, &set_value, 4);

	return count;
}
static DEVICE_ATTR_WO(gnss_regw);
#endif

bool gnss_delay_ctl(void)
{
	return (gnss_common_ctl_dev.gnss_status == GNSS_STATUS_POWERON);
}

static struct attribute *gnss_common_ctl_attrs[] = {
	&dev_attr_gnss_power_enable.attr,
	&dev_attr_gnss_dump.attr,
	&dev_attr_gnss_status.attr,
	&dev_attr_gnss_subsys.attr,
#ifndef CONFIG_SC2342_INTEG
	&dev_attr_gnss_regr.attr,
	&dev_attr_gnss_regaddr.attr,
	&dev_attr_gnss_regspaddr.attr,
	&dev_attr_gnss_regw.attr,
#endif
	NULL,
};

static struct attribute_group gnss_common_ctl_group = {
	.name = NULL,
	.attrs = gnss_common_ctl_attrs,
};

static struct miscdevice gnss_common_ctl_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gnss_common_ctl",
	.fops = NULL,
};

#ifdef CONFIG_SC2342_INTEG
static struct sprdwcn_gnss_ops gnss_common_ctl_ops = {
	.file_judge = gnss_file_judge
};
#else
static struct sprdwcn_gnss_ops gnss_common_ctl_ops = {
	.backup_data = gnss_backup_data,
	.write_data = gnss_write_data,
	.set_file_path = gnss_file_path_set,
	.wait_gnss_boot = gnss_boot_wait
};
#endif

static int gnss_common_ctl_probe(struct platform_device *pdev)
{
	int ret;
#ifdef CONFIG_WCN_PARSE_DTS
	const struct of_device_id *of_id;
#endif

	GNSSCOMM_ERR("%s enter", __func__);
	gnss_common_ctl_dev.dev = &pdev->dev;

	gnss_common_ctl_dev.gnss_status = GNSS_STATUS_POWEROFF;
	gnss_common_ctl_dev.gnss_subsys = MARLIN_GNSS;
	gnss_cali_init();

#ifdef CONFIG_WCN_PARSE_DTS
	/* considering backward compatibility, it's not use now  start */
	of_id = of_match_node(gnss_common_ctl_of_match,
		pdev->dev.of_node);
	if (!of_id) {
		dev_err(&pdev->dev,
			"get gnss_common_ctl of device id failed!\n");
		return -ENODEV;
	}
#endif
#ifdef CONFIG_WCN_PARSE_DTS
	gnss_common_ctl_dev.chip_ver = (unsigned long)(of_id->data);
#else
	gnss_common_ctl_dev.chip_ver = gnss_version;
#endif
	/* considering backward compatibility, it's not use now  end */

	platform_set_drvdata(pdev, &gnss_common_ctl_dev);
	ret = misc_register(&gnss_common_ctl_miscdev);
	if (ret) {
		GNSSCOMM_ERR("%s failed to register gnss_common_ctl.\n",
			__func__);
		return ret;
	}

	ret = sysfs_create_group(&gnss_common_ctl_miscdev.this_device->kobj,
			&gnss_common_ctl_group);
	if (ret) {
		GNSSCOMM_ERR("%s failed to create device attributes.\n",
			__func__);
		goto err_attr_failed;
	}

#ifdef CONFIG_SC2342_INTEG
	/* register dump callback func for mdbg */
	mdbg_dump_gnss_register(gnss_dump_mem_ctrl_co, NULL);
	init_completion(&gnss_dump_complete);
#endif
	wcn_gnss_ops_register(&gnss_common_ctl_ops);

	return 0;

err_attr_failed:
	misc_deregister(&gnss_common_ctl_miscdev);
	return ret;
}

static int gnss_common_ctl_remove(struct platform_device *pdev)
{
	gnss_cali_deinit();
	wcn_gnss_ops_unregister();
	sysfs_remove_group(&gnss_common_ctl_miscdev.this_device->kobj,
				&gnss_common_ctl_group);

	misc_deregister(&gnss_common_ctl_miscdev);
	return 0;
}
static struct platform_driver gnss_common_ctl_drv = {
	.driver = {
		   .name = "gnss_common_ctl",
		   .owner = THIS_MODULE,
#ifdef CONFIG_WCN_PARSE_DTS
		   .of_match_table = of_match_ptr(gnss_common_ctl_of_match),
#endif
		   },
	.probe = gnss_common_ctl_probe,
	.remove = gnss_common_ctl_remove
};

int __init gnss_common_ctl_init(void)
{
	return platform_driver_register(&gnss_common_ctl_drv);
}

void __exit gnss_common_ctl_exit(void)
{
	platform_driver_unregister(&gnss_common_ctl_drv);
}

#if (0)
module_init(gnss_common_ctl_init);
module_exit(gnss_common_ctl_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Spreadtrum Gnss Driver");
MODULE_AUTHOR("Jun.an<jun.an@spreadtrum.com>");
#endif
