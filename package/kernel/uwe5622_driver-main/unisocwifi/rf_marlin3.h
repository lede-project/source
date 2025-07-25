#ifndef __MARLIN3_RF_H__
#define __MARLIN3_RF_H__
struct nvm_cali_cmd {
	int8_t itm[64];
	int32_t par[512];
	int32_t num;
};

struct nvm_name_table {
	int8_t *itm;
	uint32_t mem_offset;
	int32_t type;
};
/*[Section 1: Version] */
struct version_t {
	uint16_t major;
	uint16_t minor;
};

/*[Section 2: Board Config]*/
struct board_config_t {
	uint16_t calib_bypass;
	uint8_t txchain_mask;
	uint8_t rxchain_mask;
};

/*[Section 3: Board Config TPC]*/
struct board_config_tpc_t {
	uint8_t dpd_lut_idx[8];
	uint16_t tpc_goal_chain0[8];
	uint16_t tpc_goal_chain1[8];
};

struct tpc_element_lut_t {
	uint8_t rf_gain_idx;
	uint8_t pa_bias_idx;
	int8_t  dvga_offset;
	int8_t  residual_error;
};
/*[Section 4: TPC-LUT]*/
struct tpc_lut_t {
	struct tpc_element_lut_t chain0_lut[8];
	struct tpc_element_lut_t chain1_lut[8];
};

/*[Section 5: Board Config Frequency Compensation]*/
struct board_conf_freq_comp_t {
	int8_t channel_2g_chain0[14];
	int8_t channel_2g_chain1[14];
	int8_t channel_5g_chain0[25];
	int8_t channel_5g_chain1[25];
	int8_t reserved[2];
};

/*[Section 6: Rate To Power with BW 20M]*/
struct power_20m_t {
	int8_t power_11b[4];
	int8_t power_11ag[8];
	int8_t power_11n[17];
	int8_t power_11ac[20];
	int8_t reserved[3];
};

/*[Section 7: Power Backoff]*/
struct power_backoff_t {
	int8_t green_wifi_offset;
	int8_t ht40_power_offset;
	int8_t vht40_power_offset;
	int8_t vht80_power_offset;
	int8_t sar_power_offset;
	int8_t mean_power_offset;
	int8_t tpc_mode;
	int8_t magic_word;
};

/*[Section 8: Reg Domain]*/
struct reg_domain_t {
	uint32_t reg_domain1;
	uint32_t reg_domain2;
};

/*[Section 9: Band Edge Power offset (MKK, FCC, ETSI)]*/
struct band_edge_power_offset_t {
	uint8_t bw20m[39];
	uint8_t bw40m[21];
	uint8_t bw80m[6];
	uint8_t reserved[2];
};

/*[Section 10: TX Scale]*/
struct tx_scale_t {
	int8_t chain0[39][16];
	int8_t chain1[39][16];
};

/*[Section 11: misc]*/
struct misc_t {
	int8_t dfs_switch;
	int8_t power_save_switch;
	int8_t fem_lan_param_setup;
	int8_t rssi_report_diff;
};

/*[Section 12: debug reg]*/
struct debug_reg_t {
	uint32_t address[16];
	uint32_t value[16];
};

/*[Section 13:coex_config] */
struct coex_config_t {
	uint32_t bt_performance_cfg0;
	uint32_t bt_performance_cfg1;
	uint32_t wifi_performance_cfg0;
	uint32_t wifi_performance_cfg2;
	uint32_t strategy_cfg0;
	uint32_t strategy_cfg1;
	uint32_t strategy_cfg2;
	uint32_t compatibility_cfg0;
	uint32_t compatibility_cfg1;
	uint32_t ant_cfg0;
	uint32_t ant_cfg1;
	uint32_t isolation_cfg0;
	uint32_t isolation_cfg1;
	uint32_t reserved_cfg0;
	uint32_t reserved_cfg1;
	uint32_t reserved_cfg2;
	uint32_t reserved_cfg3;
	uint32_t reserved_cfg4;
	uint32_t reserved_cfg5;
	uint32_t reserved_cfg6;
	uint32_t reserved_cfg7;
};

struct rf_config_t {
	int rf_data_len;
	uint8_t rf_data[1500];
};

/*wifi config section1 struct*/
struct wifi_conf_sec1_t {
	struct version_t version;
	struct board_config_t board_config;
	struct board_config_tpc_t board_config_tpc;
	struct tpc_lut_t tpc_lut;
	struct board_conf_freq_comp_t board_conf_freq_comp;
	struct power_20m_t power_20m;
	struct power_backoff_t power_backoff;
	struct reg_domain_t reg_domain;
	struct band_edge_power_offset_t band_edge_power_offset;
};

/*wifi config section2 struct*/
struct wifi_conf_sec2_t {
	struct tx_scale_t tx_scale;
	struct misc_t misc;
	struct debug_reg_t debug_reg;
	struct coex_config_t coex_config;
};

/*wifi config struct*/
struct wifi_conf_t {
	struct version_t version;
	struct board_config_t board_config;
	struct board_config_tpc_t board_config_tpc;
	struct tpc_lut_t tpc_lut;
	struct board_conf_freq_comp_t board_conf_freq_comp;
	struct power_20m_t power_20m;
	struct power_backoff_t power_backoff;
	struct reg_domain_t reg_domain;
	struct band_edge_power_offset_t band_edge_power_offset;
	struct tx_scale_t tx_scale;
	struct misc_t misc;
	struct debug_reg_t debug_reg;
	struct coex_config_t coex_config;
	struct rf_config_t rf_config;
};

int get_wifi_config_param(struct wifi_conf_t *p);
unsigned int marlin_get_wcn_chipid(void);
#endif
