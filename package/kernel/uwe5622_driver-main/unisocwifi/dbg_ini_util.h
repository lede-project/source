#ifndef __DBG_INI_UTIL_H__
#define __DBG_INI_UTIL_H__

struct dbg_ini_cfg {
	int softap_channel;
	int debug_log_level;
};

enum SEC_TYPE {
	SEC_INVALID,
	SEC_SOFTAP,
	SEC_DEBUG,
	MAX_SEC_NUM,
};

int dbg_util_init(struct dbg_ini_cfg *cfg);
bool is_valid_channel(struct wiphy *wiphy, u16 chn);
//void sprdwl_dbg_reset_tail_ht_oper(u8 *tail, int tail_len, u16 chn);
//void sprdwl_dbg_reset_head_ds_params(u8 *beacon_head, int head_len, u16 chn);
int sprdwl_dbg_new_beacon_tail(const u8 *beacon_tail, int len, u8 *new_tail, u16 chn);
int sprdwl_dbg_new_beacon_head(const u8 *beacon_head, int len, u8 *new_head,  u16 chn);
#endif
