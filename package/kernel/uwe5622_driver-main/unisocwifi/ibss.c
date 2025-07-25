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

#include "ibss.h"
#include "sprdwl.h"

#define IBSS_INITIAL_SCAN_ALLOWED	(1)
#define IBSS_COALESCE_ALLOWED		(0)
#define IBSS_COLESCE			(0)
#define IBSS_SCAN_SUPPRESS		(0)
#define IBSS_ATIM			(10)
#define WPA_RSN			(2)

/* cfg80211 */
int sprdwl_cfg80211_join_ibss(struct wiphy *wiphy,
					 struct net_device *ndev,
					 struct cfg80211_ibss_params *params)
{
	int ret = 0;
	struct ieee80211_channel *chan;
	struct sprdwl_join_params join_params;
	u32 join_params_size;
	u8 coalesce = IBSS_COLESCE;
	u8 scan_suppress = IBSS_SCAN_SUPPRESS;
	u8 atim = IBSS_ATIM;
#ifdef IBSS_RSN_SUPPORT
	u8 wpa_version = WPA_RSN;
#endif /* IBSS_RSN_SUPPORT */

	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s enter\n", __func__);

	if (SPRDWL_MODE_IBSS != vif->mode) {
		wl_ndev_log(L_ERR, ndev, "%s invalid mode: %d\n", __func__,
			   vif->mode);
		return -EINVAL;
	}

	if (!params->ssid || params->ssid_len <= 0) {
		wl_ndev_log(L_ERR, ndev, "%s invalid SSID\n", __func__);
		return -EINVAL;
	}

	/* set channel */
	chan = params->chandef.chan;
	if (chan) {
		ret =  sprdwl_set_channel(vif->priv, vif->mode,
					  ieee80211_frequency_to_channel(
					  chan->center_freq));
		if (ret < 0) {
			wl_ndev_log(L_ERR, ndev, "%s set channel failed(%d)\n",
				   __func__, ret);
			return ret;
		}
	}

	 /* Join with specific SSID */

	wl_ndev_log(L_INFO, ndev, "%s params->ssid=%s\n", __func__, params->ssid);
	wl_ndev_log(L_INFO, ndev, "%s params->ssid_len=%d\n",
			__func__, params->ssid_len);
	join_params_size = sizeof(join_params);
	memset(&join_params, 0, join_params_size);
	memcpy(join_params.ssid, params->ssid, params->ssid_len);
	join_params.ssid_len = params->ssid_len;

	if (params->bssid) {
		join_params.bssid_len = ETH_ALEN;
		ether_addr_copy(join_params.bssid, params->bssid);
	} else {
		join_params.bssid_len = 0;
		memset(join_params.bssid, 0, ETH_ALEN);
	}
	wl_ndev_log(L_INFO, ndev, "%s join_params.ssid=%s\n",
			__func__, join_params.ssid);
	wl_ndev_log(L_INFO, ndev, "%s join_params.ssid_len=%d\n",
			__func__, join_params.ssid_len);

	/* attribute */
	ret = sprdwl_set_ibss_attribute(vif->priv, vif->mode,
					SPRDWL_IBSS_COALESCE, coalesce);
	if (ret) {
		wl_ndev_log(L_ERR, ndev, "%s set coalesce failed (%d)\n",
			   __func__, ret);
		return ret;
	}

	ret = sprdwl_set_ibss_attribute(vif->priv, vif->mode,
					SPRDWL_IBSS_SCAN_SUPPRESS,
					scan_suppress);
	if (ret) {
		wl_ndev_log(L_ERR, ndev, "%s set scan_suppress failed (%d)\n",
			   __func__, ret);
		return ret;
	}

	ret = sprdwl_set_ibss_attribute(vif->priv, vif->mode,
					SPRDWL_IBSS_ATIM, atim);
	if (ret) {
		wl_ndev_log(L_ERR, ndev, "%s set ATIM failed (%d)\n",
			   __func__, ret);
		return ret;
	}

#ifdef IBSS_RSN_SUPPORT
	ret = sprdwl_set_ibss_attribute(vif->priv, vif->mode,
					SPRDWL_IBSS_WPA_VERSION, wpa_version);
	if (ret) {
		wl_ndev_log(L_ERR, ndev, "%s set wpa_version failed (%d)\n",
			   __func__, ret);
		return ret;
	}
#endif /* IBSS_RSN_SUPPORT */

	ret = sprdwl_ibss_join(vif->priv, vif->mode,
				   &join_params, join_params_size);
	if (ret) {
		wl_ndev_log(L_ERR, ndev, "%s join failed (%d)\n", __func__, ret);
		return ret;
	}

	/* update */
	ether_addr_copy(vif->bssid, join_params.bssid);
	memcpy(vif->ssid, join_params.ssid, join_params.ssid_len);
	vif->ssid_len = join_params.ssid_len;

	return ret;
}

int sprdwl_cfg80211_leave_ibss(struct wiphy *wiphy,
					  struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	enum sm_state old_state = vif->sm_state;
	int ret = 0;

	wl_ndev_log(L_DBG, ndev, "%s enter\n", __func__);

	if (SPRDWL_MODE_IBSS != vif->mode) {
		wl_ndev_log(L_ERR, ndev, "%s invalid mode: %d\n", __func__,
			   vif->mode);
		return -EINVAL;
	}

	vif->sm_state = SPRDWL_DISCONNECTING;
	/* disconect, use reason code 0 temporarily*/
	ret = sprdwl_disconnect(vif->priv, vif->mode, 0);
	if (ret < 0) {
		vif->sm_state = old_state;
		wl_ndev_log(L_ERR, ndev, "%s disconnect failed (%d)\n", __func__, ret);
		return ret;
	}

	memset(vif->ssid, 0, sizeof(vif->ssid));
	memset(vif->bssid, 0, ETH_ALEN);

	return ret;
}

/* cmd */
int sprdwl_set_ibss_attribute(struct sprdwl_priv *priv, u8 vif_mode,
				u8 sub_type, u8 value)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_ibss_attr *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_mode,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_IBSS_ATTR);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_ibss_attr *)msg->data;
	p->sub_type = sub_type;
	p->value = value;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_ibss_join(struct sprdwl_priv *priv, u8 vif_mode,
			struct sprdwl_join_params *params, u32 params_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_join_params *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_mode,
				SPRDWL_HEAD_RSP, WIFI_CMD_IBSS_JOIN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_join_params *)msg->data;
	memcpy(p, params, params_len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}
