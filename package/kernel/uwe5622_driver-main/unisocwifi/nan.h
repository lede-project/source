#ifndef __SPRDWL_NAN_H__
#define __SPRDWL_NAN_H__

/* macro */
enum nan_msg_id {
	NAN_MSG_ID_ERROR_RSP                    = 0,
	NAN_MSG_ID_CONFIGURATION_REQ            = 1,
	NAN_MSG_ID_CONFIGURATION_RSP            = 2,
	NAN_MSG_ID_PUBLISH_SERVICE_REQ          = 3,
	NAN_MSG_ID_PUBLISH_SERVICE_RSP          = 4,
	NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_REQ   = 5,
	NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_RSP   = 6,
	NAN_MSG_ID_PUBLISH_REPLIED_IND          = 7,
	NAN_MSG_ID_PUBLISH_TERMINATED_IND       = 8,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_REQ        = 9,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_RSP        = 10,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_REQ = 11,
	NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_RSP = 12,
	NAN_MSG_ID_MATCH_IND                    = 13,
	NAN_MSG_ID_MATCH_EXPIRED_IND            = 14,
	NAN_MSG_ID_SUBSCRIBE_TERMINATED_IND     = 15,
	NAN_MSG_ID_DE_EVENT_IND                 = 16,
	NAN_MSG_ID_TRANSMIT_FOLLOWUP_REQ        = 17,
	NAN_MSG_ID_TRANSMIT_FOLLOWUP_RSP        = 18,
	NAN_MSG_ID_FOLLOWUP_IND                 = 19,
	NAN_MSG_ID_STATS_REQ                    = 20,
	NAN_MSG_ID_STATS_RSP                    = 21,
	NAN_MSG_ID_ENABLE_REQ                   = 22,
	NAN_MSG_ID_ENABLE_RSP                   = 23,
	NAN_MSG_ID_DISABLE_REQ                  = 24,
	NAN_MSG_ID_DISABLE_RSP                  = 25,
	NAN_MSG_ID_DISABLE_IND                  = 26,
	NAN_MSG_ID_TCA_REQ                      = 27,
	NAN_MSG_ID_TCA_RSP                      = 28,
	NAN_MSG_ID_TCA_IND                      = 29,
	NAN_MSG_ID_BEACON_SDF_REQ               = 30,
	NAN_MSG_ID_BEACON_SDF_RSP               = 31,
	NAN_MSG_ID_BEACON_SDF_IND               = 32,
	NAN_MSG_ID_CAPABILITIES_REQ             = 33,
	NAN_MSG_ID_CAPABILITIES_RSP             = 34
};

enum nan_tlv_type {
	NAN_TLV_TYPE_FIRST = 0,

	/* Service Discovery Frame types */
	NAN_TLV_TYPE_SDF_FIRST = NAN_TLV_TYPE_FIRST,
	NAN_TLV_TYPE_SERVICE_NAME = NAN_TLV_TYPE_SDF_FIRST,
	NAN_TLV_TYPE_SDF_MATCH_FILTER,
	NAN_TLV_TYPE_TX_MATCH_FILTER,
	NAN_TLV_TYPE_RX_MATCH_FILTER,
	NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO,
	NAN_TLV_TYPE_EXT_SERVICE_SPECIFIC_INFO = 5,
	NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_TRANSMIT = 6,
	NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_RECEIVE = 7,
	NAN_TLV_TYPE_POST_NAN_CONNECTIVITY_CAPABILITIES_RECEIVE = 8,
	NAN_TLV_TYPE_POST_NAN_DISCOVERY_ATTRIBUTE_RECEIVE = 9,
	NAN_TLV_TYPE_BEACON_SDF_PAYLOAD_RECEIVE = 10,
	NAN_TLV_TYPE_SDF_LAST = 4095,

