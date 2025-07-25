/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Abstract : This file is an implementation for cfg80211 subsystem
 *
 * Authors:
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
 * Jingxiang Li <Jingxiang.li@spreadtrum.com>
 * Dong Xiang <dong.xiang@spreadtrum.com>
 * Huiquan Zhou <huiquan.zhou@spreadtrum.com>
 * Baolei Yuan <baolei.yuan@spreadtrum.com>
 * Xianwei Zhao <xianwei.zhao@spreadtrum.com>
 * Gui Zhu <gui.zhu@spreadtrum.com>
 * Andy He <andy.he@spreadtrum.com>
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
#include "cfg80211.h"
#include "cmdevt.h"
#include "work.h"
#include "ibss.h"
#include "intf_ops.h"
#include "dbg_ini_util.h"
#include "tx_msg.h"
#ifdef RND_MAC_SUPPORT
#include "rnd_mac_addr.h"
#endif
#include "rx_msg.h"

#ifdef DFS_MASTER
#include "11h.h"
#endif

#ifdef WMMAC_WFA_CERTIFICATION
#include "qos.h"
#endif

#if !defined(CONFIG_CFG80211_INTERNAL_REGDB) || defined(CUSTOM_REGDOMAIN)
#include "reg_domain.h"
#endif

#define RATETAB_ENT(_rate, _rateid, _flags)				\
{									\
	.bitrate	= (_rate),					\
	.hw_value	= (_rateid),					\
	.flags		= (_flags),					\
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#define CHAN2G(_channel, _freq, _flags)                                 \
{									\
	.band                   = NL80211_BAND_2GHZ,    		\
	.center_freq            = (_freq),              		\
	.hw_value               = (_channel),           		\
	.flags                  = (_flags),             		\
	.max_antenna_gain       = 0,                    		\
	.max_power              = 30,                   		\
}
#else
#define CHAN2G(_channel, _freq, _flags)  				\
{									\
	.band                   = IEEE80211_BAND_2GHZ,  		\
	.center_freq		= (_freq),				\
	.hw_value		= (_channel),				\
	.flags			= (_flags),				\
	.max_antenna_gain	= 0,					\
	.max_power		= 30,					\
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
#define CHAN5G(_channel, _flags) 					\
{									\
	.band                   = NL80211_BAND_5GHZ,            	\
	.center_freq		= 5000 + (5 * (_channel)),		\
	.hw_value		= (_channel),				\
	.flags			= (_flags),				\
	.max_antenna_gain	= 0,					\
	.max_power		= 30,					\
}
#else
#define CHAN5G(_channel, _flags)                                        \
{									\
	.band                   = IEEE80211_BAND_5GHZ,          	\
	.center_freq            = 5000 + (5 * (_channel)),      	\
	.hw_value               = (_channel),                   	\
	.flags                  = (_flags),                     	\
	.max_antenna_gain       = 0,                            	\
	.max_power              = 30,                           	\
}
#endif

static struct ieee80211_rate sprdwl_rates[] = {
	RATETAB_ENT(10, 0x1, 0),
	RATETAB_ENT(20, 0x2, 0),
	RATETAB_ENT(55, 0x5, 0),
	RATETAB_ENT(110, 0xb, 0),
	RATETAB_ENT(60, 0x6, 0),
	RATETAB_ENT(90, 0x9, 0),
	RATETAB_ENT(120, 0xc, 0),
	RATETAB_ENT(180, 0x12, 0),
	RATETAB_ENT(240, 0x18, 0),
	RATETAB_ENT(360, 0x24, 0),
	RATETAB_ENT(480, 0x30, 0),
	RATETAB_ENT(540, 0x36, 0),

	RATETAB_ENT(65, 0x80, 0),
	RATETAB_ENT(130, 0x81, 0),
	RATETAB_ENT(195, 0x82, 0),
	RATETAB_ENT(260, 0x83, 0),
	RATETAB_ENT(390, 0x84, 0),
	RATETAB_ENT(520, 0x85, 0),
	RATETAB_ENT(585, 0x86, 0),
	RATETAB_ENT(650, 0x87, 0),
	RATETAB_ENT(130, 0x88, 0),
	RATETAB_ENT(260, 0x89, 0),
	RATETAB_ENT(390, 0x8a, 0),
	RATETAB_ENT(520, 0x8b, 0),
	RATETAB_ENT(780, 0x8c, 0),
	RATETAB_ENT(1040, 0x8d, 0),
	RATETAB_ENT(1170, 0x8e, 0),
	RATETAB_ENT(1300, 0x8f, 0)
};

#define SPRDWL_G_RATE_NUM	28
#define sprdwl_g_rates		(sprdwl_rates)
#define SPRDWL_A_RATE_NUM	24
#define sprdwl_a_rates		(sprdwl_rates + 4)

#define sprdwl_g_htcap (IEEE80211_HT_CAP_SUP_WIDTH_20_40 | \
			IEEE80211_HT_CAP_SGI_20		 | \
			IEEE80211_HT_CAP_SGI_40)

#define sprdwl_a_htcap (IEEE80211_HT_CAP_SUP_WIDTH_20_40 | \
			IEEE80211_HT_CAP_SGI_20	| \
			IEEE80211_HT_CAP_SM_PS | IEEE80211_HT_CAP_SGI_40)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
#define sprdwl_vhtcap (IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_7991 | \
		IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE | \
		IEEE80211_VHT_CAP_BEAMFORMEE_STS_SHIFT | \
		IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE | \
		IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT | \
		IEEE80211_VHT_CAP_VHT_TXOP_PS)
#else
#define sprdwl_vhtcap (IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_7991 | \
		IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE | \
		IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE | \
		IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT | \
		IEEE80211_VHT_CAP_VHT_TXOP_PS)
#endif

static struct ieee80211_channel sprdwl_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0)
};

static struct ieee80211_supported_band sprdwl_band_2ghz = {
	.n_channels = ARRAY_SIZE(sprdwl_2ghz_channels),
	.channels = sprdwl_2ghz_channels,
	.n_bitrates = SPRDWL_G_RATE_NUM,
	.bitrates = sprdwl_g_rates,
	.ht_cap.cap = sprdwl_g_htcap,
	.ht_cap.ht_supported = true
};

static struct ieee80211_channel sprdwl_5ghz_channels[] = {
	CHAN5G(34, 0), CHAN5G(36, 0),
	CHAN5G(40, 0), CHAN5G(44, 0),
	CHAN5G(48, 0), CHAN5G(52, 0),
	CHAN5G(56, 0), CHAN5G(60, 0),
	CHAN5G(64, 0), CHAN5G(100, 0),
	CHAN5G(104, 0), CHAN5G(108, 0),
	CHAN5G(112, 0), CHAN5G(116, 0),
	CHAN5G(120, 0), CHAN5G(124, 0),
	CHAN5G(128, 0), CHAN5G(132, 0),
	CHAN5G(136, 0), CHAN5G(140, 0),
	CHAN5G(144, 0), CHAN5G(149, 0),
	CHAN5G(153, 0), CHAN5G(157, 0),
	CHAN5G(161, 0), CHAN5G(165, 0),
	CHAN5G(184, 0), CHAN5G(188, 0),
	CHAN5G(192, 0), CHAN5G(196, 0),
	CHAN5G(200, 0), CHAN5G(204, 0),
	CHAN5G(208, 0), CHAN5G(212, 0),
	CHAN5G(216, 0)
};

static struct ieee80211_supported_band sprdwl_band_5ghz = {
	.n_channels = ARRAY_SIZE(sprdwl_5ghz_channels),
	.channels = sprdwl_5ghz_channels,
	.n_bitrates = SPRDWL_A_RATE_NUM,
	.bitrates = sprdwl_a_rates,
	.ht_cap.cap = sprdwl_a_htcap,
	.ht_cap.ht_supported = true,
	.vht_cap.vht_supported = true,
	.vht_cap.cap = sprdwl_vhtcap,
	.vht_cap.vht_mcs.rx_mcs_map = 0xfff0,
	.vht_cap.vht_mcs.tx_mcs_map = 0xfff0,
	.vht_cap.vht_mcs.rx_highest = 0,
	.vht_cap.vht_mcs.tx_highest = 0,
};

static const u32 sprdwl_cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	WLAN_CIPHER_SUITE_SMS4,
	/* required by ieee802.11w */
	WLAN_CIPHER_SUITE_AES_CMAC,
	WLAN_CIPHER_SUITE_PMK
};

/* Supported mgmt frame types to be advertised to cfg80211 */
static const struct ieee80211_txrx_stypes
sprdwl_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			  BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			  BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			  BIT(IEEE80211_STYPE_AUTH >> 4) |
			  BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			  BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			  BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			  BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			  BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			  BIT(IEEE80211_STYPE_AUTH >> 4) |
			  BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			  BIT(IEEE80211_STYPE_ACTION >> 4)
	},
#ifndef CONFIG_P2P_INTF
	[NL80211_IFTYPE_P2P_DEVICE] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			  BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	}
#endif
};

static const struct ieee80211_iface_limit sprdwl_iface_limits[] = {
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_STATION) |
			 BIT(NL80211_IFTYPE_AP)
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
			 BIT(NL80211_IFTYPE_P2P_GO)
	},
#ifndef CONFIG_P2P_INTF
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_DEVICE)
	}
#endif
};

static const struct ieee80211_iface_combination sprdwl_iface_combos[] = {
	{
#ifndef CONFIG_P2P_INTF
		 .max_interfaces = 3,
#else
		 .max_interfaces = 2,
#endif
		 .num_different_channels = 2,
		 .n_limits = ARRAY_SIZE(sprdwl_iface_limits),
		 .limits = sprdwl_iface_limits
	}
};

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support sprdwl_wowlan_support = {
	.flags = WIPHY_WOWLAN_ANY | WIPHY_WOWLAN_DISCONNECT | WIPHY_WOWLAN_MAGIC_PKT,
};
#endif

/* Interface related stuff*/
inline void sprdwl_put_vif(struct sprdwl_vif *vif)
{
	if (vif) {
		spin_lock_bh(&vif->priv->list_lock);
		vif->ref--;
		spin_unlock_bh(&vif->priv->list_lock);
	}
}

inline struct sprdwl_vif *ctx_id_to_vif(struct sprdwl_priv *priv, u8 vif_ctx_id)
{
	struct sprdwl_vif *vif, *found = NULL;

	spin_lock_bh(&priv->list_lock);
	list_for_each_entry(vif, &priv->vif_list, vif_node) {
		if (vif->ctx_id == vif_ctx_id) {
			vif->ref++;
			found = vif;
			break;
		}
	}
	spin_unlock_bh(&priv->list_lock);

	return found;
}

inline struct sprdwl_vif *mode_to_vif(struct sprdwl_priv *priv, u8 vif_mode)
{
	struct sprdwl_vif *vif, *found = NULL;

	spin_lock_bh(&priv->list_lock);
	list_for_each_entry(vif, &priv->vif_list, vif_node) {
		if (vif->mode == vif_mode) {
			vif->ref++;
			found = vif;
			break;
		}
	}
	spin_unlock_bh(&priv->list_lock);

	return found;
}

static inline enum sprdwl_mode type_to_mode(enum nl80211_iftype type)
{
	enum sprdwl_mode mode;

	switch (type) {
	case NL80211_IFTYPE_STATION:
		mode = SPRDWL_MODE_STATION;
		break;
	case NL80211_IFTYPE_AP:
		mode = SPRDWL_MODE_AP;
		break;
	case NL80211_IFTYPE_P2P_GO:
		mode = SPRDWL_MODE_P2P_GO;
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		mode = SPRDWL_MODE_P2P_CLIENT;
		break;
	case NL80211_IFTYPE_P2P_DEVICE:
		mode = SPRDWL_MODE_P2P_DEVICE;
		break;
#ifdef IBSS_SUPPORT
	case NL80211_IFTYPE_ADHOC:
		mode = SPRDWL_MODE_IBSS;
		break;
#endif /* IBSS_SUPPORT */
	default:
		mode = SPRDWL_MODE_NONE;
		break;
	}

	return mode;
}

int sprdwl_init_fw(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
	enum nl80211_iftype type = vif->wdev.iftype;
	enum sprdwl_mode mode;
	u8 *mac;
	u8 vif_ctx_id = 0;

	wl_ndev_log(L_DBG, vif->ndev, "%s type %d, mode %d\n", __func__, type,
			vif->mode);

	if (vif->mode != SPRDWL_MODE_NONE) {
		wl_ndev_log(L_ERR, vif->ndev, "%s already in use: mode %d\n",
			   __func__, vif->mode);
		return -EBUSY;
	}

	mode = type_to_mode(type);
	if ((mode <= SPRDWL_MODE_NONE) || (mode >= SPRDWL_MODE_MAX)) {
		wl_ndev_log(L_ERR, vif->ndev, "%s unsupported interface type: %d\n",
			   __func__, type);
		return -EINVAL;
	}

#ifdef CONFIG_P2P_INTF
	if (type == NL80211_IFTYPE_STATION) {
		if (!strncmp(vif->ndev->name, "p2p0", 4))
			mode = SPRDWL_MODE_P2P_CLIENT;
	}
#endif
	if (priv->fw_stat[mode] == SPRDWL_INTF_OPEN) {
		wl_ndev_log(L_ERR, vif->ndev, "%s mode %d already opened\n",
			   __func__, mode);
		return 0;
	}

	vif->mode = mode;
	if (!vif->ndev)
		mac = vif->wdev.address;
	else
		mac = vif->ndev->dev_addr;

	if (sprdwl_open_fw(priv, &vif_ctx_id, vif->mode, mac)) {
		wl_ndev_log(L_ERR, vif->ndev, "%s failed!\n", __func__);
		vif->mode = SPRDWL_MODE_NONE;
		return -EIO;
	}
	vif->ctx_id  = vif_ctx_id;
	wl_ndev_log(L_DBG, vif->ndev, "%s,open success type %d, mode:%d, ctx_id:%d\n",
			__func__, type,
			vif->mode, vif->ctx_id);
	priv->fw_stat[vif->mode] = SPRDWL_INTF_OPEN;

	return 0;
}

int sprdwl_uninit_fw(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	u8 count = 0;

	if ((vif->mode <= SPRDWL_MODE_NONE) || (vif->mode >= SPRDWL_MODE_MAX)) {
		wl_ndev_log(L_ERR, vif->ndev, "%s invalid operation mode: %d\n",
			   __func__, vif->mode);
		return -EINVAL;
	}

	if (priv->fw_stat[vif->mode] == SPRDWL_INTF_CLOSE) {
		wl_ndev_log(L_ERR, vif->ndev, "%s mode %d already closed\n",
			   __func__, vif->mode);
		return -EBUSY;
	}

	priv->fw_stat[vif->mode] = SPRDWL_INTF_CLOSING;

	/*flush data belong to this mode*/
	if (atomic_read(&tx_msg->tx_list[vif->mode]->mode_list_num) > 0)
		sprdwl_flush_mode_txlist(tx_msg, vif->mode);

	/*here we need to wait for 3s to avoid there
	 *is still data of this modeattached to sdio not poped
	 */
	while ((!list_empty(&tx_msg->xmit_msg_list.to_send_list) ||
			!list_empty(&tx_msg->xmit_msg_list.to_free_list)) &&
			count < 100) {
		wl_err_ratelimited("error! %s data q not empty, wait\n", __func__);
		usleep_range(2500, 3000);
		count++;
	}

	if (sprdwl_close_fw(priv, vif->ctx_id, vif->mode)) {
		wl_ndev_log(L_ERR, vif->ndev, "%s failed!\n", __func__);
		return -EIO;
	}

	priv->fw_stat[vif->mode] = SPRDWL_INTF_CLOSE;

	handle_tx_status_after_close(vif);

	wl_ndev_log(L_DBG, vif->ndev, "%s type %d, mode %d\n", __func__,
			vif->wdev.iftype, vif->mode);
	vif->mode = SPRDWL_MODE_NONE;

	return 0;
}

static inline int sprdwl_is_valid_iftype(struct wiphy *wiphy,
					 enum nl80211_iftype type)
{
	return wiphy->interface_modes & BIT(type);
}

int sprdwl_check_p2p_coex(struct sprdwl_priv *priv)
{
	if (mode_to_vif(priv, SPRDWL_MODE_P2P_CLIENT) ||
		mode_to_vif(priv, SPRDWL_MODE_P2P_GO))
		return -1;

	return 0;
}

#ifndef CONFIG_P2P_INTF
static struct wireless_dev *sprdwl_cfg80211_add_iface(struct wiphy *wiphy,
							  const char *name,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
							  unsigned char name_assign_type,
#endif
							  enum nl80211_iftype type,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
							  u32 *flags,
#endif
							  struct vif_params *params)
{
	enum sprdwl_mode mode;
	enum nl80211_iftype iftype = type;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);

	if (!sprdwl_is_valid_iftype(wiphy, type)) {
		wl_err("%s unsupported interface type: %u\n", __func__, type);
		return ERR_PTR(-EINVAL);
	}

	if (sprdwl_check_p2p_coex(priv)) {
		wl_err("%s P2P mode already exist type: %u not allowed\n",
			   __func__, type);
		return ERR_PTR(-EINVAL);
	}

	mode = type_to_mode(type);
	if (priv->fw_stat[mode] == SPRDWL_INTF_OPEN) {
		if (type == NL80211_IFTYPE_STATION) {
			iftype = NL80211_IFTYPE_AP;
			wl_warn("%s: type %d --> %d\n", __func__, type, iftype);
		}
	}

	return sprdwl_add_iface(priv, name, iftype, params->macaddr);
}
#endif

