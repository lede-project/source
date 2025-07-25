/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Filename : slp_mgr.c
 * Abstract : This file is a implementation for  sleep manager
 *
 * Authors	: QI.SUN
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <marlin_platform.h>
#include <wcn_bus.h>

#include "mem_pd_mgr.h"
#include "wcn_glb_reg.h"
#include "../sleep/sdio_int.h"

#define MEM_PD_ERR -3
#define CP_NO_MEM_PD_TIMEROUT 2000
#define CP_TIMEROUT 30000
/* time out in waiting wifi to come up */

#define MEM_PD_UNIT_SIZE 0X8000/* 32k */
#define SDIO_CP_BASE_ADD 0X40400000/* 32k */
#define CP_MEM_OFFSET 0X00100000/* 32k */
#define DRAM_ADD 0X40580000
#define REGS_SPINLOCK_BASE 0X40850000
#define REG_SPINLOCK_SPINLOCKSTS_I (REGS_SPINLOCK_BASE + 0X0800)
#define UNLOCK_TOKEN (0X55AA10C5)
#define SPINLOCKSTS(I)  (REG_SPINLOCK_SPINLOCKSTS_I + 4*(I))
#define BT_BEGIN_OFFSET_SYNC 4
#define BT_END_OFFSET_SYNC 8
#define WIFI_BEGIN_OFFSET_SYNC 12
#define WIFI_END_OFFSET_SYNC 16

struct mem_pd_t mem_pd;
struct mem_pd_meminfo_t mem_info_cp;

/* return 0, no download ini; return 1, need download ini */
unsigned int mem_pd_wifi_state(void)
{
	unsigned int ret = 0;

	if (mem_pd.cp_version)
		return 0;
	ret = mem_pd.wifi_state;

	return ret;
}

unsigned int mem_pd_spinlock_lock(int id)
{
	int ret = 0;
	int i = 0;
	unsigned int reg_val = 0;

	do {
		i++;
		ret = sprdwcn_bus_reg_read(SPINLOCKSTS(id), &reg_val, 4);
		if (!(ret == 0)) {
			MEM_PD_MGR_INFO(" sdiohal_dt_read lock error !");
			return ret;
		}
		if (reg_val == 0)
			break;
		if (i > 200) {
			i = 0;
			MEM_PD_MGR_INFO("get spinlock time out\n");
		}
	} while (i);

	return 0;
}

unsigned int mem_pd_spinlock_unlock(int id)
{
	int ret = 0;
	unsigned int reg_val = UNLOCK_TOKEN;

	ret = sprdwcn_bus_reg_write(SPINLOCKSTS(id),  &reg_val, 4);
	if (!(ret == 0)) {
		MEM_PD_MGR_INFO(" dt_write lock error !");
		return ret;
	}

	return 0;
}

/* bit_start FORCE SHUTDOWN IRAM [16...31]*32K=512K
 * and bit_start++ bit_cnt how many 32k
 */
