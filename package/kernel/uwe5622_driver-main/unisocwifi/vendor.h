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

#ifndef __SPRDWL_VENDOR_H__
#define __SPRDWL_VENDOR_H__

#include <net/netlink.h>
#include <net/cfg80211.h>
#include <linux/ctype.h>

#define OUI_SPREAD 0x001374

enum {
	/* Memory dump of FW */
	WIFI_LOGGER_MEMORY_DUMP_SUPPORTED = (1 << (0)),
	/*PKT status*/
	WIFI_LOGGER_PER_PACKET_TX_RX_STATUS_SUPPORTED = (1 << (1)),
	/*Connectivity event*/
	WIFI_LOGGER_CONNECT_EVENT_SUPPORTED = (1 << (2)),
	/* POWER of Driver */
	WIFI_LOGGER_POWER_EVENT_SUPPORTED = (1 << (3)),
	/*WAKE LOCK of Driver*/
	WIFI_LOGGER_WAKE_LOCK_SUPPORTED = (1 << (4)),
	/*verbose log of FW*/
	WIFI_LOGGER_VERBOSE_SUPPORTED = (1 << (5)),
	/*monitor the health of FW*/
	WIFI_LOGGER_WATCHDOG_TIMER_SUPPORTED = (1 << (6)),
	/*dumps driver state*/
	WIFI_LOGGER_DRIVER_DUMP_SUPPORTED = (1 << (7)),
	/*tracks connection packets' fate*/
	WIFI_LOGGER_PACKET_FATE_SUPPORTED = (1 << (8)),
};

enum sprdwl_wifi_error {
	WIFI_SUCCESS = 0,
	WIFI_ERROR_UNKNOWN = -1,
	WIFI_ERROR_UNINITIALIZED = -2,
	WIFI_ERROR_NOT_SUPPORTED = -3,
	WIFI_ERROR_NOT_AVAILABLE = -4,
	WIFI_ERROR_INVALID_ARGS = -5,
	WIFI_ERROR_INVALID_REQUEST_ID = -6,
	WIFI_ERROR_TIMED_OUT = -7,
	WIFI_ERROR_TOO_MANY_REQUESTS = -8,
	WIFI_ERROR_OUT_OF_MEMORY = -9,
	WIFI_ERROR_BUSY = -10,
};

enum sprdwl_vendor_subcommand_id {
	SPRDWL_VENDOR_SUBCMD_ROAMING = 9,
	SPRDWL_VENDOR_SUBCMD_NAN = 12,
	SPRDWL_VENDOR_SET_LLSTAT = 14,
	SPRDWL_VENDOR_GET_LLSTAT = 15,
	SPRDWL_VENDOR_CLR_LLSTAT = 16,
	SPRDWL_VENDOR_SUBCMD_GSCAN_START = 20,
	SPRDWL_VENDOR_SUBCMD_GSCAN_STOP = 21,
	SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CHANNEL = 22,
	SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES = 23,
	SPRDWL_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS = 24,
	/* Used when report_threshold is reached in scan cache. */
	SPRDWL_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE = 25,
	/* Used to report scan results when each probe rsp. is received,
	* if report_events enabled in wifi_scan_cmd_params.
	*/
	SPRDWL_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT = 26,
	/* Indicates progress of scanning state-machine. */
	SPRDWL_VENDOR_SUBCMD_GSCAN_SCAN_EVENT = 27,
	/* Indicates BSSID Hotlist. */
	SPRDWL_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND = 28,
	SPRDWL_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST = 29,
	SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST = 30,
	SPRDWL_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE = 31,
	SPRDWL_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE = 32,
	SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE = 33,
	SPRDWL_VENDOR_SUBCMD_GET_SUPPORT_FEATURE = 38,
	SPRDWL_VENDOR_SUBCMD_SET_MAC_OUI = 39,
	SPRDWL_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_LOST = 41,
	SPRDWL_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX = 42,
	SPRDWL_VENDOR_SUBCMD_GET_FEATURE = 55,
	SPRDWL_VENDOR_SUBCMD_GET_WIFI_INFO = 61,
	SPRDWL_VENDOR_SUBCMD_START_LOGGING = 62,
	SPRDWL_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP = 63,
	SPRDWL_VENDOR_SUBCMD_ROAM = 64,
	SPRDWL_VENDOR_SUBCMD_GSCAN_SET_SSID_HOTLIST = 65,
	SPRDWL_VENDOR_SUBCMD_GSCAN_RESET_SSID_HOTLIST = 66,
	SPRDWL_VENDOR_SUBCMD_PNO_SET_LIST = 69,
	SPRDWL_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST = 70,
	SPRDWL_VENDOR_SUBCMD_PNO_RESET_PASSPOINT_LIST = 71,
	SPRDWL_VENDOR_SUBCMD_PNO_NETWORK_FOUND = 72,
	SPRDWL_VENDOR_SUBCMD_GET_LOGGER_FEATURE_SET = 76,
	SPRDWL_VENDOR_SUBCMD_GET_RING_DATA = 77,
	SPRDWL_VENDOR_SUBCMD_OFFLOADED_PACKETS = 79,
	SPRDWL_VENDOR_SUBCMD_MONITOR_RSSI = 80,
	SPRDWL_VENDOR_SUBCMD_ENABLE_ND_OFFLOAD = 82,
	SPRDWL_VENDOR_SUBCMD_GET_WAKE_REASON_STATS = 85,
	SPRDWL_VENDOR_SUBCMD_SET_SAR_LIMITS = 146,

	SPRDWL_VENDOR_SUBCOMMAND_MAX
};

#define SPRDWL_VENDOR_EVENT_NAN_INDEX 32

enum sprdwl_vendor_event {
	SPRDWL_VENDOR_SUBCMD_MONITOR_RSSI_INDEX = 0,
	/* NAN */
	SPRDWL_VENDOR_EVENT_NAN = 0x1400,
};

/* attribute id */

enum sprdwl_vendor_attr_gscan_id {
	SPRDWL_VENDOR_ATTR_FEATURE_SET = 1,
	SPRDWL_VENDOR_ATTR_GSCAN_NUM_CHANNELS = 3,
	SPRDWL_VENDOR_ATTR_GSCAN_CHANNELS = 4,
	SPRDWL_VENDOR_ATTR_MAX
};

enum sprdwl_vendor_attr_get_wifi_info {
	SPRDWL_VENDOR_ATTR_WIFI_INFO_GET_INVALID = 0,
	SPRDWL_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION = 1,
	SPRDWL_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION = 2,
};

enum sprdwl_vendor_attribute {
	SPRDWL_VENDOR_ATTR_UNSPEC,
	SPRDWL_VENDOR_ATTR_GET_LLSTAT,
	SPRDWL_VENDOR_ATTR_CLR_LLSTAT,
	/* NAN */
	SRPDWL_VENDOR_ATTR_NAN,
	SPRDWL_VENDOR_ROAMING_POLICY = 5,
};

/*start of link layer stats, CMD ID:14,15,16*/
enum sprdwl_wlan_vendor_attr_ll_stats_set {
	SPRDWL_LL_STATS_SET_INVALID = 0,
	/* Unsigned 32-bit value */
	SPRDWL_LL_STATS_MPDU_THRESHOLD = 1,
	SPRDWL_LL_STATS_GATHERING = 2,
	/* keep last */
	SPRDWL_LL_STATS_SET_AFTER_LAST,
	SPRDWL_LL_STATS_SET_MAX =
	SPRDWL_LL_STATS_SET_AFTER_LAST - 1,
};

static const struct nla_policy
sprdwl_ll_stats_policy[SPRDWL_LL_STATS_SET_MAX + 1] = {
	[SPRDWL_LL_STATS_MPDU_THRESHOLD] = { .type = NLA_U32 },
	[SPRDWL_LL_STATS_GATHERING] = { .type = NLA_U32 },
};

