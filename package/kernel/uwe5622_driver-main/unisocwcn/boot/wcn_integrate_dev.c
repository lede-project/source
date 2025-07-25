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
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/file.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/gnss.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/sipc.h>
#include <linux/slab.h>
#include <linux/sprd_otp.h>
#include <linux/unistd.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#include "gnss_firmware_bin.h"
#include "marlin_firmware_bin.h"
#include "wcn_glb.h"
#include "wcn_glb_reg.h"
#include "wcn_log.h"
#include "wcn_misc.h"
#include "wcn_procfs.h"
#include "wcn_txrx.h"

struct wcn_device_manage s_wcn_device;

static void wcn_global_source_init(void)
{
	wcn_boot_init();
	WCN_INFO("init finish!\n");
}

#ifdef CONFIG_PM_SLEEP
static int wcn_resume(struct device *dev)
{
	WCN_INFO("enter\n");
#if SUSPEND_RESUME_ENABLE
	slp_mgr_resume();
#endif
	WCN_INFO("ok\n");

	return 0;
}

static int wcn_suspend(struct device *dev)
{
	WCN_INFO("enter\n");
#if SUSPEND_RESUME_ENABLE
	slp_mgr_suspend();
#endif
	WCN_INFO("ok\n");

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

#if WCN_INTEGRATE_PLATFORM_DEBUG
static u32 s_wcn_debug_case;
static struct task_struct *s_thead_wcn_codes_debug;
static int wcn_codes_debug_thread(void *data)
{
	u32 i;
	static u32 is_first_time = 1;

	while (!kthread_should_stop()) {
		switch (s_wcn_debug_case) {
		case WCN_START_MARLIN_DEBUG:
			for (i = 0; i < 16; i++)
				start_integrate_wcn(i);

			s_wcn_debug_case = 0;
			break;

		case WCN_STOP_MARLIN_DEBUG:
			for (i = 0; i < 16; i++)
				stop_integrate_wcn(i);

			s_wcn_debug_case = 0;
			break;

		case WCN_START_MARLIN_DDR_FIRMWARE_DEBUG:
			for (i = 0; i < 16; i++)
				start_integrate_wcn(i);

			s_wcn_debug_case = 0;
			break;

		case WCN_START_GNSS_DEBUG:
			for (i = 16; i < 32; i++)
				start_integrate_wcn(i);

			s_wcn_debug_case = 0;
			break;

		case WCN_STOP_GNSS_DEBUG:
			for (i = 16; i < 32; i++)
				stop_integrate_wcn(i);

			s_wcn_debug_case = 0;
			break;

		case WCN_START_GNSS_DDR_FIRMWARE_DEBUG:
			for (i = 16; i < 32; i++)
				start_integrate_wcn(i);

			s_wcn_debug_case = 0;
			break;

		case WCN_PRINT_INFO:
			WCN_INFO(
				"cali[data=%p flag=%p]efuse=%p status=%p gnss=%p\n",
				&s_wssm_phy_offset_p->wifi.calibration_data,
				&s_wssm_phy_offset_p->wifi.calibration_flag,
				&s_wssm_phy_offset_p->wifi.efuse[0],
				&s_wssm_phy_offset_p->marlin.init_status,
				&s_wssm_phy_offset_p->include_gnss);
			break;

		case WCN_BRINGUP_DEBUG:
			if (is_first_time) {
				msleep(100000);
				is_first_time = 0;
			}

			for (i = 0; i < 16; i++) {
				msleep(5000);
				start_integrate_wcn(i);
			}

			for (i = 0; i < 16; i++) {
				msleep(5000);
				stop_integrate_wcn(i);
			}

			break;

		default:
			msleep(5000);
			break;
		}
	}

	kthread_stop(s_thead_wcn_codes_debug);

	return 0;
}

static void wcn_codes_debug(void)
{
	/* Check reg read */
	s_thead_wcn_codes_debug = kthread_create(wcn_codes_debug_thread, NULL,
			"wcn_codes_debug");
	wake_up_process(s_thead_wcn_codes_debug);
}
#endif

static void wcn_config_ctrlreg(struct wcn_device *wcn_dev, u32 start, u32 end)
{
	u32 reg_read, type, i, val, utemp_val;

	for (i = start; i < end; i++) {
		val = 0;
		type = wcn_dev->ctrl_type[i];
		reg_read = wcn_dev->ctrl_reg[i] -
			   wcn_dev->ctrl_rw_offset[i];
		wcn_regmap_read(wcn_dev->rmap[type], reg_read, &val);
		WCN_INFO("ctrl_reg[%d]=0x%x,read=0x%x, set=%x\n",
			i, reg_read, val,
			wcn_dev->ctrl_value[i]);
		utemp_val = wcn_dev->ctrl_value[i];

		if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_PIKE2) {
			if (wcn_dev->ctrl_rw_offset[i] == 0x00)
				utemp_val = val | wcn_dev->ctrl_value[i];
		}

		WCN_INFO("rmap[%d]=%p,ctrl_reg[i]=\n",
			type, wcn_dev->rmap[type]);
		wcn_regmap_raw_write_bit(wcn_dev->rmap[type],
					 wcn_dev->ctrl_reg[i],
					 utemp_val);
		if (wcn_dev->ctrl_us_delay[i] >= 10)
			usleep_range(wcn_dev->ctrl_us_delay[i],
				     wcn_dev->ctrl_us_delay[i] + 40);
		else
			udelay(wcn_dev->ctrl_us_delay[i]);
		wcn_regmap_read(wcn_dev->rmap[type], reg_read, &val);
		WCN_INFO("ctrl_reg[%d] = 0x%x, val=0x%x\n",
			i, reg_read, val);
	}
}

