/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Baolei Yuan <baolei.yuan@spreadtrum.com>
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

#include "sprdwl.h"
#include "cmdevt.h"
#include "vendor.h"
#ifdef NAN_SUPPORT
#include "nan.h"
#include <linux/version.h>
#endif /* NAN_SUPPORT */
#ifdef RTT_SUPPORT
#include "rtt.h"
#endif /* RTT_SUPPORT */

#define VENDOR_SCAN_RESULT_EXPIRE	(7 * HZ)

static const u8 *wpa_scan_get_ie(u8 *res, u8 ie_len, u8 ie)
{
	const u8 *end, *pos;

	pos = res;
	end = pos + ie_len;
	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == ie)
			return pos;
		pos += 2 + pos[1];
	}
	return NULL;
}

static const struct nla_policy
wlan_gscan_config_policy	[GSCAN_ATTR_SUBCMD_CONFIG_PARAM_MAX + 1] = {
	[GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID] =	{ .type = NLA_U32 },
	[GSCAN_ATTR_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND]
		= { .type = NLA_U32 },
	[GSCAN_ATTR_CHANNEL_SPEC_CHANNEL] = { .type = NLA_U32 },
	[GSCAN_ATTR_CHANNEL_SPEC_DWELL_TIME] = { .type = NLA_U32 },
	[GSCAN_ATTR_CHANNEL_SPEC_PASSIVE] = { .type = NLA_U8 },
	[GSCAN_ATTR_CHANNEL_SPEC_CLASS] = { .type = NLA_U8 },
	[GSCAN_ATTR_BUCKET_SPEC_INDEX] = { .type = NLA_U8 },
	[GSCAN_ATTR_BUCKET_SPEC_BAND] = { .type = NLA_U8 },
	[GSCAN_ATTR_BUCKET_SPEC_PERIOD] = { .type = NLA_U32 },
	[GSCAN_ATTR_BUCKET_SPEC_REPORT_EVENTS] = { .type = NLA_U8 },
	[GSCAN_ATTR_BUCKET_SPEC_NUM_CHANNEL_SPECS] = { .type = NLA_U32 },
	[GSCAN_ATTR_SCAN_CMD_PARAMS_BASE_PERIOD] = { .type = NLA_U32 },
	[GSCAN_ATTR_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN] = { .type = NLA_U32 },
	[GSCAN_ATTR_SCAN_CMD_PARAMS_REPORT_THR] = { .type = NLA_U8 },
	[GSCAN_ATTR_SCAN_CMD_PARAMS_NUM_BUCKETS] = { .type = NLA_U8 },
	[GSCAN_ATTR_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH]
		= { .type = NLA_U8 },
	[GSCAN_ATTR_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX]
		= { .type = NLA_U32 },
	[GSCAN_ATTR_AP_THR_PARAM_BSSID] = { .type = NLA_UNSPEC },
	[GSCAN_ATTR_AP_THR_PARAM_RSSI_LOW] = { .type = NLA_S32 },
	[GSCAN_ATTR_AP_THR_PARAM_RSSI_HIGH] = { .type = NLA_S32 },
	[GSCAN_ATTR_AP_THR_PARAM_CHANNEL] = { .type = NLA_U32 },
	[GSCAN_ATTR_BSSID_HOTLIST_PARAMS_NUM_AP] = { .type = NLA_U32 },
	[GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE]
		= { .type = NLA_U32 },
	[GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE]
		= { .type = NLA_U32 },
	[GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING]
		= { .type = NLA_U32 },
	[GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_NUM_AP] = { .type = NLA_U32 },
};

/*enable roaming functon------ CMD ID:9*/
static int sprdwl_vendor_roaming_enable(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	u8 roam_state;
	struct nlattr *tb[SPRDWL_VENDOR_ROAMING_POLICY + 1];
	int ret = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb, SPRDWL_VENDOR_ROAMING_POLICY, data, len, NULL, NULL)) {
#else
	if (nla_parse(tb, SPRDWL_VENDOR_ROAMING_POLICY, data, len, NULL)) {
#endif
		wl_err("Invalid ATTR\n");
		return -EINVAL;
	}

	if (tb[SPRDWL_VENDOR_ROAMING_POLICY]) {
		roam_state = (u8)nla_get_u32(tb[SPRDWL_VENDOR_ROAMING_POLICY]);
		wl_info("roaming offload state:%d\n", roam_state);
		 /*send roam state with roam params by roaming CMD*/
		ret = sprdwl_set_roam_offload(priv, vif->ctx_id,
					SPRDWL_ROAM_OFFLOAD_SET_FLAG,
					&roam_state, sizeof(roam_state));
	}

	return ret;
}

static int sprdwl_vendor_nan_enable(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	return WIFI_SUCCESS;
}

/*Send link layer stats CMD*/
static int sprdwl_llstat(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 subtype,
			 const void *buf, u8 len, u8 *r_buf, u16 *r_len)
{
	u8 *sub_cmd, *buf_pos;
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, len + 1, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_LLSTAT);
	if (!msg)
		return -ENOMEM;
	sub_cmd = (u8 *)msg->data;
	*sub_cmd = subtype;
	buf_pos = sub_cmd + 1;

	if (!buf && len)
		return -ENOMEM;
	memcpy(buf_pos, buf, len);

	if (subtype == SPRDWL_SUBCMD_SET)
		return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, 0, 0);
	else
		return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf,
						r_len);
}

/*set link layer status function-----CMD ID:14*/
static int sprdwl_vendor_set_llstat_handler(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int len)
{
	int ret = 0, err = 0;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	struct wifi_link_layer_params *ll_params;
	struct nlattr *tb[SPRDWL_LL_STATS_SET_MAX + 1];

	if (!priv || !vif)
		return -EIO;
	if (!(priv->fw_capa & SPRDWL_CAPA_LL_STATS))
		return -ENOTSUPP;
	if (!data) {
		wl_err("%s llstat param check filed\n", __func__);
		return -EINVAL;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	err = nla_parse(tb, SPRDWL_LL_STATS_SET_MAX, data,
			len, sprdwl_ll_stats_policy, NULL);
#else
	err = nla_parse(tb, SPRDWL_LL_STATS_SET_MAX, data,
			len, sprdwl_ll_stats_policy);
#endif
	if (err)
		return err;
	ll_params = kzalloc(sizeof(*ll_params), GFP_KERNEL);
	if (!ll_params)
		return -ENOMEM;
	if (tb[SPRDWL_LL_STATS_MPDU_THRESHOLD]) {
		ll_params->mpdu_size_threshold =
			nla_get_u32(tb[SPRDWL_LL_STATS_MPDU_THRESHOLD]);
	}
	if (tb[SPRDWL_LL_STATS_GATHERING]) {
		ll_params->aggressive_statistics_gathering =
			nla_get_u32(tb[SPRDWL_LL_STATS_GATHERING]);
	}

	wiphy_err(wiphy, "%s mpdu_threshold =%u\n gathering=%u\n",
		  __func__, ll_params->mpdu_size_threshold,
			ll_params->aggressive_statistics_gathering);
	if (ll_params->aggressive_statistics_gathering)
		ret = sprdwl_llstat(priv, vif->ctx_id, SPRDWL_SUBCMD_SET,
					ll_params, sizeof(*ll_params),
							0, 0);
	kfree(ll_params);
	return ret;
}

static int sprdwl_compose_radio_st(struct sk_buff *reply,
				   struct wifi_radio_stat *radio_st)
{
	/*2.4G only,radio_num=1,if 5G supported radio_num=2*/
	int radio_num = 1;

	if (nla_put_u32(reply, SPRDWL_LL_STATS_NUM_RADIOS,
			radio_num))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ID,
			radio_st->radio))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME,
			radio_st->on_time))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_TX_TIME,
			radio_st->tx_time))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_NUM_TX_LEVELS,
			radio_st->num_tx_levels))
		goto out_put_fail;
	if (radio_st->num_tx_levels > 0) {
		if (nla_put(reply, SPRDWL_LL_STATS_RADIO_TX_TIME_PER_LEVEL,
				sizeof(u32)*radio_st->num_tx_levels,
				radio_st->tx_time_per_levels))
			goto out_put_fail;
	}
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_RX_TIME,
			radio_st->rx_time))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME_SCAN,
			radio_st->on_time_scan))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME_NBD,
			radio_st->on_time_nbd))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME_GSCAN,
			radio_st->on_time_gscan))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME_ROAM_SCAN,
			radio_st->on_time_roam_scan))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME_PNO_SCAN,
			radio_st->on_time_pno_scan))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_ON_TIME_HS20,
			radio_st->on_time_hs20))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_RADIO_NUM_CHANNELS,
			radio_st->num_channels))
		goto out_put_fail;
	if (radio_st->num_channels > 0) {
		if (nla_put(reply, SPRDWL_LL_STATS_CH_INFO,
				radio_st->num_channels *
				sizeof(struct wifi_channel_stat),
				radio_st->channels))
			goto out_put_fail;
	}
	return 0;
out_put_fail:
	return -EMSGSIZE;
}

static int sprdwl_compose_iface_st(struct sk_buff *reply,
				   struct wifi_iface_stat *iface_st)
{
	int i;
	struct nlattr *nest1, *nest2;

	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_INFO_MODE,
			iface_st->info.mode))
		goto out_put_fail;
	if (nla_put(reply, SPRDWL_LL_STATS_IFACE_INFO_MAC_ADDR,
			sizeof(iface_st->info.mac_addr), iface_st->info.mac_addr))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_INFO_STATE,
			iface_st->info.state))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_INFO_ROAMING,
			iface_st->info.roaming))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_INFO_CAPABILITIES,
			iface_st->info.capabilities))
		goto out_put_fail;
	if (nla_put(reply, SPRDWL_LL_STATS_IFACE_INFO_SSID,
			sizeof(iface_st->info.ssid), iface_st->info.ssid))
		goto out_put_fail;
	if (nla_put(reply, SPRDWL_LL_STATS_IFACE_INFO_BSSID,
			sizeof(iface_st->info.bssid), iface_st->info.bssid))
		goto out_put_fail;
	if (nla_put(reply, SPRDWL_LL_STATS_IFACE_INFO_AP_COUNTRY_STR,
			sizeof(iface_st->info.ap_country_str),
			iface_st->info.ap_country_str))
		goto out_put_fail;
	if (nla_put(reply, SPRDWL_LL_STATS_IFACE_INFO_COUNTRY_STR,
			sizeof(iface_st->info.country_str),
			iface_st->info.country_str))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_BEACON_RX,
			iface_st->beacon_rx))
		goto out_put_fail;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	if (nla_put_u64_64bit(reply, SPRDWL_LL_STATS_IFACE_AVERAGE_TSF_OFFSET,
			iface_st->average_tsf_offset, 0))
#else
	if (nla_put_u64(reply, SPRDWL_LL_STATS_IFACE_AVERAGE_TSF_OFFSET,
			iface_st->average_tsf_offset))
#endif
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_LEAKY_AP_DETECTED,
			iface_st->leaky_ap_detected))
		goto out_put_fail;
	if (nla_put_u32(reply,
			SPRDWL_LL_STATS_IFACE_LEAKY_AP_AVG_NUM_FRAMES_LEAKED,
		iface_st->leaky_ap_avg_num_frames_leaked))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_LEAKY_AP_GUARD_TIME,
			iface_st->leaky_ap_guard_time))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_MGMT_RX,
			iface_st->mgmt_rx))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_MGMT_ACTION_RX,
			iface_st->mgmt_action_rx))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_MGMT_ACTION_TX,
			iface_st->mgmt_action_tx))
		goto out_put_fail;
	if (nla_put_s32(reply, SPRDWL_LL_STATS_IFACE_RSSI_MGMT,
			iface_st->rssi_mgmt))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_RSSI_DATA,
			iface_st->rssi_data))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_RSSI_ACK,
			iface_st->rssi_ack))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_IFACE_RSSI_DATA,
			iface_st->rssi_data))
		goto out_put_fail;
	nest1 = nla_nest_start(reply, SPRDWL_LL_STATS_WMM_INFO);
	if (!nest1)
		goto out_put_fail;
	for (i = 0; i < WIFI_AC_MAX; i++) {
		nest2 = nla_nest_start(reply, SPRDWL_LL_STATS_WMM_AC_AC);
		if (!nest2)
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_AC,
				iface_st->ac[i].ac))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_TX_MPDU,
				iface_st->ac[i].tx_mpdu))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_RX_MPDU,
				iface_st->ac[i].rx_mpdu))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_TX_MCAST,
				iface_st->ac[i].tx_mcast))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_RX_MCAST,
				iface_st->ac[i].rx_mcast))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_RX_AMPDU,
				iface_st->ac[i].rx_ampdu))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_TX_AMPDU,
				iface_st->ac[i].tx_ampdu))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_MPDU_LOST,
				iface_st->ac[i].mpdu_lost))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_RETRIES,
				iface_st->ac[i].retries))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_RETRIES_SHORT,
				iface_st->ac[i].retries_short))
			goto out_put_fail;
		if (nla_put_u32(reply, SPRDWL_LL_STATS_WMM_AC_RETRIES_LONG,
				iface_st->ac[i].retries_long))
			goto out_put_fail;
		if (nla_put_u32(reply,
				SPRDWL_LL_STATS_WMM_AC_CONTENTION_TIME_MIN,
				iface_st->ac[i].contention_time_min))
			goto out_put_fail;
		if (nla_put_u32(reply,
				SPRDWL_LL_STATS_WMM_AC_CONTENTION_TIME_MAX,
				iface_st->ac[i].contention_time_max))
			goto out_put_fail;
		if (nla_put_u32(reply,
				SPRDWL_LL_STATS_WMM_AC_CONTENTION_TIME_AVG,
				iface_st->ac[i].contention_time_avg))
			goto out_put_fail;
		if (nla_put_u32(reply,
				SPRDWL_LL_STATS_WMM_AC_CONTENTION_NUM_SAMPLES,
				iface_st->ac[i].contention_num_samples))
			goto out_put_fail;
		nla_nest_end(reply, nest2);
	}
	nla_nest_end(reply, nest1);
	return 0;
out_put_fail:
	return -EMSGSIZE;
}

void calc_radio_dif(struct sprdwl_llstat_radio *dif_radio,
			struct sprdwl_llstat_data *llst,
			struct sprdwl_llstat_radio *pre_radio)
{
	int i;

	dif_radio->rssi_mgmt = llst->rssi_mgmt;
	dif_radio->bcn_rx_cnt = llst->bcn_rx_cnt >= pre_radio->bcn_rx_cnt ?
		llst->bcn_rx_cnt - pre_radio->bcn_rx_cnt : llst->bcn_rx_cnt;
	/*save lastest beacon count*/
	pre_radio->bcn_rx_cnt = llst->bcn_rx_cnt;

	for (i = 0; i < WIFI_AC_MAX; i++) {
		dif_radio->ac[i].tx_mpdu = (llst->ac[i].tx_mpdu >=
			pre_radio->ac[i].tx_mpdu) ?
			(llst->ac[i].tx_mpdu - pre_radio->ac[i].tx_mpdu) :
			llst->ac[i].tx_mpdu;
		/*save lastest tx mpdu*/
		pre_radio->ac[i].tx_mpdu = llst->ac[i].tx_mpdu;

		dif_radio->ac[i].rx_mpdu = (llst->ac[i].rx_mpdu >=
			pre_radio->ac[i].rx_mpdu) ?
			(llst->ac[i].rx_mpdu - pre_radio->ac[i].rx_mpdu) :
			llst->ac[i].rx_mpdu;
		/*save lastest rx mpdu*/
		pre_radio->ac[i].rx_mpdu = llst->ac[i].rx_mpdu;

		dif_radio->ac[i].tx_mpdu_lost = (llst->ac[i].tx_mpdu_lost >=
			pre_radio->ac[i].tx_mpdu_lost) ?
			(llst->ac[i].tx_mpdu_lost -
			 pre_radio->ac[i].tx_mpdu_lost) :
			llst->ac[i].tx_mpdu_lost;
		/*save mpdu lost value*/
		pre_radio->ac[i].tx_mpdu_lost = llst->ac[i].tx_mpdu_lost;

		dif_radio->ac[i].tx_retries = (llst->ac[i].tx_retries >=
			pre_radio->ac[i].tx_retries) ?
			(llst->ac[i].tx_retries - pre_radio->ac[i].tx_retries) :
			llst->ac[i].tx_retries;
		/*save retries value*/
		pre_radio->ac[i].tx_retries = llst->ac[i].tx_retries;
	}
}