enum sprdwl_vendor_attr_ll_stats_results {
	SPRDWL_LL_STATS_INVALID = 0,
	SPRDWL_LL_STATS_RESULTS_REQ_ID = 1,
	SPRDWL_LL_STATS_IFACE_BEACON_RX = 2,
	SPRDWL_LL_STATS_IFACE_MGMT_RX = 3,
	SPRDWL_LL_STATS_IFACE_MGMT_ACTION_RX = 4,
	SPRDWL_LL_STATS_IFACE_MGMT_ACTION_TX = 5,
	SPRDWL_LL_STATS_IFACE_RSSI_MGMT = 6,
	SPRDWL_LL_STATS_IFACE_RSSI_DATA = 7,
	SPRDWL_LL_STATS_IFACE_RSSI_ACK = 8,
	SPRDWL_LL_STATS_IFACE_INFO_MODE = 9,
	SPRDWL_LL_STATS_IFACE_INFO_MAC_ADDR = 10,
	SPRDWL_LL_STATS_IFACE_INFO_STATE = 11,
	SPRDWL_LL_STATS_IFACE_INFO_ROAMING = 12,
	SPRDWL_LL_STATS_IFACE_INFO_CAPABILITIES = 13,
	SPRDWL_LL_STATS_IFACE_INFO_SSID = 14,
	SPRDWL_LL_STATS_IFACE_INFO_BSSID = 15,
	SPRDWL_LL_STATS_IFACE_INFO_AP_COUNTRY_STR = 16,
	SPRDWL_LL_STATS_IFACE_INFO_COUNTRY_STR = 17,
	SPRDWL_LL_STATS_WMM_AC_AC = 18,
	SPRDWL_LL_STATS_WMM_AC_TX_MPDU = 19,
	SPRDWL_LL_STATS_WMM_AC_RX_MPDU = 20,
	SPRDWL_LL_STATS_WMM_AC_TX_MCAST = 21,
	SPRDWL_LL_STATS_WMM_AC_RX_MCAST = 22,
	SPRDWL_LL_STATS_WMM_AC_RX_AMPDU = 23,
	SPRDWL_LL_STATS_WMM_AC_TX_AMPDU = 24,
	SPRDWL_LL_STATS_WMM_AC_MPDU_LOST = 25,
	SPRDWL_LL_STATS_WMM_AC_RETRIES = 26,
	SPRDWL_LL_STATS_WMM_AC_RETRIES_SHORT = 27,
	SPRDWL_LL_STATS_WMM_AC_RETRIES_LONG = 28,
	SPRDWL_LL_STATS_WMM_AC_CONTENTION_TIME_MIN = 29,
	SPRDWL_LL_STATS_WMM_AC_CONTENTION_TIME_MAX = 30,
	SPRDWL_LL_STATS_WMM_AC_CONTENTION_TIME_AVG = 31,
	SPRDWL_LL_STATS_WMM_AC_CONTENTION_NUM_SAMPLES = 32,
	SPRDWL_LL_STATS_IFACE_NUM_PEERS = 33,
	SPRDWL_LL_STATS_PEER_INFO_TYPE = 34,
	SPRDWL_LL_STATS_PEER_INFO_MAC_ADDRESS = 35,
	SPRDWL_LL_STATS_PEER_INFO_CAPABILITIES = 36,
	SPRDWL_LL_STATS_PEER_INFO_NUM_RATES = 37,
	SPRDWL_LL_STATS_RATE_PREAMBLE = 38,
	SPRDWL_LL_STATS_RATE_NSS = 39,
	SPRDWL_LL_STATS_RATE_BW = 40,
	SPRDWL_LL_STATS_RATE_MCS_INDEX = 41,
	SPRDWL_LL_STATS_RATE_BIT_RATE = 42,
	SPRDWL_LL_STATS_RATE_TX_MPDU = 43,
	SPRDWL_LL_STATS_RATE_RX_MPDU = 44,
	SPRDWL_LL_STATS_RATE_MPDU_LOST = 45,
	SPRDWL_LL_STATS_RATE_RETRIES = 46,
	SPRDWL_LL_STATS_RATE_RETRIES_SHORT = 47,
	SPRDWL_LL_STATS_RATE_RETRIES_LONG = 48,
	SPRDWL_LL_STATS_RADIO_ID = 49,
	SPRDWL_LL_STATS_RADIO_ON_TIME = 50,
	SPRDWL_LL_STATS_RADIO_TX_TIME = 51,
	SPRDWL_LL_STATS_RADIO_RX_TIME = 52,
	SPRDWL_LL_STATS_RADIO_ON_TIME_SCAN = 53,
	SPRDWL_LL_STATS_RADIO_ON_TIME_NBD = 54,
	SPRDWL_LL_STATS_RADIO_ON_TIME_GSCAN = 55,
	SPRDWL_LL_STATS_RADIO_ON_TIME_ROAM_SCAN = 56,
	SPRDWL_LL_STATS_RADIO_ON_TIME_PNO_SCAN = 57,
	SPRDWL_LL_STATS_RADIO_ON_TIME_HS20 = 58,
	SPRDWL_LL_STATS_RADIO_NUM_CHANNELS = 59,
	SPRDWL_LL_STATS_CHANNEL_INFO_WIDTH = 60,
	SPRDWL_LL_STATS_CHANNEL_INFO_CENTER_FREQ = 61,
	SPRDWL_LL_STATS_CHANNEL_INFO_CENTER_FREQ0 = 62,
	SPRDWL_LL_STATS_CHANNEL_INFO_CENTER_FREQ1 = 63,
	SPRDWL_LL_STATS_CHANNEL_ON_TIME = 64,
	SPRDWL_LL_STATS_CHANNEL_CCA_BUSY_TIME = 65,
	SPRDWL_LL_STATS_NUM_RADIOS = 66,
	SPRDWL_LL_STATS_CH_INFO = 67,
	SPRDWL_LL_STATS_PEER_INFO = 68,
	SPRDWL_LL_STATS_PEER_INFO_RATE_INFO = 69,
	SPRDWL_LL_STATS_WMM_INFO = 70,
	SPRDWL_LL_STATS_RESULTS_MORE_DATA = 71,
	SPRDWL_LL_STATS_IFACE_AVERAGE_TSF_OFFSET = 72,
	SPRDWL_LL_STATS_IFACE_LEAKY_AP_DETECTED = 73,
	SPRDWL_LL_STATS_IFACE_LEAKY_AP_AVG_NUM_FRAMES_LEAKED = 74,
	SPRDWL_LL_STATS_IFACE_LEAKY_AP_GUARD_TIME = 75,
	SPRDWL_LL_STATS_TYPE = 76,
	SPRDWL_LL_STATS_RADIO_NUM_TX_LEVELS = 77,
	SPRDWL_LL_STATS_RADIO_TX_TIME_PER_LEVEL = 78,
	SPRDWL_LL_STATS_IFACE_RTS_SUCC_CNT = 79,
	SPRDWL_LL_STATS_IFACE_RTS_FAIL_CNT = 80,
	SPRDWL_LL_STATS_IFACE_PPDU_SUCC_CNT = 81,
	SPRDWL_LL_STATS_IFACE_PPDU_FAIL_CNT = 82,

	/* keep last */
	SPRDWL_LL_STATS_AFTER_LAST,
	SPRDWL_LL_STATS_MAX =
	SPRDWL_LL_STATS_AFTER_LAST - 1,
};

enum sprdwl_wlan_vendor_attr_ll_stats_type {
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_INVALID = 0,
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_RADIO = 1,
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_IFACE = 2,
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_PEERS = 3,

	/* keep last */
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_AFTER_LAST,
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_MAX =
	SPRDWL_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_AFTER_LAST - 1,
};

enum sprdwl_attr_ll_stats_clr {
	SPRDWL_LL_STATS_CLR_INVALID = 0,
	SPRDWL_LL_STATS_CLR_CONFIG_REQ_MASK = 1,
	SPRDWL_LL_STATS_CLR_CONFIG_STOP_REQ = 2,
	SPRDWL_LL_STATS_CLR_CONFIG_RSP_MASK = 3,
	SPRDWL_LL_STATS_CLR_CONFIG_STOP_RSP = 4,
	/* keep last */
	SPRDWL_LL_STATS_CLR_AFTER_LAST,
	SPRDWL_LL_STATS_CLR_MAX =
	SPRDWL_LL_STATS_CLR_AFTER_LAST - 1,
};

/*end of link layer stats*/

/*start of gscan----CMD ID:23*/
struct sprdwl_gscan_capa {
	s16 max_scan_cache_size;
	u8 max_scan_buckets;
	u8 max_ap_cache_per_scan;
	u8 max_rssi_sample_size;
	u8 max_scan_reporting_threshold;
	u8 max_hotlist_bssids;
	u8 max_hotlist_ssids;
	u8 max_significant_wifi_change_aps;
	u8 max_bssid_history_entries;
	u8 max_number_epno_networks;
	u8 max_number_epno_networks_by_ssid;
	u8 max_whitelist_ssid;
	u8 max_blacklist_size;
};

struct sprdwl_roam_capa {
	u32 max_blacklist_size;
	u32 max_whitelist_size;
};

enum sprdwl_vendor_attr_gscan_results {
	GSCAN_RESULTS_INVALID = 0,

	/* Unsigned 32-bit value; must match the request Id supplied by
	 * Wi-Fi HAL in the corresponding subcmd NL msg.
	 */
	GSCAN_RESULTS_REQUEST_ID = 1,

	/* Unsigned 32-bit value; used to indicate the status response from
	 * firmware/driver for the vendor sub-command.
	 */
	GSCAN_STATUS = 2,

	/* GSCAN Valid Channels attributes */
	/* Unsigned 32bit value; followed by a nested array of CHANNELS. */
	GSCAN_RESULTS_NUM_CHANNELS = 3,
	/* An array of NUM_CHANNELS x unsigned 32-bit value integers
	 * representing channel numbers.
	 */
	GSCAN_RESULTS_CHANNELS = 4,

	/* GSCAN Capabilities attributes */
	/* Unsigned 32-bit value */
	GSCAN_SCAN_CACHE_SIZE = 5,
	/* Unsigned 32-bit value */
	GSCAN_MAX_SCAN_BUCKETS = 6,
	/* Unsigned 32-bit value */
	GSCAN_MAX_AP_CACHE_PER_SCAN = 7,
	/* Unsigned 32-bit value */
	GSCAN_MAX_RSSI_SAMPLE_SIZE = 8,
	/* Signed 32-bit value */
	GSCAN_MAX_SCAN_REPORTING_THRESHOLD = 9,
	/* Unsigned 32-bit value */
	GSCAN_MAX_HOTLIST_BSSIDS = 10,
	/* Unsigned 32-bit value */
	GSCAN_MAX_SIGNIFICANT_WIFI_CHANGE_APS = 11,
	/* Unsigned 32-bit value */
	GSCAN_MAX_BSSID_HISTORY_ENTRIES = 12,

	/* GSCAN Attributes used with
	 * GSCAN_SCAN_RESULTS_AVAILABLE sub-command.
	 */

	/* Unsigned 32-bit value */
	GSCAN_RESULTS_NUM_RESULTS_AVAILABLE = 13,

	/* GSCAN attributes used with
	 * GSCAN_FULL_SCAN_RESULT sub-command.
	 */

	/* An array of NUM_RESULTS_AVAILABLE x
	 * GSCAN_RESULTS_SCAN_RESULT_*
	 */
	GSCAN_RESULTS_LIST = 14,

	/* Unsigned 64-bit value; age of sample at the time of retrieval */
	GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP = 15,
	/* 33 x unsigned 8-bit value; NULL terminated SSID */
	GSCAN_RESULTS_SCAN_RESULT_SSID = 16,
	/* An array of 6 x unsigned 8-bit value */
	GSCAN_RESULTS_SCAN_RESULT_BSSID = 17,
	/* Unsigned 32-bit value; channel frequency in MHz */
	GSCAN_RESULTS_SCAN_RESULT_CHANNEL = 18,
	/* Signed 32-bit value */
	GSCAN_RESULTS_SCAN_RESULT_RSSI = 19,
	/* Unsigned 32-bit value */
	GSCAN_RESULTS_SCAN_RESULT_RTT = 20,
	/* Unsigned 32-bit value */
	GSCAN_RESULTS_SCAN_RESULT_RTT_SD = 21,
	/* Unsigned 16-bit value */
	GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD = 22,
	/* Unsigned 16-bit value */
	GSCAN_RESULTS_SCAN_RESULT_CAPABILITY = 23,
	/* Unsigned 32-bit value; size of the IE DATA blob */
	GSCAN_RESULTS_SCAN_RESULT_IE_LENGTH = 24,
	/* An array of IE_LENGTH x unsigned 8-bit value; blob of all the
	 * information elements found in the beacon; this data should be a
	 * packed list of wifi_information_element objects, one after the
	 * other.
	 */
	GSCAN_RESULTS_SCAN_RESULT_IE_DATA = 25,

	/* Unsigned 8-bit value; set by driver to indicate more scan results are
	 * available.
	 */
	GSCAN_RESULTS_SCAN_RESULT_MORE_DATA = 26,

	/* GSCAN attributes for
	 * GSCAN_SCAN_EVENT sub-command.
	 */
	/* Unsigned 8-bit value */
	GSCAN_RESULTS_SCAN_EVENT_TYPE = 27,
	/* Unsigned 32-bit value */
	GSCAN_RESULTS_SCAN_EVENT_STATUS = 28,

	/* GSCAN attributes for
	 * GSCAN_HOTLIST_AP_FOUND sub-command.
	 */
	/* Use attr GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
	 * to indicate number of results.
	 * Also, use GSCAN_RESULTS_LIST to indicate the
	 * list of results.
	 */

	/* GSCAN attributes for
	 * GSCAN_SIGNIFICANT_CHANGE sub-command.
	 */
	/* An array of 6 x unsigned 8-bit value */
	GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID = 29,
	/* Unsigned 32-bit value */
	GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL = 30,
	/* Unsigned 32-bit value. */
	GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI = 31,
	/* A nested array of signed 32-bit RSSI values. Size of the array is
	 * determined by (NUM_RSSI of SIGNIFICANT_CHANGE_RESULT_NUM_RSSI.
	 */
	GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST = 32,

	/* GSCAN attributes used with
	 * GSCAN_GET_CACHED_RESULTS sub-command.
	 */
	/* Use attr GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
	 * to indicate number of gscan cached results returned.
	 * Also, use GSCAN_CACHED_RESULTS_LIST to indicate
	 *  the list of gscan cached results.
	 */