static int sprdwl_cfg80211_del_iface(struct wiphy *wiphy,
					 struct wireless_dev *wdev)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = NULL, *tmp_vif = NULL;

	if (sprdwl_intf_is_exit(priv)) {
		wiphy_err(wiphy, "%s driver removing!\n", __func__);
		return 0;
	}
	spin_lock_bh(&priv->list_lock);
	list_for_each_entry_safe(vif, tmp_vif, &priv->vif_list, vif_node) {
		if (&vif->wdev == wdev)
			break;
	}
	spin_unlock_bh(&priv->list_lock);

	if (vif != NULL) {
		sprdwl_del_iface(priv, vif);

		spin_lock_bh(&priv->list_lock);
		list_del(&vif->vif_node);
		spin_unlock_bh(&priv->list_lock);
	}

	return 0;
}

static int sprdwl_cfg80211_change_iface(struct wiphy *wiphy,
					struct net_device *ndev,
					enum nl80211_iftype type,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
					u32 *flags,
#endif
					struct vif_params *params)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	enum nl80211_iftype old_type = vif->wdev.iftype;
	int ret;

	wl_ndev_log(L_DBG, ndev, "%s type %d -> %d\n", __func__, old_type, type);

	if (!sprdwl_is_valid_iftype(wiphy, type)) {
		wl_err("%s unsupported interface type: %u\n", __func__, type);
		return -EOPNOTSUPP;
	}

	ret = sprdwl_uninit_fw(vif);
	if (!ret) {
		vif->wdev.iftype = type;
		ret = sprdwl_init_fw(vif);
		if (ret)
			vif->wdev.iftype = old_type;
	}

	return ret;
}

static inline u8 sprdwl_parse_akm(u32 akm)
{
	u8 ret;

	switch (akm) {
	case WLAN_AKM_SUITE_PSK:
		ret = SPRDWL_AKM_SUITE_PSK;
		break;
	case WLAN_AKM_SUITE_8021X:
		ret = SPRDWL_AKM_SUITE_8021X;
		break;
	case WLAN_AKM_SUITE_FT_PSK:
		ret = SPRDWL_AKM_SUITE_FT_PSK;
		break;
	case WLAN_AKM_SUITE_FT_8021X:
		ret = SPRDWL_AKM_SUITE_FT_8021X;
		break;
	case WLAN_AKM_SUITE_WAPI_PSK:
		ret = SPRDWL_AKM_SUITE_WAPI_PSK;
		break;
	case WLAN_AKM_SUITE_WAPI_CERT:
		ret = SPRDWL_AKM_SUITE_WAPI_CERT;
		break;
	case WLAN_AKM_SUITE_PSK_SHA256:
		ret = SPRDWL_AKM_SUITE_PSK_SHA256;
		break;
	case WLAN_AKM_SUITE_8021X_SHA256:
		ret = SPRDWL_AKM_SUITE_8021X_SHA256;
		break;
	default:
		ret = SPRDWL_AKM_SUITE_NONE;
		break;
	}

	return ret;
}

/* Encryption related stuff */
static inline u8 sprdwl_parse_cipher(u32 cipher)
{
	u8 ret;

	switch (cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
		ret = SPRDWL_CIPHER_WEP40;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		ret = SPRDWL_CIPHER_WEP104;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		ret = SPRDWL_CIPHER_TKIP;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		ret = SPRDWL_CIPHER_CCMP;
		break;
	case WLAN_CIPHER_SUITE_SMS4:
		ret = SPRDWL_CIPHER_WAPI;
		break;
	case WLAN_CIPHER_SUITE_AES_CMAC:
		ret = SPRDWL_CIPHER_AES_CMAC;
		break;
	default:
		ret = SPRDWL_CIPHER_NONE;
		break;
	}

	return ret;
}

static int sprdwl_add_cipher_key(struct sprdwl_vif *vif, bool pairwise,
				 u8 key_index, u32 cipher, const u8 *key_seq,
				 const u8 *mac_addr)
{
	u8 *cipher_ptr = pairwise ? &vif->prwise_crypto : &vif->grp_crypto;
	int ret = 0;

	wl_ndev_log(L_DBG, vif->ndev, "%s %s key_index %d\n", __func__,
			pairwise ? "pairwise" : "group", key_index);

	if (vif->key_len[pairwise][0] || vif->key_len[pairwise][1] ||
		vif->key_len[pairwise][2] || vif->key_len[pairwise][3]) {
		*cipher_ptr = vif->prwise_crypto = sprdwl_parse_cipher(cipher);

		ret = sprdwl_add_key(vif->priv, vif->ctx_id,
					 vif->key[pairwise][key_index],
					 vif->key_len[pairwise][key_index],
					 pairwise, key_index, key_seq,
					 *cipher_ptr, mac_addr);
	}

	return ret;
}

static int sprdwl_cfg80211_add_key(struct wiphy *wiphy, struct net_device *ndev,
				   int link_id, u8 key_index, bool pairwise,
				   const u8 *mac_addr,
				   struct key_params *params)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	vif->key_index[pairwise] = key_index;
	vif->key_len[pairwise][key_index] = params->key_len;
	memcpy(vif->key[pairwise][key_index], params->key, params->key_len);

	/* PMK is for Roaming offload */
	if (params->cipher == WLAN_CIPHER_SUITE_PMK)
		return sprdwl_set_roam_offload(vif->priv, vif->ctx_id,
						   SPRDWL_ROAM_OFFLOAD_SET_PMK,
						   params->key, params->key_len);
	else
		return sprdwl_add_cipher_key(vif, pairwise, key_index,
						 params->cipher, params->seq,
						 mac_addr);
}

static int sprdwl_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev,
				   int link_id, u8 key_index, bool pairwise,
				   const u8 *mac_addr)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	//wl_ndev_log(L_DBG, ndev, "%s key_index=%d, pairwise=%d\n",
	//		__func__, key_index, pairwise);

	if (key_index > SPRDWL_MAX_KEY_INDEX) {
		//wl_ndev_log(L_ERR, ndev, "%s key index %d out of bounds!\n", __func__,
		//	   key_index);
		return -ENOENT;
	}

	if (!vif->key_len[pairwise][key_index]) {
		//wl_ndev_log(L_ERR, ndev, "%s key index %d is empty!\n", __func__,
		//	   key_index);
		return 0;
	}

	vif->key_len[pairwise][key_index] = 0;
	vif->prwise_crypto = SPRDWL_CIPHER_NONE;
	vif->grp_crypto = SPRDWL_CIPHER_NONE;

	return sprdwl_del_key(vif->priv, vif->ctx_id, key_index,
				  pairwise, mac_addr);
}

static int sprdwl_cfg80211_set_default_key(struct wiphy *wiphy,
					   struct net_device *ndev,
					   int link_id, u8 key_index, bool unicast,
					   bool multicast)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_debug("%s:enter\n", __func__);
	if (key_index > 3) {
		wl_ndev_log(L_ERR, ndev, "%s invalid key index: %d\n", __func__,
			   key_index);
		return -EINVAL;
	}

	return sprdwl_set_def_key(vif->priv, vif->ctx_id, key_index);
}

static int sprdwl_cfg80211_set_rekey(struct wiphy *wiphy,
					struct net_device *ndev,
					struct cfg80211_gtk_rekey_data *data)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_info("%s:enter:\n", __func__);
	return sprdwl_set_rekey_data(vif->priv, vif->ctx_id, data);
}

/* SoftAP related stuff */
int sprdwl_change_beacon(struct sprdwl_vif *vif,
		struct cfg80211_beacon_data *beacon)
{
	int ret = 0;

	if (!beacon)
		return -EINVAL;

	if (beacon->beacon_ies_len) {
		wl_ndev_log(L_DBG, vif->ndev, "set beacon extra IE\n");
		ret = sprdwl_set_ie(vif->priv, vif->ctx_id, SPRDWL_IE_BEACON,
					beacon->beacon_ies, beacon->beacon_ies_len);
	}

	if (beacon->proberesp_ies_len) {
		wl_ndev_log(L_DBG, vif->ndev, "set probe response extra IE\n");
		ret = sprdwl_set_ie(vif->priv, vif->ctx_id,
					SPRDWL_IE_PROBE_RESP,
					beacon->proberesp_ies,
					beacon->proberesp_ies_len);
	}

	if (beacon->assocresp_ies_len) {
		wl_ndev_log(L_DBG, vif->ndev, "set associate response extra IE\n");
		ret = sprdwl_set_ie(vif->priv, vif->ctx_id,
					SPRDWL_IE_ASSOC_RESP,
					beacon->assocresp_ies,
					beacon->assocresp_ies_len);
	}

	if (ret)
		wl_ndev_log(L_ERR, vif->ndev, "%s failed\n", __func__);

	return ret;
}

static int sprdwl_cfg80211_start_ap(struct wiphy *wiphy,
					struct net_device *ndev,
					struct cfg80211_ap_settings *settings)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_intf *intf = (struct sprdwl_intf *)vif->priv->hw_priv;
	struct cfg80211_beacon_data *beacon = &settings->beacon;
	struct ieee80211_mgmt *mgmt;
	u16 mgmt_len, index = 0, hidden_index;
	u8 *data = NULL;
	int ret;
	u16 chn = 0;
	u8 *head, *tail;
	int head_len, tail_len;

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

#ifdef ACS_SUPPORT
	if ((vif->mode == SPRDWL_MODE_AP) &&
		!list_empty(&vif->survey_info_list)) {
		clean_survey_info_list(vif);
	}
#endif

#ifdef DFS_MASTER
	if (settings->beacon_interval)
		vif->priv->beacon_period = settings->beacon_interval;
#endif

	if (!settings->ssid) {
		wl_ndev_log(L_ERR, ndev, "%s invalid SSID!\n", __func__);
		return -EINVAL;
	}
	strncpy(vif->ssid, settings->ssid, settings->ssid_len);
	vif->ssid_len = settings->ssid_len;

#ifdef STA_SOFTAP_SCC_MODE
	chn = intf->sta_home_channel;
#endif
	if (intf->ini_cfg.softap_channel > 0)
		chn = intf->ini_cfg.softap_channel;

	if (NULL == beacon->head || NULL == beacon->tail) {
		wl_ndev_log(L_ERR, ndev, "%s Beacon info is NULL\n", __func__);
		return -EINVAL;
	}

	head = kmalloc(settings->beacon.head_len + 32, GFP_KERNEL);
	if (head == NULL) {
		wl_ndev_log(L_ERR, ndev, "%s malloc failed!\n", __func__);
		return -ENOMEM;
	}

	tail = kmalloc(settings->beacon.tail_len + 32, GFP_KERNEL);
	if (tail == NULL) {
		wl_ndev_log(L_ERR, ndev, "%s malloc failed!\n", __func__);
		kfree(head);
		return -ENOMEM;
	}

	if (is_valid_channel(wiphy, chn)) {
		head_len = sprdwl_dbg_new_beacon_head(beacon->head,
				beacon->head_len, head, chn);

		tail_len = sprdwl_dbg_new_beacon_tail(beacon->tail,
				beacon->tail_len, tail, chn);
	} else {
		memcpy(head, beacon->head, beacon->head_len);
		memcpy(tail, beacon->tail, beacon->tail_len);
		head_len = beacon->head_len;
		tail_len = beacon->tail_len;
	}

	sprdwl_change_beacon(vif, beacon);

	if (!beacon->head) {
		ret = -EINVAL;
		goto err_start;
	}

	mgmt_len = head_len;
	/* add 1 byte for hidden ssid flag */
	mgmt_len += 1;
	if (settings->hidden_ssid != 0)
		mgmt_len += settings->ssid_len;

	if (beacon->tail)
		mgmt_len += tail_len;

	mgmt = kmalloc(mgmt_len, GFP_KERNEL);
	if (!mgmt) {
		ret = -ENOMEM;
		goto err_start;
	}

	data = (u8 *)mgmt;

#define SSID_LEN_OFFSET		(37)
	memcpy(data, head, SSID_LEN_OFFSET);
	index += SSID_LEN_OFFSET;
	/* modify ssid_len */
	*(data + index) = (u8)(settings->ssid_len + 1);
	index += 1;
	/* copy ssid */
	strncpy(data + index, settings->ssid, settings->ssid_len);
	index += settings->ssid_len;
	/* set hidden ssid flag */
	*(data + index) = (u8)settings->hidden_ssid;
	index += 1;
	/* cope left settings */
	if (settings->hidden_ssid != 0)
		hidden_index = (index - settings->ssid_len);
	else
		hidden_index = index;

	memcpy(data + index, head + hidden_index - 1,
		   head_len + 1 - hidden_index);

	if (beacon->tail)
		memcpy(data + head_len + 1 +
			(settings->hidden_ssid != 0 ? settings->ssid_len : 0),
			   tail, tail_len);

	ret = sprdwl_start_ap(vif->priv, vif->ctx_id, (unsigned char *)mgmt,
				  mgmt_len);
	kfree(mgmt);

#ifdef DFS_MASTER
	if (!netif_carrier_ok(vif->ndev))
		netif_carrier_on(vif->ndev);
	if (netif_queue_stopped(vif->ndev))
		netif_wake_queue(vif->ndev);
#endif
	if (ret)
		wl_ndev_log(L_ERR, ndev, "%s failed to start AP!\n", __func__);
	else
		netif_carrier_on(vif->ndev);

err_start:
	kfree(head);
	kfree(tail);

	return ret;
}

static int sprdwl_cfg80211_change_beacon(struct wiphy *wiphy,
					 struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0))
					 struct cfg80211_ap_update *info)
#else
					 struct cfg80211_beacon_data *beacon)
#endif
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 7, 0))
	struct cfg80211_beacon_data *beacon = &(info->beacon);
#endif

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);
#ifdef DFS_MASTER
	/*send beacon tail ie if needed*/
	if (beacon->tail_len)
		sprdwl_reset_beacon(vif->priv, vif->ctx_id,
				beacon->tail, beacon->tail_len);
	/*enable wifi traffic*/
	if (!netif_carrier_ok(vif->ndev))
		netif_carrier_on(vif->ndev);
	if (netif_queue_stopped(vif->ndev))
		netif_wake_queue(vif->ndev);
#endif

	return sprdwl_change_beacon(vif, beacon);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19, 2))
static int sprdwl_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *ndev, unsigned int link_id)
#else
static int sprdwl_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *ndev)
#endif
{
#ifdef DFS_MASTER
	struct sprdwl_vif *vif = netdev_priv(ndev);
#endif
	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);
#ifdef DFS_MASTER
	sprdwl_abort_cac(vif);
#endif

	netif_carrier_off(ndev);
	return 0;
}

static int sprdwl_cfg80211_add_station(struct wiphy *wiphy,
					   struct net_device *ndev, const u8 *mac,
					   struct station_parameters *params)
{
	return 0;
}

static int sprdwl_cfg80211_del_station(struct wiphy *wiphy,
					   struct net_device *ndev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
					   struct station_del_parameters *params
#else
					   const u8 *mac
#endif
					)

