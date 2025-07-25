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

#ifndef __WL_CORE_H__
#define __WL_CORE_H__
#include "wcn_wrapper.h"
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include "cfg80211.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "dbg_ini_util.h"

#define SPRDWL_NORMAL_MEM	0
#define SPRDWL_DEFRAG_MEM	1

#define SPRDWL_TX_CMD_TIMEOUT	3000
#define SPRDWL_TX_DATA_TIMEOUT	4000

#define SPRDWL_TX_MSG_CMD_NUM 128
#define SPRDWL_TX_QOS_POOL_SIZE 20000
#define SPRDWL_TX_DATA_START_NUM (SPRDWL_TX_QOS_POOL_SIZE - 3)
#define SPRDWL_RX_MSG_NUM 20000

/* tx len less than cp len 4 byte as sdiom 4 bytes align */
/* set MAX CMD length to 1600 on firmware side*/
#define SPRDWL_MAX_CMD_TXLEN	1596
#define SPRDWL_MAX_CMD_RXLEN	1092
#define SPRDWL_MAX_DATA_TXLEN	1672
#define SPRDWL_MAX_DATA_RXLEN	1676

#define MAX_LUT_NUM 32

struct tx_address {
	u8 da[ETH_ALEN];
	u8 sa[ETH_ALEN];
};

struct rx_address {
	u8 sa[ETH_ALEN];
	u8 da[ETH_ALEN];
};

struct sprdwl_peer_entry {
	union {
		struct rx_address rx;
		struct tx_address tx;
	};

	u8 lut_index;
	u8 ctx_id;
	u8 cipher_type;
	u8 pending_num;
	u8 ht_enable;
	u8 vht_enable;
	u8 ip_acquired;
	/*tx ba done based on tid*/
	unsigned long ba_tx_done_map;
	u8 vowifi_enabled;
	u8 vowifi_pkt_cnt;
	struct timespec time[6 + 1];
};

#if defined(MORE_DEBUG)
/*tx/rx states and performance statistics*/
struct txrx_stats {
	unsigned long	rx_packets;
	/*tx success packets num*/
	unsigned long	tx_packets;
	unsigned long	rx_bytes;
	/*tx success bytes num*/
	unsigned long	tx_bytes;
	unsigned long	rx_errors;
	unsigned long	tx_errors;
	unsigned int tx_nomem_errors;
	unsigned int tx_fail_errors;
	unsigned long	rx_dropped;
	unsigned long	tx_dropped;
	/*alloc pkt fail*/
	unsigned long rx_pktgetfail;
	unsigned long tx_pktgetfail;
	/* Number of tx packets we had to realloc for headroom */
	unsigned long tx_realloc;
	/* multicast packets received */
	unsigned long	rx_multicast;
	unsigned long	tx_multicast;
	unsigned long tx_cost_time;
	unsigned long tx_avg_time;
	unsigned long tx_arp_num;
	/*qos ac stream1 sent num*/
	unsigned long ac1_num;
	/*qos ac stream2 sent num*/
	unsigned long ac2_num;
	unsigned long tx_filter_num;
	/*statistical sample count*/
	unsigned int gap_num;
};
#endif

struct tdls_flow_count_para {
	u8 valid;
	u8 da[ETH_ALEN];
	/*u8 timer;seconds*/
	u16 threshold;/*bytes*/
	u16 data_len_counted;/*bytes*/
	u32 start_mstime;/*ms*/
	u8 timer;/*seconds*/
};

#define MAX_TDLS_PEER 32

struct sprdwl_priv;
struct sprdwl_intf {
	struct platform_device *pdev;
	/* priv use void *, after MCC adn priv->flags,
	 * and change txrx intf pass priv to void later
	 */
	struct sprdwl_priv *priv;

	/* if nedd more flags which not only exit, fix it*/
	/* unsigned int exit:1; */
	int exit;

	int flag;
	int lastflag;

	int tx_mode;
	int rx_mode;

	/*point to hif interface(sdio/pcie)*/
	void *hw_intf;

	/* Manage tx function */
	void *sprdwl_tx;
	/* Manage rx function */
	void *sprdwl_rx;

	struct sprdwl_peer_entry peer_entry[MAX_LUT_NUM];
	unsigned long tx_num[MAX_LUT_NUM];
	unsigned char *skb_da;
#if defined FPGA_LOOPBACK_TEST
	int loopback_n;
#endif

	int hif_offset;
	unsigned char rx_cmd_port;
	unsigned char rx_data_port;
	unsigned char tx_cmd_port;
	unsigned char tx_data_port;
#if defined(MORE_DEBUG)
	struct txrx_stats stats;
#endif

	u8 tdls_flow_count_enable;
	struct tdls_flow_count_para tdls_flow_count[MAX_TDLS_PEER];
	/*suspend_mode:ap suspend/resumed status
	  resumed:cp suspend/resumed status*/
#define SPRDWL_PS_SUSPENDING  1
#define SPRDWL_PS_SUSPENDED  2
#define SPRDWL_PS_RESUMING  3
#define SPRDWL_PS_RESUMED  0
	int suspend_mode;

	int fw_power_down;
	int fw_awake;
#ifdef UNISOC_WIFI_PS
	struct completion suspend_completed;
#endif
	/*for pkt log function*/
	loff_t lp;
	struct file *pfile;
	/*for suspend resume time count*/
	unsigned long sleep_time;

	u8 cp_asserted;
	struct dbg_ini_cfg ini_cfg;
	/*wifi bt coex mode, 1:BT is on, 0:BT is off*/
	u8 coex_bt_on;
	u8 boost;
	unsigned int txnum_level;
	unsigned int rxnum_level;
#ifdef STA_SOFTAP_SCC_MODE
	int sta_home_channel;
#endif
};

/* HashTable */
struct HashTable {
	struct kv **table;
};

void sprdwl_free_data(void *data, int buffer_type);
enum sprdwl_hw_type sprd_core_get_hwintf_mode(void);

void sprdwl_event_tdls_flow_count(struct sprdwl_vif *vif, u8 *data, u16 len);
void count_tdls_flow(struct sprdwl_vif *vif, u8 *data, u16 len);
void sprdwl_tdls_flow_flush(struct sprdwl_vif *vif, const u8 *peer, u8 oper);
#endif