	/* An array of NUM_RESULTS_AVAILABLE x
	 * NL80211_VENDOR_ATTR_GSCAN_CACHED_RESULTS_*
	 */
	GSCAN_CACHED_RESULTS_LIST = 33,
	/* Unsigned 32-bit value; a unique identifier for the scan unit. */
	GSCAN_CACHED_RESULTS_SCAN_ID = 34,
	/* Unsigned 32-bit value; a bitmask w/additional information about scan.
	 */
	GSCAN_CACHED_RESULTS_FLAGS = 35,
	/* Use attr GSCAN_ATTR_RESULTS_NUM_RESULTS_AVAILABLE
	 * to indicate number of wifi scan results/bssids retrieved by the scan.
	 * Also, use GSCAN_ATTR_RESULTS_LIST to indicate the
	 * list of wifi scan results returned for each cached result block.
	 */

	/* GSCAN attributes for
	 * NL80211_VENDOR_SUBCMD_PNO_NETWORK_FOUND sub-command.
	 */
	/* Use GSCAN_ATTR_RESULTS_NUM_RESULTS_AVAILABLE for
	 * number of results.
	 * Use GSCAN_ATTR_RESULTS_LIST to indicate the nested
	 * list of wifi scan results returned for each
	 * wifi_passpoint_match_result block.
	 * Array size: GSCAN_ATTR_RESULTS_NUM_RESULTS_AVAILABLE.
	 */

	/* GSCAN attributes for
	 * NL80211_VENDOR_SUBCMD_PNO_PASSPOINT_NETWORK_FOUND sub-command.
	 */
	/* Unsigned 32-bit value */
	GSCAN_PNO_RESULTS_PASSPOINT_NETWORK_FOUND_NUM_MATCHES = 36,
	/* A nested array of
	 * GSCAN_ATTR_PNO_RESULTS_PASSPOINT_MATCH_*
	 * attributes. Array size =
	 * *_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_NETWORK_FOUND_NUM_MATCHES.
	 */
	GSCAN_PNO_RESULTS_PASSPOINT_MATCH_RESULT_LIST = 37,

	/* Unsigned 32-bit value; network block id for the matched network */
	GSCAN_PNO_RESULTS_PASSPOINT_MATCH_ID = 38,
	/* Use GSCAN_ATTR_RESULTS_LIST to indicate the nested
	 * list of wifi scan results returned for each
	 * wifi_passpoint_match_result block.
	 */
	/* Unsigned 32-bit value */
	GSCAN_PNO_RESULTS_PASSPOINT_MATCH_ANQP_LEN = 39,
	/* An array size of PASSPOINT_MATCH_ANQP_LEN of unsigned 8-bit values;
	 * ANQP data in the information_element format.
	 */
	GSCAN_PNO_RESULTS_PASSPOINT_MATCH_ANQP = 40,

	/* Unsigned 32-bit value; a GSCAN Capabilities attribute. */
	GSCAN_MAX_HOTLIST_SSIDS = 41,
	/* Unsigned 32-bit value; a GSCAN Capabilities attribute. */
	GSCAN_MAX_NUM_EPNO_NETS = 42,
	/* Unsigned 32-bit value; a GSCAN Capabilities attribute. */
	GSCAN_MAX_NUM_EPNO_NETS_BY_SSID = 43,
	/* Unsigned 32-bit value; a GSCAN Capabilities attribute. */
	GSCAN_MAX_NUM_WHITELISTED_SSID = 44,

	GSCAN_RESULTS_BUCKETS_SCANNED = 45,
	/* Unsigned 32bit value; a GSCAN Capabilities attribute. */
	GSCAN_MAX_NUM_BLACKLISTED_BSSID = 46,

	/* keep last */
	GSCAN_RESULTS_AFTER_LAST,
	GSCAN_RESULTS_MAX =
	GSCAN_RESULTS_AFTER_LAST - 1,
};

enum sprdwl_vendor_attr_set_scanning_mac_oui {
	WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_INVALID = 0,
	WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI = 1,
	/* keep last */
	WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_AFTER_LAST,
	WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_MAX =
	WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_AFTER_LAST - 1,
};

/*end of gscan capability---CMD ID:23*/

/*start of get supported feature---CMD ID:38*/
/* Feature enums */
/* Basic infrastructure mode */
#define WIFI_FEATURE_INFRA              0x0001
/* Support for 5 GHz Band */
#define WIFI_FEATURE_INFRA_5G           0x0002
/* Support for GAS/ANQP */
#define WIFI_FEATURE_HOTSPOT            0x0004
/* Wifi-Direct */
#define WIFI_FEATURE_P2P                0x0008
/* Soft AP */
#define WIFI_FEATURE_SOFT_AP            0x0010
/* Google-Scan APIs */
#define WIFI_FEATURE_GSCAN              0x0020
/* Neighbor Awareness Networking */
#define WIFI_FEATURE_NAN                0x0040
/* Device-to-device RTT */
#define WIFI_FEATURE_D2D_RTT            0x0080
/* Device-to-AP RTT */
#define WIFI_FEATURE_D2AP_RTT           0x0100
/* Batched Scan (legacy) */
#define WIFI_FEATURE_BATCH_SCAN         0x0200
/* Preferred network offload */
#define WIFI_FEATURE_PNO                0x0400
/* Support for two STAs */
#define WIFI_FEATURE_ADDITIONAL_STA     0x0800
/* Tunnel directed link setup */
#define WIFI_FEATURE_TDLS               0x1000
/* Support for TDLS off channel */
#define WIFI_FEATURE_TDLS_OFFCHANNEL    0x2000
/* Enhanced power reporting */
#define WIFI_FEATURE_EPR                0x4000
/* Support for AP STA Concurrency */
#define WIFI_FEATURE_AP_STA             0x8000
/* Link layer stats collection */
#define WIFI_FEATURE_LINK_LAYER_STATS   0x10000
/* WiFi Logger */
#define WIFI_FEATURE_LOGGER             0x20000
/* WiFi PNO enhanced */
#define WIFI_FEATURE_HAL_EPNO           0x40000
/* RSSI Monitor */
#define WIFI_FEATURE_RSSI_MONITOR       0x80000
/* WiFi mkeep_alive */
#define WIFI_FEATURE_MKEEP_ALIVE        0x100000
/* ND offload configure */
#define WIFI_FEATURE_CONFIG_NDO         0x200000
/* Capture Tx transmit power levels */
#define WIFI_FEATURE_TX_TRANSMIT_POWER  0x400000
/* Enable/Disable firmware roaming */
#define WIFI_FEATURE_CONTROL_ROAMING    0x800000
/* Support Probe IE white listing */
#define WIFI_FEATURE_IE_WHITELIST       0x1000000
/* Support MAC & Probe Sequence Number randomization */
#define WIFI_FEATURE_SCAN_RAND          0x2000000

/*end of get supported feature---CMD ID:38*/

/*start of get supported feature---CMD ID:42*/

#define CDS_MAX_FEATURE_SET   8

/*enum wlan_vendor_attr_get_concurrency_matrix - get concurrency matrix*/
enum wlan_vendor_attr_get_concurrency_matrix {
	SPRDWL_ATTR_CO_MATRIX_INVALID = 0,
	SPRDWL_ATTR_CO_MATRIX_CONFIG_PARAM_SET_SIZE_MAX = 1,
	SPRDWL_ATTR_CO_MATRIX_RESULTS_SET_SIZE = 2,
	SPRDWL_ATTR_CO_MATRIX_RESULTS_SET = 3,
	SPRDWL_ATTR_CO_MATRIX_AFTER_LAST,
	SPRDWL_ATTR_CO_MATRIX_MAX =
	 SPRDWL_ATTR_CO_MATRIX_AFTER_LAST - 1,
};

/*end of get supported feature---CMD ID:42*/

/*start of get wifi info----CMD ID:61*/
enum sprdwl_attr_get_wifi_info {
	SPRDWL_ATTR_WIFI_INFO_GET_INVALID = 0,
	SPRDWL_ATTR_WIFI_INFO_DRIVER_VERSION = 1,
	SPRDWL_ATTR_WIFI_INFO_FIRMWARE_VERSION = 2,
	SPRDWL_ATTR_WIFI_INFO_GET_AFTER_LAST,
	SPRDWL_ATTR_WIFI_INFO_GET_MAX =
		SPRDWL_ATTR_WIFI_INFO_GET_AFTER_LAST - 1,
};

/*end of get wifi info----CMD ID:61*/

/*start of roaming data structure,CMD ID:64,CMD ID:9*/
enum fw_roaming_state {
	ROAMING_DISABLE,
	ROAMING_ENABLE
};

enum sprdwl_attr_roaming_config_params {
	SPRDWL_ROAM_INVALID = 0,
	SPRDWL_ROAM_SUBCMD = 1,
	SPRDWL_ROAM_REQ_ID = 2,
	SPRDWL_ROAM_WHITE_LIST_SSID_NUM_NETWORKS = 3,
	SPRDWL_ROAM_WHITE_LIST_SSID_LIST = 4,
	SPRDWL_ROAM_WHITE_LIST_SSID = 5,
	SPRDWL_ROAM_A_BAND_BOOST_THRESHOLD = 6,
	SPRDWL_ROAM_A_BAND_PENALTY_THRESHOLD = 7,
	SPRDWL_ROAM_A_BAND_BOOST_FACTOR = 8,
	SPRDWL_ROAM_A_BAND_PENALTY_FACTOR = 9,
	SPRDWL_ROAM_A_BAND_MAX_BOOST = 10,
	SPRDWL_ROAM_LAZY_ROAM_HISTERESYS = 11,
	SPRDWL_ROAM_ALERT_ROAM_RSSI_TRIGGER = 12,
	/* Attribute for set_lazy_roam */
	SPRDWL_ROAM_SET_LAZY_ROAM_ENABLE = 13,
	/* Attribute for set_lazy_roam with preferences */
	SPRDWL_ROAM_SET_BSSID_PREFS = 14,
	SPRDWL_ROAM_SET_LAZY_ROAM_NUM_BSSID = 15,
	SPRDWL_ROAM_SET_LAZY_ROAM_BSSID = 16,
	SPRDWL_ROAM_SET_LAZY_ROAM_RSSI_MODIFIER = 17,
	/* Attribute for set_blacklist bssid params */
	SPRDWL_ROAM_SET_BSSID_PARAMS = 18,
	SPRDWL_ROAM_SET_BSSID_PARAMS_NUM_BSSID = 19,
	SPRDWL_ROAM_SET_BSSID_PARAMS_BSSID = 20,
	/* keep last */
	SPRDWL_ROAM_AFTER_LAST,
	SPRDWL_ROAM_MAX =
	SPRDWL_ROAM_AFTER_LAST - 1,
};

enum sprdwl_attr_roam_subcmd {
	SPRDWL_ATTR_ROAM_SUBCMD_INVALID = 0,
	SPRDWL_ATTR_ROAM_SUBCMD_SSID_WHITE_LIST = 1,
	SPRDWL_ATTR_ROAM_SUBCMD_SET_GSCAN_ROAM_PARAMS = 2,
	SPRDWL_ATTR_ROAM_SUBCMD_SET_LAZY_ROAM = 3,
	SPRDWL_ATTR_ROAM_SUBCMD_SET_BSSID_PREFS = 4,
	SPRDWL_ATTR_ROAM_SUBCMD_SET_BSSID_PARAMS = 5,
	SPRDWL_ATTR_ROAM_SUBCMD_SET_BLACKLIST_BSSID = 6,
	/*KEEP LAST*/
	SPRDWL_ATTR_ROAM_SUBCMD_AFTER_LAST,
	SPRDWL_ATTR_ROAM_SUBCMD_MAX =
	SPRDWL_ATTR_ROAM_SUBCMD_AFTER_LAST - 1,
};

