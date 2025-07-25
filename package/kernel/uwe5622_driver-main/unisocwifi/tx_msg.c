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

#include <linux/platform_device.h>
#include <linux/utsname.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ip.h>
#include "tx_msg.h"
#include "rx_msg.h"
#include "cfg80211.h"
#include "wl_core.h"
#include "sprdwl.h"
#include "wl_intf.h"
#include "mm.h"
#include "intf_ops.h"
#include "cmdevt.h"
#include "debug.h"
#include <linux/kthread.h>

struct sprdwl_msg_buf *sprdwl_get_msg_buf(void *pdev,
					  enum sprdwl_head_type type,
					  enum sprdwl_mode mode,
					  u8 ctx_id)
{
	struct sprdwl_intf *dev;
	struct sprdwl_msg_buf *msg = NULL;
	struct sprdwl_msg_list *list = NULL;
	struct sprdwl_tx_msg *sprdwl_tx_dev = NULL;
	struct sprdwl_msg_buf *msg_buf;
#if defined(MORE_DEBUG)
	struct timespec tx_begin;
#endif

	dev = (struct sprdwl_intf *)pdev;
	sprdwl_tx_dev = (struct sprdwl_tx_msg *)dev->sprdwl_tx;
	sprdwl_tx_dev->mode = mode;

	if (unlikely(dev->exit))
		return NULL;

	if (type == SPRDWL_TYPE_DATA)
		list = &sprdwl_tx_dev->tx_list_qos_pool;
	else
		list = &sprdwl_tx_dev->tx_list_cmd;

	if (!list) {
		wl_err("%s: type %d could not get list\n", __func__, type);
		return NULL;
	}

	if (type == SPRDWL_TYPE_DATA && atomic_read(&list->ref) > (SPRDWL_TX_QOS_POOL_SIZE * 8 / 10)) {
		msg_buf = kzalloc(sizeof(*msg_buf), GFP_KERNEL);
		if (msg_buf) {
			INIT_LIST_HEAD(&msg_buf->list);
			spin_lock_bh(&sprdwl_tx_dev->tx_list_qos_pool.freelock);
			list_add_tail(&msg_buf->list,
			&sprdwl_tx_dev->tx_list_qos_pool.freelist);
			spin_unlock_bh(&sprdwl_tx_dev->tx_list_qos_pool.freelock);
			sprdwl_tx_dev->tx_list_qos_pool.maxnum++;
			msg = sprdwl_alloc_msg_buf(list);
		} else {
			wl_err("%s failed to alloc msg_buf!\n", __func__);
		}
	} else {
		msg = sprdwl_alloc_msg_buf(list);
	}

	if (msg) {
#if defined(MORE_DEBUG)
		getnstimeofday(&tx_begin);
		msg->tx_start_time = timespec_to_ns(&tx_begin);
#endif
		if (type == SPRDWL_TYPE_DATA)
			msg->msg_type = SPRDWL_TYPE_DATA;
		msg->type = type;
		msg->msglist = list;
		msg->mode = mode;
		msg->xmit_msg_list = &sprdwl_tx_dev->xmit_msg_list;
		return msg;
	}

	if (type == SPRDWL_TYPE_DATA) {
		sprdwl_tx_dev->net_stop_cnt++;
		sprdwl_net_flowcontrl(dev->priv, mode, false);
		atomic_set(&list->flow, 1);
	}
	wl_err_ratelimited("%s no more msgbuf for %s\n",
			   __func__, type == SPRDWL_TYPE_DATA ?
			   "data" : "cmd");

	return NULL;
}

static void sprdwl_dequeue_cmd_buf(struct sprdwl_msg_buf *msg_buf,
					struct sprdwl_msg_list *list)
{
	spin_lock_bh(&list->busylock);
	list_del(&msg_buf->list);
	spin_unlock_bh(&list->busylock);

	spin_lock_bh(&list->complock);
	list_add_tail(&msg_buf->list, &list->cmd_to_free);
	spin_unlock_bh(&list->complock);
}

void sprdwl_free_cmd_buf(struct sprdwl_msg_buf *msg_buf,
			   struct sprdwl_msg_list *list)
{
	spin_lock_bh(&list->complock);
	list_del(&msg_buf->list);
	spin_unlock_bh(&list->complock);
	sprdwl_free_msg_buf(msg_buf, list);
}

void sprdwl_tx_free_msg_buf(void *pdev, struct sprdwl_msg_buf *msg)
{
	sprdwl_free_msg_buf(msg, msg->msglist);
}

static inline void
sprdwl_queue_data_msg_buf(struct sprdwl_msg_buf *msg_buf)
{
#ifdef OTT_UWE
	struct tx_msdu_dscr *dscr = (struct tx_msdu_dscr *)(msg_buf->tran_data + FOUR_BYTES_ALIGN_OFFSET);
#else
	struct tx_msdu_dscr *dscr = (struct tx_msdu_dscr *)(msg_buf->tran_data);
#endif

	spin_lock_bh(&msg_buf->data_list->p_lock);
	/*to make sure ARP/TDLS/preauth can be tx ASAP*/
	if (dscr->tx_ctrl.sw_rate == 1)
		list_add(&msg_buf->list, &msg_buf->data_list->head_list);
	else
		list_add_tail(&msg_buf->list, &msg_buf->data_list->head_list);
	atomic_inc(&msg_buf->data_list->l_num);
	spin_unlock_bh(&msg_buf->data_list->p_lock);
}

static inline void
sprdwl_dequeue_qos_buf(struct sprdwl_msg_buf *msg_buf, int ac_index)
{
	spinlock_t *lock;/*to lock qos list*/

	if (ac_index != SPRDWL_AC_MAX)
		lock = &msg_buf->data_list->p_lock;
	else
		lock = &msg_buf->xmit_msg_list->send_lock;
	spin_lock_bh(lock);
	dev_kfree_skb(msg_buf->skb);
	list_del(&msg_buf->list);
	sprdwl_free_msg_buf(msg_buf, msg_buf->msglist);
	spin_unlock_bh(lock);
}

void sprdwl_flush_tx_qoslist(struct sprdwl_tx_msg *tx_msg, int mode, int ac_index, int lut_index)
{
	/*peer list lock*/
	spinlock_t *plock;
	struct sprdwl_msg_buf *pos_buf, *temp_buf;
	struct list_head *data_list;

	data_list =
	&tx_msg->tx_list[mode]->q_list[ac_index].p_list[lut_index].head_list;

	plock =
	&tx_msg->tx_list[mode]->q_list[ac_index].p_list[lut_index].p_lock;

	if (!list_empty(data_list)) {
		spin_lock_bh(plock);

		list_for_each_entry_safe(pos_buf, temp_buf,
					 data_list, list) {
			dev_kfree_skb(pos_buf->skb);
			list_del(&pos_buf->list);
			sprdwl_free_msg_buf(pos_buf, pos_buf->msglist);
		}

		spin_unlock_bh(plock);

		atomic_sub(atomic_read(&tx_msg->tx_list[mode]->q_list[ac_index].p_list[lut_index].l_num),
					&tx_msg->tx_list[mode]->mode_list_num);
		atomic_set(&tx_msg->tx_list[mode]->q_list[ac_index].p_list[lut_index].l_num, 0);
	}
}

void sprdwl_flush_mode_txlist(struct sprdwl_tx_msg *tx_msg, enum sprdwl_mode mode)
{
	int i, j;
	/*peer list lock*/
	spinlock_t *plock;
	struct sprdwl_msg_buf *pos_buf, *temp_buf;
	struct list_head *data_list;

	wl_info("%s, mode=%d\n", __func__, mode);

	for (i = 0; i < SPRDWL_AC_MAX; i++) {
		for (j = 0; j < MAX_LUT_NUM; j++) {
			data_list =
			&tx_msg->tx_list[mode]->q_list[i].p_list[j].head_list;
			if (list_empty(data_list))
				continue;
			plock =
			&tx_msg->tx_list[mode]->q_list[i].p_list[j].p_lock;

			spin_lock_bh(plock);

			list_for_each_entry_safe(pos_buf, temp_buf,
						 data_list, list) {
				dev_kfree_skb(pos_buf->skb);
				list_del(&pos_buf->list);
				sprdwl_free_msg_buf(pos_buf, pos_buf->msglist);
			}

			spin_unlock_bh(plock);

			atomic_set(&tx_msg->tx_list[mode]->q_list[i].p_list[j].l_num, 0);
		}
	}
	atomic_set(&tx_msg->tx_list[mode]->mode_list_num, 0);
}

void sprdwl_flush_tosendlist(struct sprdwl_tx_msg *tx_msg)
{
	struct sprdwl_msg_buf *pos_buf, *temp_buf;
	struct list_head *data_list;

	wl_debug("%s, %d\n", __func__, __LINE__);
	if (!list_empty(&tx_msg->xmit_msg_list.to_send_list)) {
		wl_warn("%s, %d, flush!\n", __func__, __LINE__);
		data_list = &tx_msg->xmit_msg_list.to_send_list;
		list_for_each_entry_safe(pos_buf, temp_buf,
			data_list, list) {
			sprdwl_dequeue_qos_buf(pos_buf, SPRDWL_AC_MAX);
		}
	}
}

