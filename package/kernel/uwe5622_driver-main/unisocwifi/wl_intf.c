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

#include <linux/ip.h>
#include "sprdwl.h"
#include "wl_intf.h"
#include "intf_ops.h"
#include "mm.h"
#include "tx_msg.h"
#include "rx_msg.h"
#include "work.h"
#include "tcp_ack.h"

#define INIT_INTF(num, type, out, interval, bsize, psize, max,\
			 threshold, time, pop, push, complete, suspend) \
{ .channel = num, .hif_type = type, .inout = out, .intr_interval = interval,\
.buf_size = bsize, .pool_size = psize, .once_max_trans = max,\
.rx_threshold = threshold, .timeout = time, .pop_link = pop,\
.push_link = push, .tx_complete = complete, .power_notify = suspend }

#ifndef CPUFREQ_ADJUST
#define CPUFREQ_ADJUST CPUFREQ_CREATE_POLICY
#endif

struct sprdwl_intf_ops g_intf_ops;

static inline struct sprdwl_intf *get_intf(void)
{
	return (struct sprdwl_intf *)g_intf_ops.intf;
}

#define INTF_IS_PCIE \
	(get_intf()->priv->hw_type == SPRDWL_HW_PCIE)

void sprdwl_hex_dump(unsigned char *name,
			 unsigned char *data, unsigned short len)
{
	int i, p = 0, ret;
	unsigned char buf[255] = {0};

	if ((NULL == data) || (0 == len) || (NULL == name))
		return;

	sprintf(buf, "sprdwl %s hex dump(len = %d)", name, len);
	wl_info("%s\n", buf);

	if (len > 1024)
		len = 1024;
	memset(buf, 0x00, 255);
	for (i = 0; i < len ; i++) {
		ret = sprintf((buf + p), "%02x ", *(data + i));
		if ((i != 0) && ((i + 1)%16 == 0)) {
			wl_info("%s\n", buf);
			p = 0;
			memset(buf, 0x00, 255);
		} else {
			p = p + ret;
		}
	}
	if (p != 0)
		wl_info("%s\n", buf);
}

enum sprdwl_hw_type get_hwintf_type(void)
{
	int hif = 0;
	enum sprdwl_hw_type hw_type;

	hif = sprdwcn_bus_get_hif_type();
	switch (hif) {
	case HW_TYPE_SDIO:
		hw_type = SPRDWL_HW_SDIO;
		break;
	case HW_TYPE_PCIE:
		hw_type = SPRDWL_HW_PCIE;
		break;
	case HW_TYPE_SIPC:
		hw_type = SPRDWL_HW_SIPC;
		break;
	case HW_TYPE_USB:
		hw_type = SPRDWL_HW_USB;
		break;
	default:
		hw_type = SPRDWL_HW_SDIO;
		break;
	}
	return hw_type;
}

#if defined(MORE_DEBUG)
void sprdwl_dump_stats(struct sprdwl_intf *intf)
{
	wl_err("++print txrx statistics++\n");
	wl_err("tx packets: %lu, tx bytes: %lu\n",  intf->stats.tx_packets,
		   intf->stats.tx_bytes);
	wl_err("tx filter num: %lu\n",  intf->stats.tx_filter_num);
	wl_err("tx errors: %lu, tx dropped: %lu\n",  intf->stats.tx_errors,
		   intf->stats.tx_dropped);
	wl_err("tx avg time: %lu\n",  intf->stats.tx_avg_time);
	wl_err("tx realloc: %lu\n",  intf->stats.tx_realloc);
	wl_err("tx arp num: %lu\n",  intf->stats.tx_arp_num);
	wl_err("rx packets: %lu, rx bytes: %lu\n",  intf->stats.rx_packets,
		   intf->stats.rx_bytes);
	wl_err("rx errors: %lu, rx dropped: %lu\n",  intf->stats.rx_errors,
		   intf->stats.rx_dropped);
	wl_err("rx multicast: %lu, tx multicast: %lu\n",
		   intf->stats.rx_multicast, intf->stats.tx_multicast);
	wl_err("--print txrx statistics--\n");
}

void sprdwl_clear_stats(struct sprdwl_intf *intf)
{
	memset(&intf->stats, 0x0, sizeof(struct txrx_stats));
}

/*calculate packets  average sent time from received
*from network stack to freed by HIF every STATS_COUNT packets
*/
void sprdwl_get_tx_avg_time(struct sprdwl_intf *intf,
				unsigned long tx_start_time)
{
	struct timespec tx_end;

	getnstimeofday(&tx_end);
	intf->stats.tx_cost_time +=
	timespec_to_ns(&tx_end) - tx_start_time;
	if (intf->stats.gap_num >= STATS_COUNT) {
		intf->stats.tx_avg_time =
		intf->stats.tx_cost_time / intf->stats.gap_num;
		sprdwl_dump_stats(intf);
		intf->stats.gap_num = 0;
		intf->stats.tx_cost_time = 0;
		wl_info("%s:%d packets avg cost time: %lu\n",
			__func__, __LINE__, intf->stats.tx_avg_time);
	}
}
#endif

void set_coex_bt_on_off(u8 action)
{
	struct sprdwl_intf *intf = get_intf();

	intf->coex_bt_on = action;
}

unsigned long mbufalloc;
unsigned long mbufpop;
int if_tx_one(struct sprdwl_intf *intf, unsigned char *data,
		  int len, int chn)
{
	int ret;
	struct mbuf_t *head = NULL, *tail = NULL, *mbuf = NULL;
	int num = 1;

	ret = sprdwcn_bus_list_alloc(chn, &head, &tail, &num);
	if (ret || head == NULL || tail == NULL) {
		wl_err("%s:%d sprdwcn_bus_list_alloc fail\n",
			   __func__, __LINE__);
		return -1;
	}
	mbufalloc += num;
	mbuf = head;
	mbuf->buf = data;

	mbuf->len = len;
	mbuf->next = NULL;
	if (sprdwl_debug_level >= L_DBG)
		sprdwl_hex_dump("tx to cp2 cmd data dump", data + 4, len);
	if (intf->priv->hw_type == SPRDWL_HW_PCIE) {
		mbuf->phy = mm_virt_to_phys(&intf->pdev->dev, mbuf->buf,
						mbuf->len, DMA_TO_DEVICE);
	}

	ret = sprdwcn_bus_push_list(chn, head, tail, num);
	if (ret) {
		mbuf = head;
		if (intf->priv->hw_type == SPRDWL_HW_PCIE) {
			mm_phys_to_virt(&intf->pdev->dev, mbuf->phy, mbuf->len,
					DMA_TO_DEVICE, false);
			mbuf->phy = 0;
		}
		kfree(mbuf->buf);
		mbuf->buf = NULL;

		sprdwcn_bus_list_free(chn, head, tail, num);
		mbufalloc -= num;
	}

	return ret;
}

inline int if_tx_cmd(struct sprdwl_intf *intf, unsigned char *data, int len)
{
	return if_tx_one(intf, data, len, intf->tx_cmd_port);
}

inline int if_tx_addr_trans(struct sprdwl_intf *intf,
				unsigned char *data, int len)
{
	/* FIXME: Which port is used to send ADDR TRNAS*/
	return if_tx_one(intf, data, len, intf->tx_data_port);
}

static inline struct pcie_addr_buffer
*sprdwl_alloc_pcie_addr_buf(int tx_count)
{
	struct pcie_addr_buffer *addr_buffer;
#define ADDR_OFFSET 5

	addr_buffer =
		kzalloc(sizeof(struct pcie_addr_buffer) +
		tx_count * SPRDWL_PHYS_LEN, GFP_KERNEL);
	if (addr_buffer == NULL) {
		wl_err("%s:%d alloc pcie addr buf fail\n", __func__, __LINE__);
		return NULL;
	}
	addr_buffer->common.type = SPRDWL_TYPE_DATA_PCIE_ADDR;
	addr_buffer->common.direction_ind = 0;
	addr_buffer->number = tx_count;
	addr_buffer->offset = ADDR_OFFSET;
	addr_buffer->buffer_ctrl.buffer_inuse = 1;
	addr_buffer->buffer_ctrl.buffer_type = 1;

	return addr_buffer;
}

static inline struct pcie_addr_buffer
*sprdwl_set_pcie_addr_to_mbuf(struct sprdwl_tx_msg *tx_msg, struct mbuf_t *mbuf,
				   int tx_count)
{
#define ADDR_OFFSET 5
	struct pcie_addr_buffer *addr_buffer;
	struct sprdwl_intf *intf = tx_msg->intf;

	addr_buffer = sprdwl_alloc_pcie_addr_buf(tx_count);
	if (addr_buffer == NULL)
		return NULL;
	mbuf->len = ADDR_OFFSET + tx_count * SPRDWL_PHYS_LEN;
	mbuf->buf = (unsigned char *)addr_buffer;
	mbuf->phy = mm_virt_to_phys(&intf->pdev->dev, mbuf->buf,
					mbuf->len, DMA_TO_DEVICE);

	return addr_buffer;
}

