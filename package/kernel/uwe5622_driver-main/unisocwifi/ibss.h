#ifndef __SPRDWL_IBSS_H__
#define __SPRDWL_IBSS_H__

#include "sprdwl.h"

/* IBSS attribute */
struct sprdwl_ibss_attr {
#define SPRDWL_IBSS_COALESCE		1
#define SPRDWL_IBSS_SCAN_SUPPRESS	2
#define SPRDWL_IBSS_ATIM		3
#define SPRDWL_IBSS_WPA_VERSION		4
	u8 sub_type;
	u8 value;
} __packed;

/* used for join with or without a specific bssid */
struct sprdwl_join_params {
	unsigned short ssid_len;
	unsigned char ssid[32];
	unsigned short bssid_len;
	unsigned char bssid[6];
} __packed;

/* cfg80211 */
int sprdwl_cfg80211_join_ibss(struct wiphy *wiphy,
				     struct net_device *ndev,
				     struct cfg80211_ibss_params *params);
int sprdwl_cfg80211_leave_ibss(struct wiphy *wiphy,
				      struct net_device *ndev);

/* cmd */
int sprdwl_set_ibss_attribute(struct sprdwl_priv *priv, u8 vif_mode,
				u8 sub_type, u8 value);
int sprdwl_ibss_join(struct sprdwl_priv *priv, u8 vif_mode,
			struct sprdwl_join_params *params, u32 params_len);
#endif