{
	struct sprdwl_vif *vif = netdev_priv(ndev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	if (!params->mac) {
#else
	if (!mac) {
#endif
		wl_ndev_log(L_DBG, ndev, "ignore NULL MAC address!\n");
		goto out;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	wl_ndev_log(L_DBG, ndev, "%s %pM reason:%d\n", __func__, params->mac,
			params->reason_code);
	sprdwl_del_station(vif->priv, vif->ctx_id, params->mac,
			params->reason_code);
#else
	wl_ndev_log(L_DBG, ndev, "%s %pM\n", __func__, mac);
	sprdwl_del_station(vif->priv, vif->ctx_id, mac,
			   WLAN_REASON_DEAUTH_LEAVING);
#endif

	trace_deauth_reason(vif->mode, WLAN_REASON_DEAUTH_LEAVING, LOCAL_EVENT);
out:
	return 0;
}

static int
sprdwl_cfg80211_change_station(struct wiphy *wiphy,
				   struct net_device *ndev, const u8 *mac,
				   struct station_parameters *params)
{
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
static int sprdwl_cfg80211_get_station(struct wiphy *wiphy,
					   struct net_device *ndev, const u8 *mac,
					   struct station_info *sinfo)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_cmd_get_station sta;
	struct sprdwl_rate_info *rate;
	int ret;

	sinfo->filled |= BIT(NL80211_STA_INFO_TX_BYTES) |
			 BIT(NL80211_STA_INFO_TX_PACKETS) |
			 BIT(NL80211_STA_INFO_RX_BYTES) |
			 BIT(NL80211_STA_INFO_RX_PACKETS);
	sinfo->tx_bytes = ndev->stats.tx_bytes;
	sinfo->tx_packets = ndev->stats.tx_packets;
	sinfo->rx_bytes = ndev->stats.rx_bytes;
	sinfo->rx_packets = ndev->stats.rx_packets;

	/* Get current station info */
	ret = sprdwl_get_station(vif->priv, vif->ctx_id,
				 &sta);
	if (ret)
		goto out;
	rate = (struct sprdwl_rate_info *)&sta;

	sinfo->signal = sta.signal;
	sinfo->filled |= BIT(NL80211_STA_INFO_SIGNAL);

	sinfo->tx_failed = sta.txfailed;
	sinfo->filled |= BIT(NL80211_STA_INFO_TX_BITRATE) |
		BIT(NL80211_STA_INFO_TX_FAILED);

	/*fill rate info */
	/*if bit 2,3,4 not set*/
	if (!(rate->flags & 0x1c))
		sinfo->txrate.bw = RATE_INFO_BW_20;

	if ((rate->flags) & BIT(2))
		sinfo->txrate.bw = RATE_INFO_BW_40;

	if ((rate->flags) & BIT(3))
		sinfo->txrate.bw = RATE_INFO_BW_80;

	if ((rate->flags) & BIT(4) ||
		(rate->flags) & BIT(5))
		sinfo->txrate.bw = RATE_INFO_BW_160;

	if ((rate->flags) & BIT(6))
		sinfo->txrate.flags |= RATE_INFO_FLAGS_SHORT_GI;

	if ((rate->flags & RATE_INFO_FLAGS_MCS) ||
		(rate->flags & RATE_INFO_FLAGS_VHT_MCS)) {

		sinfo->txrate.flags = (rate->flags & 0x3);
		sinfo->txrate.mcs = rate->mcs;

		if ((rate->flags & RATE_INFO_FLAGS_VHT_MCS) &&
			(0 != rate->nss)) {
			sinfo->txrate.nss = rate->nss;
		}
	} else {
		sinfo->txrate.legacy = rate->legacy;
	}

	wl_ndev_log(L_DBG, ndev, "%s signal %d legacy %d mcs:%d flags:0x:%x\n",
			__func__, sinfo->signal, sinfo->txrate.legacy,
			rate->mcs, rate->flags);
out:
	return ret;
}

#else
static int sprdwl_cfg80211_get_station(struct wiphy *wiphy,
					   struct net_device *ndev, const u8 *mac,
					   struct station_info *sinfo)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_cmd_get_station sta;
	struct sprdwl_rate_info *rate;
	int ret;

	sinfo->filled |= STATION_INFO_TX_BYTES |
			 STATION_INFO_TX_PACKETS |
			 STATION_INFO_RX_BYTES |
			 STATION_INFO_RX_PACKETS;
	sinfo->tx_bytes = ndev->stats.tx_bytes;
	sinfo->tx_packets = ndev->stats.tx_packets;
	sinfo->rx_bytes = ndev->stats.rx_bytes;
	sinfo->rx_packets = ndev->stats.rx_packets;

	/* Get current station info */
	ret = sprdwl_get_station(vif->priv, vif->ctx_id,
				 &sta);
	if (ret)
		goto out;
	rate = (struct sprdwl_rate_info *)&sta;

	sinfo->signal = sta.signal;
	sinfo->filled |= STATION_INFO_SIGNAL;

	sinfo->tx_failed = sta.txfailed;
	sinfo->filled |= STATION_INFO_TX_BITRATE | STATION_INFO_TX_FAILED;

	/*fill rate info */
	if ((rate->flags & RATE_INFO_FLAGS_MCS) ||
		(rate->flags & RATE_INFO_FLAGS_VHT_MCS)) {

		sinfo->txrate.flags = rate->flags;
		sinfo->txrate.mcs = rate->mcs;

		if ((rate->flags & RATE_INFO_FLAGS_VHT_MCS) &&
			(0 != rate->nss)) {
			sinfo->txrate.nss = rate->nss;
		}
	} else {
		sinfo->txrate.legacy = rate->legacy;
	}

	wl_ndev_log(L_DBG, ndev, "%s signal %d legacy %d mcs:%d flags:0x:%x\n",
			__func__, sinfo->signal, sinfo->txrate.legacy,
			rate->mcs, rate->flags);
out:
	return ret;
}
#endif

static int sprdwl_cfg80211_set_channel(struct wiphy *wiphy,
					   struct net_device *ndev,
					   struct ieee80211_channel *chan)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	return sprdwl_set_channel(vif->priv, vif->ctx_id,
				  ieee80211_frequency_to_channel
				  (chan->center_freq));
}

void sprdwl_report_softap(struct sprdwl_vif *vif, u8 is_connect, u8 *addr,
			  u8 *req_ie, u16 req_ie_len)
{
	struct station_info sinfo;

	/*P2P device is NULL net device,and should return if
	 * vif->ndev is NULL.
	 * */

	if (!addr || !vif->ndev)
		return;

	memset(&sinfo, 0, sizeof(sinfo));
	if (req_ie_len > 0) {
		sinfo.assoc_req_ies = req_ie;
		sinfo.assoc_req_ies_len = req_ie_len;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 83)
		sinfo.filled |= STATION_INFO_ASSOC_REQ_IES;
#endif
	}

	if (is_connect) {
		if (!netif_carrier_ok(vif->ndev)) {
			netif_carrier_on(vif->ndev);
			netif_wake_queue(vif->ndev);
		}
		cfg80211_new_sta(vif->ndev, addr, &sinfo, GFP_KERNEL);
		wl_ndev_log(L_DBG, vif->ndev, "New station (%pM) connected\n", addr);
	} else {
		cfg80211_del_sta(vif->ndev, addr, GFP_KERNEL);
		wl_ndev_log(L_DBG, vif->ndev, "The station (%pM) disconnected\n",
				addr);
		trace_deauth_reason(vif->mode, 0, REMOTE_EVENT);
	}
}

/* Station related stuff */
void sprdwl_cancel_scan(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	struct cfg80211_scan_info info = {
		.aborted = true,
	};
#endif

	wl_info("%s enter==\n", __func__);

	if (priv->scan_vif && priv->scan_vif == vif) {
		if (timer_pending(&priv->scan_timer))
			del_timer_sync(&priv->scan_timer);

		spin_lock_bh(&priv->scan_lock);

		if (priv->scan_request) {
#ifdef ACS_SUPPORT
			if (vif->mode == SPRDWL_MODE_AP)
				transfer_survey_info(vif);
#endif
			/*delete scan node*/
			if (!list_empty(&vif->scan_head_ptr))
				clean_scan_list(vif);

			wl_debug("%s:scan request addr:%p",
					__func__, priv->scan_request);
			if (priv->scan_request->n_channels != 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
				cfg80211_scan_done(priv->scan_request, &info);
#else
				cfg80211_scan_done(priv->scan_request, true);
#endif
			else
				wl_err("%s, %d, error, scan_request freed",
					   __func__, __LINE__);
			priv->scan_request = NULL;
			priv->scan_vif = NULL;
		}
		spin_unlock_bh(&priv->scan_lock);
	}

#if 0 /* Avoid set assert during hang recovery */
	wlan_set_assert(vif->priv, vif->ctx_id, WIFI_CMD_SCAN, SCAN_ERROR);
#endif
}

void sprdwl_cancel_sched_scan(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;

	if (priv->sched_scan_vif && priv->sched_scan_vif == vif) {
		spin_lock_bh(&priv->sched_scan_lock);
		if (priv->sched_scan_request) {
			priv->sched_scan_request = NULL;
			priv->sched_scan_vif = NULL;
		}
		spin_unlock_bh(&priv->sched_scan_lock);
	}
}

void sprdwl_scan_done(struct sprdwl_vif *vif, bool abort)
{
	struct sprdwl_priv *priv = vif->priv;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	struct cfg80211_scan_info info = {
		.aborted = abort,
	};
#endif

	if (priv->scan_vif && priv->scan_vif == vif) {
		if (timer_pending(&priv->scan_timer))
			del_timer_sync(&priv->scan_timer);

		spin_lock_bh(&priv->scan_lock);
		if (priv->scan_request) {
#ifdef ACS_SUPPORT
			if (vif->mode == SPRDWL_MODE_AP)
				transfer_survey_info(vif);
#endif
			/*delete scan node*/
			if (!list_empty(&vif->scan_head_ptr))
				clean_scan_list(vif);

			wl_debug("%s:scan request addr:%p",
					__func__, priv->scan_request);
			if (priv->scan_request->n_channels != 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
				wl_ndev_log(L_DBG, vif->ndev, "%s scan is %s\n", __func__, abort ? "Aborted" : "Done");
				cfg80211_scan_done(priv->scan_request, &info);
#else
				cfg80211_scan_done(priv->scan_request, abort);
#endif
			} else {
				wl_err("%s, %d, error, scan_request freed",
					   __func__, __LINE__);
			}
			priv->scan_request = NULL;
			priv->scan_vif = NULL;
		}
		spin_unlock_bh(&priv->scan_lock);
	}
}

void sprdwl_sched_scan_done(struct sprdwl_vif *vif, bool abort)
{
	struct sprdwl_priv *priv = vif->priv;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	u64 reqid = 0;
#endif

	if (priv->sched_scan_vif && priv->sched_scan_vif == vif) {
		spin_lock_bh(&priv->sched_scan_lock);
		if (priv->sched_scan_request) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
			cfg80211_sched_scan_results(vif->wdev.wiphy, reqid);
#else
			cfg80211_sched_scan_results(vif->wdev.wiphy);
#endif
			wl_ndev_log(L_DBG, priv->sched_scan_vif->ndev,
					"%s report result\n", __func__);
			priv->sched_scan_request = NULL;
			priv->sched_scan_vif = NULL;
		}
		spin_unlock_bh(&priv->sched_scan_lock);
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
void sprdwl_scan_timeout(struct timer_list *t)
{
	struct sprdwl_priv *priv = from_timer(priv, t, scan_timer);
#else
void sprdwl_scan_timeout(unsigned long data)
{
	struct sprdwl_priv *priv = (struct sprdwl_priv *)data;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	struct cfg80211_scan_info info = {
		.aborted = true,
	};
#endif
	wl_ndev_log(L_DBG, priv->scan_vif->ndev, "%s\n", __func__);

	spin_lock_bh(&priv->scan_lock);
	if (priv->scan_request) {
#ifdef ACS_SUPPORT
		clean_survey_info_list(priv->scan_vif);
#endif /* ACS_SUPPORT */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		cfg80211_scan_done(priv->scan_request, &info);
#else
		cfg80211_scan_done(priv->scan_request, true);
#endif
		priv->scan_vif = NULL;
		priv->scan_request = NULL;
	}
	spin_unlock_bh(&priv->scan_lock);
}

static int sprdwl_cfg80211_scan(struct wiphy *wiphy,
				struct cfg80211_scan_request *request)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif =
		container_of(request->wdev, struct sprdwl_vif, wdev);
	struct cfg80211_ssid *ssids = request->ssids;
	struct sprdwl_scan_ssid *scan_ssids;
	u8 *ssids_ptr = NULL;
	int scan_ssids_len = 0;
	u32 channels = 0;
	unsigned int i, n;
	int ret;
	u16 n_5g_chn = 0, chns_5g[64];

#ifdef RND_MAC_SUPPORT
	u32 flags = request->flags;
	static int random_mac_set;
	int random_mac_flag;
	static int old_mac_flag;
#endif
	struct sprdwl_intf *intf;
	intf = (struct sprdwl_intf *)(priv->hw_priv);

#ifndef CP2_RESET_SUPPORT
	if (intf->cp_asserted)
		return -EIO;
#else
	if (intf->cp_asserted || priv->sync.scan_not_allowed)
		return -EIO;
#endif

	wl_ndev_log(L_DBG, vif->ndev, "%s n_channels %u\n", __func__,
			request->n_channels);

	if (!sprdwl_is_valid_iftype(wiphy, request->wdev->iftype)) {
		wl_err("%s unsupported interface type: %u\n",
			   __func__, request->wdev->iftype);
		ret = -EOPNOTSUPP;
		goto err;
	}

	if (priv->scan_request)
		wl_ndev_log(L_ERR, vif->ndev, "%s error scan %p running [%p, %p]\n",
			   __func__, priv->scan_request, priv->scan_vif, vif);

#ifdef RND_MAC_SUPPORT
	if (vif->mode == SPRDWL_MODE_STATION) {
		if (!random_mac_set) {
			random_mac_addr(rand_addr);
			random_mac_set = SPRDWL_ENABLE_SCAN_RANDOM_ADDR;
		}
		if (flags & (1<<3)) {
			random_mac_flag = 1;
			wl_info("Random MAC support==set value:%d\n",
				   random_mac_set);
			wl_info("random mac addr: %pM\n", rand_addr);
		} else {
			wl_info("random mac feature disabled\n");
			random_mac_flag = SPRDWL_DISABLE_SCAN_RANDOM_ADDR;
		}
		if (random_mac_flag != old_mac_flag) {
			old_mac_flag = random_mac_flag;
			wlan_cmd_set_rand_mac(vif->priv, vif->ctx_id,
						  old_mac_flag, rand_addr);
		}
	}
#endif

	/* set WPS ie */
	if (request->ie_len > 0) {
		if (request->ie_len > 255) {
			wl_ndev_log(L_ERR, vif->ndev, "%s invalid len: %zu\n", __func__,
				   request->ie_len);
			ret = -EOPNOTSUPP;
			goto err;
		}

		ret = sprdwl_set_ie(priv, vif->ctx_id, SPRDWL_IE_PROBE_REQ,
					request->ie, request->ie_len);
		if (ret)
			goto err;
	}

	for (i = 0; i < request->n_channels; i++) {
		switch (request->channels[i]->hw_value) {
		case 0:
			break;

		case 1 ... 14:
			channels |= (1 << (request->channels[i]->hw_value - 1));
			break;

		default:
			if (n_5g_chn > ARRAY_SIZE(chns_5g))
				break;
			chns_5g[n_5g_chn] = request->channels[i]->hw_value;
			n_5g_chn++;
			break;
		}
#ifdef ACS_SUPPORT
		if (vif->mode == SPRDWL_MODE_AP) {
			struct sprdwl_survey_info *info = NULL;

			if ((0 == i) && (!list_empty(&vif->survey_info_list))) {
				wl_ndev_log(L_ERR, vif->ndev,
					   "%s survey info list is not empty!\n",
					   __func__);
				clean_survey_info_list(vif);
			}

			info = kmalloc(sizeof(*info), GFP_KERNEL);
			if (!info) {
				ret = -ENOMEM;
				goto err;
			}

			INIT_LIST_HEAD(&info->bssid_list);
			info->chan = request->channels[i]->hw_value;
			info->beacon_num = 0;
			info->channel = NULL;
			list_add_tail(&info->survey_list,
					  &vif->survey_info_list);
		}
#endif /* ACS_SUPPORT */
	}

	n = min(request->n_ssids, 9);
	if (n) {
		ssids_ptr = kzalloc(512, GFP_KERNEL);
		if (!ssids_ptr) {
			wl_ndev_log(L_ERR, vif->ndev,
				   "%s failed to alloc scan ssids!\n",
				   __func__);
			ret = -ENOMEM;
			goto err;
		}

		scan_ssids = (struct sprdwl_scan_ssid *)ssids_ptr;
		for (i = 0; i < n; i++) {
			if (!ssids[i].ssid_len)
				continue;
			scan_ssids->len = ssids[i].ssid_len;
			strncpy(scan_ssids->ssid, ssids[i].ssid,
				ssids[i].ssid_len);
			scan_ssids_len += (ssids[i].ssid_len
					   + sizeof(scan_ssids->len));
			scan_ssids = (struct sprdwl_scan_ssid *)
				(ssids_ptr + scan_ssids_len);
		}
	} else {
#ifndef ACS_SUPPORT
		wl_ndev_log(L_ERR, vif->ndev, "%s n_ssids is 0\n", __func__);
		ret = -EINVAL;
		goto err;
#endif /* ACS_SUPPORT */
	}

	/*init scan list*/
	init_scan_list(vif);

	spin_lock_bh(&priv->scan_lock);
	priv->scan_request = request;
	priv->scan_vif = vif;
	spin_unlock_bh(&priv->scan_lock);
	wl_debug("%s:scan request addr:%p", __func__, request);
	mod_timer(&priv->scan_timer,
		  jiffies + SPRDWL_SCAN_TIMEOUT_MS * HZ / 1000);

	ret = sprdwl_scan(vif->priv, vif->ctx_id, channels,
			  scan_ssids_len, ssids_ptr, n_5g_chn, chns_5g);
	kfree(ssids_ptr);
	if (ret) {
		sprdwl_cancel_scan(vif);
		goto err;
	}

	return 0;
err:
	wl_ndev_log(L_ERR, vif->ndev, "%s failed (%d)\n", __func__, ret);
	return ret;
}

static int sprdwl_cfg80211_sched_scan_start(struct wiphy *wiphy,
						struct net_device *ndev,
						struct cfg80211_sched_scan_request
						*request)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	struct cfg80211_sched_scan_plan *scan_plans = NULL;
#endif
	struct sprdwl_sched_scan_buf *sscan_buf = NULL;
	struct sprdwl_vif *vif = NULL;
	struct cfg80211_ssid *ssid_tmp = NULL;
	struct cfg80211_match_set *match_ssid_tmp = NULL;
	int ret = 0;
	int i = 0, j = 0;
	s32 min_rssi_thold;

	if (!ndev) {
		wl_ndev_log(L_ERR, ndev, "%s NULL ndev\n", __func__);
		return ret;
	}
	vif = netdev_priv(ndev);
	/*scan not allowed if closed*/
	if (vif->priv->fw_stat[vif->mode] == SPRDWL_INTF_CLOSE) {
		wl_err("%s, %d, error!mode%d scan after closed not allowed\n",
			   __func__, __LINE__, vif->mode);
		return -ENOMEM;
	}

	if (vif->priv->sched_scan_request) {
		wl_ndev_log(L_ERR, ndev, "%s  schedule scan is running\n", __func__);
		return 0;
	}
	/*to protect the size of struct sprdwl_sched_scan_buf*/
	if (request->n_channels > TOTAL_2G_5G_CHANNEL_NUM) {
		wl_err("%s, %d, error! request->n_channels=%d\n",
			   __func__, __LINE__, request->n_channels);
		request->n_channels = TOTAL_2G_5G_CHANNEL_NUM;
	}
	if (request->n_ssids > TOTAL_2G_5G_SSID_NUM) {
		wl_err("%s, %d, error! request->n_ssids=%d\n",
			   __func__, __LINE__, request->n_ssids);
		request->n_ssids = TOTAL_2G_5G_SSID_NUM;
	}
	if (request->n_match_sets > TOTAL_2G_5G_SSID_NUM) {
		wl_err("%s, %d, error! request->n_match_sets=%d\n",
			   __func__, __LINE__, request->n_match_sets);
		request->n_match_sets = TOTAL_2G_5G_SSID_NUM;
	}
	sscan_buf = kzalloc(sizeof(*sscan_buf), GFP_KERNEL);
	if (!sscan_buf)
		return -ENOMEM;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
	scan_plans = request->scan_plans;
	sscan_buf->interval = scan_plans->interval;
#else
	sscan_buf->interval = DIV_ROUND_UP(request->interval, MSEC_PER_SEC);
#endif
	sscan_buf->flags = request->flags;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
	min_rssi_thold = request->min_rssi_thold;
#else
	min_rssi_thold = request->rssi_thold;
#endif
	if (min_rssi_thold <= NL80211_SCAN_RSSI_THOLD_OFF)
		sscan_buf->rssi_thold = 0;
	else if (min_rssi_thold < -127)
		sscan_buf->rssi_thold = -127;
	else
		sscan_buf->rssi_thold = min_rssi_thold;

	for (i = 0, j = 0; i < request->n_channels; i++) {
		int ch = request->channels[i]->hw_value;

		if (ch == 0) {
			wl_ndev_log(L_DBG, ndev, "%s  unknown frequency %dMhz\n",
					__func__,
					request->channels[i]->center_freq);
			continue;
		}

		wl_ndev_log(L_DBG, ndev, "%s: channel is %d\n", __func__, ch);
		sscan_buf->channel[j + 1] = ch;
		j++;
	}
	sscan_buf->channel[0] = j;

	if (request->ssids && request->n_ssids > 0) {
		sscan_buf->n_ssids = request->n_ssids;

		for (i = 0; i < request->n_ssids; i++) {
			ssid_tmp = request->ssids + i;
			sscan_buf->ssid[i] = ssid_tmp->ssid;
		}
	}

	if (request->match_sets && request->n_match_sets > 0) {
		sscan_buf->n_match_ssids = request->n_match_sets;

		for (i = 0; i < request->n_match_sets; i++) {
			match_ssid_tmp = request->match_sets + i;
			sscan_buf->mssid[i] = match_ssid_tmp->ssid.ssid;
		}
	}
	sscan_buf->ie_len = request->ie_len;
	sscan_buf->ie = request->ie;

	spin_lock_bh(&priv->sched_scan_lock);
	vif->priv->sched_scan_request = request;
	vif->priv->sched_scan_vif = vif;
	spin_unlock_bh(&priv->sched_scan_lock);

	ret = sprdwl_sched_scan_start(priv, vif->ctx_id, sscan_buf);
	if (ret)
		sprdwl_cancel_sched_scan(vif);

	kfree(sscan_buf);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static int sprdwl_cfg80211_sched_scan_stop(struct wiphy *wiphy,
					   struct net_device *ndev, u64 reqid)
{
#else
static int sprdwl_cfg80211_sched_scan_stop(struct wiphy *wiphy,
					   struct net_device *ndev)
{
#endif
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_vif *vif = NULL;
	int ret = 0;

	if (!ndev) {
		wl_ndev_log(L_ERR, ndev, "%s NULL ndev\n", __func__);
		return ret;
	}
	vif = netdev_priv(ndev);
	ret = sprdwl_sched_scan_stop(priv, vif->ctx_id);
	if (!ret) {
		spin_lock_bh(&priv->sched_scan_lock);
		vif->priv->sched_scan_request = NULL;
		vif->priv->sched_scan_vif = NULL;
		spin_unlock_bh(&priv->sched_scan_lock);
	} else {
		wl_ndev_log(L_ERR, ndev, "%s  scan stop failed\n", __func__);
	}
	return ret;
}

#ifdef SYNC_DISCONNECT
void sprdwl_disconnect_handle(struct sprdwl_vif *vif)
{
	vif->sm_state = SPRDWL_DISCONNECTED;

	/* Clear bssid & ssid */
	memset(vif->bssid, 0, sizeof(vif->bssid));
	memset(vif->ssid, 0, sizeof(vif->ssid));
#ifdef WMMAC_WFA_CERTIFICATION
	reset_wmmac_parameters(vif->priv);
	reset_wmmac_ts_info();
	init_default_qos_map();
#endif
	/* Stop netif */
	if (netif_carrier_ok(vif->ndev)) {
		netif_carrier_off(vif->ndev);
		netif_stop_queue(vif->ndev);
	}

	/*clear link layer status data*/
	memset(&vif->priv->pre_radio, 0, sizeof(vif->priv->pre_radio));
}
#endif
static int sprdwl_cfg80211_disconnect(struct wiphy *wiphy,
					  struct net_device *ndev, u16 reason_code)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	enum sm_state old_state = vif->sm_state;
	int ret;
#ifdef SYNC_DISCONNECT
	u32 msec;
	ktime_t kt;
#endif
#ifdef STA_SOFTAP_SCC_MODE
	struct sprdwl_intf *intf = (struct sprdwl_intf *)vif->priv->hw_priv;
	intf->sta_home_channel = 0;
#endif

	wl_ndev_log(L_DBG, ndev, "%s %s reason: %d\n", __func__, vif->ssid,
			reason_code);

	vif->sm_state = SPRDWL_DISCONNECTING;

#ifdef SYNC_DISCONNECT
	atomic_set(&vif->sync_disconnect_event, 1);
#endif
	ret = sprdwl_disconnect(vif->priv, vif->ctx_id, reason_code);
	if (ret) {
		vif->sm_state = old_state;
		goto out;
	}
#ifdef SYNC_DISCONNECT
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 60)
	if (!sprdwl_sync_disconnect_event(vif, msecs_to_jiffies(1000))) {
		kt = ktime_get();
		msec = (u32)(div_u64(kt.tv64, NSEC_PER_MSEC));
		wl_err("Wait disconnect event timeout. [mstime = %d]\n",
		       cpu_to_le32(msec));
	} else {
		sprdwl_disconnect_handle(vif);
	}
	atomic_set(&vif->sync_disconnect_event, 0);
#endif
#endif
	trace_deauth_reason(vif->mode, reason_code, LOCAL_EVENT);
out:
	return ret;
}

static int sprdwl_cfg80211_connect(struct wiphy *wiphy, struct net_device *ndev,
				   struct cfg80211_connect_params *sme)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
#ifdef STA_SOFTAP_SCC_MODE
	struct sprdwl_intf *intf = (struct sprdwl_intf *)vif->priv->hw_priv;
#endif
	struct sprdwl_cmd_connect con;
	enum sm_state old_state = vif->sm_state;
	int is_wep = (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40) ||
		(sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104);
	int random_mac_flag;
	int ret = -EPERM;

	/*workround for bug 795430*/
	if (vif->priv->fw_stat[vif->mode] == SPRDWL_INTF_CLOSE) {
		wl_err("%s, %d, error!mode%d connect after closed not allowed",
			   __func__, __LINE__, vif->mode);
		goto err;
	}

	if (vif->mode == SPRDWL_MODE_STATION) {
		if (vif->has_rand_mac) {
			 random_mac_flag = SPRDWL_CONNECT_RANDOM_ADDR;
			 ret = wlan_cmd_set_rand_mac(vif->priv, vif->ctx_id,
						   random_mac_flag, vif->random_mac);
			 if (ret)
				 netdev_info(ndev, "Set random mac failed!\n");
		}
	}

	memset(&con, 0, sizeof(con));

	/*workround for bug 771600*/
	if (vif->sm_state == SPRDWL_CONNECTING) {
		wl_ndev_log(L_DBG, ndev, "sm_state is SPRDWL_CONNECTING, disconnect first\n");
		sprdwl_cfg80211_disconnect(wiphy, ndev, 3);
	}

	/* Set wps ie */
	if (sme->ie_len > 0) {
		wl_ndev_log(L_DBG, ndev, "set assoc req ie, len %zx\n", sme->ie_len);
		ret = sprdwl_set_ie(vif->priv, vif->ctx_id, SPRDWL_IE_ASSOC_REQ,
					sme->ie, sme->ie_len);
		if (ret)
			goto err;
	}

	wl_ndev_log(L_DBG, ndev, "wpa versions %#x\n", sme->crypto.wpa_versions);
	con.wpa_versions = sme->crypto.wpa_versions;
	wl_ndev_log(L_DBG, ndev, "management frame protection %#x\n", sme->mfp);
	con.mfp_enable = sme->mfp;

	wl_ndev_log(L_DBG, ndev, "auth type %#x\n", sme->auth_type);
	if ((sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM) ||
		((sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) && !is_wep))
		con.auth_type = SPRDWL_AUTH_OPEN;
	else if ((sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY) ||
		 ((sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) && is_wep))
		con.auth_type = SPRDWL_AUTH_SHARED;

	/* Set pairewise cipher */
	if (sme->crypto.n_ciphers_pairwise) {
		vif->prwise_crypto =
			sprdwl_parse_cipher(sme->crypto.ciphers_pairwise[0]);

		if (vif->prwise_crypto != SPRDWL_CIPHER_NONE) {
			wl_ndev_log(L_DBG, ndev, "pairwise cipher %#x\n",
					sme->crypto.ciphers_pairwise[0]);
			con.pairwise_cipher = vif->prwise_crypto;
			con.pairwise_cipher |= SPRDWL_VALID_CONFIG;
		}
	} else {
		wl_ndev_log(L_DBG, ndev, "No pairewise cipher specified!\n");
		vif->prwise_crypto = SPRDWL_CIPHER_NONE;
	}

	/* Set group cipher */
	vif->grp_crypto = sprdwl_parse_cipher(sme->crypto.cipher_group);
	if (vif->grp_crypto != SPRDWL_CIPHER_NONE) {
		wl_ndev_log(L_DBG, ndev, "group cipher %#x\n",
				sme->crypto.cipher_group);
		con.group_cipher = vif->grp_crypto;
		con.group_cipher |= SPRDWL_VALID_CONFIG;
	}

	/* Set auth key management (akm) */
	if (sme->crypto.n_akm_suites) {
		wl_ndev_log(L_DBG, ndev, "akm suites %#x\n",
				sme->crypto.akm_suites[0]);
		con.key_mgmt = sprdwl_parse_akm(sme->crypto.akm_suites[0]);
		con.key_mgmt |= SPRDWL_VALID_CONFIG;
	} else {
		wl_ndev_log(L_DBG, ndev, "No akm suites specified!\n");
	}

	/* Set PSK */
	if (sme->key_len) {
		if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40 ||
			sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104 ||
			sme->crypto.ciphers_pairwise[0] ==
			WLAN_CIPHER_SUITE_WEP40 ||
			sme->crypto.ciphers_pairwise[0] ==
			WLAN_CIPHER_SUITE_WEP104) {
			vif->key_index[SPRDWL_GROUP] = sme->key_idx;
			vif->key_len[SPRDWL_GROUP][sme->key_idx] = sme->key_len;
			memcpy(vif->key[SPRDWL_GROUP][sme->key_idx], sme->key,
				   sme->key_len);
			ret =
				sprdwl_add_cipher_key(vif, 0, sme->key_idx,
						  sme->crypto.
						  ciphers_pairwise[0],
						  NULL, NULL);
			if (ret)
				goto err;
		} else if (sme->key_len > WLAN_MAX_KEY_LEN) {
			wl_ndev_log(L_ERR, ndev, "%s invalid key len: %d\n", __func__,
				   sme->key_len);
			ret = -EINVAL;
			goto err;
		} else {
			wl_ndev_log(L_DBG, ndev, "PSK %s\n", sme->key);
			con.psk_len = sme->key_len;
			memcpy(con.psk, sme->key, sme->key_len);
		}
	}

	/* Auth RX unencrypted EAPOL is not implemented, do nothing */
	/* Set channel */