void sprdwl_add_tx_list_head(struct list_head *tx_fail_list,
				 struct list_head *tx_list,
				 int ac_index,
				 int tx_count)
{
	struct sprdwl_msg_buf *msg_buf = NULL;
	struct list_head *xmit_free_list;
	struct list_head *head, *tail;
	spinlock_t *lock;
	spinlock_t *free_lock;

	if (tx_fail_list == NULL)
		return;
	msg_buf = list_first_entry(tx_fail_list, struct sprdwl_msg_buf, list);
	xmit_free_list = &msg_buf->xmit_msg_list->to_free_list;
	free_lock = &msg_buf->xmit_msg_list->free_lock;
	if (msg_buf->msg_type != SPRDWL_TYPE_DATA) {
		lock = &msg_buf->msglist->busylock;
	} else {
		if (SPRDWL_AC_MAX != ac_index)
			lock = &msg_buf->data_list->p_lock;
		else
			lock = &msg_buf->xmit_msg_list->send_lock;
	}
	spin_lock_bh(free_lock);
	head = tx_fail_list->next;
	tail = tx_fail_list->prev;
	head->prev->next = tail->next;
	tail->next->prev = head->prev;
	head->prev = tx_fail_list;
	tail->next = tx_fail_list;
	spin_unlock_bh(free_lock);

	spin_lock_bh(lock);
	list_splice(tx_fail_list, tx_list);
	spin_unlock_bh(lock);
	INIT_LIST_HEAD(tx_fail_list);
}

static inline void
sprdwl_add_to_free_list(struct sprdwl_tx_msg *tx_msg,
			struct list_head *tx_list_head,
			int tx_count)
{
	spin_lock_bh(&tx_msg->xmit_msg_list.free_lock);
	list_splice_tail(tx_list_head, &tx_msg->xmit_msg_list.to_free_list);
	spin_unlock_bh(&tx_msg->xmit_msg_list.free_lock);
}

/*cut data list from tx data list*/
static inline void
sprdwl_list_cut_position(struct list_head *tx_list_head,
			 struct list_head *tx_list,
			 struct list_head *tail_entry, int ac_index)
{
	spinlock_t *lock;
	struct sprdwl_msg_buf *msg_buf = NULL;

	if (tail_entry == NULL)
		return;
	msg_buf = list_first_entry(tx_list, struct sprdwl_msg_buf, list);
	if (msg_buf->msg_type != SPRDWL_TYPE_DATA) {
		lock = &msg_buf->msglist->busylock;
	} else {
		if (SPRDWL_AC_MAX != ac_index)
			lock = &msg_buf->data_list->p_lock;
		else
			lock = &msg_buf->xmit_msg_list->send_lock;
	}
	spin_lock_bh(lock);
	list_cut_position(tx_list_head, tx_list, tail_entry);
	spin_unlock_bh(lock);
}

int sprdwl_add_topop_list(int chn, struct mbuf_t *head,
			   struct mbuf_t *tail, int num)
{
	struct sprdwl_intf *intf = get_intf();
	struct sprdwl_work *misc_work;
	struct sprdwl_pop_work pop_work;

	pop_work.chn = chn;
	pop_work.head = (void *)head;
	pop_work.tail = (void *)tail;
	pop_work.num = num;

	misc_work = sprdwl_alloc_work(sizeof(struct sprdwl_pop_work));
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return -1;
	}
	misc_work->vif = NULL;
	misc_work->id = SPRDWL_POP_MBUF;
	memcpy(misc_work->data, &pop_work, sizeof(struct sprdwl_pop_work));

	sprdwl_queue_work(intf->priv, misc_work);
	return 0;
}

void sprdwl_count_tx_tp(struct sprdwl_tx_msg *tx_msg, int num)
{
	long long timeus = 0;
	struct sprdwl_intf *intf = get_intf();

	tx_msg->tx_data_num += num;
	if (tx_msg->tx_data_num == num) {
		tx_msg->txtimebegin = ktime_get();
		return;
	}

	tx_msg->txtimeend = ktime_get();
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
	timeus = div_u64(tx_msg->txtimeend.tv64 - tx_msg->txtimebegin.tv64, NSEC_PER_USEC);
#else
	timeus = ktime_to_us(tx_msg->txtimeend - tx_msg->txtimebegin);
#endif
	if (div_u64((tx_msg->tx_data_num * 1000), timeus) >= intf->txnum_level &&
		tx_msg->tx_data_num >= 1000) {
		tx_msg->tx_data_num = 0;
		sprdwl_boost();
	} else if (timeus >= USEC_PER_SEC) {
		tx_msg->tx_data_num = 0;
	}
}

