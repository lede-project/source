/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Abstract : This file is an implementation for cfg80211 subsystem
 *
 * Authors:
 * Chaojie Xu <chaojie.xu@spreadtrum.com>
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

#include <linux/etherdevice.h>
#include <net/cfg80211.h>
#include <net/netlink.h>
#include "sprdwl.h"
#include "rtt.h"

/* FTM session ID we use with FW */
#define FTM_ESSION_ID		1

/* fixed spare allocation we reserve in NL messages we allocate */
#define FTM_NL_EXTRA_ALLOC		32

/* approx maximum length for FTM_MEAS_RESULT NL80211 event */
#define FTM_MEAS_RESULT_MAX_LENGTH	2048

/* maximum number of allowed FTM measurements per burst */
#define FTM_MAX_MEAS_PER_BURST	31

/* initial token to use on non-secure FTM measurement */
#define FTM_DEFAULT_INITIAL_TOKEN	2

#define FTM_MAX_LCI_LENGTH		(240)
#define FTM_MAX_LCR_LENGTH		(240)

/* max rtt cmd response length */
#define RTT_RSP_LEN (128)

enum session_start_flags {
	FTM_SESSION_START_FLAG_SECURED	= 0x1,
	FTM_SESSION_START_FLAG_ASAP		= 0x2,
	FTM_SESSION_START_FLAG_LCI_REQ	= 0x4,
	FTM_SESSION_START_FLAG_LCR_REQ	= 0x8,
};

enum rtt_subcmd {
	RTT_ENABLE,
	RTT_DISABLE,
	RTT_GET_CAPABILITIES,
	RTT_RANGE_REQUEST,
	RTT_RANGE_CANCEL,
	RTT_SET_CLI,
	RTT_SET_CLR,
	RTT_GET_RESPONDER_INFO,
	RTT_ENABLE_RESPONDER,
	RTT_DISABLE_RESPONDER,
};

enum rtt_subevt {
	RTT_SESSION_END,
	RTT_PER_DEST_RES,
};

/* Responder FTM Results */
struct sprdwl_responder_ftm_res {
	u8 t1[6];
	u8 t2[6];
	u8 t3[6];
	u8 t4[6];
	__le16 tod_err;
	__le16 toa_err;
	__le16 tod_err_initiator;
	__le16 toa_err_initiator;
} __packed;

enum ftm_per_dest_res_status {
	FTM_PER_DEST_RES_NO_ERROR		= 0x00,
	FTM_PER_DEST_RES_TX_RX_FAIL		= 0x01,
	FTM_PER_DEST_RES_PARAM_DONT_MATCH	= 0x02,
};

enum ftm_per_dest_res_flags {
	FTM_PER_DEST_RES_REQ_START		= 0x01,
	FTM_PER_DEST_RES_BURST_REPORT_END	= 0x02,
	FTM_PER_DEST_RES_REQ_END		= 0x04,
	FTM_PER_DEST_RES_PARAM_UPDATE		= 0x08,
};

struct ftm_per_dest_res {
	/* FTM session ID */
	__le32 session_id;
	/* destination MAC address */
	u8 dst_mac[ETH_ALEN];
	/* wmi_tof_ftm_per_dest_res_flags_e */
	u8 flags;
	/* wmi_tof_ftm_per_dest_res_status_e */
	u8 status;
	/* responder ASAP */
	u8 responder_asap;
	/* responder number of FTM per burst */
	u8 responder_num_ftm_per_burst;
	/* responder number of FTM burst exponent */
	u8 responder_num_ftm_bursts_exp;
	/* responder burst duration ,wmi_tof_burst_duration_e */
	u8 responder_burst_duration;
	/* responder burst period, indicate interval between two consecutive
	 * burst instances, in units of 100 ms
	 */
	__le16 responder_burst_period;
	/* receive burst counter */
	__le16 bursts_cnt;
	/* tsf of responder start burst */
	__le32 tsf_sync;
	/* actual received ftm per burst */
	u8 actual_ftm_per_burst;
	u8 reserved0[7];
	struct sprdwl_responder_ftm_res responder_ftm_res[0];
} __packed;

struct ftm_dest_info {
	u8 channel;
	u8 flags;
	u8 initial_token;
	u8 num_of_ftm_per_burst;
	u8 num_of_bursts_exp;
	u8 burst_duration;
	/* Burst Period indicate interval between two consecutive burst
	 * instances, in units of 100 ms
	 */
	__le16 burst_period;
	u8 dst_mac[ETH_ALEN];
	__le16 reserved;
} __packed;