static int mem_pd_power_switch(int subsys, int val)
{
	int ret = 0;
	unsigned int reg_val = 0;
	unsigned int wif_bt_mem_cfg = 0;
	unsigned int bt_ram_mask;

	/* unsigned int mem_pd_power_delay; */
	MEM_PD_MGR_INFO("%s", __func__);
	/* get the lock to write the register, use spinlock id=0 */
	mem_pd_spinlock_lock(0);
	/* CP reset write 1, mask mem CGG reg */
	/* should write 0 */
	switch (subsys) {
	case MARLIN_WIFI:
		if (val) {
			if (mem_pd.wifi_state == THREAD_DELETE) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO("wifi_state=0, forbid on");
				return ret;
			}
			/* wifi iram mem pd range */
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG1;
			ret = sprdwcn_bus_reg_read(wif_bt_mem_cfg, &reg_val, 4);
			if (!(ret == 0)) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO(" sdiohal_dt_read error !");
				return ret;
			}
			if (reg_val & mem_info_cp.wifi_iram_mask) {
				/* val =1 ,powerdown */
				reg_val &= ~(mem_info_cp.wifi_iram_mask);
				/* set bit_start ,mem power down */
				ret = sprdwcn_bus_reg_write(
				wif_bt_mem_cfg, &reg_val, 4);
				if (!(ret == 0)) {
					mem_pd_spinlock_unlock(0);
					MEM_PD_MGR_INFO("dt_write error !");
					return ret;
				}
			}
			MEM_PD_MGR_INFO("wifi irammem power on");
			/* wifi dram mem pd range */
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
#else
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG3;
#endif

#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
			if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L)
				wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
			else
				wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG3;
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

			ret = sprdwcn_bus_reg_read(wif_bt_mem_cfg, &reg_val, 4);
			if (!(ret == 0)) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO("sdiohal_dt_read error !");
				return ret;
			}
			if (reg_val & mem_info_cp.wifi_dram_mask) {
				/* val =1 ,powerdown */
				reg_val &= ~(mem_info_cp.wifi_dram_mask);
				/* set bit_start ,mem power down */
				ret = sprdwcn_bus_reg_write(
				wif_bt_mem_cfg, &reg_val, 4);
				if (!(ret == 0)) {
					mem_pd_spinlock_unlock(0);
					MEM_PD_MGR_INFO("dt_write error !");
					return ret;
				}
			}
			MEM_PD_MGR_INFO("wifi drammem power on");
		} else {
			if (mem_pd.wifi_state == THREAD_CREATE) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO("wifi_state=1, forbid off");
				return ret;
			}
			/* wifi iram mem pd range */
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG1;
			ret = sprdwcn_bus_reg_read(wif_bt_mem_cfg, &reg_val, 4);
			if (!(ret == 0)) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO("dt read error !");
				return ret;
			}
			if (!(reg_val & mem_info_cp.wifi_iram_mask)) {
				reg_val |=  (mem_info_cp.wifi_iram_mask);
				/* clear bit_start ,mem power on */
				ret = sprdwcn_bus_reg_write(
				wif_bt_mem_cfg, &reg_val, 4);
				if (!(ret == 0)) {
					mem_pd_spinlock_unlock(0);
					MEM_PD_MGR_INFO("dt write error !");
					return ret;
				}
			}
			MEM_PD_MGR_INFO("wifi irammem power down");
			/* wifi dram mem pd range */

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
#else
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG3;
#endif

#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
		if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L)
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
		else
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG3;
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

			ret = sprdwcn_bus_reg_read(wif_bt_mem_cfg, &reg_val, 4);
			if (!(ret == 0)) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO(" sdio read error !");
				return ret;
			}
			if (!(reg_val & mem_info_cp.wifi_dram_mask)) {
				reg_val |= (mem_info_cp.wifi_dram_mask);
				/* clear bit_start ,mem power on */
				ret = sprdwcn_bus_reg_write(
					wif_bt_mem_cfg, &reg_val, 4);
				if (!(ret == 0)) {
					mem_pd_spinlock_unlock(0);
					MEM_PD_MGR_INFO("dt write error !");
					return ret;
				}
			}
			MEM_PD_MGR_INFO("wifi drammem power down");
		}
	break;
	case MARLIN_BLUETOOTH:
		if (val) {
			if (mem_pd.bt_state == THREAD_DELETE) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO("bt_state=0, forbid on");
				return ret;
			}
			/* bt iram mem pd range */
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
			bt_ram_mask = mem_info_cp.bt_dram_mask;
#else
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG1;
			bt_ram_mask = mem_info_cp.bt_iram_mask;
#endif

#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

			if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
				wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
				bt_ram_mask = mem_info_cp.bt_dram_mask;
			} else {
				wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG1;
				bt_ram_mask = mem_info_cp.bt_iram_mask;
			}