unsigned long tx_packets;
int sprdwl_intf_tx_list(struct sprdwl_intf *dev,
			struct list_head *tx_list,
			struct list_head *tx_list_head,
			int tx_count,
			int ac_index,
			u8 coex_bt_on)
{
#define PCIE_TX_NUM 96
	int ret, i = 0, j = PCIE_TX_NUM, pcie_count = 0, cnt = 0, num = 0;
	struct sprdwl_msg_buf *msg_pos, *msg_temp;
	struct pcie_addr_buffer *addr_buffer = NULL;
	struct sprdwl_tx_msg *tx_msg;
	struct mbuf_t *head = NULL, *tail = NULL, *mbuf_pos;
	unsigned long pcie_addr = 0;
	/*struct sprdwl_data_hdr *hdr; *//*temp for test*/
	struct list_head *pos, *tx_list_tail, *n_list;
	unsigned long *msg_ptr;
	unsigned char *data_ptr;
	struct tx_msdu_dscr *dscr;
#if defined(MORE_DEBUG)
	unsigned long tx_bytes = 0;
#endif
	int tx_count_saved = tx_count;
	int list_num;

#ifdef CP2_RESET_SUPPORT
	if (dev->cp_asserted == 1)
		return 0;
#endif

	wl_debug("%s:%d tx_count is %d\n", __func__, __LINE__, tx_count);
	list_num = get_list_num(tx_list);
	if (list_num < tx_count) {
		wl_err("%s, %d, error!, tx_count:%d, list_num:%d\n",
				__func__, __LINE__, tx_count, list_num);
		WARN_ON(1);
	}
	tx_msg = (struct sprdwl_tx_msg *)dev->sprdwl_tx;
	sprdwl_count_tx_tp(tx_msg, tx_count);
	if (dev->priv->hw_type == SPRDWL_HW_PCIE) {
		if (tx_count <= PCIE_TX_NUM) {
			pcie_count = 1;
		} else {
			cnt = tx_count;
			while (cnt > PCIE_TX_NUM) {
				++num;
				cnt -= PCIE_TX_NUM;
			}
			pcie_count = num + 1;
		}
		ret = sprdwcn_bus_list_alloc(dev->tx_data_port, &head,
					  &tail, &pcie_count);
	} else {
		ret = sprdwcn_bus_list_alloc(dev->tx_data_port, &head,
					  &tail, &tx_count);
	}
	if (ret != 0 || head == NULL || tail == NULL) {
		wl_err("%s, %d, mbuf alloc fail\n",
			   __func__, __LINE__);
		sprdwcn_bus_list_free(dev->tx_data_port, head, tail, tx_count);
		return -ENOMEM;
	}
	if (tx_count_saved != tx_count) {
		wl_err("%s, %d error!mbuf not enough%d\n",
			   __func__, __LINE__, (tx_count_saved - tx_count));
		tx_msg->mbuf_short += (tx_count_saved - tx_count);
		sprdwcn_bus_list_free(dev->tx_data_port, head, tail, tx_count);
		return -ENOMEM;
	}
	mbufalloc += tx_count;

	mbuf_pos = head;
	if (dev->priv->hw_type == SPRDWL_HW_PCIE) {
		if (pcie_count > 1) {
			addr_buffer =
			sprdwl_set_pcie_addr_to_mbuf(tx_msg,
							 mbuf_pos, PCIE_TX_NUM);
		} else {
			addr_buffer =
			sprdwl_set_pcie_addr_to_mbuf(tx_msg,
							 mbuf_pos, tx_count);
		}
		if (addr_buffer == NULL) {
			wl_err("%s:%d alloc pcie addr buf fail\n",
				   __func__, __LINE__);
			return -1;
		}
	}
	list_for_each_safe(pos, n_list, tx_list) {
		msg_pos = list_entry(pos, struct sprdwl_msg_buf, list);
		sprdwl_move_tcpack_msg(dev->priv, msg_pos);
		data_ptr = (unsigned char *)(msg_pos->tran_data) -
			dev->hif_offset;
#ifdef OTT_UWE
		dscr = (struct tx_msdu_dscr *)(msg_pos->tran_data + FOUR_BYTES_ALIGN_OFFSET);
#else
		dscr = (struct tx_msdu_dscr *)(msg_pos->tran_data);
#endif
		dscr->color_bit = sprdwl_fc_set_clor_bit(tx_msg, i + 1);
/*TODO*/
		if (sprdwl_debug_level >= L_DBG) {
			int print_len = msg_pos->len;

			if (print_len > 200)
				print_len = 200;
			sprdwl_hex_dump("tx to cp2 data",
					(unsigned char *)(msg_pos->tran_data),
					print_len);
		}
#if defined(MORE_DEBUG)
		tx_bytes += msg_pos->skb->len;
#endif
		msg_ptr =
		(unsigned long *)(data_ptr - sizeof(unsigned long *));
		/*store msg_buf ptr to skb header room
		*for call back func free
		*/
		*msg_ptr = (unsigned long)msg_pos;
		if (dev->priv->hw_type == SPRDWL_HW_PCIE) {
			if (pcie_count > 1 && num > 0 && i >= j) {
				if (--num == 0) {
					if (cnt > 0)
						addr_buffer =
						sprdwl_set_pcie_addr_to_mbuf(
						tx_msg, mbuf_pos, cnt);
				} else {
					/*if data num greater than PCIE_TX_NUM,
					*alloc another pcie addr buf
					*/
					j += PCIE_TX_NUM;
						addr_buffer =
						sprdwl_set_pcie_addr_to_mbuf(
						tx_msg, mbuf_pos, PCIE_TX_NUM);
				}
				if (addr_buffer == NULL) {
					wl_err("%s:%d alloc pcie addr buf fail\n",
						   __func__, __LINE__);
					return -1;
				}
			}

			pcie_addr =
				mm_virt_to_phys(&dev->pdev->dev,
						msg_pos->tran_data,
						msg_pos->len, DMA_TO_DEVICE);
			memcpy(&addr_buffer->pcie_addr[i],
				   &pcie_addr, SPRDWL_PHYS_LEN);
		} else {
			mbuf_pos->buf = data_ptr;
			mbuf_pos->len = msg_pos->len;
			/*TODO, to check msgbuf before list push*/
			msg_temp = GET_MSG_BUF(mbuf_pos);
			if (!virt_addr_valid(msg_temp) ||
				!virt_addr_valid(msg_temp->skb))
				BUG_ON(1);
		}
		mbuf_pos = mbuf_pos->next;
		if (++i == tx_count)
			break;
	}

	tx_list_tail = pos;
	sprdwl_list_cut_position(tx_list_head, tx_list, tx_list_tail,
				 ac_index);
	sprdwl_add_to_free_list(tx_msg, tx_list_head, tx_count);

	if (dev->priv->hw_type == SPRDWL_HW_PCIE) {
		/*ret = sprdwcn_bus_push_list(dev->tx_data_port, head, tail,*/
		/*pcie_count);*/
		/*edma sync function*/
		ret = sprdwcn_bus_push_link_wait_complete(dev->tx_data_port, head,
						   tail, pcie_count, 50000);
		if (ret != 0) {
			for (mbuf_pos = head; mbuf_pos != NULL;
				mbuf_pos = mbuf_pos->next) {
				kfree(mbuf_pos->buf);
				mbuf_pos->buf = NULL;
				if (--pcie_count == 0)
					break;
			}
			sprdwcn_bus_list_free(dev->tx_data_port, head, tail,
					   pcie_count);
			/*add data list to tx list header if tx fail*/
			sprdwl_add_tx_list_head(tx_list_head,
						tx_list, ac_index, tx_count);
			wl_err("%s:%d Tx pcie addr buf fail\n",
				   __func__, __LINE__);
		} else {
#if defined(MORE_DEBUG)
			UPDATE_TX_PACKETS(dev, tx_count, tx_bytes);
#endif
			INIT_LIST_HEAD(tx_list_head);
		}
		return ret;
	}

	/*BT is on: call sdiohal_txthread to send data.*/
	if (dev->priv->hw_type == SPRDWL_HW_SDIO) {
		if (coex_bt_on)
			ret = sprdwcn_bus_push_list(dev->tx_data_port,
							head, tail, tx_count);
		else
			ret = sprdwcn_bus_push_list_direct(dev->tx_data_port,
							   head, tail, tx_count);
	} else {
		ret = sprdwcn_bus_push_list(dev->tx_data_port,
						head, tail, tx_count);
	}

	if (ret != 0) {
		sprdwcn_bus_list_free(dev->tx_data_port, head, tail, tx_count);
		sprdwl_add_tx_list_head(tx_list_head, tx_list,
					ac_index, tx_count);
		wl_err("%s:%d err Tx data fail\n", __func__, __LINE__);
		mbufalloc -= tx_count;
	} else {
#if defined(MORE_DEBUG)
		UPDATE_TX_PACKETS(dev, tx_count, tx_bytes);
#endif
		INIT_LIST_HEAD(tx_list_head);
		tx_packets += tx_count;
		wl_info("%s, %d, tx_count=%d, total=%lu, mbufalloc=%lu, mbufpop=%lu\n",
			__func__, __LINE__, tx_count, tx_packets,
			mbufalloc, mbufpop);/*TODO*/
		if (dev->priv->hw_type == SPRDWL_HW_SDIO) {
			if (!coex_bt_on)
				sprdwl_add_topop_list(dev->tx_data_port,
							  head, tail, tx_count);
		}
	}

	return ret;
}

struct sprdwl_peer_entry
*sprdwl_find_peer_entry_using_addr(struct sprdwl_vif *vif, u8 *addr)
{
	struct sprdwl_intf *intf;
	struct sprdwl_peer_entry *peer_entry = NULL;
	u8 i;

	intf = (struct sprdwl_intf *)vif->priv->hw_priv;
	for (i = 0; i < MAX_LUT_NUM; i++) {
		if (ether_addr_equal(intf->peer_entry[i].tx.da, addr)) {
			peer_entry = &intf->peer_entry[i];
			break;
		}
	}
	if (!peer_entry)
		wl_err("not find peer_entry at :%s\n", __func__);

	return peer_entry;
}

/* It is tx private function, just use in sprdwl_intf_fill_msdu_dscr()  */
unsigned char sprdwl_find_lut_index(struct sprdwl_intf *intf,
					struct sprdwl_vif *vif)
{
	u8 ret;
	unsigned char i;

	if (intf->skb_da == NULL)/*TODO*/
		goto out;

	wl_debug("%s,bssid: %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
		 intf->skb_da[0], intf->skb_da[1], intf->skb_da[2],
		 intf->skb_da[3], intf->skb_da[4], intf->skb_da[5]);
	if (sprdwl_is_group(intf->skb_da) &&
		(vif->mode == SPRDWL_MODE_AP || vif->mode == SPRDWL_MODE_P2P_GO)) {
		for (i = 0; i < MAX_LUT_NUM; i++) {
			if ((sprdwl_is_group(intf->peer_entry[i].tx.da)) &&
				(intf->peer_entry[i].ctx_id == vif->ctx_id)) {
				wl_info("%s, %d, group lut_index=%d\n",
					__func__, __LINE__,
					intf->peer_entry[i].lut_index);
				return intf->peer_entry[i].lut_index;
			}
		}
		if (vif->mode == SPRDWL_MODE_AP) {
			wl_info("%s,AP mode, group bssid,\n"
				"lut not found, ctx_id:%d, return lut:4\n",
				__func__, vif->ctx_id);
			return 4;
		}
		if (vif->mode == SPRDWL_MODE_P2P_GO) {
			wl_info("%s,GO mode, group bssid,\n"
				"lut not found, ctx_id:%d, return lut:5\n",
				__func__, vif->ctx_id);
			return 5;
		}
	}

	for (i = 0; i < MAX_LUT_NUM; i++) {
		if ((0 == memcmp(intf->peer_entry[i].tx.da,
				 intf->skb_da, ETH_ALEN)) &&
			(intf->peer_entry[i].ctx_id == vif->ctx_id)) {
			wl_debug("%s, %d, lut_index=%d\n",
				 __func__, __LINE__,
				 intf->peer_entry[i].lut_index);
			return intf->peer_entry[i].lut_index;
		}
	}

	for (i = 0; i < MAX_LUT_NUM; i++) {
		if ((vif->mode == SPRDWL_MODE_STATION ||
			 vif->mode == SPRDWL_MODE_P2P_CLIENT) &&
			(intf->peer_entry[i].ctx_id == vif->ctx_id)) {
			wl_debug("%s, %d, lut_index=%d\n",
				 __func__, __LINE__,
				 intf->peer_entry[i].lut_index);
			return intf->peer_entry[i].lut_index;
		}
	}

out:
	switch (vif->mode) {
	case SPRDWL_MODE_AP:
		ret = 4;
		break;

	case SPRDWL_MODE_P2P_GO:
		ret = 5;
		break;

	default:
		ret = 0;
		break;
	}

	wl_err("ctx_id = %d, sm_state = %d, bssid =%pM\n",
			vif->ctx_id, vif->sm_state, intf->skb_da);

	return ret;
}