struct ftm_session_start {
	__le32 session_id;
	u8 num_of_aoa_measures;
	u8 aoa_type;
	__le16 num_of_dest;
	u8 reserved[4];
	struct ftm_dest_info dest_info[0];
} __packed;

struct sprdwl_cmd_rtt {
	u8 sub_cmd;
	__le16 len;
	u8 data[0];
} __packed;

static const struct
nla_policy sprdwl_nl80211_loc_policy[SPRDWL_VENDOR_ATTR_LOC_MAX + 1] = {
	[SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE] = { .type = NLA_U64 },
	[SPRDWL_VENDOR_ATTR_LOC_CAPA] = { .type = NLA_NESTED },
	[SPRDWL_VENDOR_ATTR_FTM_MEAS_PEERS] = { .type = NLA_NESTED },
	[SPRDWL_VENDOR_ATTR_FTM_MEAS_PEER_RESULTS] = { .type = NLA_NESTED },
	[SPRDWL_VENDOR_ATTR_FTM_RESPONDER_ENABLE] = { .type = NLA_FLAG },
	[SPRDWL_VENDOR_ATTR_LOC_SESSION_STATUS] = { .type = NLA_U32 },
	[SPRDWL_VENDOR_ATTR_FTM_INITIAL_TOKEN] = { .type = NLA_U8 },
	[SPRDWL_VENDOR_ATTR_AOA_TYPE] = { .type = NLA_U32 },
	[SPRDWL_VENDOR_ATTR_LOC_ANTENNA_ARRAY_MASK] = { .type = NLA_U32 },
	[SPRDWL_VENDOR_ATTR_FREQ] = { .type = NLA_U32 },
};

static const struct
nla_policy sprdwl_nl80211_ftm_peer_policy[
	SPRDWL_VENDOR_ATTR_FTM_PEER_MAX + 1] = {
	[SPRDWL_VENDOR_ATTR_FTM_PEER_MAC_ADDR] = { .len = ETH_ALEN },
	[SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAGS] = { .type = NLA_U32 },
	[SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_PARAMS] = { .type = NLA_NESTED },
	[SPRDWL_VENDOR_ATTR_FTM_PEER_SECURE_TOKEN_ID] = { .type = NLA_U8 },
	[SPRDWL_VENDOR_ATTR_FTM_PEER_FREQ] = { .type = NLA_U32 },
};

static const struct
nla_policy sprdwl_nl80211_ftm_meas_param_policy[
	SPRDWL_VENDOR_ATTR_FTM_PARAM_MAX + 1] = {
	[SPRDWL_VENDOR_ATTR_FTM_PARAM_MEAS_PER_BURST] = { .type = NLA_U8 },
	[SPRDWL_VENDOR_ATTR_FTM_PARAM_NUM_BURSTS_EXP] = { .type = NLA_U8 },
	[SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_DURATION] = { .type = NLA_U8 },
	[SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_PERIOD] = { .type = NLA_U16 },
};

static u8 sprdwl_ftm_get_channel(struct wiphy *wiphy,
				 const u8 *mac_addr, u32 freq)
{
	struct cfg80211_bss *bss;
	struct ieee80211_channel *chan;
	u8 channel;

	if (freq) {
		chan = ieee80211_get_channel(wiphy, freq);
		if (!chan) {
			wl_err("invalid freq: %d\n", freq);
			return 0;
		}
		channel = chan->hw_value;
	} else {
		bss = cfg80211_get_bss(wiphy, NULL, mac_addr,
					   NULL, 0, WLAN_CAPABILITY_ESS,
					   WLAN_CAPABILITY_ESS);
		if (!bss) {
			wl_err("Unable to find BSS\n");
			return 0;
		}
		channel = bss->channel->hw_value;
		cfg80211_put_bss(wiphy, bss);
	}

	wl_info("target %pM at channel %d\n", mac_addr, channel);
	return channel;
}