static void sprdwl_flush_data_txlist(struct sprdwl_tx_msg *tx_msg)
{
	enum sprdwl_mode mode;
	struct list_head *data_list;
	int cnt = 0;

	for (mode = SPRDWL_MODE_STATION; mode < SPRDWL_MODE_MAX; mode++) {
		if (atomic_read(&tx_msg->tx_list[mode]->mode_list_num) == 0)
			continue;
		sprdwl_flush_mode_txlist(tx_msg, mode);
	}

	sprdwl_flush_tosendlist(tx_msg);
	data_list = &tx_msg->xmit_msg_list.to_free_list;
	/*wait until data list sent completely and freed by HIF*/
	wl_debug("%s check if data freed complete start\n", __func__);
	while (!list_empty(data_list) && (cnt < 1000)) {
		usleep_range(2500, 3000);
		cnt++;
	}

	wl_debug("%s check if data freed complete end\n", __func__);
}

void sprdwl_dequeue_data_buf(struct sprdwl_msg_buf *msg_buf)
{
	spin_lock_bh(&msg_buf->xmit_msg_list->free_lock);
	list_del(&msg_buf->list);
	spin_unlock_bh(&msg_buf->xmit_msg_list->free_lock);
	sprdwl_free_msg_buf(msg_buf, msg_buf->msglist);
}

void sprdwl_dequeue_data_list(struct mbuf_t *head, int num)
{
	int i;
	struct sprdwl_msg_buf *msg_pos;
	struct mbuf_t *mbuf_pos = NULL;

	mbuf_pos = head;
	for (i = 0; i < num; i++) {
		msg_pos = GET_MSG_BUF(mbuf_pos);
		/*TODO, check msg_buf after pop link*/
		if (msg_pos == NULL ||
			!virt_addr_valid(msg_pos) ||
			!virt_addr_valid(msg_pos->skb)) {
			wl_err("%s,%d, error! wrong sprdwl_msg_buf\n",
				   __func__, __LINE__);
			BUG_ON(1);
		}
		dev_kfree_skb(msg_pos->skb);
		/*delete node from to_free_list*/
		spin_lock_bh(&msg_pos->xmit_msg_list->free_lock);
		list_del(&msg_pos->list);
		spin_unlock_bh(&msg_pos->xmit_msg_list->free_lock);
		/*add it to free_list*/
		spin_lock_bh(&msg_pos->msglist->freelock);
		list_add_tail(&msg_pos->list, &msg_pos->msglist->freelist);
		spin_unlock_bh(&msg_pos->msglist->freelock);
		mbuf_pos = mbuf_pos->next;
	}
}

/* seam for tx_thread */
void tx_down(struct sprdwl_tx_msg *tx_msg)
{
	int ret;
	do {
		ret = wait_for_completion_interruptible(&tx_msg->tx_completed);
	} while (ret == -ERESTARTSYS);
}

void tx_up(struct sprdwl_tx_msg *tx_msg)
{
	complete(&tx_msg->tx_completed);
}

/*To clear mode assigned in flow_ctrl
 *and to flush data lit of closed mode
 */
void handle_tx_status_after_close(struct sprdwl_vif *vif)
{
	struct sprdwl_priv *priv = vif->priv;
	enum sprdwl_mode mode;
	u8 i, allmode_closed = 1;
	struct sprdwl_intf *intf;
	struct sprdwl_tx_msg *tx_msg;

	for (mode = SPRDWL_MODE_STATION; mode < SPRDWL_MODE_MAX; mode++) {
		if (priv->fw_stat[mode] != SPRDWL_INTF_CLOSE) {
			allmode_closed = 0;
			break;
		}
	}
	intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	if (allmode_closed == 1) {
		/*all modee closed,
		 *reset all credit
		*/
		wl_info("%s, %d, _fc_, delete flow num after all closed\n",
			__func__, __LINE__);
		for (i = 0; i < MAX_COLOR_BIT; i++) {
			tx_msg->flow_ctrl[i].mode = SPRDWL_MODE_NONE;
			tx_msg->flow_ctrl[i].color_bit = i;
			tx_msg->ring_cp = 0;
			tx_msg->ring_ap = 0;
			atomic_set(&tx_msg->flow_ctrl[i].flow, 0);
		}
	} else {
		/*a mode closed,
		 *remove it from flow control to
		 *make it shared by other still open mode
		*/
		for (i = 0; i < MAX_COLOR_BIT; i++) {
			if (tx_msg->flow_ctrl[i].mode == vif->mode) {
				wl_info(" %s, %d, _fc_, clear mode%d because closed\n",
					__func__, __LINE__, vif->mode);
				tx_msg->flow_ctrl[i].mode = SPRDWL_MODE_NONE;
			}
		}
		/*if tx_list[mode] not empty,
		 *but mode is closed, should flush it
		*/
		if (mode < SPRDWL_MODE_MAX && priv->fw_stat[mode] == SPRDWL_INTF_CLOSE &&
			atomic_read(&tx_msg->tx_list[mode]->mode_list_num) != 0)
			sprdwl_flush_mode_txlist(tx_msg, mode);
	}
}

void sprdwl_init_xmit_list(struct sprdwl_tx_msg *tx_msg)
{
	INIT_LIST_HEAD(&tx_msg->xmit_msg_list.to_send_list);
	INIT_LIST_HEAD(&tx_msg->xmit_msg_list.to_free_list);
	spin_lock_init(&tx_msg->xmit_msg_list.send_lock);
	spin_lock_init(&tx_msg->xmit_msg_list.free_lock);
}

static void
add_xmit_list_tail(struct sprdwl_tx_msg *tx_msg,
		   struct peer_list *p_list,
		   int add_num)
{
	struct list_head *pos_list = NULL, *n_list;
	struct list_head temp_list;
	int num = 0;

	if (add_num == 0)
		return;
	spin_lock_bh(&p_list->p_lock);
	list_for_each_safe(pos_list, n_list, &p_list->head_list) {
		num++;
		if (num == add_num)
			break;
	}
	if (num != add_num)
		wl_err("%s, %d, error! add_num:%d, num:%d\n",
			   __func__, __LINE__, add_num, num);
	INIT_LIST_HEAD(&temp_list);
	list_cut_position(&temp_list,
			  &p_list->head_list,
			  pos_list);
	list_splice_tail(&temp_list,
		&tx_msg->xmit_msg_list.to_send_list);
	if (list_empty(&p_list->head_list))
		INIT_LIST_HEAD(&p_list->head_list);
	spin_unlock_bh(&p_list->p_lock);
	wl_debug("%s,%d,q_num%d,tosend_num%d\n", __func__, __LINE__,
		 get_list_num(&p_list->head_list),
		 get_list_num(&tx_msg->xmit_msg_list.to_send_list));
}

unsigned int queue_is_empty(struct sprdwl_tx_msg *tx_msg, enum sprdwl_mode mode)
{
	int i, j;
	struct tx_t *tx_t_list = tx_msg->tx_list[mode];

	if (mode == SPRDWL_MODE_AP || mode == SPRDWL_MODE_P2P_GO) {
		for (i = 0;  i < SPRDWL_AC_MAX; i++) {
			for (j = 0;  j < MAX_LUT_NUM; j++) {
				if (!list_empty(&tx_t_list->q_list[i].p_list[j].head_list))
					return 0;
			}
		}
		return 1;
	}
	/*other mode, STA/GC/...*/
	j = tx_msg->tx_list[mode]->lut_id;
	for (i = 0;  i < SPRDWL_AC_MAX; i++) {
		if (!list_empty(&tx_t_list->q_list[i].p_list[j].head_list))
			return 0;
	}
	return 1;
}

void sprdwl_wake_net_ifneed(struct sprdwl_intf *dev,
				struct sprdwl_msg_list *list,
				enum sprdwl_mode mode)
{
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)dev->sprdwl_tx;

	if (atomic_read(&list->flow)) {
		if (atomic_read(&list->ref) <= SPRDWL_TX_DATA_START_NUM) {
			atomic_set(&list->flow, 0);
			tx_msg->net_start_cnt++;
			sprdwl_net_flowcontrl(dev->priv, mode, true);
		}
	}
}

static void sprdwl_sdio_flush_txlist(struct sprdwl_msg_list *list)
{
	struct sprdwl_msg_buf *msgbuf;
	int cnt = 0;

	/*wait until cmd list sent completely and freed by HIF*/
	while (!list_empty(&list->cmd_to_free) && (cnt < 3000)) {
		wl_debug("%s cmd not yet transmited", __func__);
		usleep_range(2950, 3050);
		cnt++;
	}
	while ((msgbuf = sprdwl_peek_msg_buf(list))) {
		if (msgbuf->skb)
			dev_kfree_skb(msgbuf->skb);
		else
			kfree(msgbuf->tran_data);
		sprdwl_dequeue_msg_buf(msgbuf, list);
		continue;
	}
}