#ifdef STA_SOFTAP_SCC_MODE
	intf->sta_home_channel = 0;
#endif
	if (!sme->channel) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
		if (sme->channel_hint) {
			u16 center_freq = sme->channel_hint->center_freq;

			con.channel =
				ieee80211_frequency_to_channel(center_freq);
			wl_ndev_log(L_DBG, ndev, "channel_hint %d\n", con.channel);
#ifdef STA_SOFTAP_SCC_MODE
			if (sme->channel_hint->flags != IEEE80211_CHAN_RADAR)
				intf->sta_home_channel = con.channel;
#endif
		} else
#endif
		{
			wl_ndev_log(L_DBG, ndev, "No channel specified!\n");
		}
	} else {
		con.channel =
			ieee80211_frequency_to_channel(sme->channel->center_freq);
		wl_ndev_log(L_DBG, ndev, "channel %d\n", con.channel);
#ifdef STA_SOFTAP_SCC_MODE
		if (sme->channel->flags != IEEE80211_CHAN_RADAR)
			intf->sta_home_channel = con.channel;
#endif
	}

	/* Set BSSID */
	if (sme->bssid != NULL) {
		memcpy(con.bssid, sme->bssid, sizeof(con.bssid));
		memcpy(vif->bssid, sme->bssid, sizeof(vif->bssid));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
	} else if (sme->bssid_hint != NULL) {
		memcpy(con.bssid, sme->bssid_hint, sizeof(con.bssid));
		memcpy(vif->bssid, sme->bssid_hint, sizeof(vif->bssid));
#endif
	} else {
		wl_ndev_log(L_DBG, ndev, "No BSSID specified!\n");
	}

	/* Special process for WEP(WEP key must be set before essid) */
	if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40 ||
		sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104) {
		wl_ndev_log(L_DBG, ndev, "%s WEP cipher_group\n", __func__);

		if (sme->key_len <= 0) {
			wl_ndev_log(L_DBG, ndev, "No key specified!\n");
		} else {
			if (sme->key_len != WLAN_KEY_LEN_WEP104 &&
				sme->key_len != WLAN_KEY_LEN_WEP40) {
				wl_ndev_log(L_ERR, ndev, "%s invalid WEP key length!\n",
					   __func__);
				ret = -EINVAL;
				goto err;
			}

			sprdwl_set_def_key(vif->priv, vif->ctx_id,
					   sme->key_idx);
			if (ret)
				goto err;
		}
	}

	/* Set ESSID */
	if (!sme->ssid) {
		wl_ndev_log(L_DBG, ndev, "No SSID specified!\n");
	} else {
		strncpy(con.ssid, sme->ssid, sme->ssid_len);
		con.ssid_len = sme->ssid_len;
		vif->sm_state = SPRDWL_CONNECTING;

		if (vif->wps_flag) {
			if (strstr(con.ssid, "Marvell") || strstr(con.ssid, "Ralink")) {
				wl_info("%s, WPS connection\n", __func__);
				msleep(3000);
			}
			vif->wps_flag = 0;
		}

		ret = sprdwl_connect(vif->priv, vif->ctx_id, &con);
		if (ret)
			goto err;
		strncpy(vif->ssid, sme->ssid, sme->ssid_len);
		vif->ssid_len = sme->ssid_len;
		wl_ndev_log(L_DBG, ndev, "%s %s\n", __func__, vif->ssid);
	}

	return 0;
err:
	wl_ndev_log(L_ERR, ndev, "%s failed\n", __func__);
	vif->sm_state = old_state;
#ifdef STA_SOFTAP_SCC_MODE
	intf->sta_home_channel = 0;
#endif
	return ret;
}

static int sprdwl_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	u32 rts = 0, frag = 0;

	if (changed & WIPHY_PARAM_RTS_THRESHOLD)
		rts = wiphy->rts_threshold;

	if (changed & WIPHY_PARAM_FRAG_THRESHOLD)
		frag = wiphy->frag_threshold;

	return sprdwl_set_param(priv, rts, frag);
}

static int sprdwl_cfg80211_set_pmksa(struct wiphy *wiphy,
					 struct net_device *ndev,
					 struct cfg80211_pmksa *pmksa)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_pmksa(vif->priv, vif->ctx_id, pmksa->bssid,
				pmksa->pmkid, SPRDWL_SUBCMD_SET);
}

static int sprdwl_cfg80211_del_pmksa(struct wiphy *wiphy,
					 struct net_device *ndev,
					 struct cfg80211_pmksa *pmksa)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_pmksa(vif->priv, vif->ctx_id, pmksa->bssid,
				pmksa->pmkid, SPRDWL_SUBCMD_DEL);
}

static int sprdwl_cfg80211_flush_pmksa(struct wiphy *wiphy,
					   struct net_device *ndev)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_pmksa(vif->priv, vif->ctx_id, vif->bssid, NULL,
				SPRDWL_SUBCMD_FLUSH);
}

void sprdwl_report_fake_probe(struct wiphy *wiphy, u8 *ie, size_t ielen)
{
	static int local_mac_ind, flush_count;
	struct ieee80211_channel *channel;
	struct cfg80211_bss *bss;
	char fake_ssid[IEEE80211_MAX_SSID_LEN] = "&%^#!%&&?@#$&@3iU@Code1";
	static char fake_ie[SPRDWL_MAX_IE_LEN];
	char fake_bssid[6] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
	static u16 fake_ielen;
	u16 capability, beacon_interval;
	u32 freq;
	s32 signal;

	if (0 == local_mac_ind) {
		if ((ielen+IEEE80211_MAX_SSID_LEN) < SPRDWL_MAX_IE_LEN) {
			/*add SSID IE*/
			ie = ie + *(ie+1) + 2;
			/*total IE length sub SSID IE;*/
			ielen = ielen - *(ie+1) - 2;
			/*fill in new SSID element*/
			*fake_ie = 0;
			/*set SSID IE length*/
			*(fake_ie+1) = strlen(fake_ssid);
			/*fill resp IE with fake ssid*/
			memcpy((fake_ie+2), fake_ssid, strlen(fake_ssid));
			/*fill resp IE with other IE */
			memcpy((fake_ie+2+strlen(fake_ssid)), ie, ielen);
			fake_ielen = ielen + 2 + strlen(fake_ssid);
			local_mac_ind = 1;
		} else {
			return;
		}
	}
	if (0 == ((flush_count++)%5)) {
		freq = 2412;
		capability = 0x2D31;
		beacon_interval = 100;
		signal = -20;
		channel = ieee80211_get_channel(wiphy, freq);
		bss = cfg80211_inform_bss(wiphy, channel,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
					  CFG80211_BSS_FTYPE_UNKNOWN,
#endif
					  fake_bssid, 0, capability,
					  beacon_interval, fake_ie,
					  fake_ielen, signal,
					  GFP_KERNEL);
		cfg80211_put_bss(wiphy, bss);
	}
}

