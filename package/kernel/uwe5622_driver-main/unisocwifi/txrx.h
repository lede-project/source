/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
 * Jingxiang Li <Jingxiang.li@spreadtrum.com>
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

#ifndef __SPRDWL_TXRX_H__
#define __SPRDWL_TXRX_H__

struct sprdwl_vif;
struct sprdwl_priv;
struct sprdwl_msg_buf;

#define STAP_MODE_NPI 0
#define STAP_MODE_STA 0
#define STAP_MODE_AP 1
#define STAP_MODE_P2P 1
#define STAP_MODE_P2P_DEVICE 2
#define STAP_MODE_COEXI_NUM 4
#define STAP_MODE_OTHER STAP_MODE_STA

int sprdwl_send_data(struct sprdwl_vif *vif, struct sprdwl_msg_buf *msg,
		     struct sk_buff *skb, u8 offset);
int sprdwl_send_cmd(struct sprdwl_priv *priv, struct sprdwl_msg_buf *msg);

unsigned short sprdwl_rx_data_process(struct sprdwl_priv *priv,
				      unsigned char *msg);
unsigned short sprdwl_rx_event_process(struct sprdwl_priv *priv, u8 *msg);
unsigned short sprdwl_rx_rsp_process(struct sprdwl_priv *priv, u8 *msg);
void sprdwl_rx_skb_process(struct sprdwl_priv *priv, struct sk_buff *pskb);
void sprdwl_rx_send_cmd_process(struct sprdwl_priv *priv, void *data, int len,
				unsigned char id, unsigned char ctx_id);
#if defined FPGA_LOOPBACK_TEST
int sprdwl_send_data_fpga_test(struct sprdwl_priv *priv,
				struct sprdwl_msg_buf *msg,
				struct sk_buff *skb, u8 type, u8 offset);
#endif
#endif