static int sprdwl_tx_cmd(struct sprdwl_intf *intf, struct sprdwl_msg_list *list)
{
	int ret = 0;
	struct sprdwl_msg_buf *msgbuf;
	struct sprdwl_tx_msg *tx_msg;

	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	while ((msgbuf = sprdwl_peek_msg_buf(list))) {
		if (unlikely(intf->exit)) {
			kfree(msgbuf->tran_data);
			msgbuf->tran_data = NULL;
			sprdwl_dequeue_msg_buf(msgbuf, list);
			continue;
		}
		if (time_after(jiffies, msgbuf->timeout)) {
			tx_msg->drop_cmd_cnt++;
			wl_err("tx drop cmd msg,dropcnt:%lu\n",
				   tx_msg->drop_cmd_cnt);
			kfree(msgbuf->tran_data);
			msgbuf->tran_data = NULL;
			sprdwl_dequeue_msg_buf(msgbuf, list);
			continue;
		}
		sprdwl_dequeue_cmd_buf(msgbuf, list);
		tx_msg->cmd_send++;
		wl_debug("tx_cmd cmd_send num: %d\n", tx_msg->cmd_send);

		/*TBD, temp solution: send CMD one by one*/
		ret = if_tx_cmd(intf, (unsigned char *)msgbuf->tran_data,
				msgbuf->len);
		if (ret) {
			wl_err("%s err:%d\n", __func__, ret);
			/* fixme if need retry */
			msgbuf->tran_data = NULL;
			sprdwl_free_cmd_buf(msgbuf, list);
		}
	}

	return 0;
}

void sprdwl_fc_add_share_credit(struct sprdwl_vif *vif)
{
	struct sprdwl_intf *intf;
	struct sprdwl_tx_msg *tx_msg;
	u8 i;

	intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	for (i = 0; i < MAX_COLOR_BIT; i++) {
		if (tx_msg->flow_ctrl[i].mode == vif->mode) {
			//wl_err("%s, %d, mode:%d closed, index:%d, share it\n",
			//	   __func__, __LINE__,
			//	   vif->mode, i);
			tx_msg->flow_ctrl[i].mode = SPRDWL_MODE_NONE;
			break;
		}
	}
}

int sprdwl_fc_find_color_per_mode(struct sprdwl_tx_msg *tx_msg,
				enum sprdwl_mode mode,
				u8 *index)
{
	u8 i = 0, found = 0;
	struct sprdwl_priv *priv = tx_msg->intf->priv;

	for (i = 0; i < MAX_COLOR_BIT; i++) {
		if (tx_msg->flow_ctrl[i].mode == mode) {
			found = 1;
			wl_debug("%s, %d, mode:%d found, index:%d\n",
				 __func__, __LINE__,
				 mode, i);
			break;
		}
	}
	if (found == 0) {
		/*a new mode. sould assign new color to this mode*/
		for (i = 0; i < MAX_COLOR_BIT; i++) {
			if ((tx_msg->flow_ctrl[i].mode != SPRDWL_MODE_NONE) &&
				(priv->fw_stat[tx_msg->flow_ctrl[i].mode]
				== SPRDWL_INTF_CLOSE))
				tx_msg->flow_ctrl[i].mode = SPRDWL_MODE_NONE;
		}
		for (i = 0; i < MAX_COLOR_BIT; i++) {
			if (tx_msg->flow_ctrl[i].mode == SPRDWL_MODE_NONE) {
				found = 1;
				tx_msg->flow_ctrl[i].mode = mode;
				tx_msg->flow_ctrl[i].color_bit = i;
				wl_info("%s, %d, new mode:%d, assign color:%d\n",
					__func__, __LINE__,
					mode, i);
				break;
			}
		}
	}
	if (found == 1)
		*index = i;
	return found;
}

int sprdwl_fc_get_shared_num(struct sprdwl_tx_msg *tx_msg, u8 num)
{
	u8 i;
	int shared_flow_num = 0;
	unsigned int color_flow;

	for (i = 0; i < MAX_COLOR_BIT; i++) {
		color_flow = atomic_read(&tx_msg->flow_ctrl[i].flow);
		if ((tx_msg->flow_ctrl[i].mode == SPRDWL_MODE_NONE) &&
			(0 != color_flow)) {
			if ((num - shared_flow_num) <= color_flow) {
				/*one shared color is enough?*/
				tx_msg->color_num[i] = num - shared_flow_num;
				shared_flow_num += num - shared_flow_num;
				break;
			} else {
				/*need one more shared color*/
				tx_msg->color_num[i] = color_flow;
				shared_flow_num += color_flow;
			}
		}
	}
	return shared_flow_num;
}

int sprdwl_fc_get_send_num(struct sprdwl_tx_msg *tx_msg,
				 enum sprdwl_mode mode,
				 int data_num)
{
	int excusive_flow_num = 0, shared_flow_num = 0;
	int send_num = 0;
	u8 i = 0;
	struct sprdwl_priv *priv = tx_msg->intf->priv;

	if (data_num <= 0 || mode == SPRDWL_MODE_NONE)
		return 0;
	/*send all data in buff with PCIe interface*/
	if (priv->hw_type == SPRDWL_HW_PCIE)
		return data_num;
	if (data_num > 100)
		data_num = 100;
	if (priv->credit_capa == TX_NO_CREDIT)
		return data_num;
	memset(tx_msg->color_num, 0x00, MAX_COLOR_BIT);

	if (sprdwl_fc_find_color_per_mode(tx_msg, mode, &i) == 1) {
		excusive_flow_num  = atomic_read(&tx_msg->flow_ctrl[i].flow);
		if (excusive_flow_num >= data_num) {
			/*excusive flow is enough, do not need shared flow*/
			send_num = tx_msg->color_num[i] = data_num;
		} else {
			/*excusive flow not enough, need shared flow
			 *total give num =  excusive + shared
			 *(may be more than one color)*/
			u8 num_need = data_num - excusive_flow_num;

			shared_flow_num =
				sprdwl_fc_get_shared_num(tx_msg, num_need);
			tx_msg->color_num[i] = excusive_flow_num;
			send_num = excusive_flow_num + shared_flow_num;
		}

		if (send_num <= 0) {
			wl_err("%s, %d, mode:%d, e_num:%d, s_num:%d, d_num:%d\n",
				   __func__, __LINE__,
				   (u8)mode, excusive_flow_num,
				   shared_flow_num, data_num);
			return -ENOMEM;
		}
		wl_debug("%s, %d, mode:%d, e_num:%d, s_num:%d, d_num:%d\n"
			"color_num = {%d, %d, %d, %d}\n",
			__func__, __LINE__, mode, excusive_flow_num,
			shared_flow_num, data_num,
			tx_msg->color_num[0], tx_msg->color_num[1],
			tx_msg->color_num[2], tx_msg->color_num[3]);
	} else {
		wl_err("%s, %d, wrong mode:%d?\n",
			   __func__, __LINE__,
			   (u8)mode);
		for (i = 0; i < MAX_COLOR_BIT; i++)
			wl_err("color[%d] assigned mode%d\n",
				   i, (u8)tx_msg->flow_ctrl[i].mode);
		return -ENOMEM;
	}

	return send_num;
}

/*to see there is shared flow or not*/
int sprdwl_fc_test_shared_num(struct sprdwl_tx_msg *tx_msg)
{
	u8 i;
	int shared_flow_num = 0;
	unsigned int color_flow;

	for (i = 0; i < MAX_COLOR_BIT; i++) {
		color_flow = atomic_read(&tx_msg->flow_ctrl[i].flow);
		if ((tx_msg->flow_ctrl[i].mode == SPRDWL_MODE_NONE) &&
			(0 != color_flow)) {
			shared_flow_num += color_flow;
		}
	}
	return shared_flow_num;
}
/*to check flow number, no flow number, no send*/
int sprdwl_fc_test_send_num(struct sprdwl_tx_msg *tx_msg,
				 enum sprdwl_mode mode,
				 int data_num)
{
	int excusive_flow_num = 0, shared_flow_num = 0;
	int send_num = 0;
	u8 i = 0;
	struct sprdwl_priv *priv = tx_msg->intf->priv;

	if (data_num <= 0 || mode == SPRDWL_MODE_NONE)
		return 0;
	/*send all data in buff with PCIe interface, TODO*/
	if (priv->hw_type == SPRDWL_HW_PCIE)
		return data_num;
	if (data_num > 100)
		data_num = 100;
	if (priv->credit_capa == TX_NO_CREDIT)
		return data_num;
	if (sprdwl_fc_find_color_per_mode(tx_msg, mode, &i) == 1) {
		excusive_flow_num = atomic_read(&tx_msg->flow_ctrl[i].flow);
		shared_flow_num =
				sprdwl_fc_test_shared_num(tx_msg);
		send_num = excusive_flow_num + shared_flow_num;

		if (send_num <= 0) {
			wl_debug("%s, %d, err, mode:%d, e_num:%d, s_num:%d, d_num=%d\n",
				 __func__, __LINE__, (u8)mode,
				 excusive_flow_num, shared_flow_num, data_num);
			return -ENOMEM;
		}
		wl_debug("%s, %d, e_num=%d, s_num=%d, d_num=%d\n",
			 __func__, __LINE__, excusive_flow_num,
			 shared_flow_num, data_num);
	} else {
		wl_err("%s, %d, wrong mode:%d?\n",
			   __func__, __LINE__,
			   (u8)mode);
		for (i = 0; i < MAX_COLOR_BIT; i++)
			wl_err_ratelimited("color[%d] assigned mode%d\n",
				   i, (u8)tx_msg->flow_ctrl[i].mode);
		return -ENOMEM;
	}

	return min(send_num, data_num);
}