#define MAX_WHITE_SSID 4
#define MAX_BLACK_BSSID  16

struct ssid_t {
	uint32_t length;
	char ssid_str[IEEE80211_MAX_SSID_LEN];
} __packed;

struct bssid_t {
	uint8_t MAC_addr[ETH_ALEN];
} __packed;

struct roam_white_list_params {
	uint8_t num_white_ssid;
	struct ssid_t white_list[MAX_WHITE_SSID];
} __packed;

struct roam_black_list_params {
	uint8_t num_black_bssid;
	struct bssid_t black_list[MAX_BLACK_BSSID];
} __packed;

/*end of roaming data structure,CMD ID:64*/

/*RSSI monitor start */

enum sprdwl_rssi_monitor_control {
	WLAN_RSSI_MONITORING_CONTROL_INVALID = 0,
	WLAN_RSSI_MONITORING_START,
	WLAN_RSSI_MONITORING_STOP,
};

/* struct rssi_monitor_req - rssi monitoring
 * @request_id: request id
 * @session_id: session id
 * @min_rssi: minimum rssi
 * @max_rssi: maximum rssi
 * @control: flag to indicate start or stop
 */
struct rssi_monitor_req {
	uint32_t request_id;
	int8_t   min_rssi;
	int8_t   max_rssi;
	bool control;
} __packed;

struct rssi_monitor_event {
	uint32_t     request_id;
	int8_t       curr_rssi;
	uint8_t curr_bssid[ETH_ALEN];
};

#define EVENT_BUF_SIZE (1024)

enum sprdwl_attr_rssi_monitoring {
	SPRDWL_ATTR__RSSI_MONITORING_INVALID = 0,
	SPRDWL_ATTR_RSSI_MONITORING_CONTROL,
	SPRDWL_ATTR_RSSI_MONITORING_REQUEST_ID,
	SPRDWL_ATTR_RSSI_MONITORING_MAX_RSSI,
	SPRDWL_ATTR_RSSI_MONITORING_MIN_RSSI,
	/* attributes to be used/received in callback */
	SPRDWL_ATTR_RSSI_MONITORING_CUR_BSSID,
	SPRDWL_ATTR_RSSI_MONITORING_CUR_RSSI,
	/* keep last */
	SPRDWL_ATTR_RSSI_MONITORING_AFTER_LAST,
	SPRDWL_ATTR_RSSI_MONITORING_MAX =
	SPRDWL_ATTR_RSSI_MONITORING_AFTER_LAST - 1,
};

/*RSSI monitor End*/

enum wifi_connection_state {
	WIFI_DISCONNECTED = 0,
	WIFI_AUTHENTICATING = 1,
	WIFI_ASSOCIATING = 2,
	WIFI_ASSOCIATED = 3,
	WIFI_EAPOL_STARTED = 4,
	WIFI_EAPOL_COMPLETED = 5,
};

enum wifi_roam_state {
	WIFI_ROAMING_IDLE = 0,
	WIFI_ROAMING_ACTIVE = 1,
};

/* access categories */
enum wifi_traffic_ac {
	WIFI_AC_VO = 0,
	WIFI_AC_VI = 1,
	WIFI_AC_BE = 2,
	WIFI_AC_BK = 3,
	WIFI_AC_MAX = 4,
};

/* configuration params */
struct wifi_link_layer_params {
	u32 mpdu_size_threshold;
	u32 aggressive_statistics_gathering;
} __packed;

struct wifi_clr_llstat_rsp {
	u32 stats_clear_rsp_mask;
	u8 stop_rsp;
};

/* wifi rate */
struct wifi_rate {
	u32 preamble:3;
	u32 nss:2;
	u32 bw:3;
	u32 ratemcsidx:8;
	u32 reserved:16;
	u32 bitrate;
};

struct wifi_rate_stat {
	struct wifi_rate rate;
	u32 tx_mpdu;
	u32 rx_mpdu;
	u32 mpdu_lost;
	u32 retries;
	u32 retries_short;
	u32 retries_long;
};

/* per peer statistics */
struct wifi_peer_info {
	u8 type;
	u8 peer_mac_address[6];
	u32 capabilities;
	u32 num_rate;
	struct wifi_rate_stat rate_stats[];
};

struct wifi_interface_link_layer_info {
	enum sprdwl_mode mode;
	u8 mac_addr[6];
	enum wifi_connection_state state;
	enum wifi_roam_state roaming;
	u32 capabilities;
	u8 ssid[33];
	u8 bssid[6];
	u8 ap_country_str[3];
	u8 country_str[3];
};

/* Per access category statistics */
struct wifi_wmm_ac_stat {
	enum wifi_traffic_ac ac;
	u32 tx_mpdu;
	u32 rx_mpdu;
	u32 tx_mcast;
	u32 rx_mcast;
	u32 rx_ampdu;
	u32 tx_ampdu;
	u32 mpdu_lost;
	u32 retries;
	u32 retries_short;
	u32 retries_long;
	u32 contention_time_min;
	u32 contention_time_max;
	u32 contention_time_avg;
	u32 contention_num_samples;
};

/* interface statistics */
struct wifi_iface_stat {
	void *iface;
	struct wifi_interface_link_layer_info info;
	u32 beacon_rx;
	u64 average_tsf_offset;
	u32 leaky_ap_detected;
	u32 leaky_ap_avg_num_frames_leaked;
	u32 leaky_ap_guard_time;
	u32 mgmt_rx;
	u32 mgmt_action_rx;
	u32 mgmt_action_tx;
	int rssi_mgmt;
	u32 rssi_data;
	u32 rssi_ack;
	struct wifi_wmm_ac_stat ac[WIFI_AC_MAX];
	u32 num_peers;
	struct wifi_peer_info peer_info[];
};

/* WiFi Common definitions */
/* channel operating width */
enum wifi_channel_width {
	WIFI_CHAN_WIDTH_20 = 0,
	WIFI_CHAN_WIDTH_40 = 1,
	WIFI_CHAN_WIDTH_80 = 2,
	WIFI_CHAN_WIDTH_160 = 3,
	WIFI_CHAN_WIDTH_80P80 = 4,
	WIFI_CHAN_WIDTH_5 = 5,
	WIFI_CHAN_WIDTH_10 = 6,
	WIFI_CHAN_WIDTH_INVALID = -1
};

/* channel information */
struct wifi_channel_info {
	enum wifi_channel_width width;
	u32 center_freq;
	u32 center_freq0;
	u32 center_freq1;
};

/* channel statistics */
struct wifi_channel_stat {
	struct wifi_channel_info channel;
	u32 on_time;
	u32 cca_busy_time;
};

/* radio statistics */
struct wifi_radio_stat {
	u32 radio;
	u32 on_time;
	u32 tx_time;
	u32 num_tx_levels;
	u32 *tx_time_per_levels;
	u32 rx_time;
	u32 on_time_scan;
	u32 on_time_nbd;
	u32 on_time_gscan;
	u32 on_time_roam_scan;
	u32 on_time_pno_scan;
	u32 on_time_hs20;
	u32 num_channels;
	struct wifi_channel_stat channels[];
};

struct sprdwl_wmm_ac_stat {
	u32 tx_mpdu;
	u32 rx_mpdu;
	u32 tx_mpdu_lost;
	u32 tx_retries;
};

struct sprdwl_llstat_data {
	int rssi_mgmt;
	u32 bcn_rx_cnt;
	struct sprdwl_wmm_ac_stat ac[WIFI_AC_MAX];
	u32 on_time;
	u32 on_time_scan;
	u64 radio_tx_time;
	u64 radio_rx_time;
};

struct sprdwl_llstat_radio {
	int rssi_mgmt;
	u32 bcn_rx_cnt;
	struct sprdwl_wmm_ac_stat ac[WIFI_AC_MAX];
};


struct sprdwl_vendor_data {
	struct wifi_radio_stat radio_st;
	struct wifi_iface_stat iface_st;
};

/*end of link layer stats*/

/*start of wake stats---CMD ID:85*/
enum sprdwl_wake_stats {
	WLAN_ATTR_GET_WAKE_STATS_INVALID = 0,
	WLAN_ATTR_TOTAL_CMD_EVENT_WAKE,
	WLAN_ATTR_CMD_EVENT_WAKE_CNT_PTR,
	WLAN_ATTR_CMD_EVENT_WAKE_CNT_SZ,
	WLAN_ATTR_TOTAL_DRIVER_FW_LOCAL_WAKE,
	WLAN_ATTR_DRIVER_FW_LOCAL_WAKE_CNT_PTR,
	WLAN_ATTR_DRIVER_FW_LOCAL_WAKE_CNT_SZ,
	WLAN_ATTR_TOTAL_RX_DATA_WAKE,
	WLAN_ATTR_RX_UNICAST_CNT,
	WLAN_ATTR_RX_MULTICAST_CNT,
	WLAN_ATTR_RX_BROADCAST_CNT,
	WLAN_ATTR_ICMP_PKT,
	WLAN_ATTR_ICMP6_PKT,
	WLAN_ATTR_ICMP6_RA,
	WLAN_ATTR_ICMP6_NA,
	WLAN_ATTR_ICMP6_NS,
	WLAN_ATTR_ICMP4_RX_MULTICAST_CNT,
	WLAN_ATTR_ICMP6_RX_MULTICAST_CNT,
	WLAN_ATTR_OTHER_RX_MULTICAST_CNT,
	/* keep last */
	WLAN_GET_WAKE_STATS_AFTER_LAST,
	WLAN_GET_WAKE_STATS_MAX =
	    WLAN_GET_WAKE_STATS_AFTER_LAST - 1,
};

/*end of wake sats---CMD ID:85*/


/*start of SAR limit---- CMD ID:146*/
enum sprdwl_vendor_sar_limits_select {
	WLAN_SAR_LIMITS_BDF0 = 0,
	WLAN_SAR_LIMITS_BDF1 = 1,
	WLAN_SAR_LIMITS_BDF2 = 2,
	WLAN_SAR_LIMITS_BDF3 = 3,
	WLAN_SAR_LIMITS_BDF4 = 4,
	WLAN_SAR_LIMITS_NONE = 5,
	WLAN_SAR_LIMITS_USER = 6,
};

enum sprdwl_vendor_attr_sar_limits {
	WLAN_ATTR_SAR_LIMITS_INVALID = 0,
	WLAN_ATTR_SAR_LIMITS_SAR_ENABLE = 1,
	WLAN_ATTR_SAR_LIMITS_NUM_SPECS = 2,
	WLAN_ATTR_SAR_LIMITS_SPEC = 3,
	WLAN_ATTR_SAR_LIMITS_SPEC_BAND = 4,
	WLAN_ATTR_SAR_LIMITS_SPEC_CHAIN = 5,
	WLAN_ATTR_SAR_LIMITS_SPEC_MODULATION = 6,
	WLAN_ATTR_SAR_LIMITS_SPEC_POWER_LIMIT = 7,
	WLAN_ATTR_SAR_LIMITS_AFTER_LAST,
	WLAN_ATTR_SAR_LIMITS_MAX =
		WLAN_ATTR_SAR_LIMITS_AFTER_LAST - 1
};
/*end of SAR limit---CMD ID:146*/
int sprdwl_vendor_init(struct wiphy *wiphy);
int sprdwl_vendor_deinit(struct wiphy *wiphy);

