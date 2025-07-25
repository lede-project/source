#ifndef __WLAN_QOS_H__
#define __WLAN_QOS_H__
#include "msg.h"
#include "cfg80211.h"
#include "wl_core.h"
#include <linux/skbuff.h>
#include "sprdwl.h"

typedef enum {
	SPRDWL_AC_VO,
	SPRDWL_AC_VI,
	SPRDWL_AC_BE,
	SPRDWL_AC_BK,
	SPRDWL_AC_MAX,
} qos_head_type_t;

struct peer_list {
	struct list_head head_list;
	spinlock_t p_lock;/*peer list lock*/
	atomic_t l_num;
	/*u8 delay_flag;
	u8 l_prio;*/
};

struct qos_list {
	struct peer_list p_list[MAX_LUT_NUM];
};

struct tx_t {
	int ac_index;
	unsigned char lut_id;
	atomic_t mode_list_num;
	struct qos_list q_list[SPRDWL_AC_MAX];
	/*int index;*/
	/*int going[SPRDWL_AC_MAX];*/
	/*high priority tx_index of two streams*/
	/*tx ratio of two streams*/
	/*int ratio;
	unsigned char *dscp2up_table;*/
};

typedef enum {
	prio_0 = 0,/* Mapped to AC_BE_Q */
	prio_1 = 1,/* Mapped to AC_BK_Q */
	prio_4 = 4,/* Mapped to AC_VI_Q */
	prio_6 = 6,/* Mapped to AC_VO_Q */
} ip_pkt_prio_t;

struct qos_capab_info {
	unsigned char id;
	unsigned char len;
	unsigned char qos_info[1];
};

struct dscp_range {
	u8 low;
	u8 high;
};

struct dscp_exception {
	u8 dscp;
	u8 up;
};

struct qos_map_range {
	u8 low;
	u8 high;
	u8 up;
};

#define QOS_MAP_MAX_DSCP_EXCEPTION 21

struct qos_map_set {
	struct dscp_exception qos_exceptions[QOS_MAP_MAX_DSCP_EXCEPTION];
	struct qos_map_range qos_ranges[8];
};

#ifdef WMMAC_WFA_CERTIFICATION
#define NUM_AC 4
#define NUM_TID 16
#define WMMAC_EDCA_TIMEOUT_MS		1000
#define WMMAC_TIME_RATIO	12

#define WLAN_EID_VENDOR_SPECIFIC 221
#define OUI_MICROSOFT 0x0050f2 /* Microsoft (also used in Wi-Fi specs)
				* 00:50:F2 */
#define WMM_OUI_TYPE 2
#define WMM_AC_ACM 0x10


typedef enum {
	AC_BK = 0,
	AC_BE = 1,
	AC_VI = 2,
	AC_VO = 3,
} edca_ac_t;

struct wmm_ac_ts_t {
	bool exist;
	u8 ac;
	u8 up;
	u8 direction;
	u16 admitted_time;
};
#endif

#define INCR_RING_BUFF_INDX(indx, max_num) \
	((((indx) + 1) < (max_num)) ? ((indx) + 1) : (0))

#define ETHER_ADDR_LEN 6

struct ether_header {
	unsigned char     ether_dhost[ETHER_ADDR_LEN];
	unsigned char     ether_shost[ETHER_ADDR_LEN];
	unsigned short     ether_type;

} __packed;

struct ethervlan_header {
	unsigned char ether_dhost[ETHER_ADDR_LEN];
	unsigned char     ether_shost[ETHER_ADDR_LEN];
	/* 0x8100 */
	unsigned short vlan_type;
	/* priority, cfi and vid */
	unsigned short vlan_tag;
	unsigned short ether_type;
};
/* 11u QoS map set */
#define DOT11_MNG_QOS_MAP_ID 110
/* DSCP ranges fixed with 8 entries */
#define QOS_MAP_FIXED_LENGTH	(8 * 2)
/* header length */
#define TLV_HDR_LEN 2

/* user priority */
#define VLAN_PRI_SHIFT	13
/* 3 bits of priority */
#define VLAN_PRI_MASK	7
/* VLAN ethertype/Tag Protocol ID */
#define VLAN_TPID	0x8100

/* IPV4 and IPV6 common */
#define ETHER_TYPE_IP	0x0800
/* IPv6 */
#define ETHER_TYPE_IPV6 0x86dd
/* offset to version field */
#define IP_VER_OFFSET	0x0
/* version mask */
#define IP_VER_MASK	0xf0
/* version shift */
#define IP_VER_SHIFT	4
/* version number for IPV4 */
#define IP_VER_4	4
/* version number for IPV6 */
#define IP_VER_6	6
 /* type of service offset */
