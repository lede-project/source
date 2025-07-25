/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Authors	:
 * star.liu <star.liu@spreadtrum.com>
 * yifei.li <yifei.li@spreadtrum.com>
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

#include "reorder.h"
#include "rx_msg.h"
#include "work.h"
#include "cmdevt.h"
#include "wl_intf.h"
#include "qos.h"
#include "debug.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static void ba_reorder_timeout(struct timer_list *t);
#else
static void ba_reorder_timeout(unsigned long data);
#endif

static inline unsigned int get_index_size(unsigned int size)
{
	unsigned int index_size = MIN_INDEX_SIZE;

	while (size > index_size)
		index_size = (index_size << 1);

	wl_info("%s: rx ba index size: %d\n", __func__, index_size);

	return index_size;
}

static inline void
set_ba_node_desc(struct rx_ba_node_desc *ba_node_desc,
		 unsigned short win_start, unsigned short win_size,
		 unsigned int index_mask)
{
	ba_node_desc->win_size = win_size;
	ba_node_desc->win_start = win_start;
	ba_node_desc->win_limit = SEQNO_ADD(ba_node_desc->win_start,
						(ba_node_desc->win_size - 1));
	ba_node_desc->win_tail = SEQNO_SUB(ba_node_desc->win_start, 1);
	ba_node_desc->index_mask = index_mask;
	ba_node_desc->buff_cnt = 0;

	wl_info("%s:(win_start:%d, win_size:%d, win_tail:%d, index_mask:%d)\n",
		__func__, ba_node_desc->win_start, ba_node_desc->win_size,
		ba_node_desc->win_tail, ba_node_desc->index_mask);
}

static inline void set_ba_pkt_desc(struct rx_ba_pkt_desc *ba_pkt_desc,
				   struct rx_msdu_desc *msdu_desc)
{
	ba_pkt_desc->seq = msdu_desc->seq_num;
	ba_pkt_desc->pn_l = msdu_desc->pn_l;
	ba_pkt_desc->pn_h = msdu_desc->pn_h;
	ba_pkt_desc->cipher_type = msdu_desc->cipher_type;
}

static inline void
reorder_set_skb_list(struct sprdwl_rx_ba_entry *ba_entry,
			 struct sk_buff *skb_head, struct sk_buff *skb_last)
{
	spin_lock_bh(&ba_entry->skb_list_lock);
	if (!ba_entry->skb_head) {
		ba_entry->skb_head = skb_head;
		ba_entry->skb_last = skb_last;
	} else {
		ba_entry->skb_last->next = skb_head;
		ba_entry->skb_last = skb_last;
	}
	ba_entry->skb_last->next = NULL;
	spin_unlock_bh(&ba_entry->skb_list_lock);
}

#ifdef SPLIT_STACK
struct sk_buff *reorder_get_skb_list(struct sprdwl_rx_ba_entry *ba_entry)
#else
static inline struct sk_buff
*reorder_get_skb_list(struct sprdwl_rx_ba_entry *ba_entry)
#endif
{
	struct sk_buff *skb = NULL;

	spin_lock_bh(&ba_entry->skb_list_lock);
	skb = ba_entry->skb_head;
	ba_entry->skb_head = NULL;
	ba_entry->skb_last = NULL;
	spin_unlock_bh(&ba_entry->skb_list_lock);

	return skb;
}

static inline void mod_reorder_timer(struct rx_ba_node *ba_node)
{
	if (ba_node->rx_ba->buff_cnt) {
		mod_timer(&ba_node->reorder_timer,
			  jiffies + RX_BA_LOSS_RECOVERY_TIMEOUT);
	} else {
		del_timer(&ba_node->reorder_timer);
		ba_node->timeout_cnt = 0;
	}
}

static inline bool is_same_pn(struct rx_ba_pkt_desc *ba_pkt_desc,
				  struct rx_msdu_desc *msdu_desc)
{
	bool ret = true;
	unsigned char cipher_type = 0;

	cipher_type = ba_pkt_desc->cipher_type;
	if ((cipher_type == SPRDWL_HW_TKIP) ||
		(cipher_type == SPRDWL_HW_CCMP)) {
		if ((ba_pkt_desc->pn_l != msdu_desc->pn_l) ||
			(ba_pkt_desc->pn_h != msdu_desc->pn_h))
			ret = false;
	}

	return ret;
}

static inline bool replay_detection(struct rx_ba_pkt_desc *ba_pkt_desc,
					struct rx_ba_node_desc *ba_node_desc)
{
	bool ret = true;
	unsigned int old_val_low = 0;
	unsigned int old_val_high = 0;
	unsigned int rx_val_low = 0;
	unsigned int rx_val_high = 0;
	unsigned char cipher_type = 0;

	/* FIXME: Need to check sta entry instead of check cipher_type param */
	cipher_type = ba_pkt_desc->cipher_type;

