/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __WCN_SIPC_H__
#define __WCN_SIPC_H__
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/sipc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/swcnblk.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "bus_common.h"
#include "mdbg_type.h"
#include "wcn_types.h"

#define SIPC_SBUF_HEAD_RESERV 4
#define SIPC_SBLOCK_HEAD_RESERV 32

enum wcn_sipc_chn_list {
	SIPC_ATCMD_TX = 0,
	SIPC_ATCMD_RX,
	SIPC_LOOPCHECK_RX,
	SIPC_ASSERT_RX,
	SIPC_LOG_RX,
	SIPC_BT_TX,
	SIPC_BT_RX,
	SIPC_FM_TX,
	SIPC_FM_RX,
	SIPC_WIFI_CMD_TX,
	SIPC_WIFI_CMD_RX,
	SIPC_WIFI_DATA0_TX,
	SIPC_WIFI_DATA0_RX,
	SIPC_WIFI_DATA1_TX,
	SIPC_WIFI_DATA1_RX,
	SIPC_CHN_NUM
};

struct wcn_sipc_data_ops {
	int (*sipc_write)(u8 channel, void *buf, int len);
	void (*sipc_notifer)(int event, void *data);
};

struct wcn_sipc_info_t {
	u32 sipc_chn_status;
	struct mutex status_lock;
};

struct sbuf_info {
	u8	bufid;
	u32	len;
	u32	bufnum;
	u32	txbufsize;
	u32	rxbufsize;
};

struct sblock_info {
	u32	txblocknum;
	u32	txblocksize;
	u32	rxblocknum;
	u32	rxblocksize;
};

struct sipc_chn_info {
	u8 index;
	u8 chntype;	/* sbuf/sblock */
	u8 chn;
	u8 dst;
	union {
		struct sbuf_info sbuf;
		struct sblock_info sblk;
	};
};
#endif