#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

			ret = sprdwcn_bus_reg_read(wif_bt_mem_cfg, &reg_val, 4);
			if (!(ret == 0)) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO(" sdio dt read error !");
				return ret;
			}
			if (reg_val & (bt_ram_mask)) {
				/* val =1 ,powerdown */
				reg_val &= ~(bt_ram_mask);
				/* set bit_start ,mem power down */
				ret = sprdwcn_bus_reg_write(
					wif_bt_mem_cfg, &reg_val, 4);
				if (!(ret == 0)) {
					mem_pd_spinlock_unlock(0);
					MEM_PD_MGR_INFO(" error !");
					return ret;
				}
			}
			MEM_PD_MGR_INFO("bt irampower on");
		} else {
			if (mem_pd.bt_state == THREAD_CREATE) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO("bt_state=1, forbid off");
				return ret;
			}
			/* bt iram mem pd range */
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
			bt_ram_mask = mem_info_cp.bt_dram_mask;
#else
			wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG1;
			bt_ram_mask = mem_info_cp.bt_iram_mask;
#endif
#else
			if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
				wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG2;
				bt_ram_mask = mem_info_cp.bt_dram_mask;
			} else{
				wif_bt_mem_cfg = REG_AON_APB_BTWF_MEM_CGG1;
				bt_ram_mask = mem_info_cp.bt_iram_mask;
			}
#endif
			ret = sprdwcn_bus_reg_read(wif_bt_mem_cfg, &reg_val, 4);
			if (!(ret == 0)) {
				mem_pd_spinlock_unlock(0);
				MEM_PD_MGR_INFO(" error !");
				return ret;
			}
			if (reg_val & bt_ram_mask) {
				/* val =1 ,powerdown */
				MEM_PD_MGR_INFO(" mem reg val =1 !");
			} else{
				reg_val |= (bt_ram_mask);
				/* clear bit_start ,mem power on */
				ret = sprdwcn_bus_reg_write(
				wif_bt_mem_cfg, &reg_val, 4);
				if (!(ret == 0)) {
					mem_pd_spinlock_unlock(0);
					MEM_PD_MGR_INFO(" error !");
					return ret;
				}
			}
			MEM_PD_MGR_INFO("bt iram power down");
		}
	break;
	default:
	break;
	}
	mem_pd_spinlock_unlock(0);

	return 0;
}

static int inform_cp_wifi_download(void)
{
	sdio_ap_int_cp0(WIFI_BIN_DOWNLOAD);
	MEM_PD_MGR_INFO("%s\n", __func__);

	return 0;
}
static int inform_cp_bt_download(void)
{
	sdio_ap_int_cp0(BT_BIN_DOWNLOAD);
	MEM_PD_MGR_INFO("%s", __func__);

	return 0;
}

static void wifi_cp_open(void)
{
	MEM_PD_MGR_INFO(" wifi_open int from cp");
	complete(&(mem_pd.wifi_open_completion));
}
static void wifi_cp_close(void)
{
	complete(&(mem_pd.wifi_cls_cpl));
	MEM_PD_MGR_INFO("wifi_thread_delete int");
}

static void bt_cp_open(void)
{
	MEM_PD_MGR_INFO("bt_open int from cp");
	complete(&(mem_pd.bt_open_completion));
}
static void bt_cp_close(void)
{
	complete(&(mem_pd.bt_close_completion));
	MEM_PD_MGR_INFO("bt_thread_delete int");
}
static void save_bin_cp_ready(void)
{
	if (mem_pd.cp_mem_all_off == 0) {
		complete(&(mem_pd.save_bin_completion));
		MEM_PD_MGR_INFO("%s ,cp while(1) state", __func__);
		mem_pd.cp_mem_all_off = 1;
		return;
	}
	MEM_PD_MGR_INFO("%s ,wifi/bt power down, wait event", __func__);
}
static int mem_pd_pub_int_RegCb(void)
{
	sdio_pub_int_RegCb(WIFI_OPEN, (PUB_INT_ISR)wifi_cp_open);
	sdio_pub_int_RegCb(WIFI_CLOSE, (PUB_INT_ISR)wifi_cp_close);
	sdio_pub_int_RegCb(BT_OPEN, (PUB_INT_ISR)bt_cp_open);
	sdio_pub_int_RegCb(BT_CLOSE, (PUB_INT_ISR)bt_cp_close);
	sdio_pub_int_RegCb(MEM_SAVE_BIN, (PUB_INT_ISR)save_bin_cp_ready);

	return 0;
}

