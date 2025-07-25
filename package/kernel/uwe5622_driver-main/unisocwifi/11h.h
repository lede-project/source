/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * Filename : 11h.h
 * Abstract : This file is a general definition for 802.11h
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

#ifndef _SPRDWL_11H_H
#define _SPRDWL_11H_H

#include <linux/ieee80211.h>
#include <uapi/linux/if_arp.h>
#include <net/mac80211.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include "sprdwl.h"

struct ieee_types_header {
	u8 element_id;
	u8 len;
} __packed;

struct sprdwl_radar_params {
	u8 chan_num;
	u8 chan_width;
	__le32 cac_time_ms;
} __packed;

struct sprdwl_radar_event {
	u8 reg_domain; /*1=fcc, 2=etsi, 3=mic*/
	u8 det_type; /*0=none, 1=pw(chirp), 2=pri(radar) */
} __packed;

int sprdwl_init_dfs_master(struct sprdwl_vif *vif);
void sprdwl_deinit_dfs_master(struct sprdwl_vif *vif);
void sprdwl_dfs_chan_sw_work_queue(struct work_struct *work);
void sprdwl_dfs_cac_work_queue(struct work_struct *work);
int sprdwl_cfg80211_start_radar_detection(struct wiphy *wiphy,
		 struct net_device *dev, struct cfg80211_chan_def *chandef,
		  u32 cac_time_ms);
int sprdwl_cfg80211_channel_switch(struct wiphy *wiphy, struct net_device *dev,
		struct cfg80211_csa_settings *params);
void sprdwl_stop_radar_detection(struct sprdwl_vif *vif,
		struct cfg80211_chan_def *chandef);
void sprdwl_abort_cac(struct sprdwl_vif *vif);
int sprdwl_11h_handle_radar_detected(struct sprdwl_vif *vif,
		u8 *data, u16 len);
void sprdwl_send_dfs_cmd(struct sprdwl_vif *vif, void *data, int len);
#endif
