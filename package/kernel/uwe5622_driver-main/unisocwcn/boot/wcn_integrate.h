/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * Filename : wcn_integrate_platform.h
 * Abstract : This file is a implementation for driver of integrated marlin:
 *                The marlin chip and GNSS chip were integrated with AP chipset.
 *
 * Authors	: yaoguang.chen
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

#ifndef __WCN_INTEGRATE_H__
#define __WCN_INTEGRATE_H__
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/file.h>
#include <linux/firmware.h>
#include <linux/fs.h>
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
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#include "linux/sipc.h"
#include "linux/sprd_otp.h"

#include "wcn_integrate_dev.h"

#define REGMAP_UPDATE_BITS_ENABLE 0	/* It can't work well. */

#define MDBG_CACHE_FLAG_VALUE	(0xcdcddcdc)

#define WCN_AON_CHIP_ID0 0x00E0
#define WCN_AON_CHIP_ID1 0x00E4
#define WCN_AON_PLATFORM_ID0 0x00E8
#define WCN_AON_PLATFORM_ID1 0x00EC
#define WCN_AON_CHIP_ID 0x00FC
#define WCN_AON_VERSION_ID 0x00F8

#define PIKE2_CHIP_ID0 0x32000000	/* 2 */
#define PIKE2_CHIP_ID1 0x50696B65	/* Pike */
#define SHARKLE_CHIP_ID0 0x6B4C4500	/* kle */
#define SHARKLE_CHIP_ID1 0x53686172	/* Shar */
#define SHARKL3_CHIP_ID0 0x6B4C3300	/* kl3 */
#define SHARKL3_CHIP_ID1 0x53686172	/* Shar */

#define AON_CHIP_ID_AA 0x96360000
#define AON_CHIP_ID_AC 0x96360002

struct platform_chip_id {
	u32 aon_chip_id0;
	u32 aon_chip_id1;
	u32 aon_platform_id0;
	u32 aon_platform_id1;
	u32 aon_chip_id;
};

enum {
	WCN_PLATFORM_TYPE_SHARKLE,
	WCN_PLATFORM_TYPE_PIKE2,
	WCN_PLATFORM_TYPE_SHARKL3,
	WCN_PLATFORM_TYPE,
};

enum wcn_gnss_type {
	WCN_GNSS_TYPE_INVALID,
	WCN_GNSS_TYPE_GL,
	WCN_GNSS_TYPE_BD,
};

enum wcn_aon_chip_id {
	WCN_AON_CHIP_ID_INVALID,
	WCN_SHARKLE_CHIP_AA_OR_AB,
	WCN_SHARKLE_CHIP_AC,
	WCN_SHARKLE_CHIP_AD,
	WCN_PIKE2_CHIP,
	WCN_PIKE2_CHIP_AA,
	WCN_PIKE2_CHIP_AB,
};

struct wcn_chip_type {
	u32 chipid;
	enum wcn_aon_chip_id chiptype;
};

#define WCN_SPECIAL_SHARME_MEM_ADDR	(0x0017c000)
struct wifi_special_share_mem {
	struct wifi_calibration calibration_data;
	u32 efuse[WIFI_EFUSE_BLOCK_COUNT];
	u32 calibration_flag;
};

struct marlin_special_share_mem {
	u32 init_status;
	u32 loopcheck_cnt;
};

struct gnss_special_share_mem {
	u32 calibration_flag;
	u32 efuse[GNSS_EFUSE_BLOCK_COUNT];
};

struct wcn_special_share_mem {
	/* 0x17c000 */
	struct wifi_special_share_mem wifi;
	/* 0x17cf54 */
	struct marlin_special_share_mem marlin;
	/* 0x17cf5c */
	u32 include_gnss;
	/* 0x17cf60 */
	u32 gnss_flag_addr;
	/* 0x17cf64 */
	u32 cp2_sleep_status;
	/* 0x17cf68 */
	u32 sleep_flag_addr;
	/* 0x17cf6c */
	u32 efuse_temper_magic;
	/* 0x17cf70 */
	u32 efuse_temper_val;
	/* 0x17cf74 */
	struct gnss_special_share_mem gnss;
};

typedef int (*marlin_reset_callback) (void *para);

extern struct platform_chip_id g_platform_chip_id;
extern char functionmask[8];
extern struct wcn_special_share_mem *s_wssm_phy_offset_p;

int wcn_write_data_to_phy_addr(phys_addr_t phy_addr,
			       void *src_data, u32 size);
int wcn_read_data_from_phy_addr(phys_addr_t phy_addr,
				void *tar_data, u32 size);
void *wcn_mem_ram_vmap_nocache(phys_addr_t start, size_t size,
			       unsigned int *count);
void wcn_mem_ram_unmap(const void *mem, unsigned int count);
u32 wcn_platform_chip_id(void);
u32 wcn_platform_chip_type(void);
u32 wcn_get_cp2_comm_rx_count(void);
phys_addr_t wcn_get_btwf_base_addr(void);
phys_addr_t wcn_get_btwf_sleep_addr(void);
phys_addr_t wcn_get_btwf_init_status_addr(void);
int wcn_get_btwf_power_status(void);
void wcn_regmap_read(struct regmap *cur_regmap,
			    u32 reg,
			    unsigned int *val);
void wcn_regmap_raw_write_bit(struct regmap *cur_regmap,
				     u32 reg,
				     unsigned int val);
struct regmap *wcn_get_btwf_regmap(u32 regmap_type);
struct regmap *wcn_get_gnss_regmap(u32 regmap_type);
phys_addr_t wcn_get_gnss_base_addr(void);
bool wcn_get_download_status(void);
void wcn_set_download_status(bool status);
u32 gnss_get_boot_status(void);
void gnss_set_boot_status(u32 status);
int marlin_get_module_status(void);
int wcn_get_module_status_changed(void);
void wcn_set_module_status_changed(bool status);
int marlin_reset_register_notify(void *callback_func, void *para);
int marlin_reset_unregister_notify(void);
void wcn_set_module_state(bool status);

int wcn_send_force_sleep_cmd(struct wcn_device *wcn_dev);
u32 wcn_get_sleep_status(struct wcn_device *wcn_dev, int force_sleep);
void wcn_power_domain_set(struct wcn_device *wcn_dev, u32 set_type);
void wcn_xtl_auto_sel(bool enable);
int wcn_power_enable_sys_domain(bool enable);
void wcn_sys_soft_reset(void);
void wcn_sys_ctrl_26m(bool enable);
void wcn_clock_ctrl(bool enable);
void wcn_sys_soft_release(void);
void wcn_sys_deep_sleep_en(void);
void wcn_power_set_vddcon(u32 value);
int wcn_power_enable_vddcon(bool enable);
void wcn_power_set_vddwifipa(u32 value);
int wcn_marlin_power_enable_vddwifipa(bool enable);
u32 wcn_parse_platform_chip_id(struct wcn_device *wcn_dev);
void mdbg_hold_cpu(void);
enum wcn_aon_chip_id wcn_get_aon_chip_id(void);
#endif