	/* FIXME: Maybe other cipher type need to do replay detection
	 *	  HW do not support other cipher type now
	 */
	if ((cipher_type == SPRDWL_HW_TKIP) ||
		(cipher_type == SPRDWL_HW_CCMP)) {
		old_val_low = ba_node_desc->pn_l;
		old_val_high = ba_node_desc->pn_h;
		rx_val_low = ba_pkt_desc->pn_l;
		rx_val_high = ba_pkt_desc->pn_h;

		if ((1 == ba_node_desc->reset_pn) &&
			(old_val_low >= rx_val_low) && (old_val_high >= rx_val_high)) {
			wl_err("%s: clear reset_pn,old_val_low: %d, old_val_high: %d, rx_val_low: %d, rx_val_high: %d\n",
				   __func__, old_val_low, old_val_high, rx_val_low, rx_val_high);
			ba_node_desc->reset_pn = 0;
			ba_node_desc->pn_l = rx_val_low;
			ba_node_desc->pn_h = rx_val_high;
		} else if (((old_val_high == rx_val_high) &&
			 (old_val_low < rx_val_low)) ||
			(old_val_high < rx_val_high)) {
			ba_node_desc->pn_l = rx_val_low;
			ba_node_desc->pn_h = rx_val_high;
		} else {
			ret = false;
			wl_err("%s: old_val_low: %d, old_val_high: %d\n",
				__func__, old_val_low, old_val_high);
			wl_err("%s: rx_val_low: %d, rx_val_high: %d\n",
				__func__, rx_val_low, rx_val_high);
		}
	}

	return ret;
}

static inline void send_order_msdu(struct sprdwl_rx_ba_entry *ba_entry,
				   struct rx_msdu_desc *msdu_desc,
				   struct sk_buff *skb,
				   struct rx_ba_node_desc *ba_node_desc)
{
	struct rx_ba_pkt_desc ba_pkt_desc;

	set_ba_pkt_desc(&ba_pkt_desc, msdu_desc);
	ba_node_desc->win_start = SEQNO_ADD(ba_node_desc->win_start, 1);
	ba_node_desc->win_limit = SEQNO_ADD(ba_node_desc->win_limit, 1);
	ba_node_desc->win_tail = SEQNO_SUB(ba_node_desc->win_start, 1);

	wl_debug("%s: seq: %d\n", __func__, ba_pkt_desc.seq);
	wl_debug("%s: win_start: %d, win_tail: %d, buff_cnt: %d\n",
		 __func__, ba_node_desc->win_start,
		 ba_node_desc->win_tail, ba_node_desc->buff_cnt);

	if (skb) {
		if (replay_detection(&ba_pkt_desc, ba_node_desc))
			reorder_set_skb_list(ba_entry, skb, skb);
		else
			dev_kfree_skb(skb);
	}
}

static inline void
send_reorder_skb(struct sprdwl_rx_ba_entry *ba_entry,
		 struct rx_ba_node_desc *ba_node_desc, struct rx_ba_pkt *pkt)
{
	wl_debug("%s: seq: %d, msdu num: %d\n", __func__,
		 pkt->desc.seq, pkt->desc.msdu_num);

	if (pkt->skb && pkt->skb_last) {
		if (replay_detection(&pkt->desc, ba_node_desc))
			reorder_set_skb_list(ba_entry, pkt->skb, pkt->skb_last);
		else
			kfree_skb_list(pkt->skb);
	}

	memset(pkt, 0, sizeof(struct rx_ba_pkt));
}

static inline void flush_reorder_buffer(struct rx_ba_node_desc *ba_node_desc)
{
	int i = 0;
	struct rx_ba_pkt *pkt = NULL;

	for (i = 0; i < (ba_node_desc->index_mask + 1); i++) {
		pkt = &ba_node_desc->reorder_buffer[i];
		kfree_skb_list(pkt->skb);
		memset(pkt, 0, sizeof(struct rx_ba_pkt));
	}

	ba_node_desc->buff_cnt = 0;
}

static inline void joint_msdu(struct rx_ba_pkt *pkt, struct sk_buff *newsk)
{
	if (newsk) {
		if (pkt->skb_last) {
			pkt->skb_last->next = newsk;
			pkt->skb_last = pkt->skb_last->next;
		} else {
			pkt->skb = newsk;
			pkt->skb_last = pkt->skb;
		}
		pkt->skb_last->next = NULL;
	}
}

static unsigned short send_msdu_in_order(struct sprdwl_rx_ba_entry *ba_entry,
					 struct rx_ba_node_desc *ba_node_desc)
{
	unsigned short seq_num = ba_node_desc->win_start;
	struct rx_ba_pkt *pkt = NULL;
	unsigned int index = 0;

	while (1) {
		index = seq_num & ba_node_desc->index_mask;
		pkt = &ba_node_desc->reorder_buffer[index];
		if (!ba_node_desc->buff_cnt || !pkt->desc.last)
			break;

		send_reorder_skb(ba_entry, ba_node_desc, pkt);
		ba_node_desc->buff_cnt--;
		seq_num++;
	}