	/* Configuration types */
	NAN_TLV_TYPE_CONFIG_FIRST = 4096,
	NAN_TLV_TYPE_24G_SUPPORT = NAN_TLV_TYPE_CONFIG_FIRST,
	NAN_TLV_TYPE_24G_BEACON,
	NAN_TLV_TYPE_24G_SDF,
	NAN_TLV_TYPE_24G_RSSI_CLOSE,
	NAN_TLV_TYPE_24G_RSSI_MIDDLE = 4100,
	NAN_TLV_TYPE_24G_RSSI_CLOSE_PROXIMITY,
	NAN_TLV_TYPE_5G_SUPPORT,
	NAN_TLV_TYPE_5G_BEACON,
	NAN_TLV_TYPE_5G_SDF,
	NAN_TLV_TYPE_5G_RSSI_CLOSE,
	NAN_TLV_TYPE_5G_RSSI_MIDDLE,
	NAN_TLV_TYPE_5G_RSSI_CLOSE_PROXIMITY,
	NAN_TLV_TYPE_SID_BEACON,
	NAN_TLV_TYPE_HOP_COUNT_LIMIT,
	NAN_TLV_TYPE_MASTER_PREFERENCE = 4110,
	NAN_TLV_TYPE_CLUSTER_ID_LOW,
	NAN_TLV_TYPE_CLUSTER_ID_HIGH,
	NAN_TLV_TYPE_RSSI_AVERAGING_WINDOW_SIZE,
	NAN_TLV_TYPE_CLUSTER_OUI_NETWORK_ID,
	NAN_TLV_TYPE_SOURCE_MAC_ADDRESS,
	NAN_TLV_TYPE_CLUSTER_ATTRIBUTE_IN_SDF,
	NAN_TLV_TYPE_SOCIAL_CHANNEL_SCAN_PARAMS,
	NAN_TLV_TYPE_DEBUGGING_FLAGS,
	NAN_TLV_TYPE_POST_NAN_CONNECTIVITY_CAPABILITIES_TRANSMIT,
	NAN_TLV_TYPE_POST_NAN_DISCOVERY_ATTRIBUTE_TRANSMIT = 4120,
	NAN_TLV_TYPE_FURTHER_AVAILABILITY_MAP,
	NAN_TLV_TYPE_HOP_COUNT_FORCE,
	NAN_TLV_TYPE_RANDOM_FACTOR_FORCE,
	NAN_TLV_TYPE_RANDOM_UPDATE_TIME = 4124,
	NAN_TLV_TYPE_EARLY_WAKEUP,
	NAN_TLV_TYPE_PERIODIC_SCAN_INTERVAL,
	NAN_TLV_TYPE_DW_INTERVAL = 4128,
	NAN_TLV_TYPE_DB_INTERVAL,
	NAN_TLV_TYPE_FURTHER_AVAILABILITY,
	NAN_TLV_TYPE_24G_CHANNEL,
	NAN_TLV_TYPE_5G_CHANNEL,
	NAN_TLV_TYPE_CONFIG_LAST = 8191,

	/* Attributes types */
	NAN_TLV_TYPE_ATTRS_FIRST = 8192,
	NAN_TLV_TYPE_AVAILABILITY_INTERVALS_MAP = NAN_TLV_TYPE_ATTRS_FIRST,
	NAN_TLV_TYPE_WLAN_MESH_ID,
	NAN_TLV_TYPE_MAC_ADDRESS,
	NAN_TLV_TYPE_RECEIVED_RSSI_VALUE,
	NAN_TLV_TYPE_CLUSTER_ATTRIBUTE,
	NAN_TLV_TYPE_WLAN_INFRA_SSID,
	NAN_TLV_TYPE_ATTRS_LAST = 12287,

	/* Events Type */
	NAN_TLV_TYPE_EVENTS_FIRST = 12288,
	NAN_TLV_TYPE_EVENT_SELF_STATION_MAC_ADDRESS = NAN_TLV_TYPE_EVENTS_FIRST,
	NAN_TLV_TYPE_EVENT_STARTED_CLUSTER,
	NAN_TLV_TYPE_EVENT_JOINED_CLUSTER,
	NAN_TLV_TYPE_EVENT_CLUSTER_SCAN_RESULTS,
	NAN_TLV_TYPE_FAW_MEM_AVAIL,
	NAN_TLV_TYPE_EVENTS_LAST = 16383,

	/* TCA types */
	NAN_TLV_TYPE_TCA_FIRST = 16384,
	NAN_TLV_TYPE_CLUSTER_SIZE_REQ = NAN_TLV_TYPE_TCA_FIRST,
	NAN_TLV_TYPE_CLUSTER_SIZE_RSP,
	NAN_TLV_TYPE_TCA_LAST = 32767,

	/* Statistics types */
	NAN_TLV_TYPE_STATS_FIRST = 32768,
	NAN_TLV_TYPE_DE_PUBLISH_STATS = NAN_TLV_TYPE_STATS_FIRST,
	NAN_TLV_TYPE_DE_SUBSCRIBE_STATS,
	NAN_TLV_TYPE_DE_MAC_STATS,
	NAN_TLV_TYPE_DE_TIMING_SYNC_STATS,
	NAN_TLV_TYPE_DE_DW_STATS,
	NAN_TLV_TYPE_DE_STATS,
	NAN_TLV_TYPE_STATS_LAST = 36863,

	NAN_TLV_TYPE_LAST = 65535
};