/*get link layer status function---CMD ID:15*/
static int sprdwl_vendor_get_llstat_handler(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int len)
{
	struct sk_buff *reply_radio, *reply_iface;
	struct sprdwl_llstat_data *llst;
	struct wifi_radio_stat *radio_st;
	struct wifi_iface_stat *iface_st;
	struct sprdwl_llstat_radio *dif_radio;
	u16 r_len = sizeof(*llst);
	u8 r_buf[sizeof(*llst)], ret, i;
	u32 reply_radio_length, reply_iface_length;

	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
#ifdef CP2_RESET_SUPPORT
	if (true == priv->sync.scan_not_allowed)
		return 0;
#endif
	if (!priv || !vif)
		return -EIO;
	if (!(priv->fw_capa & SPRDWL_CAPA_LL_STATS))
		return -ENOTSUPP;
	memset(r_buf, 0, r_len);
	radio_st = kzalloc(sizeof(*radio_st), GFP_KERNEL);
	iface_st = kzalloc(sizeof(*iface_st), GFP_KERNEL);
	dif_radio = kzalloc(sizeof(*dif_radio), GFP_KERNEL);

	if (!radio_st || !iface_st || !dif_radio)
		goto out_put_fail;
	ret = sprdwl_llstat(priv, vif->ctx_id, SPRDWL_SUBCMD_GET, NULL, 0,
				r_buf, &r_len);
	if (ret)
		goto out_put_fail;

	llst = (struct sprdwl_llstat_data *)r_buf;

	calc_radio_dif(dif_radio, llst, &priv->pre_radio);

	/*set data for iface struct*/
	iface_st->info.mode = vif->mode;
	memcpy(iface_st->info.mac_addr, vif->ndev->dev_addr,
		   ETH_ALEN);
	iface_st->info.state = vif->sm_state;
	memcpy(iface_st->info.ssid, vif->ssid,
		   IEEE80211_MAX_SSID_LEN);
	ether_addr_copy(iface_st->info.bssid, vif->bssid);
	iface_st->beacon_rx = dif_radio->bcn_rx_cnt;
	iface_st->rssi_mgmt = dif_radio->rssi_mgmt;
	for (i = 0; i < WIFI_AC_MAX; i++) {
		iface_st->ac[i].tx_mpdu = dif_radio->ac[i].tx_mpdu;
		iface_st->ac[i].rx_mpdu = dif_radio->ac[i].rx_mpdu;
		iface_st->ac[i].mpdu_lost = dif_radio->ac[i].tx_mpdu_lost;
		iface_st->ac[i].retries = dif_radio->ac[i].tx_retries;
	}
	/*set data for radio struct*/
	radio_st->on_time = llst->on_time;
	radio_st->tx_time = (u32)llst->radio_tx_time;
	radio_st->rx_time = (u32)llst->radio_rx_time;
	radio_st->on_time_scan = llst->on_time_scan;
	wl_info("beacon_rx=%d, rssi_mgmt=%d\n",
		iface_st->beacon_rx, iface_st->rssi_mgmt);
	wl_info("on_time=%d, tx_time=%d\n",
		radio_st->on_time, radio_st->tx_time);
	wl_info("rx_time=%d, on_time_scan=%d,\n",
		radio_st->rx_time, radio_st->on_time_scan);
	radio_st->num_tx_levels = 1;
	radio_st->tx_time_per_levels = (u32 *)&llst->radio_tx_time;

	/*alloc radio reply buffer*/
	reply_radio_length = sizeof(struct wifi_radio_stat) + 1000;
	reply_iface_length = sizeof(struct wifi_iface_stat) + 1000;

	wl_info("start to put radio data\n");
	reply_radio = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
							  reply_radio_length);
	if (!reply_radio)
		goto out_put_fail;

	if (nla_put_u32(reply_radio, NL80211_ATTR_VENDOR_ID, OUI_SPREAD))
		goto out_put_fail;
	if (nla_put_u32(reply_radio, NL80211_ATTR_VENDOR_SUBCMD,
			SPRDWL_VENDOR_GET_LLSTAT))
		goto out_put_fail;
	if (nla_put_u32(reply_radio, SPRDWL_LL_STATS_TYPE,
			SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_RADIO))
		goto out_put_fail;

	ret = sprdwl_compose_radio_st(reply_radio, radio_st);

	ret = cfg80211_vendor_cmd_reply(reply_radio);

	wl_info("start to put iface data\n");
	/*alloc iface reply buffer*/
	reply_iface = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
							  reply_iface_length);
	if (!reply_iface)
		goto out_put_fail;

	if (nla_put_u32(reply_iface, NL80211_ATTR_VENDOR_ID, OUI_SPREAD))
		goto out_put_fail;
	if (nla_put_u32(reply_iface, NL80211_ATTR_VENDOR_SUBCMD,
			SPRDWL_VENDOR_GET_LLSTAT))
		goto out_put_fail;
	if (nla_put_u32(reply_iface, SPRDWL_LL_STATS_TYPE,
			SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_IFACE))
		goto out_put_fail;
	ret = sprdwl_compose_iface_st(reply_iface, iface_st);
	ret = cfg80211_vendor_cmd_reply(reply_iface);

	kfree(radio_st);
	kfree(iface_st);
	kfree(dif_radio);
	return ret;
out_put_fail:
	kfree(radio_st);
	kfree(iface_st);
	kfree(dif_radio);
	WARN_ON(1);
	return -EMSGSIZE;
}

/*clear link layer status function--- CMD ID:16*/
static int sprdwl_vendor_clr_llstat_handler(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int len)
{
	struct sk_buff *reply;
	struct wifi_clr_llstat_rsp clr_rsp;
	struct nlattr *tb[SPRDWL_LL_STATS_CLR_MAX + 1];
	u32 *stats_clear_rsp_mask, stats_clear_req_mask = 0;
	u16 r_len = sizeof(*stats_clear_rsp_mask);
	u8 r_buf[r_len];
	u32 reply_length, ret, err;

	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);

	if (!(priv->fw_capa & SPRDWL_CAPA_LL_STATS))
		return -ENOTSUPP;
	memset(r_buf, 0, r_len);
	if (!data) {
		wl_err("%s wrong llstat clear req mask\n", __func__);
		return -EINVAL;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	err = nla_parse(tb, SPRDWL_LL_STATS_CLR_MAX, data,
			len, NULL, NULL);
#else
	err = nla_parse(tb, SPRDWL_LL_STATS_CLR_MAX, data,
			len, NULL);
#endif
	if (err)
		return err;
	if (tb[SPRDWL_LL_STATS_CLR_CONFIG_REQ_MASK]) {
		stats_clear_req_mask =
		nla_get_u32(tb[SPRDWL_LL_STATS_CLR_CONFIG_REQ_MASK]);
	}
	wiphy_info(wiphy, "stats_clear_req_mask = %u\n", stats_clear_req_mask);
	ret = sprdwl_llstat(priv, vif->ctx_id, SPRDWL_SUBCMD_DEL,
				&stats_clear_req_mask, r_len, r_buf, &r_len);
	stats_clear_rsp_mask = (u32 *)r_buf;
	clr_rsp.stats_clear_rsp_mask = *stats_clear_rsp_mask;
	clr_rsp.stop_rsp = 1;

	reply_length = sizeof(clr_rsp) + 100;
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, reply_length);
	if (!reply)
		return -ENOMEM;
	if (nla_put_u32(reply, NL80211_ATTR_VENDOR_ID, OUI_SPREAD))
		goto out_put_fail;
	if (nla_put_u32(reply, NL80211_ATTR_VENDOR_SUBCMD,
			SPRDWL_VENDOR_CLR_LLSTAT))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_CLR_CONFIG_RSP_MASK,
			clr_rsp.stats_clear_rsp_mask))
		goto out_put_fail;
	if (nla_put_u32(reply, SPRDWL_LL_STATS_CLR_CONFIG_STOP_RSP,
			clr_rsp.stop_rsp))
		goto out_put_fail;
	ret = cfg80211_vendor_cmd_reply(reply);

	return ret;
out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}
/*start gscan functon, including scan params configuration------ CMD ID:20*/
static int sprdwl_vendor_gscan_start(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	u64 tlen;
	int i, j, ret = 0, enable;
	int rem_len, rem_outer_len, type;
	int rem_inner_len, rem_outer_len1, rem_inner_len1;
	struct nlattr *pos, *outer_iter, *inner_iter;
	struct nlattr *outer_iter1, *inner_iter1;
	struct sprdwl_cmd_gscan_set_config *params;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	wl_info("%s enter\n", __func__);
	params = kmalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	/*malloc memory to store scan params*/
	memset(params, 0, sizeof(*params));

	/*parse attri from hal, to configure scan params*/
	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);
		switch (type) {
		case GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID:
			vif->priv->gscan_req_id = nla_get_u32(pos);
		break;

		case GSCAN_ATTR_SCAN_CMD_PARAMS_BASE_PERIOD:
			params->base_period = nla_get_u32(pos);
		break;

		case GSCAN_ATTR_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN:
			params->maxAPperScan = nla_get_u32(pos);
		break;

		case GSCAN_ATTR_SCAN_CMD_PARAMS_REPORT_THR:
			params->reportThreshold = nla_get_u8(pos);
		break;

		case GSCAN_ATTR_SCAN_CMD_PARAMS_REPORT_THR_NUM_SCANS:
			params->report_threshold_num_scans
				= nla_get_u8(pos);
		break;

		case GSCAN_ATTR_SCAN_CMD_PARAMS_NUM_BUCKETS:
			params->num_buckets = nla_get_u8(pos);
		break;

		case GSCAN_ATTR_BUCKET_SPEC:
		i = 0;
		nla_for_each_nested(outer_iter, pos, rem_outer_len) {
			nla_for_each_nested(inner_iter, outer_iter,
						rem_inner_len){
				type = nla_type(inner_iter);
				switch (type) {
				case GSCAN_ATTR_BUCKET_SPEC_INDEX:
					params->buckets[i].bucket
						= nla_get_u8(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_BAND:
					params->buckets[i].band
						= nla_get_u8(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_PERIOD:
					params->buckets[i].period
						= nla_get_u32(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_REPORT_EVENTS:
					params->buckets[i].report_events
						= nla_get_u8(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_NUM_CHANNEL_SPECS:
					params->buckets[i].num_channels
						= nla_get_u32(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_MAX_PERIOD:
					params->buckets[i].max_period
						= nla_get_u32(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_BASE:
					params->buckets[i].base
						= nla_get_u32(inner_iter);
				break;

				case GSCAN_ATTR_BUCKET_SPEC_STEP_COUNT:
					params->buckets[i].step_count
						= nla_get_u32(inner_iter);
				break;

				case GSCAN_ATTR_CHANNEL_SPEC:
				j = 0;
				nla_for_each_nested(outer_iter1,
							inner_iter,
					rem_outer_len1) {
					nla_for_each_nested(inner_iter1,
								outer_iter1,
						rem_inner_len1) {
						type = nla_type(inner_iter1);

					switch (type) {
					case GSCAN_ATTR_CHANNEL_SPEC_CHANNEL:
					params->buckets[i].channels[j].channel
						= nla_get_u32(inner_iter1);
					break;
					case GSCAN_ATTR_CHANNEL_SPEC_DWELL_TIME:
					params->buckets[i].channels[j].dwelltime
						= nla_get_u32(inner_iter1);
					break;
					case GSCAN_ATTR_CHANNEL_SPEC_PASSIVE:
					params->buckets[i].channels[j].passive
						= nla_get_u32(inner_iter1);
					break;
					}
					}
					j++;
					if (j >= MAX_CHANNELS)
						break;
				}
				break;

				default:
					wl_ndev_log(L_ERR, vif->ndev,
						   "bucket nla type 0x%x not support\n",
						   type);
					ret = -EINVAL;
				break;
				}
			}
			if (ret < 0)
				break;
			i++;
			if (i >= MAX_BUCKETS)
				break;
		}
		break;

		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
				   type);
			ret = -EINVAL;
			break;
		}
	}

	wl_ndev_log(L_INFO, vif->ndev, "parse config %s\n",
			!ret ? "success" : "failture");

	kfree(vif->priv->gscan_res);
	vif->priv->gscan_buckets_num = params->num_buckets;
	tlen = sizeof(struct sprdwl_gscan_cached_results);

	/*malloc memory to store scan results*/
	vif->priv->gscan_res =
		kmalloc((u64)(vif->priv->gscan_buckets_num * tlen),
				GFP_KERNEL);

	if (!vif->priv->gscan_res) {
		kfree(params);
		return -ENOMEM;
	}

	memset(vif->priv->gscan_res, 0x0,
		   vif->priv->gscan_buckets_num *
		sizeof(struct sprdwl_gscan_cached_results));

	tlen = sizeof(struct sprdwl_cmd_gscan_set_config);

	for (i = 0; i < params->num_buckets; i++) {
		if (params->buckets[i].num_channels == 0) {
			wl_err("%s, %d, gscan channel not set\n", __func__, __LINE__);
			params->buckets[i].num_channels = 11;
			for (j = 0; j < 11; j++)
				params->buckets[i].channels[j].channel = j+1;
		}
	}
	/*send scan params configure command*/
	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  (void *)params,
		SPRDWL_GSCAN_SUBCMD_SET_CONFIG,
		tlen, (u8 *)(&rsp), &rlen);
	if (ret == 0) {
		enable = 1;

		/*start gscan*/
		ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					  (void *)(&enable),
			SPRDWL_GSCAN_SUBCMD_ENABLE_GSCAN,
			sizeof(int), (u8 *)(&rsp), &rlen);
	}

	if (ret < 0)
		kfree(vif->priv->gscan_res);
	kfree(params);

	return ret;
}

/*stop gscan functon------ CMD ID:21*/
static int sprdwl_vendor_gscan_stop(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					  const void *data, int len)
{
	int enable;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	enable = 0;
	wl_ndev_log(L_INFO, vif->ndev, "%s\n", __func__);

	return sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					(void *)(&enable),
					SPRDWL_GSCAN_SUBCMD_ENABLE_GSCAN,
					sizeof(int), (u8 *)(&rsp), &rlen);
}

/*get valid channel list functon, need input band value------ CMD ID:22*/
static int sprdwl_vendor_get_channel_list(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
					  const void *data, int len)
{
	int ret = 0, payload, request_id;
	int type;
	int band = 0, max_channel;
	int rem_len;
	struct nlattr *pos;
	struct sprdwl_cmd_gscan_channel_list channel_list;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sk_buff *reply;
	u16 rlen;

	rlen = sizeof(struct sprdwl_cmd_gscan_channel_list)
		+ sizeof(struct sprdwl_cmd_gscan_rsp_header);

	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);
		switch (type) {
		case GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID:
			request_id = nla_get_s32(pos);
		break;
		case GSCAN_ATTR_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND:
			band = nla_get_s32(pos);
		break;
		case GSCAN_ATTR_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS:
			max_channel = nla_get_s32(pos);
		break;
		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
				   type);
			ret = -EINVAL;
		break;
		}
		if (ret < 0)
		break;
	}

	wl_ndev_log(L_INFO, vif->ndev, "parse channel list %s band=%d\n",
			!ret ? "success" : "failture", band);

	payload = rlen + 0x100;
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);
	if (!reply) {
		ret = -ENOMEM;
		goto out;
	}
	if (band == WIFI_BAND_A) {
		channel_list.num_channels =
			vif->priv->ch_5g_without_dfs_info.num_channels;
		memcpy(channel_list.channels,
			vif->priv->ch_5g_without_dfs_info.channels,
			sizeof(int) * channel_list.num_channels);
	} else if (band == WIFI_BAND_A_DFS) {
		channel_list.num_channels =
			vif->priv->ch_5g_dfs_info.num_channels;
		memcpy(channel_list.channels,
			vif->priv->ch_5g_dfs_info.channels,
			sizeof(int) * channel_list.num_channels);
	} else {
		/*return 2.4G channel list by default*/
		channel_list.num_channels = vif->priv->ch_2g4_info.num_channels;
		memcpy(channel_list.channels, vif->priv->ch_2g4_info.channels,
			sizeof(int) * channel_list.num_channels);
	}

	if (nla_put_u32(reply, GSCAN_RESULTS_NUM_CHANNELS,
				channel_list.num_channels))
		goto out_put_fail;
	if (nla_put(reply, GSCAN_RESULTS_CHANNELS,
		sizeof(int) * channel_list.num_channels, channel_list.channels))

		goto out_put_fail;
	ret = cfg80211_vendor_cmd_reply(reply);
	if (ret)
		wl_ndev_log(L_ERR, vif->ndev, "%s failed to reply skb!\n", __func__);

out:
	return ret;
out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

