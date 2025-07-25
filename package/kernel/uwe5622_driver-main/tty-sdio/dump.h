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

#ifndef __DUMP_H
#define __DUMP_H
#include <linux/time.h>
#include <linux/rtc.h>
#ifndef timespec
#define timespec timespec64
#define timespec_to_ns timespec64_to_ns
#define getnstimeofday ktime_get_real_ts64
#define timeval __kernel_old_timeval
#define rtc_time_to_tm rtc_time64_to_tm
#define timeval_to_ns ktime_to_ns
#endif

#define BT_MAX_DUMP_FRAME_LEN 2
#define BT_MAX_DUMP_DATA_LEN 20
#define BT_DATA_OUT 0
#define BT_DATA_IN 1
#define HCI_COMMAND 0x01
#define HCI_EVENT 0x04
#define HCI_COMMAND_STATUS 0x0f
#define HCI_COMMAND_COMPELET 0x0e

typedef struct bt_host_time {
	struct rtc_time rtc_t;
	struct timeval tv;
} bt_host_time;

typedef struct bt_host_data_dump {
	unsigned char tx[BT_MAX_DUMP_FRAME_LEN][BT_MAX_DUMP_DATA_LEN];
	bt_host_time txtime_t[BT_MAX_DUMP_FRAME_LEN];
	unsigned char rx[BT_MAX_DUMP_FRAME_LEN][BT_MAX_DUMP_DATA_LEN];
	bt_host_time rxtime_t[BT_MAX_DUMP_FRAME_LEN];
} bt_host_data_dump;

void bt_host_data_save(const unsigned char *buf, int count, unsigned char data_inout);
void bt_host_data_printf(void);

#endif