void wcn_cpu_bootup(struct wcn_device *wcn_dev)
{
	u32 reg_nr;

	if (!wcn_dev)
		return;

	reg_nr = wcn_dev->reg_nr < REG_CTRL_CNT_MAX ?
		wcn_dev->reg_nr : REG_CTRL_CNT_MAX;
	wcn_config_ctrlreg(wcn_dev, wcn_dev->ctrl_probe_num, reg_nr);
}

static struct wcn_proc_data g_proc_data;
static const struct of_device_id wcn_match_table[] = {
	{ .compatible = "sprd,integrate_marlin", .data = &g_proc_data},
	{ .compatible = "sprd,integrate_gnss", .data = &g_proc_data},
	{ },
};

static int wcn_parse_dt(struct platform_device *pdev,
	struct wcn_device *wcn_dev)
{
	struct device_node *np = pdev->dev.of_node;
	u32 cr_num;
	int index, ret;
	u32 i;
	struct resource res;
	const struct of_device_id *of_id =
		of_match_node(wcn_match_table, np);
	struct wcn_proc_data *pcproc_data;

	WCN_INFO("start!\n");

	if (of_id)
		pcproc_data = (struct wcn_proc_data *)of_id->data;
	else {
		WCN_ERR("not find matched id!");
		return -EINVAL;
	}

	if (!wcn_dev) {
		WCN_ERR("wcn_dev NULL\n");
		return -EINVAL;
	}

	/* get the wcn chip name */
	ret = of_property_read_string(np,
				      "sprd,name",
				      (const char **)&wcn_dev->name);

	/* get apb reg handle */
	wcn_dev->rmap[REGMAP_AON_APB] = syscon_regmap_lookup_by_phandle(np,
						"sprd,syscon-ap-apb");
	if (IS_ERR(wcn_dev->rmap[REGMAP_AON_APB])) {
		WCN_ERR("failed to find sprd,syscon-ap-apb\n");
		return -EINVAL;
	}

	wcn_parse_platform_chip_id(wcn_dev);

	/* get pmu reg handle */
	wcn_dev->rmap[REGMAP_PMU_APB] = syscon_regmap_lookup_by_phandle(np,
						"sprd,syscon-ap-pmu");
	if (IS_ERR(wcn_dev->rmap[REGMAP_PMU_APB])) {
		WCN_ERR("failed to find sprd,syscon-ap-pmu\n");
		return -EINVAL;
	}

