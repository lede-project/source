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
#ifndef __GNSS_DUMP_H__
#define __GNSS_DUMP_H__

#ifdef CONFIG_SC2342_INTEG
#define DUMP_PACKET_SIZE		(1024)
#define GNSS_SHARE_MEMORY_SIZE		(0x15a800)
#define GNSS_CP_IRAM_DATA_NUM		8192
#define GNSS_DUMP_IRAM_START_ADDR		0x18000000

/* ap aon registers start */
#define DUMP_REG_PMU_SLEEP_CTRL		0x402B00CC
#define DUMP_REG_PMU_SLEEP_STATUS	0x402B00D4

#define DUMP_REG_SYS_EN_STATUS		0x402e057c   /* sys_en */
#define DUMP_REG_WCN_SYS_CFG		0x402e0578   /* wcn_sys_cfg */
#define DUMP_REG_GNSS_CLK_STATUS	0x402d02d4   /* clk */

#define DUMP_REG_WCN_PD_STATUS		0x402b0100   /* wcn_pd */
#define DUMP_REG_BT_WIFI_PD		0x402b0104   /* bt_wifi_pd */
#define DUMP_REG_GNSS_STATUS		0x402b0108   /* gnss */
/* ap aon registers end */

/* cp reg start */
/* APB */
#define DUMP_REG_GNSS_APB_CTRL_ADDR	0xA0060000
#define DUMP_REG_GNSS_APB_CTRL_LEN	0x400
/* AHB */
#define DUMP_REG_GNSS_AHB_CTRL_ADDR	0xC0300000
#define DUMP_REG_GNSS_AHB_CTRL_LEN	0x400
/* Com_sys */
#define DUMP_COM_SYS_CTRL_ADDR		0xD0020800
#define DUMP_COM_SYS_CTRL_LEN		0x10

/* wcn_cp_clk */
#define DUMP_WCN_CP_CLK_CORE_ADDR	0xD0020000
#define DUMP_WCN_CP_CLK_LEN		0x100
/* cp reg end */

#define ANLG_WCN_WRITE_ADDR 0XFF4
#define ANLG_WCN_READ_ADDR 0XFFC

#else
#define GNSS_DRAM_ADDR      0x40a80000
#define GNSS_DRAM_SIZE      0x30000
#define GNSS_TE_MEM         0x40e40000
#define GNSS_TE_MEM_SIZE    0x30000
#define GNSS_BASE_AON_APB   0x4083c000
#define GNSS_BASE_AON_APB_SIZE 0x354
#define CTL_BASE_AON_CLOCK  0x40844200
#define CTL_BASE_AON_CLOCK_SIZE  0x144


#endif
int gnss_dump_mem(char flag);

#endif