/*Gscan get capabilities function----CMD ID:23*/
static int sprdwl_vendor_get_gscan_capabilities(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int len)
{
	u16 rlen;
	struct sk_buff *reply;
	int ret = 0, payload;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_cmd_gscan_rsp_header *hdr;
	struct sprdwl_gscan_capa *p = NULL;
	void *rbuf;

	wl_info("%s enter\n", __func__);

	rlen = sizeof(struct sprdwl_gscan_capa) +
		sizeof(struct sprdwl_cmd_gscan_rsp_header);
	rbuf = kmalloc(rlen, GFP_KERNEL);
	if (!rbuf)
		return -ENOMEM;

	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  NULL, SPRDWL_GSCAN_SUBCMD_GET_CAPABILITIES,
					0, (u8 *)rbuf, &rlen);

	if (ret < 0) {
		wl_ndev_log(L_ERR, vif->ndev, "%s failed to get capabilities!\n",
			   __func__);
		goto out;
	}
	hdr = (struct sprdwl_cmd_gscan_rsp_header *)rbuf;
	p = (struct sprdwl_gscan_capa *)
		(rbuf + sizeof(struct sprdwl_cmd_gscan_rsp_header));
	wl_info("cache_size: %d scan_bucket:%d\n",
		p->max_scan_cache_size, p->max_scan_buckets);
	wl_info("max AP per scan:%d,max_rssi_sample_size:%d\n",
		p->max_ap_cache_per_scan, p->max_rssi_sample_size);
	wl_info("max_white_list:%d,max_black_list:%d\n",
		p->max_whitelist_ssid, p->max_blacklist_size);
	payload = rlen + 0x100;
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);
	if (!reply) {
		ret = -ENOMEM;
		goto out;
	}

	if (nla_put_u32(reply, GSCAN_SCAN_CACHE_SIZE,
			p->max_scan_cache_size) ||
		nla_put_u32(reply, GSCAN_MAX_SCAN_BUCKETS,
				p->max_scan_buckets) ||
		nla_put_u32(reply, GSCAN_MAX_AP_CACHE_PER_SCAN,
				p->max_ap_cache_per_scan) ||
		nla_put_u32(reply, GSCAN_MAX_RSSI_SAMPLE_SIZE,
				p->max_rssi_sample_size) ||
		nla_put_s32(reply, GSCAN_MAX_SCAN_REPORTING_THRESHOLD,
				p->max_scan_reporting_threshold) ||
		nla_put_u32(reply, GSCAN_MAX_HOTLIST_BSSIDS,
				p->max_hotlist_bssids) ||
		nla_put_u32(reply, GSCAN_MAX_SIGNIFICANT_WIFI_CHANGE_APS,
				p->max_significant_wifi_change_aps) ||
		nla_put_u32(reply, GSCAN_MAX_BSSID_HISTORY_ENTRIES,
				p->max_bssid_history_entries) ||
		nla_put_u32(reply, GSCAN_MAX_HOTLIST_SSIDS,
				p->max_hotlist_bssids) ||
		nla_put_u32(reply, GSCAN_MAX_NUM_EPNO_NETS,
				p->max_number_epno_networks) ||
		nla_put_u32(reply, GSCAN_MAX_NUM_EPNO_NETS_BY_SSID,
				p->max_number_epno_networks_by_ssid) ||
		nla_put_u32(reply, GSCAN_MAX_NUM_WHITELISTED_SSID,
				p->max_whitelist_ssid) ||
		nla_put_u32(reply, GSCAN_MAX_NUM_BLACKLISTED_BSSID,
				p->max_blacklist_size)){
		wl_err("failed to put Gscan capabilies\n");
		goto out_put_fail;
	}
		vif->priv->roam_capa.max_blacklist_size = p->max_blacklist_size;
		vif->priv->roam_capa.max_whitelist_size = p->max_whitelist_ssid;

	ret = cfg80211_vendor_cmd_reply(reply);
	if (ret)
		wl_ndev_log(L_ERR, vif->ndev, "%s failed to reply skb!\n", __func__);
out:
	kfree(rbuf);
	return ret;
out_put_fail:
	kfree_skb(reply);
	kfree(rbuf);
	return ret;
}

/*get cached gscan results functon------ CMD ID:24*/
static int sprdwl_vendor_get_cached_gscan_results(struct wiphy *wiphy,
						  struct wireless_dev *wdev,
					   const void *data, int len)
{
	int ret = 0, i, j, rlen, payload, request_id = 0, moredata = 0;
	int rem_len, type, flush = 0, max_param = 0, n, buckets_scanned = 1;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sk_buff *reply;
	struct nlattr *pos, *scan_res, *cached_list, *res_list;

	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);
		switch (type) {
		case GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID:
			request_id = nla_get_u32(pos);
		break;
		case GSCAN_ATTR_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH:
			flush = nla_get_u32(pos);
		break;
		case GSCAN_ATTR_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX:
			max_param = nla_get_u32(pos);
		break;
		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla gscan result 0x%x not support\n",
				   type);
			ret = -EINVAL;
		break;
		}
		if (ret < 0)
			break;
	}

	rlen = vif->priv->gscan_buckets_num
		* sizeof(struct sprdwl_gscan_cached_results);
	payload = rlen + 0x100;
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);
	if (!reply)
		return -ENOMEM;
	for (i = 0; i < vif->priv->gscan_buckets_num; i++) {
		if (!(vif->priv->gscan_res + i)->num_results)
			continue;

		for (j = 0; j <= (vif->priv->gscan_res + i)->num_results; j++) {
			if (time_after(jiffies - VENDOR_SCAN_RESULT_EXPIRE,
				(vif->priv->gscan_res + i)->results[j].ts)) {
				memcpy((void *)
				(&(vif->priv->gscan_res + i)->results[j]),
				(void *)
				(&(vif->priv->gscan_res + i)->results[j + 1]),
				sizeof(struct sprdwl_gscan_result)
				* ((vif->priv->gscan_res + i)->num_results
				- j - 1));
				(vif->priv->gscan_res + i)->num_results--;
				j = 0;
			}
		}

		if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
				request_id) ||
			nla_put_u32(reply,
					GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,
			(vif->priv->gscan_res + i)->num_results)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

		if (nla_put_u8(reply,
				   GSCAN_RESULTS_SCAN_RESULT_MORE_DATA,
			moredata)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

		if (nla_put_u32(reply,
				GSCAN_CACHED_RESULTS_SCAN_ID,
			(vif->priv->gscan_res + i)->scan_id)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

		if ((vif->priv->gscan_res + i)->num_results == 0)
			break;

		cached_list = nla_nest_start(reply, GSCAN_CACHED_RESULTS_LIST);
		for (n = 0; n < vif->priv->gscan_buckets_num; n++) {
			res_list = nla_nest_start(reply, n);

			if (!res_list)
				goto out_put_fail;

			if (nla_put_u32(reply,
					GSCAN_CACHED_RESULTS_SCAN_ID,
				(vif->priv->gscan_res + i)->scan_id)) {
				wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
				goto out_put_fail;
			}

			if (nla_put_u32(reply,
					GSCAN_CACHED_RESULTS_FLAGS,
				(vif->priv->gscan_res + i)->flags)) {
				wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
				goto out_put_fail;
			}

			if (nla_put_u32(reply,
					GSCAN_RESULTS_BUCKETS_SCANNED,
				buckets_scanned)) {
				wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
				goto out_put_fail;
			}

			if (nla_put_u32(reply,
					GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,
				(vif->priv->gscan_res + i)->num_results)) {
				wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
				goto out_put_fail;
			}

			scan_res = nla_nest_start(reply, GSCAN_RESULTS_LIST);
			if (!scan_res)
				goto out_put_fail;

			for (j = 0;
			j < (vif->priv->gscan_res + i)->num_results;
			j++) {
				struct nlattr *ap;
				struct sprdwl_gscan_cached_results *p
						= vif->priv->gscan_res + i;

				wl_info("[index=%d] Timestamp(%lu) Ssid (%s) Bssid: %pM Channel (%d) Rssi (%d) RTT (%u) RTT_SD (%u)\n",
					j,
					p->results[j].ts,
					p->results[j].ssid,
					p->results[j].bssid,
					p->results[j].channel,
					p->results[j].rssi,
					p->results[j].rtt,
					p->results[j].rtt_sd);

				ap = nla_nest_start(reply, j + 1);
				if (!ap) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
				if (nla_put_u64_64bit(reply,
					  GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
					p->results[j].ts, 0)) {
#else
				if (nla_put_u64(reply,
					GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
					p->results[j].ts)) {
#endif
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
				if (nla_put(reply,
					  GSCAN_RESULTS_SCAN_RESULT_SSID,
					sizeof(p->results[j].ssid),
					p->results[j].ssid)) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
				if (nla_put(reply,
						GSCAN_RESULTS_SCAN_RESULT_BSSID,
					sizeof(p->results[j].bssid),
					p->results[j].bssid)) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
				if (nla_put_u32(reply,
					GSCAN_RESULTS_SCAN_RESULT_CHANNEL,
					p->results[j].channel)) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
				if (nla_put_s32(reply,
						GSCAN_RESULTS_SCAN_RESULT_RSSI,
					p->results[j].rssi)) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
				if (nla_put_u32(reply,
						GSCAN_RESULTS_SCAN_RESULT_RTT,
					p->results[j].rtt)) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
				if (nla_put_u32(reply,
					GSCAN_RESULTS_SCAN_RESULT_RTT_SD,
					p->results[j].rtt_sd)) {
					wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
					goto out_put_fail;
				}
			nla_nest_end(reply, ap);
			}
		nla_nest_end(reply, scan_res);
		nla_nest_end(reply, res_list);
		}
	nla_nest_end(reply, cached_list);
	}

	ret = cfg80211_vendor_cmd_reply(reply);
	if (ret < 0)
		wl_ndev_log(L_ERR, vif->ndev, "%s failed to reply skb!\n", __func__);
	return ret;

out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

/*buffer scan result in host driver when receive frame from cp2*/
int sprdwl_vendor_cache_scan_result(struct sprdwl_vif *vif,
						u8 bucket_id,
						struct sprdwl_gscan_result *item)
{
	struct sprdwl_priv *priv = vif->priv;
	u32 i;
	struct sprdwl_gscan_cached_results *p = NULL;

	if (bucket_id >= priv->gscan_buckets_num || !priv->gscan_res) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the gscan buffer invalid!priv->gscan_buckets_num: %d, bucket_id:%d\n",
			   __func__, priv->gscan_buckets_num, bucket_id);
		return -EINVAL;
	}
	for (i = 0; i < priv->gscan_buckets_num; i++) {
		p = priv->gscan_res + i;
		if (p->scan_id == bucket_id)
			break;
	}
	if (!p) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the bucket isnot exsit.\n", __func__);
		return -EINVAL;
	}
	if (MAX_AP_CACHE_PER_SCAN <= p->num_results) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the scan result reach the MAX num.\n",
			   __func__);
		return -EINVAL;
	}

	for (i = 0; i < p->num_results; i++) {
		if (time_after(jiffies - VENDOR_SCAN_RESULT_EXPIRE,
				   p->results[i].ts)) {
			memcpy((void *)(&p->results[i]),
				   (void *)(&p->results[i+1]),
				sizeof(struct sprdwl_gscan_result)
				* (p->num_results - i - 1));

			p->num_results--;
		}

		if (!memcmp(p->results[i].bssid, item->bssid, ETH_ALEN) &&
			strlen(p->results[i].ssid) == strlen(item->ssid) &&
			!memcmp(p->results[i].ssid, item->ssid,
				strlen(item->ssid))) {
			wl_ndev_log(L_ERR, vif->ndev, "%s BSS : %s  %pM exist, but also update it.\n",
				   __func__, item->ssid, item->bssid);

			memcpy((void *)(&p->results[i]),
				   (void *)item,
				sizeof(struct sprdwl_gscan_result));
			return 0;
		}
	}
	memcpy((void *)(&p->results[p->num_results]),
		   (void *)item, sizeof(struct sprdwl_gscan_result));
	p->results[p->num_results].ie_length = 0;
	p->results[p->num_results].ie_data[0] = 0;
	p->num_results++;
	return 0;
}

int sprdwl_vendor_cache_hotlist_result(struct sprdwl_vif *vif,
						  struct sprdwl_gscan_result *item)
{
	struct sprdwl_priv *priv = vif->priv;
	u32 i;
	struct sprdwl_gscan_hotlist_results *p = priv->hotlist_res;

	if (!priv->hotlist_res) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the hotlist buffer invalid!\n",
			   __func__);
		return -EINVAL;
	}

	if (MAX_HOTLIST_APS <= p->num_results) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the hotlist result reach the MAX num.\n",
			   __func__);
		return -EINVAL;
	}

	for (i = 0; i < p->num_results; i++) {
		if (time_after(jiffies - VENDOR_SCAN_RESULT_EXPIRE,
				   p->results[i].ts)) {
			memcpy((void *)(&p->results[i]),
				   (void *)(&p->results[i+1]),
				sizeof(struct sprdwl_gscan_result)
				* (p->num_results - i - 1));

			p->num_results--;
		}

		if (!memcmp(p->results[i].bssid, item->bssid, ETH_ALEN) &&
			strlen(p->results[i].ssid) == strlen(item->ssid) &&
			!memcmp(p->results[i].ssid, item->ssid,
				strlen(item->ssid))) {
			wl_ndev_log(L_ERR, vif->ndev, "%s BSS : %s  %pM exist, but also update it.\n",
				   __func__, item->ssid, item->bssid);

			memcpy((void *)(&p->results[i]),
				   (void *)item,
				sizeof(struct sprdwl_gscan_result));
			return 0;
		}
	}
	memcpy((void *)(&p->results[p->num_results]),
		   (void *)item, sizeof(struct sprdwl_gscan_result));
	p->results[p->num_results].ie_length = 0;
	p->results[p->num_results].ie_data[0] = 0;
	p->num_results++;
	return 0;
}

int sprdwl_vendor_cache_significant_change_result(struct sprdwl_vif *vif,
							u8 *data, u16 data_len)
{
	struct sprdwl_priv *priv = vif->priv;
	struct significant_change_info *frame;
	struct sprdwl_significant_change_result *p = priv->significant_res;
	u8 *pos = data;
	u16 avail_len = data_len;

	if (!priv->significant_res) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the significant_change buffer invalid!\n",
			   __func__);
		return -EINVAL;
	}

	if (MAX_SIGNIFICANT_CHANGE_APS <= p->num_results) {
		wl_ndev_log(L_ERR, vif->ndev, "%s the significant_change result reach the MAX num.\n",
			   __func__);
		return -EINVAL;
	}

	while (avail_len > 0) {
		if (avail_len < (sizeof(struct significant_change_info) + 1)) {
			wl_ndev_log(L_ERR, vif->ndev,
				   "%s invalid available length: %d!\n",
				   __func__, avail_len);
			break;
		}

		pos++;
		frame = (struct significant_change_info *)pos;

		memcpy((void *)(&p->results[p->num_results]),
			   (void *)pos, sizeof(struct significant_change_info));
		p->num_results++;

		avail_len -= sizeof(struct significant_change_info) + 1;
		pos += sizeof(struct significant_change_info);
	}
	return 0;
}

/*report full scan result to upper layer, it will only report one AP,*/
/*including its IE data*/
int sprdwl_vendor_report_full_scan(struct sprdwl_vif *vif,
					  struct sprdwl_gscan_result *item)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *reply;
	int payload, rlen;
	int ret = 0;

	rlen = sizeof(struct sprdwl_gscan_result) + item->ie_length;
	payload = rlen + 0x100;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	reply = cfg80211_vendor_event_alloc(wiphy, &vif->wdev,
#else
	reply = cfg80211_vendor_event_alloc(wiphy,
#endif
						payload,
		NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT_INDEX,
		GFP_KERNEL);
	if (!reply) {
		ret = -ENOMEM;
		goto out;
	}

	if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
			priv->gscan_req_id) ||
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	nla_put_u64_64bit(reply,
			GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
		item->ts, 0)
#else
	nla_put_u64(reply,
			GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
		item->ts)
#endif
			||
	nla_put(reply, GSCAN_RESULTS_SCAN_RESULT_SSID,
		sizeof(item->ssid),
		item->ssid) ||
	nla_put(reply, GSCAN_RESULTS_SCAN_RESULT_BSSID,
		6,
		item->bssid) ||
	nla_put_u32(reply,
			GSCAN_RESULTS_SCAN_RESULT_CHANNEL,
		item->channel) ||
	nla_put_s32(reply, GSCAN_RESULTS_SCAN_RESULT_RSSI,
			item->rssi) ||
	nla_put_u32(reply, GSCAN_RESULTS_SCAN_RESULT_RTT,
			item->rtt) ||
	nla_put_u32(reply,
			GSCAN_RESULTS_SCAN_RESULT_RTT_SD,
		item->rtt_sd) ||
	nla_put_u16(reply,
			GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD,
		item->beacon_period) ||
	nla_put_u16(reply,
			GSCAN_RESULTS_SCAN_RESULT_CAPABILITY,
		item->capability) ||
	nla_put_u32(reply,
			GSCAN_RESULTS_SCAN_RESULT_IE_LENGTH,
		item->ie_length))	{
		wl_ndev_log(L_ERR, vif->ndev, "%s nla put fail\n", __func__);
		goto out_put_fail;
	}
	if (nla_put(reply, GSCAN_RESULTS_SCAN_RESULT_IE_DATA,
			item->ie_length,
		item->ie_data)) {
		wl_ndev_log(L_ERR, vif->ndev, "%s nla put fail\n", __func__);
		goto out_put_fail;
	}

	cfg80211_vendor_event(reply, GFP_KERNEL);
