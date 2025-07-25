/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * Filename : gnss_common.h
 * Abstract : This file is a implementation for driver of gnss:
 *
 * Authors  : zhaohui.chen
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

#ifndef __GNSS_COMMON_H__
#define __GNSS_COMMON_H__

/* begin: address map on gnss side, operate by SDIO BUS */
/* set(s)/clear(c) */
#define GNSS_SET_OFFSET                 0x1000
#define GNSS_CLEAR_OFFSET               0x2000

#define GNSS_APB_BASE                   0x40bc8000
#define REG_GNSS_APB_MCU_AP_RST         (GNSS_APB_BASE + 0x0280) /* s/c */
#define BIT_GNSS_APB_MCU_AP_RST_SOFT    (1<<0)                   /* bit0 */

#define GNSS_INDIRECT_OP_REG            0x40b20000

#define GNSS_AHB_BASE                   0x40b18000
#define GNSS_ARCH_EB_REG                (GNSS_AHB_BASE + 0x084)
#define GNSS_ARCH_EB_REG_BYPASS         (1<<1)
#define GNSS_BOOTSTATUS_SIZE            0x4
#define GNSS_BOOTSTATUS_MAGIC           0x12345678
#define GNSS_EFUSE_DATA_SIZE            0xc
#define GNSS_CALI_DONE_FLAG             0x1314520

#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
#undef CONFIG_CHECK_DRIVER_BY_CHIPID
#define CONFIG_UWE5622
#endif
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
#define GNSS_CALI_ADDRESS               0x40aabf4c
#define GNSS_CALI_DATA_SIZE             0x1c
#else
#define GNSS_CALI_ADDRESS               0x40aaff4c
#define GNSS_CALI_DATA_SIZE             0x14
#endif

#ifdef CONFIG_UWE5622
#define GNSS_EFUSE_ADDRESS              0x40aabf40
#else
#define GNSS_EFUSE_ADDRESS              0x40aaff40
#endif

/*  GNSS assert workaround */
#ifdef CONFIG_UWE5622
#define GNSS_BOOTSTATUS_ADDRESS         0x40aabf6c
#else
#define GNSS_BOOTSTATUS_ADDRESS         0x40aaff6c
#endif

#else
#include "uwe562x_glb.h"

#define GNSS_CALI_ADDRESS_M3               0x40aaff4c
#define GNSS_CALI_ADDRESS_M3L              0x40aabf4c
#define GNSS_CALI_ADDRESS  SET_REG_VALUE(GNSS_CALI_ADDRESS_M3, \
									GNSS_CALI_ADDRESS_M3L, 0)

#define GNSS_CALI_DATA_SIZE_M3              0x14
#define GNSS_CALI_DATA_SIZE_M3L             0x1c
#define GNSS_CALI_DATA_SIZE SET_REG_VALUE(GNSS_CALI_DATA_SIZE_M3, \
									GNSS_CALI_DATA_SIZE_M3L, 0)

#define GNSS_EFUSE_ADDRESS_M3              0x40aaff40
#define GNSS_EFUSE_ADDRESS_M3L             0x40aabf40
#define GNSS_EFUSE_ADDRESS SET_REG_VALUE(GNSS_EFUSE_ADDRESS_M3, \
									GNSS_EFUSE_ADDRESS_M3L, 0)

#define GNSS_BOOTSTATUS_ADDRESS_M3         0x40aaff6c
#define GNSS_BOOTSTATUS_ADDRESS_M3L        0x40aabf6c
#define GNSS_BOOTSTATUS_ADDRESS SET_REG_VALUE(GNSS_BOOTSTATUS_ADDRESS_M3, \
									GNSS_BOOTSTATUS_ADDRESS_M3L, 0)

#endif

/* end: address map on gnss side */

#endif