u8 sprdwl_fc_set_clor_bit(struct sprdwl_tx_msg *tx_msg, int num)
{
	u8 i = 0;
	int count_num = 0;
	struct sprdwl_priv *priv = tx_msg->intf->priv;

	if (priv->credit_capa == TX_NO_CREDIT)
		return 0;

	for (i = 0; i < MAX_COLOR_BIT; i++) {
		count_num += tx_msg->color_num[i];
		if (num <= count_num)
			break;
	}
	wl_debug("%s, %d, color bit =%d\n", __func__, __LINE__, i);
	return i;
}

void sprdwl_handle_tx_return(struct sprdwl_tx_msg *tx_msg,
				  struct sprdwl_msg_list *list,
				  int send_num, int ret)
{
	u8 i;
	struct sprdwl_priv *priv = tx_msg->intf->priv;

	if (ret) {
		wl_err_ratelimited("%s tx err:%d\n", __func__, ret);
		memset(tx_msg->color_num, 0x00, MAX_COLOR_BIT);
		usleep_range(20, 30);
		return;
	}
	tx_msg->ring_ap += send_num;
	atomic_sub(send_num, &list->ref);
	sprdwl_wake_net_ifneed(tx_msg->intf, list, tx_msg->mode);

	if (priv->credit_capa == TX_NO_CREDIT)
		return;

	for (i = 0; i < MAX_COLOR_BIT; i++) {
		if (tx_msg->color_num[i] == 0)
			continue;
		atomic_sub(tx_msg->color_num[i],
			&tx_msg->flow_ctrl[i].flow);
		wl_debug("%s, _fc_, color bit:%d, flow num-%d=%d, seq_num=%d\n",
			 __func__, i, tx_msg->color_num[i],
			 atomic_read(&tx_msg->flow_ctrl[i].flow),
			 tx_msg->seq_num);
	}
}

int handle_tx_timeout(struct sprdwl_tx_msg *tx_msg,
			  struct sprdwl_msg_list *msg_list,
			  struct peer_list *p_list, int ac_index)
{
	u8 mode;
	char *pinfo;
	spinlock_t *lock;
	int cnt, i, del_list_num;
	struct list_head *tx_list;
	struct sprdwl_msg_buf *pos_buf, *temp_buf, *tailbuf;

	if (SPRDWL_AC_MAX != ac_index) {
		tx_list = &p_list->head_list;
		lock = &p_list->p_lock;
		spin_lock_bh(lock);
		if (list_empty(tx_list)) {
			spin_unlock_bh(lock);
			return 0;
		}
		tailbuf = list_first_entry(tx_list, struct sprdwl_msg_buf, list);
		spin_unlock_bh(lock);
	} else {
		tx_list = &tx_msg->xmit_msg_list.to_send_list;
		if (list_empty(tx_list))
			return 0;
		tailbuf = list_first_entry(tx_list, struct sprdwl_msg_buf, list);
	}

	if (time_after(jiffies, tailbuf->timeout)) {
		mode = tailbuf->mode;
		i = 0;
		lock = &p_list->p_lock;
		spin_lock_bh(lock);
		del_list_num = atomic_read(&p_list->l_num) * atomic_read(&p_list->l_num) /
						msg_list->maxnum;
		if (del_list_num >= atomic_read(&p_list->l_num))
			del_list_num = atomic_read(&p_list->l_num);
		wl_err("tx timeout drop num:%d, l_num:%d, maxnum:%d",
			del_list_num, atomic_read(&p_list->l_num), msg_list->maxnum);
		spin_unlock_bh(lock);
		list_for_each_entry_safe(pos_buf,
		temp_buf, tx_list, list) {
			if (i >= del_list_num)
				break;
			wl_info("%s:%d buf->timeout\n",
				 __func__, __LINE__);
			if (pos_buf->mode <= SPRDWL_MODE_AP) {
				pinfo = "STA/AP mode";
				cnt = tx_msg->drop_data1_cnt++;
			} else {
				pinfo = "P2P mode";
				cnt = tx_msg->drop_data2_cnt++;
			}
			wl_info("tx drop %s, dropcnt:%u\n",
				pinfo, cnt);
			sprdwl_dequeue_qos_buf(pos_buf, ac_index);
			atomic_dec(&tx_msg->tx_list[mode]->mode_list_num);
#if defined(MORE_DEBUG)
			tx_msg->intf->stats.tx_dropped++;
#endif
			i++;
		}
		lock = &p_list->p_lock;
		spin_lock_bh(lock);
		atomic_sub(del_list_num, &p_list->l_num);
		spin_unlock_bh(lock);
		return -ENOMEM;
	}
	return 0;
}

static int sprdwl_handle_to_send_list(struct sprdwl_intf *intf,
					  enum sprdwl_mode mode)
{
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	struct list_head *to_send_list, tx_list_head;
	spinlock_t *t_lock;/*to protect get_list_num*/
	int tosendnum = 0, credit = 0, ret = 0;
	struct sprdwl_msg_list *list = &tx_msg->tx_list_qos_pool;
	u8 coex_bt_on = intf->coex_bt_on;

#ifdef CP2_RESET_SUPPORT
	if (intf->cp_asserted == 1)
		return 0;
#endif

	if (!list_empty(&tx_msg->xmit_msg_list.to_send_list)) {
		to_send_list = &tx_msg->xmit_msg_list.to_send_list;
		t_lock = &tx_msg->xmit_msg_list.send_lock;
		spin_lock_bh(t_lock);
		tosendnum = get_list_num(to_send_list);
		spin_unlock_bh(t_lock);
		credit = sprdwl_fc_get_send_num(tx_msg, mode, tosendnum);
		if (credit < tosendnum)
			wl_err("%s, %d,error! credit:%d,tosendnum:%d\n",
				   __func__, __LINE__,
				   credit, tosendnum);
		if (credit <= 0)
			return -ENOMEM;
		tx_msg->xmit_msg_list.mode = mode;

		ret = sprdwl_intf_tx_list(intf,
					  to_send_list,
					  &tx_list_head,
					  credit,
					  SPRDWL_AC_MAX,
					  coex_bt_on);
		sprdwl_handle_tx_return(tx_msg, list, credit, ret);
		if (0 != ret) {
			wl_err("%s, %d: tx return err!\n",
				   __func__, __LINE__);
			tx_msg->xmit_msg_list.failcount++;
			if (tx_msg->xmit_msg_list.failcount > 10) {
				sprdwl_flush_tosendlist(tx_msg);
				tx_msg->xmit_msg_list.failcount = 0;
			}
			return -ENOMEM;
		}
	}
	return 0;
}

unsigned int vo_ratio = 87;
unsigned int vi_ratio = 90;
unsigned int be_ratio = 81;
static int sprdwl_tx_eachmode_data(struct sprdwl_intf *intf,
				   enum sprdwl_mode mode)
{
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	int ret, i, j;
	struct list_head tx_list_head;
	struct qos_list *q_list;
	struct peer_list *p_list;
	struct sprdwl_msg_list *list = &tx_msg->tx_list_qos_pool;
	struct tx_t *tx_list = tx_msg->tx_list[mode];
	int send_num = 0, total = 0, min_num = 0, round_num = 0;
	int q_list_num[SPRDWL_AC_MAX] = {0, 0, 0, 0};
	int p_list_num[SPRDWL_AC_MAX][MAX_LUT_NUM] = {{0}, {0}, {0}, {0} };

	INIT_LIST_HEAD(&tx_list_head);
	/* first, go through all list, handle timeout msg
	 * and count each TID's tx_num and total tx_num
	 */
	for (i = 0; i < SPRDWL_AC_MAX; i++) {
		for (j = 0; j < MAX_LUT_NUM; j++) {
			p_list = &tx_list->q_list[i].p_list[j];
			if (atomic_read(&p_list->l_num) > 0) {
				if (0 != handle_tx_timeout(tx_msg,
							   list,
							   p_list,
							   i))
					wl_err("TID=%s%s%s%s, timeout!\n",
						   (i == SPRDWL_AC_VO) ? "VO" : "",
						   (i == SPRDWL_AC_VI) ? "VI" : "",
						   (i == SPRDWL_AC_BE) ? "BE" : "",
						   (i == SPRDWL_AC_BK) ? "BK" : "");
				p_list_num[i][j] = atomic_read(&p_list->l_num);
				/*wl_info("TID=%d,PEER=%d,l_num=%d\n",i,j,p_list_num[i][j]);*/
				q_list_num[i] += p_list_num[i][j];
			}
		}
		total += q_list_num[i];
		if (q_list_num[i] != 0)
			wl_debug("TID%s%s%s%snum=%d, total=%d\n",
				   (i == SPRDWL_AC_VO) ? "VO" : "",
				   (i == SPRDWL_AC_VI) ? "VI" : "",
				   (i == SPRDWL_AC_BE) ? "BE" : "",
				   (i == SPRDWL_AC_BK) ? "BK" : "",
				   q_list_num[i], total);
	}
	send_num = sprdwl_fc_test_send_num(tx_msg, mode, total);
	if (total != 0 && send_num <= 0) {
		wl_err("%s, %d: _fc_ no credit!\n",
			   __func__, __LINE__);
		return -ENOMEM;
	}