int sprdwl_intf_fill_msdu_dscr(struct sprdwl_vif *vif,
				   struct sk_buff *skb,
				   u8 type,
				   u8 offset)
{
	u8 protocol;
	struct tx_msdu_dscr *dscr;
	struct sprdwl_intf *dev;
	u8 lut_index;
	struct sk_buff *temp_skb;
	unsigned char dscr_rsvd = 0;
	struct ethhdr *ethhdr = (struct ethhdr *)skb->data;
	u8 is_special_data = 0;
	bool is_vowifi2cmd = false;
#define DSCR_LEN	11
#define MSG_PTR_LEN 8

	if (ethhdr->h_proto == htons(ETH_P_ARP) ||
		ethhdr->h_proto == htons(ETH_P_TDLS) ||
		ethhdr->h_proto == htons(ETH_P_PREAUTH))
		is_special_data = 1;
	else if ((type == SPRDWL_TYPE_CMD) &&
		 is_vowifi_pkt(skb, &is_vowifi2cmd))
		is_special_data = 1;

	dev = (struct sprdwl_intf *)(vif->priv->hw_priv);
	dscr_rsvd = INTF_IS_PCIE ? MSDU_DSCR_RSVD : 0;
	if (skb_headroom(skb) < (DSCR_LEN + dev->hif_offset +
		MSG_PTR_LEN + dscr_rsvd)) {
		temp_skb = skb;

		skb = skb_realloc_headroom(skb, (DSCR_LEN + dev->hif_offset +
						 MSG_PTR_LEN + dscr_rsvd));
		kfree_skb(temp_skb);
		if (skb == NULL) {
			wl_err("%s:%d failed to unshare skbuff: NULL\n",
				   __func__, __LINE__);
			return -EPERM;
		}
#if defined(MORE_DEBUG)
		dev->stats.tx_realloc++;
#endif
	}

	dev->skb_da = skb->data;

	lut_index = sprdwl_find_lut_index(dev, vif);
	if ((lut_index < 6) && (!sprdwl_is_group(dev->skb_da))) {
		wl_err("%s, %d, sta disconn, no data tx!", __func__, __LINE__);
		return -EPERM;
	}
	skb_push(skb, sizeof(struct tx_msdu_dscr) + offset + dscr_rsvd);
	dscr = (struct tx_msdu_dscr *)(skb->data);
	memset(dscr, 0x00, sizeof(struct tx_msdu_dscr));
	dscr->common.type = (type == SPRDWL_TYPE_CMD ?
		SPRDWL_TYPE_CMD : SPRDWL_TYPE_DATA);
	dscr->common.direction_ind = 0;
	dscr->common.need_rsp = 0;/*TODO*/
	dscr->common.interface = vif->ctx_id;
	dscr->pkt_len = cpu_to_le16(skb->len - DSCR_LEN - dscr_rsvd);
	dscr->offset = DSCR_LEN;
/*TODO*/
	dscr->tx_ctrl.sw_rate = (is_special_data == 1 ? 1 : 0);
	dscr->tx_ctrl.wds = 0; /*TBD*/
	dscr->tx_ctrl.swq_flag = 0; /*TBD*/
	dscr->tx_ctrl.rsvd = 0; /*TBD*/
	dscr->tx_ctrl.next_buffer_type = 0;
	dscr->tx_ctrl.pcie_mh_readcomp = 0;
	dscr->buffer_info.msdu_tid = 0;
	dscr->buffer_info.mac_data_offset = 0;
	dscr->sta_lut_index = lut_index;

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		dscr->tx_ctrl.checksum_offload = 1;
		if (ethhdr->h_proto == htons(ETH_P_IPV6))
			protocol = ipv6_hdr(skb)->nexthdr;
		else
			protocol = ip_hdr(skb)->protocol;

		dscr->tx_ctrl.checksum_type =
			protocol == IPPROTO_TCP ? 1 : 0;
		dscr->tcp_udp_header_offset =
			skb->transport_header - skb->mac_header;
		wl_debug("%s: offload: offset: %d, protocol: %d\n",
			 __func__, dscr->tcp_udp_header_offset, protocol);
	}

	return 0;
}

#if defined FPGA_LOOPBACK_TEST
unsigned char tx_ipv4_udp[] = {
0x02, 0x04, 0x00, 0x01, 0x00, 0x06, 0x40, 0x45,
0xda, 0xf0, 0xff, 0x7e, 0x08, 0x00, 0x45, 0x00,
0x00, 0xe4, 0xc8, 0xc0, 0x40, 0x00, 0x40, 0x11,
0x8d, 0xb2, 0xc0, 0xa8, 0x31, 0x01, 0xc0, 0xa8,
0x31, 0x44, 0x67, 0x62, 0x3c, 0xbe, 0x00, 0xd0,
0xa0, 0x26, 0x80, 0x21, 0x81, 0x4b, 0x03, 0x68,
0x2b, 0x37, 0xde, 0xad, 0xbe, 0xef, 0x47, 0x10,
0x11, 0x35, 0xb1, 0x00, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00,
0x00, 0x00, 0xAA, 0xBB};

unsigned char tx_arp[] = {
0x02, 0x00, 0x00, 0x01, 0x00, 0x06, 0x74, 0x27,
0xea, 0xc8, 0x3e, 0x69, 0x08, 0x06, 0x00, 0x01,
0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x74, 0x27,
0xea, 0xc8, 0x3e, 0x69, 0xc0, 0xa8, 0x01, 0x14,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8,
0x01, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb};

int sprdwl_intf_tx_data_fpga_test(struct sprdwl_intf *intf,
				  unsigned char *data, int len)
{
	struct sprdwl_msg_buf *msg;
	struct sk_buff *skb;
	int ret;

	wl_debug("%s: #%d start\n", __func__,  intf->loopback_n);
	msg = sprdwl_get_msg_buf(intf, SPRDWL_TYPE_DATA,
				 SPRDWL_MODE_STATION, 1);
	if (!msg) {
		wl_err("%s:%d get msg buf failed\n", __func__, __LINE__);
		return -1;
	}
	if (data == NULL) {
		skb = dev_alloc_skb(244 + NET_IP_ALIGN);
		skb_reserve(skb, NET_IP_ALIGN);
		memcpy(skb->data, tx_ipv4_udp, 244);
		skb_put(skb, 244);
	} else {
		skb = dev_alloc_skb(len + NET_IP_ALIGN);
		skb_reserve(skb, NET_IP_ALIGN);
		memcpy(skb->data, data, len);
		skb_put(skb, len);
	}
	ret = sprdwl_send_data_fpga_test(intf->priv,
					 msg, skb, SPRDWL_TYPE_DATA, 0);
	wl_debug("%s:%d loopback_n#%d end ret=%d\n", __func__, __LINE__,
		 intf->loopback_n, ret);
	intf->loopback_n++;
	return ret;
}

int sprdwl_intf_fill_msdu_dscr_test(struct sprdwl_priv *priv,
					struct sk_buff *skb,
					u8 type,
					u8 offset)
{
	struct tx_msdu_dscr *dscr;
	struct sprdwl_intf *dev;
	u8 lut_index;
	struct sk_buff *temp_skb;
#define DSCR_LEN	11
#define MSG_PTR_LEN 8

	if (skb_headroom(skb) < (DSCR_LEN + intf->hif_offset + MSG_PTR_LEN)) {
		temp_skb = skb;

		skb = skb_realloc_headroom(skb, (DSCR_LEN + intf->hif_offset +
						 MSG_PTR_LEN));
		kfree_skb(temp_skb);
		if (skb == NULL) {
			wl_err("%s:%d failed to realloc skbuff: NULL\n",
				   __func__, __LINE__);
			return 0;
		}
	}
	dev = (struct sprdwl_intf *)(priv->hw_priv);
	dev->skb_da = skb->data;
	lut_index = sprdwl_find_index_using_addr(dev);
	skb_push(skb, sizeof(struct tx_msdu_dscr) + offset);
	dscr = (struct tx_msdu_dscr *)(skb->data);
	dscr->common.type = type;
	dscr->pkt_len = cpu_to_le16(skb->len - (DSCR_LEN));
	dscr->offset = DSCR_LEN;
	dscr->tx_ctrl.checksum_offload = 1;
	dscr->tx_ctrl.checksum_type =
		ip_hdr(skb)->protocol == IPPROTO_TCP ? 1 : 0;
	dscr->tx_ctrl.sw_rate = (type == SPRDWL_TYPE_DATA ? 0 : 1);
	dscr->tx_ctrl.wds = 0;
	dscr->sta_lut_index = lut_index;
	dscr->tcp_udp_header_offset = 34;
	return 1;
}
#endif

int sprdwl_rx_fill_mbuf(struct mbuf_t *head, struct mbuf_t *tail, int num, int len)
{
	struct sprdwl_intf *intf = get_intf();
	int ret = 0, count = 0;
	struct mbuf_t *pos = NULL;

	for (pos = head, count = 0; count < num; count++) {
		pos->len = ALIGN(len, SMP_CACHE_BYTES);
		pos->buf = netdev_alloc_frag(pos->len);
		pos->phy = mm_virt_to_phys(&intf->pdev->dev, pos->buf,
					   pos->len, DMA_FROM_DEVICE);
		if (unlikely(!pos->buf)) {
			ret = -ENOMEM;
			break;
		}
		pos = pos->next;
	}

	if (ret) {
		pos = head;
		while (count--) {
			sprdwl_free_data(pos->buf, SPRDWL_DEFRAG_MEM);
			pos = pos->next;
		}
	}

	return ret;
}

