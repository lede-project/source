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

#ifndef __SPRDWL_H__
#define __SPRDWL_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>
#include <net/cfg80211.h>
#include <linux/inetdevice.h>
#include <linux/wireless.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <net/if_inet6.h>
#include <net/addrconf.h>
#include <linux/dcache.h>
#include <linux/udp.h>
#include <linux/version.h>
#include "wcn_wrapper.h"

#include "cfg80211.h"
#include "cmdevt.h"
#include "intf.h"
#include "vendor.h"
#include "tcp_ack.h"
#include "rtt.h"
#include "version.h"
#include "tracer.h"

#define SPRDWL_UNALIAGN		1
#ifdef SPRDWL_UNALIAGN
#define SPRDWL_PUT_LE16(val, addr)	put_unaligned_le16((val), (&addr))
#define SPRDWL_PUT_LE32(val, addr)	put_unaligned_le32((val), (&addr))
#define SPRDWL_GET_LE16(addr)		get_unaligned_le16(&addr)
#define SPRDWL_GET_LE32(addr)		get_unaligned_le32(&addr)
#define SPRDWL_GET_LE64(addr)		get_unaligned_le64(&addr)
#else
#define SPRDWL_PUT_LE16(val, addr)	cpu_to_le16((val), (addr))
#define SPRDWL_PUT_LE32(val, addr)	cpu_to_le32((val), (addr))
#define SPRDWL_GET_LE16(addr)		le16_to_cpu((addr))
#define SPRDWL_GET_LE32(addr)		le32_to_cpu((addr))
#endif

/* the max length between data_head and net data */
#define SPRDWL_SKB_HEAD_RESERV_LEN	16
#define SPRDWL_COUNTRY_CODE_LEN		2
#define ETHER_TYPE_IP 0x0800           /* IP */
#define ETHER_TYPE_IPV6 0x86dd             /* IPv6 */
#define WAPI_TYPE                 0x88B4

#ifdef OTT_UWE
#define FOUR_BYTES_ALIGN_OFFSET 3
#endif

struct sprdwl_mc_filter {
	bool mc_change;
	u8 subtype;
	u8 mac_num;
	u8 mac_addr[0];
};

struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
};

struct scan_result {
	struct list_head list;
	int signal;
	unsigned char bssid[6];
};

struct sprdwl_vif {
	struct net_device *ndev;	/* Linux net device */
	struct wireless_dev wdev;	/* Linux wireless device */
	struct sprdwl_priv *priv;

	char name[IFNAMSIZ];
	enum sprdwl_mode mode;
	struct list_head vif_node;	/* node for virtual interface list */
	int ref;

	/* multicast filter stuff */
	struct sprdwl_mc_filter *mc_filter;

	/* common stuff */
	enum sm_state sm_state;
	unsigned char mac[ETH_ALEN];
	int ssid_len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
	u8 bssid[ETH_ALEN];
	unsigned char beacon_loss;
	bool local_mac_flag;

	/* encryption stuff */
	u8 prwise_crypto;
	u8 grp_crypto;
	u8 key_index[2];
	u8 key[2][4][WLAN_MAX_KEY_LEN];
	u8 key_len[2][4];
	unsigned long mgmt_reg;

	/* P2P stuff */
	struct ieee80211_channel listen_channel;
	u64 listen_cookie;
	u8 ctx_id;
	struct  list_head  scan_head_ptr;
#ifdef ACS_SUPPORT
	/* ACS stuff */
	struct list_head survey_info_list;
#endif /* ACS_SUPPORT*/
#ifdef DFS_MASTER
	/* dfs master mode */
	struct workqueue_struct *dfs_cac_workqueue;
	struct delayed_work dfs_cac_work;
	struct workqueue_struct *dfs_chan_sw_workqueue;
	struct delayed_work dfs_chan_sw_work;
	struct cfg80211_chan_def dfs_chandef;
#endif
	u8 wps_flag;
#ifdef SYNC_DISCONNECT
	atomic_t sync_disconnect_event;
	u16 disconnect_event_code;
	wait_queue_head_t disconnect_wq;
#endif
	bool has_rand_mac;
	u8 random_mac[ETH_ALEN];
};

enum sprdwl_hw_type {
	SPRDWL_HW_SIPC,
	SPRDWL_HW_SDIO,
	SPRDWL_HW_PCIE,
	SPRDWL_HW_USB
};