void signal_level_enhance(struct sprdwl_vif *vif,
			  struct ieee80211_mgmt *mgmt, s32 *signal)
{
	struct scan_result *scan_node;
	struct sprdwl_priv *priv = vif->priv;

	if (!priv->scan_vif || priv->scan_vif != vif)
		return;
	spin_lock_bh(&priv->scan_lock);
	/*check whether there is a same bssid & ssid*/
	if (priv->scan_request && !list_empty(&vif->scan_head_ptr)) {
		list_for_each_entry(scan_node, &vif->scan_head_ptr, list) {
			if (!memcmp(scan_node->bssid, mgmt->bssid, ETH_ALEN)) {
				/*if found,compare signal and decide
				* whether to replae it with a better one
				*/
				if (scan_node->signal > *signal)
					*signal = scan_node->signal;
				else
					scan_node->signal = *signal;
				goto unlock;
			}
		}
	}
	/*if didn't found,create a node*/
	scan_node = kmalloc(sizeof(*scan_node), GFP_ATOMIC);
	if (!scan_node)
		goto unlock;
	scan_node->signal = *signal;
	memcpy(scan_node->bssid, mgmt->bssid, 6);
	list_add_tail(&scan_node->list, &vif->scan_head_ptr);

unlock:
	spin_unlock_bh(&priv->scan_lock);
}

void sprdwl_report_scan_result(struct sprdwl_vif *vif, u16 chan, s16 rssi,
				   u8 *frame, u16 len)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)frame;
	struct ieee80211_channel *channel;
	struct cfg80211_bss *bss;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
	struct timespec ts;
#else
	struct timespec64 ts;
#endif
	u16 capability, beacon_interval;
	u32 freq;
	s32 signal;
	u64 tsf;
	u8 *ie;
	size_t ielen;

	if (!priv->scan_request && !priv->sched_scan_request) {
		wl_ndev_log(L_DBG, vif->ndev, "%s Unexpected event\n", __func__);
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	freq = ieee80211_channel_to_frequency(chan, chan <= CH_MAX_2G_CHANNEL ?
			NL80211_BAND_2GHZ : NL80211_BAND_5GHZ);
#else
	freq = ieee80211_channel_to_frequency(chan, chan <= CH_MAX_2G_CHANNEL ?
			IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ);
#endif
	channel = ieee80211_get_channel(wiphy, freq);
	if (!channel) {
		wl_ndev_log(L_ERR, vif->ndev, "%s invalid freq!\n", __func__);
		return;
	}

	if (!mgmt) {
		wl_ndev_log(L_ERR, vif->ndev, "%s NULL frame!\n", __func__);
		return;
	}

	signal = rssi * 100;
	/*signal level enhance*/
	signal_level_enhance(vif, mgmt, &signal);
	/*if signal has been update & enhanced*/

	if ((rssi * 100) != signal)
		wl_debug("old signal level:%d,new signal level:%d\n",
			   (rssi*100), signal);

#ifdef ACS_SUPPORT
	if (vif->mode == SPRDWL_MODE_AP)
		acs_scan_result(vif, chan, mgmt);
#endif

	ie = mgmt->u.probe_resp.variable;
	ielen = len - offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
	/* framework use system bootup time */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
	get_monotonic_boottime(&ts);
#else
	ktime_get_boottime_ts64(&ts);
#endif
	tsf = (u64)ts.tv_sec * 1000000 + div_u64(ts.tv_nsec, 1000);
	beacon_interval = le16_to_cpu(mgmt->u.probe_resp.beacon_int);
	capability = le16_to_cpu(mgmt->u.probe_resp.capab_info);

	wl_ndev_log(L_DBG, vif->ndev, "   %s, %pM, channel %2u, signal %d\n",
			ieee80211_is_probe_resp(mgmt->frame_control)
			? "proberesp" : "beacon   ", mgmt->bssid, chan, signal);

	bss = cfg80211_inform_bss(wiphy, channel,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
				  CFG80211_BSS_FTYPE_UNKNOWN,
#endif
				  mgmt->bssid, tsf, capability, beacon_interval,
				  ie, ielen, signal, GFP_KERNEL);

	if (unlikely(!bss))
		wl_ndev_log(L_ERR, vif->ndev,
			   "%s failed to inform bss frame!\n", __func__);
	cfg80211_put_bss(wiphy, bss);

	/*check log mac flag and call report fake probe*/
	if (vif->local_mac_flag)
		sprdwl_report_fake_probe(wiphy, ie, ielen);

	if (vif->beacon_loss) {
		bss = cfg80211_get_bss(wiphy, NULL, vif->bssid,
					   vif->ssid, vif->ssid_len,
					   WLAN_CAPABILITY_ESS,
					   WLAN_CAPABILITY_ESS);
		if (bss) {
			cfg80211_unlink_bss(wiphy, bss);
			wl_ndev_log(L_DBG, vif->ndev,
					"unlink %pM due to beacon loss\n",
					bss->bssid);
			vif->beacon_loss = 0;
		}
	}
}

void sprdwl_report_connection(struct sprdwl_vif *vif,
					struct sprdwl_connect_info *conn_info,
					u8 status_code)
{
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;
	struct ieee80211_channel *channel;
	struct ieee80211_mgmt *mgmt;
	struct cfg80211_bss *bss = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
	struct timespec ts;
#else
	struct timespec64 ts;
#endif
#ifdef WMMAC_WFA_CERTIFICATION
	struct wmm_params_element *wmm_params;
	int i;
#endif
	u16 capability, beacon_interval;
	u32 freq;
	u64 tsf;
	u8 *ie;
	size_t ielen;
#ifdef STA_SOFTAP_SCC_MODE
	struct sprdwl_intf *intf = (struct sprdwl_intf *)vif->priv->hw_priv;
#endif

	if (vif->sm_state != SPRDWL_CONNECTING &&
		vif->sm_state != SPRDWL_CONNECTED) {
		wl_ndev_log(L_ERR, vif->ndev, "%s Unexpected event!\n", __func__);
		return;
	}
#ifndef IBSS_SUPPORT
	if (conn_info->status != SPRDWL_CONNECT_SUCCESS &&
		conn_info->status != SPRDWL_ROAM_SUCCESS)
		goto err;
#else
	if (conn_info->status != SPRDWL_CONNECT_SUCCESS &&
		conn_info->status != SPRDWL_ROAM_SUCCESS &&
		conn_info->status != SPRDWL_IBSS_JOIN &&
		conn_info->status != SPRDWL_IBSS_START)
		goto err;
#endif /* IBSS_SUPPORT */
	if (!conn_info->bssid) {
		wl_ndev_log(L_ERR, vif->ndev, "%s NULL BSSID!\n", __func__);
		goto err;
	}
	if (!conn_info->req_ie_len) {
		wl_ndev_log(L_ERR, vif->ndev, "%s No associate REQ IE!\n", __func__);
		goto err;
	}
	if (!conn_info->resp_ie_len) {
		wl_ndev_log(L_ERR, vif->ndev, "%s No associate RESP IE!\n", __func__);
		goto err;
	}

	if (conn_info->bea_ie_len) {
		wl_debug("%s channel num:%d\n", __func__, conn_info->channel);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		freq = ieee80211_channel_to_frequency(conn_info->channel,
									conn_info->channel <= CH_MAX_2G_CHANNEL ?
									NL80211_BAND_2GHZ : NL80211_BAND_5GHZ);
#else
		freq = ieee80211_channel_to_frequency(conn_info->channel,
									conn_info->channel <= CH_MAX_2G_CHANNEL ?
									IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ);
#endif
		channel = ieee80211_get_channel(wiphy, freq);
		if (!channel) {
			wl_err("%s invalid freq!channel num:%d\n", __func__,
				conn_info->channel);
			goto err;
		}

		mgmt = (struct ieee80211_mgmt *)conn_info->bea_ie;
		wl_ndev_log(L_DBG, vif->ndev, "%s update BSS %s\n", __func__,
				vif->ssid);
		if (!mgmt) {
			wl_ndev_log(L_ERR, vif->ndev, "%s NULL frame!\n", __func__);
			goto err;
		}
		//if (!ether_addr_equal(conn_info->bssid, mgmt->bssid))
		//	wl_ndev_log(L_ERR, vif->ndev,
		//			"%s Invalid Beacon!,vif->bssid = %pM, con->bssid = %pM, mgmt->bssid = %pM\n",
		//			__func__, vif->bssid, conn_info->bssid, mgmt->bssid);
		ie = mgmt->u.probe_resp.variable;
		ielen = conn_info->bea_ie_len - offsetof(struct ieee80211_mgmt,
						 u.probe_resp.variable);
		/* framework use system bootup time */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
		get_monotonic_boottime(&ts);
#else
		ktime_get_boottime_ts64(&ts);
#endif
		tsf = (u64)ts.tv_sec * 1000000 + div_u64(ts.tv_nsec, 1000);
		beacon_interval = le16_to_cpu(mgmt->u.probe_resp.beacon_int);
		capability = le16_to_cpu(mgmt->u.probe_resp.capab_info);
		wl_ndev_log(L_DBG, vif->ndev, "%s, %pM, signal: %d\n",
			   ieee80211_is_probe_resp(mgmt->frame_control)
			   ? "proberesp" : "beacon", mgmt->bssid,
			   conn_info->signal);

		bss = cfg80211_inform_bss(wiphy, channel,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
					  CFG80211_BSS_FTYPE_UNKNOWN,
#endif
					  mgmt->bssid, tsf,
					  capability, beacon_interval,
					  ie, ielen, conn_info->signal, GFP_KERNEL);
		//if (unlikely(!bss))
		//	wl_ndev_log(L_ERR, vif->ndev,
		//		   "%s failed to inform bss frame!\n",
		//		   __func__);
	} else {
		wl_ndev_log(L_ERR, vif->ndev, "%s No Beason IE!\n", __func__);
	}

	if (vif->sm_state == SPRDWL_CONNECTING &&
		conn_info->status == SPRDWL_CONNECT_SUCCESS)
		cfg80211_connect_result(vif->ndev,
					conn_info->bssid, conn_info->req_ie, conn_info->req_ie_len,
					conn_info->resp_ie, conn_info->resp_ie_len,
					WLAN_STATUS_SUCCESS, GFP_KERNEL);
	else if (vif->sm_state == SPRDWL_CONNECTED &&
		 conn_info->status == SPRDWL_ROAM_SUCCESS){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
		struct cfg80211_roam_info roam_info = {
			.links[0].bss = bss,
			.req_ie = conn_info->req_ie,
			.req_ie_len = conn_info->req_ie_len,
			.resp_ie = conn_info->resp_ie,
			.resp_ie_len = conn_info->resp_ie_len,
		};
		cfg80211_roamed(vif->ndev, &roam_info, GFP_KERNEL);
#else
		cfg80211_roamed_bss(vif->ndev, bss, conn_info->req_ie, conn_info->req_ie_len,
					conn_info->resp_ie, conn_info->resp_ie_len, GFP_KERNEL);
#endif
	}
#ifdef IBSS_SUPPORT
	else if (vif->sm_state == SPRDWL_CONNECTED &&
		 (conn_info->status == SPRDWL_IBSS_JOIN ||
		 conn_info->status == SPRDWL_IBSS_START)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		freq = ieee80211_channel_to_frequency(conn_info->channel,
				conn_info->channel <= CH_MAX_2G_CHANNEL ?
				NL80211_BAND_2GHZ : NL80211_BAND_5GHZ);
#else
		freq = ieee80211_channel_to_frequency(conn_info->channel,
				conn_info->channel <= CH_MAX_2G_CHANNEL ?
				IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ);
#endif
		channel = ieee80211_get_channel(wiphy, freq);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
		cfg80211_ibss_joined(vif->ndev, conn_info->bssid, channel, GFP_KERNEL);
#else
		cfg80211_ibss_joined(vif->ndev, conn_info->bssid, GFP_KERNEL);
#endif
		}
#endif /* IBSS_SUPPORT */
	else {
		wl_ndev_log(L_ERR, vif->ndev, "%s sm_state (%d), status: (%d)!\n",
			   __func__, vif->sm_state, conn_info->status);
		goto err;
	}

	if (!netif_carrier_ok(vif->ndev)) {
		netif_carrier_on(vif->ndev);
		netif_wake_queue(vif->ndev);
	}

#ifdef WMMAC_WFA_CERTIFICATION
	wmm_params = (struct wmm_params_element *)get_wmm_ie(conn_info->resp_ie,
						conn_info->resp_ie_len,
						WLAN_EID_VENDOR_SPECIFIC,
						OUI_MICROSOFT,
						WMM_OUI_TYPE);

	if (wmm_params != NULL) {
		for (i = 0; i < NUM_AC; i++) {
			wl_ndev_log(L_DBG, vif->ndev, "wmm_params->ac[%d].aci_aifsn: %x\n",
					i, wmm_params->ac[i].aci_aifsn);
			priv->wmmac.ac[i].aci_aifsn = wmm_params->ac[i].aci_aifsn;
		}
	} else {
		wl_ndev_log(L_DBG, vif->ndev, "%s wmm_params is NULL!!!!\n", __func__);
	}
#endif

	vif->sm_state = SPRDWL_CONNECTED;
	memcpy(vif->bssid, conn_info->bssid, sizeof(vif->bssid));
	wl_ndev_log(L_DBG, vif->ndev, "%s %s to %s (%pM)\n", __func__,
			conn_info->status == SPRDWL_CONNECT_SUCCESS ?
			"connect" : "roam", vif->ssid, vif->bssid);
	return;
err:
#ifdef STA_SOFTAP_SCC_MODE
	intf->sta_home_channel = 0;
#endif
	if (status_code == WLAN_STATUS_SUCCESS)
		status_code = WLAN_STATUS_UNSPECIFIED_FAILURE;
	if (vif->sm_state == SPRDWL_CONNECTING)
		cfg80211_connect_result(vif->ndev, vif->bssid, NULL, 0, NULL, 0,
					status_code, GFP_KERNEL);

	wl_ndev_log(L_ERR, vif->ndev, "%s %s failed status code:%d!\n",
				__func__, vif->ssid, status_code);
	memset(vif->bssid, 0, sizeof(vif->bssid));
	memset(vif->ssid, 0, sizeof(vif->ssid));
}

void sprdwl_report_disconnection(struct sprdwl_vif *vif, u16 reason_code)
{
	if (vif->sm_state == SPRDWL_CONNECTING) {
		cfg80211_connect_result(vif->ndev, vif->bssid, NULL, 0, NULL, 0,
					WLAN_STATUS_UNSPECIFIED_FAILURE,
					GFP_KERNEL);
	} else if ((vif->sm_state == SPRDWL_CONNECTED) ||
			(vif->sm_state == SPRDWL_DISCONNECTING)) {
		cfg80211_disconnected(vif->ndev, reason_code,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
				NULL, 0, false, GFP_KERNEL);
#else
					  NULL, 0, GFP_KERNEL);
#endif
		wl_ndev_log(L_DBG, vif->ndev,
				"%s %s, reason_code %d\n", __func__,
				vif->ssid, reason_code);
	} else {
		wl_ndev_log(L_ERR, vif->ndev, "%s Unexpected event!\n", __func__);
		return;
	}

	vif->sm_state = SPRDWL_DISCONNECTED;

	sprdwl_fc_add_share_credit(vif);

	/* Clear bssid & ssid */
	memset(vif->bssid, 0, sizeof(vif->bssid));
	memset(vif->ssid, 0, sizeof(vif->ssid));
#ifdef WMMAC_WFA_CERTIFICATION
	reset_wmmac_parameters(vif->priv);
	reset_wmmac_ts_info();
	init_default_qos_map();
#endif
	/* Stop netif */
	if (netif_carrier_ok(vif->ndev)) {
		netif_carrier_off(vif->ndev);
		netif_stop_queue(vif->ndev);
	}
	/*clear link layer status data*/
	memset(&vif->priv->pre_radio, 0, sizeof(vif->priv->pre_radio));

	trace_deauth_reason(vif->mode, reason_code, REMOTE_EVENT);
}

void sprdwl_report_mic_failure(struct sprdwl_vif *vif, u8 is_mcast, u8 key_id)
{
	wl_ndev_log(L_DBG, vif->ndev,
			"%s is_mcast:0x%x key_id: 0x%x bssid: %pM\n",
			__func__, is_mcast, key_id, vif->bssid);

	cfg80211_michael_mic_failure(vif->ndev, vif->bssid,
					 (is_mcast ? NL80211_KEYTYPE_GROUP :
					  NL80211_KEYTYPE_PAIRWISE),
					 key_id, NULL, GFP_KERNEL);
}

static char type_name[16][32] = {
	"ASSO REQ",
	"ASSO RESP",
	"REASSO REQ",
	"REASSO RESP",
	"PROBE REQ",
	"PROBE RESP",
	"TIMING ADV",
	"RESERVED",
	"BEACON",
	"ATIM",
	"DISASSO",
	"AUTH",
	"DEAUTH",
	"ACTION",
	"ACTION NO ACK",
	"RESERVED"
};

static char pub_action_name[][32] = {
	"GO Negotiation Req",
	"GO Negotiation Resp",
	"GO Negotiation Conf",
	"P2P Invitation Req",
	"P2P Invitation Resp",
	"Device Discovery Req",
	"Device Discovery Resp",
	"Provision Discovery Req",
	"Provision Discovery Resp",
	"Reserved"
};

static char p2p_action_name[][32] = {
	"Notice of Absence",
	"P2P Precence Req",
	"P2P Precence Resp",
	"GO Discoverability Req",
	"Reserved"
};

#define MAC_LEN			(24)
#define ADDR1_OFFSET		(4)
#define ADDR2_OFFSET		(10)
#define ACTION_TYPE		(13)
#define ACTION_SUBTYPE_OFFSET	(30)
#define PUB_ACTION		(0x4)
#define P2P_ACTION		(0x7f)