int sprdwl_rx_common_push(int chn, struct mbuf_t **head, struct mbuf_t **tail,
			  int *num, int len)
{
	int ret = 0;

	ret = sprdwcn_bus_list_alloc(chn, head, tail, num);
	if (ret || head == NULL) {
		wl_err("%s:%d sprdwcn_bus_list_alloc fail\n", __func__, __LINE__);
		ret = -ENOMEM;
	} else {
		ret = sprdwl_rx_fill_mbuf(*head, *tail, *num, len);
		if (ret) {
			wl_err("%s: alloc buf fail\n", __func__);
			sprdwcn_bus_list_free(chn, *head, *tail, *num);
			*head = NULL;
			*tail = NULL;
			*num = 0;
		}
	}

	return ret;
}

inline void *sprdwl_get_rx_data(struct sprdwl_intf *intf,
				void *pos, void **data,
				void **tran_data, int *len, int offset)
{
	struct mbuf_t *mbuf = (struct mbuf_t *)pos;

	if (intf->priv->hw_type == SPRDWL_HW_PCIE) {
		mm_phys_to_virt(&intf->pdev->dev, mbuf->phy, mbuf->len,
				DMA_FROM_DEVICE, false);
		mbuf->phy = 0;
	}

	*tran_data = mbuf->buf;
	*data = (*tran_data) + offset;
	*len = mbuf->len;
	mbuf->buf = NULL;

	return (void *)mbuf->next;
}

inline void sprdwl_free_rx_data(struct sprdwl_intf *intf,
				int chn, void *head, void *tail, int num)
{
	int len = 0, ret = 0;

	/* We should refill mbuf in pcie mode */
	if (intf->priv->hw_type == SPRDWL_HW_PCIE) {
		if (intf->rx_cmd_port == chn)
			len = SPRDWL_MAX_CMD_RXLEN;
		else
			len = SPRDWL_MAX_DATA_RXLEN;

		ret = sprdwl_rx_fill_mbuf(head, tail, num, len);
		if (ret) {
			wl_err("%s: alloc buf fail\n", __func__);
			sprdwcn_bus_list_free(chn, (struct mbuf_t *)head,
					   (struct mbuf_t *)tail, num);
			head = NULL;
			tail = NULL;
			num = 0;
		}
	}

	if (!ret)
		sprdwcn_bus_push_list(chn, (struct mbuf_t *)head, (struct mbuf_t *)tail, num);
}

void sprdwl_count_rx_tp(struct sprdwl_rx_if *rx_if, int num)
{
	long long timeus = 0;
	struct sprdwl_intf *intf = get_intf();

	rx_if->rx_data_num += num;
	if (rx_if->rx_data_num == num) {
		rx_if->rxtimebegin = ktime_get();
		return;
	}

	rx_if->rxtimeend = ktime_get();
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
	timeus = div_u64(rx_if->rxtimeend.tv64 - rx_if->rxtimebegin.tv64, NSEC_PER_USEC);
#else
	timeus = ktime_to_us(rx_if->rxtimeend - rx_if->rxtimebegin);
#endif
	if (div_u64((rx_if->rx_data_num * 1000), timeus) >= intf->rxnum_level &&
		rx_if->rx_data_num >= 1000) {
		rx_if->rx_data_num = 0;
		sprdwl_boost();
	} else if (timeus >= USEC_PER_SEC) {
		rx_if->rx_data_num = 0;
	}
}
static int intf_rx_handle(int chn, struct mbuf_t *head,
				   struct mbuf_t *tail, int num)
{
	struct sprdwl_intf *intf = get_intf();
	struct sprdwl_rx_if *rx_if = (struct sprdwl_rx_if *)intf->sprdwl_rx;
	struct sprdwl_msg_buf *msg = NULL;

	wl_debug("%s: channel:%d head:%p tail:%p num:%d\n",
		__func__, chn, head, tail, num);
	if ((intf->priv->hw_type == SPRDWL_HW_SDIO && chn == SDIO_RX_DATA_PORT) ||
		(intf->priv->hw_type == SPRDWL_HW_USB && chn == USB_RX_DATA_PORT))
		sprdwl_count_rx_tp(rx_if, num);

	/*To process credit earlier*/
	if (intf->priv->hw_type == SPRDWL_HW_SDIO ||
		intf->priv->hw_type == SPRDWL_HW_USB) {
		unsigned int i = 0;
		struct mbuf_t *mbuf = NULL;

		mbuf = head;
		for (i = num; i > 0; i--) {
			sprdwl_sdio_process_credit(intf,
				(void *)(mbuf->buf + intf->hif_offset));
			mbuf = mbuf->next;
		}
	}

	/* FIXME: Should we use replace msg? */
	msg = sprdwl_alloc_msg_buf(&rx_if->rx_list);
	if (!msg) {
		wl_err("%s: no msgbuf\n", __func__);
		sprdwcn_bus_push_list(chn, head, tail, num);
		return 0;
	}

	sprdwl_fill_msg(msg, NULL, (void *)head, num);
	msg->fifo_id = chn;
	msg->buffer_type = SPRDWL_DEFRAG_MEM;
	msg->data = (void *)tail;

	sprdwl_queue_msg_buf(msg, &rx_if->rx_list);
	queue_work(rx_if->rx_queue, &rx_if->rx_work);

	return 0;
}

#ifdef RX_NAPI
static int intf_napi_rx_handle(int chn, struct mbuf_t *head,
					struct mbuf_t *tail, int num)
{
	struct sprdwl_intf *intf = get_intf();
	struct sprdwl_rx_if *rx_if = (struct sprdwl_rx_if *)intf->sprdwl_rx;
	struct sprdwl_msg_buf *msg = NULL;

	wl_info("%s: channel:%d head:%p tail:%p num:%d\n",
		__func__, chn, head, tail, num);

	/* FIXME: Should we use replace msg? */
	msg = sprdwl_alloc_msg_buf(&rx_if->rx_data_list);
	if (!msg) {
		wl_err("%s: no msgbuf\n", __func__);
		sprdwcn_bus_push_list(chn, head, tail, num);
		return 0;
	}

	sprdwl_fill_msg(msg, NULL, (void *)head, num);
	msg->fifo_id = chn;
	msg->buffer_type = SPRDWL_DEFRAG_MEM;
	msg->data = (void *)tail;

	sprdwl_queue_msg_buf(msg, &rx_if->rx_data_list);
	napi_schedule(&rx_if->napi_rx);

	return 0;
}
#endif

void sprdwl_handle_pop_list(void *data)
{
	int i;
	struct sprdwl_msg_buf *msg_pos;
	struct mbuf_t *mbuf_pos = NULL;
	struct sprdwl_pop_work *pop = (struct sprdwl_pop_work *)data;
	struct sprdwl_tx_msg *tx_msg;
	struct sprdwl_intf *intf = get_intf();
	struct list_head tmp_list;
	struct sprdwl_msg_buf *msg_head, *msg_tail;

	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	mbuf_pos = (struct mbuf_t *)pop->head;
	msg_pos = GET_MSG_BUF(mbuf_pos);

	msg_head = GET_MSG_BUF((struct mbuf_t *)pop->head);
	msg_tail = GET_MSG_BUF((struct mbuf_t *)pop->tail);

	spin_lock_bh(&tx_msg->xmit_msg_list.free_lock);
	list_cut_position(&tmp_list, msg_head->list.prev, &msg_tail->list);
	spin_unlock_bh(&tx_msg->xmit_msg_list.free_lock);

	for (i = 0; i < pop->num; i++) {
		msg_pos = GET_MSG_BUF(mbuf_pos);
		dev_kfree_skb(msg_pos->skb);
		mbuf_pos = mbuf_pos->next;
	}

	spin_lock_bh(&tx_msg->tx_list_qos_pool.freelock);
	list_splice_tail(&tmp_list, &msg_pos->msglist->freelist);
	spin_unlock_bh(&tx_msg->tx_list_qos_pool.freelock);
	sprdwcn_bus_list_free(pop->chn,
				  pop->head,
				  pop->tail,
				  pop->num);
	mbufpop += pop->num;
}

/*call back func for HIF pop_link*/
int sprdwl_tx_data_pop_list(int channel, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	struct mbuf_t *mbuf_pos = NULL;
#if defined(MORE_DEBUG)
	struct sprdwl_msg_buf *msg_head;
#endif
	struct sprdwl_intf *intf = get_intf();

	wl_debug("%s channel: %d, head: %p, tail: %p num: %d\n",
		__func__, channel, head, tail, num);

	if (intf->priv->hw_type == SPRDWL_HW_PCIE) {
		/* FIXME: Temp solution, addr node pos hard to sync dma */
		for (mbuf_pos = head; mbuf_pos != NULL;
		mbuf_pos = mbuf_pos->next) {
			mm_phys_to_virt(&intf->pdev->dev, mbuf_pos->phy,
					mbuf_pos->len, DMA_TO_DEVICE, false);
			mbuf_pos->phy = 0;
			kfree(mbuf_pos->buf);
			mbuf_pos->buf = NULL;
			if (--num == 0)
				break;
		}
		sprdwcn_bus_list_free(channel, head, tail, num);
		wl_info("%s:%d free : %d msg buf\n", __func__, __LINE__, num);
		return 0;
	}
#if defined(MORE_DEBUG)
	msg_head = GET_MSG_BUF(head);
	/*show packet average sent time, unit: ns*/
	sprdwl_get_tx_avg_time(intf, msg_head->tx_start_time);
#endif

	sprdwl_add_topop_list(channel, head, tail, num);
	wl_debug("%s:%d free : %d msg buf\n", __func__, __LINE__, num);

	return 0;
}