#ifdef WMMAC_WFA_CERTIFICATION
struct wmm_ac_params {
	u8 aci_aifsn; /* AIFSN, ACM, ACI */
	u8 cw; /* ECWmin, ECWmax (CW = 2^ECW - 1) */
	u16 txop_limit;
};

struct wmm_params_element {
	/* Element ID: 221 (0xdd); Length: 24 */
	/* required fields for WMM version 1 */
	u8 oui[3]; /* 00:50:f2 */
	u8 oui_type; /* 2 */
	u8 oui_subtype; /* 1 */
	u8 version; /* 1 for WMM version 1.0 */
	u8 qos_info; /* AP/STA specific QoS info */
	u8 reserved; /* 0 */
	struct wmm_ac_params ac[4]; /* AC_BE, AC_BK, AC_VI, AC_VO */
};

struct sprdwl_wmmac_params {
	struct wmm_ac_params ac[4];
	struct timer_list wmmac_edcaf_timer;
};
#endif

struct sprdwl_channel_list {
	int num_channels;
	int channels[TOTAL_2G_5G_CHANNEL_NUM];
};

#ifdef CP2_RESET_SUPPORT
struct sprlwl_drv_cp_sync {
	char country[2];
	unsigned char fw_stat[SPRDWL_MODE_MAX];
	bool scan_not_allowed;
	bool cmd_not_allowed;
	struct regulatory_request request;

};
#endif

struct sprdwl_priv {
	struct wiphy *wiphy;
	/* virtual interface list */
	spinlock_t list_lock;
	struct list_head vif_list;

	/* necessary info from fw */
	u32 chip_model;
	u32 chip_ver;
	u32 fw_ver;
	u32 fw_std;
	u32 fw_capa;
	struct sprdwl_ver wl_ver;
	u8 max_ap_assoc_sta;
	u8 max_acl_mac_addrs;
	u8 max_mc_mac_addrs;
	u8 mac_addr[ETH_ALEN];
	u8 wnm_ft_support;
	u32 wiphy_sec2_flag;
	struct wiphy_sec2_t wiphy_sec2;
	struct sync_api_verion_t sync_api;
	unsigned short skb_head_len;
	enum sprdwl_hw_type hw_type;
	void *hw_priv;
	int hw_offset;
	struct sprdwl_if_ops *if_ops;
	u16 beacon_period;

	/* scan */
	spinlock_t scan_lock;
	struct sprdwl_vif *scan_vif;
	struct cfg80211_scan_request *scan_request;
	struct timer_list scan_timer;

	/* schedule scan */
	spinlock_t sched_scan_lock;
	struct sprdwl_vif *sched_scan_vif;
	struct cfg80211_sched_scan_request *sched_scan_request;

	/*gscan*/
	u8 gscan_buckets_num;
	struct sprdwl_gscan_cached_results *gscan_res;

	struct sprdwl_gscan_hotlist_results *hotlist_res;

	int gscan_req_id;
	struct sprdwl_significant_change_result *significant_res;
	struct sprdwl_roam_capa roam_capa;
	/*ll status*/
	struct sprdwl_llstat_radio pre_radio;

	/* default MAC addr*/
	unsigned char default_mac[ETH_ALEN];
#define SPRDWL_INTF_CLOSE	(0)
#define SPRDWL_INTF_OPEN	(1)
#define SPRDWL_INTF_CLOSING	(2)
	unsigned char fw_stat[SPRDWL_MODE_MAX];

	/* delayed work */
	spinlock_t work_lock;
	struct work_struct work;
	struct list_head work_list;
	struct workqueue_struct *common_workq;

	struct dentry *debugfs;

	/* tcp ack management */
	struct sprdwl_tcp_ack_manage ack_m;

	/* FTM */
	struct sprdwl_ftm_priv ftm;
	struct wakeup_trace wakeup_tracer;
#ifdef WMMAC_WFA_CERTIFICATION
	/*wmmac*/
	struct sprdwl_wmmac_params wmmac;
#endif
	struct sprdwl_channel_list ch_2g4_info;
	struct sprdwl_channel_list ch_5g_without_dfs_info;
	struct sprdwl_channel_list ch_5g_dfs_info;
	/* with credit or without credit */
#define TX_WITH_CREDIT	(0)
#define TX_NO_CREDIT	(1)
	unsigned char credit_capa;
	int is_suspending;

	/* OTT support */
#define OTT_NO_SUPT	(0)
#define OTT_SUPT	(1)
	unsigned char ott_supt;

#ifdef CP2_RESET_SUPPORT
	struct sprlwl_drv_cp_sync sync;
#endif
};