#define	PRINT_BUF_LEN		(1 << 10)
static char print_buf[PRINT_BUF_LEN];
void sprdwl_cfg80211_dump_frame_prot_info(int send, int freq,
					  const unsigned char *buf, int len)
{
	int idx = 0;
	int type = ((*buf) & IEEE80211_FCTL_FTYPE) >> 2;
	int subtype = ((*buf) & IEEE80211_FCTL_STYPE) >> 4;
	int action, action_subtype;
	char *p = print_buf;

	idx += sprintf(p + idx, "[cfg80211] ");

	if (send)
		idx += sprintf(p + idx, "SEND: ");
	else
		idx += sprintf(p + idx, "RECV: ");

	if (type == IEEE80211_FTYPE_MGMT) {
		idx += sprintf(p + idx, "%dMHz, %s, ",
				   freq, type_name[subtype]);
	} else {
		idx += sprintf(p + idx,
				   "%dMHz, not mgmt frame, type=%d, ", freq, type);
	}

	if (subtype == ACTION_TYPE) {
		action = *(buf + MAC_LEN);
		action_subtype = *(buf + ACTION_SUBTYPE_OFFSET);
		if (action == PUB_ACTION)
			idx += sprintf(p + idx, "PUB:%s ",
					   pub_action_name[action_subtype]);
		else if (action == P2P_ACTION)
			idx += sprintf(p + idx, "P2P:%s ",
					   p2p_action_name[action_subtype]);
		else
			idx += sprintf(p + idx, "Unknown ACTION(0x%x)", action);
	}
	p[idx] = '\0';

	wl_debug("%s %pM %pM\n", p, &buf[4], &buf[10]);
}

/* P2P related stuff */
static int sprdwl_cfg80211_remain_on_channel(struct wiphy *wiphy,
						 struct wireless_dev *wdev,
						 struct ieee80211_channel *chan,
						 unsigned int duration, u64 *cookie)
{
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	enum nl80211_channel_type channel_type = 0;
	static u64 remain_index;
	int ret;

	*cookie = vif->listen_cookie = ++remain_index;
	wl_ndev_log(L_DBG, wdev->netdev, "%s %d for %dms, cookie %lld\n",
			__func__, chan->center_freq, duration, *cookie);
	memcpy(&vif->listen_channel, chan, sizeof(struct ieee80211_channel));

	ret = sprdwl_remain_chan(vif->priv, vif->ctx_id, chan,
				 channel_type, duration, cookie);
	if (ret)
		return ret;

	cfg80211_ready_on_channel(wdev, *cookie, chan, duration, GFP_KERNEL);

	return 0;
}

static int sprdwl_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
							struct wireless_dev *wdev,
							u64 cookie)
{
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);

	wl_ndev_log(L_DBG, wdev->netdev, "%s cookie %lld\n", __func__, cookie);

	return sprdwl_cancel_remain_chan(vif->priv, vif->ctx_id, cookie);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
static int sprdwl_cfg80211_mgmt_tx(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct cfg80211_mgmt_tx_params *params,
				   u64 *cookie)
#else
static int sprdwl_cfg80211_mgmt_tx(struct wiphy *wiphy,
				   struct wireless_dev *wdev,
				   struct ieee80211_channel *chan, bool offchan,
				   unsigned int wait, const u8 *buf, size_t len,
				   bool no_cck, bool dont_wait_for_ack, u64 *cookie)
#endif
{
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
	struct ieee80211_channel *chan = params->chan;
	const u8 *buf = params->buf;
	size_t len = params->len;
	unsigned int wait = params->wait;
	bool dont_wait_for_ack = params->dont_wait_for_ack;
#endif
	static u64 mgmt_index;
	int ret = 0;

	*cookie = ++mgmt_index;
	wl_ndev_log(L_DBG, wdev->netdev, "%s cookie %lld\n", __func__, *cookie);

	sprdwl_cfg80211_dump_frame_prot_info(1, chan->center_freq, buf, len);
	/* send tx mgmt */
	if (len > 0) {
		ret = sprdwl_tx_mgmt(vif->priv, vif->ctx_id,
				     ieee80211_frequency_to_channel
				     (chan->center_freq), dont_wait_for_ack,
				     wait, cookie, buf, len);

		if (ret == 0 && len == 217 && (chan->center_freq == 2412 || chan->center_freq == 5200)) {
			int type = ((*buf) & IEEE80211_FCTL_FTYPE) >> 2;
			int subtype = ((*buf) & IEEE80211_FCTL_STYPE) >> 4;
			int action = *(buf + MAC_LEN);
			int action_subtype = *(buf + ACTION_SUBTYPE_OFFSET);
			if (type == IEEE80211_FTYPE_MGMT && subtype == ACTION_TYPE &&
				action == PUB_ACTION && action_subtype == 1 &&
				buf[4] == 0x00 && buf[5] == 0x01 && buf[6] == 0x02 &&
				buf[7] == 0x03 && buf[8] == 0x04 && buf[9] == 0x05) {
				printk("sprdwl: %s(%d), DPP Frame Received\n", __func__, __LINE__);
				cfg80211_mgmt_tx_status(wdev, *cookie, buf, len, 0, GFP_KERNEL);
			}
		}

		if (ret)
			if (!dont_wait_for_ack)
				cfg80211_mgmt_tx_status(wdev, *cookie, buf, len,
							0, GFP_KERNEL);
	}

	return ret;
}

static void sprdwl_cfg80211_mgmt_frame_register(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						struct mgmt_frame_regs *upd)
{
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);
	struct sprdwl_work *misc_work;
	struct sprdwl_reg_mgmt *reg_mgmt;
	u16 mgmt_type;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
       u16 frame_type = BIT(upd->global_stypes << 4);
       bool reg = false;
#endif

	if (vif->mode == SPRDWL_MODE_NONE)
		return;

	mgmt_type = (frame_type & IEEE80211_FCTL_STYPE) >> 4;
	if ((reg && test_and_set_bit(mgmt_type, &vif->mgmt_reg)) ||
		(!reg && !test_and_clear_bit(mgmt_type, &vif->mgmt_reg))) {
		wl_ndev_log(L_DBG, wdev->netdev, "%s  mgmt %d has %sreg\n", __func__,
			   frame_type, reg ? "" : "un");
		return;
	}

	wl_ndev_log(L_DBG, wdev->netdev, "frame_type %d, reg %d\n", frame_type, reg);

	misc_work = sprdwl_alloc_work(sizeof(*reg_mgmt));
	if (!misc_work) {
		wl_ndev_log(L_ERR, wdev->netdev, "%s out of memory\n", __func__);
		return;
	}

	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_REG_MGMT;

	reg_mgmt = (struct sprdwl_reg_mgmt *)misc_work->data;
	reg_mgmt->type = frame_type;
	reg_mgmt->reg = reg;

	sprdwl_queue_work(vif->priv, misc_work);
}

void sprdwl_report_remain_on_channel_expired(struct sprdwl_vif *vif)
{
	wl_ndev_log(L_DBG, vif->ndev, "%s\n", __func__);

	cfg80211_remain_on_channel_expired(&vif->wdev, vif->listen_cookie,
					   &vif->listen_channel, GFP_KERNEL);
}

void sprdwl_report_mgmt_tx_status(struct sprdwl_vif *vif, u64 cookie,
				  const u8 *buf, u32 len, u8 ack)
{
	wl_ndev_log(L_DBG, vif->ndev, "%s cookie %lld\n", __func__, cookie);

	cfg80211_mgmt_tx_status(&vif->wdev, cookie, buf, len, ack, GFP_KERNEL);
}

void sprdwl_report_rx_mgmt(struct sprdwl_vif *vif, u8 chan, const u8 *buf,
			   size_t len)
{
	bool ret;
	int freq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	freq = ieee80211_channel_to_frequency(chan,
						  chan <= CH_MAX_2G_CHANNEL ?
						  NL80211_BAND_2GHZ :
						  NL80211_BAND_5GHZ);
#else
	freq = ieee80211_channel_to_frequency(chan,
						chan <= CH_MAX_2G_CHANNEL ?
						IEEE80211_BAND_2GHZ :
						IEEE80211_BAND_5GHZ);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
	ret = cfg80211_rx_mgmt(&vif->wdev, freq, 0, buf, len, GFP_ATOMIC);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
	ret = cfg80211_rx_mgmt(&vif->wdev, freq, 0, buf, len, 0);
#else
	ret = cfg80211_rx_mgmt(&vif->wdev, freq, 0, buf, len, 0, GFP_ATOMIC);
#endif
	if (!ret)
		wl_ndev_log(L_ERR, vif->ndev, "%s unregistered frame!", __func__);
}

void sprdwl_report_mgmt_deauth(struct sprdwl_vif *vif, const u8 *buf,
				   size_t len)
{
	struct sprdwl_work *misc_work;

	misc_work = sprdwl_alloc_work(len);
	if (!misc_work) {
		wl_ndev_log(L_ERR, vif->ndev, "%s out of memory", __func__);
		return;
	}

	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_DEAUTH;
	memcpy(misc_work->data, buf, len);

	sprdwl_queue_work(vif->priv, misc_work);
}

void sprdwl_report_mgmt_disassoc(struct sprdwl_vif *vif, const u8 *buf,
				 size_t len)
{
	struct sprdwl_work *misc_work;

	misc_work = sprdwl_alloc_work(len);
	if (!misc_work) {
		wl_ndev_log(L_ERR, vif->ndev, "%s out of memory", __func__);
		return;
	}

	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_DISASSOC;
	memcpy(misc_work->data, buf, len);

	sprdwl_queue_work(vif->priv, misc_work);
}

static int sprdwl_cfg80211_start_p2p_device(struct wiphy *wiphy,
						struct wireless_dev *wdev)
{
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);

	wl_ndev_log(L_DBG, vif->ndev, "%s\n", __func__);

	return sprdwl_init_fw(vif);
}

static void sprdwl_cfg80211_stop_p2p_device(struct wiphy *wiphy,
						struct wireless_dev *wdev)
{
	struct sprdwl_vif *vif = container_of(wdev, struct sprdwl_vif, wdev);

	wl_ndev_log(L_DBG, vif->ndev, "%s\n", __func__);

	sprdwl_uninit_fw(vif);

	if (vif->priv->scan_request)
		sprdwl_scan_done(vif, true);
}

static int sprdwl_cfg80211_tdls_mgmt(struct wiphy *wiphy,
				     struct net_device *ndev, const u8 *peer,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0))
				     int link_id,
#endif
				     u8 action_code, u8 dialog_token,
				     u16 status_code,  u32 peer_capability,
				     bool initiator, const u8 *buf, size_t len)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sk_buff *tdls_skb;
	struct sprdwl_cmd_tdls_mgmt *p;
	u16 datalen, ielen;
	u32 end = 0x1a2b3c4d;
	int ret = 0;

	wl_ndev_log(L_DBG, ndev, "%s action_code=%d(%pM)\n", __func__,
			action_code, peer);

	datalen = sizeof(*p) + len + sizeof(end);
	ielen = len + sizeof(end);
	tdls_skb = dev_alloc_skb(datalen + NET_IP_ALIGN);
	if (!tdls_skb) {
		wl_err("dev_alloc_skb failed\n");
		return -ENOMEM;
	}
	skb_reserve(tdls_skb, NET_IP_ALIGN);
	p = (struct sprdwl_cmd_tdls_mgmt *)skb_put(tdls_skb,
			offsetof(struct sprdwl_cmd_tdls_mgmt, u));

	ether_addr_copy(p->da, peer);
	ether_addr_copy(p->sa, vif->ndev->dev_addr);
	p->ether_type = cpu_to_be16(ETH_P_TDLS);
	p->payloadtype = WLAN_TDLS_SNAP_RFTYPE;
	switch (action_code) {
	case WLAN_TDLS_SETUP_REQUEST:
		p->category = WLAN_CATEGORY_TDLS;
		p->action_code = WLAN_TDLS_SETUP_REQUEST;
		p = (struct sprdwl_cmd_tdls_mgmt *)skb_put(tdls_skb,
			(sizeof(p->u.setup_req) + ielen));
		memcpy(p, &dialog_token, 1);
		memcpy((u8 *)p + 1, buf, len);
		memcpy((u8 *)p + 1 + len, &end, sizeof(end));
		break;
	case WLAN_TDLS_SETUP_RESPONSE:
		p->category = WLAN_CATEGORY_TDLS;
		p->action_code = WLAN_TDLS_SETUP_RESPONSE;
		p = (struct sprdwl_cmd_tdls_mgmt *)skb_put(tdls_skb,
			(sizeof(p->u.setup_resp) + ielen));
		memcpy(p, &status_code, 2);
		memcpy((u8 *)p + 2, &dialog_token, 1);
		memcpy((u8 *)p + 3, buf, len);
		memcpy((u8 *)p + 3 + len, &end, sizeof(end));
		break;
	case WLAN_TDLS_SETUP_CONFIRM:
		p->category = WLAN_CATEGORY_TDLS;
		p->action_code = WLAN_TDLS_SETUP_CONFIRM;
		p = (struct sprdwl_cmd_tdls_mgmt *)skb_put(tdls_skb,
			(sizeof(p->u.setup_cfm) + ielen));
		memcpy(p, &status_code, 2);
		memcpy((u8 *)p + 2, &dialog_token, 1);
		memcpy((u8 *)p + 3, buf, len);
		memcpy((u8 *)p + 3 + len, &end, sizeof(end));
		break;
	case WLAN_TDLS_TEARDOWN:
		p->category = WLAN_CATEGORY_TDLS;
		p->action_code = WLAN_TDLS_TEARDOWN;
		p = (struct sprdwl_cmd_tdls_mgmt *)skb_put(tdls_skb,
			(sizeof(p->u.teardown) + ielen));
		memcpy(p, &status_code, 2);
		memcpy((u8 *)p + 2, buf, len);
		memcpy((u8 *)p + 2 + len, &end, sizeof(end));
		break;
	case SPRDWL_TDLS_DISCOVERY_RESPONSE:
		p->category = WLAN_CATEGORY_TDLS;
		p->action_code = SPRDWL_TDLS_DISCOVERY_RESPONSE;
		p = (struct sprdwl_cmd_tdls_mgmt *)skb_put(tdls_skb,
			(sizeof(p->u.discover_resp) + ielen));
		memcpy(p, &dialog_token, 1);
		memcpy((u8 *)p + 1, buf, len);
		memcpy((u8 *)p + 1 + len, &end, sizeof(end));
		break;
	default:
		wl_err("%s, %d, error action_code%d\n", __func__, __LINE__, action_code);
		dev_kfree_skb(tdls_skb);
		return -ENOMEM;
		break;
	}

	ret = sprdwl_tdls_mgmt(vif, tdls_skb);
	dev_kfree_skb(tdls_skb);
	return ret;
}

static int sprdwl_cfg80211_tdls_oper(struct wiphy *wiphy,
					 struct net_device *ndev, const u8 *peer,
					 enum nl80211_tdls_operation oper)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	int ret;

	wl_ndev_log(L_DBG, ndev, "%s oper=%d\n", __func__, oper);

	if (oper == NL80211_TDLS_ENABLE_LINK) {
		sprdwl_tdls_flow_flush(vif, peer, oper);
		oper = SPRDWL_TDLS_ENABLE_LINK;
	} else if (oper == NL80211_TDLS_DISABLE_LINK)
		oper = SPRDWL_TDLS_DISABLE_LINK;
	else
		wl_ndev_log(L_ERR, ndev, "unsupported this TDLS oper\n");

	ret = sprdwl_tdls_oper(vif->priv, vif->ctx_id, peer, oper);
	/*to enable tx_addba_req*/
	if (!ret && oper == SPRDWL_TDLS_ENABLE_LINK) {
		u8 i;
		struct sprdwl_intf *intf;

		intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
		for (i = 0; i < MAX_LUT_NUM; i++) {
			if ((0 == memcmp(intf->peer_entry[i].tx.da,
					 peer, ETH_ALEN)) &&
				(intf->peer_entry[i].ctx_id == vif->ctx_id)) {
				wl_info("%s, %d, lut_index=%d\n",
					__func__, __LINE__,
					intf->peer_entry[i].lut_index);
				intf->peer_entry[i].ip_acquired  = 1;
				break;
			}
		}
	}
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
static int sprdwl_cfg80211_tdls_chan_switch(struct wiphy *wiphy,
						struct net_device *ndev,
						const u8 *addr, u8 oper_class,
						struct cfg80211_chan_def *chandef)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	u8 chan, band;

	chan = chandef->chan->hw_value;
	band = chandef->chan->band;

	wl_ndev_log(L_DBG, ndev, "%s: chan=%u, band=%u\n", __func__, chan, band);
	return sprdwl_start_tdls_channel_switch(vif->priv, vif->ctx_id, addr,
						chan, 0, band);
}

static void sprdwl_cfg80211_tdls_cancel_chan_switch(struct wiphy *wiphy,
							struct net_device *ndev,
							const u8 *addr)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);
	sprdwl_cancel_tdls_channel_switch(vif->priv, vif->ctx_id, addr);
}
#endif

void sprdwl_report_tdls(struct sprdwl_vif *vif, const u8 *peer,
			u8 oper, u16 reason_code)
{
	wl_ndev_log(L_DBG, vif->ndev, "%s A station (%pM)found\n", __func__, peer);

	cfg80211_tdls_oper_request(vif->ndev, peer, oper,
				   reason_code, GFP_KERNEL);
}

/* Roaming related stuff */
int sprdwl_cfg80211_set_cqm_rssi_config(struct wiphy *wiphy,
					struct net_device *ndev,
					s32 rssi_thold, u32 rssi_hyst)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s rssi_thold %d rssi_hyst %d",
			__func__, rssi_thold, rssi_hyst);

	return sprdwl_set_cqm_rssi(vif->priv, vif->ctx_id,
				   rssi_thold, rssi_hyst);
}