	seq_num &= SEQNO_MASK;
	return seq_num;
}

static unsigned short
send_msdu_with_gap(struct sprdwl_rx_ba_entry *ba_entry,
		   struct rx_ba_node_desc *ba_node_desc,
		   unsigned short last_seqno)
{
	unsigned short seq_num = ba_node_desc->win_start;
	struct rx_ba_pkt *pkt = NULL;
	unsigned short num_frms = 0;
	unsigned short num = SEQNO_SUB(last_seqno, seq_num);
	unsigned int index = 0;

	while (num--) {
		index = seq_num & ba_node_desc->index_mask;
		pkt = &ba_node_desc->reorder_buffer[index];
		if (ba_node_desc->buff_cnt && pkt->desc.msdu_num) {
			send_reorder_skb(ba_entry, ba_node_desc, pkt);
			num_frms++;
			ba_node_desc->buff_cnt--;
		}
		seq_num++;
	}

	return num_frms;
}

static inline void between_seqlo_seqhi(struct sprdwl_rx_ba_entry *ba_entry,
					   struct rx_ba_node_desc *ba_node_desc)
{
	ba_node_desc->win_start = send_msdu_in_order(ba_entry, ba_node_desc);
	ba_node_desc->win_limit = SEQNO_ADD(ba_node_desc->win_start,
						(ba_node_desc->win_size - 1));
}

static inline void
greater_than_seqhi(struct sprdwl_rx_ba_entry *ba_entry,
		   struct rx_ba_node_desc *ba_node_desc, unsigned short seq_num)
{
	unsigned short pos_win_end;
	unsigned short pos_win_start;

	pos_win_end = seq_num;
	pos_win_start = SEQNO_SUB(pos_win_end, (ba_node_desc->win_size - 1));
	send_msdu_with_gap(ba_entry, ba_node_desc, pos_win_start);
	ba_node_desc->win_start = pos_win_start;
	ba_node_desc->win_start = send_msdu_in_order(ba_entry, ba_node_desc);
	ba_node_desc->win_limit = SEQNO_ADD(ba_node_desc->win_start,
						(ba_node_desc->win_size - 1));
}

static inline void bar_send_ba_buffer(struct sprdwl_rx_ba_entry *ba_entry,
					  struct rx_ba_node_desc *ba_node_desc,
					  unsigned short seq_num)
{
	if (!seqno_leq(seq_num, ba_node_desc->win_start)) {
		send_msdu_with_gap(ba_entry, ba_node_desc, seq_num);
		ba_node_desc->win_start = seq_num;
		ba_node_desc->win_start =
				send_msdu_in_order(ba_entry, ba_node_desc);
		ba_node_desc->win_limit =
				SEQNO_ADD(ba_node_desc->win_start,
					  (ba_node_desc->win_size - 1));
	}
}

static inline int
insert_msdu(struct rx_msdu_desc *msdu_desc, struct sk_buff *skb,
		struct rx_ba_node_desc *ba_node_desc)
{
	int ret = 0;
	unsigned short seq_num = msdu_desc->seq_num;
	unsigned short index = seq_num & ba_node_desc->index_mask;
	struct rx_ba_pkt *insert = &ba_node_desc->reorder_buffer[index];
	bool last_msdu_flag = msdu_desc->last_msdu_of_mpdu;

	wl_debug("%s: index: %d, seq: %d\n", __func__, index, insert->desc.seq);

	if (insert->desc.msdu_num != 0) {
		if ((insert->desc.seq == seq_num) && (insert->desc.last != 1) &&
			is_same_pn(&insert->desc, msdu_desc)) {
			joint_msdu(insert, skb);
			insert->desc.msdu_num++;
			insert->desc.last = last_msdu_flag;
		} else {
			wl_err("%s: in_use: %d\n", __func__, insert->desc.seq);
			ret = -EINVAL;
		}
	} else {
		joint_msdu(insert, skb);
		set_ba_pkt_desc(&insert->desc, msdu_desc);
		insert->desc.last = last_msdu_flag;
		insert->desc.msdu_num = 1;
		ba_node_desc->buff_cnt++;
	}

	return ret;
}

static int reorder_msdu(struct sprdwl_rx_ba_entry *ba_entry,
			struct rx_msdu_desc *msdu_desc, struct sk_buff *skb,
			struct rx_ba_node *ba_node)
{
	int ret = -EINVAL;
	unsigned short seq_num = msdu_desc->seq_num;
	struct rx_ba_node_desc *ba_node_desc = ba_node->rx_ba;

	if (seqno_geq(seq_num, ba_node_desc->win_start)) {
		if (!seqno_leq(seq_num, ba_node_desc->win_limit)) {
			/* Buffer is full, send data now */
			greater_than_seqhi(ba_entry, ba_node_desc, seq_num);
		}

		ret = insert_msdu(msdu_desc, skb, ba_node_desc);
		if (!ret && seqno_geq(seq_num, ba_node_desc->win_tail))
			ba_node_desc->win_tail = seq_num;
	} else {
		wl_debug("%s: seq_num: %d is less than win_start: %d\n",
			   __func__, seq_num, ba_node_desc->win_start);
	}