static int sprdwl_ftm_parse_meas_params(struct sprdwl_vif *vif,
					struct nlattr *attr,
					struct sprdwl_ftm_meas_params *params)
{
	struct nlattr *tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_MAX + 1];
	int rc;

	if (!attr) {
		/* temporary defaults for one-shot measurement */
		params->meas_per_burst = 1;
		params->burst_period = 5; /* 500 milliseconds */
		return 0;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	rc = nla_parse_nested(tb, SPRDWL_VENDOR_ATTR_FTM_PARAM_MAX,
				  attr, sprdwl_nl80211_ftm_meas_param_policy, NULL);
#else
	rc = nla_parse_nested(tb, SPRDWL_VENDOR_ATTR_FTM_PARAM_MAX,
				attr, sprdwl_nl80211_ftm_meas_param_policy);
#endif
	if (rc) {
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s: invalid measurement params\n", __func__);
		return rc;
	}
	if (tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_MEAS_PER_BURST])
		params->meas_per_burst = nla_get_u8(
			tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_MEAS_PER_BURST]);
	if (tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_NUM_BURSTS_EXP])
		params->num_of_bursts_exp = nla_get_u8(
			tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_NUM_BURSTS_EXP]);
	if (tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_DURATION])
		params->burst_duration = nla_get_u8(
			tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_DURATION]);
	if (tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_PERIOD])
		params->burst_period = nla_get_u16(
			tb[SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_PERIOD]);
	return 0;
}