void sprdwl_report_cqm(struct sprdwl_vif *vif, u8 rssi_event)
{
	wl_ndev_log(L_DBG, vif->ndev, "%s rssi_event: %d\n", __func__, rssi_event);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	cfg80211_cqm_rssi_notify(vif->ndev, rssi_event, 0, GFP_KERNEL);
#else
	cfg80211_cqm_rssi_notify(vif->ndev, rssi_event, GFP_KERNEL);
#endif
}

int sprdwl_cfg80211_update_ft_ies(struct wiphy *wiphy, struct net_device *ndev,
				  struct cfg80211_update_ft_ies_params *ftie)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_set_roam_offload(vif->priv, vif->ctx_id,
					   SPRDWL_ROAM_OFFLOAD_SET_FTIE,
					   ftie->ie, ftie->ie_len);
}

static int sprdwl_cfg80211_set_qos_map(struct wiphy *wiphy,
					   struct net_device *ndev,
					   struct cfg80211_qos_map *qos_map)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_set_qos_map(vif->priv, vif->ctx_id, (void *)qos_map);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
static int sprdwl_cfg80211_add_tx_ts(struct wiphy *wiphy,
					 struct net_device *ndev,
					 u8 tsid, const u8 *peer,
					 u8 user_prio, u16 admitted_time)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_add_tx_ts(vif->priv, vif->ctx_id, tsid, peer,
				user_prio, admitted_time);
}

static int sprdwl_cfg80211_del_tx_ts(struct wiphy *wiphy,
					 struct net_device *ndev,
					 u8 tsid, const u8 *peer)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s\n", __func__);

	return sprdwl_del_tx_ts(vif->priv, vif->ctx_id, tsid, peer);
}
#endif

static int sprdwl_cfg80211_set_mac_acl(struct wiphy *wiphy,
					   struct net_device *ndev,
					   const struct cfg80211_acl_data *acl)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	int index, num;
	int mode = SPRDWL_ACL_MODE_DISABLE;
	unsigned char *mac_addr = NULL;

	if (!acl || !acl->n_acl_entries) {
		wl_ndev_log(L_ERR, ndev, "%s no ACL data\n", __func__);
		return 0;
	}

	if (acl->acl_policy == NL80211_ACL_POLICY_DENY_UNLESS_LISTED) {
		mode = SPRDWL_ACL_MODE_WHITELIST;
	} else if (acl->acl_policy == NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED) {
		mode = SPRDWL_ACL_MODE_BLACKLIST;
	} else {
		wl_ndev_log(L_ERR, ndev, "%s invalid ACL mode\n", __func__);
		return -EINVAL;
	}

	num = acl->n_acl_entries;
	wl_ndev_log(L_DBG, ndev, "%s ACL MAC num:%d\n", __func__, num);
	if (num < 0 || num > vif->priv->max_acl_mac_addrs)
		return -EINVAL;

	mac_addr = kzalloc(num * ETH_ALEN, GFP_KERNEL);
	if (IS_ERR(mac_addr))
		return -ENOMEM;

	for (index = 0; index < num; index++) {
		wl_ndev_log(L_DBG, ndev, "%s  MAC: %pM\n", __func__,
				&acl->mac_addrs[index]);
		memcpy(mac_addr + index * ETH_ALEN,
			   &acl->mac_addrs[index], ETH_ALEN);
	}

	if (mode == SPRDWL_ACL_MODE_WHITELIST)
		return sprdwl_set_whitelist(vif->priv, vif->ctx_id,
						SPRDWL_SUBCMD_ENABLE,
						num, mac_addr);
	else
		return sprdwl_set_blacklist(vif->priv, vif->ctx_id,
						SPRDWL_SUBCMD_ADD, num, mac_addr);
}

int sprdwl_cfg80211_set_power_mgmt(struct wiphy *wiphy, struct net_device *ndev,
				   bool enabled, int timeout)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);

	wl_ndev_log(L_DBG, ndev, "%s power save status:%d\n", __func__, enabled);
	return sprdwl_power_save(vif->priv, vif->ctx_id,
				 SPRDWL_SET_PS_STATE, enabled);
}

#ifdef ACS_SUPPORT
static int
sprdwl_cfg80211_dump_survey(struct wiphy *wiphy, struct net_device *ndev,
				int idx, struct survey_info *s_info)
{
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_survey_info *info = NULL;
	struct sprdwl_bssid *bssid = NULL, *pos = NULL;
	static int survey_count;
	int err = 0;

	if (vif->mode != SPRDWL_MODE_AP) {
		wl_ndev_log(L_DBG, vif->ndev, "Not AP mode, exit %s!\n", __func__);
		err = -ENOENT;
		goto out;
	}

	if (!list_empty(&vif->survey_info_list)) {
		info = list_first_entry(&vif->survey_info_list,
					struct sprdwl_survey_info, survey_list);
		list_del(&info->survey_list);

		if (info->channel) {
			s_info->channel = info->channel;
			s_info->noise = info->noise;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
			s_info->time = SPRDWL_ACS_SCAN_TIME;
			s_info->time_busy = info->cca_busy_time;
			s_info->filled = (SURVEY_INFO_NOISE_DBM |
					SURVEY_INFO_TIME |
					SURVEY_INFO_TIME_BUSY);
#else
			s_info->channel_time = SPRDWL_ACS_SCAN_TIME;
			s_info->channel_time_busy = info->cca_busy_time;
			s_info->filled = (SURVEY_INFO_NOISE_DBM |
					  SURVEY_INFO_CHANNEL_TIME |
					  SURVEY_INFO_CHANNEL_TIME_BUSY);
#endif

			survey_count++;
		}

		list_for_each_entry_safe(bssid, pos, &info->bssid_list, list) {
			list_del(&bssid->list);
			kfree(bssid);
			bssid = NULL;
		}

		kfree(info);
	} else {
		/* There are no more survey info in list */
		err = -ENOENT;
		wl_ndev_log(L_DBG, vif->ndev, "%s report %d surveys\n",
				__func__, survey_count);
		survey_count = 0;
	}

out:
	return err;
}
#endif /* ACS_SUPPORT */

#ifdef WOW_SUPPORT
static void sprdwl_set_wakeup(struct wiphy *wiphy, bool enabled)
{
	struct cfg80211_wowlan *cfg = wiphy->wowlan_config;
	struct sprdwl_priv *priv = wiphy_priv(wiphy);

	if (!enabled) {
		wl_debug("%s: disabled\n", __func__);
		sprdwl_set_wowlan(priv, SPRDWL_WOWLAN_ANY, NULL, 0);
		return;
	}

	if (cfg->any) {
		wl_debug("%s: any\n", __func__);
		sprdwl_set_wowlan(priv, SPRDWL_WOWLAN_ANY, NULL, 0);
		return;
	}

	if (cfg->magic_pkt) {
		wl_debug("%s: magic packet\n", __func__);
		sprdwl_set_wowlan(priv, SPRDWL_WOWLAN_MAGIC_PKT, NULL, 0);
	}

	if (cfg->disconnect) {
		wl_debug("%s: disconnect\n", __func__);
		sprdwl_set_wowlan(priv, SPRDWL_WOWLAN_DISCONNECT, NULL, 0);
	}
}
#endif

static struct cfg80211_ops sprdwl_cfg80211_ops = {
#ifndef CONFIG_P2P_INTF
	.add_virtual_intf = sprdwl_cfg80211_add_iface,
	.del_virtual_intf = sprdwl_cfg80211_del_iface,
#endif
	.change_virtual_intf = sprdwl_cfg80211_change_iface,
	.add_key = sprdwl_cfg80211_add_key,
	.del_key = sprdwl_cfg80211_del_key,
	.set_default_key = sprdwl_cfg80211_set_default_key,
	.set_rekey_data = sprdwl_cfg80211_set_rekey,
	.start_ap = sprdwl_cfg80211_start_ap,
	.change_beacon = sprdwl_cfg80211_change_beacon,
	.stop_ap = sprdwl_cfg80211_stop_ap,
	.add_station = sprdwl_cfg80211_add_station,
	.del_station = sprdwl_cfg80211_del_station,
	.change_station = sprdwl_cfg80211_change_station,
	.get_station = sprdwl_cfg80211_get_station,
	.libertas_set_mesh_channel = sprdwl_cfg80211_set_channel,
	.scan = sprdwl_cfg80211_scan,
	.connect = sprdwl_cfg80211_connect,
	.disconnect = sprdwl_cfg80211_disconnect,
#ifdef IBSS_SUPPORT
	.join_ibss = sprdwl_cfg80211_join_ibss,
	.leave_ibss = sprdwl_cfg80211_leave_ibss,
#endif /* IBSS_SUPPORT */
	.set_wiphy_params = sprdwl_cfg80211_set_wiphy_params,
	.set_pmksa = sprdwl_cfg80211_set_pmksa,
	.del_pmksa = sprdwl_cfg80211_del_pmksa,
	.flush_pmksa = sprdwl_cfg80211_flush_pmksa,
	.remain_on_channel = sprdwl_cfg80211_remain_on_channel,
	.cancel_remain_on_channel = sprdwl_cfg80211_cancel_remain_on_channel,
	.mgmt_tx = sprdwl_cfg80211_mgmt_tx,
	//.mgmt_frame_register = sprdwl_cfg80211_mgmt_frame_register,
	.update_mgmt_frame_registrations = sprdwl_cfg80211_mgmt_frame_register,
	.set_power_mgmt = sprdwl_cfg80211_set_power_mgmt,
	.set_cqm_rssi_config = sprdwl_cfg80211_set_cqm_rssi_config,
	.sched_scan_start = sprdwl_cfg80211_sched_scan_start,
	.sched_scan_stop = sprdwl_cfg80211_sched_scan_stop,
	.tdls_mgmt = sprdwl_cfg80211_tdls_mgmt,
	.tdls_oper = sprdwl_cfg80211_tdls_oper,
	.start_p2p_device = sprdwl_cfg80211_start_p2p_device,
	.stop_p2p_device = sprdwl_cfg80211_stop_p2p_device,
	.set_mac_acl = sprdwl_cfg80211_set_mac_acl,
	.update_ft_ies = sprdwl_cfg80211_update_ft_ies,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
	.set_qos_map = sprdwl_cfg80211_set_qos_map,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
	.add_tx_ts = sprdwl_cfg80211_add_tx_ts,
	.del_tx_ts = sprdwl_cfg80211_del_tx_ts,
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	.tdls_channel_switch = sprdwl_cfg80211_tdls_chan_switch,
	.tdls_cancel_channel_switch = sprdwl_cfg80211_tdls_cancel_chan_switch,
#endif

#ifdef ACS_SUPPORT
	.dump_survey = sprdwl_cfg80211_dump_survey,
#endif /*ACS_SUPPORT*/
#ifdef DFS_MASTER
	.start_radar_detection = sprdwl_cfg80211_start_radar_detection,
	.channel_switch = sprdwl_cfg80211_channel_switch,
#endif
#ifdef WOW_SUPPORT
	.set_wakeup = sprdwl_set_wakeup,
#endif
};