	if (ret && skb) {
		wl_debug("%s: kfree skb %d", __func__, ret);
		dev_kfree_skb(skb);
	}

	return ret;
}

static void reorder_msdu_process(struct sprdwl_rx_ba_entry *ba_entry,
				 struct rx_msdu_desc *msdu_desc,
				 struct sk_buff *skb,
				 struct rx_ba_node *ba_node)
{
	struct rx_ba_node_desc *ba_node_desc = ba_node->rx_ba;
	int ret = 0;
	int seq_num = msdu_desc->seq_num;
	bool last_msdu_flag = msdu_desc->last_msdu_of_mpdu;
	unsigned short old_win_start = 0;

	spin_lock_bh(&ba_node->ba_node_lock);
	if (likely(ba_node->active)) {
		wl_debug("%s: seq: %d, last_msdu_of_mpdu: %d\n",
			 __func__, seq_num, last_msdu_flag);
		wl_debug("%s: win_start: %d, win_tail: %d, buff_cnt: %d\n",
			 __func__, ba_node_desc->win_start,
			ba_node_desc->win_tail, ba_node_desc->buff_cnt);

		/* FIXME: Data come in sequence in default */
		if ((seq_num == ba_node_desc->win_start) &&
			!ba_node_desc->buff_cnt && last_msdu_flag) {
			send_order_msdu(ba_entry, msdu_desc, skb, ba_node_desc);
			goto out;
		}

		old_win_start = ba_node_desc->win_start;
		ret = reorder_msdu(ba_entry, msdu_desc, skb, ba_node);
		if (!ret) {
			if (last_msdu_flag &&
				(seq_num == ba_node_desc->win_start)) {
				between_seqlo_seqhi(ba_entry, ba_node_desc);
				mod_reorder_timer(ba_node);
			} else if (!timer_pending(&ba_node->reorder_timer) ||
				   (old_win_start != ba_node_desc->win_start)) {
				wl_debug("%s: start timer\n", __func__);
				mod_reorder_timer(ba_node);
			}
		} else if (unlikely(!ba_node_desc->buff_cnt)) {
			/* Should never happen */
			del_timer(&ba_node->reorder_timer);
			ba_node->timeout_cnt = 0;
		}
	} else {
		//wl_err("%s: BA SESSION IS NO ACTIVE sta_lut_index: %d, tid: %d\n",
		//	   __func__, msdu_desc->sta_lut_index, msdu_desc->tid);
		reorder_set_skb_list(ba_entry, skb, skb);
	}

out:
	spin_unlock_bh(&ba_node->ba_node_lock);
}

static inline void init_ba_node(struct sprdwl_rx_ba_entry *ba_entry,
				struct rx_ba_node *ba_node,
				unsigned char sta_lut_index, unsigned char tid)
{
	/* Init reorder spinlock */
	spin_lock_init(&ba_node->ba_node_lock);

	ba_node->active = 0;
	ba_node->sta_lut_index = sta_lut_index;
	ba_node->tid = tid;
	ba_node->ba_entry = ba_entry;

	/* Init reorder timer */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	timer_setup(&ba_node->reorder_timer, ba_reorder_timeout, 0);
#else
	init_timer(&ba_node->reorder_timer);
	ba_node->reorder_timer.data = (unsigned long)ba_node;
	ba_node->reorder_timer.function = ba_reorder_timeout;
#endif
}

static inline bool is_same_ba(struct rx_ba_node *ba_node,
				  unsigned char sta_lut_index, unsigned char tid)
{
	bool ret = false;

	if (ba_node) {
		if ((ba_node->sta_lut_index == sta_lut_index) &&
			(ba_node->tid == tid))
			ret = true;
	}

	return ret;
}

static struct rx_ba_node
*find_ba_node(struct sprdwl_rx_ba_entry *ba_entry,
		  unsigned char sta_lut_index, unsigned char tid)
{
	struct rx_ba_node *ba_node = NULL;

	if (tid < NUM_TIDS) {
		if (is_same_ba(ba_entry->current_ba_node, sta_lut_index, tid)) {
			ba_node = ba_entry->current_ba_node;
		} else {
			struct hlist_head *head = &ba_entry->hlist[tid];

			if (!hlist_empty(head)) {
				hlist_for_each_entry(ba_node, head, hlist) {
					if (sta_lut_index ==
						ba_node->sta_lut_index) {
						ba_entry->current_ba_node =
							ba_node;
						break;
					}
				}
			}
		}
	} else {
		wl_err("%s: TID is too large sta_lut_index: %d, tid: %d\n",
			   __func__, sta_lut_index, tid);
	}

	return ba_node;
}

