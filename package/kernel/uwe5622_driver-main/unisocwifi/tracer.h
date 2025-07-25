#ifndef __TRACER_H__
#define __TRACER_H__

#include "cfg80211.h"

#define MAX_DEAUTH_REASON 256

#define LOCAL_EVENT  0
#define REMOTE_EVENT 1

struct deauth_info {
	unsigned long local_deauth[MAX_DEAUTH_REASON];
	unsigned long remote_deauth[MAX_DEAUTH_REASON];
};

struct deauth_trace {
	spinlock_t lock; /* spinlock for deauth statistics */
	struct  deauth_info deauth_mode[SPRDWL_MODE_MAX];
};

struct rx_pkt_type_wakeup_details {
	u32 icmp_pkt_cnt;
	u32 icmp6_pkt_cnt;
	u32 icmp6_ra_cnt;
	u32 icmp6_na_cnt;
	u32 icmp6_ns_cnt;
};

struct rx_mc_wakeup_details {
	u32 ipv4_mc_cnt;
	u32 ipv6_mc_cnt;
	u32 other_mc_cnt;
};

struct rx_data_wakeup_details {
	u32 rx_unicast_cnt;
	u32 rx_brdcst_cnt;
	struct rx_mc_wakeup_details rx_mc_dtl;
};

struct wakeup_trace {
	u32 resume_flag;
	u32 total_cmd_event_wake;
	u32 total_rx_data_wake;
	u32 total_local_wake;
	struct rx_data_wakeup_details rx_data_dtl;
	struct rx_pkt_type_wakeup_details pkt_type_dtl;
};

void trace_info_init(void);
void trace_info_deinit(void);
void trace_deauth_reason(int mode, u16 reason_code, int dirction);
void trace_rx_wakeup(struct wakeup_trace *tracer, void *data, void *rdata);
#endif