#define IPV4_TOS_OFFSET            1
/* DiffServ codepoint shift */
#define IPV4_TOS_DSCP_SHIFT	2
#define IPV4_TOS(ipv4_body)\
	(((unsigned char *)(ipv4_body))[IPV4_TOS_OFFSET])
/* Historical precedence shift */
#define IPV4_TOS_PREC_SHIFT 5
/* 802.1Q */
#define ETHER_TYPE_8021Q 0x8100

/* IPV6 field decodes */
#define IPV6_TRAFFIC_CLASS(ipv6_body) \
	(((((unsigned char *)(ipv6_body))[0] & 0x0f) << 4) | \
	((((unsigned char *)(ipv6_body))[1] & 0xf0) >> 4))

#define IP_VER(ip_body) \
	((((unsigned char *)(ip_body))[IP_VER_OFFSET] & IP_VER_MASK) >> \
	IP_VER_SHIFT)

/* IPV4 TOS or IPV6 Traffic Classifier or 0 */
#define IP_TOS46(ip_body) \
	(IP_VER(ip_body) == IP_VER_4 ? IPV4_TOS(ip_body) : \
	IP_VER(ip_body) == IP_VER_6 ? IPV6_TRAFFIC_CLASS(ip_body) : 0)

#define PKT_SET_PRIO(skb, x) (((struct sk_buff *)(skb))->priority = (x))

#define VI_TOTAL_QUOTA 1500
#define BE_TOTAL_QUOTA 200
#define BK_TOTAL_QUOTA 200


static inline u8 qos_index_2_tid(unsigned int qos_index)
{
	unsigned char tid = 0;

	switch (qos_index) {
	case SPRDWL_AC_VO:
		tid = 6;
		break;
	case SPRDWL_AC_VI:
		tid = 4;
		break;
	case SPRDWL_AC_BK:
		tid = 1;
		break;
	default:
		tid = 0;
		break;
	}
	return tid;
}

extern struct qos_map_set g_11u_qos_map;
void qos_init(struct tx_t *qos);
unsigned int qos_match_q(void *skb, int data_offset);
void qos_enable(int flag);
unsigned int pkt_get_prio(void *skb, int data_offset, unsigned char *tos);
#if 0
void qos_deinit(struct tx_t *qos);
void qos_sched_tx_most(struct tx_t *qos, struct qos_list **data_list);
void qos_set_dscp2up_table(unsigned char *dscp2up_table,
			   struct qos_capab_info *qos_map_ie);
struct qos_capab_info *qos_parse_capab_info(void *buf, int buflen, uint key);
void qos_sched(struct tx_t *qos, struct qos_list **q, int *num);
int qos_fq_ratio(struct tx_t *qos);
#endif
int get_list_num(struct list_head *list);
unsigned int tid_map_to_qosindex(unsigned char tid);
unsigned int get_tid_qosindex(void *skb, int data_offset, unsigned char *tid, unsigned char *tos);
#ifdef WMMAC_WFA_CERTIFICATION
void init_default_qos_map(void);
void wmm_ac_init(struct sprdwl_priv *priv);
void reset_wmmac_parameters(struct sprdwl_priv *priv);
void reset_wmmac_ts_info(void);
unsigned int map_edca_ac_to_priority(u8 ac);
unsigned int map_priority_to_edca_ac(int priority);
void update_wmmac_ts_info(u8 tsid, u8 up, u8 ac, bool status, u16 admitted_time);
void remove_wmmac_ts_info(u8 tsid);
void update_admitted_time(struct sprdwl_priv *priv, u8 tsid, u16 medium_time, bool increase);
u16 get_wmmac_admitted_time(u8 tsid);
void reset_wmmac_parameters(struct sprdwl_priv *priv);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
void update_wmmac_edcaftime_timeout(struct timer_list *t);
#else
void update_wmmac_edcaftime_timeout(unsigned long data);
#endif
void update_wmmac_vo_timeout(unsigned long data);
void update_wmmac_vi_timeout(unsigned long data);
unsigned int change_priority_if(struct sprdwl_priv *priv, unsigned char *tid, unsigned char *tos, u16 len);
const u8 *get_wmm_ie(u8 *res, u16 ie_len, u8 ie, uint oui, uint oui_type);
#endif
#endif