static struct rx_ba_node
*create_ba_node(struct sprdwl_rx_ba_entry *ba_entry,
		unsigned char sta_lut_index, unsigned char tid,
		unsigned int size)
{
	struct rx_ba_node *ba_node = NULL;
	struct hlist_head *head = &ba_entry->hlist[tid];
	unsigned int rx_ba_size = sizeof(struct rx_ba_node_desc) +
				(size * sizeof(struct rx_ba_pkt));

	ba_node = kzalloc(sizeof(*ba_node), GFP_ATOMIC);
	if (ba_node) {
		ba_node->rx_ba = kzalloc(rx_ba_size, GFP_ATOMIC);
		if (ba_node->rx_ba) {
			init_ba_node(ba_entry, ba_node, sta_lut_index, tid);
			INIT_HLIST_NODE(&ba_node->hlist);
			hlist_add_head(&ba_node->hlist, head);
			ba_entry->current_ba_node = ba_node;
		} else {
			kfree(ba_node);
			ba_node = NULL;
		}
	}

	return ba_node;
}

void reset_pn(struct sprdwl_priv *priv, const u8 *mac_addr)
{
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_rx_if *rx_if = (struct sprdwl_rx_if *)intf->sprdwl_rx;
	struct sprdwl_rx_ba_entry *ba_entry = &rx_if->ba_entry;
	unsigned char i, tid, lut_id = 0xff;
	struct rx_ba_node *ba_node = NULL;

	if (!mac_addr || !priv)
		return;

	for (i = 0; i < MAX_LUT_NUM; i++) {
		if (ether_addr_equal(intf->peer_entry[i].tx.da, mac_addr)) {
			lut_id = intf->peer_entry[i].lut_index;
			break;
		}
	}
	if (lut_id == 0xff)
		return;

	for (tid = 0; tid < NUM_TIDS; tid++) {
		ba_node = find_ba_node(ba_entry, lut_id, tid);
		if (ba_node) {
			spin_lock_bh(&ba_node->ba_node_lock);
			ba_node->rx_ba->reset_pn = 1;
			wl_info("%s: set,lut=%d,tid=%d,pn_l=%d,pn_h=%d\n",
				   __func__, lut_id, tid,
				   ba_node->rx_ba->pn_l,
				   ba_node->rx_ba->pn_h);
			spin_unlock_bh(&ba_node->ba_node_lock);
		}
	}
}

struct sk_buff *reorder_data_process(struct sprdwl_rx_ba_entry *ba_entry,
					 struct sk_buff *pskb)
{
	struct rx_ba_node *ba_node = NULL;
	struct rx_msdu_desc *msdu_desc = NULL;

	if (pskb) {
		msdu_desc = (struct rx_msdu_desc *)pskb->data;

		wl_debug("%s: qos_flag: %d, ampdu_flag: %d, bc_mc_flag: %d\n",
			 __func__, msdu_desc->qos_flag,
			msdu_desc->ampdu_flag, msdu_desc->bc_mc_flag);

		if (!msdu_desc->bc_mc_flag && msdu_desc->qos_flag) {
			ba_node = find_ba_node(ba_entry,
						   msdu_desc->sta_lut_index,
						   msdu_desc->tid);
			if (ba_node)
				reorder_msdu_process(ba_entry, msdu_desc,
							 pskb, ba_node);
			else
				reorder_set_skb_list(ba_entry, pskb, pskb);
		} else {
			reorder_set_skb_list(ba_entry, pskb, pskb);
		}
	}

#ifdef SPLIT_STACK
	return NULL;
#else
	return reorder_get_skb_list(ba_entry);
#endif
}

static void wlan_filter_event(struct sprdwl_rx_ba_entry *ba_entry,
				  struct sprdwl_event_ba *ba_event)
{
	struct rx_ba_node *ba_node = NULL;
	struct rx_msdu_desc msdu_desc;

	ba_node = find_ba_node(ba_entry,
				   ba_event->sta_lut_index, ba_event->tid);
	if (ba_node) {
		msdu_desc.last_msdu_of_mpdu = 1;
		msdu_desc.seq_num = ba_event->msdu_param.seq_num;
		msdu_desc.msdu_index_of_mpdu = 0;
		msdu_desc.pn_l = 0;
		msdu_desc.pn_h = 0;
		msdu_desc.cipher_type = 0;
		reorder_msdu_process(ba_entry, &msdu_desc, NULL, ba_node);
	}
}

static void wlan_delba_event(struct sprdwl_rx_ba_entry *ba_entry,
				 struct sprdwl_event_ba *ba_event)
{
	struct rx_ba_node *ba_node = NULL;
	struct rx_ba_node_desc *ba_node_desc = NULL;

	wl_info("enter %s\n", __func__);
	ba_node = find_ba_node(ba_entry,
				   ba_event->sta_lut_index, ba_event->tid);
	if (!ba_node) {
		wl_err("%s: NOT FOUND sta_lut_index: %d, tid: %d\n",
			   __func__, ba_event->sta_lut_index, ba_event->tid);
		return;
	}