static int
sprdwl_ftm_validate_meas_params(struct sprdwl_vif *vif,
				struct sprdwl_ftm_meas_params *params)
{
	if (params->meas_per_burst > FTM_MAX_MEAS_PER_BURST ||
		params->num_of_bursts_exp != 0) {
		wl_ndev_log(L_ERR, vif->ndev, "%s: invalid meas per burst\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int sprdwl_ftm_append_meas_params(struct sprdwl_priv *priv,
					 struct sk_buff *msg,
					 struct sprdwl_ftm_meas_params *params)
{
	struct nlattr *nl_p;

	nl_p = nla_nest_start(
		msg, SPRDWL_VENDOR_ATTR_FTM_PEER_RES_MEAS_PARAMS);
	if (!nl_p)
		goto out_put_failure;
	if (nla_put_u8(msg, SPRDWL_VENDOR_ATTR_FTM_PARAM_MEAS_PER_BURST,
			   params->meas_per_burst) ||
		nla_put_u8(msg, SPRDWL_VENDOR_ATTR_FTM_PARAM_NUM_BURSTS_EXP,
			   params->num_of_bursts_exp) ||
		nla_put_u8(msg, SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_DURATION,
			   params->burst_duration) ||
		nla_put_u16(msg, SPRDWL_VENDOR_ATTR_FTM_PARAM_BURST_PERIOD,
			params->burst_period))
		goto out_put_failure;
	nla_nest_end(msg, nl_p);
	return 0;
out_put_failure:
	return -ENOBUFS;
}

static int sprdwl_ftm_append_peer_meas_res(struct sprdwl_priv *priv,
					   struct sk_buff *msg,
					   struct sprdwl_ftm_peer_meas_res *res)
{
	struct nlattr *nl_mres, *nl_f;
	int i;

	if (nla_put(msg, SPRDWL_VENDOR_ATTR_FTM_PEER_RES_MAC_ADDR,
			ETH_ALEN, res->mac_addr) ||
		nla_put_u32(msg, SPRDWL_VENDOR_ATTR_FTM_PEER_RES_FLAGS,
			res->flags) ||
		nla_put_u8(msg, SPRDWL_VENDOR_ATTR_FTM_PEER_RES_STATUS,
			   res->status))
		goto out_put_failure;
	if (res->status == SPRDWL_VENDOR_ATTR_FTM_PEER_RES_STATUS_FAILED &&
		nla_put_u8(msg,
			   SPRDWL_VENDOR_ATTR_FTM_PEER_RES_VALUE_SECONDS,
			   res->value_seconds))
		goto out_put_failure;
	if (res->has_params &&
		sprdwl_ftm_append_meas_params(priv, msg, &res->params))
		goto out_put_failure;
	nl_mres = nla_nest_start(msg, SPRDWL_VENDOR_ATTR_FTM_PEER_RES_MEAS);
	if (!nl_mres)
		goto out_put_failure;
	for (i = 0; i < res->n_meas; i++) {
		nl_f = nla_nest_start(msg, i);
		if (!nl_f)
			goto out_put_failure;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		if (nla_put_u64_64bit(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T1,
				res->meas[i].t1, 0) ||
			nla_put_u64_64bit(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T2,
				res->meas[i].t2, 0) ||
			nla_put_u64_64bit(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T3,
				res->meas[i].t3, 0) ||
			nla_put_u64_64bit(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T4,
				res->meas[i].t4, 0))
			goto out_put_failure;
#else
		if (nla_put_u64(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T1,
				res->meas[i].t1) ||
			nla_put_u64(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T2,
				res->meas[i].t2) ||
			nla_put_u64(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T3,
				res->meas[i].t3) ||
			nla_put_u64(msg, SPRDWL_VENDOR_ATTR_FTM_MEAS_T4,
				res->meas[i].t4))
			goto out_put_failure;
#endif
		nla_nest_end(msg, nl_f);
	}
	nla_nest_end(msg, nl_mres);
	return 0;
out_put_failure:
	wl_err("%s: fail to append peer result\n", __func__);
	return -ENOBUFS;
}

static void sprdwl_ftm_send_meas_result(struct sprdwl_priv *priv,
					struct sprdwl_ftm_peer_meas_res *res)
{
	struct sk_buff *skb = NULL;
	struct nlattr *nl_res;
	int rc = 0;

	wl_info("sending %d results for peer %pM\n",
		res->n_meas, res->mac_addr);

	skb = cfg80211_vendor_event_alloc(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
				priv->wiphy, NULL,
#else
				priv->wiphy,
#endif
				FTM_MEAS_RESULT_MAX_LENGTH,
				SPRD_VENDOR_EVENT_FTM_MEAS_RESULT_INDEX,
				GFP_KERNEL);
	if (!skb) {
		wl_err("fail to allocate measurement result\n");
		rc = -ENOMEM;
		goto out;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	if (nla_put_u64_64bit(
		skb, SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE,
		priv->ftm.session_cookie, 0)) {
#else
	if (nla_put_u64(
		skb, SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE,
		priv->ftm.session_cookie)) {
#endif
		rc = -ENOBUFS;
		goto out;
	}

	nl_res = nla_nest_start(skb,
				SPRDWL_VENDOR_ATTR_FTM_MEAS_PEER_RESULTS);
	if (!nl_res) {
		rc = -ENOBUFS;
		goto out;
	}

	rc = sprdwl_ftm_append_peer_meas_res(priv, skb, res);
	if (rc)
		goto out;

	nla_nest_end(skb, nl_res);
	cfg80211_vendor_event(skb, GFP_KERNEL);
	skb = NULL;
out:
	if (skb)
		kfree_skb(skb);
	if (rc)
		wl_err("send peer result failed, err %d\n", rc);
}

static void sprdwl_ftm_send_peer_res(struct sprdwl_priv *priv)
{
	if (!priv->ftm.has_ftm_res || !priv->ftm.ftm_res)
		return;

	sprdwl_ftm_send_meas_result(priv, priv->ftm.ftm_res);
	priv->ftm.has_ftm_res = 0;
	priv->ftm.ftm_res->n_meas = 0;
}

static int
sprdwl_ftm_cfg80211_start_session(struct sprdwl_priv *priv,
				  struct sprdwl_vif *vif,
				  struct sprdwl_ftm_session_request *request)
{
	int ret = 0;
	bool has_lci = false, has_lcr = false;
	u8 max_meas = 0, channel, *ptr;
	u32 i, cmd_len;
	struct ftm_session_start *cmd;
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_rtt *rtt;

	mutex_lock(&priv->ftm.lock);
	if (priv->ftm.session_started) {
		wl_err("%s: FTM session already running\n", __func__);
		ret = -EALREADY;
		goto out;
	}

	for (i = 0; i < request->n_peers; i++) {
		if (request->peers[i].flags &
			SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAG_LCI)
			has_lci = true;
		if (request->peers[i].flags &
			SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAG_LCR)
			has_lcr = true;
		max_meas = max(max_meas,
				   request->peers[i].params.meas_per_burst);
	}

	priv->ftm.ftm_res = kzalloc(sizeof(*priv->ftm.ftm_res) +
			  max_meas * sizeof(struct sprdwl_ftm_peer_meas) +
			  (has_lci ? FTM_MAX_LCI_LENGTH : 0) +
			  (has_lcr ? FTM_MAX_LCR_LENGTH : 0), GFP_KERNEL);
	if (!priv->ftm.ftm_res) {
		ret = -ENOMEM;
		goto out;
	}
	ptr = (u8 *)priv->ftm.ftm_res;
	ptr += sizeof(struct sprdwl_ftm_peer_meas_res) +
		   max_meas * sizeof(struct sprdwl_ftm_peer_meas);
	if (has_lci) {
		priv->ftm.ftm_res->lci = ptr;
		ptr += FTM_MAX_LCI_LENGTH;
	}
	if (has_lcr)
		priv->ftm.ftm_res->lcr = ptr;
	priv->ftm.max_ftm_meas = max_meas;

	cmd_len = sizeof(struct ftm_session_start) +
		  request->n_peers * sizeof(struct ftm_dest_info);
	cmd = kzalloc(cmd_len, GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out_ftm_res;
	}

	cmd->session_id = cpu_to_le32(FTM_ESSION_ID);
	cmd->num_of_dest = cpu_to_le16(request->n_peers);
	for (i = 0; i < request->n_peers; i++) {
		ether_addr_copy(cmd->dest_info[i].dst_mac,
				request->peers[i].mac_addr);
		channel = sprdwl_ftm_get_channel(priv->wiphy,
						 request->peers[i].mac_addr,
						 request->peers[i].freq);
		if (!channel) {
			wl_err("%s: can't find FTM target at index %d\n",
				   __func__, i);
			ret = -EINVAL;
			goto out_cmd;
		}
		cmd->dest_info[i].channel = channel - 1;
		if (request->peers[i].flags &
			SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAG_SECURE) {
			cmd->dest_info[i].flags |=
				FTM_SESSION_START_FLAG_SECURED;
			cmd->dest_info[i].initial_token =
				request->peers[i].secure_token_id;
		} else {
			cmd->dest_info[i].initial_token =
				FTM_DEFAULT_INITIAL_TOKEN;
		}
		if (request->peers[i].flags &
			SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAG_ASAP)
			cmd->dest_info[i].flags |=
				FTM_SESSION_START_FLAG_ASAP;
		if (request->peers[i].flags &
			SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAG_LCI)
			cmd->dest_info[i].flags |=
				FTM_SESSION_START_FLAG_LCI_REQ;
		if (request->peers[i].flags &
			SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAG_LCR)
			cmd->dest_info[i].flags |=
				FTM_SESSION_START_FLAG_LCR_REQ;
		cmd->dest_info[i].num_of_ftm_per_burst =
			request->peers[i].params.meas_per_burst;
		cmd->dest_info[i].num_of_bursts_exp =
			request->peers[i].params.num_of_bursts_exp;
		cmd->dest_info[i].burst_duration =
			request->peers[i].params.burst_duration;
		cmd->dest_info[i].burst_period =
			cpu_to_le16(request->peers[i].params.burst_period);
	}

	/* send range request data to the FW */
	msg = sprdwl_cmd_getbuf(priv, sizeof(struct sprdwl_cmd_rtt) + cmd_len,
				vif->ctx_id, SPRDWL_HEAD_RSP, WIFI_CMD_RTT);
	if (!msg) {
		ret = -ENOMEM;
		goto out_cmd;
	}
	rtt = (struct sprdwl_cmd_rtt *)msg->data;
	rtt->sub_cmd = RTT_GET_CAPABILITIES;
	rtt->len = cmd_len;
	memcpy(rtt->data, cmd, cmd_len);

	ret = sprdwl_cmd_send_recv(priv, msg,
				   CMD_WAIT_TIMEOUT, NULL, 0);
	if (ret) {
		wl_ndev_log(L_ERR, vif->ndev, "%s: ret=%d\n", __func__, ret);
	} else {
		priv->ftm.session_cookie = request->session_cookie;
		priv->ftm.session_started = 1;
	}
out_cmd:
	kfree(cmd);
out_ftm_res:
	if (ret) {
		kfree(priv->ftm.ftm_res);
		priv->ftm.ftm_res = NULL;
	}
out:
	mutex_unlock(&priv->ftm.lock);
	return ret;
}

static void
sprdwl_ftm_session_ended(struct sprdwl_priv *priv, u32 status)
{
	struct sk_buff *skb = NULL;

	mutex_lock(&priv->ftm.lock);

	if (!priv->ftm.session_started) {
		wl_err("%s: FTM session not started, ignoring\n", __func__);
		return;
	}

	wl_info("%s: finishing FTM session\n", __func__);

	/* send left-over results if any */
	sprdwl_ftm_send_peer_res(priv);

	priv->ftm.session_started = 0;
	kfree(priv->ftm.ftm_res);
	priv->ftm.ftm_res = NULL;

	skb = cfg80211_vendor_event_alloc(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
		priv->wiphy, NULL,
#else
		priv->wiphy,
#endif
		FTM_NL_EXTRA_ALLOC,
		SPRD_VENDOR_EVENT_FTM_SESSION_DONE_INDEX,
		GFP_KERNEL);
	if (!skb)
		goto out;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	if (nla_put_u64_64bit(skb,
				SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE,
				priv->ftm.session_cookie, 0) ||
		nla_put_u32(skb,
			SPRDWL_VENDOR_ATTR_LOC_SESSION_STATUS, status)) {
#else
	if (nla_put_u64(skb,
			SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE,
			priv->ftm.session_cookie) ||
		nla_put_u32(skb,
			SPRDWL_VENDOR_ATTR_LOC_SESSION_STATUS, status)) {
#endif
		wl_err("%s: failed to fill session done event\n", __func__);
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	skb = NULL;
out:
	kfree_skb(skb);
	mutex_unlock(&priv->ftm.lock);
}

void sprdwl_ftm_event_per_dest_res(struct sprdwl_priv *priv,
				   struct ftm_per_dest_res *res)
{
	u32 i, index;
	__le64 tmp = 0;
	u8 n_meas;

	mutex_lock(&priv->ftm.lock);

	if (!priv->ftm.session_started || !priv->ftm.ftm_res) {
		wl_err("%s: Session not running, ignoring res event\n",
			   __func__);
		goto out;
	}
	if (priv->ftm.has_ftm_res &&
		!ether_addr_equal(res->dst_mac, priv->ftm.ftm_res->mac_addr)) {
		wl_err("%s: previous peer not properly terminated\n",
			   __func__);
		sprdwl_ftm_send_peer_res(priv);
	}

	if (!priv->ftm.has_ftm_res) {
		ether_addr_copy(priv->ftm.ftm_res->mac_addr, res->dst_mac);
		priv->ftm.has_ftm_res = 1;
	}

	n_meas = res->actual_ftm_per_burst;
	switch (res->status) {
	case FTM_PER_DEST_RES_NO_ERROR:
		priv->ftm.ftm_res->status =
			SPRDWL_VENDOR_ATTR_FTM_PEER_RES_STATUS_OK;
		break;
	case FTM_PER_DEST_RES_TX_RX_FAIL:
		/* FW reports corrupted results here, discard. */
		n_meas = 0;
		priv->ftm.ftm_res->status =
			SPRDWL_VENDOR_ATTR_FTM_PEER_RES_STATUS_OK;
		break;
	case FTM_PER_DEST_RES_PARAM_DONT_MATCH:
		priv->ftm.ftm_res->status =
			SPRDWL_VENDOR_ATTR_FTM_PEER_RES_STATUS_INVALID;
		break;
	default:
		wl_err("%s: unexpected status %d\n", __func__, res->status);
		priv->ftm.ftm_res->status =
			SPRDWL_VENDOR_ATTR_FTM_PEER_RES_STATUS_INVALID;
		break;
	}

	for (i = 0; i < n_meas; i++) {
		index = priv->ftm.ftm_res->n_meas;
		if (index >= priv->ftm.max_ftm_meas) {
			wl_info("%s: Too many measurements\n", __func__);
			break;
		}
		memcpy(&tmp, res->responder_ftm_res[i].t1,
			   sizeof(res->responder_ftm_res[i].t1));
		priv->ftm.ftm_res->meas[index].t1 = le64_to_cpu(tmp);
		memcpy(&tmp, res->responder_ftm_res[i].t2,
			   sizeof(res->responder_ftm_res[i].t2));
		priv->ftm.ftm_res->meas[index].t2 = le64_to_cpu(tmp);
		memcpy(&tmp, res->responder_ftm_res[i].t3,
			   sizeof(res->responder_ftm_res[i].t3));
		priv->ftm.ftm_res->meas[index].t3 = le64_to_cpu(tmp);
		memcpy(&tmp, res->responder_ftm_res[i].t4,
			   sizeof(res->responder_ftm_res[i].t4));
		priv->ftm.ftm_res->meas[index].t4 = le64_to_cpu(tmp);
		priv->ftm.ftm_res->n_meas++;
	}

	if (res->flags & FTM_PER_DEST_RES_BURST_REPORT_END)
		sprdwl_ftm_send_peer_res(priv);
out:
	mutex_unlock(&priv->ftm.lock);
}

int sprdwl_event_ftm(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_priv *priv = vif->priv;
	u8 sub_event;
	u32 status;
	struct ftm_per_dest_res *res;

	memcpy(&sub_event, data, sizeof(sub_event));
	data += sizeof(sub_event);
	len -= sizeof(sub_event);

	switch (sub_event) {
	case RTT_SESSION_END:
		memcpy(&status, data, sizeof(status));
		sprdwl_ftm_session_ended(priv, status);
		break;
	case RTT_PER_DEST_RES:
		res = (struct ftm_per_dest_res *)data;
		sprdwl_ftm_event_per_dest_res(priv, res);
		break;
	default:
		wl_ndev_log(L_ERR, vif->ndev, "%s: unknown FTM event\n", __func__);
		break;
	}
	return 0;
}

int sprdwl_ftm_get_capabilities(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_rtt *cmd;
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	u8 rsp[RTT_RSP_LEN] = {0x0};
	u16 rsp_len = RTT_RSP_LEN;
	int ret = 0;
	struct sk_buff *skb;
	struct nlattr *attr;

	/* get the capabilities from the FW */
	msg = sprdwl_cmd_getbuf(vif->priv, sizeof(struct sprdwl_cmd_rtt) + len,
				vif->ctx_id, SPRDWL_HEAD_RSP, WIFI_CMD_RTT);
	if (!msg)
		return -ENOMEM;
	cmd = (struct sprdwl_cmd_rtt *)msg->data;
	cmd->sub_cmd = RTT_GET_CAPABILITIES;
	cmd->len = len;
	memcpy(cmd->data, data, len);

	ret = sprdwl_cmd_send_recv(vif->priv, msg,
				   CMD_WAIT_TIMEOUT, rsp, &rsp_len);
	if (ret) {
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s: ret=%d, rsp_len=%d\n", __func__, ret, rsp_len);
	}

	/* report capabilities */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, RTT_RSP_LEN);
	if (!skb)
		return -ENOMEM;
	attr = nla_nest_start(skb, SPRDWL_VENDOR_ATTR_LOC_CAPA);
	if (!attr ||
		nla_put_u32(skb, SPRDWL_VENDOR_ATTR_LOC_CAPA_FLAGS,
			SPRDWL_VENDOR_ATTR_LOC_CAPA_FLAG_FTM_RESPONDER |
			SPRDWL_VENDOR_ATTR_LOC_CAPA_FLAG_FTM_INITIATOR |
			SPRDWL_VENDOR_ATTR_LOC_CAPA_FLAG_ASAP |
			SPRDWL_VENDOR_ATTR_LOC_CAPA_FLAG_AOA) ||
		nla_put_u16(skb, SPRDWL_VENDOR_ATTR_FTM_CAPA_MAX_NUM_SESSIONS,
			1) ||
		nla_put_u16(skb, SPRDWL_VENDOR_ATTR_FTM_CAPA_MAX_NUM_PEERS, 1) ||
		nla_put_u8(skb, SPRDWL_VENDOR_ATTR_FTM_CAPA_MAX_NUM_BURSTS_EXP,
			   0) ||
		nla_put_u8(skb, SPRDWL_VENDOR_ATTR_FTM_CAPA_MAX_MEAS_PER_BURST,
			   4) ||
		nla_put_u32(skb, SPRDWL_VENDOR_ATTR_AOA_CAPA_SUPPORTED_TYPES,
			BIT(SPRDWL_VENDOR_ATTR_AOA_TYPE_TOP_CIR_PHASE))) {
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s: fail to fill capabilities\n", __func__);
		kfree_skb(skb);
		return -ENOMEM;
	}
	nla_nest_end(skb, attr);

	return cfg80211_vendor_cmd_reply(skb);
}

int sprdwl_ftm_start_session(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	struct sprdwl_ftm_session_request *request;
	struct nlattr *tb[SPRDWL_VENDOR_ATTR_LOC_MAX + 1];
	struct nlattr *tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MAX + 1];
	struct nlattr *peer;
	int rc, n_peers = 0, index = 0, rem;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	rc = nla_parse(tb, SPRDWL_VENDOR_ATTR_LOC_MAX, data, data_len,
			   sprdwl_nl80211_loc_policy, NULL);
#else
	rc = nla_parse(tb, SPRDWL_VENDOR_ATTR_LOC_MAX, data, data_len,
			sprdwl_nl80211_loc_policy);
#endif
	if (rc) {
		wl_ndev_log(L_ERR, vif->ndev, "%s: invalid FTM attribute\n", __func__);
		return rc;
	}

	if (!tb[SPRDWL_VENDOR_ATTR_FTM_MEAS_PEERS]) {
		wl_ndev_log(L_ERR, vif->ndev, "%s: no peers specified\n", __func__);
		return -EINVAL;
	}

	if (!tb[SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE]) {
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s: session cookie not specified\n", __func__);
		return -EINVAL;
	}

	nla_for_each_nested(peer, tb[SPRDWL_VENDOR_ATTR_FTM_MEAS_PEERS],
				rem)
		n_peers++;

	if (!n_peers) {
		wl_ndev_log(L_ERR, vif->ndev, "%s: empty peer list\n", __func__);
		return -EINVAL;
	}

	/* for now only allow measurement for a single peer */
	if (n_peers != 1) {
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s: only single peer allowed\n", __func__);
		return -EINVAL;
	}

	request = kzalloc(sizeof(*request) +
			  n_peers * sizeof(struct sprdwl_ftm_meas_peer_info),
			  GFP_KERNEL);
	if (!request)
		return -ENOMEM;

	request->session_cookie =
		nla_get_u64(tb[SPRDWL_VENDOR_ATTR_FTM_SESSION_COOKIE]);
	request->n_peers = n_peers;
	nla_for_each_nested(peer, tb[SPRDWL_VENDOR_ATTR_FTM_MEAS_PEERS],
				rem) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
		rc = nla_parse_nested(tb2, SPRDWL_VENDOR_ATTR_FTM_PEER_MAX,
					  peer, sprdwl_nl80211_ftm_peer_policy, NULL);
#else
		rc = nla_parse_nested(tb2, SPRDWL_VENDOR_ATTR_FTM_PEER_MAX,
				peer, sprdwl_nl80211_ftm_peer_policy);
#endif
		if (rc) {
			wl_ndev_log(L_ERR, vif->ndev,
				   "%s: invalid peer attribute\n", __func__);
			goto out;
		}
		if (!tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MAC_ADDR] ||
			nla_len(tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MAC_ADDR])
				!= ETH_ALEN) {
			wl_ndev_log(L_ERR, vif->ndev,
				   "%s: peer MAC address missing or invalid\n",
				   __func__);
			rc = -EINVAL;
			goto out;
		}
		memcpy(request->peers[index].mac_addr,
			   nla_data(tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MAC_ADDR]),
			   ETH_ALEN);
		if (tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_FREQ])
			request->peers[index].freq = nla_get_u32(
				tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_FREQ]);
		if (tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAGS])
			request->peers[index].flags = nla_get_u32(
				tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_FLAGS]);
		if (tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_SECURE_TOKEN_ID])
			request->peers[index].secure_token_id = nla_get_u8(
			   tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_SECURE_TOKEN_ID]);
		rc = sprdwl_ftm_parse_meas_params(
			vif,
			tb2[SPRDWL_VENDOR_ATTR_FTM_PEER_MEAS_PARAMS],
			&request->peers[index].params);
		if (!rc)
			rc = sprdwl_ftm_validate_meas_params(
				vif, &request->peers[index].params);
		if (rc)
			goto out;
		index++;
	}

	rc = sprdwl_ftm_cfg80211_start_session(priv, vif, request);
