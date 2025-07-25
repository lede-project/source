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

#ifndef __MTTY_H
#define __MTTY_H

#include <wcn_bus.h>

enum mtty_state {
	MTTY_STATE_CLOSE,
	MTTY_STATE_OPEN
};

enum mtty_log_level {
	MTTY_LOG_LEVEL_NONE,
	MTTY_LOG_LEVEL_VER,
};

struct mtty_init_data {
    char *name;
};

#define MTTY_DEV_MAX_NR     1
#define BT_TX_CHANNEL       3
//#define BT_TX_CHANNEL1    4
#define BT_RX_CHANNEL       17
#define BT_TX_INOUT         1
#define BT_RX_INOUT         0
#define BT_TX_POOL_SIZE     64  // the max buffer is 64
#define BT_RX_POOL_SIZE     1
#define BT_SDIO_HEAD_LEN    4

int marlin_sdio_write(const unsigned char *buf, int count);
#endif
