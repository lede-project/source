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

#ifndef __SPRDWL_CFG80211_H__
#define __SPRDWL_CFG80211_H__

#include <net/cfg80211.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
#define NL80211_SCAN_FLAG_RANDOM_ADDR          (1<<3)

#define NL80211_FEATURE_SUPPORTS_WMM_ADMISSION (1 << 26)
#define NL80211_FEATURE_TDLS_CHANNEL_SWITCH    (1 << 28)
#define NL80211_FEATURE_SCAN_RANDOM_MAC_ADDR   (1 << 29)
#endif

/* auth type */
#define SPRDWL_AUTH_OPEN		0
#define SPRDWL_AUTH_SHARED		1
/* parise or group key type */
#define SPRDWL_GROUP			0
#define SPRDWL_PAIRWISE			1
/* cipher suite */
#define WLAN_CIPHER_SUITE_PMK           0x000FACFF
/* AKM suite */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
#define WLAN_AKM_SUITE_FT_8021X		0x000FAC03
#define WLAN_AKM_SUITE_FT_PSK		0x000FAC04
#endif
#define WLAN_AKM_SUITE_WAPI_CERT	0x00147201
#define WLAN_AKM_SUITE_WAPI_PSK		0x00147202

#define SPRDWL_AKM_SUITE_NONE		(0)
#define SPRDWL_AKM_SUITE_8021X		(1)
#define SPRDWL_AKM_SUITE_PSK		(2)
#define SPRDWL_AKM_SUITE_FT_8021X	(3)
#define SPRDWL_AKM_SUITE_FT_PSK		(4)
#define SPRDWL_AKM_SUITE_WAPI_PSK	(4)
#define SPRDWL_AKM_SUITE_8021X_SHA256	(5)
#define SPRDWL_AKM_SUITE_PSK_SHA256	(6)
#define SPRDWL_AKM_SUITE_WAPI_CERT	(12)

/* determine the actual values for the macros below*/
#define SPRDWL_MAX_SCAN_SSIDS		12
#define SPRDWL_MAX_SCAN_IE_LEN		2304
#define SPRDWL_MAX_NUM_PMKIDS		4
#define SPRDWL_MAX_KEY_INDEX		3
#define SPRDWL_SCAN_TIMEOUT_MS		10000
#define SPRDWL_MAX_PFN_LIST_COUNT	9
#define SPRDWL_MAX_IE_LEN           500

#define SPRDWL_MAC_INDEX_MAX		4

#define SPRDWL_ACS_SCAN_TIME		20

#define CH_MAX_2G_CHANNEL			(14)
#define CH_MAX_5G_CHANNEL			(25)
#define TOTAL_2G_5G_CHANNEL_NUM			(39)/*14+25=39*/
#define TOTAL_2G_5G_SSID_NUM         9

enum sprdwl_mode {
	SPRDWL_MODE_NONE,
	SPRDWL_MODE_STATION,
	SPRDWL_MODE_AP,

	SPRDWL_MODE_P2P_DEVICE = 4,
	SPRDWL_MODE_P2P_CLIENT,
	SPRDWL_MODE_P2P_GO,

	SPRDWL_MODE_IBSS,

	SPRDWL_MODE_MAX,
};

enum sm_state {
	SPRDWL_UNKNOWN = 0,
	SPRDWL_SCANNING,
	SPRDWL_SCAN_ABORTING,
	SPRDWL_DISCONNECTING,
	SPRDWL_DISCONNECTED,
	SPRDWL_CONNECTING,
	SPRDWL_CONNECTED
};

enum connect_result {
	SPRDWL_CONNECT_SUCCESS,
	SPRDWL_CONNECT_FAILED,
	SPRDWL_ROAM_SUCCESS,
	SPRDWL_IBSS_JOIN,
	SPRDWL_IBSS_START
};

enum acl_mode {
	SPRDWL_ACL_MODE_DISABLE,
	SPRDWL_ACL_MODE_WHITELIST,
	SPRDWL_ACL_MODE_BLACKLIST,
};

struct sprdwl_scan_ssid {
	u8 len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
} __packed;