static int sdio_read_mem_from_cp(void)
{
	int err = 0;

	MEM_PD_MGR_INFO("%s  read wifi/bt mem bin", __func__);
	err = sprdwcn_bus_direct_read(mem_info_cp.wifi_begin_addr,
				      mem_pd.wifi_mem, mem_info_cp.wifi_size);
	if (err < 0) {
		pr_err("%s wifi save mem bin error:%d", __func__, err);
		return err;
	}
	err = sprdwcn_bus_direct_read(mem_info_cp.bt_begin_addr,
				      mem_pd.bt_mem, mem_info_cp.bt_size);
	if (err < 0) {
		pr_err("%s bt save mem bin error:%d", __func__, err);
		return err;
	}
	MEM_PD_MGR_INFO("%s save wifi/bt mem bin ok", __func__);

	return 0;
}
static int sdio_ap_int_cp_save_cp_mem(void)
{
	sdio_ap_int_cp0(SAVE_CP_MEM);
	MEM_PD_MGR_INFO("%s, cp while(1) break", __func__);

	return 0;
}

static int mem_pd_read_add_from_cp(void)
{
	int ret;
	unsigned int bt_begin, bt_end, wifi_begin, wifi_end;

	ret = sprdwcn_bus_reg_read(SYNC_ADDR + BT_BEGIN_OFFSET_SYNC,
				   &bt_begin, 4);
	if (ret < 0) {
		MEM_PD_MGR_ERR("%s mem_pd read  bt begin addr error:%d\n",
			       __func__, ret);
		return ret;
	}
	ret = sprdwcn_bus_reg_read(SYNC_ADDR + BT_END_OFFSET_SYNC, &bt_end, 4);
	if (ret < 0) {
		MEM_PD_MGR_ERR("%s mem_pd read  bt end addr error:%d\n",
			       __func__, ret);
		return ret;
	}
	ret = sprdwcn_bus_reg_read(SYNC_ADDR + WIFI_BEGIN_OFFSET_SYNC,
				   &wifi_begin, 4);
	if (ret < 0) {
		MEM_PD_MGR_ERR("%s mem_pd read  wifi begin addr error:%d\n",
			       __func__, ret);
		return ret;
	}
	ret = sprdwcn_bus_reg_read(SYNC_ADDR + WIFI_END_OFFSET_SYNC,
				   &wifi_end, 4);
	if (ret < 0) {
		MEM_PD_MGR_ERR("%s mem_pd read  wifi end addr error:%d\n",
			       __func__, ret);
		return ret;
	}
	mem_info_cp.chip_id = marlin_get_wcn_chipid();
	mem_info_cp.bt_begin_addr = bt_begin + SDIO_CP_BASE_ADD;
	mem_info_cp.bt_end_addr = bt_end + SDIO_CP_BASE_ADD;
	mem_info_cp.bt_size = bt_end - bt_begin;
	mem_info_cp.wifi_begin_addr = wifi_begin + SDIO_CP_BASE_ADD;
	mem_info_cp.wifi_end_addr = wifi_end + SDIO_CP_BASE_ADD;
	mem_info_cp.wifi_size = wifi_end - wifi_begin;

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
	mem_info_cp.bt_iram_mask = 0;
	mem_info_cp.bt_dram_mask = 0xC00;
	mem_info_cp.wifi_iram_mask = 0xFC00;
	mem_info_cp.wifi_dram_mask = 0x80;
#else
	mem_info_cp.bt_iram_mask = 0x0FE00000;
	mem_info_cp.bt_dram_mask = 0;
	mem_info_cp.wifi_iram_mask = 0xE0000000;
	mem_info_cp.wifi_dram_mask = 0x1FFF0000;
#endif

#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
		mem_info_cp.bt_iram_mask = 0;
		mem_info_cp.bt_dram_mask = 0xC00;
		mem_info_cp.wifi_iram_mask = 0xFC00;
		mem_info_cp.wifi_dram_mask = 0x80;
	} else {
		mem_info_cp.bt_iram_mask = 0x0FE00000;
		mem_info_cp.bt_dram_mask = 0;
		mem_info_cp.wifi_iram_mask = 0xE0000000;
		mem_info_cp.wifi_dram_mask = 0x1FFF0000;
	}