#define MAX_CHANNELS 16
#define MAX_BUCKETS 4
#define MAX_HOTLIST_APS 16
#define MAX_WHITELIST_APS 16
#define MAX_PREFER_APS 16
#define MAX_BLACKLIST_BSSID 16
#define MAX_SIGNIFICANT_CHANGE_APS 16
#define MAX_AP_CACHE_PER_SCAN 32
#define MAX_CHANNLES_NUM 14

#define MAX_EPNO_NETWORKS 16

enum sprdwl_vendor_attr_gscan_config_params {
	GSCAN_ATTR_SUBCMD_CONFIG_PARAM_INVALID = 0,
	/* Unsigned 32-bit value; Middleware provides it to the driver.
	*Middle ware either gets it from caller, e.g., framework,
	or generates one if framework doesn't provide it.
	*/

	GSCAN_ATTR_SUBCMD_CONFIG_PARAM_REQUEST_ID,

	/* NL attributes for data used by
	* NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS sub command.
	*/
	/* Unsigned 32-bit value */
	GSCAN_ATTR_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
	/* Unsigned 32-bit value */
	GSCAN_ATTR_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,

	/* NL attributes for input params used by
	 * NL80211_VENDOR_SUBCMD_GSCAN_START sub command.
	*/

	/* Unsigned 32-bit value; channel frequency */
	GSCAN_ATTR_CHANNEL_SPEC_CHANNEL,
	/* Unsigned 32-bit value; dwell time in ms. */
	GSCAN_ATTR_CHANNEL_SPEC_DWELL_TIME,
	/* Unsigned 8-bit value; 0: active; 1: passive; N/A for DFS */
	GSCAN_ATTR_CHANNEL_SPEC_PASSIVE,
	/* Unsigned 8-bit value; channel class */
	GSCAN_ATTR_CHANNEL_SPEC_CLASS,

	/* Unsigned 8-bit value; bucket index, 0 based */
	GSCAN_ATTR_BUCKET_SPEC_INDEX,
	/* Unsigned 8-bit value; band. */
	GSCAN_ATTR_BUCKET_SPEC_BAND,
	/* Unsigned 32-bit value; desired period, in ms. */
	GSCAN_ATTR_BUCKET_SPEC_PERIOD = 10,
	/* Unsigned 8-bit value; report events semantics. */
	GSCAN_ATTR_BUCKET_SPEC_REPORT_EVENTS,
	/* Unsigned 32-bit value. Followed by a nested array of
	* GSCAN_CHANNEL_SPECS attributes.
	*/
	GSCAN_ATTR_BUCKET_SPEC_NUM_CHANNEL_SPECS,

	/* Array of GSCAN_ATTR_CHANNEL_SPEC_* attributes.
	* Array size: GSCAN_ATTR_BUCKET_SPEC_NUM_CHANNEL_SPECS
	*/
	GSCAN_ATTR_CHANNEL_SPEC,

	/* Unsigned 32-bit value; base timer period in ms. */
	GSCAN_ATTR_SCAN_CMD_PARAMS_BASE_PERIOD,
	/* Unsigned 32-bit value; number of APs to store in each scan in the
	* BSSID/RSSI history buffer (keep the highest RSSI APs).
	*/
	GSCAN_ATTR_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN = 15,

	/* Unsigned 8-bit value; In %, when scan buffer is this much full,
	*wake up APPS.
	*/
	GSCAN_ATTR_SCAN_CMD_PARAMS_REPORT_THR,

	/* Unsigned 8-bit value; number of scan bucket specs; followed by
	*a nested array of_GSCAN_BUCKET_SPEC_* attributes and values.
	*The size of the array is determined by NUM_BUCKETS.
	*/
	GSCAN_ATTR_SCAN_CMD_PARAMS_NUM_BUCKETS,

	/* Array of GSCAN_ATTR_BUCKET_SPEC_* attributes.
	* Array size: GSCAN_ATTR_SCAN_CMD_PARAMS_NUM_BUCKETS
	*/
	GSCAN_ATTR_BUCKET_SPEC,

	/* Unsigned 8-bit value */
	GSCAN_ATTR_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
	/* Unsigned 32-bit value; maximum number of results to be returned. */
	GSCAN_ATTR_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX = 20,

	/* An array of 6 x Unsigned 8-bit value */
	GSCAN_ATTR_AP_THR_PARAM_BSSID,
	/* Signed 32-bit value */
	GSCAN_ATTR_AP_THR_PARAM_RSSI_LOW,
	/* Signed 32-bit value */
	GSCAN_ATTR_AP_THR_PARAM_RSSI_HIGH,
	/* Unsigned 32-bit value */
	GSCAN_ATTR_AP_THR_PARAM_CHANNEL,

	/* Number of hotlist APs as unsigned 32-bit value, followed by a nested
	* array of AP_THR_PARAM attributes and values. The size of the
	* array is determined by NUM_AP.
	*/
	GSCAN_ATTR_BSSID_HOTLIST_PARAMS_NUM_AP = 25,

	/* Array of GSCAN_ATTR_AP_THR_PARAM_* attributes.
	* Array size: GSCAN_ATTR_BUCKET_SPEC_NUM_CHANNEL_SPECS
	*/
	GSCAN_ATTR_AP_THR_PARAM,

	/* Unsigned 32bit value; number of samples for averaging RSSI. */
	GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
	/* Unsigned 32bit value; number of samples to confirm AP loss. */
	GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
	/* Unsigned 32bit value; number of APs breaching threshold. */
	GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
	/* Unsigned 32bit value; number of APs. Followed by an array of
	* AP_THR_PARAM attributes. Size of the array is NUM_AP.
	*/
	GSCAN_ATTR_SIGNIFICANT_CHANGE_PARAMS_NUM_AP = 30,
	GSCAN_ATTR_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE,

	GSCAN_ATTR_BUCKET_SPEC_MAX_PERIOD = 32,
	/* Unsigned 32-bit value. */
	GSCAN_ATTR_BUCKET_SPEC_BASE = 33,
	/* Unsigned 32-bit value. For exponential back off bucket, number of
	 * scans to perform for a given period.
	 */
	GSCAN_ATTR_BUCKET_SPEC_STEP_COUNT = 34,

	GSCAN_ATTR_SCAN_CMD_PARAMS_REPORT_THR_NUM_SCANS = 35,
	/* Unsigned 3-2bit value; number of samples to confirm SSID loss. */
	GSCAN_ATTR_GSCAN_SSID_HOTLIST_PARAMS_LOST_SSID_SAMPLE_SIZE = 36,
	/* Number of hotlist SSIDs as unsigned 32-bit value, followed by a
	 * nested array of SSID_THRESHOLD_PARAM_* attributes and values. The
	 * size of the array is determined by NUM_SSID.
	 */
	GSCAN_ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID = 37,
	/* Array of GSCAN_ATTR_GSCAN_SSID_THRESHOLD_PARAM_*
	 * attributes.
	 * Array size: GSCAN_ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID
	 */
	GSCAN_ATTR_GSCAN_SSID_THR_PARAM = 38,

	/* An array of 33 x unsigned 8-bit value; NULL terminated SSID */
	GSCAN_ATTR_GSCAN_SSID_THR_PARAM_SSID = 39,
	/* Unsigned 8-bit value */
	GSCAN_ATTR_GSCAN_SSID_THR_PARAM_BAND = 40,
	/* Signed 32-bit value */
	GSCAN_ATTR_GSCAN_SSID_THR_PARAM_RSSI_LOW = 41,
	/* Signed 32-bit value */
	GSCAN_ATTR_GSCAN_SSID_THR_PARAM_RSSI_HIGH = 42,

	/* keep last */
	GSCAN_ATTR_SUBCMD_CONFIG_PARAM_AFTER_LAST,
	GSCAN_ATTR_SUBCMD_CONFIG_PARAM_MAX =
	GSCAN_ATTR_SUBCMD_CONFIG_PARAM_AFTER_LAST - 1,

};
enum nl80211_vendor_subcmds_index {
	NL80211_VENDOR_SUBCMD_LL_STATS_SET_INDEX,
	NL80211_VENDOR_SUBCMD_LL_STATS_GET_INDEX,
	NL80211_VENDOR_SUBCMD_LL_STATS_CLR_INDEX,
	NL80211_VENDOR_SUBCMD_LL_RADIO_STATS_INDEX,
	NL80211_VENDOR_SUBCMD_LL_IFACE_STATS_INDEX,
	NL80211_VENDOR_SUBCMD_LL_PEER_INFO_STATS_INDEX,
	/* GSCAN Events */
	NL80211_VENDOR_SUBCMD_GSCAN_START_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_STOP_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_LOST_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST_INDEX,
	NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST_INDEX,
	NL80211_VENDOR_SUBCMD_SIGNIFICANT_CHANGE_INDEX,
	NL80211_VENDOR_SUBCMD_SET_SIGNIFICANT_CHANGE_INDEX,
	NL80211_VENDOR_SUBCMD_RESET_SIGNIFICANT_CHANGE_INDEX,
	/*EXT TDLS*/
	NL80211_VENDOR_SUBCMD_TDLS_STATE_CHANGE_INDEX,
	NL80211_VENDOR_SUBCMD_NAN_INDEX,
};
enum sprdwl_gscan_attribute {
	GSCAN_NUM_BUCKETS = 1,
	GSCAN_BASE_PERIOD,
	GSCAN_BUCKETS_BAND,
	GSCAN_BUCKET_ID,
	GSCAN_BUCKET_PERIOD,
	GSCAN_BUCKET_NUM_CHANNELS,
	GSCAN_BUCKET_CHANNELS,
	GSCAN_BUCKET_SPEC,
	GSCAN_BUCKET_CHANNELS_SPEC,
	GSCAN_CH_DWELL_TIME,
	GSCAN_CH_PASSIVE,
	GSCAN_NUM_AP_PER_SCAN,
	GSCAN_REPORT_THRESHOLD,
	GSCAN_NUM_SCANS_TO_CACHE,
	GSCAN_BAND = GSCAN_BUCKETS_BAND,

	GSCAN_ENABLE_FEATURE,
	GSCAN_SCAN_RESULTS_COMPLETE,	/* indicates no more results */
	GSCAN_FLUSH_FEATURE,	/* Flush all the configs */
	GSCAN_FULL_SCAN_RESULTS,
	GSCAN_REPORT_EVENTS,

	/* remaining reserved for additional attributes */
	GSCAN_NUM_OF_RESULTS,
	GSCAN_FLUSH_RESULTS,
	GSCAN_ATT_SCAN_RESULTS,	/* flat array of wifi_scan_result */
	GSCAN_SCAN_ID,	/* indicates scan number */
	GSCAN_SCAN_FLAGS,	/* indicates if scan was aborted */
	GSCAN_AP_FLAGS,	/* flags on significant change event */
	GSCAN_NUM_CHANNELS,
	GSCAN_CHANNEL_LIST,

