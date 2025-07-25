/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * Filename : 11h.c
 * Abstract : This file is a general implement of 802.11h
 *
 * Authors  :
 * Jay.Yang <Jay.Yang@spreadtrum.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "11h.h"
#include "work.h"

int sprdwl_init_dfs_master(struct sprdwl_vif *vif)
{
	vif->dfs_cac_workqueue = alloc_workqueue("SPRDWL_DFS_CAC%s",
			WQ_HIGHPRI | WQ_MEM_RECLAIM | WQ_UNBOUND, 1, vif->name);
	if (!vif->dfs_cac_workqueue) {
		wl_err("alloc DFS CAC workqueue failure\n");
		vif->ndev = NULL;
		memset(&vif->wdev, 0, sizeof(vif->wdev));
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&vif->dfs_cac_work, sprdwl_dfs_cac_work_queue);
	vif->dfs_chan_sw_workqueue = alloc_workqueue("SPRDWL_DFS_CHSW%s",
		WQ_HIGHPRI | WQ_UNBOUND | WQ_MEM_RECLAIM, 1, vif->name);
	if (!vif->dfs_chan_sw_workqueue) {
		wl_err("alloc DFS CHSW workqueue failure\n");
		vif->ndev = NULL;
		memset(&vif->wdev, 0, sizeof(vif->wdev));
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&vif->dfs_chan_sw_work,
			  sprdwl_dfs_chan_sw_work_queue);

	return 0;
}

void sprdwl_deinit_dfs_master(struct sprdwl_vif *vif)
{
	if (vif->dfs_cac_workqueue) {
		flush_workqueue(vif->dfs_cac_workqueue);
		destroy_workqueue(vif->dfs_cac_workqueue);
		vif->dfs_cac_workqueue = NULL;
	}

	if (vif->dfs_chan_sw_workqueue) {
		flush_workqueue(vif->dfs_chan_sw_workqueue);
		destroy_workqueue(vif->dfs_chan_sw_workqueue);
		vif->dfs_chan_sw_workqueue = NULL;
	}
}

/* This is work queue function for channel switch handling.
 * This function takes care of updating new channel definitin to
 * bss config structure, restart AP and indicate channel switch success
 * to cfg80211.
 */
void sprdwl_dfs_chan_sw_work_queue(struct work_struct *work)
{
	struct delayed_work *delayed_work =
		container_of(work, struct delayed_work, work);
	struct sprdwl_vif *vif =
		container_of(delayed_work, struct sprdwl_vif,
			     dfs_chan_sw_work);

	cfg80211_ch_switch_notify(vif->ndev, &vif->dfs_chandef);
}

/*This is delayed work emits CAC finished event for cfg80211 if
 * CAC was started earlier.
 */
void sprdwl_dfs_cac_work_queue(struct work_struct *work)
{
	struct cfg80211_chan_def chandef;
	struct delayed_work *delayed_work =
		container_of(work, struct delayed_work, work);
	struct sprdwl_vif *vif =
		container_of(delayed_work, struct sprdwl_vif,
			     dfs_cac_work);

	chandef = vif->dfs_chandef;
	if (vif->wdev.cac_started) {
		wl_err("CAC timer finished; No radar detected\n");
		cfg80211_cac_event(vif->ndev, &chandef,
				   NL80211_RADAR_CAC_FINISHED,
				   GFP_KERNEL);
	}
}

int sprdwl_cfg80211_start_radar_detection(struct wiphy *wiphy,
					  struct net_device *ndev,
					  struct cfg80211_chan_def *chandef,
					  u32 cac_time_ms)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_radar_params radar_params;

	wl_debug("%s enter:\n", __func__);
	radar_params.chan_num = chandef->chan->hw_value;
	radar_params.chan_width = chandef->width;
	radar_params.cac_time_ms = cac_time_ms;

	memcpy(&vif->dfs_chandef, chandef, sizeof(vif->dfs_chandef));
	/*send radar detect CMD*/
	sprdwl_send_dfs_cmd(vif, &radar_params, sizeof(radar_params));

	queue_delayed_work(vif->dfs_cac_workqueue, &vif->dfs_cac_work,
			   msecs_to_jiffies(cac_time_ms));

	return 0;
}