struct sprdwl_eap_hdr {
	u8 version;
#define EAP_PACKET_TYPE		(0)
	u8 type;
	u16 len;
#define EAP_FAILURE_CODE	(4)
	u8 code;
	u8 id;
	u16 auth_proc_len;
	u8 auth_proc_type;
	u64 ex_id:24;
	u64 ex_type:32;
#define EAP_WSC_DONE	(5)
	u64 opcode:8;
};

enum sprdwl_debug {
	L_NONE = 0,
	L_ERR, /*LEVEL_ERR*/
	L_WARN, /*LEVEL_WARNING*/
	L_INFO,/*LEVEL_INFO*/
	L_DBG, /*LEVEL_DEBUG*/
};

extern int sprdwl_debug_level;
extern struct device *sprdwl_dev;
#define wl_debug(fmt, args...) \
	do { \
		if (sprdwl_debug_level >= L_DBG) { \
			pr_err("sprdwl:" fmt, ##args); \
		} \
	} while (0)

#define wl_err(fmt, args...) \
	do { \
		if (sprdwl_debug_level >= L_ERR) \
			pr_err("sprdwl:" fmt, ##args); \
	} while (0)

#define wl_warn(fmt, args...) \
	do { \
		if (sprdwl_debug_level >= L_WARN) \
			pr_err("sprdwl:" fmt, ##args); \
	} while (0)

#define wl_info(fmt, args...) \
	do { \
		if (sprdwl_debug_level >= L_INFO) { \
			pr_err("sprdwl:" fmt, ##args); \
		} \
	} while (0)

#define wl_err_ratelimited(fmt, args...) \
	do { \
		if (sprdwl_debug_level >= L_ERR) \
			printk_ratelimited("sprdwl:" fmt, ##args); \
	} while (0)

#define wl_ndev_log(level, ndev, fmt, args...) \
	do { \
		if (sprdwl_debug_level >= level) { \
			netdev_err(ndev, fmt, ##args); \
		} \
	} while (0)

#define wl_hex_dump(level, _str, _type, _row, _gp, _buf, _len, _ascii) \
	do { \
		if (sprdwl_debug_level >= level) { \
			print_hex_dump(KERN_ERR, _str, _type, _row, _gp, _buf, _len, _ascii); \
		} \
	} while (0)

#define wl_err_ratelimited(fmt, args...) \
	do { \
		if (sprdwl_debug_level >= L_ERR) \
			printk_ratelimited("sprdwl:" fmt, ##args); \
	} while (0)

#ifdef ACS_SUPPORT
struct sprdwl_bssid {
	unsigned char bssid[ETH_ALEN];
	struct list_head list;
};

struct sprdwl_survey_info {
	/* survey info */
	unsigned int cca_busy_time;
	char noise;
	struct ieee80211_channel *channel;
	struct list_head survey_list;
	/* channel info */
	unsigned short chan;
	unsigned short beacon_num;
	struct list_head bssid_list;
};

void clean_survey_info_list(struct sprdwl_vif *vif);
void transfer_survey_info(struct sprdwl_vif *vif);
void acs_scan_result(struct sprdwl_vif *vif, u16 chan,
		     struct ieee80211_mgmt *mgmt);
#endif /* ACS_SUPPORT */

void init_scan_list(struct sprdwl_vif *vif);
void clean_scan_list(struct sprdwl_vif *vif);
extern struct sprdwl_priv *g_sprdwl_priv;

void sprdwl_netif_rx(struct sk_buff *skb, struct net_device *ndev);
void sprdwl_stop_net(struct sprdwl_vif *vif);
void sprdwl_net_flowcontrl(struct sprdwl_priv *priv,
			   enum sprdwl_mode mode, bool state);

struct wireless_dev *sprdwl_add_iface(struct sprdwl_priv *priv,
				      const char *name,
				      enum nl80211_iftype type, u8 *addr);
int sprdwl_del_iface(struct sprdwl_priv *priv, struct sprdwl_vif *vif);
struct sprdwl_priv *sprdwl_core_create(enum sprdwl_hw_type type,
				       struct sprdwl_if_ops *ops);
void sprdwl_core_free(struct sprdwl_priv *priv);
int sprdwl_core_init(struct device *dev, struct sprdwl_priv *priv);
int sprdwl_core_deinit(struct sprdwl_priv *priv);
int marlin_reset_register_notify(void *callback_func, void *para);
int marlin_reset_unregister_notify(void);
#endif /* __SPRDWL_H__ */
