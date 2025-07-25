/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Dong Xiang <dong.xiang@spreadtrum.com>
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

#ifndef __SPRDWL_WORK_H__
#define __SPRDWL_WORK_H__

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#ifdef DFS_MASTER
#include "11h.h"
#endif

struct sprdwl_work {
	struct list_head list;
	struct sprdwl_vif *vif;
#define SPRDWL_WORK_NONE	0
#define SPRDWL_WORK_REG_MGMT	1
#define SPRDWL_WORK_DEAUTH	2
#define SPRDWL_WORK_DISASSOC	3
#define SPRDWL_WORK_MC_FILTER	4
#define SPRDWL_WORK_NOTIFY_IP	5
#define SPRDWL_WORK_BA_MGMT	6
#define SPRDWL_WORK_ADDBA 7
#define SPRDWL_WORK_DELBA 8
#ifdef DFS_MASTER
#define    SPRDWL_WORK_DFS   9
#endif
#define SPRDWL_ASSERT 10
#define SPRDWL_HANG_RECEIVED 11
#define SPRDWL_POP_MBUF 12
#define SPRDWL_TDLS_CMD 13
#define SPRDWL_SEND_CLOSE 14
#define SPRDWL_CMD_TX_DATA 15
#define SPRDWL_WORK_FW_PWR_DOWN 16
#define SPRDWL_WORK_HOST_WAKEUP_FW 17
#define SPRDWL_WORK_VOWIFI_DATA_PROTECTION 18
	u8 id;
	u32 len;
	u8 data[0];
};

struct sprdwl_reg_mgmt {
	u16 type;
	bool reg;
};

struct sprdwl_data2mgmt {
	struct sk_buff *skb;
	struct net_device *ndev;
};

struct sprdwl_tdls_work {
	u8 vif_ctx_id;
	u8 peer[ETH_ALEN];
	int oper;
};

struct sprdwl_assert_info {
	u8 cmd_id;
	u8 reason;
};

struct sprdwl_work *sprdwl_alloc_work(int len);
void sprdwl_queue_work(struct sprdwl_priv *priv,
		       struct sprdwl_work *sprdwl_work);
void sprdwl_cancle_work(struct sprdwl_priv *priv, struct sprdwl_vif *vif);
int sprdwl_init_work(struct sprdwl_priv *priv);
void sprdwl_deinit_work(struct sprdwl_priv *priv);

#endif
