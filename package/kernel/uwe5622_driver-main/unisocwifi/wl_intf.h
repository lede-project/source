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

#ifndef __WL_INTF_H__
#define __WL_INTF_H__

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/cpufreq.h>
#include "wl_core.h"
#include <wcn_bus.h>

#define HW_TYPE_SDIO 0
#define HW_TYPE_PCIE 1
#define HW_TYPE_SIPC 2
#define HW_TYPE_USB 3

#define SDIO_RX_CMD_PORT	22
#define SDIO_RX_PKT_LOG_PORT	23
/*use port 24 because fifo_len = 8*/
#define SDIO_RX_DATA_PORT	24
#define SDIO_TX_CMD_PORT	8
/*use port 10 because fifo_len = 8*/
#define SDIO_TX_DATA_PORT	10

#define PCIE_RX_CMD_PORT	22
#define PCIE_RX_DATA_PORT	23
#define PCIE_TX_CMD_PORT	2
#define PCIE_TX_DATA_PORT	3

#define USB_RX_CMD_PORT	20
#define USB_RX_PKT_LOG_PORT	21
#define USB_RX_DATA_PORT	22
#define USB_TX_CMD_PORT	4
#define USB_TX_DATA_PORT	6

#define MSDU_DSCR_RSVD	5

#define DEL_LUT_INDEX 0
#define ADD_LUT_INDEX 1
#define UPD_LUT_INDEX 2

#define BOOST_TXNUM_LEVEL	16
#define BOOST_RXNUM_LEVEL	16

#ifdef SPRDWL_TX_SELF
struct sprdwl_tx_buf {
	unsigned char   *base;
	unsigned short  buf_len;
	unsigned short  curpos;
	int change_size;
};
#endif

#define MAX_CHN_NUM 16
struct sprdwl_intf_ops {
	unsigned int max_num;
	void *intf;
	struct mchn_ops_t *hif_ops;
};

struct sdiohal_puh {
	unsigned int pad:6;
	unsigned int check_sum:1;
	unsigned int len:16;
	unsigned int eof:1;
	unsigned int subtype:4;
	unsigned int type:4;
}; /* 32bits public header */

struct tx_msdu_dscr {
	struct {
		/*0:cmd, 1:event, 2:normal data,*/
		/*3:special data, 4:PCIE remote addr*/
		unsigned char type:3;
		/*direction of address buffer of cmd/event,*/
		/*0:Tx, 1:Rx*/
		unsigned char direction_ind:1;
		unsigned char need_rsp:1;
		/*ctxt_id*/
		unsigned char interface:3;
	} common;
	unsigned char offset;
	struct {
		/*1:need HW to do checksum*/
		unsigned char checksum_offload:1;
		/*0:udp, 1:tcp*/
		unsigned char checksum_type:1;
		/*1:use SW rate,no aggregation 0:normal*/
		unsigned char sw_rate:1;
		/*WDS frame*/
		unsigned char wds:1;
		/*1:frame sent from SWQ to MH,
		 *0:frame sent from TXQ to MH,
		   default:0
		 */
		unsigned char swq_flag:1;
		unsigned char rsvd:1;
		/*used by PCIe address buffer, need set default:0*/
		unsigned char next_buffer_type:1;
		/*used by PCIe address buffer, need set default:0*/
		unsigned char pcie_mh_readcomp:1;
	} tx_ctrl;
	unsigned short pkt_len;
	struct {
		unsigned char msdu_tid:4;
		unsigned char mac_data_offset:4;
	} buffer_info;
	unsigned char sta_lut_index;
	unsigned char color_bit:2;
	unsigned short rsvd:14;
	unsigned short tcp_udp_header_offset;
} __packed;

struct pcie_addr_buffer {
	struct {
		unsigned char type:3;
		/*direction of address buffer of cmd/event,*/
		/*0:Tx, 1:Rx*/
		unsigned char direction_ind:1;
		unsigned char need_rsp:1;
		unsigned char interface:3;
	} common;
	unsigned short number;
	unsigned char offset;
	struct {
		unsigned char rsvd:6;
		unsigned char buffer_type:1;
		unsigned char buffer_inuse:1;
	} buffer_ctrl;
	unsigned char pcie_addr[0][5];
} __packed;

struct txc_addr_buff {
	struct {
		unsigned char type:3;
		unsigned char direction_ind:1;
		unsigned char need_rsp:1;
		unsigned char interface:3;
	} common;
	/*addr offset from common*/
	unsigned char offset;
	struct {
		unsigned char cksum:1;
		unsigned char cksum_type:1;
		unsigned char sw_ctrl:1;
		unsigned char wds:1;
		unsigned char swq_flag:1;
		unsigned char rsvd:1;
		/*0: data buffer, 1: address buffer*/
		unsigned char next_buffer_type:1;
		/*used only by address buffer*/
		/*0: MH process done, 1: before send to MH*/
		unsigned char mh_done:1;
	} tx_ctrl;
	unsigned short number;
	unsigned short rsvd;
} __packed;

#define GET_MSG_BUF(ptr) \
	((struct sprdwl_msg_buf *) \
	(*(unsigned long *)((ptr)->buf - sizeof(unsigned long *))))