	del_timer_sync(&ba_node->reorder_timer);
	spin_lock_bh(&ba_node->ba_node_lock);
	if (ba_node->active) {
		ba_node_desc = ba_node->rx_ba;
		ba_node->active = 0;
		ba_node->timeout_cnt = 0;
		between_seqlo_seqhi(ba_entry, ba_node_desc);
		flush_reorder_buffer(ba_node_desc);
	}
	hlist_del(&ba_node->hlist);
	spin_unlock_bh(&ba_node->ba_node_lock);

	kfree(ba_node->rx_ba);
	kfree(ba_node);
	ba_node = NULL;
	ba_entry->current_ba_node = NULL;
}

#if 0
static void wlan_bar_event(struct sprdwl_rx_ba_entry *ba_entry,
			   struct sprdwl_event_ba *ba_event)
{
	struct rx_ba_node *ba_node = NULL;
	struct rx_ba_node_desc *ba_node_desc = NULL;

	wl_info("enter %s\n", __func__);
	ba_node = find_ba_node(ba_entry,
				   ba_event->sta_lut_index, ba_event->tid);
	if (!ba_node) {
		wl_err("%s: NOT FOUND sta_lut_index: %d, tid: %d\n",
			   __func__, ba_event->sta_lut_index, ba_event->tid);
		return;
	}

	spin_lock_bh(&ba_node->ba_node_lock);
	if (ba_node->active) {
		ba_node_desc = ba_node->rx_ba;
		if (!seqno_leq(ba_event->win_param.win_start,
				   ba_node_desc->win_start)) {
			bar_send_ba_buffer(ba_entry, ba_node_desc,
					   ba_event->win_param.win_start);
			mod_reorder_timer(ba_node);
		}

		wl_info("%s:(active:%d, tid:%d)\n",
			__func__, ba_node->active, ba_node->tid);
		wl_info("%s:(win_size:%d, win_start:%d, win_tail:%d)\n",
			__func__, ba_node_desc->win_size,
			ba_node_desc->win_start, ba_node_desc->win_tail);
	} else {
		wl_err("%s: BA SESSION IS NO ACTIVE sta_lut_index: %d, tid: %d\n",
			   __func__, ba_event->sta_lut_index, ba_event->tid);
	}
	spin_unlock_bh(&ba_node->ba_node_lock);
}
#endif

static void send_addba_rsp(struct sprdwl_rx_ba_entry *ba_entry,
			   unsigned char tid, unsigned char sta_lut_index,
			   int status)
{
	struct sprdwl_ba_event_data ba_data;
	struct sprdwl_intf *intf = NULL;
	struct sprdwl_peer_entry *peer_entry = NULL;
	struct sprdwl_rx_if *rx_if = container_of(ba_entry,
						  struct sprdwl_rx_if,
						  ba_entry);

	intf = rx_if->intf;
	peer_entry =
		sprdwl_find_peer_entry_using_lut_index(intf, sta_lut_index);
	if (peer_entry == NULL) {
		wl_err("%s, peer not found\n", __func__);
		return;
	}

	ba_data.addba_rsp.type = SPRDWL_ADDBA_RSP_CMD;
	ba_data.addba_rsp.tid = tid;
	ether_addr_copy(ba_data.addba_rsp.da, peer_entry->tx.da);
	ba_data.addba_rsp.success = (status) ? 0 : 1;
	ba_data.ba_entry = ba_entry;
	ba_data.sta_lut_index = sta_lut_index;

	sprdwl_rx_send_cmd(intf, (void *)(&ba_data), sizeof(ba_data),
			   SPRDWL_WORK_BA_MGMT, peer_entry->ctx_id);
}

static void send_delba(struct sprdwl_rx_ba_entry *ba_entry,
			   unsigned short tid, unsigned char sta_lut_index)
{
	struct sprdwl_cmd_ba delba;
	struct sprdwl_intf *intf = NULL;
	struct sprdwl_peer_entry *peer_entry = NULL;
	struct sprdwl_rx_if *rx_if = container_of(ba_entry,
						  struct sprdwl_rx_if,
						  ba_entry);

	intf = rx_if->intf;
	peer_entry =
		sprdwl_find_peer_entry_using_lut_index(intf, sta_lut_index);
	if (peer_entry == NULL) {
		wl_err("%s, peer not found\n", __func__);
		return;
	}

	delba.type = SPRDWL_DELBA_CMD;
	delba.tid = tid;
	ether_addr_copy(delba.da, peer_entry->tx.da);
	delba.success = 1;

	sprdwl_rx_send_cmd(intf, (void *)(&delba), sizeof(delba),
			   SPRDWL_WORK_BA_MGMT, peer_entry->ctx_id);
}