struct sprdwl_sched_scan_buf {
	u32 interval;
	u32 flags;
	s32 rssi_thold;
	u8 channel[TOTAL_2G_5G_CHANNEL_NUM + 1];

	u32 n_ssids;
	u8 *ssid[TOTAL_2G_5G_CHANNEL_NUM];
	u32 n_match_ssids;
	u8 *mssid[TOTAL_2G_5G_CHANNEL_NUM];

	const u8 *ie;
	size_t ie_len;
};

struct unisoc_reg_rule {
	struct ieee80211_freq_range freq_range;
	struct ieee80211_power_rule power_rule;
	u32 flags;
	u32 dfs_cac_ms;
};

struct sprdwl_ieee80211_regdomain {
	u32 n_reg_rules;
	char alpha2[2];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0))
	struct ieee80211_reg_rule reg_rules[];
#else
	struct unisoc_reg_rule reg_rules[];
#endif
};

/* WIFI_EVENT_CONNECT */
struct sprdwl_connect_info {
	u8 status;
	u8 *bssid;
	u8 channel;
	s8 signal;
	u8 *bea_ie;
	u16 bea_ie_len;
	u8 *req_ie;
	u16 req_ie_len;
	u8 *resp_ie;
	u16 resp_ie_len;
} __packed;

struct sprdwl_vif;
struct sprdwl_priv;

void sprdwl_setup_wiphy(struct wiphy *wiphy, struct sprdwl_priv *priv);

int sprdwl_init_fw(struct sprdwl_vif *vif);
int sprdwl_uninit_fw(struct sprdwl_vif *vif);

struct sprdwl_vif *ctx_id_to_vif(struct sprdwl_priv *priv, u8 vif_ctx_id);
struct sprdwl_vif *mode_to_vif(struct sprdwl_priv *priv, u8 vif_mode);
void sprdwl_put_vif(struct sprdwl_vif *vif);

void sprdwl_report_softap(struct sprdwl_vif *vif, u8 is_connect, u8 *addr,
			  u8 *req_ie, u16 req_ie_len);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
void sprdwl_scan_timeout(struct timer_list *t);
#else
void sprdwl_scan_timeout(unsigned long data);
#endif
void sprdwl_scan_done(struct sprdwl_vif *vif, bool abort);
void sprdwl_sched_scan_done(struct sprdwl_vif *vif, bool abort);
void sprdwl_report_scan_result(struct sprdwl_vif *vif, u16 chan, s16 rssi,
			       u8 *frame, u16 len);
void sprdwl_report_connection(struct sprdwl_vif *vif,
							struct sprdwl_connect_info *conn_info,
							u8 status_code);
void sprdwl_report_disconnection(struct sprdwl_vif *vif, u16 reason_code);
void sprdwl_report_mic_failure(struct sprdwl_vif *vif, u8 is_mcast, u8 key_id);
void sprdwl_cfg80211_dump_frame_prot_info(int send, int freq,
					  const unsigned char *buf, int len);
void sprdwl_report_remain_on_channel_expired(struct sprdwl_vif *vif);
void sprdwl_report_mgmt_tx_status(struct sprdwl_vif *vif, u64 cookie,
				  const u8 *buf, u32 len, u8 ack);
void sprdwl_report_rx_mgmt(struct sprdwl_vif *vif, u8 chan, const u8 *buf,
			   size_t len);
void sprdwl_report_mgmt_deauth(struct sprdwl_vif *vif, const u8 *buf,
			       size_t len);
void sprdwl_report_mgmt_disassoc(struct sprdwl_vif *vif, const u8 *buf,
				 size_t len);
void sprdwl_report_cqm(struct sprdwl_vif *vif, u8 rssi_event);
void sprdwl_report_tdls(struct sprdwl_vif *vif, const u8 *peer,
			u8 oper, u16 reason_code);
void sprdwl_report_fake_probe(struct wiphy *wiphy, u8 *ie, size_t ielen);
int sprdwl_change_beacon(struct sprdwl_vif *vif,
		struct cfg80211_beacon_data *beacon);
#endif
