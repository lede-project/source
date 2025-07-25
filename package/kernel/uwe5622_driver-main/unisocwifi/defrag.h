#ifndef __DEFRAG__H__
#define __DEFRAG__H__

#include <linux/skbuff.h>

#define MAX_DEFRAG_NUM 3

struct rx_defrag_desc {
	unsigned char sta_lut_index;
	unsigned char tid;
	unsigned char frag_num;
	unsigned short seq_num;
};

struct rx_defrag_node {
	struct list_head list;
	struct rx_defrag_desc desc;
	struct sk_buff_head skb_list;
	unsigned int msdu_len;
	unsigned char last_frag_num;
};

struct sprdwl_rx_defrag_entry {
	struct list_head list;
	struct sk_buff *skb_head;
	struct sk_buff *skb_last;
};

int sprdwl_defrag_init(struct sprdwl_rx_defrag_entry *defrag_entry);
void sprdwl_defrag_deinit(struct sprdwl_rx_defrag_entry *defrag_entry);
struct sk_buff
*defrag_data_process(struct sprdwl_rx_defrag_entry *defrag_entry,
		     struct sk_buff *pskb);

#endif /*__DEFRAG__H__*/