/*free PCIe data when receive txc event from cp*/
int sprdwl_tx_free_pcie_data(struct sprdwl_intf *dev, unsigned char *data,
				 unsigned short len)
{
	int i;
	struct sprdwl_tx_msg *tx_msg;
	unsigned char *data_addr_ptr;
	uint64_t pcie_addr;
	unsigned long timeout;
	unsigned short  data_num;
	struct list_head *free_list;
	struct txc_addr_buff *txc_addr;
	unsigned char (*pos)[5];
	struct sprdwl_msg_buf *msg_buf, *pos_buf, *temp_buf;
#if defined(MORE_DEBUG)
	unsigned long tx_start_time = 0;
#endif

	txc_addr = (struct txc_addr_buff *)data;
	data_num = txc_addr->number;
	pos = (unsigned char (*)[5])(txc_addr + 1);
	for (i = 0; i < data_num; i++, pos++) {
		memcpy(&pcie_addr, pos, SPRDWL_PHYS_LEN);
		data_addr_ptr = (unsigned char *)
		mm_phys_to_virt(&dev->pdev->dev, pcie_addr,
				SPRDWL_MAX_DATA_TXLEN, DMA_TO_DEVICE, true);
		msg_buf = (struct sprdwl_msg_buf *)
		(data_addr_ptr - (sizeof(unsigned long *) + dev->hif_offset));
#if defined(MORE_DEBUG)
		if (i == 0)
			tx_start_time = msg_buf->tx_start_time;
#endif
		dev_kfree_skb(msg_buf->skb);
		sprdwl_dequeue_data_buf(msg_buf);
	}
#if defined(MORE_DEBUG)
	sprdwl_get_tx_avg_time(dev, tx_start_time);
#endif
	tx_msg = (struct sprdwl_tx_msg *)dev->sprdwl_tx;
	free_list = &tx_msg->xmit_msg_list.to_free_list;
	/*if cp fail to sent txc event, data will be freed timeout*/
	if (!list_empty(free_list)) {
		timeout = msecs_to_jiffies(SPRDWL_TX_DATA_TIMEOUT);
		list_for_each_entry_safe(pos_buf,
					 temp_buf, free_list, list) {
			if (time_after(jiffies, pos_buf->timeout + timeout)) {
				dev_kfree_skb(pos_buf->skb);
				sprdwl_dequeue_data_buf(pos_buf);
			} else {
				return 0;
			}
		}
	}
	return 0;
}

int sprdwl_tx_cmd_pop_list(int channel, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	int count = 0;
	struct mbuf_t *pos = NULL;
	struct sprdwl_intf *intf = get_intf();
	struct sprdwl_tx_msg *tx_msg;
	struct sprdwl_msg_buf *pos_buf, *temp_buf;

	wl_debug("%s channel: %d, head: %p, tail: %p num: %d\n",
		 __func__, channel, head, tail, num);

	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	wl_debug("%s len: %d buf: %s\n", __func__, head->len, head->buf + 4);

	pos = head;

	list_for_each_entry_safe(pos_buf, temp_buf,
				 &tx_msg->tx_list_cmd.cmd_to_free, list) {
		if (pos_buf->tran_data == pos->buf) {
			wl_debug("move CMD node from to_free to free list\n");
			/*list msg_buf from to_free list  to free list*/
			sprdwl_free_cmd_buf(pos_buf, &tx_msg->tx_list_cmd);

			if (intf->priv->hw_type == SPRDWL_HW_PCIE) {
				mm_phys_to_virt(&intf->pdev->dev, pos->phy,
						pos->len, DMA_TO_DEVICE, false);
				pos->phy = 0;
			}
			/*free it*/
			kfree(pos->buf);
			pos->buf = NULL;
			pos = pos->next;
			count++;
		}
		if (count == num)
			break;
	}

	tx_msg->cmd_poped += num;
	mbufpop += num;
	wl_info("tx_cmd_pop add num: %d=cmd_poped%d, cmd_send%d\n",
		num, tx_msg->cmd_poped, tx_msg->cmd_send);
	sprdwcn_bus_list_free(channel, head, tail, num);

	return 0;
}

int sprdwl_rx_cmd_push(int chn, struct mbuf_t **head, struct mbuf_t **tail, int *num)
{
	return sprdwl_rx_common_push(chn, head, tail,
					 num, SPRDWL_MAX_CMD_RXLEN);
}

int sprdwl_rx_data_push(int chn, struct mbuf_t **head, struct mbuf_t **tail, int *num)
{
	return sprdwl_rx_common_push(chn, head, tail,
					 num, SPRDWL_MAX_DATA_RXLEN);
}

/*
 * mode:
 * 0 - suspend
 * 1 - resume
 */
int sprdwl_suspend_resume_handle(int chn, int mode)
{
	struct sprdwl_intf *intf = get_intf();
	struct sprdwl_priv *priv = intf->priv;
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	int ret;
	struct sprdwl_vif *vif;
	struct timespec time;
	enum sprdwl_mode sprdwl_mode = SPRDWL_MODE_STATION;
	u8 mode_found = 0;

	for (sprdwl_mode = SPRDWL_MODE_STATION; sprdwl_mode < SPRDWL_MODE_MAX; sprdwl_mode++) {
		if (priv->fw_stat[sprdwl_mode] == SPRDWL_INTF_OPEN) {
			mode_found = 1;
			break;
		}
	}

	if (0 == mode_found) {
		wl_err("%s suspend failed, mode not found\n", __func__);
		return -EBUSY;
	}

	vif = mode_to_vif(priv, sprdwl_mode);
	if (vif == NULL || intf->cp_asserted) {
		wl_err("%s, %d, error! NULL vif or assert\n", __func__, __LINE__);
		sprdwl_put_vif(vif);
		return -EBUSY;
	}

	if (mode == 0) {
		if (atomic_read(&tx_msg->tx_list_qos_pool.ref) > 0 ||
			atomic_read(&tx_msg->tx_list_cmd.ref) > 0 ||
			!list_empty(&tx_msg->xmit_msg_list.to_send_list) ||
			!list_empty(&tx_msg->xmit_msg_list.to_free_list)) {
			wl_info("%s, %d,Q not empty suspend not allowed\n",
				__func__, __LINE__);
			sprdwl_put_vif(vif);
			return -EBUSY;
		}
		priv->wakeup_tracer.resume_flag = 0;
		intf->suspend_mode = SPRDWL_PS_SUSPENDING;
		getnstimeofday(&time);
		intf->sleep_time = timespec_to_ns(&time);
		priv->is_suspending = 1;
		ret = sprdwl_power_save(priv,
					vif->ctx_id,
					SPRDWL_SUSPEND_RESUME,
					0);
		if (ret == 0) {
			intf->suspend_mode = SPRDWL_PS_SUSPENDED;
#ifdef UNISOC_WIFI_PS
			sprdwcn_bus_allow_sleep(WIFI);
			wl_info("sprdwcn bus allow sleep\n");
#endif
		}
		else
			intf->suspend_mode = SPRDWL_PS_RESUMED;
		sprdwl_put_vif(vif);
		return ret;
	} else if (mode == 1) {
#ifdef UNISOC_WIFI_PS
		sprdwcn_bus_sleep_wakeup(WIFI);
		wl_info("sprdwcn bus wake up\n");
#endif
		intf->suspend_mode = SPRDWL_PS_RESUMING;
		priv->wakeup_tracer.resume_flag = 1;
#ifdef UNISOC_WIFI_PS
		complete(&intf->suspend_completed);
#endif
		getnstimeofday(&time);
		intf->sleep_time = timespec_to_ns(&time) - intf->sleep_time;
		ret = sprdwl_power_save(priv,
					vif->ctx_id,
					SPRDWL_SUSPEND_RESUME,
					1);
		wl_info("%s, %d,resume ret=%d, resume after %lu ms\n",
			__func__, __LINE__,
			ret, intf->sleep_time/1000000);
		sprdwl_put_vif(vif);
		return ret;
	}
	sprdwl_put_vif(vif);
	return -EBUSY;
}

/*  SDIO TX:
 *  Type 3:WIFI
 *  Subtype 0  --> port 8
 *  Subtype 1  --> port 9
 *  Subtype 2  --> port 10(fifolen=8)
 *  Subtype 3  --> port 11(fifolen=8)
 *  SDIO RX:
 *  Type 3:WIFI
 *  Subtype 0  --> port 10
 *  Subtype 1  --> port 11
 *  Subtype 2  --> port 12(fifolen=8)
 *  Subtype 3  --> port 13(fifolen=8)
 */
struct mchn_ops_t sdio_hif_ops[] = {
	/* RX INTF */
	/* NOTE: Requested by SDIO team, pool_size MUST be 1 in RX */
	INIT_INTF(SDIO_RX_CMD_PORT, 0, 0, 0,
		  SPRDWL_MAX_CMD_RXLEN, 1, 0, 0, 0,
		  intf_rx_handle, NULL, NULL, NULL),
	INIT_INTF(SDIO_RX_PKT_LOG_PORT, 0, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 1, 0, 0, 0,
		  intf_rx_handle, NULL, NULL, NULL),
#ifdef RX_NAPI
	INIT_INTF(SDIO_RX_DATA_PORT, 0, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 1, 0, 0, 0,
		  intf_napi_rx_handle, NULL, NULL, NULL),
#else
	INIT_INTF(SDIO_RX_DATA_PORT, 0, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 1, 0, 0, 0,
		  intf_rx_handle, NULL, NULL, NULL),
#endif