	/* get pub apb reg handle:SHARKLE has it, but PIKE2 hasn't  */
	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKLE) {
		wcn_dev->rmap[REGMAP_PUB_APB] =
				syscon_regmap_lookup_by_phandle(np,
						"sprd,syscon-ap-pub-apb");
		if (IS_ERR(wcn_dev->rmap[REGMAP_PUB_APB])) {
			WCN_ERR("failed to find sprd,syscon-ap-pub-apb\n");
			return -EINVAL;
		}
	}

	/* get  anlg wrap wcn reg handle */
	wcn_dev->rmap[REGMAP_ANLG_WRAP_WCN] =
					syscon_regmap_lookup_by_phandle(
					np, "sprd,syscon-anlg-wrap-wcn");
	if (IS_ERR(wcn_dev->rmap[REGMAP_ANLG_WRAP_WCN])) {
		WCN_ERR("failed to find sprd,anlg-wrap-wcn\n");
		return -EINVAL;
	}

	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKLE) {
		/* get  anlg wrap wcn reg handle */
		wcn_dev->rmap[REGMAP_ANLG_PHY_G6] =
					syscon_regmap_lookup_by_phandle(
					np, "sprd,syscon-anlg-phy-g6");
		if (IS_ERR(wcn_dev->rmap[REGMAP_ANLG_PHY_G6])) {
			WCN_ERR("failed to find sprd,anlg-phy-g6\n");
			return -EINVAL;
		}
	}

	/* SharkL3:The base Reg changed which used by AP read CP2 Regs */
	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKL3) {
		/* get  anlg wrap wcn reg handle */
		wcn_dev->rmap[REGMAP_WCN_REG] =
		syscon_regmap_lookup_by_phandle(np, "sprd,syscon-wcn-reg");
		if (IS_ERR(wcn_dev->rmap[REGMAP_WCN_REG])) {
			WCN_ERR("failed to find sprd,wcn-reg\n");
			return -EINVAL;
		}

		WCN_INFO("success to find sprd,wcn-reg for SharkL3 %p\n",
			   wcn_dev->rmap[REGMAP_WCN_REG]);
	}

	if (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKL3) {
		/* get  anlg wrap wcn reg handle */
		wcn_dev->rmap[REGMAP_ANLG_PHY_G5] =
		syscon_regmap_lookup_by_phandle(np, "sprd,syscon-anlg-phy-g5");
		if (IS_ERR(wcn_dev->rmap[REGMAP_ANLG_PHY_G5]))
			WCN_ERR("failed to find sprd,anlg-phy-g5\n");
	}

	ret = of_property_read_u32(np, "sprd,ctrl-probe-num",
				   &wcn_dev->ctrl_probe_num);
	if (ret) {
		WCN_ERR("failed to find sprd,ctrl-probe-num\n");
		return -EINVAL;
	}

	/*
	 * get ctrl_reg offset, the ctrl-reg variable number, so need
	 * to start reading from the largest until success
	 */
	cr_num = of_property_count_elems_of_size(np, "sprd,ctrl-reg", 4);
	if (cr_num > REG_CTRL_CNT_MAX) {
		WCN_ERR("DTS config err. cr_num=%d\n", cr_num);
		return -EINVAL;
	}

	do {
		ret = of_property_read_u32_array(np, "sprd,ctrl-reg",
					(u32 *)wcn_dev->ctrl_reg, cr_num);
		if (ret)
			cr_num--;
		if (!cr_num)
			return -EINVAL;
	} while (ret);

	wcn_dev->reg_nr = cr_num;
	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_reg[%d] = 0x%x\n",
			i, wcn_dev->ctrl_reg[i]);

	/* get ctrl_mask */
	ret = of_property_read_u32_array(np, "sprd,ctrl-mask",
					(u32 *)wcn_dev->ctrl_mask, cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_mask[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_mask[i]);

	/* get ctrl_value */
	ret = of_property_read_u32_array(np,
					 "sprd,ctrl-value",
					 (u32 *)wcn_dev->ctrl_value,
					 cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_value[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_value[i]);

	/* get ctrl_rw_offset */
	ret = of_property_read_u32_array(np,
					 "sprd,ctrl-rw-offset",
					 (u32 *)wcn_dev->ctrl_rw_offset,
					 cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_rw_offset[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_rw_offset[i]);

	/* get ctrl_us_delay */
	ret = of_property_read_u32_array(np,
					 "sprd,ctrl-us-delay",
					 (u32 *)wcn_dev->ctrl_us_delay,
					 cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_us_delay[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_us_delay[i]);

	/* get ctrl_type */
	ret = of_property_read_u32_array(np, "sprd,ctrl-type",
					(u32 *)wcn_dev->ctrl_type, cr_num);
	if (ret)
		return -EINVAL;

	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_type[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_type[i]);

	/*
	 * Add a new group to control shut down WCN
	 * get ctrl_reg offset, the ctrl-reg variable number, so need
	 * to start reading from the largest until success
	 */
	cr_num = of_property_count_elems_of_size(np,
				 "sprd,ctrl-shutdown-reg", 4);
	if (cr_num > REG_CTRL_CNT_MAX) {
		WCN_ERR("DTS config err. cr_num=%d\n", cr_num);
		return -EINVAL;
	}

	do {
		ret = of_property_read_u32_array(np,
				"sprd,ctrl-shutdown-reg",
				(u32 *)wcn_dev->ctrl_shutdown_reg,
				 cr_num);
		if (ret)
			cr_num--;
		if (!cr_num)
			return -EINVAL;
	} while (ret);

	wcn_dev->reg_shutdown_nr = cr_num;
	for (i = 0; i < cr_num; i++) {
		WCN_INFO("ctrl_shutdown_reg[%d] = 0x%x\n",
			i, wcn_dev->ctrl_shutdown_reg[i]);
	}

	/* get ctrl_shutdown_mask */
	ret = of_property_read_u32_array(np,
					 "sprd,ctrl-shutdown-mask",
					 (u32 *)wcn_dev->ctrl_shutdown_mask,
					 cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++) {
		WCN_INFO("ctrl_shutdown_mask[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_shutdown_mask[i]);
	}

	/* get ctrl_shutdown_value */
	ret = of_property_read_u32_array(np, "sprd,ctrl-shutdown-value",
				(u32 *)wcn_dev->ctrl_shutdown_value, cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++) {
		WCN_INFO("ctrl_shutdown_value[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_shutdown_value[i]);
	}

	/* get ctrl_shutdown_rw_offset */
	ret = of_property_read_u32_array(np,
			"sprd,ctrl-shutdown-rw-offset",
			(u32 *)wcn_dev->ctrl_shutdown_rw_offset, cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++) {
		WCN_INFO("ctrl_shutdown_rw_offset[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_shutdown_rw_offset[i]);
	}

	/* get ctrl_shutdown_us_delay */
	ret = of_property_read_u32_array(np,
			"sprd,ctrl-shutdown-us-delay",
			(u32 *)wcn_dev->ctrl_shutdown_us_delay, cr_num);
	if (ret)
		return -EINVAL;
	for (i = 0; i < cr_num; i++) {
		WCN_INFO("ctrl_shutdown_us_delay[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_shutdown_us_delay[i]);
	}

	/* get ctrl_shutdown_type */
	ret = of_property_read_u32_array(np,
			"sprd,ctrl-shutdown-type",
			(u32 *)wcn_dev->ctrl_shutdown_type, cr_num);
	if (ret)
		return -EINVAL;

	for (i = 0; i < cr_num; i++)
		WCN_INFO("ctrl_shutdown_type[%d] = 0x%08x\n",
			i, wcn_dev->ctrl_shutdown_type[i]);

	/* get vddwcn */
	if (s_wcn_device.vddwcn == NULL) {
		s_wcn_device.vddwcn = devm_regulator_get(&pdev->dev,
						     "vddwcn");
		if (IS_ERR(s_wcn_device.vddwcn)) {
			WCN_ERR("Get regulator of vddwcn error!\n");
			return -EINVAL;
		}
	}

	/* get vddwifipa: only MARLIN has it */
	if (strcmp(wcn_dev->name, WCN_MARLIN_DEV_NAME) == 0) {
		wcn_dev->vddwifipa = devm_regulator_get(&pdev->dev,
							"vddwifipa");
		if (IS_ERR(wcn_dev->vddwifipa)) {
			WCN_ERR("Get regulator of vddwifipa error!\n");
			return -EINVAL;
		}
	}

	/* get cp base */
	index = 0;
	ret = of_address_to_resource(np, index, &res);
	if (ret)
		return -EINVAL;
	wcn_dev->base_addr = res.start;
	wcn_dev->maxsz = res.end - res.start + 1;
	WCN_INFO("cp base = %llu, size = 0x%x\n",
			(u64)wcn_dev->base_addr, wcn_dev->maxsz);

	ret = of_property_read_string(np, "sprd,file-name",
				      (const char **)&wcn_dev->file_path);
	if (!ret)
		WCN_INFO("firmware name:%s\n", wcn_dev->file_path);

	ret = of_property_read_string(np, "sprd,file-name-ext",
				      (const char **)&wcn_dev->file_path_ext);
	if (!ret)
		WCN_INFO("firmware name ext:%s\n", wcn_dev->file_path_ext);
	/* get cp source file length */
	ret = of_property_read_u32_index(np,
					  "sprd,file-length",
					  0,
					  &wcn_dev->file_length);
	WCN_INFO("wcn_dev->file_length:%d\n", wcn_dev->file_length);
	if (ret)
		return -EINVAL;

	wcn_dev->start = pcproc_data->start;
	wcn_dev->stop = pcproc_data->stop;

	return 0;
}

static struct wcn_proc_data g_proc_data = {
	.start = wcn_proc_native_start,
	.stop  = wcn_proc_native_stop,
};

static int wcn_platform_open(struct inode *inode, struct file *filp)
{
	struct platform_proc_file_entry
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19, 2))
	*entry = (struct platform_proc_file_entry *)pde_data(inode);
#else
	*entry = (struct platform_proc_file_entry *)PDE_DATA(inode);
#endif

	WCN_INFO("entry name:%s\n!", entry->name);

	filp->private_data = entry;

	return 0;
}

static ssize_t wcn_platform_read(struct file *filp,
			       char __user *buf,
			       size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t wcn_platform_write(struct file *filp,
				const char __user *buf,
				size_t count,
				loff_t *ppos)
{
	struct platform_proc_file_entry
		*entry = (struct platform_proc_file_entry *)filp->private_data;
	struct wcn_device *wcn_dev = entry->wcn_dev;
	char *type = entry->name;
	unsigned int flag;
	char str[WCN_PROC_FILE_LENGTH_MAX + 1];
	u32 sub_sys = 0;

	flag = entry->flag;
	WCN_INFO("type = %s flag = 0x%x\n", type, flag);

	if ((flag & BE_WRONLY) == 0)
		return -EPERM;

	memset(&str[0], 0, WCN_PROC_FILE_LENGTH_MAX + 1);
	if (copy_from_user(&str[0], buf, WCN_PROC_FILE_LENGTH_MAX) == 0) {
		if (strncmp(str, "gnss", strlen("gnss")) == 0)
			sub_sys = WCN_GNSS;
		else
			sub_sys = str[0] - '0';
	} else {
		WCN_ERR("copy_from_user too length %s!\n", buf);
		return -EINVAL;
	}

	if ((flag & BE_CTRL_ON) != 0) {
		start_integrate_wcn(sub_sys);
		wcn_dev->status = CP_NORMAL_STATUS;
		WCN_INFO("start, str=%s!\n", str);

		return count;
	} else if ((flag & BE_CTRL_OFF) != 0) {
		stop_integrate_wcn(sub_sys);
		wcn_dev->status = CP_STOP_STATUS;
		WCN_INFO("stop, str=%s!\n", str);

		return count;
	}

	return 0;
}

static const struct file_operations wcn_platform_fs_fops = {
	.open		= wcn_platform_open,
	.read		= wcn_platform_read,
	.write		= wcn_platform_write,
};

static inline void wcn_platform_fs_init(struct wcn_device *wcn_dev)
{
	u8 i, ucnt;
	unsigned int flag;
	umode_t mode = 0;

	wcn_dev->platform_fs.platform_proc_dir_entry =
		proc_mkdir(wcn_dev->name, NULL);

	memset(wcn_dev->platform_fs.entrys,
		0,
		sizeof(wcn_dev->platform_fs.entrys));

	for (flag = 0, ucnt = 0, i = 0;
		i < MAX_PLATFORM_ENTRY_NUM;
		i++, flag = 0, mode = 0) {
		switch (i) {
		case 0:
			wcn_dev->platform_fs.entrys[i].name = "start";
			flag |= (BE_WRONLY | BE_CTRL_ON);
			ucnt++;
			break;

		case 1:
			wcn_dev->platform_fs.entrys[i].name = "stop";
			flag |= (BE_WRONLY | BE_CTRL_OFF);
			ucnt++;
			break;

		case 2:
			wcn_dev->platform_fs.entrys[i].name = "status";
			flag |= (BE_RDONLY | BE_RDWDTS);
			ucnt++;
			break;

		default:
			return;		/* we didn't use it until now */
		}

		wcn_dev->platform_fs.entrys[i].flag = flag;

		mode |= (0600);
		if (flag & (BE_CPDUMP | BE_MNDUMP))
			mode |= 0004;

		WCN_INFO("entry name is %s type 0x%x addr: 0x%p\n",
			wcn_dev->platform_fs.entrys[i].name,
			wcn_dev->platform_fs.entrys[i].flag,
			&wcn_dev->platform_fs.entrys[i]);

		wcn_dev->platform_fs.entrys[i].platform_proc_dir_entry =
			proc_create_data(
				wcn_dev->platform_fs.entrys[i].name,
				mode,
				wcn_dev->platform_fs.platform_proc_dir_entry,
				&wcn_platform_fs_fops,
				&wcn_dev->platform_fs.entrys[i]);
		wcn_dev->platform_fs.entrys[i].wcn_dev = wcn_dev;
	}
}

static inline void wcn_platform_fs_exit(struct wcn_device *wcn_dev)
{
	u8 i = 0;

	for (i = 0; i < MAX_PLATFORM_ENTRY_NUM; i++) {
		if (!wcn_dev->platform_fs.entrys[i].name)
			break;

		if (wcn_dev->platform_fs.entrys[i].flag != 0) {
			remove_proc_entry(wcn_dev->platform_fs.entrys[i].name,
				wcn_dev->platform_fs.platform_proc_dir_entry);
		}
	}

	remove_proc_entry(wcn_dev->name, NULL);
}

static int wcn_probe(struct platform_device *pdev)
{
	struct wcn_device *wcn_dev;
	static int first = 1;

	WCN_INFO("start!\n");

	wcn_dev = kzalloc(sizeof(struct wcn_device), GFP_KERNEL);
	if (!wcn_dev)
		return -ENOMEM;

	if (wcn_parse_dt(pdev, wcn_dev) < 0) {
		WCN_ERR("wcn_parse_dt Failed!\n");
		kfree(wcn_dev);
		return -EINVAL;
	}

	/* init the regs which can be init in the driver probe */
	wcn_config_ctrlreg(wcn_dev, 0, wcn_dev->ctrl_probe_num);

	mutex_init(&(wcn_dev->power_lock));

	wcn_platform_fs_init(wcn_dev);

	platform_set_drvdata(pdev, (void *)wcn_dev);

	if (strcmp(wcn_dev->name, WCN_MARLIN_DEV_NAME) == 0)
		s_wcn_device.btwf_device = wcn_dev;
	else if (strcmp(wcn_dev->name, WCN_GNSS_DEV_NAME) == 0)
		s_wcn_device.gnss_device = wcn_dev;

	/* default vddcon is 1.6V, we should set it to 1.2v */
	if (s_wcn_device.vddcon_voltage_setted == false) {
		s_wcn_device.vddcon_voltage_setted = true;
		wcn_power_set_vddcon(WCN_VDDCON_WORK_VOLTAGE);
		mutex_init(&(s_wcn_device.vddwcn_lock));
	}

	if (strcmp(wcn_dev->name, WCN_MARLIN_DEV_NAME) == 0) {
		mutex_init(&(wcn_dev->vddwifipa_lock));
		if (wcn_platform_chip_id() == AON_CHIP_ID_AA)
			wcn_power_set_vddwifipa(WCN_VDDWIFIPA_WORK_VOLTAGE);
		wcn_global_source_init();

		/* register ops */
		wcn_bus_init();

		proc_fs_init();
		log_dev_init();
		mdbg_atcmd_owner_init();
		wcn_marlin_write_efuse();
	} else if (strcmp(wcn_dev->name, WCN_GNSS_DEV_NAME) == 0)
		gnss_write_efuse_data();

	INIT_DELAYED_WORK(&wcn_dev->power_wq, wcn_power_wq);

	if (first) {
		/* Transceiver can't get into LP, so force deep sleep */
		if ((wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKLE) ||
		    (wcn_platform_chip_type() == WCN_PLATFORM_TYPE_SHARKL3)) {
			wcn_sys_soft_release();
			wcn_sys_deep_sleep_en();
		}
		first = 0;
	}

#if WCN_INTEGRATE_PLATFORM_DEBUG
	wcn_codes_debug();
#endif

	WCN_INFO("finish!\n");

	return 0;
}

static int  wcn_remove(struct platform_device *pdev)
{
	struct wcn_device *wcn_dev = platform_get_drvdata(pdev);

	if (wcn_dev)
		WCN_INFO("dev name %s\n", wcn_dev->name);

	wcn_platform_fs_exit(wcn_dev);
	kfree(wcn_dev);
	wcn_dev = NULL;

	return 0;
}

static void wcn_shutdown(struct platform_device *pdev)
{
	struct wcn_device *wcn_dev = platform_get_drvdata(pdev);

	if (wcn_dev && wcn_dev->wcn_open_status) {
		/* CPU hold on */
		wcn_proc_native_stop(wcn_dev);
		/* wifipa power off */
		if (strcmp(wcn_dev->name, WCN_MARLIN_DEV_NAME) == 0) {
			wcn_marlin_power_enable_vddwifipa(false);
			/* ASIC: disable vddcon, wifipa interval time > 1ms */
			usleep_range(VDDWIFIPA_VDDCON_MIN_INTERVAL_TIME,
				VDDWIFIPA_VDDCON_MAX_INTERVAL_TIME);
		}
		/* vddcon power off */
		wcn_power_enable_vddcon(false);
		wcn_sys_soft_reset();
		wcn_sys_soft_release();
		wcn_sys_deep_sleep_en();
		WCN_INFO("dev name %s\n", wcn_dev->name);
	}
}

static SIMPLE_DEV_PM_OPS(wcn_pm_ops, wcn_suspend, wcn_resume);
static struct platform_driver wcn_driver = {
	.driver = {
		.name = "wcn_integrate_platform",
		.pm = &wcn_pm_ops,
		.of_match_table = wcn_match_table,
	},
	.probe = wcn_probe,
	.remove = wcn_remove,
	.shutdown = wcn_shutdown,
};

static int __init wcn_init(void)
{
	WCN_INFO("entry!\n");

	return platform_driver_register(&wcn_driver);
}
late_initcall(wcn_init);

static void __exit wcn_exit(void)
{
	platform_driver_unregister(&wcn_driver);
}
module_exit(wcn_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Spreadtrum  WCN Integrate Platform Driver");
MODULE_AUTHOR("YaoGuang Chen <yaoguang.chen@spreadtrum.com>");