int sprdwl_cfg80211_channel_switch(struct wiphy *wiphy, struct net_device *ndev,
				   struct cfg80211_csa_settings *params)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct ieee_types_header *chsw_ie;
	struct ieee80211_channel_sw_ie *channel_sw;
	struct cfg80211_beacon_data *beacon = &params->beacon_csa;
	int chsw_msec = 0;

	if (vif->wdev.cac_started)
		return -EBUSY;

	if (cfg80211_chandef_identical(&params->chandef, &vif->dfs_chandef))
		return -EINVAL;

	chsw_ie = (void *)cfg80211_find_ie(WLAN_EID_CHANNEL_SWITCH,
			 params->beacon_csa.tail, params->beacon_csa.tail_len);

	if (!chsw_ie) {
		wl_err("Couldn't parse chan switch announcement IE\n");
		return -EINVAL;
	}

	channel_sw = (void *)(chsw_ie + 1);
	if (channel_sw->mode) {
		if (netif_carrier_ok(vif->ndev))
			netif_carrier_off(vif->ndev);
		/*stop tx Q*/
		if (!netif_queue_stopped(vif->ndev))
			netif_stop_queue(vif->ndev);
	}

	if (beacon->tail_len)
		sprdwl_reset_beacon(vif->priv, vif->ctx_id,
				    beacon->tail, beacon->tail_len);

	/*set mgmt ies*/
	if (sprdwl_change_beacon(vif, beacon)) {
		wl_err("set beacon IE failure\n");
		return -EINVAL;
	}

	memcpy(&vif->dfs_chandef, &params->chandef, sizeof(vif->dfs_chandef));
	chsw_msec = max(channel_sw->count * vif->priv->beacon_period, 500);
	queue_delayed_work(vif->dfs_chan_sw_workqueue, &vif->dfs_chan_sw_work,
			   msecs_to_jiffies(chsw_msec));
	return 0;
}

void sprdwl_stop_radar_detection(struct sprdwl_vif *vif,
				 struct cfg80211_chan_def *chandef)
{
	struct sprdwl_work *misc_work;
	struct sprdwl_radar_params radar_params;

	memset(&radar_params, 0, sizeof(struct sprdwl_radar_params));
	radar_params.chan_num = chandef->chan->hw_value;
	radar_params.chan_width = chandef->width;
	radar_params.cac_time_ms = 0;

	/*send stop radar detection cmd*/
	misc_work = sprdwl_alloc_work(sizeof(radar_params));
	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_DFS;
	memcpy(misc_work->data, &radar_params, sizeof(radar_params));

	sprdwl_queue_work(vif->priv, misc_work);
}

/* This function is to abort ongoing CAC upon stopping AP operations
 * or during unload.
 */
void sprdwl_abort_cac(struct sprdwl_vif *vif)
{
	if (vif->wdev.cac_started) {
		sprdwl_stop_radar_detection(vif, &vif->dfs_chandef);
		cancel_delayed_work_sync(&vif->dfs_cac_work);
		cfg80211_cac_event(vif->ndev, &vif->dfs_chandef,
				   NL80211_RADAR_CAC_ABORTED, GFP_KERNEL);
	}
}

/* Handler for radar detected event from FW.*/
int sprdwl_11h_handle_radar_detected(struct sprdwl_vif *vif,
				     u8 *data, u16 len)
{
	struct sprdwl_radar_event *rdr_event;

	rdr_event = (struct sprdwl_radar_event *)data;

	wl_debug("radar detected; indicating kernel\n");
	sprdwl_stop_radar_detection(vif, &vif->dfs_chandef);
	cfg80211_radar_event(vif->priv->wiphy, &vif->dfs_chandef,
			     GFP_KERNEL);
	/*print radar detect reg & type*/
	wl_debug("regdomain:%d,radar detection type:%d\n",
		 rdr_event->reg_domain, rdr_event->det_type);
	return 0;
}

void sprdwl_send_dfs_cmd(struct sprdwl_vif *vif, void *data, int len)
{
	struct sprdwl_msg_buf *msg;

	wl_debug("%s:enter\n", __func__);
	msg = sprdwl_cmd_getbuf(vif->priv, len, vif->ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_RADAR_DETECT);
	memcpy(msg->data, data, len);
	sprdwl_cmd_send_recv(vif->priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