out:
	return ret;

out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

void sprdwl_report_gscan_result(struct sprdwl_vif *vif,
				u32 report_event, u8 bucket_id,
				u16 chan, s16 rssi, const u8 *frame, u16 len)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)frame;
	struct ieee80211_channel *channel;
	struct sprdwl_gscan_result *gscan_res = NULL;
	u16 capability, beacon_interval;
	u32 freq;
	s32 signal;
	u64 tsf;
	u8 *ie;
	size_t ielen;
	const u8 *ssid;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	freq = ieee80211_channel_to_frequency(chan,
						  chan <= CH_MAX_2G_CHANNEL ?
						NL80211_BAND_2GHZ : NL80211_BAND_5GHZ);
#else
	freq = ieee80211_channel_to_frequency(chan,
						chan <= CH_MAX_2G_CHANNEL ?
						IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ);
#endif
	channel = ieee80211_get_channel(wiphy, freq);
	if (!channel) {
		wl_ndev_log(L_ERR, vif->ndev, "%s invalid freq!\n", __func__);
		return;
	}
	signal = rssi * 100;
	if (!mgmt) {
		wl_ndev_log(L_ERR, vif->ndev, "%s NULL frame!\n", __func__);
		return;
	}
	ie = mgmt->u.probe_resp.variable;
	ielen = len - offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
	/*tsf = le64_to_cpu(mgmt->u.probe_resp.timestamp);*/
	tsf = jiffies;
	beacon_interval = le16_to_cpu(mgmt->u.probe_resp.beacon_int);
	capability = le16_to_cpu(mgmt->u.probe_resp.capab_info);
	wl_ndev_log(L_DBG, vif->ndev, "   %s, %pM, channel %2u, signal %d\n",
		   ieee80211_is_probe_resp(mgmt->frame_control)
		   ? "proberesp" : "beacon   ", mgmt->bssid, chan, rssi);

	gscan_res = kmalloc(sizeof(*gscan_res) + ielen, GFP_KERNEL);
	if (!gscan_res)
		return;
	memset(gscan_res, 0x0, sizeof(struct sprdwl_gscan_result) + ielen);
	gscan_res->channel = freq;
	gscan_res->beacon_period = beacon_interval;
	gscan_res->ts = tsf;
	gscan_res->rssi = signal;
	gscan_res->ie_length = ielen;
	memcpy(gscan_res->bssid, mgmt->bssid, 6);
	memcpy(gscan_res->ie_data, ie, ielen);

	ssid = wpa_scan_get_ie(ie, ielen, WLAN_EID_SSID);
	if (!ssid) {
		wl_ndev_log(L_ERR, vif->ndev, "%s BSS: No SSID IE included for %pM!\n",
			   __func__, mgmt->bssid);
		goto out;
	}
	if (ssid[1] > 32) {
		wl_ndev_log(L_ERR, vif->ndev, "%s BSS: Too long SSID IE for %pM!\n",
			   __func__, mgmt->bssid);
		goto out;
	}
	memcpy(gscan_res->ssid, ssid + 2, ssid[1]);
	wl_ndev_log(L_ERR, vif->ndev, "%s %pM : %s !report_event =%d\n", __func__,
		   mgmt->bssid, gscan_res->ssid, report_event);

	if ((report_event == REPORT_EVENTS_BUFFER_FULL) ||
		(report_event & REPORT_EVENTS_EACH_SCAN) ||
		(report_event & REPORT_EVENTS_FULL_RESULTS) ||
		(report_event & REPORT_EVENTS_SIGNIFICANT_CHANGE)) {
		sprdwl_vendor_cache_scan_result(vif, bucket_id, gscan_res);
	} else if ((report_event & REPORT_EVENTS_HOTLIST_RESULTS_FOUND) ||
		(report_event & REPORT_EVENTS_HOTLIST_RESULTS_LOST)) {
		sprdwl_vendor_cache_hotlist_result(vif, gscan_res);
	}

	if (report_event & REPORT_EVENTS_FULL_RESULTS)
		sprdwl_vendor_report_full_scan(vif, gscan_res);
out:
	kfree(gscan_res);
}

/*report event to upper layer when buffer is full,*/
/*it only include event, not scan result*/
int sprdwl_buffer_full_event(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *reply;
	int payload, rlen;
	int ret = 0;

	rlen = sizeof(enum sprdwl_gscan_event);
	payload = rlen + 0x100;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	reply = cfg80211_vendor_event_alloc(wiphy, &vif->wdev,
#else
	reply = cfg80211_vendor_event_alloc(wiphy,
#endif
						payload,
		NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE_INDEX,
		GFP_KERNEL);
	if (!reply) {
		ret = -ENOMEM;
		goto out;
	}

	if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
			priv->gscan_req_id))
		goto out_put_fail;

	cfg80211_vendor_event(reply, GFP_KERNEL);
out:
	return ret;
out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

int sprdwl_available_event(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *reply;
	int payload, rlen;
	int ret = 0;

	rlen = sizeof(enum nl80211_vendor_subcmds_index);
	payload = rlen + 0x100;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	reply = cfg80211_vendor_event_alloc(wiphy, &vif->wdev,
#else
	reply = cfg80211_vendor_event_alloc(wiphy,
#endif
						payload,
		NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE_INDEX,
		GFP_KERNEL);
	if (!reply) {
		ret = -ENOMEM;
		goto out;
	}

	if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
			priv->gscan_req_id))
		goto out_put_fail;

	cfg80211_vendor_event(reply, GFP_KERNEL);
out:
	return ret;
out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

/*report scan done event to upper layer*/
int sprdwl_gscan_done(struct sprdwl_vif *vif, u8 bucketid)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *reply;
	int payload, rlen, ret = 0;
	u8 event_type;

	rlen = sizeof(enum nl80211_vendor_subcmds_index);
	payload = rlen + 0x100;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	reply = cfg80211_vendor_event_alloc(wiphy, &vif->wdev,
#else
	reply = cfg80211_vendor_event_alloc(wiphy,
#endif
						payload,
		NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT_INDEX,
		GFP_KERNEL);
	if (!reply) {
		ret = -ENOMEM;
		goto out;
	}

	if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
			priv->gscan_req_id))
		goto out_put_fail;

	event_type = WIFI_SCAN_COMPLETE;
	if (nla_put_u8(reply, GSCAN_RESULTS_SCAN_EVENT_TYPE,
			   event_type))
		goto out_put_fail;

	cfg80211_vendor_event(reply, GFP_KERNEL);
out:
	return ret;
out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

/*set_ssid_hotlist function---CMD ID:29*/
static int sprdwl_vendor_set_bssid_hotlist(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					 const void *data, int len)
{
	int i, ret = 0, tlen;
	int type;
	int rem_len, rem_outer_len, rem_inner_len;
	struct nlattr *pos, *outer_iter, *inner_iter;
	struct wifi_bssid_hotlist_params *bssid_hotlist_params;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	bssid_hotlist_params =
		kmalloc(sizeof(*bssid_hotlist_params), GFP_KERNEL);

	if (!bssid_hotlist_params)
		return -ENOMEM;

	vif->priv->hotlist_res =
			kmalloc(sizeof(struct sprdwl_gscan_hotlist_results),
				GFP_KERNEL);

	if (!vif->priv->hotlist_res) {
		ret = -ENOMEM;
		goto out;
	}

	memset(vif->priv->hotlist_res, 0x0,
		   sizeof(struct sprdwl_gscan_hotlist_results));

	nla_for_each_attr(pos, (void *)data, len, rem_len) {
			type = nla_type(pos);

		switch (type) {
		case GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID:
			vif->priv->hotlist_res->req_id = nla_get_s32(pos);
		break;

		case GSCAN_ATTR_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE:
			bssid_hotlist_params->lost_ap_sample_size
				= nla_get_s32(pos);
		break;

		case GSCAN_ATTR_BSSID_HOTLIST_PARAMS_NUM_AP:
			bssid_hotlist_params->num_bssid = nla_get_s32(pos);
		break;

		case GSCAN_ATTR_AP_THR_PARAM:
			i = 0;
			nla_for_each_nested(outer_iter, pos, rem_outer_len) {
				nla_for_each_nested(inner_iter, outer_iter,
				rem_inner_len) {
					type = nla_type(inner_iter);
					switch (type) {
					case GSCAN_ATTR_AP_THR_PARAM_BSSID:
						memcpy(
						bssid_hotlist_params->ap[i].bssid,
						nla_data(inner_iter),
						6 * sizeof(unsigned char));
					break;

					case GSCAN_ATTR_AP_THR_PARAM_RSSI_LOW:
						bssid_hotlist_params->ap[i].low
						= nla_get_s32(inner_iter);
					break;

					case GSCAN_ATTR_AP_THR_PARAM_RSSI_HIGH:
						bssid_hotlist_params->ap[i].high
						= nla_get_s32(inner_iter);
					break;
					default:
					wl_ndev_log(L_ERR, vif->ndev,
						"networks nla type 0x%x not support\n",
						type);
						ret = -EINVAL;
					break;
					}
				}

				if (ret < 0)
					break;

				i++;
				if (i >= MAX_HOTLIST_APS)
					break;
			}
		break;

		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
			type);
			ret = -EINVAL;
		break;
		}

		if (ret < 0)
			break;
		}

	wl_ndev_log(L_INFO, vif->ndev, "parse bssid hotlist %s\n",
			!ret ? "success" : "failture");

	tlen = sizeof(struct wifi_bssid_hotlist_params);

	if (!ret)
		ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					  (void *)bssid_hotlist_params,
				SPRDWL_GSCAN_SUBCMD_SET_HOTLIST,
				tlen, (u8 *)(&rsp), &rlen);

	if (ret < 0)
		kfree(vif->priv->hotlist_res);

out:
	kfree(bssid_hotlist_params);
	return ret;
}

/*reset_bssid_hotlist function---CMD ID:30*/
static int sprdwl_vendor_reset_bssid_hotlist(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
					 const void *data, int len)
{
	int flush = 1;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	wl_ndev_log(L_INFO, vif->ndev, "%s %d\n", __func__, flush);

	memset(vif->priv->hotlist_res, 0x0,
		sizeof(struct sprdwl_gscan_hotlist_results));

	return sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					(void *)(&flush),
					SPRDWL_GSCAN_SUBCMD_RESET_HOTLIST,
					sizeof(int), (u8 *)(&rsp), &rlen);
}

/*set_significant_change function---CMD ID:32*/
static int sprdwl_vendor_set_significant_change(struct wiphy *wiphy,
						struct wireless_dev *wdev,
					 const void *data, int len)
{
	int i, ret = 0, tlen;
	int type;
	int rem_len, rem_outer_len, rem_inner_len;
	struct nlattr *pos, *outer_iter, *inner_iter;
	struct wifi_significant_change_params *significant_change_params;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	significant_change_params =
		kmalloc(sizeof(*significant_change_params), GFP_KERNEL);

	if (!significant_change_params)
		return -ENOMEM;

	vif->priv->significant_res =
			kmalloc(sizeof(struct sprdwl_significant_change_result),
				GFP_KERNEL);

	if (!vif->priv->significant_res) {
		ret = -ENOMEM;
		goto out;
	}

	memset(vif->priv->significant_res, 0x0,
		   sizeof(struct sprdwl_significant_change_result));


	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);
		switch (type) {
		case GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID:
			vif->priv->significant_res->req_id = nla_get_s32(pos);
		break;

		case GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE:
			significant_change_params->rssi_sample_size
				= nla_get_s32(pos);
		break;

		case GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE:
			significant_change_params->lost_ap_sample_size
				= nla_get_s32(pos);
		break;

		case GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING:
			significant_change_params->min_breaching
				= nla_get_s32(pos);
		break;

		case GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_NUM_AP:
			significant_change_params->num_bssid
				= nla_get_s32(pos);
		break;

		case GSCAN_ATTR_AP_THR_PARAM:
		i = 0;
		nla_for_each_nested(outer_iter, pos, rem_outer_len) {
			nla_for_each_nested(inner_iter, outer_iter,
				rem_inner_len) {
				type = nla_type(inner_iter);
				switch (type) {
				case GSCAN_ATTR_AP_THR_PARAM_BSSID:
					memcpy(
					significant_change_params->ap[i].bssid,
					nla_data(inner_iter),
					6 * sizeof(unsigned char));
				break;

				case GSCAN_ATTR_AP_THR_PARAM_RSSI_LOW:
					significant_change_params->ap[i].low
						= nla_get_s32(inner_iter);
				break;

				case GSCAN_ATTR_AP_THR_PARAM_RSSI_HIGH:
					significant_change_params->ap[i].high
						= nla_get_s32(inner_iter);
				break;
				default:
					wl_ndev_log(L_ERR, vif->ndev,
					"networks nla type 0x%x not support\n",
					type);
					ret = -EINVAL;
				break;
				}
			}

			if (ret < 0)
				break;

			i++;
			if (i >= MAX_SIGNIFICANT_CHANGE_APS)
				break;
		}
		break;

		default:
		wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
		type);
		ret = -EINVAL;
		break;
		}

		if (ret < 0)
			break;
	}

	tlen = sizeof(struct wifi_significant_change_params);
	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  (void *)significant_change_params,
			SPRDWL_GSCAN_SUBCMD_SET_SIGNIFICANT_CHANGE_CONFIG,
			tlen, (u8 *)(&rsp), &rlen);

	if (ret < 0)
		kfree(vif->priv->significant_res);

out:
	kfree(significant_change_params);
	return ret;
}

/*set_significant_change function---CMD ID:33*/
static int sprdwl_vendor_reset_significant_change(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
					 const void *data, int len)
{
	int flush = 1;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	wl_ndev_log(L_INFO, vif->ndev, "%s %d\n", __func__, flush);

	if (vif->priv->significant_res) {
		memset(vif->priv->significant_res, 0x0,
			sizeof(struct sprdwl_significant_change_result));
	}

	return sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
		(void *)(&flush),
		SPRDWL_GSCAN_SUBCMD_RESET_SIGNIFICANT_CHANGE_CONFIG,
		sizeof(int),
		(u8 *)(&rsp),
		&rlen);
}

