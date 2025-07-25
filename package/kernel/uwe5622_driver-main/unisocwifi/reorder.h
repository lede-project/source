#ifndef __SPRDWL_REORDER_H__
#define __SPRDWL_REORDER_H__

#include "sprdwl.h"
#include <linux/timer.h>
#include <linux/skbuff.h>

#define NUM_TIDS 8
#define RX_BA_LOSS_RECOVERY_TIMEOUT (HZ / 10)
#define MAX_TIMEOUT_CNT 60
#define MIN_INDEX_SIZE (1 << 6)
#define INDEX_SIZE_MASK(index_size) (index_size - 1)

struct rx_ba_pkt_desc {
	unsigned int pn_l;
	unsigned short pn_h;
	unsigned short seq;
	unsigned char cipher_type;
	unsigned char last;
	unsigned short msdu_num;
};

struct rx_ba_pkt {
	struct sk_buff *skb;
	struct sk_buff *skb_last; /* TODO: could we just search last skb? */
	struct rx_ba_pkt_desc desc;
};

struct rx_ba_node_desc {
	unsigned short win_start;
	unsigned short win_limit;
	unsigned short win_tail;
	unsigned short win_size;
	unsigned short buff_cnt;
	unsigned short pn_h;
	unsigned int pn_l;
	unsigned char reset_pn;
	unsigned int index_mask;
	struct rx_ba_pkt reorder_buffer[0];
};

struct rx_ba_node {
	unsigned char sta_lut_index;
	unsigned char tid;
	unsigned char active;
	unsigned char timeout_cnt;
	struct hlist_node hlist;
	struct rx_ba_node_desc *rx_ba;

	/* For reorder timeout */
	spinlock_t ba_node_lock;
	struct timer_list reorder_timer;
	struct sprdwl_rx_ba_entry *ba_entry;
};

struct sprdwl_rx_ba_entry {
	struct hlist_head hlist[NUM_TIDS];
	struct rx_ba_node *current_ba_node;
	spinlock_t skb_list_lock;
	struct sk_buff *skb_head;
	struct sk_buff *skb_last;
};

void sprdwl_reorder_init(struct sprdwl_rx_ba_entry *ba_entry);
void sprdwl_reorder_deinit(struct sprdwl_rx_ba_entry *ba_entry);
struct sk_buff *reorder_data_process(struct sprdwl_rx_ba_entry *ba_entry,
				     struct sk_buff *pskb);
#ifdef SPLIT_STACK
struct sk_buff *reorder_get_skb_list(struct sprdwl_rx_ba_entry *ba_entry);
#endif
void wlan_ba_session_event(void *hw_intf,
			   unsigned char *data, unsigned short len);
void peer_entry_delba(void *hw_intf, unsigned char sta_lut_index);
void reset_pn(struct sprdwl_priv *priv, const u8 *mac_addr);

void sprdwl_active_ba_node(struct sprdwl_rx_ba_entry *ba_entry,
				  u8 sta_lut_index, u8 tid);
#endif /* __SPRDWL_REORDER_H__ */