out:
	kfree(request);
	return rc;
}

int sprdwl_ftm_abort_session(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_rtt *cmd;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);
	int ret;

	mutex_lock(&priv->ftm.lock);
	if (!priv->ftm.session_started) {
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s: FTM session not started\n", __func__);
		return -EAGAIN;
	}
	/* send cancel range request */
	msg = sprdwl_cmd_getbuf(priv, sizeof(struct sprdwl_cmd_rtt) + len,
				vif->ctx_id, 0, WIFI_CMD_RTT);
	if (!msg)
		return -ENOMEM;
	cmd = (struct sprdwl_cmd_rtt *)msg->data;
	cmd->sub_cmd = RTT_RANGE_CANCEL;
	cmd->len = len;
	memcpy(cmd->data, data, len);

	ret = sprdwl_cmd_send_recv(priv, msg,
				   CMD_WAIT_TIMEOUT, NULL, 0);
	if (ret)
		wl_ndev_log(L_ERR, vif->ndev, "%s: ret=%d\n", __func__, ret);

	mutex_unlock(&priv->ftm.lock);

	return ret;
}

int sprdwl_ftm_get_responder_info(struct wiphy *wiphy,
				  struct wireless_dev *wdev,
				  const void *data, int len)
{
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);

	/* get responder info */
	wl_ndev_log(L_INFO, vif->ndev, "%s: not implemented yet\n", __func__);
	return -ENOTSUPP;
}

int sprdwl_ftm_configure_responder(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   const void *data, int data_len)
{
	struct sprdwl_vif *vif = netdev_priv(wdev->netdev);

	/* enable or disable responder */
	wl_ndev_log(L_INFO, vif->ndev, "%s: not implemented yet\n", __func__);
	return -ENOTSUPP;
}

void sprdwl_ftm_init(struct sprdwl_priv *priv)
{
	mutex_init(&priv->ftm.lock);
}

void sprdwl_ftm_deinit(struct sprdwl_priv *priv)
{
	kfree(priv->ftm.ftm_res);
}

void sprdwl_ftm_stop_operations(struct sprdwl_priv *priv)
{
	sprdwl_ftm_session_ended(
		priv, SPRDWL_VENDOR_ATTR_LOC_SESSION_STATUS_ABORTED);
}