/*get support feature function---CMD ID:38*/
static int sprdwl_vendor_get_support_feature(struct wiphy *wiphy,
						 struct wireless_dev *wdev,
						const void *data, int len)
{
	int ret;
	struct sk_buff *reply;
	uint32_t feature = 0, payload;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);

	wiphy_info(wiphy, "%s\n", __func__);
	payload = sizeof(feature);
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);

	if (!reply)
		return -ENOMEM;
	/*bit 1:Basic infrastructure mode*/
	if (wiphy->interface_modes & BIT(NL80211_IFTYPE_STATION)) {
		wl_info("STA mode is supported\n");
		feature |= WIFI_FEATURE_INFRA;
	}
	/*bit 2:Support for 5 GHz Band*/
	if (priv->fw_capa & SPRDWL_CAPA_5G) {
		wl_info("INFRA 5G is supported\n");
		feature |= WIFI_FEATURE_INFRA_5G;
	}
	/*bit3:HOTSPOT is a supplicant feature, enable it by default*/
		wl_info("HotSpot feature is supported\n");
		feature |= WIFI_FEATURE_HOTSPOT;
	/*bit 4:P2P*/
	if ((wiphy->interface_modes & BIT(NL80211_IFTYPE_P2P_CLIENT)) &&
		(wiphy->interface_modes & BIT(NL80211_IFTYPE_P2P_GO))) {
		wl_info("P2P is supported\n");
		feature |= WIFI_FEATURE_P2P;
	}
	/*bit 5:soft AP feature supported*/
	if (wiphy->interface_modes & BIT(NL80211_IFTYPE_AP)) {
		wl_info("Soft AP is supported\n");
		feature |= WIFI_FEATURE_SOFT_AP;
	}
	/*bit 6:GSCAN feature supported*/
	if (priv->fw_capa & SPRDWL_CAPA_GSCAN) {
		wl_info("GSCAN feature supported\n");
		feature |= WIFI_FEATURE_GSCAN;
	}
	/*bit 7:NAN feature supported*/
	if (priv->fw_capa & SPRDWL_CAPA_NAN) {
		wl_info("NAN is supported\n");
		feature |= WIFI_FEATURE_NAN;
	}
	/*bit 8: Device-to-device RTT */
	if (priv->fw_capa & SPRDWL_CAPA_D2D_RTT) {
		wl_info("D2D RTT supported\n");
		feature |= WIFI_FEATURE_D2D_RTT;
	}
	/*bit 9: Device-to-AP RTT*/
	if (priv->fw_capa &  SPRDWL_CAPA_D2AP_RTT) {
		wl_info("Device-to-AP RTT supported\n");
		feature |= WIFI_FEATURE_D2AP_RTT;
	}
	/*bit 10: Batched Scan (legacy)*/
	if (priv->fw_capa & SPRDWL_CAPA_BATCH_SCAN) {
		wl_info("Batched Scan supported\n");
		feature |= WIFI_FEATURE_BATCH_SCAN;
	}
	/*bit 11: PNO feature supported*/
	if (priv->fw_capa & SPRDWL_CAPA_PNO) {
		wl_info("PNO feature supported\n");
		feature |= WIFI_FEATURE_PNO;
	}
	/*bit 12:Support for two STAs*/
	if (priv->fw_capa & SPRDWL_CAPA_ADDITIONAL_STA) {
		wl_info("Two sta feature supported\n");
		feature |= WIFI_FEATURE_ADDITIONAL_STA;
	}
	/*bit 13:Tunnel directed link setup */
	if (priv->fw_capa & SPRDWL_CAPA_TDLS) {
		wl_info("TDLS feature supported\n");
		feature |= WIFI_FEATURE_TDLS;
	}
	/*bit 14:Support for TDLS off channel*/
	if (priv->fw_capa & SPRDWL_CAPA_TDLS_OFFCHANNEL) {
		wl_info("TDLS off channel supported\n");
		feature |= WIFI_FEATURE_TDLS_OFFCHANNEL;
	}
	/*bit 15:Enhanced power reporting*/
	if (priv->fw_capa & SPRDWL_CAPA_EPR) {
		wl_info("Enhanced power report supported\n");
		feature |= WIFI_FEATURE_EPR;
	}
	/*bit 16:Support for AP STA Concurrency*/
	if (priv->fw_capa & SPRDWL_CAPA_AP_STA) {
		wl_info("AP STA Concurrency supported\n");
		feature |= WIFI_FEATURE_AP_STA;
	}
	/*bit 17:Link layer stats collection*/
	if (priv->fw_capa & SPRDWL_CAPA_LL_STATS) {
		wl_info("LinkLayer status supported\n");
		feature |= WIFI_FEATURE_LINK_LAYER_STATS;
	}
	/*bit 18:WiFi Logger*/
	if (priv->fw_capa & SPRDWL_CAPA_WIFI_LOGGER) {
		wl_info("WiFi Logger supported\n");
		feature |= WIFI_FEATURE_LOGGER;
	}
	/*bit 19:WiFi PNO enhanced*/
	if (priv->fw_capa & SPRDWL_CAPA_EPNO) {
		wl_info("WIFI ENPO supported\n");
		feature |= WIFI_FEATURE_HAL_EPNO;
	}
	/*bit 20:RSSI monitor supported*/
	if (priv->fw_capa & SPRDWL_CAPA_RSSI_MONITOR) {
		wl_info("RSSI Monitor supported\n");
		feature |= WIFI_FEATURE_RSSI_MONITOR;
	}
	/*bit 21:WiFi mkeep_alive*/
	if (priv->fw_capa & SPRDWL_CAPA_MKEEP_ALIVE) {
		wl_info("WiFi mkeep alive supported\n");
		feature |= WIFI_FEATURE_MKEEP_ALIVE;
	}
	/*bit 22:ND offload configure*/
	if (priv->fw_capa & SPRDWL_CAPA_CONFIG_NDO) {
		wl_info("ND offload supported\n");
		feature |= WIFI_FEATURE_CONFIG_NDO;
	}
	/*bit 23:Capture Tx transmit power levels*/
	if (priv->fw_capa & SPRDWL_CAPA_TX_POWER) {
		wl_info("Tx power supported\n");
		feature |= WIFI_FEATURE_TX_TRANSMIT_POWER;
	}
	/*bit 24:Enable/Disable firmware roaming*/
	if (priv->fw_capa & SPRDWL_CAPA_11R_ROAM_OFFLOAD) {
		wl_info("ROAMING offload supported\n");
		feature |= WIFI_FEATURE_CONTROL_ROAMING;
	}
	/*bit 25:Support Probe IE white listing*/
	if (priv->fw_capa & SPRDWL_CAPA_IE_WHITELIST) {
		wl_info("Probe IE white listing supported\n");
		feature |= WIFI_FEATURE_IE_WHITELIST;
	}
	/*bit 26: Support MAC & Probe Sequence Number randomization*/
	if (priv->fw_capa & SPRDWL_CAPA_SCAN_RAND) {
		wl_info("RAND MAC SCAN supported\n");
		feature |= WIFI_FEATURE_SCAN_RAND;
	}

	wl_info("Supported Feature:0x%x\n", feature);

	if (nla_put_u32(reply, SPRDWL_VENDOR_ATTR_FEATURE_SET, feature)) {
		wiphy_err(wiphy, "%s put u32 error\n", __func__);
		goto out_put_fail;
	}

	ret = cfg80211_vendor_cmd_reply(reply);
	if (ret)
		wiphy_err(wiphy, "%s reply cmd error\n", __func__);
	return ret;

out_put_fail:
	kfree_skb(reply);
	return -EMSGSIZE;
}

/*set_mac_oui functon------ CMD ID:39*/
static int sprdwl_vendor_set_mac_oui(struct wiphy *wiphy,
						  struct wireless_dev *wdev,
					 const void *data, int len)
{
	struct nlattr *pos;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct v_MACADDR_t *rand_mac;
	int tlen = 0, ret = 0, rem_len, type;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	wiphy_info(wiphy, "%s\n", __func__);

	rand_mac = kmalloc(sizeof(*rand_mac), GFP_KERNEL);
	if (!rand_mac)
		return -ENOMEM;

	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);
		switch (type) {
		case WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI:
			memcpy(rand_mac, nla_data(pos), 3);
		break;

		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
				   type);
			ret = -EINVAL;
			goto out;
		break;
		}
	}

	tlen = sizeof(struct v_MACADDR_t);
	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  (void *)rand_mac,
				SPRDWL_WIFI_SUBCMD_SET_PNO_RANDOM_MAC_OUI,
				tlen, (u8 *)(&rsp), &rlen);

out:
	kfree(rand_mac);
	return ret;
}

/**
 * get concurrency matrix function---CMD ID:42
 * sprdwl_vendor_get_concurrency_matrix() - to retrieve concurrency matrix
 * @wiphy: pointer phy adapter
 * @wdev: pointer to wireless device structure
 * @data: pointer to data buffer
 * @data: length of data
 *
 * This routine will give concurrency matrix
 *
 * Return: int status code
 */

static int sprdwl_vendor_get_concurrency_matrix(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int len)
{
	uint32_t feature_set_matrix[CDS_MAX_FEATURE_SET] = {0};
	uint8_t i, feature_sets, max_feature_sets;
	struct nlattr *tb[SPRDWL_ATTR_CO_MATRIX_MAX + 1];
	struct sk_buff *reply_skb;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb, SPRDWL_ATTR_CO_MATRIX_MAX,
			  data, len, NULL, NULL)) {
#else
	if (nla_parse(tb, SPRDWL_ATTR_CO_MATRIX_MAX,
			data, len, NULL)) {
#endif
		wl_err("Invalid ATTR\n");
		return -EINVAL;
	}

	/* Parse and fetch max feature set */
	if (!tb[SPRDWL_ATTR_CO_MATRIX_CONFIG_PARAM_SET_SIZE_MAX]) {
		wl_err("Attr max feature set size failed\n");
		return -EINVAL;
	}
	max_feature_sets = nla_get_u32(
			tb[SPRDWL_ATTR_CO_MATRIX_CONFIG_PARAM_SET_SIZE_MAX]);

	wl_info("Max feature set size (%d)", max_feature_sets);

	/*Fill feature combination matrix*/
	feature_sets = 0;
	feature_set_matrix[feature_sets++] =
		WIFI_FEATURE_INFRA | WIFI_FEATURE_P2P;
	feature_set_matrix[feature_sets++] =
		WIFI_FEATURE_INFRA_5G | WIFI_FEATURE_P2P;
	feature_set_matrix[feature_sets++] =
		WIFI_FEATURE_INFRA | WIFI_FEATURE_GSCAN;
	feature_set_matrix[feature_sets++] =
		WIFI_FEATURE_INFRA_5G | WIFI_FEATURE_GSCAN;

	feature_sets = min(feature_sets, max_feature_sets);
	wl_info("Number of feature sets (%d)\n", feature_sets);

	wl_info("Feature set matrix:");
	for (i = 0; i < feature_sets; i++)
		wl_info("[%d] 0x%02X", i, feature_set_matrix[i]);

	reply_skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(u32) +
			sizeof(u32) * feature_sets);

	if (reply_skb) {
		if (nla_put_u32(reply_skb,
				SPRDWL_ATTR_CO_MATRIX_RESULTS_SET_SIZE,
					feature_sets) ||
			nla_put(reply_skb,
				SPRDWL_ATTR_CO_MATRIX_RESULTS_SET,
				sizeof(u32) * feature_sets,
				feature_set_matrix)) {
			wl_err("nla put failure\n");
			kfree_skb(reply_skb);
			return -EINVAL;
		}
		return cfg80211_vendor_cmd_reply(reply_skb);
	}
	wl_err("set matrix: buffer alloc failure\n");
	return -ENOMEM;
}


/*get support feature function---CMD ID:55*/
static int sprdwl_vendor_get_feature(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int len)
{
	return 0;
}

/*get wake up reason statistic*/
static int sprdwl_vendor_get_wake_state(struct wiphy *wiphy,
					struct wireless_dev *wdev,
					const void *data, int len)
{
	struct sk_buff *skb;
	uint32_t buf_len;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct wakeup_trace *wake_cnt;
	uint32_t rx_multi_cnt, ipv4_mc_cnt, ipv6_mc_cnt;
	uint32_t other_mc_cnt;

	wiphy_info(wiphy, "%s\n", __func__);
	wake_cnt = &priv->wakeup_tracer;
	buf_len = NLMSG_HDRLEN;
	buf_len += WLAN_GET_WAKE_STATS_MAX *
			 (NLMSG_HDRLEN + sizeof(uint32_t));
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, buf_len);

	if (!skb) {
				wl_err("cfg80211_vendor_cmd_alloc_reply_skb failed\n");
				return -ENOMEM;
	}

	ipv4_mc_cnt = wake_cnt->rx_data_dtl.rx_mc_dtl.ipv4_mc_cnt;
	ipv6_mc_cnt = wake_cnt->rx_data_dtl.rx_mc_dtl.ipv6_mc_cnt;
	other_mc_cnt = wake_cnt->rx_data_dtl.rx_mc_dtl.other_mc_cnt;

	/*rx multicast count contain IPV4,IPV6 and other mc pkt */
	rx_multi_cnt = ipv4_mc_cnt + ipv6_mc_cnt + other_mc_cnt;

	wl_info("total_cmd_event_wake:%d\n", wake_cnt->total_cmd_event_wake);
	wl_info("total_local_wake:%d\n", wake_cnt->total_local_wake);
	wl_info("total_rx_data_wake:%d\n", wake_cnt->total_rx_data_wake);
	wl_info("rx_unicast_cnt:%d\n", wake_cnt->rx_data_dtl.rx_unicast_cnt);
	wl_info("rx_multi_cnt:%d\n", rx_multi_cnt);
	wl_info("rx_brdcst_cnt:%d\n", wake_cnt->rx_data_dtl.rx_brdcst_cnt);
	wl_info("icmp_pkt_cnt:%d\n", wake_cnt->pkt_type_dtl.icmp_pkt_cnt);
	wl_info("icmp6_pkt_cnt:%d\n", wake_cnt->pkt_type_dtl.icmp6_pkt_cnt);
	wl_info("icmp6_ra_cnt:%d\n", wake_cnt->pkt_type_dtl.icmp6_ra_cnt);
	wl_info("icmp6_na_cnt:%d\n", wake_cnt->pkt_type_dtl.icmp6_na_cnt);
	wl_info("icmp6_ns_cnt:%d\n", wake_cnt->pkt_type_dtl.icmp6_ns_cnt);
	wl_info("ipv4_mc_cnt:%d\n", ipv4_mc_cnt);
	wl_info("ipv6_mc_cnt:%d\n", ipv6_mc_cnt);
	wl_info("other_mc_cnt:%d\n", other_mc_cnt);

	if (nla_put_u32(skb, WLAN_ATTR_TOTAL_CMD_EVENT_WAKE,
					wake_cnt->total_cmd_event_wake) ||
		nla_put_u32(skb, WLAN_ATTR_CMD_EVENT_WAKE_CNT_PTR, 0) ||
		nla_put_u32(skb, WLAN_ATTR_CMD_EVENT_WAKE_CNT_SZ, 0) ||
		nla_put_u32(skb, WLAN_ATTR_TOTAL_DRIVER_FW_LOCAL_WAKE,
					wake_cnt->total_local_wake) ||
		nla_put_u32(skb, WLAN_ATTR_DRIVER_FW_LOCAL_WAKE_CNT_PTR, 0) ||
		nla_put_u32(skb, WLAN_ATTR_DRIVER_FW_LOCAL_WAKE_CNT_SZ, 0) ||
		nla_put_u32(skb, WLAN_ATTR_TOTAL_RX_DATA_WAKE,
					wake_cnt->total_rx_data_wake) ||
		nla_put_u32(skb, WLAN_ATTR_RX_UNICAST_CNT,
					wake_cnt->rx_data_dtl.rx_unicast_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_RX_MULTICAST_CNT,
					rx_multi_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_RX_BROADCAST_CNT,
					wake_cnt->rx_data_dtl.rx_brdcst_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP_PKT,
					wake_cnt->pkt_type_dtl.icmp_pkt_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP6_PKT,
					wake_cnt->pkt_type_dtl.icmp6_pkt_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP6_RA,
					wake_cnt->pkt_type_dtl.icmp6_ra_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP6_NA,
					wake_cnt->pkt_type_dtl.icmp6_na_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP6_NS,
					wake_cnt->pkt_type_dtl.icmp6_ns_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP4_RX_MULTICAST_CNT,
					ipv4_mc_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_ICMP6_RX_MULTICAST_CNT,
					ipv6_mc_cnt) ||
		nla_put_u32(skb, WLAN_ATTR_OTHER_RX_MULTICAST_CNT,
					other_mc_cnt)) {
		wl_err("nla put failure\n");
		goto nla_put_failure;
	}
	cfg80211_vendor_cmd_reply(skb);

	return WIFI_SUCCESS;

nla_put_failure:
	kfree_skb(skb);
	return -EINVAL;
}

static int sprdwl_vendor_enable_nd_offload(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data, int len)
{
	wiphy_info(wiphy, "%s\n", __func__);

	return WIFI_SUCCESS;
}

static int sprdwl_vendor_start_logging(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data, int len)
{
	wiphy_info(wiphy, "%s\n", __func__);

	return WIFI_SUCCESS;
}

static int sprdwl_vendor_get_ring_data(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data, int len)
{
	wiphy_info(wiphy, "%s\n", __func__);

	return WIFI_SUCCESS;
}

static int sprdwl_vendor_memory_dump(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int len)
{
	wiphy_info(wiphy, "%s\n", __func__);

	return -EOPNOTSUPP;
}

/*CMD ID:61*/
static const struct nla_policy sprdwl_get_wifi_info_policy[
		SPRDWL_ATTR_WIFI_INFO_GET_MAX + 1] = {
		[SPRDWL_ATTR_WIFI_INFO_DRIVER_VERSION] = {.type = NLA_U8},
		[SPRDWL_ATTR_WIFI_INFO_FIRMWARE_VERSION] = {.type = NLA_U8},
};