static int wlan_addba_event(struct sprdwl_rx_ba_entry *ba_entry,
				struct sprdwl_event_ba *ba_event)
{
	struct rx_ba_node *ba_node = NULL;
	int ret = 0;
	unsigned char sta_lut_index = ba_event->sta_lut_index;
	unsigned char tid = ba_event->tid;
	unsigned short win_start = ba_event->win_param.win_start;
	unsigned short win_size = ba_event->win_param.win_size;
	unsigned int index_size = get_index_size(2 * win_size);

	wl_info("enter %s\n", __func__);
	ba_node = find_ba_node(ba_entry, sta_lut_index, tid);
	if (!ba_node) {
		ba_node = create_ba_node(ba_entry, sta_lut_index,
					 tid, index_size);
		if (!ba_node) {
			wl_err("%s: Create ba_entry fail\n", __func__);
			ret = -ENOMEM;
			goto out;
		}
	}

	spin_lock_bh(&ba_node->ba_node_lock);
#ifdef CP2_RESET_SUPPORT
	ba_node->active = 0;
#endif
	if (likely(!ba_node->active)) {
		set_ba_node_desc(ba_node->rx_ba, win_start, win_size,
				 INDEX_SIZE_MASK(index_size));
#if 0
		ba_node->active = 1;
		wl_debug("%s:(active:%d, tid:%d)\n",
			 __func__, ba_node->active, ba_node->tid);
#endif
	} else {
		/* Should never happen */
		wl_err("%s: BA SESSION IS ACTIVE sta_lut_index: %d, tid: %d\n",
			   __func__, sta_lut_index, tid);
		ret = -EINVAL;
	}
	spin_unlock_bh(&ba_node->ba_node_lock);

out:
	return ret;
}

void wlan_ba_session_event(void *hw_intf,
			   unsigned char *data, unsigned short len)
{
	struct sprdwl_intf *intf = (struct sprdwl_intf *)hw_intf;
	struct sprdwl_rx_if *rx_if = (struct sprdwl_rx_if *)intf->sprdwl_rx;
	struct sprdwl_rx_ba_entry *ba_entry = &rx_if->ba_entry;
	struct sprdwl_event_ba *ba_event =
				  (struct sprdwl_event_ba *)data;
	unsigned char type = ba_event->type;
	int ret = 0;
	struct sprdwl_peer_entry *peer_entry = NULL;
	u8 qos_index;

	switch (type) {
	case SPRDWL_ADDBA_REQ_EVENT:
		ret = wlan_addba_event(ba_entry, ba_event);
		send_addba_rsp(ba_entry, ba_event->tid,
				   ba_event->sta_lut_index, ret);
		break;
	case SPRDWL_DELBA_EVENT:
		wlan_delba_event(ba_entry, ba_event);
		break;
	case SPRDWL_BAR_EVENT:
		/*wlan_bar_event(ba_entry, ba_event);*/
		break;
	case SPRDWL_FILTER_EVENT:
		wlan_filter_event(ba_entry, ba_event);
		break;
	case SPRDWL_DELTXBA_EVENT:
		peer_entry = &intf->peer_entry[ba_event->sta_lut_index];
		qos_index = tid_map_to_qosindex(ba_event->tid);
		peer_entry = &intf->peer_entry[ba_event->sta_lut_index];
		if (test_and_clear_bit(ba_event->tid, &peer_entry->ba_tx_done_map))
			wl_info("%s, %d, deltxba, lut=%d, tid=%d, map=%lu\n",
				__func__, __LINE__,
				ba_event->sta_lut_index,
				ba_event->tid,
				peer_entry->ba_tx_done_map);
		break;
	default:
		wl_err("%s: Error type: %d\n", __func__, type);
		break;
	}

	/* TODO:Should we handle skb list here? */
}

void sprdwl_reorder_init(struct sprdwl_rx_ba_entry *ba_entry)
{
	int i = 0;

	for (i = 0; i < NUM_TIDS; i++)
		INIT_HLIST_HEAD(&ba_entry->hlist[i]);

	spin_lock_init(&ba_entry->skb_list_lock);
}

void sprdwl_reorder_deinit(struct sprdwl_rx_ba_entry *ba_entry)
{
	int i = 0;
	struct rx_ba_node *ba_node = NULL;

	for (i = 0; i < NUM_TIDS; i++) {
		struct hlist_head *head = &ba_entry->hlist[i];
		struct hlist_node *node = NULL;

		if (hlist_empty(head))
			continue;

		hlist_for_each_entry_safe(ba_node, node, head, hlist) {
			del_timer_sync(&ba_node->reorder_timer);
			spin_lock_bh(&ba_node->ba_node_lock);
			ba_node->active = 0;
			flush_reorder_buffer(ba_node->rx_ba);
			hlist_del(&ba_node->hlist);
			spin_unlock_bh(&ba_node->ba_node_lock);
			kfree(ba_node->rx_ba);
			kfree(ba_node);
			ba_node = NULL;
		}
	}
}