void sprdwl_save_ch_info(struct sprdwl_priv *priv, u32 band, u32 flags, int center_freq)
{
	int index = 0;
	/* Workaround for bug873327, report freq list instead of channel list */
	// int tmp_ch = ieee80211_frequency_to_channel(center_freq);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	if (band == NL80211_BAND_2GHZ) {
#else
	if (band == IEEE80211_BAND_2GHZ) {
#endif
		index = priv->ch_2g4_info.num_channels;
		// priv->ch_2g4_info.channels[index] = tmp_ch;
		priv->ch_2g4_info.channels[index] = center_freq;
		priv->ch_2g4_info.num_channels++;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	else if (band == NL80211_BAND_5GHZ) {
#else
	else if (band == IEEE80211_BAND_5GHZ) {
#endif
		if (flags & IEEE80211_CHAN_RADAR) {
			index = priv->ch_5g_dfs_info.num_channels;
			// priv->ch_5g_dfs_info.channels[index] = tmp_ch;
			priv->ch_5g_dfs_info.channels[index] = center_freq;
			priv->ch_5g_dfs_info.num_channels++;
		} else {
			index = priv->ch_5g_without_dfs_info.num_channels;
			// priv->ch_5g_without_dfs_info.channels[index] = tmp_ch;
			priv->ch_5g_without_dfs_info.channels[index] = center_freq;
			priv->ch_5g_without_dfs_info.num_channels++;
		}
	} else
		wl_err("invalid band param!\n");

}

#if defined(CONFIG_CFG80211_INTERNAL_REGDB) && !defined(CUSTOM_REGDOMAIN)
static void sprdwl_reg_notify(struct wiphy *wiphy,
				  struct regulatory_request *request)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
	struct sprdwl_intf *intf = (struct sprdwl_intf *)priv->hw_priv;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	const struct ieee80211_freq_range *freq_range;
	const struct ieee80211_reg_rule *reg_rule;
	struct sprdwl_ieee80211_regdomain *rd = NULL;
	u32 band, channel, i;
	u32 last_start_freq;
	u32 n_rules = 0, rd_size;

	wl_info("%s %c%c initiator %d hint_type %d\n", __func__,
		request->alpha2[0], request->alpha2[1],
		request->initiator, request->user_reg_hint_type);

	memset(&priv->ch_2g4_info, 0, sizeof(struct sprdwl_channel_list));
	memset(&priv->ch_5g_without_dfs_info, 0,
		sizeof(struct sprdwl_channel_list));
	memset(&priv->ch_5g_dfs_info, 0, sizeof(struct sprdwl_channel_list));

	/* Figure out the actual rule number */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	for (band = 0; band < NUM_NL80211_BANDS; band++) {
#else
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
#endif
		sband = wiphy->bands[band];
		if (!sband)
			continue;

		last_start_freq = 0;
		for (channel = 0; channel < sband->n_channels; channel++) {
			chan = &sband->channels[channel];

			reg_rule =
				freq_reg_info(wiphy, MHZ_TO_KHZ(chan->center_freq));
			if (IS_ERR(reg_rule))
				continue;

			freq_range = &reg_rule->freq_range;
			if (last_start_freq != freq_range->start_freq_khz) {
				last_start_freq = freq_range->start_freq_khz;
				n_rules++;
			}

			sprdwl_save_ch_info(priv, band, chan->flags,
						(int)(chan->center_freq));
		}
	}

	rd_size = sizeof(struct sprdwl_ieee80211_regdomain) +
		n_rules * sizeof(struct ieee80211_reg_rule);

	rd = kzalloc(rd_size, GFP_KERNEL);
	if (!rd) {
		wl_err("%s failed to alloc sprdwl_ieee80211_regdomain!\n",
			   __func__);
		return;
	}

	/* Fill regulatory domain */
	rd->n_reg_rules = n_rules;
	memcpy(rd->alpha2, request->alpha2, ARRAY_SIZE(rd->alpha2));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	for (band = 0, i = 0; band < NUM_NL80211_BANDS; band++) {
#else
	for (band = 0, i = 0; band < IEEE80211_NUM_BANDS; band++) {
#endif
		sband = wiphy->bands[band];
		if (!sband)
			continue;

		last_start_freq = 0;
		for (channel = 0; channel < sband->n_channels; channel++) {
			chan = &sband->channels[channel];

			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

#ifdef STA_SOFTAP_SCC_MODE
			if (intf->sta_home_channel && chan->flags & IEEE80211_CHAN_RADAR)
				intf->sta_home_channel = 0;
#endif
			reg_rule =
				freq_reg_info(wiphy, MHZ_TO_KHZ(chan->center_freq));
			if (IS_ERR(reg_rule))
				continue;

			freq_range = &reg_rule->freq_range;
			if (last_start_freq != freq_range->start_freq_khz &&
				i < n_rules) {
				last_start_freq = freq_range->start_freq_khz;

				memcpy(&rd->reg_rules[i], reg_rule,
					   sizeof(struct ieee80211_reg_rule));
				i++;

				wl_info(
					  "%d KHz - %d KHz @ %d KHz flags %#x, chan->flags:%x\n",
					  freq_range->start_freq_khz,
					  freq_range->end_freq_khz,
					  freq_range->max_bandwidth_khz,
					  reg_rule->flags, chan->flags);
			}
		}
	}

	print_hex_dump_debug("regdom:", DUMP_PREFIX_OFFSET, 16, 1,
				 rd, rd_size, true);
	if (sprdwl_set_regdom(priv, (u8 *)rd, rd_size))
		wl_err("%s failed to set regdomain!\n", __func__);
	if (rd != NULL) {
		kfree(rd);
		rd = NULL;
	}
}
#else
void sprdwl_reg_notify(struct wiphy *wiphy,
				  struct regulatory_request *request)
{
	struct sprdwl_priv *priv = wiphy_priv(wiphy);
#ifdef STA_SOFTAP_SCC_MODE
	struct sprdwl_intf *intf = (struct sprdwl_intf *)priv->hw_priv;
#endif
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	const struct ieee80211_freq_range *freq_range;
	const struct ieee80211_reg_rule *reg_rule;
	struct sprdwl_ieee80211_regdomain *rd = NULL;
	u32 band, channel, i;
	u32 last_start_freq;
	u32 n_rules = 0, rd_size;
	const struct ieee80211_regdomain *pRegdom;

	wl_info("%s %c%c initiator %d hint_type %d\n", __func__,
		request->alpha2[0], request->alpha2[1],
		request->initiator, request->user_reg_hint_type);

	if (!wiphy) {
		wl_err("%s(): Wiphy = NULL\n", __func__);
		return;
	}

#ifdef CP2_RESET_SUPPORT
	if (NL80211_REGDOM_SET_BY_COUNTRY_IE == request->initiator)
		memcpy(&priv->sync.request, request, sizeof(struct regulatory_request));
#endif

	pRegdom = getRegdomainFromSprdDB(request->alpha2);
	if (!pRegdom) {
		wl_err("%s: Error, no correct RegDomain, country:%c%c\n",
			__func__, request->alpha2[0], request->alpha2[1]);

		return;
	}

	memset(&priv->ch_2g4_info, 0, sizeof(struct sprdwl_channel_list));
	memset(&priv->ch_5g_without_dfs_info, 0,
		sizeof(struct sprdwl_channel_list));
	memset(&priv->ch_5g_dfs_info, 0, sizeof(struct sprdwl_channel_list));

	/* Figure out the actual rule number */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	for (band = 0; band < NUM_NL80211_BANDS; band++) {
#else
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
#endif
		sband = wiphy->bands[band];
		if (!sband)
			continue;

		last_start_freq = 0;
		for (channel = 0; channel < sband->n_channels; channel++) {
			chan = &sband->channels[channel];

			reg_rule =
				sprd_freq_reg_info_regd(MHZ_TO_KHZ(chan->center_freq), pRegdom);
			if (IS_ERR(reg_rule)) {
				wl_debug("%s, %d, chan=%d\n", __func__, __LINE__, (int)(chan->center_freq));
				continue;
			}

			freq_range = &reg_rule->freq_range;
			if (last_start_freq != freq_range->start_freq_khz) {
				last_start_freq = freq_range->start_freq_khz;
				n_rules++;
			}

			sprdwl_save_ch_info(priv, band, chan->flags,
						(int)(chan->center_freq));
		}
	}

	rd_size = sizeof(struct sprdwl_ieee80211_regdomain) +
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
		n_rules * sizeof(struct ieee80211_reg_rule);
#else
		n_rules * sizeof(struct unisoc_reg_rule);
#endif

	rd = kzalloc(rd_size, GFP_KERNEL);
	if (!rd) {
		wl_err("%s failed to alloc sprdwl_ieee80211_regdomain!\n",
			   __func__);
		return;
	}

	/* Fill regulatory domain */
	rd->n_reg_rules = n_rules;
	memcpy(rd->alpha2, request->alpha2, ARRAY_SIZE(rd->alpha2));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	for (band = 0, i = 0; band < NUM_NL80211_BANDS; band++) {
#else
	for (band = 0, i = 0; band < IEEE80211_NUM_BANDS; band++) {
#endif
		sband = wiphy->bands[band];
		if (!sband)
			continue;

		last_start_freq = 0;
		for (channel = 0; channel < sband->n_channels; channel++) {
			chan = &sband->channels[channel];

			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

#ifdef STA_SOFTAP_SCC_MODE
			if (intf->sta_home_channel && chan->flags & IEEE80211_CHAN_RADAR)
				intf->sta_home_channel = 0;
#endif

			reg_rule =
				 sprd_freq_reg_info_regd(MHZ_TO_KHZ(chan->center_freq), pRegdom);
			if (IS_ERR(reg_rule))
				continue;

			freq_range = &reg_rule->freq_range;
			if (last_start_freq != freq_range->start_freq_khz &&
				i < n_rules) {
				last_start_freq = freq_range->start_freq_khz;

				memcpy(&rd->reg_rules[i], reg_rule,
					   sizeof(struct ieee80211_reg_rule));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 15, 0))
				rd->reg_rules[i].dfs_cac_ms = 0;
#endif
				i++;

				wiphy_dbg(wiphy,
					  "   %d KHz - %d KHz @ %d KHz flags %#x\n",
					  freq_range->start_freq_khz,
					  freq_range->end_freq_khz,
					  freq_range->max_bandwidth_khz,
					  reg_rule->flags);
			}
		}
	}

	wl_hex_dump(L_DBG, "regdom:", DUMP_PREFIX_OFFSET, 16, 1,
				 rd, rd_size, true);
	if (sprdwl_set_regdom(priv, (u8 *)rd, rd_size))
		wl_err("%s failed to set regdomain!\n", __func__);

	kfree(rd);
}
#endif

static void sprdwl_ht_cap_update(struct ieee80211_sta_ht_cap *ht_info,
		struct sprdwl_priv *priv)
{
	struct wiphy_sec2_t *sec2 = &priv->wiphy_sec2;

	wl_info("%s enter:\n", __func__);
	ht_info->ht_supported = true;
	/*set Max A-MPDU length factor*/
	if (sec2->ampdu_para) {
		/*bit 0,1*/
		ht_info->ampdu_factor = (sec2->ampdu_para & 0x3);
		/*bit 2,3,4*/
		ht_info->ampdu_density = ((sec2->ampdu_para >> 2) & 0x7);
	}
	/*set HT capabilities map as described in 802.11n spec */
	if (sec2->ht_cap_info)
		ht_info->cap = sec2->ht_cap_info;
	/*set Supported MCS rates*/
	memcpy(&ht_info->mcs, &sec2->ht_mcs_set,
			sizeof(struct ieee80211_mcs_info));
}

static void sprdwl_vht_cap_update(struct ieee80211_sta_vht_cap *vht_cap,
		struct sprdwl_priv *priv)
{
	struct wiphy_sec2_t *sec2 = &priv->wiphy_sec2;

	wl_debug("%s enter:\n", __func__);
	vht_cap->vht_supported = true;
	if (sec2->vht_cap_info)
		vht_cap->cap = sec2->vht_cap_info;
	memcpy(&vht_cap->vht_mcs, &sec2->vht_mcs_set,
			sizeof(struct ieee80211_vht_mcs_info));
}

void sprdwl_setup_wiphy(struct wiphy *wiphy, struct sprdwl_priv *priv)
{
	struct wiphy_sec2_t *sec2 = NULL;
	struct ieee80211_sta_vht_cap *vht_info = NULL;
	struct ieee80211_sta_ht_cap *ht_info = NULL;
#if !defined (CONFIG_CFG80211_INTERNAL_REGDB) || defined(CUSTOM_REGDOMAIN)
	const struct ieee80211_regdomain *pRegdom;
	char alpha2[2];
#endif

	wiphy->mgmt_stypes = sprdwl_mgmt_stypes;
	wiphy->interface_modes =
		BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP) |
		BIT(NL80211_IFTYPE_P2P_GO) | BIT(NL80211_IFTYPE_P2P_CLIENT);
#ifndef CONFIG_P2P_INTF
	wiphy->interface_modes |= BIT(NL80211_IFTYPE_P2P_DEVICE);
#endif

#if defined(IBSS_SUPPORT)
	wiphy->interface_modes |= BIT(NL80211_IFTYPE_ADHOC);
#endif /* IBSS_SUPPORT */

	wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;
	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	wiphy->max_scan_ssids = SPRDWL_MAX_SCAN_SSIDS;
	wiphy->max_scan_ie_len = SPRDWL_MAX_SCAN_IE_LEN;
	wiphy->cipher_suites = sprdwl_cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(sprdwl_cipher_suites);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
	wiphy->max_ap_assoc_sta = priv->max_ap_assoc_sta;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	wiphy->bands[NL80211_BAND_2GHZ] = &sprdwl_band_2ghz;
#else
	wiphy->bands[IEEE80211_BAND_2GHZ] = &sprdwl_band_2ghz;
#endif
	if (priv->wiphy_sec2_flag) {
		/*update HT capa got from fw*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		ht_info = &wiphy->bands[NL80211_BAND_2GHZ]->ht_cap;
#else
		ht_info = &wiphy->bands[IEEE80211_BAND_2GHZ]->ht_cap;
#endif
		sprdwl_ht_cap_update(ht_info, priv);

		sec2 = &priv->wiphy_sec2;
		/*set antenna mask*/
		if (sec2->antenna_tx) {
			wl_info("tx antenna:%d\n", sec2->antenna_tx);
			wiphy->available_antennas_tx = sec2->antenna_tx;
		}
		if (sec2->antenna_rx) {
			wl_info("rx antenna:%d\n", sec2->antenna_rx);
			wiphy->available_antennas_rx = sec2->antenna_rx;
		}
		/*set retry limit for short or long frame*/
		if (sec2->retry_short) {
			wl_info("retry short num:%d\n", sec2->retry_short);
			wiphy->retry_short = sec2->retry_short;
		}
		if (sec2->retry_long) {
			wl_info("retry long num:%d\n", sec2->retry_long);
			wiphy->retry_long = sec2->retry_long;
		}
		/*Fragmentation threshold (dot11FragmentationThreshold)*/
		if ((sec2->frag_threshold) &&
			(sec2->frag_threshold <=
			 IEEE80211_MAX_FRAG_THRESHOLD)) {
				wl_info("frag threshold:%d\n", sec2->frag_threshold);
				wiphy->frag_threshold = sec2->frag_threshold;
		} else {
				wl_info("flag threshold invalid:%d,set to default:%d\n",
					sec2->frag_threshold,
					IEEE80211_MAX_FRAG_THRESHOLD);
				sec2->frag_threshold = IEEE80211_MAX_FRAG_THRESHOLD;
		}
		/*RTS threshold (dot11RTSThreshold); -1 = RTS/CTS disabled*/
		if ((sec2->rts_threshold) &&
			(sec2->rts_threshold <= IEEE80211_MAX_RTS_THRESHOLD)) {
				wl_info("rts threshold:%d\n", sec2->rts_threshold);
				wiphy->rts_threshold = sec2->rts_threshold;
		} else {
				wl_info("rts threshold invalid:%d,set to default:%d\n",
					sec2->rts_threshold, IEEE80211_MAX_RTS_THRESHOLD);
				wiphy->rts_threshold = IEEE80211_MAX_RTS_THRESHOLD;
		}
	}

#ifdef CONFIG_PM
	/* Set WoWLAN flags */
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0)
	wiphy->wowlan = &sprdwl_wowlan_support;
#else
	memcpy(&wiphy->wowlan, &sprdwl_wowlan_support, sizeof(struct wiphy_wowlan_support));
#endif
#endif
	wiphy->max_remain_on_channel_duration = 5000;
	wiphy->max_num_pmkids = SPRDWL_MAX_NUM_PMKIDS;
#ifdef RND_MAC_SUPPORT
	wiphy->features |= (SCAN_RANDOM_MAC_ADDR);
#endif
	wiphy->features |= NL80211_FEATURE_CELL_BASE_REG_HINTS;

#if 0
	if (priv->fw_std & SPRDWL_STD_11E) {
		wl_info("\tIEEE802.11e supported\n");
		wiphy->features |= NL80211_FEATURE_SUPPORTS_WMM_ADMISSION;
		wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
	}
#endif

	if (priv->fw_std & SPRDWL_STD_11K)
		wl_info("\tIEEE802.11k supported\n");

	if (priv->fw_std & SPRDWL_STD_11R)
		wl_info("\tIEEE802.11r supported\n");

	if (priv->fw_std & SPRDWL_STD_11U)
		wl_info("\tIEEE802.11u supported\n");

	if (priv->fw_std & SPRDWL_STD_11V)
		wl_info("\tIEEE802.11v supported\n");

	if (priv->fw_std & SPRDWL_STD_11W)
		wl_info("\tIEEE802.11w supported\n");

	if (priv->fw_capa & SPRDWL_CAPA_5G) {
		wl_info("\tDual band supported\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		wiphy->bands[NL80211_BAND_5GHZ] = &sprdwl_band_5ghz;
#else
		wiphy->bands[IEEE80211_BAND_5GHZ] = &sprdwl_band_5ghz;
#endif
		if (priv->wiphy_sec2_flag) {
			/*update HT capa got from fw*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
			ht_info = &wiphy->bands[NL80211_BAND_5GHZ]->ht_cap;
#else
			ht_info = &wiphy->bands[IEEE80211_BAND_5GHZ]->ht_cap;
#endif
			sprdwl_ht_cap_update(ht_info, priv);
			/*update VHT capa got from fw*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
			vht_info = &wiphy->bands[NL80211_BAND_5GHZ]->vht_cap;
#else
			vht_info = &wiphy->bands[IEEE80211_BAND_5GHZ]->vht_cap;
#endif
			sprdwl_vht_cap_update(vht_info, priv);
		}
	}

	if (priv->fw_std & SPRDWL_STD_11D) {
		wl_info("\tIEEE802.11d supported\n");
		wiphy->reg_notifier = sprdwl_reg_notify;

#if !defined (CONFIG_CFG80211_INTERNAL_REGDB) || defined(CUSTOM_REGDOMAIN)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
	wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
#else
	wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
#endif
		alpha2[0] = '0';
		alpha2[1] = '0';
		pRegdom = getRegdomainFromSprdDB((char *)alpha2);
		if (pRegdom) {
			apply_custom_regulatory(wiphy, pRegdom);
			ShowChannel(wiphy);
		}
#endif
	}

	if (priv->fw_capa & SPRDWL_CAPA_MCC) {
		wl_info("\tMCC supported\n");
		wiphy->n_iface_combinations = ARRAY_SIZE(sprdwl_iface_combos);
		wiphy->iface_combinations = sprdwl_iface_combos;
	} else {
		wl_info("\tSCC supported\n");
		wiphy->software_iftypes =
			BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP) |
			BIT(NL80211_IFTYPE_P2P_CLIENT) |
			BIT(NL80211_IFTYPE_P2P_GO);
#ifndef CONFIG_P2P_INTF
		wiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_DEVICE);
#endif
	}

	if (priv->fw_capa & SPRDWL_CAPA_ACL) {
		wl_info("\tACL supported (%d)\n", priv->max_acl_mac_addrs);
		wiphy->max_acl_mac_addrs = priv->max_acl_mac_addrs;
	}

	if (priv->fw_capa & SPRDWL_CAPA_AP_SME) {
		wl_info("\tAP SME enabled\n");
		wiphy->flags |= WIPHY_FLAG_HAVE_AP_SME;
		wiphy->ap_sme_capa = 1;
	}
#if 0
	if (priv->fw_capa & SPRDWL_CAPA_PMK_OKC_OFFLOAD &&
		priv->fw_capa & SPRDWL_CAPA_11R_ROAM_OFFLOAD) {
		wl_info("\tRoaming offload supported\n");
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM;
	}
#endif
	if (priv->fw_capa & SPRDWL_CAPA_SCHED_SCAN) {
		wl_info("\tScheduled scan supported\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
#endif
		wiphy->max_sched_scan_ssids = SPRDWL_MAX_PFN_LIST_COUNT;
		wiphy->max_match_sets = SPRDWL_MAX_PFN_LIST_COUNT;
		wiphy->max_sched_scan_ie_len = SPRDWL_MAX_SCAN_IE_LEN;
	}
#if 0
	if (priv->fw_capa & SPRDWL_CAPA_TDLS) {
		wl_info("\tTDLS supported\n");
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
		wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;
		wiphy->features |= NL80211_FEATURE_TDLS_CHANNEL_SWITCH;
	}
#endif
	if (priv->fw_capa & SPRDWL_CAPA_LL_STATS)
		wl_info("\tLink layer stats supported\n");

#if defined(IBSS_SUPPORT) && defined(IBSS_RSN_SUPPORT)
	wiphy->flags |= WIPHY_FLAG_IBSS_RSN;
#endif /* IBSS_SUPPORT && IBSS_RSN_SUPPORT */

#ifdef DFS_MASTER
	wiphy->flags |= WIPHY_FLAG_HAS_CHANNEL_SWITCH;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	wiphy->max_sched_scan_reqs = 1;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
	wiphy_ext_feature_set(wiphy,
		NL80211_EXT_FEATURE_SCHED_SCAN_RELATIVE_RSSI);
#endif
}

static void sprdwl_check_intf_ops(struct sprdwl_if_ops *ops)
{
	WARN_ON(!ops->get_msg_buf);
	WARN_ON(!ops->free_msg_buf);
	WARN_ON(!ops->tx);
	WARN_ON(!ops->force_exit);
	WARN_ON(!ops->is_exit);
}

struct sprdwl_priv *sprdwl_core_create(enum sprdwl_hw_type type,
					   struct sprdwl_if_ops *ops)
{
	struct wiphy *wiphy;
	struct sprdwl_priv *priv;
	int ret = 0;

	sprdwl_check_intf_ops(ops);
	sprdwl_cmd_init();

	wiphy = wiphy_new(&sprdwl_cfg80211_ops, sizeof(*priv));
	if (!wiphy) {
		wl_err("failed to allocate wiphy!\n");
		return NULL;
	}
	priv = wiphy_priv(wiphy);
	priv->wiphy = wiphy;
	g_sprdwl_priv = priv;
	priv->hw_type = type;
	wl_info("hw_type:%d\n", priv->hw_type);

	priv->skb_head_len = sizeof(struct sprdwl_data_hdr) + NET_IP_ALIGN +
		SPRDWL_SKB_HEAD_RESERV_LEN + 3;

	priv->if_ops = ops;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	timer_setup(&priv->scan_timer, sprdwl_scan_timeout, 0);
#else
	setup_timer(&priv->scan_timer, sprdwl_scan_timeout,
			(unsigned long)priv);
#endif

#ifdef WMMAC_WFA_CERTIFICATION
	wmm_ac_init(priv);
	init_default_qos_map();
#endif
	spin_lock_init(&priv->scan_lock);
	spin_lock_init(&priv->sched_scan_lock);
	spin_lock_init(&priv->list_lock);
	INIT_LIST_HEAD(&priv->vif_list);
	ret = sprdwl_init_work(priv);
	if (ret != 0) {
		wl_err("sprdwl_init_work failed!\n");
		return NULL;
	}

	return priv;
}

void sprdwl_core_free(struct sprdwl_priv *priv)
{
	if (priv)
		sprdwl_deinit_work(priv);
	sprdwl_cmd_deinit();
	if (priv) {
		struct wiphy *wiphy = priv->wiphy;

		if (wiphy)
			wiphy_free(wiphy);
		g_sprdwl_priv = NULL;
	}
}