static int sprdwl_vendor_get_driver_info(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int len)
{
	int ret, payload = 0;
	struct sk_buff *reply;
	uint8_t attr;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct nlattr *tb_vendor[SPRDWL_ATTR_WIFI_INFO_GET_MAX + 1];
	char version[32];

	wl_info("%s enter\n", __func__);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb_vendor, SPRDWL_ATTR_WIFI_INFO_GET_MAX, data,
			  len, sprdwl_get_wifi_info_policy, NULL)) {
#else
	if (nla_parse(tb_vendor, SPRDWL_ATTR_WIFI_INFO_GET_MAX, data,
			len, sprdwl_get_wifi_info_policy)) {
#endif
		wl_err("WIFI_INFO_GET CMD parsing failed\n");
		return -EINVAL;
	}

	if (tb_vendor[SPRDWL_ATTR_WIFI_INFO_DRIVER_VERSION]) {
		wl_info("Recived req for Drv version\n");
		memcpy(version, &priv->wl_ver, sizeof(version));
		attr = SPRDWL_ATTR_WIFI_INFO_DRIVER_VERSION;
		payload = sizeof(priv->wl_ver);
	} else if (tb_vendor[SPRDWL_ATTR_WIFI_INFO_FIRMWARE_VERSION]) {
		wl_info("Recived req for FW version\n");
		snprintf(version, sizeof(version), "%d", priv->fw_ver);
		wl_info("fw version:%s\n", version);
		attr = SPRDWL_ATTR_WIFI_INFO_FIRMWARE_VERSION;
		payload = strlen(version);
	}

	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);

	if (!reply)
		return -ENOMEM;

	if (nla_put(reply, SPRDWL_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION,
			payload, version)) {
		wiphy_err(wiphy, "%s put version error\n", __func__);
		goto out_put_fail;
	}

	ret = cfg80211_vendor_cmd_reply(reply);
	if (ret)
		wiphy_err(wiphy, "%s reply cmd error\n", __func__);
	return ret;

out_put_fail:
	kfree_skb(reply);
	return -EMSGSIZE;
}

/*Roaming function---CMD ID:64*/
static int sprdwl_vendor_set_roam_params(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
						const void *data, int len)
{
	uint32_t cmd_type, req_id;
	struct roam_white_list_params white_params;
	struct roam_black_list_params black_params;
	struct nlattr *curr_attr;
	struct nlattr *tb[SPRDWL_ROAM_MAX + 1];
	struct nlattr *tb2[SPRDWL_ROAM_MAX + 1];
	int rem, i;
	int white_limit = 0, black_limit = 0;
	int fw_max_whitelist = 0, fw_max_blacklist = 0;
	uint32_t buf_len = 0;
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	int ret = 0;

	memset(&white_params, 0, sizeof(white_params));
	memset(&black_params, 0, sizeof(black_params));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb, SPRDWL_ROAM_MAX, data, len, NULL, NULL)) {
#else
	if (nla_parse(tb, SPRDWL_ROAM_MAX, data, len, NULL)) {
#endif
		wl_err("Invalid ATTR\n");
		return -EINVAL;
	}
	/* Parse and fetch Command Type*/
	if (!tb[SPRDWL_ROAM_SUBCMD]) {
		wl_err("roam cmd type failed\n");
		goto fail;
	}

	cmd_type = nla_get_u32(tb[SPRDWL_ROAM_SUBCMD]);
	if (!tb[SPRDWL_ROAM_REQ_ID]) {
		wl_err("%s:attr request id failed\n", __func__);
		goto fail;
	}
	req_id = nla_get_u32(tb[SPRDWL_ROAM_REQ_ID]);
	wl_info("Req ID:%d, Cmd Type:%d", req_id, cmd_type);
	switch (cmd_type) {
	case SPRDWL_ATTR_ROAM_SUBCMD_SSID_WHITE_LIST:
		if (!tb[SPRDWL_ROAM_WHITE_LIST_SSID_LIST])
			break;
		i = 0;
		nla_for_each_nested(curr_attr,
					tb[SPRDWL_ROAM_WHITE_LIST_SSID_LIST],
					rem) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
			if (nla_parse(tb2, SPRDWL_ATTR_ROAM_SUBCMD_MAX,
					  nla_data(curr_attr),
					nla_len(curr_attr),
					NULL, NULL)) {
#else
			if (nla_parse(tb2, SPRDWL_ATTR_ROAM_SUBCMD_MAX,
					nla_data(curr_attr),
					nla_len(curr_attr),
					NULL)) {
#endif
				wl_err("nla parse failed\n");
				goto fail;
			}
			/* Parse and Fetch allowed SSID list*/
			if (!tb2[SPRDWL_ROAM_WHITE_LIST_SSID]) {
				wl_err("attr allowed ssid failed\n");
				goto fail;
			}
			buf_len = nla_len(tb2[SPRDWL_ROAM_WHITE_LIST_SSID]);
			/* Upper Layers include a null termination character.
			* Check for the actual permissible length of SSID and
			* also ensure not to copy the NULL termination
			* character to the driver buffer.
			*/
			fw_max_whitelist = priv->roam_capa.max_whitelist_size;
			white_limit = min(fw_max_whitelist, MAX_WHITE_SSID);

			if (buf_len && (i < white_limit) &&
				((buf_len - 1) <= IEEE80211_MAX_SSID_LEN)) {
				nla_memcpy(
					white_params.white_list[i].ssid_str,
					tb2[SPRDWL_ROAM_WHITE_LIST_SSID],
					buf_len - 1);
				white_params.white_list[i].length =
					buf_len - 1;
				wl_info("SSID[%d]:%.*s, length=%d\n", i,
					white_params.white_list[i].length,
					white_params.white_list[i].ssid_str,
					white_params.white_list[i].length);
				i++;
			} else {
				wl_err("Invalid buffer length\n");
			}
		}
		white_params.num_white_ssid = i;
		wl_info("Num of white list:%d", i);
		/*send white list with roam params by roaming CMD*/
		ret = sprdwl_set_roam_offload(priv, vif->ctx_id,
					SPRDWL_ROAM_SET_WHITE_LIST,
				(u8 *)&white_params,
				(i * sizeof(struct ssid_t) + 1));
		break;
	case SPRDWL_ATTR_ROAM_SUBCMD_SET_BLACKLIST_BSSID:
		/*Parse and fetch number of blacklist BSSID*/
		if (!tb[SPRDWL_ROAM_SET_BSSID_PARAMS_NUM_BSSID]) {
			wl_err("attr num of blacklist bssid failed\n");
			goto fail;
		}
		black_params.num_black_bssid = nla_get_u32(
			tb[SPRDWL_ROAM_SET_BSSID_PARAMS_NUM_BSSID]);
		wl_info("Num of black BSSID:%d\n",
			black_params.num_black_bssid);

		if (!tb[SPRDWL_ROAM_SET_BSSID_PARAMS])
			break;

		fw_max_blacklist = priv->roam_capa.max_blacklist_size;
		black_limit = min(fw_max_blacklist, MAX_BLACK_BSSID);

		if (black_params.num_black_bssid > black_limit) {
			wl_err("black size exceed the limit:%d\n", black_limit);
			break;
		}
		i = 0;
		nla_for_each_nested(curr_attr,
					tb[SPRDWL_ROAM_SET_BSSID_PARAMS], rem) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
			if (nla_parse(tb2, SPRDWL_ROAM_MAX,
					  nla_data(curr_attr), nla_len(curr_attr),
					NULL, NULL)) {
#else
			if (nla_parse(tb2, SPRDWL_ROAM_MAX,
					nla_data(curr_attr), nla_len(curr_attr),
					NULL)) {
#endif
					wl_err("nla parse failed\n");
					goto fail;
			}
		/* Parse and fetch MAC address */
			if (!tb2[SPRDWL_ROAM_SET_BSSID_PARAMS_BSSID]) {
				wl_err("attr blacklist addr failed\n");
				goto fail;
			}
			nla_memcpy(black_params.black_list[i].MAC_addr,
				   tb2[SPRDWL_ROAM_SET_BSSID_PARAMS_BSSID],
					sizeof(struct bssid_t));
			wl_info("black list mac addr:%pM\n",
				black_params.black_list[i].MAC_addr);
			i++;
		}
		black_params.num_black_bssid = i;
		/*send black list with roam_params CMD*/
		ret = sprdwl_set_roam_offload(priv, vif->ctx_id,
					SPRDWL_ROAM_SET_BLACK_LIST,
				(u8 *)&black_params,
				(i * sizeof(struct bssid_t) + 1));
		break;
	default:
		break;
	}
	return ret;
fail:
	return -EINVAL;
}

/*set_ssid_hotlist function---CMD ID:65*/
static int sprdwl_vendor_set_ssid_hotlist(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
					 const void *data, int len)
{
	int i, ret = 0, tlen;
	int type, request_id;
	int rem_len, rem_outer_len, rem_inner_len;
	struct nlattr *pos, *outer_iter, *inner_iter;
	struct wifi_ssid_hotlist_params *ssid_hotlist_params;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	ssid_hotlist_params =
		kmalloc(sizeof(*ssid_hotlist_params), GFP_KERNEL);

	if (!ssid_hotlist_params)
		return -ENOMEM;

	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);

		switch (type) {
		case GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID:
			request_id = nla_get_s32(pos);
		break;

		case GSCAN_ATTR_GSCAN_SSID_HOTLIST_PARAMS_LOST_SSID_SAMPLE_SIZE:
			ssid_hotlist_params->lost_ssid_sample_size
				= nla_get_s32(pos);
		break;

		case GSCAN_ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID:
			ssid_hotlist_params->num_ssid = nla_get_s32(pos);
		break;

		case GSCAN_ATTR_GSCAN_SSID_THR_PARAM:
			i = 0;
			nla_for_each_nested(outer_iter, pos, rem_outer_len) {
				nla_for_each_nested(inner_iter, outer_iter,
						rem_inner_len) {
					type = nla_type(inner_iter);
				switch (type) {
				case GSCAN_ATTR_GSCAN_SSID_THR_PARAM_SSID:
				memcpy(
				ssid_hotlist_params->ssid[i].ssid,
				nla_data(inner_iter),
				IEEE80211_MAX_SSID_LEN * sizeof(unsigned char));
				break;

				case GSCAN_ATTR_GSCAN_SSID_THR_PARAM_RSSI_LOW:
					ssid_hotlist_params->ssid[i].low
						= nla_get_s32(inner_iter);
				break;

				case GSCAN_ATTR_GSCAN_SSID_THR_PARAM_RSSI_HIGH:
					ssid_hotlist_params->ssid[i].high
						= nla_get_s32(inner_iter);
				break;
				default:
					wl_ndev_log(L_ERR, vif->ndev,
						"networks nla type 0x%x not support\n",
						type);
						ret = -EINVAL;
				break;
				}
			}

			if (ret < 0)
				break;

			i++;
			if (i >= MAX_HOTLIST_APS)
				break;
		}
		break;

		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
			type);
			ret = -EINVAL;
		break;
		}

	if (ret < 0)
		break;
	}

	wl_ndev_log(L_INFO, vif->ndev, "parse bssid hotlist %s\n",
			!ret ? "success" : "failture");

	tlen = sizeof(struct wifi_ssid_hotlist_params);
	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  (void *)ssid_hotlist_params,
			SPRDWL_GSCAN_SUBCMD_SET_SSID_HOTLIST,
			tlen, (u8 *)(&rsp), &rlen);

	kfree(ssid_hotlist_params);
	return ret;
}

/*reset_ssid_hotlist function---CMD ID:66*/
static int sprdwl_vendor_reset_ssid_hotlist(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
					 const void *data, int len)
{
	int flush = 1;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	wl_ndev_log(L_INFO, vif->ndev, "%s %d\n", __func__, flush);

	return sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					(void *)(&flush),
					SPRDWL_GSCAN_SUBCMD_RESET_SSID_HOTLIST,
					sizeof(int), (u8 *)(&rsp), &rlen);

}