	/* remaining reserved for additional attributes */

	/* Adaptive scan attributes */
	GSCAN_BUCKET_STEP_COUNT,
	GSCAN_BUCKET_MAX_PERIOD,

	GSCAN_SSID,
	GSCAN_BSSID,
	GSCAN_CHANNEL,
	GSCAN_RSSI,
	GSCAN_TIMESTAMP,
	GSCAN_RTT,
	GSCAN_RTTSD,

	/* remaining reserved for additional attributes */

	GSCAN_HOTLIST_BSSIDS,
	GSCAN_RSSI_LOW,
	GSCAN_RSSI_HIGH,
	GSCAN_HOTLIST_ELEM,
	GSCAN_HOTLIST_FLUSH,

	/* remaining reserved for additional attributes */
	GSCAN_RSSI_SAMPLE_SIZE,
	GSCAN_LOST_AP_SAMPLE_SIZE,
	GSCAN_MIN_BREACHING,
	GSCAN_SIGNIFICANT_CHANGE_BSSIDS,
	GSCAN_SIGNIFICANT_CHANGE_FLUSH,

	/* remaining reserved for additional attributes */

	GSCAN_WHITELIST_SSID,
	GSCAN_NUM_WL_SSID,
	/*GSCAN_WL_SSID_LEN,*/
	GSCAN_WL_SSID_FLUSH,
	GSCAN_WHITELIST_SSID_ELEM,
	GSCAN_NUM_BSSID,
	GSCAN_BSSID_PREF_LIST,
	GSCAN_BSSID_PREF_FLUSH,
	GSCAN_BSSID_PREF,
	GSCAN_RSSI_MODIFIER,

	/* remaining reserved for additional attributes */

	GSCAN_A_BAND_BOOST_THRESHOLD,
	GSCAN_A_BAND_PENALTY_THRESHOLD,
	GSCAN_A_BAND_BOOST_FACTOR,
	GSCAN_A_BAND_PENALTY_FACTOR,
	GSCAN_A_BAND_MAX_BOOST,
	GSCAN_LAZY_ROAM_HYSTERESIS,
	GSCAN_ALERT_ROAM_RSSI_TRIGGER,
	GSCAN_LAZY_ROAM_ENABLE,

	/* BSSID blacklist */
	GSCAN_BSSID_BLACKLIST_FLUSH,
	GSCAN_BLACKLIST_BSSID,
	GSCAN_BLACKLIST_BSSID_SPEC,

	/* ANQPO */
	GSCAN_ANQPO_HS_LIST,
	GSCAN_ANQPO_HS_LIST_SIZE,
	GSCAN_ANQPO_HS_NETWORK_ID,
	GSCAN_ANQPO_HS_NAI_REALM,
	GSCAN_ANQPO_HS_ROAM_CONSORTIUM_ID,
	GSCAN_ANQPO_HS_PLMN,

	GSCAN_SSID_HOTLIST_FLUSH,
	GSCAN_SSID_LOST_SAMPLE_SIZE,
	GSCAN_HOTLIST_SSIDS,
	GSCAN_SSID_RSSI_HIGH,
	GSCAN_SSID_RSSI_LOW,
	GSCAN_ANQPO_LIST_FLUSH,
	ANDR_WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI,

	GSCAN_MAX
};

enum SPRD_wlan_vendor_attr_pno_config_params {
	SPRD_WLAN_VENDOR_ATTR_PNO_INVALID = 0,
	/* Attributes for data used by
	 * SPRD_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST sub command.
	 */
	/* Unsigned 32-bit value */
	SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NUM = 1,
	/* Array of nested SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_*
	 * attributes. Array size =
	 * SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NUM.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NETWORK_ARRAY = 2,

	/* Unsigned 32-bit value */
	SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ID = 3,
	/* An array of 256 x unsigned 8-bit value; NULL terminated UTF-8 encoded
	 * realm, 0 if unspecified.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_REALM = 4,
	/* An array of 16 x unsigned 32-bit value; roaming consortium ids to
	 * match, 0 if unspecified.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_CNSRTM_ID = 5,
	/* An array of 6 x unsigned 8-bit value; MCC/MNC combination, 0s if
	 * unspecified.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_PLMN = 6,

	/* Attributes for data used by
	 * SPRD_NL80211_VENDOR_SUBCMD_PNO_SET_LIST sub command.
	 */
	/* Unsigned 32-bit value */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS = 7,
	/* Array of nested
	 * SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_*
	 * attributes. Array size =
	 * SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST = 8,
	/* An array of 33 x unsigned 8-bit value; NULL terminated SSID */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_SSID = 9,
	/* Signed 8-bit value; threshold for considering this SSID as found,
	 * required granularity for this threshold is 4 dBm to 8 dBm.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_RSSI_THRESHOLD
	= 10,
	/* Unsigned 8-bit value; WIFI_PNO_FLAG_XXX */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_FLAGS = 11,
	/* Unsigned 8-bit value; auth bit field for matching WPA IE */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_AUTH_BIT = 12,
	/* Unsigned 8-bit to indicate ePNO type;
	 * It takes values from SPRD_wlan_epno_type
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_TYPE = 13,

	/* Nested attribute to send the channel list */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_CHANNEL_LIST = 14,

	/* Unsigned 32-bit value; indicates the interval between PNO scan
	 * cycles in msec.
	 */
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_SCAN_INTERVAL = 15,
	SPRD_WLAN_VENDOR_ATTR_EPNO_MIN5GHZ_RSSI = 16,
	SPRD_WLAN_VENDOR_ATTR_EPNO_MIN24GHZ_RSSI = 17,
	SPRD_WLAN_VENDOR_ATTR_EPNO_INITIAL_SCORE_MAX = 18,
	SPRD_WLAN_VENDOR_ATTR_EPNO_CURRENT_CONNECTION_BONUS = 19,
	SPRD_WLAN_VENDOR_ATTR_EPNO_SAME_NETWORK_BONUS = 20,
	SPRD_WLAN_VENDOR_ATTR_EPNO_SECURE_BONUS = 21,
	SPRD_WLAN_VENDOR_ATTR_EPNO_BAND5GHZ_BONUS = 22,

	/* keep last */
	SPRD_WLAN_VENDOR_ATTR_PNO_AFTER_LAST,
	SPRD_WLAN_VENDOR_ATTR_PNO_MAX =
	SPRD_WLAN_VENDOR_ATTR_PNO_AFTER_LAST - 1,
};

#define SPRDWL_EPNO_PARAM_NETWORK_SSID \
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_SSID
#define SPRDWL_EPNO_PARAM_MIN5GHZ_RSSI \
	SPRD_WLAN_VENDOR_ATTR_EPNO_MIN5GHZ_RSSI
#define SPRDWL_EPNO_PARAM_MIN24GHZ_RSSI  \
	SPRD_WLAN_VENDOR_ATTR_EPNO_MIN24GHZ_RSSI
#define SPRDWL_EPNO_PARAM_INITIAL_SCORE_MAX \
	SPRD_WLAN_VENDOR_ATTR_EPNO_INITIAL_SCORE_MAX
#define SPRDWL_EPNO_PARAM_CURRENT_CONNECTION_BONUS \
	SPRD_WLAN_VENDOR_ATTR_EPNO_CURRENT_CONNECTION_BONUS
#define SPRDWL_EPNO_PARAM_SAME_NETWORK_BONUS \
	SPRD_WLAN_VENDOR_ATTR_EPNO_SAME_NETWORK_BONUS
#define SPRDWL_EPNO_PARAM_SECURE_BONUS \
	SPRD_WLAN_VENDOR_ATTR_EPNO_SECURE_BONUS
#define SPRDWL_EPNO_PARAM_BAND5GHZ_BONUS \
	SPRD_WLAN_VENDOR_ATTR_EPNO_BAND5GHZ_BONUS
#define SPRDWL_EPNO_PARAM_NUM_NETWORKS \
	 SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS
#define SPRDWL_EPNO_PARAM_NETWORKS_LIST \
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST

#define SPRDWL_EPNO_PARAM_NETWORK_SSID \
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_SSID
#define SPRDWL_EPNO_PARAM_NETWORK_FLAGS \
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_FLAGS
#define SPRDWL_EPNO_PARAM_NETWORK_AUTH_BIT \
	SPRD_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_AUTH_BIT
#define SPRDWL_VENDOR_EVENT_EPNO_FOUND \
	SPRDWL_VENDOR_SUBCMD_PNO_NETWORK_FOUND

enum sprdwl_gscan_wifi_band {
	WIFI_BAND_UNSPECIFIED,
	WIFI_BAND_BG = 1,
	WIFI_BAND_A = 2,
	WIFI_BAND_A_DFS = 4,
	WIFI_BAND_A_WITH_DFS = 6,
	WIFI_BAND_ABG = 3,
	WIFI_BAND_ABG_WITH_DFS = 7,
};

enum sprdwl_gscan_wifi_event {
	SPRD_RESERVED1,
	SPRD_RESERVED2,
	GSCAN_EVENT_SIGNIFICANT_CHANGE_RESULTS,
	GSCAN_EVENT_HOTLIST_RESULTS_FOUND,
	GSCAN_EVENT_SCAN_RESULTS_AVAILABLE,
	GSCAN_EVENT_FULL_SCAN_RESULTS,
	RTT_EVENT_COMPLETE,
	GSCAN_EVENT_COMPLETE_SCAN,
	GSCAN_EVENT_HOTLIST_RESULTS_LOST,
	GSCAN_EVENT_EPNO_EVENT,
	GOOGLE_DEBUG_RING_EVENT,
	GOOGLE_DEBUG_MEM_DUMP_EVENT,
	GSCAN_EVENT_ANQPO_HOTSPOT_MATCH,
	GOOGLE_RSSI_MONITOR_EVENT,
	GSCAN_EVENT_SSID_HOTLIST_RESULTS_FOUND,
	GSCAN_EVENT_SSID_HOTLIST_RESULTS_LOST,

};

struct sprdwl_gscan_channel_spec {
	u8 channel;
	u8 dwelltime;
	u8 passive;
	u8 resv;
};

#define REPORT_EVENTS_BUFFER_FULL      0
#define REPORT_EVENTS_EACH_SCAN        (1<<0)
#define REPORT_EVENTS_FULL_RESULTS     (1<<1)
#define REPORT_EVENTS_NO_BATCH         (1<<2)

#define REPORT_EVENTS_HOTLIST_RESULTS_FOUND         (1<<3)
#define REPORT_EVENTS_HOTLIST_RESULTS_LOST         (1<<4)
#define REPORT_EVENTS_SIGNIFICANT_CHANGE         (1<<5)
#define REPORT_EVENTS_EPNO         (1<<6)
#define REPORT_EVENTS_ANQPO_HOTSPOT_MATCH         (1<<7)
#define REPORT_EVENTS_SSID_HOTLIST_RESULTS_FOUND         (1<<8)
#define REPORT_EVENTS_SSID_HOTLIST_RESULTS_LOST         (1<<9)

enum sprdwl_gscan_event {
	WIFI_SCAN_BUFFER_FULL,
	WIFI_SCAN_COMPLETE,
};