	/* TX INTF */
	INIT_INTF(SDIO_TX_CMD_PORT, 0, 1, 0,
		  SPRDWL_MAX_CMD_TXLEN, 10, 0, 0, 0,
		  sprdwl_tx_cmd_pop_list, NULL, NULL,
		  sprdwl_suspend_resume_handle),
	INIT_INTF(SDIO_TX_DATA_PORT, 0, 1, 0,
		  SPRDWL_MAX_DATA_TXLEN, 800, 0, 0, 0,
		  sprdwl_tx_data_pop_list, NULL, NULL, NULL),
};

struct mchn_ops_t pcie_hif_ops[] = {
	/* RX INTF */
	INIT_INTF(PCIE_RX_CMD_PORT, 1, 0, 0,
		  SPRDWL_MAX_CMD_RXLEN, 1, 0, 0, 0,
		  intf_rx_handle,
		  sprdwl_rx_cmd_push, NULL, NULL),
	INIT_INTF(PCIE_RX_DATA_PORT, 1, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 1, 0, 0, 0,
		  intf_rx_handle,
		  sprdwl_rx_data_push, NULL, NULL),
	/* TX INTF */
	INIT_INTF(SDIO_TX_CMD_PORT, 1, 1, 0,
		  SPRDWL_MAX_CMD_TXLEN, 10, 0, 0, 0,
		  sprdwl_tx_cmd_pop_list, NULL, NULL, NULL),
	INIT_INTF(SDIO_TX_DATA_PORT, 1, 1, 0,
		  SPRDWL_MAX_DATA_TXLEN, 300, 0, 0, 0,
		  sprdwl_tx_data_pop_list, NULL, NULL, NULL)
};

struct mchn_ops_t usb_hif_ops[] = {
	/* RX INTF */
	INIT_INTF(USB_RX_CMD_PORT, 3, 0, 0,
		  SPRDWL_MAX_CMD_RXLEN, 10, 0, 0, 0,
		  intf_rx_handle, NULL, NULL, NULL),
	INIT_INTF(USB_RX_PKT_LOG_PORT, 3, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 50, 0, 0, 0,
		  intf_rx_handle, NULL, NULL, NULL),
#ifndef RX_NAPI
	INIT_INTF(USB_RX_DATA_PORT, 3, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 1000, 0, 0, 0,
		  intf_rx_handle, NULL, NULL, NULL),
#else
	INIT_INTF(USB_RX_DATA_PORT, 3, 0, 0,
		  SPRDWL_MAX_DATA_RXLEN, 300, 0, 0, 0,
		  intf_napi_rx_handle, NULL, NULL, NULL),
#endif

	/* TX INTF */
	INIT_INTF(USB_TX_CMD_PORT, 3, 1, 0,
		  SPRDWL_MAX_CMD_TXLEN, 20, 0, 0, 0,
		  sprdwl_tx_cmd_pop_list, NULL, NULL,
		  sprdwl_suspend_resume_handle),
	INIT_INTF(USB_TX_DATA_PORT, 3, 1, 0,
		  SPRDWL_MAX_DATA_TXLEN, 300, 0, 0, 0,
		  sprdwl_tx_data_pop_list, NULL, NULL, NULL),
};

struct sprdwl_peer_entry
*sprdwl_find_peer_entry_using_lut_index(struct sprdwl_intf *intf,
					unsigned char sta_lut_index)
{
	int i = 0;
	struct sprdwl_peer_entry *peer_entry = NULL;

	for (i = 0; i < MAX_LUT_NUM; i++) {
		if (sta_lut_index == intf->peer_entry[i].lut_index) {
			peer_entry = &intf->peer_entry[i];
			break;
		}
	}

	return peer_entry;
}

/* update lut-inidex if event_sta_lut received
 * at CP side, lut_index range 0-31
 * but 0-3 were used to send non-assoc frame(only used by CP)
 * so for Ap-CP interface, there is only 4-31
*/
void sprdwl_event_sta_lut(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_intf *intf;
	struct sprdwl_sta_lut_ind *sta_lut = NULL;
	u8 i;

	if (len < sizeof(*sta_lut)) {
		wl_err("%s, len:%d too short!\n", __func__, len);
		return;
	}
	intf = (struct sprdwl_intf *)vif->priv->hw_priv;
	sta_lut = (struct sprdwl_sta_lut_ind *)data;
	if (intf != get_intf()) {
		wl_err("%s, wrong intf!\n", __func__);
		return;
	}
	if (sta_lut == NULL) {
		wl_err("%s, NULL input data!\n", __func__);
		return;
	}

	i = sta_lut->sta_lut_index;

	wl_debug("ctx_id:%d,action:%d,lut:%d\n", sta_lut->ctx_id,
		sta_lut->action, sta_lut->sta_lut_index);
	switch (sta_lut->action) {
	case DEL_LUT_INDEX:
		if (intf->peer_entry[i].ba_tx_done_map != 0) {
			intf->peer_entry[i].ht_enable = 0;
			intf->peer_entry[i].ip_acquired = 0;
			intf->peer_entry[i].ba_tx_done_map = 0;
			/*sprdwl_tx_delba(intf, intf->peer_entry + i);*/
		}
		peer_entry_delba((void *)intf, i);
		memset(&intf->peer_entry[i], 0x00,
			   sizeof(struct sprdwl_peer_entry));
		intf->peer_entry[i].ctx_id = 0xFF;
		intf->tx_num[i] = 0;
		sprdwl_dis_flush_txlist(intf, i);
		break;
	case UPD_LUT_INDEX:
		peer_entry_delba((void *)intf, i);
		sprdwl_dis_flush_txlist(intf, i);
	case ADD_LUT_INDEX:
		intf->peer_entry[i].lut_index = i;
		intf->peer_entry[i].ctx_id = sta_lut->ctx_id;
		intf->peer_entry[i].ht_enable = sta_lut->is_ht_enable;
		intf->peer_entry[i].vht_enable = sta_lut->is_vht_enable;
		intf->peer_entry[i].ba_tx_done_map = 0;
		intf->tx_num[i] = 0;

		wl_debug("ctx_id%d,action%d,lut%d,%x:%x:%x:%x:%x:%x\n",
			sta_lut->ctx_id, sta_lut->action,
			sta_lut->sta_lut_index,
			sta_lut->ra[0], sta_lut->ra[1], sta_lut->ra[2],
			sta_lut->ra[3], sta_lut->ra[4], sta_lut->ra[5]);
		ether_addr_copy(intf->peer_entry[i].tx.da, sta_lut->ra);
		break;
	default:
		break;
	}
}

void sprdwl_tx_ba_mgmt(struct sprdwl_priv *priv, void *data, int len,
			   unsigned char cmd_id, unsigned char ctx_id)
{
	struct sprdwl_msg_buf *msg;
	unsigned char *data_ptr;
	u8 *rbuf;
	u16 rlen = (1 + sizeof(struct host_addba_param));

	msg = sprdwl_cmd_getbuf(priv, len, ctx_id, SPRDWL_HEAD_RSP,
				cmd_id);
	if (!msg) {
		wl_err("%s, %d, get msg err\n", __func__, __LINE__);
		return;
	}
	rbuf = kzalloc(rlen, GFP_KERNEL);
	if (!rbuf) {
		wl_err("%s, %d, alloc rbuf err\n", __func__, __LINE__);
		return;
	}
	memcpy(msg->data, data, len);
	data_ptr = (unsigned char *)data;

	if (sprdwl_debug_level >= L_DBG)
		sprdwl_hex_dump("sprdwl_tx_ba_mgmt", data_ptr, len);

	if (sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, rbuf, &rlen))
		goto out;
	/*if tx ba req failed, need to clear txba map*/
	if (cmd_id == WIFI_CMD_ADDBA_REQ &&
		rbuf[0] != ADDBA_REQ_RESULT_SUCCESS) {
		struct host_addba_param *addba;
		struct sprdwl_peer_entry *peer_entry = NULL;
		struct sprdwl_intf *intf = get_intf();
		u16 tid = 0;

		addba = (struct host_addba_param *)(rbuf + 1);
		peer_entry = &intf->peer_entry[addba->lut_index];
		tid = addba->addba_param.tid;
		if (!test_and_clear_bit(tid, &peer_entry->ba_tx_done_map))
			goto out;
		wl_err("%s, %d, tx_addba failed, reason=%d, lut_index=%d, tid=%d, map=%lu\n",
			   __func__, __LINE__,
			   rbuf[0],
			   addba->lut_index,
			   tid,
			   peer_entry->ba_tx_done_map);
	}
out:
	kfree(rbuf);
}