/*set_passpoint_list functon------ CMD ID:70*/
static int sprdwl_vendor_set_passpoint_list(struct wiphy *wiphy,
						struct wireless_dev *wdev,
					 const void *data, int len)
{
	struct nlattr *tb[GSCAN_MAX + 1];
	struct nlattr *tb2[GSCAN_MAX + 1];
	struct nlattr *HS_list;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct wifi_passpoint_network *HS_list_params;
	int i = 0, rem, flush, ret = 0, tlen, hs_num;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb, GSCAN_MAX, data, len,
			  wlan_gscan_config_policy, NULL)) {
#else
	if (nla_parse(tb, GSCAN_MAX, data, len,
			wlan_gscan_config_policy)) {
#endif
		wl_ndev_log(L_INFO, vif->ndev,
				"%s :Fail to parse attribute\n",
			__func__);
		return -EINVAL;
	}

	HS_list_params = kmalloc(sizeof(*HS_list_params), GFP_KERNEL);
	if (!HS_list_params)
		return -ENOMEM;

	/* Parse and fetch */
	if (!tb[GSCAN_ANQPO_LIST_FLUSH]) {
		wl_ndev_log(L_INFO, vif->ndev,
				"%s :Fail to parse GSCAN_ANQPO_LIST_FLUSH\n",
			__func__);
		ret = -EINVAL;
		goto out;
	}

	flush = nla_get_u32(tb[GSCAN_ANQPO_LIST_FLUSH]);

	/* Parse and fetch */
	if (!tb[GSCAN_ANQPO_HS_LIST_SIZE]) {
		if (flush == 1)	{
			ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
						  (void *)(&flush),
					SPRDWL_GSCAN_SUBCMD_RESET_ANQPO_CONFIG,
					sizeof(int), (u8 *)(&rsp), &rlen);
		} else{
			ret = -EINVAL;
		}
		goto out;
	}

	hs_num = nla_get_u32(tb[GSCAN_ANQPO_HS_LIST_SIZE]);

	nla_for_each_nested(HS_list,
				tb[GSCAN_ANQPO_HS_LIST], rem) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
		if (nla_parse(tb2, GSCAN_MAX,
				  nla_data(HS_list), nla_len(HS_list),
					NULL, NULL)) {
#else
		if (nla_parse(tb2, GSCAN_MAX,
				nla_data(HS_list), nla_len(HS_list),
				NULL)) {
#endif
			wl_ndev_log(L_INFO, vif->ndev,
					"%s :Fail to parse tb2\n",
				__func__);
			ret = -EINVAL;
				 goto out;
		}

		if (!tb2[GSCAN_ANQPO_HS_NETWORK_ID]) {
			wl_ndev_log(L_INFO, vif->ndev,
					"%s :Fail to parse GSCAN_ATTR_ANQPO_HS_NETWORK_ID\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}

		HS_list_params->id =
			nla_get_u32(tb[GSCAN_ANQPO_HS_NETWORK_ID]);

		if (!tb2[GSCAN_ANQPO_HS_NAI_REALM]) {
			wl_ndev_log(L_INFO, vif->ndev,
					"%s :Fail to parse GSCAN_ATTR_ANQPO_HS_NAI_REALM\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}
		memcpy(HS_list_params->realm, nla_data(
			tb2[GSCAN_ANQPO_HS_NAI_REALM]),
			256);

		if (!tb2[GSCAN_ANQPO_HS_ROAM_CONSORTIUM_ID]) {
			wl_ndev_log(L_INFO, vif->ndev,
					"%s :Fail to parse GSCAN_ATTR_ANQPO_HS_ROAM_CONSORTIUM_ID\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}

		memcpy(HS_list_params->roaming_ids, nla_data(
			tb2[GSCAN_ANQPO_HS_ROAM_CONSORTIUM_ID]),
			128);

		if (!tb2[GSCAN_ANQPO_HS_PLMN]) {
			wl_ndev_log(L_INFO, vif->ndev,
					"%s :Fail to parse GSCAN_ATTR_ANQPO_HS_PLMN\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}

		memcpy(HS_list_params->plmn, nla_data(
			tb2[GSCAN_ANQPO_HS_PLMN]),
			3);
		i++;
	}

	tlen = sizeof(struct wifi_passpoint_network);
	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  (void *)HS_list_params,
				  SPRDWL_GSCAN_SUBCMD_ANQPO_CONFIG,
					tlen, (u8 *)(&rsp), &rlen);

out:
	kfree(HS_list_params);
	return ret;
}

/*reset_passpoint_list functon------ CMD ID:71*/
static int sprdwl_vendor_reset_passpoint_list(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
					 const void *data, int len)
{
	int flush = 1;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	wl_ndev_log(L_INFO, vif->ndev, "%s %d\n", __func__, flush);

	return sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					(void *)(&flush),
					SPRDWL_GSCAN_SUBCMD_RESET_ANQPO_CONFIG,
					sizeof(int), (u8 *)(&rsp), &rlen);
}

/*RSSI monitor function---CMD ID:80*/

static int send_rssi_cmd(struct sprdwl_priv *priv, u8 vif_ctx_id,
			 const void *buf, u8 len)
{
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_RSSI_MONITOR);
	if (!msg)
		return -ENOMEM;
	memcpy(msg->data, buf, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, 0, 0);
}

#define MONITOR_MAX      SPRDWL_ATTR_RSSI_MONITORING_MAX
#define REQUEST_ID       SPRDWL_ATTR_RSSI_MONITORING_REQUEST_ID
#define MONITOR_CONTROL  SPRDWL_ATTR_RSSI_MONITORING_CONTROL
#define MIN_RSSI         SPRDWL_ATTR_RSSI_MONITORING_MIN_RSSI
#define MAX_RSSI         SPRDWL_ATTR_RSSI_MONITORING_MAX_RSSI
static int sprdwl_vendor_monitor_rssi(struct wiphy *wiphy,
					  struct wireless_dev *wdev,
		const void *data, int len)
{
	struct nlattr *tb[MONITOR_MAX + 1];
	uint32_t control;
	struct rssi_monitor_req req;
	static const struct nla_policy policy[MONITOR_MAX + 1] = {
		[REQUEST_ID] = { .type = NLA_U32 },
		[MONITOR_CONTROL] = { .type = NLA_U32 },
		[MIN_RSSI] = { .type = NLA_S8 },
		[MAX_RSSI] = { .type = NLA_S8 },
	};
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	struct sprdwl_priv *priv = wiphy_priv(wiphy);

	/*if wifi not connected,return	*/
	if (SPRDWL_CONNECTED != vif->sm_state) {
		wl_err("Wifi not connected!\n");
		return -ENOTSUPP;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb, MONITOR_MAX, data, len, policy, NULL)) {
#else
	if (nla_parse(tb, MONITOR_MAX, data, len, policy)) {
#endif
		wl_err("Invalid ATTR\n");
		return -EINVAL;
	}

	if (!tb[REQUEST_ID]) {
		wl_err("attr request id failed\n");
		return -EINVAL;
	}

	if (!tb[MONITOR_CONTROL]) {
		wl_err("attr control failed\n");
		return -EINVAL;
	}

	req.request_id = nla_get_u32(tb[REQUEST_ID]);
	control = nla_get_u32(tb[MONITOR_CONTROL]);

	if (control == WLAN_RSSI_MONITORING_START) {
		req.control = true;
		if (!tb[MIN_RSSI]) {
			wl_err("get min rssi fail\n");
			return -EINVAL;
		}

		if (!tb[MAX_RSSI]) {
			wl_err("get max rssi fail\n");
			return -EINVAL;
		}

		req.min_rssi = nla_get_s8(tb[MIN_RSSI]);
		req.max_rssi = nla_get_s8(tb[MAX_RSSI]);

		if (!(req.min_rssi < req.max_rssi)) {
			wl_err("min rssi %d must be less than max_rssi:%d\n",
				   req.min_rssi, req.max_rssi);
			return -EINVAL;
		}
		wl_info("min_rssi:%d max_rssi:%d\n",
			req.min_rssi, req.max_rssi);
	} else if (control == WLAN_RSSI_MONITORING_STOP) {
		req.control = false;
		wl_info("stop rssi monitor!\n");
	} else {
		wl_err("Invalid control cmd:%d\n", control);
		return -EINVAL;
	}
	wl_info("Request id:%u,control:%d", req.request_id, req.control);

	/*send rssi monitor cmd*/
	send_rssi_cmd(priv, vif->ctx_id, &req, sizeof(req));

	return 0;
}

void sprdwl_event_rssi_monitor(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *skb;
	struct rssi_monitor_event *mon = (struct rssi_monitor_event *)data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	skb = cfg80211_vendor_event_alloc(wiphy, &vif->wdev,
#else
	skb = cfg80211_vendor_event_alloc(wiphy,
#endif
					  EVENT_BUF_SIZE + NLMSG_HDRLEN,
			SPRDWL_VENDOR_SUBCMD_MONITOR_RSSI_INDEX,
			GFP_KERNEL);
	if (!skb) {
		wl_err("%s vendor alloc event failed\n", __func__);
		return;
	}
	wl_info("Req Id:%u,current RSSI:%d, Current BSSID:%pM\n",
		mon->request_id, mon->curr_rssi, mon->curr_bssid);
	if (nla_put_u32(skb, SPRDWL_ATTR_RSSI_MONITORING_REQUEST_ID,
			mon->request_id) ||
		nla_put(skb, SPRDWL_ATTR_RSSI_MONITORING_CUR_BSSID,
			sizeof(mon->curr_bssid), mon->curr_bssid) ||
		nla_put_s8(skb, SPRDWL_ATTR_RSSI_MONITORING_CUR_RSSI,
			   mon->curr_rssi)) {
		wl_err("nla data put fail\n");
		goto fail;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return;

fail:
	kfree_skb(skb);
}

static int sprdwl_vendor_get_logger_feature(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int len)
{
	int ret;
	struct sk_buff *reply;
	int feature, payload;

	payload = sizeof(feature);
	feature = 0;
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);

	if (!reply)
		return -ENOMEM;

	feature |= WIFI_LOGGER_CONNECT_EVENT_SUPPORTED;

	/*vts will test wake reason state function*/
	feature |= WIFI_LOGGER_WAKE_LOCK_SUPPORTED;

	if (nla_put_u32(reply, SPRDWL_VENDOR_ATTR_FEATURE_SET, feature)) {
		wiphy_err(wiphy, "put skb u32 failed\n");
		goto out_put_fail;
	}

	ret = cfg80211_vendor_cmd_reply(reply);
	if (ret)
		wiphy_err(wiphy, "reply cmd error\n");
	return ret;

out_put_fail:
	kfree_skb(reply);
	return -EMSGSIZE;
}

static int sprdwl_flush_epno_list(struct sprdwl_vif *vif)
{
	int ret;
	char flush_data = 1;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
				  (void *)&flush_data,
				  SPRDWL_GSCAN_SUBCMD_SET_EPNO_FLUSH,
				  sizeof(flush_data),
				  (u8 *)(&rsp), &rlen);
	wl_debug("flush epno list, ret = %d\n", ret);
	return ret;
}

static int sprdwl_vendor_set_epno_list(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   const void *data, int len)
{
	int i, ret = 0;
	int type;
	int rem_len, rem_outer_len, rem_inner_len;
	struct nlattr *pos, *outer_iter, *inner_iter;
	struct wifi_epno_network *epno_network;
	struct wifi_epno_params epno_params;
	struct sprdwl_cmd_gscan_rsp_header rsp;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	u16 rlen = sizeof(struct sprdwl_cmd_gscan_rsp_header);

	nla_for_each_attr(pos, (void *)data, len, rem_len) {
		type = nla_type(pos);
		switch (type) {
		case GSCAN_RESULTS_REQUEST_ID:
			epno_params.request_id = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_MIN5GHZ_RSSI:
			epno_params.min5ghz_rssi = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_MIN24GHZ_RSSI:
			epno_params.min24ghz_rssi = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_INITIAL_SCORE_MAX:
			epno_params.initial_score_max = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_CURRENT_CONNECTION_BONUS:
			epno_params.current_connection_bonus = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_SAME_NETWORK_BONUS:
			epno_params.same_network_bonus = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_SECURE_BONUS:
			epno_params.secure_bonus = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_BAND5GHZ_BONUS:
			epno_params.band5ghz_bonus = nla_get_u32(pos);
			break;

		case SPRDWL_EPNO_PARAM_NUM_NETWORKS:
			epno_params.num_networks = nla_get_u32(pos);
			if (epno_params.num_networks == 0)
				return sprdwl_flush_epno_list(vif);

			break;

		case SPRDWL_EPNO_PARAM_NETWORKS_LIST:
			i = 0;
			nla_for_each_nested(outer_iter, pos, rem_outer_len) {
				epno_network = &epno_params.networks[i];
				nla_for_each_nested(inner_iter, outer_iter,
							rem_inner_len) {
					type = nla_type(inner_iter);
					switch (type) {
					case SPRDWL_EPNO_PARAM_NETWORK_SSID:
						memcpy(epno_network->ssid,
							   nla_data(inner_iter),
							   IEEE80211_MAX_SSID_LEN);
						break;

					case SPRDWL_EPNO_PARAM_NETWORK_FLAGS:
						epno_network->flags =
							nla_get_u8(inner_iter);
						break;

					case SPRDWL_EPNO_PARAM_NETWORK_AUTH_BIT:
						epno_network->auth_bit_field =
							nla_get_u8(inner_iter);
						break;

					default:
						wl_ndev_log(L_ERR, vif->ndev,
							   "networks nla type 0x%x not support\n",
							   type);
						ret = -EINVAL;
						break;
					}
				}

				if (ret < 0)
					break;

				i++;
				if (i >= MAX_EPNO_NETWORKS)
					break;
			}
			break;


		default:
			wl_ndev_log(L_ERR, vif->ndev, "nla type 0x%x not support\n",
				   type);
			ret = -EINVAL;
			break;
		}

		if (ret < 0)
			break;
	}

	epno_params.boot_time = jiffies;

	wl_ndev_log(L_INFO, vif->ndev, "parse epno list %s\n",
			!ret ? "success" : "failture");
	if (!ret)
		ret = sprdwl_gscan_subcmd(vif->priv, vif->ctx_id,
					  (void *)&epno_params,
					  SPRDWL_GSCAN_SUBCMD_SET_EPNO_SSID,
					  sizeof(epno_params), (u8 *)(&rsp),
					  &rlen);

	return ret;
}

int sprdwl_hotlist_change_event(struct sprdwl_vif *vif, u32 report_event)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *reply;
	int payload, rlen, event_idx;
	int ret = 0, j, moredata = 0;
	struct nlattr *cached_list;


	rlen = priv->hotlist_res->num_results
			* sizeof(struct sprdwl_gscan_result) + sizeof(u32);
	payload = rlen + 0x100;

	if (report_event & REPORT_EVENTS_HOTLIST_RESULTS_FOUND) {
		event_idx = NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND_INDEX;
	} else if (report_event & REPORT_EVENTS_HOTLIST_RESULTS_LOST) {
		event_idx = NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_LOST_INDEX;
	} else {
		/* unknown event, should not happened*/
		event_idx = SPRD_RESERVED1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	reply = cfg80211_vendor_event_alloc(wiphy, &vif->wdev, payload,
#else
	reply = cfg80211_vendor_event_alloc(wiphy, payload,
#endif
						event_idx, GFP_KERNEL);

	if (!reply)
		return -ENOMEM;

	if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
		priv->hotlist_res->req_id) ||
		nla_put_u32(reply,
		GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,
		priv->hotlist_res->num_results)) {
		wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
		goto out_put_fail;
	}

	if (nla_put_u8(reply,
		GSCAN_RESULTS_SCAN_RESULT_MORE_DATA,
		moredata)) {
		wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
		goto out_put_fail;
	}

	if (priv->hotlist_res->num_results == 0)
		goto out_put_fail;


	cached_list = nla_nest_start(reply, GSCAN_RESULTS_LIST);
	if (!cached_list)
		goto out_put_fail;

	for (j = 0; j < priv->hotlist_res->num_results; j++) {
		struct nlattr *ap;
		struct sprdwl_gscan_hotlist_results *p = priv->hotlist_res;

		wl_info("[index=%d] Timestamp(%lu) Ssid (%s) Bssid: %pM Channel (%d) Rssi (%d) RTT (%u) RTT_SD (%u)\n",
			j,
			p->results[j].ts,
			p->results[j].ssid,
			p->results[j].bssid,
			p->results[j].channel,
			p->results[j].rssi,
			p->results[j].rtt,
			p->results[j].rtt_sd);

		ap = nla_nest_start(reply, j + 1);
		if (!ap) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		if (nla_put_u64_64bit(reply,
			GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
			p->results[j].ts, 0)) {
#else
		if (nla_put_u64(reply,
			GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
			p->results[j].ts)) {
#endif
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put(reply,
			GSCAN_RESULTS_SCAN_RESULT_SSID,
			sizeof(p->results[j].ssid),
			p->results[j].ssid)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put(reply,
			GSCAN_RESULTS_SCAN_RESULT_BSSID,
			sizeof(p->results[j].bssid),
			p->results[j].bssid)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put_u32(reply,
			GSCAN_RESULTS_SCAN_RESULT_CHANNEL,
			p->results[j].channel)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put_s32(reply,
			GSCAN_RESULTS_SCAN_RESULT_RSSI,
			p->results[j].rssi)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put_u32(reply,
			GSCAN_RESULTS_SCAN_RESULT_RTT,
			p->results[j].rtt)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put_u32(reply,
			GSCAN_RESULTS_SCAN_RESULT_RTT_SD,
			p->results[j].rtt_sd)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
	nla_nest_end(reply, ap);
	}
	nla_nest_end(reply, cached_list);

	cfg80211_vendor_event(reply, GFP_KERNEL);
	/*reset results buffer when finished event report*/
	if (vif->priv->hotlist_res) {
		memset(vif->priv->hotlist_res, 0x0,
		sizeof(struct sprdwl_gscan_hotlist_results));
	}

	return ret;

out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

int sprdwl_significant_change_event(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct sk_buff *reply;
	int payload, rlen;
	int ret = 0, j;
	struct nlattr *cached_list;

	rlen = sizeof(struct sprdwl_significant_change_result);
	payload = rlen + 0x100;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	reply = cfg80211_vendor_event_alloc(wiphy, &vif->wdev,
#else
	reply = cfg80211_vendor_event_alloc(wiphy,
#endif
				payload,
				NL80211_VENDOR_SUBCMD_SIGNIFICANT_CHANGE_INDEX,
				GFP_KERNEL);

	if (!reply)
		return -ENOMEM;

	if (nla_put_u32(reply, GSCAN_RESULTS_REQUEST_ID,
		priv->significant_res->req_id) ||
		nla_put_u32(reply,
		GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,
		priv->significant_res->num_results)) {
		wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
		goto out_put_fail;
	}

	cached_list = nla_nest_start(reply, GSCAN_RESULTS_LIST);
	if (!cached_list)
		goto out_put_fail;

	for (j = 0; j < priv->significant_res->num_results; j++) {
		struct nlattr *ap;
		struct significant_change_info *p =
				priv->significant_res->results+j;

		ap = nla_nest_start(reply, j + 1);
		if (!ap) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

		if (nla_put(reply,
			GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID,
			sizeof(p->bssid),
			p->bssid)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

		if (nla_put_u32(reply,
			GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL,
			p->channel)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

		if (nla_put_u32(reply,
			GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI,
			p->num_rssi)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}
		if (nla_put(reply,
			GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST,
			sizeof(s8) * 3, /*here, we fixed rssi list as 3*/
			p->rssi)) {
			wl_ndev_log(L_ERR, vif->ndev, "failed to put!\n");
			goto out_put_fail;
		}

	nla_nest_end(reply, ap);
	}
	nla_nest_end(reply, cached_list);

	cfg80211_vendor_event(reply, GFP_KERNEL);

	/*reset results buffer when finished event report*/
	if (vif->priv->significant_res) {
		memset(vif->priv->significant_res, 0x0,
		sizeof(struct sprdwl_significant_change_result));
	}

	return ret;

out_put_fail:
	kfree_skb(reply);
	WARN_ON(1);
	return -EMSGSIZE;
}

/*set SAR limits function------CMD ID:146*/
static int sprdwl_vendor_set_sar_limits(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		const void *data, int len)
{
	/*to pass vts*/
	return -EOPNOTSUPP;
#if	0
	int ret = 0;
	uint32_t bdf = 0xff;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	struct nlattr *tb[WLAN_ATTR_SAR_LIMITS_MAX + 1];

	wl_info("%s enter:\n", __func__);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if (nla_parse(tb, WLAN_ATTR_SAR_LIMITS_MAX, data, len, NULL, NULL)) {
#else
	if (nla_parse(tb, WLAN_ATTR_SAR_LIMITS_MAX, data, len, NULL)) {
#endif
		wl_err("Invalid ATTR\n");
		return -EINVAL;
	}

	if (!tb[WLAN_ATTR_SAR_LIMITS_SAR_ENABLE]) {
		wl_err("attr sar enable failed\n");
		return -EINVAL;
	}

	bdf = nla_get_u32(tb[WLAN_ATTR_SAR_LIMITS_SAR_ENABLE]);
	if (bdf > WLAN_SAR_LIMITS_USER) {
		wl_err("bdf value:%d exceed the max value\n", bdf);
		return -EINVAL;
	}

	if (WLAN_SAR_LIMITS_BDF0 == bdf) {
		/*set sar limits*/
		ret = sprdwl_power_save(priv, vif->ctx_id,
				 SPRDWL_SET_TX_POWER, bdf);
	} else if (WLAN_SAR_LIMITS_NONE == bdf) {
		/*reset sar limits*/
		ret = sprdwl_power_save(priv, vif->ctx_id,
				SPRDWL_SET_TX_POWER, -1);
	}
	return ret;
#endif
}

static int sprdwl_start_offload_packet(struct sprdwl_priv *priv,
					   u8 vif_ctx_id,
					   struct nlattr **tb,
					   u32 request_id)
{
	u8 src[ETH_ALEN], dest[ETH_ALEN];
	u32 period, len;
	u16 prot_type;
	u8 *data, *pos;
	int ret;

	if (!tb[ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA] ||
	    !tb[ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR] ||
	    !tb[ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR] ||
	    !tb[ATTR_OFFLOADED_PACKETS_PERIOD] ||
	    !tb[ATTR_OFFLOADED_PACKETS_ETHER_PROTO_TYPE]) {
		pr_err("check start offload para failed\n");
		return -EINVAL;
	}

	period = nla_get_u32(tb[ATTR_OFFLOADED_PACKETS_PERIOD]);
	prot_type = nla_get_u16(tb[ATTR_OFFLOADED_PACKETS_ETHER_PROTO_TYPE]);
	prot_type = htons(prot_type);
	nla_memcpy(src, tb[ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR], ETH_ALEN);
	nla_memcpy(dest, tb[ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR], ETH_ALEN);
	len = nla_len(tb[ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA]);

	data = kzalloc(len + 14, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	pos = data;
	memcpy(pos, dest, ETH_ALEN);
	pos += ETH_ALEN;
	memcpy(pos, src, ETH_ALEN);
	pos += ETH_ALEN;
	memcpy(pos, &prot_type, 2);
	pos += 2;
	memcpy(pos, nla_data(tb[ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA]), len);

	ret = sprdwl_set_packet_offload(priv, vif_ctx_id,
					request_id, 1, period,
					len + 14,  data);
	kfree(data);

	return ret;
}

static int sprdwl_stop_offload_packet(struct sprdwl_priv *priv,
					  u8 vif_ctx_id, u32 request_id)
{
	return sprdwl_set_packet_offload(priv, vif_ctx_id,
					 request_id, 0, 0, 0, NULL);
}

static int sprdwl_set_offload_packet(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
					 const void *data, int len)
{
	int err;
	u8 control;
	u32 req;
	struct nlattr *tb[ATTR_OFFLOADED_PACKETS_MAX + 1];
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	struct sprdwl_priv *priv = wiphy_priv(wiphy);

	if (!data) {
		wiphy_err(wiphy, "%s offload failed\n", __func__);
		return -EINVAL;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	err = nla_parse(tb, ATTR_OFFLOADED_PACKETS_MAX, data,
			len, NULL, NULL);
#else
	err = nla_parse(tb, ATTR_OFFLOADED_PACKETS_MAX, data,
			len, NULL);
#endif
	if (err) {
		wiphy_err(wiphy, "%s parse attr failed", __func__);
		return err;
	}

	if (!tb[ATTR_OFFLOADED_PACKETS_REQUEST_ID] ||
	    !tb[ATTR_OFFLOADED_PACKETS_SENDING_CONTROL]) {
		wiphy_err(wiphy, "check request id or control failed\n");
		return -EINVAL;
	}

	req = nla_get_u32(tb[ATTR_OFFLOADED_PACKETS_REQUEST_ID]);
	control = nla_get_u32(tb[ATTR_OFFLOADED_PACKETS_SENDING_CONTROL]);

	switch (control) {
	case OFFLOADED_PACKETS_SENDING_STOP:
		return sprdwl_stop_offload_packet(priv, vif->ctx_id, req);
	case OFFLOADED_PACKETS_SENDING_START:
		return  sprdwl_start_offload_packet(priv, vif->ctx_id, tb, req);
	default:
		wiphy_err(wiphy, "control value is invalid\n");
		return -EINVAL;
	}

	return 0;
}

#define sprdwl_vendor_cmd_default_policy VENDOR_CMD_RAW_DATA

/* CMD ID: 9 */
static const struct nla_policy
sprdwl_vendor_roaming_enable_policy[ATTR_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_ROAMING_POLICY] = {.type = NLA_U32 },
};

/* CMD ID: 14 */
static const struct nla_policy
sprdwl_vendor_set_llstat_handler_policy[ATTR_LL_STATS_SET_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_LL_STATS_SET_CONFIG_MPDU_SIZE_THRESHOLD] = {.type = NLA_U32 },
	[ATTR_LL_STATS_SET_CONFIG_AGGRESSIVE_STATS_GATHERING] = { .type = NLA_U32 },
};

/* CMD ID: 15 */
static const struct nla_policy
sprdwl_vendor_get_llstat_handler_policy[ATTR_LL_STATS_GET_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_LL_STATS_GET_CONFIG_REQ_ID] = {.type = NLA_U32 },
	[ATTR_LL_STATS_GET_CONFIG_REQ_MASK] = { .type = NLA_U32 },
};

/* CMD ID: 22 */
static const struct nla_policy
sprdwl_vendor_get_channel_list_policy[ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID] = {.type = NLA_U32 },
	[ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND] = {.type = NLA_U32 },
	[ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS] = {.type = NLA_U32 },
};

/* CMD ID: 23 */
static const struct nla_policy
sprdwl_vendor_get_gscan_capabilities_policy[ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID] = {.type = NLA_U32 },
};

/* CMD ID: 62 */
static const struct nla_policy
sprdwl_vendor_start_logging_policy[ATTR_WIFI_LOGGER_START_GET_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_WIFI_LOGGER_RING_ID] = {.type = NLA_U32 },
	[ATTR_WIFI_LOGGER_VERBOSE_LEVEL] = {.type = NLA_U32 },
	[ATTR_WIFI_LOGGER_FLAGS] = {.type = NLA_U32 },
};

/* CMD ID: 64 */
static const struct nla_policy
sprdwl_vendor_set_roam_params_policy[ATTR_ROAMING_PARAM_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_ROAMING_SUBCMD] = {.type = NLA_U32 },
	[ATTR_ROAMING_REQ_ID] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_WHITE_LIST_SSID_NUM_NETWORKS] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_WHITE_LIST_SSID_LIST] = {.type = NLA_NESTED },
	[ATTR_ROAMING_PARAM_WHITE_LIST_SSID] = {.type = NLA_BINARY },
	[ATTR_ROAMING_PARAM_A_BAND_BOOST_THRESHOLD] = {.type = NLA_S32 },
	[ATTR_ROAMING_PARAM_A_BAND_PENALTY_THRESHOLD] = {.type = NLA_S32 },
	[ATTR_ROAMING_PARAM_A_BAND_BOOST_FACTOR] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_A_BAND_PENALTY_FACTOR] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_A_BAND_MAX_BOOST] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_LAZY_ROAM_HISTERESYS] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_ALERT_ROAM_RSSI_TRIGGER] = {.type = NLA_S32 },
	[ATTR_ROAMING_PARAM_SET_LAZY_ROAM_ENABLE] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_SET_BSSID_PREFS] = {.type = NLA_NESTED },
	[ATTR_ROAMING_PARAM_SET_LAZY_ROAM_NUM_BSSID] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_SET_LAZY_ROAM_BSSID] = {.type = NLA_MSECS, .len  = ETH_ALEN },
	[ATTR_ROAMING_PARAM_SET_LAZY_ROAM_RSSI_MODIFIER] = {.type = NLA_S32 },
	[ATTR_ROAMING_PARAM_SET_BSSID_PARAMS] = {.type = NLA_NESTED },
	[ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID] = {.type = NLA_U32 },
	[ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID] = {.type = NLA_MSECS, .len  = ETH_ALEN },
};