#if defined(MORE_DEBUG)
#define STATS_COUNT 200

#define UPDATE_TX_PACKETS(dev, tx_count, tx_bytes) do { \
	(dev)->stats.tx_packets += (tx_count); \
	(dev)->stats.tx_bytes += (tx_bytes); \
	(dev)->stats.gap_num += (tx_count); \
} while (0)
#endif

#define DOT11_ADDBA_POLICY_DELAYED	0 /* delayed BA policy */

#define DOT11_ADDBA_POLICY_IMMEDIATE	1 /* immediate BA policy */

enum addba_req_result {
	ADDBA_REQ_RESULT_SUCCESS,
	ADDBA_REQ_RESULT_FAIL,
	ADDBA_REQ_RESULT_TIMEOUT,
	ADDBA_REQ_RESULT_DECLINE,
};

struct ieeetypes_addba_param {
	u16 amsdu_permit : 1;
	u16 ba_policy : 1;
	u16 tid : 4;
	u16 buffer_size : 10;
} __packed;

struct ieeetypes_delba_param {
	u16 reserved : 11;
	u16 initiator : 1;
	u16 tid : 4;
} __packed;

struct host_addba_param {
	u8 lut_index;
	u8 perr_mac_addr[6];
	u8 dialog_token;
	struct ieeetypes_addba_param addba_param;
	u16 timeout;
} __packed;

struct host_delba_param {
	u8 lut_index;
	u8 perr_mac_addr[6];
	struct ieeetypes_delba_param delba_param;
	u16 reason_code;
} __packed;

struct sprdwl_pop_work {
	int chn;
	void *head;
	void *tail;
	int num;
};

static inline bool sprdwl_is_group(unsigned char *addr)
{
	if ((addr[0] & BIT(0)) != 0)
		return true;

	return false;
}

int sprdwl_intf_init(struct sprdwl_priv *priv, struct sprdwl_intf *intf);
void sprdwl_intf_deinit(struct sprdwl_intf *dev);
int if_tx_cmd(struct sprdwl_intf *intf, unsigned char *data, int len);
int if_tx_addr_trans(struct sprdwl_intf *intf, unsigned char *data, int len);
int sprdwl_intf_tx_list(struct sprdwl_intf *dev,
			struct list_head *tx_list,
			struct list_head *tx_list_head,
			int tx_count,
			int ac_index,
			u8 coex_bt_on);
int sprdwl_intf_fill_msdu_dscr(struct sprdwl_vif *vif,
			       struct sk_buff *skb,
				   u8 type,
			       u8 offset);
int sprdwl_tx_free_pcie_data(struct sprdwl_intf *dev, unsigned char *data,
			     unsigned short len);
void *sprdwl_get_rx_data(struct sprdwl_intf *intf, void *pos, void **data,
			 void **tran_data, int *len, int offset);
void sprdwl_free_rx_data(struct sprdwl_intf *intf,
			 int chn, void *head, void *tail, int num);

#if defined FPGA_LOOPBACK_TEST
int sprdwl_intf_tx_data_fpga_test(struct sprdwl_intf *intf,
				  unsigned char *data, int len);
int sprdwl_intf_fill_msdu_dscr_test(struct sprdwl_priv *priv,
				    struct sk_buff *skb,
				    u8 type,
				    u8 offset);
#endif /* FPGA_LOOPBACK_TEST */

void sprdwl_hex_dump(unsigned char *name,
		     unsigned char *data, unsigned short len);
struct sprdwl_peer_entry
*sprdwl_find_peer_entry_using_lut_index(struct sprdwl_intf *intf,
					unsigned char sta_lut_index);
void sprdwl_event_sta_lut(struct sprdwl_vif *vif, u8 *data, u16 len);
struct sprdwl_peer_entry
*sprdwl_find_peer_entry_using_addr(struct sprdwl_vif *vif, u8 *addr);
void sprdwl_tx_addba(struct sprdwl_intf *intf,
		     struct sprdwl_peer_entry *peer_entry, unsigned char tid);
void sprdwl_tx_delba(struct sprdwl_intf *intf,
		     struct sprdwl_peer_entry *peer_entry, unsigned int ac_index);
void sprdwl_tx_send_addba(struct sprdwl_vif *vif, void *data, int len);
void sprdwl_tx_send_delba(struct sprdwl_vif *vif, void *data, int len);
unsigned char sprdwl_find_lut_index(struct sprdwl_intf *intf,
				    struct sprdwl_vif *vif);
int sprdwl_dis_flush_txlist(struct sprdwl_intf *intf, u8 lut_index);
void sprdwl_handle_pop_list(void *data);
int sprdwl_add_topop_list(int chn, struct mbuf_t *head,
				struct mbuf_t *tail, int num);
enum sprdwl_hw_type get_hwintf_type(void);
void set_coex_bt_on_off(u8 action);
int sprdwl_notifier_boost(struct notifier_block *nb, unsigned long event, void *data);
void sprdwl_boost(void);
void sprdwl_unboost(void);
void adjust_txnum_level(char *buf, unsigned char offset);
void adjust_rxnum_level(char *buf, unsigned char offset);
#endif /* __SPRDWL_INTF_SDIO_SC2355_H__ */