	/* merge qos queues to to_send_list
	 * to best use of HIF interrupt
	 */
	/* case1: send_num >= total
	 * remained _fc_ num is more than remained qos data,
	 * just add all remained qos list to xmit list
	 * and send all xmit to_send_list
	 */
	if (send_num >= total) {
		for (i = 0; i < SPRDWL_AC_MAX; i++) {
			q_list = &tx_list->q_list[i];
			if (q_list_num[i] <= 0)
				continue;
			for (j = 0; j < MAX_LUT_NUM; j++) {
				p_list = &q_list->p_list[j];
				if (p_list_num[i][j] ==  0)
					continue;
				add_xmit_list_tail(tx_msg, p_list, p_list_num[i][j]);
				atomic_sub(p_list_num[i][j], &p_list->l_num);
				atomic_sub(p_list_num[i][j], &tx_list->mode_list_num);
				wl_debug("%s, %d, mode=%d, TID=%d, lut=%d, %d add to xmit_list, then l_num=%d, mode_list_num=%d\n",
					 __func__, __LINE__, mode, i, j,
					 p_list_num[i][j],
					 atomic_read(&p_list->l_num),
					 atomic_read(&tx_msg->tx_list[mode]->mode_list_num));
			}
		}
		ret = sprdwl_handle_to_send_list(intf, mode);
		return ret;
	}

	/* case2: send_num < total
	 * vo get 87%,vi get 90%,be get remain 81%
	 */
	for (i = 0; i < SPRDWL_AC_MAX; i++) {
		int fp_num = 0;/*assigned _fc_ num to qoslist*/

		q_list = &tx_list->q_list[i];
		if (q_list_num[i] <= 0)
			continue;
		if (send_num <= 0)
			break;

		if ((i == SPRDWL_AC_VO) && (total > q_list_num[i])) {
			round_num = send_num * vo_ratio / 100;
			fp_num = min(round_num, q_list_num[i]);
		} else if ((i == SPRDWL_AC_VI) && (total > q_list_num[i])) {
			round_num = send_num * vi_ratio / 100;
			fp_num = min(round_num, q_list_num[i]);
		} else if ((i == SPRDWL_AC_BE) && (total > q_list_num[i])) {
			round_num = send_num * be_ratio / 100;
			fp_num = min(round_num, q_list_num[i]);
		} else {
			fp_num = send_num * q_list_num[i] / total;
		}
		if (((total - q_list_num[i]) < (send_num - fp_num)) &&
			((total - q_list_num[i]) > 0))
			fp_num += (send_num - fp_num - (total - q_list_num[i]));

		total -= q_list_num[i];

		wl_debug("TID%s%s%s%s, credit=%d, fp_num=%d, remain=%d\n",
			(i == SPRDWL_AC_VO) ? "VO" : "",
			(i == SPRDWL_AC_VI) ? "VI" : "",
			(i == SPRDWL_AC_BE) ? "BE" : "",
			(i == SPRDWL_AC_BK) ? "BK" : "",
			send_num, fp_num, total);

		send_num -= fp_num;
		for (j = 0; j < MAX_LUT_NUM; j++) {
			if (p_list_num[i][j] == 0)
				continue;
			round_num = p_list_num[i][j] * fp_num / q_list_num[i];
			if (fp_num > 0 && round_num == 0)
				round_num = 1;/*round_num = 0.1~0.9*/
			min_num = min(round_num, fp_num);
			wl_debug("TID=%d,PEER=%d,%d,%d,%d,%d,%d\n",
				 i, j, p_list_num[i][j], q_list_num[i], round_num, fp_num, min_num);
			if (min_num <= 0)
				break;
			q_list_num[i] -= p_list_num[i][j];
			fp_num -= min_num;
			add_xmit_list_tail(tx_msg,
					   &q_list->p_list[j],
					   min_num);
			atomic_sub(min_num, &q_list->p_list[j].l_num);
			atomic_sub(min_num, &tx_list->mode_list_num);
			wl_debug("%s, %d, mode=%d, TID=%d, lut=%d, %d add to xmit_list, then l_num=%d, mode_list_num=%d\n",
				 __func__, __LINE__, mode, i, j,
				 min_num,
				 atomic_read(&p_list->l_num),
				 atomic_read(&tx_msg->tx_list[mode]->mode_list_num));
			if (fp_num <= 0)
				break;
		}
	}
	ret = sprdwl_handle_to_send_list(intf, mode);
	return ret;
}

void sprdwl_flush_all_txlist(struct sprdwl_tx_msg *sprdwl_tx_dev)
{
	sprdwl_sdio_flush_txlist(&sprdwl_tx_dev->tx_list_cmd);
	sprdwl_flush_data_txlist(sprdwl_tx_dev);
}

int sprdwl_sdio_process_credit(void *pdev, void *data)
{
	int ret = 0, i;
	unsigned char *flow;
	struct sprdwl_common_hdr *common;
	struct sprdwl_intf *intf;
	struct sprdwl_tx_msg *tx_msg;
	ktime_t kt;
	int in_count = 0;

	intf = (struct sprdwl_intf *)pdev;
	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	common = (struct sprdwl_common_hdr *)data;

	if (common->type == SPRDWL_TYPE_DATA_SPECIAL) {
		int offset = (size_t)&((struct rx_msdu_desc *)0)->rsvd5;

		flow = data + offset;
		goto out;
	}

	if (common->type == SPRDWL_TYPE_EVENT) {
		struct sprdwl_cmd_hdr *cmd;

		cmd = (struct sprdwl_cmd_hdr *)data;
		if (cmd->cmd_id == WIFI_EVENT_SDIO_FLOWCON) {
			flow = cmd->paydata;
			ret = -1;
			goto out;
		}
	}
	return 0;

out:
	if (flow[0])
		atomic_add(flow[0], &tx_msg->flow_ctrl[0].flow);
	if (flow[1])
		atomic_add(flow[1], &tx_msg->flow_ctrl[1].flow);
	if (flow[2])
		atomic_add(flow[2], &tx_msg->flow_ctrl[2].flow);
	if (flow[3])
		atomic_add(flow[3], &tx_msg->flow_ctrl[3].flow);
	if (flow[0] || flow[1] || flow[2] || flow[3]) {
		in_count = flow[0] + flow[1] + flow[2] + flow[3];
		tx_msg->ring_cp += in_count;
		tx_up(tx_msg);
	}
	/* Firmware want to reset credit, will send us
	 * a credit event with all 4 parameters set to zero
	 */
	if (in_count == 0) {
		/*in_count==0: reset credit event or a data without credit
		 *ret == -1:reset credit event
		 *for a data without credit:just return,donot print log
		*/
		if (ret == -1) {
			wl_info("%s, %d, _fc_ reset credit\n", __func__, __LINE__);
			for (i = 0; i < MAX_COLOR_BIT; i++) {
				if (tx_msg->ring_cp != 0)
					tx_msg->ring_cp -= atomic_read(&tx_msg->flow_ctrl[i].flow);
				atomic_set(&tx_msg->flow_ctrl[i].flow, 0);
				tx_msg->color_num[i] = 0;
			}
		}
		goto exit;
	}
	kt = ktime_get();
	/*1.(tx_msg->kt.tv64 == 0) means 1st event
	2.add (in_count == 0) to avoid
	division by expression in_count which
	may be zero has undefined behavior*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	if ((tx_msg->kt == 0) || (in_count == 0)) {
		tx_msg->kt = kt;
	} else {
		wl_info("%s, %d, %s, %dadded, %lld usec per flow\n",
		__func__, __LINE__,
		(ret == -1) ? "event" : "data",
		in_count,
		div_u64(div_u64(kt - tx_msg->kt, NSEC_PER_USEC), in_count));

		debug_record_add(TX_CREDIT_ADD, in_count);
		debug_record_add(TX_CREDIT_PER_ADD,
			div_u64(div_u64(kt - tx_msg->kt,
				 NSEC_PER_USEC), in_count));
		debug_record_add(TX_CREDIT_RECORD, jiffies_to_usecs(jiffies));
		debug_record_add(TX_CREDIT_TIME_DIFF,
			div_u64(kt - tx_msg->kt, NSEC_PER_USEC));
	}
#else
	if ((tx_msg->kt.tv64 == 0) || (in_count == 0)) {
		tx_msg->kt = kt;
	} else {
		wl_debug("%s, %s, %dadded, %lld usec per flow\n",
		__func__,
		(ret == -1) ? "event" : "data",
		in_count,
		div_u64(div_u64(kt.tv64 - tx_msg->kt.tv64, NSEC_PER_USEC), in_count));

		debug_record_add(TX_CREDIT_ADD, in_count);
		debug_record_add(TX_CREDIT_PER_ADD,
			div_u64(div_u64(kt.tv64 - tx_msg->kt.tv64,
				NSEC_PER_USEC), in_count));
		debug_record_add(TX_CREDIT_RECORD, jiffies_to_usecs(jiffies));
		debug_record_add(TX_CREDIT_TIME_DIFF,
			div_u64(kt.tv64 - tx_msg->kt.tv64, NSEC_PER_USEC));
	}
#endif
	tx_msg->kt = ktime_get();

	wl_info("_fc_,R+%d=%d,G+%d=%d,B+%d=%d,W+%d=%d,cp=%lu,ap=%lu\n",
		flow[0], atomic_read(&tx_msg->flow_ctrl[0].flow),
		flow[1], atomic_read(&tx_msg->flow_ctrl[1].flow),
		flow[2], atomic_read(&tx_msg->flow_ctrl[2].flow),
		flow[3], atomic_read(&tx_msg->flow_ctrl[3].flow),
		tx_msg->ring_cp, tx_msg->ring_ap);
exit:
	return ret;
}

void prepare_addba(struct sprdwl_intf *intf, unsigned char lut_index,
		   struct sk_buff *skb, struct sprdwl_peer_entry *peer_entry,
		   unsigned char tid)
{
	if (intf->tx_num[lut_index] > 9 &&
		peer_entry &&
		(peer_entry->ip_acquired || !sprdwl_is_group(skb->data)) &&
		peer_entry->ht_enable &&
		peer_entry->vowifi_enabled != 1 &&
		!test_bit(tid, &peer_entry->ba_tx_done_map)) {
		struct timespec time;

		getnstimeofday(&time);
		/*need to delay 3s if priv addba failed*/
		if (((timespec_to_ns(&time) - timespec_to_ns(&peer_entry->time[tid]))/1000000) > 3000 ||
			peer_entry->time[tid].tv_nsec == 0) {
			wl_debug("%s, %d, tx_addba, tid=%d\n",
				__func__, __LINE__, tid);
			getnstimeofday(&peer_entry->time[tid]);
			test_and_set_bit(tid, &peer_entry->ba_tx_done_map);
			sprdwl_tx_addba(intf, peer_entry, tid);
		}
	}
}

