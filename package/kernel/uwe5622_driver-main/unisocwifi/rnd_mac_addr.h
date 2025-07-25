#ifndef __SPRDWL_RND_MAC_ADDR_H__
#define __SPRDWL_RND_MAC_ADDR_H__

#include "sprdwl.h"

#define SCAN_RANDOM_MAC_ADDR (1 << 29)

enum sprdwl_random_mac_flags {
	SPRDWL_DISABLE_SCAN_RANDOM_ADDR,
	SPRDWL_ENABLE_SCAN_RANDOM_ADDR,
	SPRDWL_CONNECT_RANDOM_ADDR,
};

extern u8 rand_addr[ETH_ALEN];
void random_mac_addr(u8 *addr);
int wlan_cmd_set_rand_mac(struct sprdwl_priv *priv, u8 vif_mode,
		u8 random_mac_flag, u8 *addr);

#endif