struct sprdwl_gscan_bucket_spec {
	u8 bucket;
	u8 band;
	u8 num_channels;
	u8 base;
	u8 step_count;
	u8 reserved;	/*reserved for data align*/
	u16 report_events;
	u32 period;
	u32 max_period;
	struct sprdwl_gscan_channel_spec channels[MAX_CHANNELS];
};

struct sprdwl_cmd_gscan_set_config {
	u32 base_period;
	u8 maxAPperScan;
	u8 reportThreshold;
	u8 report_threshold_num_scans;
	u8 num_buckets;
	struct sprdwl_gscan_bucket_spec buckets[MAX_BUCKETS];
};

struct sprdwl_cmd_gscan_rsp_header {
	u8 subcmd;
	u8 status;
	u16 data_len;
} __packed;

struct sprdwl_cmd_gscan_channel_list {
	int num_channels;
	int channels[TOTAL_2G_5G_CHANNEL_NUM];
};

struct sprdwl_gscan_result {
	unsigned long ts;
	char ssid[32 + 1];
	char bssid[ETH_ALEN];
	u8 channel;
	s8 rssi;
	u32 rtt;
	u32 rtt_sd;
	u16 beacon_period;
	u16 capability;
	u16 ie_length;
	char ie_data[1];
} __packed;

struct sprdwl_gscan_cached_results {
	u8 scan_id;
	u8 flags;
	int num_results;
	struct sprdwl_gscan_result results[MAX_AP_CACHE_PER_SCAN];
};

struct sprdwl_gscan_hotlist_results {
	int req_id;
	u8 flags;
	int num_results;
	struct sprdwl_gscan_result results[MAX_HOTLIST_APS];
};

struct ssid_threshold_param {
	unsigned char ssid[IEEE80211_MAX_SSID_LEN];	/* AP SSID*/
	s8 low;		/* low threshold*/
	s8 high;		/* low threshold*/
};

struct wifi_ssid_hotlist_params {
	u8 lost_ssid_sample_size; /* number of samples to confirm AP loss*/
	u8 num_ssid;	/* number of hotlist APs*/
	struct ssid_threshold_param ssid[MAX_HOTLIST_APS];	/* hotlist APs*/
};

struct ap_threshold_param {
	unsigned char bssid[6];	/* AP BSSID*/
	s8 low;		/* low threshold*/
	s8 high;		/* low threshold*/
};

struct wifi_bssid_hotlist_params {
	u8 lost_ap_sample_size; /* number of samples to confirm AP loss*/
	u8 num_bssid;	/* number of hotlist APs*/
	struct ap_threshold_param ap[MAX_HOTLIST_APS];	/* hotlist APs*/
};

struct wifi_significant_change_params {
	u8 rssi_sample_size; /*number of samples for averaging RSSI */
	u8 lost_ap_sample_size;/*number of samples to confirm AP loss*/
	u8 min_breaching;/*number of APs breaching threshold */
	u8 num_bssid;/*max 64*/
	struct ap_threshold_param ap[MAX_SIGNIFICANT_CHANGE_APS];
};

struct significant_change_info {
	unsigned char bssid[6];	/* AP BSSID*/
	u8 channel;		/*channel frequency in MHz*/
	u8 num_rssi;			/*number of rssi samples*/
	s8 rssi[3];		/*RSSI history in db, here fixed 3*/
} __packed;

struct sprdwl_significant_change_result {
	int req_id;
	u8 flags;
	int num_results;
	struct significant_change_info results[MAX_SIGNIFICANT_CHANGE_APS];
};

struct wifi_epno_network {
	u8 ssid[32 + 1];
	/* threshold for considering this SSID as found required
	 *granularity for this threshold is 4dBm to 8dBm
	 */
	/* unsigned char rssi_threshold; */
	u8 flags; /* WIFI_PNO_FLAG_XXX*/
	u8 auth_bit_field; /* auth bit field for matching WPA IE*/
} __packed;

struct wifi_epno_params {
	u64 boot_time;
	u8 request_id;
	/* minimum 5GHz RSSI for a BSSID to be considered */
	s8 min5ghz_rssi;

	/* minimum 2.4GHz RSSI for a BSSID to be considered */
	s8 min24ghz_rssi;

	/* the maximum score that a network can have before bonuses */
	s8 initial_score_max;

	/* only report when there is a network's score this much higher */
    /* than the current connection. */
	s8 current_connection_bonus;

	/* score bonus for all networks with the same network flag */
	s8 same_network_bonus;

	/* score bonus for networks that are not open */
	s8 secure_bonus;

	/* 5GHz RSSI score bonus (applied to all 5GHz networks) */
	s8 band5ghz_bonus;

	/* number of wifi_epno_network objects */
	s8 num_networks;

	/* PNO networks */
	struct wifi_epno_network networks[MAX_EPNO_NETWORKS];
} __packed;

struct sprdwl_epno_results {
	u64 boot_time;
	u8 request_id;
	u8 nr_scan_results;
	struct sprdwl_gscan_result results[0];
} __packed;

struct wifi_ssid {
	char ssid[32+1]; /* null terminated*/
};


struct wifi_roam_params {
    /* Lazy roam parameters
     * A_band_XX parameters are applied to 5GHz BSSIDs when comparing with
     * a 2.4GHz BSSID they may not be applied when comparing two 5GHz BSSIDs
	 */

	/* RSSI threshold above which 5GHz RSSI is favored*/
	int A_band_boost_threshold;

	/* RSSI threshold below which 5GHz RSSI is penalized*/
	int A_band_penalty_threshold;

	/* factor by which 5GHz RSSI is boosted*/
	/*boost=RSSI_measured-5GHz_boost_threshold)*5GHz_boost_factor*/
	int A_band_boost_factor;

	/* factor by which 5GHz RSSI is penalized*/
	/*penalty=(5GHz_penalty_factor-RSSI_measured)*5GHz_penalty_factor*/
	int A_band_penalty_factor;

	/* maximum boost that can be applied to a 5GHz RSSI*/
	int A_band_max_boost;

	/* Hysteresis: ensuring the currently associated BSSID is favored*/
	/*so as to prevent ping-pong situations,boost applied to current BSSID*/
	int lazy_roam_hysteresis;

	/* Alert mode enable, i.e. configuring when firmware enters alert mode*/
	/* RSSI below which "Alert" roam is enabled*/
	int alert_roam_rssi_trigger;
};

struct wifi_bssid_preference {
	unsigned char bssid[6];
	/* modifier applied to the RSSI of the BSSID for
	 *the purpose of comparing it with other roam candidate
	 */
	int rssi_modifier;
};

struct wifi_bssid_preference_params {
	int num_bssid;	/* number of preference APs*/
	/* preference APs*/
	struct wifi_bssid_preference pref_ap[MAX_PREFER_APS];
};

struct v_MACADDR_t {
	u8 bytes[3];
};

struct wifi_passpoint_network {
	/*identifier of this network block, report this in event*/
	u8  id;
	/*null terminated UTF8 encoded realm, 0 if unspecified*/
	char realm[256];
	/*roaming consortium ids to match, 0s if unspecified*/
	int64_t roaming_ids[16];
	/*mcc/mnc combination as per rules, 0s if unspecified*/
	unsigned char plmn[3];
};

enum offloaded_packets_sending_control {
	OFFLOADED_PACKETS_SENDING_CONTROL_INVALID = 0,
	OFFLOADED_PACKETS_SENDING_START,
	OFFLOADED_PACKETS_SENDING_STOP
};

/* CMD ID: 9 */
enum sprd_attr_vendor {
	ATTR_INVALID = 0,
	ATTR_DFS     = 1,
	ATTR_NAN     = 2,
	ATTR_STATS_EXT     = 3,
	ATTR_IFINDEX     = 4,
	ATTR_ROAMING_POLICY = 5,
	ATTR_MAC_ADDR = 6,
	ATTR_FEATURE_FLAGS = 7,
	ATTR_TEST = 8,
	ATTR_CONCURRENCY_CAPA = 9,
	ATTR_MAX_CONCURRENT_CHANNELS_2_4_BAND = 10,
	ATTR_MAX_CONCURRENT_CHANNELS_5_0_BAND = 11,
	ATTR_SETBAND_VALUE = 12,
	ATTR_PAD = 13,
	ATTR_FTM_SESSION_COOKIE = 14,
	ATTR_LOC_CAPA = 15,
	ATTR_FTM_MEAS_PEERS = 16,
	ATTR_FTM_MEAS_PEER_RESULTS = 17,
	ATTR_FTM_RESPONDER_ENABLE = 18,
	ATTR_FTM_LCI = 19,
	ATTR_FTM_LCR = 20,
	ATTR_LOC_SESSION_STATUS = 21,
	ATTR_FTM_INITIAL_TOKEN = 22,
	ATTR_AOA_TYPE = 23,
	ATTR_LOC_ANTENNA_ARRAY_MASK = 24,
	ATTR_AOA_MEAS_RESULT = 25,
	ATTR_CHAIN_INDEX = 26,
	ATTR_CHAIN_RSSI = 27,
	ATTR_FREQ = 28,
	ATTR_TSF = 29,
	ATTR_DMG_RF_SECTOR_INDEX = 30,
	ATTR_DMG_RF_SECTOR_TYPE = 31,
	ATTR_DMG_RF_MODULE_MASK = 32,
	ATTR_DMG_RF_SECTOR_CFG = 33,
	ATTR_RX_AGGREGATION_STATS_HOLES_NUM = 34,
	ATTR_RX_AGGREGATION_STATS_HOLES_INFO = 35,
	ATTR_BTM_MBO_TRANSITION_REASON = 36,
	ATTR_BTM_CANDIDATE_INFO = 37,
	ATTR_BRP_ANT_LIMIT_MODE = 38,
	ATTR_BRP_ANT_NUM_LIMIT = 39,
	ATTR_ANTENNA_INFO = 40,
	/* keep last */
	ATTR_AFTER_LAST,
	ATTR_MAX =
		ATTR_AFTER_LAST - 1,
};

/* CMD ID: 14 */
enum sprd_attr_ll_stats_set {
	ATTR_LL_STATS_SET_INVALID = 0,
	ATTR_LL_STATS_SET_CONFIG_MPDU_SIZE_THRESHOLD = 1,
	ATTR_LL_STATS_SET_CONFIG_AGGRESSIVE_STATS_GATHERING = 2,
	/* keep last */
	ATTR_LL_STATS_SET_AFTER_LAST,
	ATTR_LL_STATS_SET_MAX =
		ATTR_LL_STATS_SET_AFTER_LAST - 1,
};

/* CMD ID: 15 */
enum sprd_attr_ll_stats_get {
	ATTR_LL_STATS_GET_INVALID = 0,
	ATTR_LL_STATS_GET_CONFIG_REQ_ID = 1,
	ATTR_LL_STATS_GET_CONFIG_REQ_MASK = 2,
	/* keep last */
	ATTR_LL_STATS_GET_AFTER_LAST,
	ATTR_LL_STATS_GET_MAX =
		ATTR_LL_STATS_GET_AFTER_LAST - 1,
};