int sprdwl_tx_msg_func(void *pdev, struct sprdwl_msg_buf *msg)
{
	u16 len;
	unsigned char *info;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)pdev;
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	unsigned int qos_index = 0;
	struct sprdwl_peer_entry *peer_entry = NULL;
	unsigned char tid = 0, tos = 0;
	struct tx_msdu_dscr *dscr = (struct tx_msdu_dscr *)msg->tran_data;

	if (msg->msglist == &tx_msg->tx_list_cmd) {
		len = SPRDWL_MAX_CMD_TXLEN;
		info = "cmd";
		msg->timeout = jiffies + tx_msg->cmd_timeout;
	} else {
		len = SPRDWL_MAX_DATA_TXLEN;
		info = "data";
		msg->timeout = jiffies + tx_msg->data_timeout;
	}

	if (msg->len > len) {
		wl_err("%s err:%s too long:%d > %d,drop it\n",
				   __func__, info, msg->len, len);
#if defined(MORE_DEBUG)
		intf->stats.tx_dropped++;
#endif
		INIT_LIST_HEAD(&msg->list);
		sprdwl_free_msg_buf(msg, msg->msglist);
		return -EPERM;
	}

#ifdef OTT_UWE
	dscr = (struct tx_msdu_dscr *)(msg->tran_data + 3);
#else
	dscr = (struct tx_msdu_dscr *)(msg->tran_data);
#endif

	if (msg->msglist == &tx_msg->tx_list_qos_pool) {
		struct peer_list *data_list;

#ifdef OTT_UWE
		qos_index = get_tid_qosindex(msg->skb, 11 + 3, &tid, &tos);
#else
		qos_index = get_tid_qosindex(msg->skb, 11, &tid, &tos);
#endif

#ifdef WMMAC_WFA_CERTIFICATION
		qos_index = change_priority_if(intf->priv, &tid, &tos, msg->len);
		wl_debug("%s qos_index: %d tid: %d, tos:%d\n", __func__, qos_index, tid, tos);
		if (SPRDWL_AC_MAX == qos_index) {
			INIT_LIST_HEAD(&msg->list);
			sprdwl_free_msg_buf(msg, msg->msglist);
			return -EPERM;
		}
#endif
		/*send group in BK to avoid FW hang*/
		if ((msg->mode == SPRDWL_MODE_AP ||
			msg->mode == SPRDWL_MODE_P2P_GO) &&
			(dscr->sta_lut_index < 6)) {
			qos_index = SPRDWL_AC_BK;
			tid = prio_1;
			wl_debug("%s, %d, SOFTAP/GO group go as BK\n", __func__, __LINE__);
		} else {
			intf->tx_num[dscr->sta_lut_index]++;
		}
		dscr->buffer_info.msdu_tid = tid;
		peer_entry = &intf->peer_entry[dscr->sta_lut_index];
/*TODO. temp for MARLIN2 test*/
#if 0
		qos_index = qos_match_q(&tx_msg->tx_list_data,
					msg->skb, 10);/*temp for test*/
#endif
		data_list =
			&tx_msg->tx_list[msg->mode]->q_list[qos_index].p_list[dscr->sta_lut_index];
		tx_msg->tx_list[msg->mode]->lut_id = dscr->sta_lut_index;
		/*if ((qos_index == SPRDWL_AC_VO) ||
			(qos_index == SPRDWL_AC_VI) ||
			(qos_index == SPRDWL_AC_BE && data_list->l_num <= BE_TOTAL_QUOTA) ||
			(qos_index == SPRDWL_AC_BK && data_list->l_num <= BK_TOTAL_QUOTA)
			) {*/
			msg->data_list = data_list;
			sprdwl_queue_data_msg_buf(msg);
			atomic_inc(&tx_msg->tx_list[msg->mode]->mode_list_num);
		/*} else {
			dev_kfree_skb(msg->skb);
			INIT_LIST_HEAD(&msg->list);
			sprdwl_free_msg_buf(msg, msg->msglist);
			return 0;
		}*/
	}

	if (msg->msg_type != SPRDWL_TYPE_DATA)
		sprdwl_queue_msg_buf(msg, msg->msglist);

	prepare_addba(intf, dscr->sta_lut_index, (struct sk_buff *)msg->skb,
			 peer_entry, tid);

	if (msg->msg_type == SPRDWL_TYPE_CMD)
		tx_up(tx_msg);
	if (msg->msg_type == SPRDWL_TYPE_DATA &&
		((intf->fw_awake == 0 &&
		intf->fw_power_down == 1) ||
		intf->fw_awake == 1))
		tx_up(tx_msg);

	return 0;
}

static int sprdwl_tx_work_queue(void *data)
{
	unsigned long need_polling = 0;
	unsigned int polling_times = 0;
	struct sprdwl_intf *intf;
	struct sprdwl_tx_msg *tx_msg;
	enum sprdwl_mode mode = SPRDWL_MODE_NONE;
	int send_num = 0;
	struct sprdwl_priv *priv;

	tx_msg = (struct sprdwl_tx_msg *)data;
	intf = tx_msg->intf;
	priv = intf->priv;
	set_user_nice(current, -20);

	while (1) {
		tx_down(tx_msg);
		if (intf->exit || kthread_should_stop())
			return 0;
		need_polling = 0;
		polling_times = 0;
		/*During hang recovery, send data is not allowed.
		* but we still need to send cmd to cp2
		*/
		if (tx_msg->hang_recovery_status == HANG_RECOVERY_BEGIN) {
			if (sprdwl_msg_tx_pended(&tx_msg->tx_list_cmd))
				sprdwl_tx_cmd(intf, &tx_msg->tx_list_cmd);
			usleep_range(50, 100);
			continue;
		}

		if (tx_msg->thermal_status == THERMAL_WIFI_DOWN) {
			if (sprdwl_msg_tx_pended(&tx_msg->tx_list_cmd))
				sprdwl_tx_cmd(intf, &tx_msg->tx_list_cmd);
			usleep_range(100, 200);
			continue;
		}

		if (tx_msg->thermal_status == THERMAL_TX_STOP) {
			if (sprdwl_msg_tx_pended(&tx_msg->tx_list_cmd))
				sprdwl_tx_cmd(intf, &tx_msg->tx_list_cmd);
			usleep_range(50, 100);
			continue;
		}

		if (sprdwl_msg_tx_pended(&tx_msg->tx_list_cmd))
			sprdwl_tx_cmd(intf, &tx_msg->tx_list_cmd);

		/* if tx list, send wakeup firstly */
		if (intf->fw_power_down == 1 &&
			(atomic_read(&tx_msg->tx_list_qos_pool.ref) > 0 ||
			 !list_empty(&tx_msg->xmit_msg_list.to_send_list) ||
			 !list_empty(&tx_msg->xmit_msg_list.to_free_list))) {
				struct sprdwl_vif *vif;

				sprdwcn_bus_sleep_wakeup(WIFI);
				vif = mode_to_vif(priv, tx_msg->mode);
				intf->fw_power_down = 0;
				sprdwl_work_host_wakeup_fw(vif);
				sprdwl_put_vif(vif);
				continue;
		}

		if (intf->fw_awake == 0 ||
			intf->suspend_mode != SPRDWL_PS_RESUMED) {
			usleep_range(10, 20);
			continue;
		}

		if (!list_empty(&tx_msg->xmit_msg_list.to_send_list)) {
			if (sprdwl_handle_to_send_list(intf, tx_msg->xmit_msg_list.mode)) {
				usleep_range(590, 610);
				continue;
			}
		}

		for (mode = SPRDWL_MODE_NONE; mode < SPRDWL_MODE_MAX; mode++) {
			int num = atomic_read(&tx_msg->tx_list[mode]->mode_list_num);

			if (num <= 0)
				continue;
			if (num > 0 && priv->fw_stat[mode] != SPRDWL_INTF_OPEN) {
				sprdwl_flush_mode_txlist(tx_msg, mode);
				continue;
			}

			send_num = sprdwl_fc_test_send_num(tx_msg, mode, num);
			if (send_num > 0)
				sprdwl_tx_eachmode_data(intf, mode);
			else
				need_polling |= (1 << (u8)mode);
		}

		if (need_polling) {
			/* retry to wait create */
			usleep_range(5, 10);
		}
	}

	wl_err("%s no longer exsit, flush data, return!\n", __func__);
	sprdwl_flush_all_txlist(tx_msg);

	return 0;
}