void sprdwl_tx_send_addba(struct sprdwl_vif *vif, void *data, int len)
{
	sprdwl_tx_ba_mgmt(vif->priv, data, len, WIFI_CMD_ADDBA_REQ,
			  vif->ctx_id);
}

void sprdwl_tx_send_delba(struct sprdwl_vif *vif, void *data, int len)
{
	u8 i;
	struct host_delba_param *delba;

	delba = (struct host_delba_param *)data;
	for (i = 0; i < SPRDWL_AC_MAX; i++)
			sprdwl_tx_ba_mgmt(vif->priv, delba + i,
					  sizeof(struct host_delba_param),
					  WIFI_CMD_DELBA_REQ,
					  vif->ctx_id);
}

void sprdwl_tx_addba(struct sprdwl_intf *intf,
			 struct sprdwl_peer_entry *peer_entry, unsigned char tid)
{
#define WIN_SIZE 64
	struct host_addba_param addba;
	struct sprdwl_work *misc_work;
	struct sprdwl_vif *vif;

	vif = ctx_id_to_vif(intf->priv, peer_entry->ctx_id);
	if (!vif)
		return;
	memset(&addba, 0x0, sizeof(struct host_addba_param));

	addba.lut_index = peer_entry->lut_index;
	ether_addr_copy(addba.perr_mac_addr, peer_entry->tx.da);
	wl_debug("%s, lut_index is %d\n", __func__, peer_entry->lut_index);
	addba.dialog_token = 1;
	addba.addba_param.amsdu_permit = 0;
	addba.addba_param.ba_policy = DOT11_ADDBA_POLICY_IMMEDIATE;
	addba.addba_param.tid = tid;
	addba.addba_param.buffer_size = WIN_SIZE;
	misc_work = sprdwl_alloc_work(sizeof(struct host_addba_param));
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		sprdwl_put_vif(vif);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_ADDBA;
	memcpy(misc_work->data, &addba, sizeof(struct host_addba_param));

	sprdwl_queue_work(vif->priv, misc_work);
	sprdwl_put_vif(vif);
}

void sprdwl_tx_delba(struct sprdwl_intf *intf,
			 struct sprdwl_peer_entry *peer_entry,  unsigned int ac_index)
{
	struct host_delba_param delba[SPRDWL_AC_MAX];
	struct sprdwl_work *misc_work;
	struct sprdwl_vif *vif;

	vif = ctx_id_to_vif(intf->priv, peer_entry->ctx_id);
	if (!vif)
		return;
	memset(delba, 0x0, sizeof(delba));

	wl_info("enter--at %s\n", __func__);
	ether_addr_copy(delba[ac_index].perr_mac_addr, peer_entry->tx.da);
	delba[ac_index].lut_index = peer_entry->lut_index;
	delba[ac_index].delba_param.initiator = 1;
	delba[ac_index].delba_param.tid = qos_index_2_tid(ac_index);
	delba[ac_index].reason_code = 0;


	misc_work =
	sprdwl_alloc_work(sizeof(struct host_delba_param) * SPRDWL_AC_MAX);
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		sprdwl_put_vif(vif);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_DELBA;
	memcpy(misc_work->data, delba,
		   sizeof(struct host_delba_param) * SPRDWL_AC_MAX);
	peer_entry->ht_enable = 0;
	peer_entry->ba_tx_done_map = 0;

	sprdwl_queue_work(vif->priv, misc_work);
	sprdwl_put_vif(vif);
}

int sprdwl_notifier_boost(struct notifier_block *nb, unsigned long event, void *data)
{
	struct cpufreq_policy_data *policy = data;
	unsigned long min_freq;
	unsigned long max_freq = policy->cpuinfo.max_freq;
	struct sprdwl_intf *intf = get_intf();
	u8 boost;
	if (NULL == intf)
		return NOTIFY_DONE;

	boost = intf->boost;

	if (event != CPUFREQ_ADJUST)
		return NOTIFY_DONE;

	min_freq = boost ? 1200000 : 400000;
	cpufreq_verify_within_limits(policy, min_freq, max_freq);

	return NOTIFY_OK;
}

void sprdwl_boost(void)
{
	struct sprdwl_intf *intf = get_intf();

	if (intf->boost == 0) {
		intf->boost = 1;
		cpufreq_update_policy(0);
	}
}

void sprdwl_unboost(void)
{
	struct sprdwl_intf *intf = get_intf();

	if (intf->boost == 1) {
		intf->boost = 0;
		cpufreq_update_policy(0);
	}
}

void adjust_txnum_level(char *buf, unsigned char offset)
{
#define MAX_LEN 4
	unsigned int cnt = 0;
	unsigned int i = 0;
	struct sprdwl_intf *intf = get_intf();

	for (i = 0; i < MAX_LEN; (cnt *= 10), i++) {
		if ((buf[offset + i] >= '0') &&
		   (buf[offset + i] <= '9')) {
			cnt += (buf[offset + i] - '0');
		} else {
			cnt /= 10;
			break;
		}
	}

	if (cnt < 0 || cnt > 9999)
		cnt = BOOST_TXNUM_LEVEL;
	intf->txnum_level = cnt;
	wl_info("credit_level: %d\n", intf->txnum_level);
#undef MAX_LEN
}

void adjust_rxnum_level(char *buf, unsigned char offset)
{
#define MAX_LEN 2
	unsigned int cnt = 0;
	unsigned int i = 0;
	struct sprdwl_intf *intf = get_intf();

	for (i = 0; i < MAX_LEN; (cnt *= 10), i++) {
		if ((buf[offset + i] >= '0') &&
		   (buf[offset + i] <= '9')) {
			cnt += (buf[offset + i] - '0');
		} else {
			cnt /= 10;
			break;
		}
	}

	if (cnt < 0 || cnt > 99)
		cnt = BOOST_RXNUM_LEVEL;
	intf->rxnum_level = cnt;
	wl_info("rxnum_level: %d\n", intf->rxnum_level);
#undef MAX_LEN
}

int sprdwl_intf_init(struct sprdwl_priv *priv, struct sprdwl_intf *intf)
{
	int ret = -EINVAL, chn = 0;

	if (priv->hw_type == SPRDWL_HW_SDIO) {
		g_intf_ops.hif_ops = sdio_hif_ops;
		g_intf_ops.max_num =
			sizeof(sdio_hif_ops)/sizeof(struct mchn_ops_t);
	} else if (priv->hw_type == SPRDWL_HW_PCIE) {
		g_intf_ops.hif_ops = pcie_hif_ops;
		g_intf_ops.max_num =
			sizeof(pcie_hif_ops)/sizeof(struct mchn_ops_t);
	} else if (priv->hw_type == SPRDWL_HW_USB) {
		g_intf_ops.hif_ops = usb_hif_ops;
		g_intf_ops.max_num =
			sizeof(usb_hif_ops)/sizeof(struct mchn_ops_t);
	}

	if (g_intf_ops.max_num < MAX_CHN_NUM) {
		wl_info("%s: register %d ops\n",
			__func__, g_intf_ops.max_num);

		for (chn = 0; chn < g_intf_ops.max_num; chn++) {
			ret = sprdwcn_bus_chn_init(&g_intf_ops.hif_ops[chn]);
			if (ret < 0)
				goto err;
		}

		g_intf_ops.intf = (void *)intf;
		/* TODO: Need we reserve g_intf_ops? */
		intf->hw_intf = (void *)&g_intf_ops;

	priv->hw_priv = intf;
	priv->hw_offset = intf->hif_offset;
	intf->priv = priv;
	intf->fw_awake = 1;
	intf->fw_power_down = 0;
	intf->txnum_level = BOOST_TXNUM_LEVEL;
	intf->rxnum_level = BOOST_RXNUM_LEVEL;
	intf->boost = 0;
#ifdef UNISOC_WIFI_PS
		init_completion(&intf->suspend_completed);
#endif
	} else {
err:
		wl_err("%s: unregister %d ops\n",
		       __func__, g_intf_ops.max_num);

		for (; chn > 0; chn--)
			sprdwcn_bus_chn_deinit(&g_intf_ops.hif_ops[chn]);

		g_intf_ops.hif_ops = NULL;
		g_intf_ops.max_num = 0;
	}

	return ret;
}

void sprdwl_intf_deinit(struct sprdwl_intf *dev)
{
	int chn = 0;

	for (chn = 0; chn < g_intf_ops.max_num; chn++)
		sprdwcn_bus_chn_deinit(&g_intf_ops.hif_ops[chn]);

	g_intf_ops.intf = NULL;
	g_intf_ops.max_num = 0;
	dev->hw_intf = NULL;
}

int sprdwl_dis_flush_txlist(struct sprdwl_intf *intf, u8 lut_index)
{
	struct sprdwl_tx_msg *tx_msg;
	int i, j;

	if (lut_index <= 5) {
		wl_err("err lut_index:%d, %s, %d\n",
				lut_index, __func__, __LINE__);
		return -1;
	}
	wl_debug("disconnect, flush qoslist, %s, %d\n", __func__, __LINE__);
	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	for (i = 0; i < SPRDWL_MODE_MAX; i++)
		for (j = 0; j < SPRDWL_AC_MAX; j++)
				sprdwl_flush_tx_qoslist(tx_msg, i, j, lut_index);
	return 0;
}