/* CMD ID: 23 */
enum sprd_attr_gscan_config_params {
	ATTR_GSCAN_SUBCMD_CONFIG_PARAM_INVALID = 0,
	ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID = 1,
	ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND = 2,
	ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS = 3,
	ATTR_GSCAN_CHANNEL_SPEC_CHANNEL = 4,
	ATTR_GSCAN_CHANNEL_SPEC_DWELL_TIME = 5,
	ATTR_GSCAN_CHANNEL_SPEC_PASSIVE = 6,
	ATTR_GSCAN_CHANNEL_SPEC_CLASS = 7,
	ATTR_GSCAN_BUCKET_SPEC_INDEX = 8,
	ATTR_GSCAN_BUCKET_SPEC_BAND = 9,
	ATTR_GSCAN_BUCKET_SPEC_PERIOD = 10,
	ATTR_GSCAN_BUCKET_SPEC_REPORT_EVENTS = 11,
	ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS = 12,
	ATTR_GSCAN_CHANNEL_SPEC = 13,
	ATTR_GSCAN_SCAN_CMD_PARAMS_BASE_PERIOD = 14,
	ATTR_GSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN = 15,
	ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_PERCENT = 16,
	ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS = 17,
	ATTR_GSCAN_BUCKET_SPEC = 18,
	ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH = 19,
	ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX = 20,
	ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID = 21,
	ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW = 22,
	ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH = 23,
	ATTR_GSCAN_AP_THRESHOLD_PARAM_CHANNEL = 24,
	ATTR_GSCAN_BSSID_HOTLIST_PARAMS_NUM_AP = 25,
	ATTR_GSCAN_AP_THRESHOLD_PARAM = 26,
	ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE = 27,
	ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE = 28,
	ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING = 29,
	ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP = 30,
	ATTR_GSCAN_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE = 31,
	ATTR_GSCAN_BUCKET_SPEC_MAX_PERIOD = 32,
	ATTR_GSCAN_BUCKET_SPEC_BASE = 33,
	ATTR_GSCAN_BUCKET_SPEC_STEP_COUNT = 34,
	ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_NUM_SCANS = 35,
	ATTR_GSCAN_SSID_HOTLIST_PARAMS_LOST_SSID_SAMPLE_SIZE = 36,
	ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID = 37,
	ATTR_GSCAN_SSID_THRESHOLD_PARAM = 38,
	ATTR_GSCAN_SSID_THRESHOLD_PARAM_SSID = 39,
	ATTR_GSCAN_SSID_THRESHOLD_PARAM_BAND = 40,
	ATTR_GSCAN_SSID_THRESHOLD_PARAM_RSSI_LOW = 41,
	ATTR_GSCAN_SSID_THRESHOLD_PARAM_RSSI_HIGH = 42,
	ATTR_GSCAN_CONFIGURATION_FLAGS = 43,
	/* keep last */
	ATTR_GSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST,
	ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX =
		ATTR_GSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST - 1,
};

/* CMD ID: 62 */
enum sprd_attr_wifi_logger_start {
	ATTR_WIFI_LOGGER_START_INVALID = 0,
	ATTR_WIFI_LOGGER_RING_ID = 1,
	ATTR_WIFI_LOGGER_VERBOSE_LEVEL = 2,
	ATTR_WIFI_LOGGER_FLAGS = 3,
	/* keep last */
	ATTR_WIFI_LOGGER_START_AFTER_LAST,
	ATTR_WIFI_LOGGER_START_GET_MAX =
		ATTR_WIFI_LOGGER_START_AFTER_LAST - 1,
};

/* CMD ID: 64 */
enum sprd_attr_roaming_config_params {
	ATTR_ROAMING_PARAM_INVALID = 0,
	ATTR_ROAMING_SUBCMD = 1,
	ATTR_ROAMING_REQ_ID = 2,
	ATTR_ROAMING_PARAM_WHITE_LIST_SSID_NUM_NETWORKS = 3,
	ATTR_ROAMING_PARAM_WHITE_LIST_SSID_LIST = 4,
	ATTR_ROAMING_PARAM_WHITE_LIST_SSID = 5,
	ATTR_ROAMING_PARAM_A_BAND_BOOST_THRESHOLD = 6,
	ATTR_ROAMING_PARAM_A_BAND_PENALTY_THRESHOLD = 7,
	ATTR_ROAMING_PARAM_A_BAND_BOOST_FACTOR = 8,
	ATTR_ROAMING_PARAM_A_BAND_PENALTY_FACTOR = 9,
	ATTR_ROAMING_PARAM_A_BAND_MAX_BOOST = 10,
	ATTR_ROAMING_PARAM_LAZY_ROAM_HISTERESYS = 11,
	ATTR_ROAMING_PARAM_ALERT_ROAM_RSSI_TRIGGER = 12,
	ATTR_ROAMING_PARAM_SET_LAZY_ROAM_ENABLE = 13,
	ATTR_ROAMING_PARAM_SET_BSSID_PREFS = 14,
	ATTR_ROAMING_PARAM_SET_LAZY_ROAM_NUM_BSSID = 15,
	ATTR_ROAMING_PARAM_SET_LAZY_ROAM_BSSID = 16,
	ATTR_ROAMING_PARAM_SET_LAZY_ROAM_RSSI_MODIFIER = 17,
	ATTR_ROAMING_PARAM_SET_BSSID_PARAMS = 18,
	ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID = 19,
	ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID = 20,
	/* keep last */
	ATTR_ROAMING_PARAM_AFTER_LAST,
	ATTR_ROAMING_PARAM_MAX =
		ATTR_ROAMING_PARAM_AFTER_LAST - 1,
};

/* CMD ID: 76 */
enum sprd_attr_get_logger_features {
	ATTR_LOGGER_INVALID = 0,
	ATTR_LOGGER_SUPPORTED = 1,
	/* keep last */
	ATTR_LOGGER_AFTER_LAST,
	ATTR_LOGGER_MAX =
		ATTR_LOGGER_AFTER_LAST - 1,
};

/* CMD ID: 79 */
enum sprdwl_attr_offloaded_packets {
	ATTR_OFFLOADED_PACKETS_INVALID = 0,
	ATTR_OFFLOADED_PACKETS_SENDING_CONTROL,
	ATTR_OFFLOADED_PACKETS_REQUEST_ID,
	ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA,
	ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR,
	ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR,
	ATTR_OFFLOADED_PACKETS_PERIOD,
	ATTR_OFFLOADED_PACKETS_ETHER_PROTO_TYPE,
	/* Keep last */
	ATTR_OFFLOADED_PACKETS_AFTER_LAST,
	ATTR_OFFLOADED_PACKETS_MAX =
		ATTR_OFFLOADED_PACKETS_AFTER_LAST - 1,
};

/* CMD ID: 82 */
enum sprd_attr_nd_offload {
	ATTR_ND_OFFLOAD_INVALID = 0,
	ATTR_ND_OFFLOAD_FLAG,
	/* Keep last */
	ATTR_ND_OFFLOAD_AFTER_LAST,
	ATTR_ND_OFFLOAD_MAX =
		ATTR_ND_OFFLOAD_AFTER_LAST - 1,
};

/* CMD ID: 85 */
enum sprd_attr_wake_stats {
	ATTR_WAKE_STATS_INVALID = 0,
	ATTR_WAKE_STATS_TOTAL_CMD_EVENT_WAKE,
	ATTR_WAKE_STATS_CMD_EVENT_WAKE_CNT_PTR,
	ATTR_WAKE_STATS_CMD_EVENT_WAKE_CNT_SZ,
	ATTR_WAKE_STATS_TOTAL_DRIVER_FW_LOCAL_WAKE,
	ATTR_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_PTR,
	ATTR_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_SZ,
	ATTR_WAKE_STATS_TOTAL_RX_DATA_WAKE,
	ATTR_WAKE_STATS_RX_UNICAST_CNT,
	ATTR_WAKE_STATS_RX_MULTICAST_CNT,
	ATTR_WAKE_STATS_RX_BROADCAST_CNT,
	ATTR_WAKE_STATS_ICMP_PKT,
	ATTR_WAKE_STATS_ICMP6_PKT,
	ATTR_WAKE_STATS_ICMP6_RA,
	ATTR_WAKE_STATS_ICMP6_NA,
	ATTR_WAKE_STATS_ICMP6_NS,
	ATTR_WAKE_STATS_ICMP4_RX_MULTICAST_CNT,
	ATTR_WAKE_STATS_ICMP6_RX_MULTICAST_CNT,
	ATTR_WAKE_STATS_OTHER_RX_MULTICAST_CNT,
	ATTR_WAKE_STATS_RSSI_BREACH_CNT,
	ATTR_WAKE_STATS_LOW_RSSI_CNT,
	ATTR_WAKE_STATS_GSCAN_CNT,
	ATTR_WAKE_STATS_PNO_COMPLETE_CNT,
	ATTR_WAKE_STATS_PNO_MATCH_CNT,
	/* keep last */
	ATTR_WAKE_STATS_AFTER_LAST,
	ATTR_WAKE_STATS_MAX =
		ATTR_WAKE_STATS_AFTER_LAST - 1,
};

/* CMD ID: 146 */
enum sprd_attr_sar_limits {
	ATTR_SAR_LIMITS_INVALID = 0,
	ATTR_SAR_LIMITS_SAR_ENABLE = 1,
	ATTR_SAR_LIMITS_NUM_SPECS = 2,
	ATTR_SAR_LIMITS_SPEC = 3,
	ATTR_SAR_LIMITS_SPEC_BAND = 4,
	ATTR_SAR_LIMITS_SPEC_CHAIN = 5,
	ATTR_SAR_LIMITS_SPEC_MODULATION = 6,
	ATTR_SAR_LIMITS_SPEC_POWER_LIMIT = 7,
	ATTR_SAR_LIMITS_SPEC_POWER_LIMIT_INDEX = 8,
	/* keep last */
	ATTR_SAR_LIMITS_AFTER_LAST,
	ATTR_SAR_LIMITS_MAX =
		ATTR_SAR_LIMITS_AFTER_LAST - 1
};

void sprdwl_report_gscan_result(struct sprdwl_vif *vif,
				u32 report_event, u8 bucketid,
				u16 chan, s16 rssi, const u8 *buf, u16 len);
int sprdwl_gscan_done(struct sprdwl_vif *vif, u8 bucketid);
int sprdwl_buffer_full_event(struct sprdwl_vif *vif);
int sprdwl_available_event(struct sprdwl_vif *vif);
int sprdwl_vendor_report_full_scan(struct sprdwl_vif *vif,
					struct sprdwl_gscan_result *item);
int sprdwl_hotlist_change_event(struct sprdwl_vif *vif, u32 report_event);
void sprdwl_event_rssi_monitor(struct sprdwl_vif *vif, u8 *data, u16 len);
int sprdwl_vendor_cache_scan_result(struct sprdwl_vif *vif,
				u8 bucket_id, struct sprdwl_gscan_result *item);
int sprdwl_vendor_cache_hotlist_result(struct sprdwl_vif *vif,
				struct sprdwl_gscan_result *item);
int sprdwl_significant_change_event(struct sprdwl_vif *vif);
int sprdwl_vendor_cache_significant_change_result(struct sprdwl_vif *vif,
				u8 *data, u16 data_len);
int sprdwl_set_packet_offload(struct sprdwl_priv *priv, u8 vif_ctx_id,
			      u32 req, u8 enable, u32 interval,
			      u32 len, u8 *data);

#endif