int sprdwl_tx_init(struct sprdwl_intf *intf)
{
	int ret = 0;
	u8 i, j;
	struct sprdwl_tx_msg *tx_msg = NULL;

	tx_msg = kzalloc(sizeof(struct sprdwl_tx_msg), GFP_KERNEL);
	if (!tx_msg) {
		ret = -ENOMEM;
		wl_err("%s kzalloc failed!\n", __func__);
		goto exit;
	}

	spin_lock_init(&tx_msg->lock);/*useless?*/
	tx_msg->cmd_timeout = msecs_to_jiffies(SPRDWL_TX_CMD_TIMEOUT);
	tx_msg->data_timeout = msecs_to_jiffies(SPRDWL_TX_DATA_TIMEOUT);
	atomic_set(&tx_msg->flow0, 0);
	atomic_set(&tx_msg->flow1, 0);
	atomic_set(&tx_msg->flow2, 0);

	ret = sprdwl_msg_init(SPRDWL_TX_MSG_CMD_NUM, &tx_msg->tx_list_cmd);
	if (ret) {
		wl_err("%s tx_list_cmd alloc failed\n", __func__);
		goto err_tx_work;
	}

	ret = sprdwl_msg_init(SPRDWL_TX_QOS_POOL_SIZE,
				  &tx_msg->tx_list_qos_pool);
	if (ret) {
		wl_err("%s tx_list_qos_pool alloc failed\n", __func__);
		goto err_tx_list_cmd;
	}

	for (i = 0; i < SPRDWL_MODE_MAX; i++) {
		tx_msg->tx_list[i] = kzalloc(sizeof(struct tx_t), GFP_KERNEL);
		if (!tx_msg->tx_list[i])
			goto err_txlist;
		qos_init(tx_msg->tx_list[i]);
	}
	sprdwl_init_xmit_list(tx_msg);

	tx_msg->tx_thread =
		kthread_create(sprdwl_tx_work_queue,
				   (void *)tx_msg, "SPRDWL_TX_QUEUE");
	if (!tx_msg->tx_thread) {
		wl_err("%s SPRDWL_TX_QUEUE create failed", __func__);
		ret = -ENOMEM;
		goto err_txlist;
	}

	intf->sprdwl_tx = (void *)tx_msg;
	tx_msg->intf = intf;

	init_completion(&tx_msg->tx_completed);
	wake_up_process(tx_msg->tx_thread);

#ifdef WMMAC_WFA_CERTIFICATION
	reset_wmmac_parameters(tx_msg->intf->priv);
	reset_wmmac_ts_info();
#endif

	for (i = 0; i < MAX_COLOR_BIT; i++) {
		tx_msg->flow_ctrl[i].mode = SPRDWL_MODE_NONE;
		tx_msg->flow_ctrl[i].color_bit = i;
		atomic_set(&tx_msg->flow_ctrl[i].flow, 0);
	}

	tx_msg->hang_recovery_status = HANG_RECOVERY_END;

	return ret;

err_txlist:
	for (j = 0; j < i; j++)
		kfree(tx_msg->tx_list[j]);

	sprdwl_msg_deinit(&tx_msg->tx_list_qos_pool);
err_tx_list_cmd:
	sprdwl_msg_deinit(&tx_msg->tx_list_cmd);
err_tx_work:
	kfree(tx_msg);
exit:
	return ret;
}

void sprdwl_tx_deinit(struct sprdwl_intf *intf)
{
	struct sprdwl_tx_msg *tx_msg = NULL;
	u8 i;

	tx_msg = (void *)intf->sprdwl_tx;

	/*let tx work queue exit*/
	intf->exit = 1;
	if (tx_msg->tx_thread) {
		tx_up(tx_msg);
		kthread_stop(tx_msg->tx_thread);
		tx_msg->tx_thread = NULL;
	}

	/*need to check if there is some data and cmdpending
	*or sending by HIF, and wait until tx complete and freed
	*/
	if (!list_empty(&tx_msg->tx_list_cmd.cmd_to_free))
		wl_err("%s cmd not yet transmited, cmd_send:%d, cmd_poped:%d\n",
			   __func__, tx_msg->cmd_send, tx_msg->cmd_poped);

	sprdwl_flush_all_txlist(tx_msg);

	sprdwl_msg_deinit(&tx_msg->tx_list_cmd);
	sprdwl_msg_deinit(&tx_msg->tx_list_qos_pool);
	for (i = 0; i < SPRDWL_MODE_MAX; i++)
		kfree(tx_msg->tx_list[i]);
	kfree(tx_msg);
	intf->sprdwl_tx = NULL;
}

static inline unsigned short from32to16(unsigned int x)
{
	/* add up 16-bit and 16-bit for 16+c bit */
	x = (x & 0xffff) + (x >> 16);
	/* add up carry.. */
	x = (x & 0xffff) + (x >> 16);
	return x;
}

unsigned int do_csum(const unsigned char *buff, int len)
{
	int odd;
	unsigned int result = 0;

	if (len <= 0)
		goto out;
	odd = 1 & (unsigned long) buff;
	if (odd) {
#ifdef __LITTLE_ENDIAN
		result += (*buff << 8);
#else
		result = *buff;
#endif
		len--;
		buff++;
	}
	if (len >= 2) {
		if (2 & (unsigned long) buff) {
			result += *(unsigned short *) buff;
			len -= 2;
			buff += 2;
		}
		if (len >= 4) {
			const unsigned char *end = buff + ((unsigned)len & ~3);
			unsigned int carry = 0;
			do {
				unsigned int w = *(unsigned int *) buff;
				buff += 4;
				result += carry;
				result += w;
				carry = (w > result);
			} while (buff < end);
			result += carry;
			result = (result & 0xffff) + (result >> 16);
		}
		if (len & 2) {
			result += *(unsigned short *) buff;
			buff += 2;
		}
	}
	if (len & 1)
#ifdef __LITTLE_ENDIAN
		result += *buff;
#else
		result += (*buff << 8);
#endif
	result = from32to16(result);
	if (odd)
		result = ((result >> 8) & 0xff) | ((result & 0xff) << 8);
out:
	return result;
}

static int is_multicast_mac_addr(const u8 *addr)
{
	return ((addr[0] != 0xff) && (0x01 & addr[0]));
}

static int sprdwl_mc_pkt_checksum(struct sk_buff *skb, struct net_device *ndev)
{
	struct udphdr *udphdr;
	struct tcphdr *tcphdr;
	struct ipv6hdr *ipv6hdr;
	__sum16 checksum = 0;
	unsigned char iphdrlen = 0;
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf;

	vif = netdev_priv(ndev);
	intf = (struct sprdwl_intf *)vif->priv->hw_priv;
	ipv6hdr = (struct ipv6hdr *)(skb->data + ETHER_HDR_LEN);
	iphdrlen = sizeof(*ipv6hdr);

	udphdr = (struct udphdr *)(skb->data + ETHER_HDR_LEN + iphdrlen);
	tcphdr = (struct tcphdr *)(skb->data + ETHER_HDR_LEN + iphdrlen);

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		checksum =
			(__force __sum16)do_csum(
			skb->data + ETHER_HDR_LEN + iphdrlen,
			skb->len - ETHER_HDR_LEN - iphdrlen);
		if (ipv6hdr->nexthdr == IPPROTO_UDP) {
			udphdr->check = ~checksum;
			wl_info("csum:%x,udp check:%x\n",
				checksum, udphdr->check);
		} else if (ipv6hdr->nexthdr == IPPROTO_TCP) {
			tcphdr->check = ~checksum;
			wl_info("csum:%x,tcp check:%x\n",
				checksum, tcphdr->check);
		} else
			return 1;
		skb->ip_summed = CHECKSUM_NONE;
		return 0;
	}
	return 1;
}

int sprdwl_tx_mc_pkt(struct sk_buff *skb, struct net_device *ndev)
{
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf;

	vif = netdev_priv(ndev);
	intf = (struct sprdwl_intf *)vif->priv->hw_priv;

	intf->skb_da = skb->data;
	if (intf->skb_da == NULL)/*TODO*/
		return 1;

	if (is_multicast_mac_addr(intf->skb_da) && vif->mode == SPRDWL_MODE_AP) {
		wl_debug("%s,AP mode, multicast bssid: %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
			 intf->skb_da[0], intf->skb_da[1], intf->skb_da[2],
			 intf->skb_da[3], intf->skb_da[4], intf->skb_da[5]);
		sprdwl_mc_pkt_checksum(skb, ndev);
		sprdwl_xmit_data2cmd_wq(skb, ndev);
		return NETDEV_TX_OK;
	}
	return 1;

}