static unsigned short
get_first_seqno_in_buff(struct rx_ba_node_desc *ba_node_desc)
{
	unsigned short seqno = ba_node_desc->win_start;
	unsigned short index = 0;

	while (seqno_leq(seqno, ba_node_desc->win_tail)) {
		index = seqno & ba_node_desc->index_mask;
		if (ba_node_desc->reorder_buffer[index].desc.last)
			break;

		seqno = SEQNO_ADD(seqno, 1);
	}

	wl_info("%s: first seqno: %d\n", __func__, seqno);
	return seqno;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static void ba_reorder_timeout(struct timer_list *t)
{
	struct rx_ba_node *ba_node = from_timer(ba_node, t, reorder_timer);
#else
static void ba_reorder_timeout(unsigned long data)
{
	struct rx_ba_node *ba_node = (struct rx_ba_node *)data;
#endif
	struct sprdwl_rx_ba_entry *ba_entry = ba_node->ba_entry;
	struct rx_ba_node_desc *ba_node_desc = ba_node->rx_ba;
	struct sprdwl_rx_if *rx_if = container_of(ba_entry,
						  struct sprdwl_rx_if,
						  ba_entry);
	unsigned short pos_seqno = 0;

	wl_info("enter %s\n", __func__);
	debug_cnt_inc(REORDER_TIMEOUT_CNT);
	spin_lock_bh(&ba_node->ba_node_lock);
	if (ba_node->active && ba_node_desc->buff_cnt &&
		!timer_pending(&ba_node->reorder_timer)) {
		pos_seqno = get_first_seqno_in_buff(ba_node_desc);
		send_msdu_with_gap(ba_entry, ba_node_desc, pos_seqno);
		ba_node_desc->win_start = pos_seqno;
		ba_node_desc->win_start =
				send_msdu_in_order(ba_entry, ba_node_desc);
		ba_node_desc->win_limit =
				SEQNO_ADD(ba_node_desc->win_start,
					  (ba_node_desc->win_size - 1));

		ba_node->timeout_cnt++;
		if (ba_node->timeout_cnt > MAX_TIMEOUT_CNT) {
			ba_node->active = 0;
			ba_node->timeout_cnt = 0;
			wl_info("%s, %d, send_delba\n", __func__, __LINE__);
			send_delba(ba_entry, ba_node->tid,
				   ba_node->sta_lut_index);
		}

		mod_reorder_timer(ba_node);
	}
	spin_unlock_bh(&ba_node->ba_node_lock);

	spin_lock_bh(&ba_entry->skb_list_lock);
	if (ba_entry->skb_head) {
		spin_unlock_bh(&ba_entry->skb_list_lock);

#ifndef RX_NAPI
		if (!work_pending(&rx_if->rx_work)) {
			wl_info("%s: queue rx workqueue\n", __func__);
			queue_work(rx_if->rx_queue, &rx_if->rx_work);
		}
#else
		napi_schedule(&rx_if->napi_rx);
#endif
	} else {
		spin_unlock_bh(&ba_entry->skb_list_lock);
	}
	wl_info("leave %s\n", __func__);
}

void peer_entry_delba(void *hw_intf, unsigned char lut_index)
{
	int tid = 0;
	struct rx_ba_node *ba_node = NULL;
	struct rx_ba_node_desc *ba_node_desc = NULL;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)hw_intf;
	struct sprdwl_rx_if *rx_if = (struct sprdwl_rx_if *)intf->sprdwl_rx;
	struct sprdwl_rx_ba_entry *ba_entry = &rx_if->ba_entry;

	wl_info("enter %s\n", __func__);
	for (tid = 0; tid < NUM_TIDS; tid++) {
		ba_node = find_ba_node(ba_entry, lut_index, tid);
		if (ba_node) {
			wl_info("%s: del ba lut_index: %d, tid %d\n",
				__func__, lut_index, tid);
			del_timer_sync(&ba_node->reorder_timer);
			spin_lock_bh(&ba_node->ba_node_lock);
			if (ba_node->active) {
				ba_node_desc = ba_node->rx_ba;
				ba_node->active = 0;
				ba_node->timeout_cnt = 0;
				flush_reorder_buffer(ba_node_desc);
			}
			hlist_del(&ba_node->hlist);
			spin_unlock_bh(&ba_node->ba_node_lock);

			kfree(ba_node->rx_ba);
			kfree(ba_node);
			ba_node = NULL;
			ba_entry->current_ba_node = NULL;
		}
	}
}

void sprdwl_active_ba_node(struct sprdwl_rx_ba_entry *ba_entry,
				  u8 sta_lut_index, u8 tid)
{
	struct rx_ba_node *ba_node = NULL;

	ba_node = find_ba_node(ba_entry, sta_lut_index, tid);
	if (ba_node == NULL) {
		wl_err("BA node not found, tid = %d\n", tid);
		return;
	}

	spin_lock_bh(&ba_node->ba_node_lock);
	ba_node->active = 1;
	spin_unlock_bh(&ba_node->ba_node_lock);
	wl_info("%s BA active tid = %d\n", __func__, tid);
}