/* structure */
struct nan_msg_header {
	u16 msg_ver:4; /* NAN_MSG_VERSION 1 */
	u16 msg_id:12;
	u16 msg_len;
	u16 handle; /* publish_id or subscribe_id */
	u16 transaction_id;
} __packed;

struct nan_tlv {
	u16 type;
	u16 length;
	u8 *value;
} __packed;

struct nan_enable_req {
	struct nan_msg_header header;
	/* TLVs:
	 *
	 * Required: Cluster Low, Cluster High, Master Preference,
	 * Optional: 5G Support, SID, 5G Sync Disc, RSSI Close, RSSI Medium,
	 *           Hop Count Limit, Random Time, Master Preference,
	 *           WLAN Intra Attr, P2P Operation Attr, WLAN IBSS Attr,
	 *           WLAN Mesh Attr
	*/
	u8 nan_tlv[];
} __packed;

struct nan_disable_req {
	struct nan_msg_header header;
} __packed;

struct nan_disable_rsp {
	struct nan_msg_header header;
	/* status of the request */
	u16 status;
	u16 value;
} __packed;

struct publish_config_params {
	u16 ttl;
	u16 period;
	u32 reserved:1;
	u32 publish_type:2;
	u32 tx_type:1;
	u32 rssi_threshold_flag:1;
	u32 ota_flag:1;
	u32 publish_match_indicator:2;
	u32 publish_count:8;
	u32 connmap:8;
	u32 disable_pub_terminated_ind:1;
	u32 disable_pub_match_expired_ind:1;
	u32 disable_followup_rx_ind:1;
	u32 reserved2:5;
} __packed;

struct nan_publish_req {
	struct nan_msg_header header;
	struct publish_config_params pub_params;
	/* TLVs:
	 *
	 * Required: Service Name,
	 * Optional: Tx Match Filter, Rx Match Filter, Service Specific Info,
	 */
	u8 nan_tlv[];
} __packed;

struct nan_cancel_pub_req {
	struct nan_msg_header header;
} __packed;

struct subscribe_config_parames {
	u16 ttl;
	u16 period;
	u32 subscribe_type:1;
	u32 srf_type:1;
	u32 srf_include_type:1;
	u32 srf_state:1;
	u32 ssi_required:1;
	u32 subscribe_match_indicator:2;
	u32 xbit:1;
	u32 subscribe_count:8;
	u32 rssi_threshold_flag:1;
	u32 ota_flag:1;
	u32 disable_sub_terminated_ind:1;
	u32 disable_sub_match_expired_ind:1;
	u32 disable_followup_rx_ind:1;
	u32 reserved:3;
	u32 connmap:8;
} __packed;

struct nan_subscribe_req {
	struct nan_msg_header header;
	struct subscribe_config_parames sub_params;
	/* TLVs:
	 *
	 * Required: Service Name
	 * Optional: Rx Match Filter, Tx Match Filter, Service Specific Info,
	 */
	u8 nan_tlv[];
} __packed;

struct nan_cancel_sub_req {
	struct nan_msg_header header;
} __packed;

struct followup_config_params {
	u32 requestor_instance_id;
	u32 priority:4;
	u32 dw_or_faw:1;
	u32 recv_indication_cfg:1;
	u32 reserved:26;
} __packed;

struct nan_followup_req {
	struct nan_msg_header header;
	struct followup_config_params followup_params;
	/* TLVs:
	 *
	 * Required: Service Specific Info or Extended Service Specific Info
	 */
	u8 nan_tlv[];
} __packed;

struct nan_capabilities_req {
	struct nan_msg_header header;
} __packed;

struct sprdwl_event_nan {
	struct nan_msg_header header;
	/* status of the request */
	u16 status;
	u16 value;
} __packed;

struct nan_capa {
	struct nan_msg_header header;
	u32 status;
	u32 value;
	u32 max_concurrent_nan_clusters;
	u32 max_publishes;
	u32 max_subscribes;
	u32 max_service_name_len;
	u32 max_match_filter_len;
	u32 max_total_match_filter_len;
	u32 max_service_specific_info_len;
	u32 max_vsa_data_len;
	u32 max_mesh_data_len;
	u32 max_ndi_interfaces;
	u32 max_ndp_sessions;
	u32 max_app_info_len;
} __packed;

struct nan_cmd_header {
	u16 data_len;
	u8 data[0];
} __packed;

/* cmd handler*/
int sprdwl_vendor_nan_cmds(struct wiphy *wiphy,
			   struct wireless_dev *wdev,
			   const void  *data, int len);
/* event handler*/
int sprdwl_event_nan(struct sprdwl_vif *vif, u8 *data, u16 len);

#endif /* __SPRDWL_NAN_H__ */