bool is_vowifi_pkt(struct sk_buff *skb, bool *b_cmd_path)
{
	bool ret = false;
	u8 dscp = 0;
	struct ethhdr *ethhdr = (struct ethhdr *)skb->data;
	unsigned char iphdrlen = 0;
	struct iphdr *iphdr = NULL;
	struct udphdr *udphdr;

	if (ethhdr->h_proto != htons(ETH_P_IP))
		return false;

	iphdr = (struct iphdr *)(skb->data + ETHER_HDR_LEN);

	if (iphdr->protocol != IPPROTO_UDP)
		return false;

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0)
	iphdrlen = ip_hdrlen(skb);
#else
	iphdrlen = iphdr->ihl * 4;
#endif
	udphdr = (struct udphdr *)(skb->data + ETHER_HDR_LEN + iphdrlen);
	dscp = (iphdr->tos >> 2);
	switch (dscp) {
	case VOWIFI_IKE_DSCP:
		if ((udphdr->dest == htons(VOWIFI_IKE_SIP_PORT)) ||
			(udphdr->dest == htons(VOWIFI_IKE_ONLY_PORT))) {
			ret = true;
			(*b_cmd_path) = true;
		}
		break;
	case VOWIFI_SIP_DSCP:
		if (udphdr->dest == htons(VOWIFI_IKE_SIP_PORT)) {
			ret = true;
			(*b_cmd_path) = true;
		}
		break;
	case VOWIFI_VIDEO_DSCP:
	case VOWIFI_AUDIO_DSCP:
		ret = true;
		(*b_cmd_path) = false;
		break;
	default:
		ret = false;
		(*b_cmd_path) = false;
		break;
	}

	return ret;
}

int sprdwl_tx_filter_ip_pkt(struct sk_buff *skb, struct net_device *ndev)
{
	bool is_data2cmd;
	bool is_ipv4_dhcp, is_ipv6_dhcp;
	bool is_vowifi2cmd;
	unsigned char *dhcpdata = NULL;
	struct udphdr *udphdr;
	struct iphdr *iphdr = NULL;
	struct ipv6hdr *ipv6hdr;
	__sum16 checksum = 0;
	struct ethhdr *ethhdr = (struct ethhdr *)skb->data;
	unsigned char iphdrlen = 0;
	unsigned char lut_index;
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf;

	vif = netdev_priv(ndev);
	intf = (struct sprdwl_intf *)vif->priv->hw_priv;

	if (ethhdr->h_proto == htons(ETH_P_IPV6)) {
		ipv6hdr = (struct ipv6hdr *)(skb->data + ETHER_HDR_LEN);
		/* check for udp header */
		if (ipv6hdr->nexthdr != IPPROTO_UDP)
			return 1;
		iphdrlen = sizeof(*ipv6hdr);
	} else if (ethhdr->h_proto == htons(ETH_P_IP)) {
		iphdr = (struct iphdr *)(skb->data + ETHER_HDR_LEN);
		/* check for udp header */
		if (iphdr->protocol != IPPROTO_UDP)
			return 1;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0)
		iphdrlen = ip_hdrlen(skb);
#else
		iphdrlen = iphdr->ihl * 4;
#endif
	} else {
		return 1;
	}

	udphdr = (struct udphdr *)(skb->data + ETHER_HDR_LEN + iphdrlen);

	is_ipv4_dhcp =
	((ethhdr->h_proto == htons(ETH_P_IP)) &&
	((udphdr->source == htons(DHCP_SERVER_PORT)) ||
	(udphdr->source == htons(DHCP_CLIENT_PORT))));
	is_ipv6_dhcp =
	((ethhdr->h_proto == htons(ETH_P_IPV6)) &&
	((udphdr->source == htons(DHCP_SERVER_PORT_IPV6)) ||
	(udphdr->source == htons(DHCP_CLIENT_PORT_IPV6))));

	if (is_vowifi_pkt(skb, &is_vowifi2cmd)) {
		if (is_vowifi2cmd == false) {
			struct sprdwl_peer_entry *peer_entry = NULL;

			lut_index = sprdwl_find_lut_index(intf, vif);
			peer_entry = &intf->peer_entry[lut_index];
			if (peer_entry->vowifi_enabled == 1) {
				if (peer_entry->vowifi_pkt_cnt < 11)
					peer_entry->vowifi_pkt_cnt++;
				if (peer_entry->vowifi_pkt_cnt == 10)
					sprdwl_vowifi_data_protection(vif);
			}
		}
	} else {
		is_vowifi2cmd = false;
	}

	is_data2cmd = (is_ipv4_dhcp || is_ipv6_dhcp || is_vowifi2cmd);

	if (is_ipv4_dhcp) {
		intf->skb_da = skb->data;
		lut_index = sprdwl_find_lut_index(intf, vif);
		dhcpdata = skb->data + ETHER_HDR_LEN + iphdrlen + 250;
		if (*dhcpdata == 0x01) {
			wl_info("DHCP: TX DISCOVER\n");
		} else if (*dhcpdata == 0x02) {
			wl_info("DHCP: TX OFFER\n");
		} else if (*dhcpdata == 0x03) {
			wl_info("DHCP: TX REQUEST\n");
			intf->peer_entry[lut_index].ip_acquired = 1;
			if (sprdwl_is_group(skb->data))
				intf->peer_entry[lut_index].ba_tx_done_map = 0;
		} else if (*dhcpdata == 0x04) {
			wl_info("DHCP: TX DECLINE\n");
		} else if (*dhcpdata == 0x05) {
			wl_info("DHCP: TX ACK\n");
			intf->peer_entry[lut_index].ip_acquired = 1;
		} else if (*dhcpdata == 0x06) {
			wl_info("DHCP: TX NACK\n");
		}
	}

	/*as CP request, send data with CMD*/
	if (is_data2cmd) {
		if (is_ipv4_dhcp || is_ipv6_dhcp)
			wl_debug("dhcp,check:%x,skb->ip_summed:%d\n",
				udphdr->check, skb->ip_summed);
		if (is_vowifi2cmd) {
			iphdr = (struct iphdr *)(skb->data + ETHER_HDR_LEN);
			wl_info("vowifi, proto=0x%x, tos=0x%x, dest=0x%x\n",
				ethhdr->h_proto, iphdr->tos, udphdr->dest);
		}

		if (skb->ip_summed == CHECKSUM_PARTIAL) {
			checksum =
				(__force __sum16)do_csum(
				skb->data + ETHER_HDR_LEN + iphdrlen,
				skb->len - ETHER_HDR_LEN - iphdrlen);
			udphdr->check = ~checksum;
			wl_debug("dhcp,csum:%x,check:%x\n",
				checksum, udphdr->check);
			skb->ip_summed = CHECKSUM_NONE;
		}

		sprdwl_xmit_data2cmd_wq(skb, ndev);
		return NETDEV_TX_OK;
	}

	return 1;
}

int sprdwl_tx_filter_packet(struct sk_buff *skb, struct net_device *ndev)
{
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf;
	struct ethhdr *ethhdr = (struct ethhdr *)skb->data;
	unsigned char lut_index;

	vif = netdev_priv(ndev);
	intf = (struct sprdwl_intf *)vif->priv->hw_priv;

#if defined(MORE_DEBUG)
	if (ethhdr->h_proto == htons(ETH_P_ARP))
		intf->stats.tx_arp_num++;
	if (sprdwl_is_group(skb->data))
		intf->stats.tx_multicast++;
#endif

	if (ethhdr->h_proto == htons(ETH_P_ARP)) {
		wl_debug("incoming ARP packet\n");
		sprdwl_xmit_data2cmd_wq(skb, ndev);
		return NETDEV_TX_OK;
	}
	if (ethhdr->h_proto == htons(ETH_P_TDLS))
		wl_info("incoming TDLS packet\n");
	if (ethhdr->h_proto == htons(ETH_P_PREAUTH))
		wl_info("incoming PREAUTH packet\n");

	intf->skb_da = skb->data;
	if (ethhdr->h_proto == htons(ETH_P_IPV6)) {
		lut_index = sprdwl_find_lut_index(intf, vif);
		if ((vif->mode == SPRDWL_MODE_AP || vif->mode == SPRDWL_MODE_P2P_GO) &&
			(lut_index != 4) && intf->peer_entry[lut_index].ip_acquired == 0) {
			wl_debug("ipv6 ethhdr->h_proto=%x\n", ethhdr->h_proto);
			dev_kfree_skb(skb);
			return 0;
		}
	}

	if ((ethhdr->h_proto == htons(ETH_P_IPV6)) &&
		!sprdwl_tx_mc_pkt(skb, ndev))
		return NETDEV_TX_OK;

	if (ethhdr->h_proto == htons(ETH_P_IP) ||
		ethhdr->h_proto == htons(ETH_P_IPV6))
		return sprdwl_tx_filter_ip_pkt(skb, ndev);
	return 1;
}