#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

	mem_pd.wifi_mem = kmalloc(mem_info_cp.wifi_size, GFP_KERNEL);
	if (!mem_pd.wifi_mem) {
		MEM_PD_MGR_INFO("mem pd wifi save buff malloc Failed.");
		return MEM_PD_ERR;
	}
	mem_pd.bt_mem = kmalloc(mem_info_cp.bt_size, GFP_KERNEL);
	if (!mem_pd.bt_mem) {
		kfree(mem_pd.wifi_mem);
		MEM_PD_MGR_INFO("mem pd bt save buff malloc Failed.");
		return MEM_PD_ERR;
	}
	mem_pd.wifi_clear = kmalloc(mem_info_cp.wifi_size, GFP_KERNEL);
	if (!mem_pd.wifi_clear) {
		kfree(mem_pd.wifi_mem);
		kfree(mem_pd.bt_mem);
		MEM_PD_MGR_INFO("mem pd clear buff malloc Failed.");
		return MEM_PD_ERR;
	}
	mem_pd.bt_clear = kmalloc(mem_info_cp.bt_size, GFP_KERNEL);
	if (!mem_pd.bt_clear) {
		kfree(mem_pd.wifi_mem);
		kfree(mem_pd.bt_mem);
		kfree(mem_pd.wifi_clear);
		MEM_PD_MGR_INFO("mem pd clear buff malloc Failed.");
		return MEM_PD_ERR;
	}
	memset(mem_pd.wifi_clear, 0x0, mem_info_cp.wifi_size);
	memset(mem_pd.bt_clear, 0x0, mem_info_cp.bt_size);

	return 0;
}

int mem_pd_save_bin(void)
{
	/* mutex_lock(&(mem_pd.mem_pd_lock)); */
	MEM_PD_MGR_INFO("%s entry", __func__);
	if (wait_for_completion_timeout(
		&(mem_pd.save_bin_completion),
		msecs_to_jiffies(CP_NO_MEM_PD_TIMEROUT)) <= 0) {
		MEM_PD_MGR_INFO("cp version has no mem_pd function");
		/* mutex_unlock(&(mem_pd.mem_pd_lock)); */
		mem_pd.cp_version = 1;
		return 0;
	}
	if (mem_pd.bin_save_done == 0) {
		mem_pd.bin_save_done = 1;
		MEM_PD_MGR_INFO("cp first power on");
		mem_pd_read_add_from_cp();
		sdio_read_mem_from_cp();
		/* save to char[] */
	} else
		MEM_PD_MGR_INFO("cp not first power on %s do nothing",
							__func__);
	mem_pd_power_switch(MARLIN_WIFI, FALSE);
	mem_pd_power_switch(MARLIN_BLUETOOTH, FALSE);
	MEM_PD_MGR_INFO("wifi/bt mem power down");
	sdio_ap_int_cp_save_cp_mem();
	/* save done , AP inform cp by INT. */
	/* mutex_unlock(&(mem_pd.mem_pd_lock)); */

	return 0;
}

static int ap_int_cp_wifi_bin_done(int subsys)
{
	switch (subsys) {
	case MARLIN_WIFI:
		inform_cp_wifi_download();
	break;
	case MARLIN_BLUETOOTH:
		inform_cp_bt_download();
	break;
	default:
	return MEM_PD_ERR;
	}

	return 0;
}

int test_mem_clrear(int subsys)
{
	int err;

	switch (subsys) {
	case MARLIN_WIFI:
		err = sprdwcn_bus_direct_write(mem_info_cp.wifi_begin_addr,
					       mem_pd.wifi_clear,
					       mem_info_cp.wifi_size);
		if (err < 0) {
			pr_err("%s wifi down bin error:%d", __func__, err);
			return err;
		}
	break;
	case MARLIN_BLUETOOTH:
		err = sprdwcn_bus_direct_write(mem_info_cp.bt_begin_addr,
					       mem_pd.bt_clear,
					       mem_info_cp.bt_size);
		if (err < 0) {
			pr_err("%s bt down mem bin error:%d", __func__, err);
			return err;
		}
	break;
	default:
	return MEM_PD_ERR;
	}

	return 0;
}

