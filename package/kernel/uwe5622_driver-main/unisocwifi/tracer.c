#include <linux/etherdevice.h>
#include <linux/ipv6.h>
#include <linux/ip.h>
#include <net/ndisc.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/kallsyms.h>
#include "msg.h"
#include "tracer.h"
#include "cmdevt.h"
#include "rx_msg.h"

#define WAKEUP_TIME_EXPIRED 500

static struct deauth_trace deauth;
static void deauth_reason_worker(struct work_struct *work);
static DECLARE_WORK(deauth_worker, deauth_reason_worker);

static void deauth_reason_worker(struct work_struct *work)
{
	int i, j;
	struct deauth_info *dinfo;

	wl_info("deauth reason dump: == START ==\n");
	for (i = 0; i < SPRDWL_MODE_MAX; i++) {
		for (j = 0; j < MAX_DEAUTH_REASON; j++) {
			dinfo = &deauth.deauth_mode[i];
			if (dinfo->local_deauth[j] != 0)
				wl_info("mode[%d] local reason[%d]:%ld times\n",
					i, j, dinfo->local_deauth[j]);

			if (dinfo->remote_deauth[j] != 0)
				wl_info("mode[%d] remote reason[%d]:%ld times\n",
					i, j, dinfo->remote_deauth[j]);
		}
	}

	wl_info("deauth reason dump: == END ==\n");
}

void trace_deauth_reason(int mode, u16 reason_code, int dirction)
{
	struct deauth_info *dinfo;

	if (reason_code > MAX_DEAUTH_REASON) {
		wl_info("deauth reason:%d not record\n", reason_code);
		return;
	}

	dinfo = &deauth.deauth_mode[mode];
	spin_lock_bh(&deauth.lock);
	switch (dirction) {
	case LOCAL_EVENT:
		dinfo->local_deauth[reason_code]++;
		break;
	default:
		dinfo->remote_deauth[reason_code]++;
		break;
	}

	spin_unlock_bh(&deauth.lock);

	schedule_work(&deauth_worker);
}

static void trace_rx_icmp6_wake(struct wakeup_trace *tracer, void *data)
{
	void *iphdr_addr;
	struct ipv6hdr *ipv6_hdr;
	struct icmp6hdr *icmp6_hdr;

	iphdr_addr = data + sizeof(struct ethhdr);
	ipv6_hdr = iphdr_addr;
	if (ipv6_hdr->nexthdr == IPPROTO_ICMPV6) {
		tracer->pkt_type_dtl.icmp6_pkt_cnt++;

		icmp6_hdr = (struct icmp6hdr *)(iphdr_addr + sizeof(*ipv6_hdr));
		switch (icmp6_hdr->icmp6_type) {
		case NDISC_ROUTER_ADVERTISEMENT:
			tracer->pkt_type_dtl.icmp6_ra_cnt++;
			break;

		case NDISC_NEIGHBOUR_SOLICITATION:
			tracer->pkt_type_dtl.icmp6_ns_cnt++;
			break;

		case NDISC_NEIGHBOUR_ADVERTISEMENT:
			tracer->pkt_type_dtl.icmp6_na_cnt++;
			break;

		default:
			break;
		}
	}
}

static void trace_rx_data_wake(struct wakeup_trace *tracer, void *data)
{
	void *iphdr_addr = data;
	struct ethhdr *ether_hdr = data;
	struct iphdr *ipv4_hdr;

	if (is_broadcast_ether_addr(ether_hdr->h_dest)) {
		tracer->rx_data_dtl.rx_brdcst_cnt++;
	} else if (is_multicast_ether_addr(ether_hdr->h_dest)) {
		switch (ntohs(ether_hdr->h_proto)) {
		case ETH_P_IP:
			tracer->rx_data_dtl.rx_mc_dtl.ipv4_mc_cnt++;
			break;
		case ETH_P_IPV6:
			tracer->rx_data_dtl.rx_mc_dtl.ipv6_mc_cnt++;
			trace_rx_icmp6_wake(tracer, ether_hdr);
			break;
		default:
			tracer->rx_data_dtl.rx_mc_dtl.other_mc_cnt++;
			break;
		}

	} else { /*unicast*/
		tracer->rx_data_dtl.rx_unicast_cnt++;
		iphdr_addr += sizeof(*ether_hdr);
		switch (ntohs(ether_hdr->h_proto)) {
		case ETH_P_IP:
			ipv4_hdr = iphdr_addr;
			if (ipv4_hdr->protocol == IPPROTO_ICMP)
				tracer->pkt_type_dtl.icmp_pkt_cnt++;
			break;

		case ETH_P_IPV6:
			trace_rx_icmp6_wake(tracer, ether_hdr);
			break;
		default:
			wl_info("recv proto = 0x%x\n",
				ntohs(ether_hdr->h_proto));
			break;
		}
	}
}

void trace_rx_wakeup(struct wakeup_trace *tracer, void *data, void *rdata)
{
	int type;
	struct rx_msdu_desc *msdu;

	if (!SPRD_HEAD_GET_RESUME_BIT(data))
		return;

	tracer->resume_flag = 0;

	type = SPRDWL_HEAD_GET_TYPE(data);
	switch (type) {
	/* commands or events between command WIFI_CMD_POWER_SAVE
	 * and its respone just consider as one wake up source
	 */
	case SPRDWL_TYPE_CMD:
	case SPRDWL_TYPE_EVENT:
		tracer->total_cmd_event_wake++;
		wl_info("wake up by cmd/event[%d] (%d times)\n",
			type, tracer->total_cmd_event_wake);
		break;

	case SPRDWL_TYPE_DATA_SPECIAL:
		tracer->total_rx_data_wake++;

		msdu = (struct rx_msdu_desc *)rdata;
		trace_rx_data_wake(tracer, rdata + msdu->msdu_offset);
		wl_info("wake up by data (%d times)\n",
			tracer->total_rx_data_wake);
		break;

	case SPRDWL_TYPE_PKT_LOG:
		tracer->total_local_wake++;
		wl_info("wake up by pkt log (%d times)\n",
			tracer->total_local_wake);
		break;

	default:
		wl_info("wake up source untrace type = 0x%x\n", type);
		break;
	}
}

void trace_info_init(void)
{
	spin_lock_init(&deauth.lock);
}

void trace_info_deinit(void)
{
	cancel_work_sync(&deauth_worker);
}