/* CMD ID: 76 */
static const struct nla_policy
sprdwl_vendor_get_logger_feature_policy[ATTR_LOGGER_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_LOGGER_SUPPORTED] = {.type = NLA_U32 },
};

/* CMD ID: 77 */
static const struct nla_policy
sprdwl_vendor_get_ring_data_policy[ATTR_WIFI_LOGGER_START_GET_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_WIFI_LOGGER_RING_ID] = { .type = NLA_U32 },
};

/* CMD ID: 79 */
static const struct nla_policy
sprdwl_set_offload_packet_policy[ATTR_OFFLOADED_PACKETS_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_OFFLOADED_PACKETS_SENDING_CONTROL] = {.type = NLA_U32 },
	[ATTR_OFFLOADED_PACKETS_REQUEST_ID] = { .type = NLA_U32 },
	[ATTR_OFFLOADED_PACKETS_ETHER_PROTO_TYPE] = { .type = NLA_U16 },
	[ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA] = { .type = NLA_MSECS },
	[ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR] = { .type = NLA_MSECS, .len  = ETH_ALEN },
	[ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR] = { .type = NLA_MSECS, .len  = ETH_ALEN },
	[ATTR_OFFLOADED_PACKETS_PERIOD]  = { .type = NLA_U32 },
};

/* CMD ID: 82 */
static const struct nla_policy
sprdwl_vendor_enable_nd_offload_policy[ATTR_ND_OFFLOAD_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_ND_OFFLOAD_FLAG] = {.type = NLA_U8 },
};

/* CMD ID: 85 */
static const struct nla_policy
sprdwl_vendor_get_wake_state_policy[ATTR_WAKE_STATS_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_WAKE_STATS_CMD_EVENT_WAKE_CNT_SZ] = { .type = NLA_U32 },
	[ATTR_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_SZ] = { .type = NLA_U32 },
};

/* CMD ID: 146 */
static const struct nla_policy
sprdwl_vendor_set_sar_limits_policy[ATTR_SAR_LIMITS_MAX + 1] = {
	[0] = {.type = NLA_UNSPEC },
	[ATTR_SAR_LIMITS_SAR_ENABLE] = {.type = NLA_U32 },
};

const struct wiphy_vendor_command sprdwl_vendor_cmd[] = {
	{/*9*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_ROAMING,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_roaming_enable_policy,
		.maxattr = ATTR_MAX,
		.doit = sprdwl_vendor_roaming_enable,
	},
	{/*12*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_NAN,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_nan_enable,
	},
	{/*14*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SET_LLSTAT
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_set_llstat_handler_policy,
		.maxattr = ATTR_LL_STATS_SET_AFTER_LAST,
		.doit = sprdwl_vendor_set_llstat_handler
	},
	{/*15*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_GET_LLSTAT
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_get_llstat_handler_policy,
		.maxattr = ATTR_LL_STATS_GET_MAX,
		.doit = sprdwl_vendor_get_llstat_handler
	},
	{/*16*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_CLR_LLSTAT
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_clr_llstat_handler
	},
	{/*20*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_START,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_gscan_start,
	},
	{/*21*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_STOP,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_gscan_stop,
	},
	{/*22*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CHANNEL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_get_channel_list_policy,
		.maxattr = ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX,
		.doit = sprdwl_vendor_get_channel_list,
	},
	{/*23*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_get_gscan_capabilities_policy,
		.maxattr = ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX,
		.doit = sprdwl_vendor_get_gscan_capabilities,
	},
	{/*24*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_get_cached_gscan_results,
	},
	{/*29*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_set_bssid_hotlist,
	},
	{/*30*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_reset_bssid_hotlist,
	},
	{/*32*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_set_significant_change,
	},
	{/*33*/
	    {
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE,
	    },
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_reset_significant_change,
	},
	{/*38*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GET_SUPPORT_FEATURE,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_get_support_feature,
	},
	{/*39*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_SET_MAC_OUI,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_set_mac_oui,
	},
	{/*42*/
		{
			.vendor_id = OUI_SPREAD,
			.subcmd = SPRDWL_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_get_concurrency_matrix,
	},
	{/*55*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GET_FEATURE,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_get_feature,
	},
	{/*61*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GET_WIFI_INFO,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_get_wifi_info_policy,
		.maxattr = SPRDWL_ATTR_WIFI_INFO_GET_MAX,
		.doit = sprdwl_vendor_get_driver_info,
	},
	{/*62*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_START_LOGGING,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_start_logging_policy,
		.maxattr = ATTR_WIFI_LOGGER_START_GET_MAX,
		.doit = sprdwl_vendor_start_logging,
	},
	{/*63*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_memory_dump,
	},
	{/*64*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_ROAM,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_set_roam_params_policy,
		.maxattr = ATTR_ROAMING_PARAM_MAX,
		.doit = sprdwl_vendor_set_roam_params,
	},
	{/*65*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SET_SSID_HOTLIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_set_ssid_hotlist,
	},
	{/*66*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_SSID_HOTLIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_reset_ssid_hotlist,
	},
	{/*69*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_PNO_SET_LIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_set_epno_list,
	},
	{/*70*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_set_passpoint_list,
	},
	{/*71 */
		{
			.vendor_id = OUI_SPREAD,
			.subcmd = SPRDWL_VENDOR_SUBCMD_PNO_RESET_PASSPOINT_LIST,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_reset_passpoint_list,
	},
	{/*76 */
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GET_LOGGER_FEATURE_SET,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_get_logger_feature_policy,
		.maxattr = ATTR_LOGGER_MAX,
		.doit = sprdwl_vendor_get_logger_feature,
	},
	{/*77*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GET_RING_DATA,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_get_ring_data_policy,
		.maxattr = ATTR_WIFI_LOGGER_START_GET_MAX,
		.doit = sprdwl_vendor_get_ring_data,
	},
	{/*79*/
		{
			.vendor_id = OUI_SPREAD,
			.subcmd = SPRDWL_VENDOR_SUBCMD_OFFLOADED_PACKETS,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.policy = sprdwl_set_offload_packet_policy,
		.maxattr = ATTR_OFFLOADED_PACKETS_MAX,
		.doit = sprdwl_set_offload_packet,
	},
	{/*80*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_MONITOR_RSSI,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_monitor_rssi,
	},
	{/*82*/
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_ENABLE_ND_OFFLOAD,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_enable_nd_offload_policy,
		.maxattr = ATTR_ND_OFFLOAD_MAX,
		.doit = sprdwl_vendor_enable_nd_offload,
	},
	{/*85 */
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_GET_WAKE_REASON_STATS,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_get_wake_state_policy,
		.maxattr = ATTR_WAKE_STATS_MAX,
		.doit = sprdwl_vendor_get_wake_state,
	},
	{/*146 */
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_SET_SAR_LIMITS,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_set_sar_limits_policy,
		.maxattr = ATTR_SAR_LIMITS_MAX,
		.doit = sprdwl_vendor_set_sar_limits,
	},

#ifdef NAN_SUPPORT
	{ /* 0x1300 */
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRDWL_VENDOR_SUBCMD_NAN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_NETDEV,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_vendor_nan_cmds
	},
#endif /* NAN_SUPPORT */
#ifdef RTT_SUPPORT
	{
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRD_NL80211_VENDOR_SUBCMD_LOC_GET_CAPA
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_ftm_get_capabilities
	},
	{
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRD_NL80211_VENDOR_SUBCMD_FTM_START_SESSION
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_ftm_start_session
	},
	{
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRD_NL80211_VENDOR_SUBCMD_FTM_ABORT_SESSION
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_ftm_abort_session
	},
	{
		{
		    .vendor_id = OUI_SPREAD,
		    .subcmd = SPRD_NL80211_VENDOR_SUBCMD_FTM_CFG_RESPONDER
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.policy = sprdwl_vendor_cmd_default_policy,
		.doit = sprdwl_ftm_configure_responder
	}
#endif /* RTT_SUPPORT */
};

static const struct nl80211_vendor_cmd_info sprdwl_vendor_events[] = {
	[SPRDWL_VENDOR_SUBCMD_MONITOR_RSSI_INDEX]{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_MONITOR_RSSI,
	},
	{/*1*/
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRD_RESERVED2,
	},
	{/*2*/
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRD_RESERVED2,
	},
	{/*3*/
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRD_RESERVED2,
	},
	{/*4*/
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRD_RESERVED2,
	},
	{/*5*/
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRD_RESERVED2,
	},
	/*reserver for array align*/
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_START
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_STOP
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SCAN_EVENT
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_LOST
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE
	},
	{
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE
	},

	[SPRDWL_VENDOR_EVENT_NAN_INDEX] = {
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_EVENT_NAN,
	},
	[SPRDWL_VENDOR_EVENT_EPNO_FOUND] = {
		.vendor_id = OUI_SPREAD,
		.subcmd = SPRDWL_VENDOR_EVENT_EPNO_FOUND,
	},
	[SPRD_VENDOR_EVENT_FTM_MEAS_RESULT_INDEX] = {
			.vendor_id = OUI_SPREAD,
			.subcmd = SPRD_NL80211_VENDOR_SUBCMD_FTM_MEAS_RESULT
	},
	[SPRD_VENDOR_EVENT_FTM_SESSION_DONE_INDEX] = {
			.vendor_id = OUI_SPREAD,
			.subcmd = SPRD_NL80211_VENDOR_SUBCMD_FTM_SESSION_DONE
	}
};

int sprdwl_vendor_init(struct wiphy *wiphy)
{
	wiphy->vendor_commands = sprdwl_vendor_cmd;
	wiphy->n_vendor_commands = ARRAY_SIZE(sprdwl_vendor_cmd);
	wiphy->vendor_events = sprdwl_vendor_events;
	wiphy->n_vendor_events = ARRAY_SIZE(sprdwl_vendor_events);
	return 0;
}

int sprdwl_vendor_deinit(struct wiphy *wiphy)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);

	wiphy->vendor_commands = NULL;
	wiphy->n_vendor_commands = 0;
	kfree(priv->gscan_res);
	kfree(priv->hotlist_res);
	kfree(priv->significant_res);

	return 0;
}