static int mem_pd_download_mem_bin(int subsys)
{
	int err;
	unsigned int addr = 0;
	char *mem;
	unsigned int len = 0;

	MEM_PD_MGR_INFO("%s", __func__);
	switch (subsys) {
	case MARLIN_WIFI:
		addr = mem_info_cp.wifi_begin_addr;
		mem = mem_pd.wifi_mem;
		len = mem_info_cp.wifi_size;
		MEM_PD_MGR_INFO("%s, wifi mem download ok", __func__);
	break;
	case MARLIN_BLUETOOTH:
		addr = mem_info_cp.bt_begin_addr;
		mem = mem_pd.bt_mem;
		len = mem_info_cp.bt_size;
		MEM_PD_MGR_INFO("%s, bt mem download ok", __func__);
	break;
	default:
		return MEM_PD_ERR;
	}
	err = sprdwcn_bus_direct_write(addr, mem, len);
	if (err < 0) {
		pr_err("%s download mem bin error:%d", __func__, err);
		return err;
	}

	return 0;
}
int mem_pd_mgr(int subsys, int val)
{
	if (mem_pd.cp_version)
		return 0;
	MEM_PD_MGR_INFO("%s mem on/off", __func__);
	if ((subsys != MARLIN_WIFI) && (subsys != MARLIN_BLUETOOTH)) {
		MEM_PD_MGR_INFO("subsys:%d, do nothing, return ok", subsys);
		return 0;
	}
	mutex_lock(&(mem_pd.mem_pd_lock));
	switch (subsys) {
	case MARLIN_WIFI:
		MEM_PD_MGR_INFO("marlin wifi state:%d, subsys %d power %d",
				mem_pd.wifi_state, subsys, val);
		if (val) {
			if (mem_pd.wifi_state != THREAD_DELETE) {
				MEM_PD_MGR_INFO("wifi opened ,do nothing");
				goto out;
			}
			mem_pd.wifi_state = THREAD_CREATE;
			mem_pd_power_switch(subsys, val);
			mem_pd_download_mem_bin(subsys);
			/* avoid fake interrupt , and reinit */
			reinit_completion(&(mem_pd.wifi_open_completion));
			ap_int_cp_wifi_bin_done(subsys);
			if (wait_for_completion_timeout(
				&(mem_pd.wifi_open_completion),
			msecs_to_jiffies(CP_TIMEROUT))
			<= 0) {
				MEM_PD_MGR_ERR("wifi creat fail");
				goto mem_pd_err;
			}
			MEM_PD_MGR_INFO("cp wifi creat thread ok");
		} else {
			if (mem_pd.wifi_state != THREAD_CREATE) {
				MEM_PD_MGR_INFO("wifi closed ,do nothing");
				goto out;
			}
			/* avoid fake interrupt , and reinit */
			reinit_completion(&(mem_pd.wifi_cls_cpl));
			sprdwcn_bus_aon_writeb(0x1b0, 0x10);
			/* instead of cp wifi delet thread ,inform sdio. */
			if (wait_for_completion_timeout(&(mem_pd.wifi_cls_cpl),
						msecs_to_jiffies(CP_TIMEROUT))
			<= 0) {
				MEM_PD_MGR_ERR("wifi delete fail");
				goto mem_pd_err;
			}
			mem_pd.wifi_state = THREAD_DELETE;
			mem_pd_power_switch(subsys, val);
			MEM_PD_MGR_INFO("cp wifi delete thread ok");
		}
		break;
	case MARLIN_BLUETOOTH:
		MEM_PD_MGR_INFO("marlin bt state:%d, subsys %d power %d",
				mem_pd.bt_state, subsys, val);
		if (val) {
			if (mem_pd.bt_state != THREAD_DELETE) {
				MEM_PD_MGR_INFO("bt opened ,do nothing\n");
				goto out;
			}
			mem_pd.bt_state = THREAD_CREATE;
			mem_pd_power_switch(subsys, val);
			mem_pd_download_mem_bin(subsys);
			/* avoid fake interrupt, and reinit */
			reinit_completion(&(mem_pd.bt_open_completion));
			ap_int_cp_wifi_bin_done(subsys);
		if (wait_for_completion_timeout(&(mem_pd.bt_open_completion),
			msecs_to_jiffies(CP_TIMEROUT)) <= 0) {
			MEM_PD_MGR_ERR("cp bt creat thread fail");
			goto mem_pd_err;
		}
			MEM_PD_MGR_INFO("cp bt creat thread ok");
		} else {
			if (mem_pd.bt_state != THREAD_CREATE) {
				MEM_PD_MGR_INFO("bt closed ,do nothing");
				goto out;
			}
			/* avoid fake interrupt , and reinit */
			/* reinit_completion(&(mem_pd.bt_close_completion)); */
			if (wait_for_completion_timeout(
				&(mem_pd.bt_close_completion),
			msecs_to_jiffies(CP_TIMEROUT))
				<= 0) {
				MEM_PD_MGR_ERR("bt delete fail");
				goto mem_pd_err;
			}
			mem_pd.bt_state = THREAD_DELETE;
			mem_pd_power_switch(subsys, val);
			MEM_PD_MGR_INFO("cp bt delete thread ok");
		}
		break;
	default:
		MEM_PD_MGR_INFO("%s switch default", __func__);
	}

out:
		mutex_unlock(&(mem_pd.mem_pd_lock));

		return 0;

mem_pd_err:
		mutex_unlock(&(mem_pd.mem_pd_lock));
		MEM_PD_MGR_ERR("%s return error", __func__);

		return -1;
}
int mem_pd_poweroff_deinit(void)
{
	if (mem_pd.cp_version)
		return 0;
	MEM_PD_MGR_INFO("mem_pd_chip_poweroff_deinit");
	mem_pd.wifi_state = 0;
	mem_pd.bt_state = 0;
	mem_pd.cp_version = 0;
	mem_pd.cp_mem_all_off = 0;
	reinit_completion(&(mem_pd.wifi_open_completion));
	reinit_completion(&(mem_pd.wifi_cls_cpl));
	reinit_completion(&(mem_pd.bt_open_completion));
	reinit_completion(&(mem_pd.bt_close_completion));
	reinit_completion(&(mem_pd.save_bin_completion));

	return 0;
}
int mem_pd_init(void)
{
	MEM_PD_MGR_INFO("%s enter", __func__);
	mutex_init(&(mem_pd.mem_pd_lock));
	init_completion(&(mem_pd.wifi_open_completion));
	init_completion(&(mem_pd.wifi_cls_cpl));
	init_completion(&(mem_pd.bt_open_completion));
	init_completion(&(mem_pd.bt_close_completion));
	init_completion(&(mem_pd.save_bin_completion));
	mem_pd_pub_int_RegCb();
	/* mem_pd.wifi_state = 0; */
	/* mem_pd.bt_state = 0; */
	/* mem_pd.cp_version = 0; */
	/* mem_pd.cp_mem_all_off = 0; */

	MEM_PD_MGR_INFO("%s ok!", __func__);

	return 0;
}

int mem_pd_exit(void)
{
	MEM_PD_MGR_INFO("%s enter", __func__);
	/* atomic_set(&(slp_mgr.cp2_state), STAY_SLPING); */
	/* sleep_active_modules = 0; */
	/* wake_cnt = 0; */
	mutex_destroy(&(mem_pd.mem_pd_lock));
	/* mutex_destroy(&(slp_mgr.wakeup_lock)); */
	kfree(mem_pd.wifi_mem);
	mem_pd.wifi_mem = NULL;
	kfree(mem_pd.bt_mem);
	mem_pd.bt_mem = NULL;
	kfree(mem_pd.wifi_clear);
	kfree(mem_pd.bt_clear);
	mem_pd.wifi_clear = NULL;
	mem_pd.bt_clear = NULL;
	MEM_PD_MGR_INFO("%s ok!", __func__);

	return 0;
}

