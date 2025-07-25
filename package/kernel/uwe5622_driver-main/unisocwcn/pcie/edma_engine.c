/*
 * Copyright (C) 2016-2018 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <wcn_bus.h>

#include "edma_engine.h"
#include "mchn.h"
#include "pcie_dbg.h"
#include "pcie.h"

#define TX 1
#define RX 0

static int hisrfunc_debug;
static int hisrfunc_line;
static int hisrfunc_last_msg;
static struct edma_info g_edma = { 0 };

static unsigned char *mpool_buffer;
static struct dma_buf mpool_dm = {0};

int time_sub_us(struct timeval *start, struct timeval *end)
{
	return (end->tv_sec - start->tv_sec)*1000000 +
		(end->tv_usec - start->tv_usec);
}

void *mpool_vir_to_phy(void *p)
{
	unsigned long offset;

	offset = (unsigned long)p - mpool_dm.vir;
	return (void *)(mpool_dm.phy + offset);
}

void *mpool_phy_to_vir(void *p)
{
	unsigned long offset;

	offset = (unsigned long)p - mpool_dm.phy;
	return (void *)(mpool_dm.vir + offset);
}

void *mpool_malloc(int len)
{
	int ret;
	unsigned char *p;
	struct edma_info *edma = edma_info();

	if (mpool_buffer == NULL) {
		ret = dmalloc(edma->pcie_info, &mpool_dm, 0x8000);
		if (ret != 0)
			return NULL;
		mpool_buffer = (unsigned char *)(mpool_dm.vir);
		PCIE_INFO("%s {0x%lx,0x%lx} -- {0x%lx,0x%lx}\n",
			  __func__, mpool_dm.vir, mpool_dm.phy,
			  mpool_dm.vir + 0x20000,
			  mpool_dm.phy + 0x20000);
	}
	if (len <= 0)
		return NULL;
	p = mpool_buffer;
	memset(p, 0x56, len);
	mpool_buffer += len;
	PCIE_INFO("%s(%d) = {0x%p, 0x%p}\n", __func__, len, p,
		  mpool_vir_to_phy((void *)p));
	return p;
}

int mpool_free(void)
{
	struct edma_info *edma = edma_info();

	if ((mpool_dm.vir != 0) && (mpool_dm.phy != 0))
		dmfree(edma->pcie_info, &mpool_dm);
	return 0;
}

static int create_wcnevent(struct event_t *event, int id)
{
	PCIE_INFO("create event(0x%p)[+]\n", event);
	memset((unsigned char *)event, 0x00, sizeof(struct event_t));
	sema_init(&(event->wait_sem), 0);

	return 0;
}

static int wait_wcnevent(struct event_t *event, int timeout)
{
	if (timeout < 0) {
		int dt;
		struct timeval time;
		struct timespec now;

		getnstimeofday(&now);
		time.tv_sec = now.tv_sec;
		time.tv_usec = now.tv_nsec/1000;
		if (event->wait_sem.count == 0) {
			if (event->flag == 0) {
				event->flag = 1;
				event->time = time;
				return 0;
			}
			dt = time_sub_us(&(event->time), &time);
			if (dt < 200)
				return 0;
			event->flag = 0;
		} else
			event->flag = 0;
		down(&event->wait_sem);
		return 0;
	}
	return down_timeout(&(event->wait_sem), msecs_to_jiffies(timeout));
}

static int set_wcnevent(struct event_t *event)
{
	if (event->tasklet != NULL)
		tasklet_schedule(event->tasklet);
	else
		up(&(event->wait_sem));
	return 0;
}

static int edma_spin_lock_init(struct irq_lock_t *lock)
{
	lock->irq_spinlock_p = kmalloc(sizeof(spinlock_t), GFP_KERNEL);
	lock->flag = 0;
	spin_lock_init(lock->irq_spinlock_p);

	return 0;
}

void *pcie_alloc_memory(int len)
{
	int ret;
	unsigned char *p;
	struct edma_info *edma = edma_info();

	if (mpool_buffer == NULL) {
		ret = dmalloc(edma->pcie_info, &mpool_dm, 0x8000);
		if (ret != 0)
			return NULL;
		mpool_buffer = (unsigned char *)(mpool_dm.vir);
		PCIE_INFO("%s {0x%lx,0x%lx} -- {0x%lx,0x%lx}\n",
			  __func__, mpool_dm.vir, mpool_dm.phy,
			  mpool_dm.vir + 0x20000,
			  mpool_dm.phy + 0x20000);
	}
	if (len <= 0)
		return NULL;
	p = mpool_buffer;
	memset(p, 0x56, len);
	mpool_buffer += len;
	PCIE_INFO("%s(%d) = {0x%p, 0x%p}\n", __func__, len, p,
			    mpool_vir_to_phy((void *)p));

	return p;
}

struct edma_info *edma_info(void)
{
	return &g_edma;
}

static int create_queue(struct msg_q *q, int size, int num)
{
	int ret;

	PCIE_INFO("[+]%s(0x%p, %d, %d)\n", __func__,
		     (void *)virt_to_phys((void *)(q)), size, num);
	q->mem = mpool_malloc(size * num);
	if (q->mem == NULL) {
		PCIE_INFO("%s malloc err\n", __func__);
		return ERROR;
	}

	ret = edma_spin_lock_init(&(q->lock));
	if (ret) {
		PCIE_INFO("%s spin_lock_init err\n", __func__);
		return ERROR;
	}
	ret = create_wcnevent(&(q->event), 0);
	if (ret != 0) {
		PCIE_INFO("%s event_create err\n", __func__);
		return ERROR;
	}
	q->wt = 0;
	q->rd = 0;
	q->max = num;
	q->size = size;
	PCIE_INFO("[-]%s\n", __func__);

	return OK;
}

int delete_queue(struct msg_q *q)
{
	PCIE_INFO("[+]%s\n", __func__);
	kfree(q->mem);
	memset((unsigned char *)(q), 0x00, sizeof(struct msg_q));
	PCIE_INFO("[-]%s\n", __func__);

	return OK;
}

static int dequeue(struct msg_q *q, unsigned char *msg, int timeout)
{
	if (q->wt == q->rd)
		return ERROR;

	/* spin_lock_irqsave */
	spin_lock_irqsave(q->lock.irq_spinlock_p, q->lock.flag);
	memcpy(msg, q->mem + (q->size) * (q->rd), q->size);
	q->rd = INCR_RING_BUFF_INDX(q->rd, q->max);
	/* spin_unlock_irqrestore */
	spin_unlock_irqrestore(q->lock.irq_spinlock_p, q->lock.flag);

	return OK;
}

static int enqueue(struct msg_q *q, unsigned char *msg)
{
	if ((q->wt + 1) % (q->max) == q->rd) {
		PCIE_INFO("%s full\n", __func__);
		return ERROR;
	}
	q->seq++;
	*((unsigned int *)msg) = q->seq;
	memcpy((q->mem + (q->size) * (q->wt)), msg, q->size);
	q->wt = INCR_RING_BUFF_INDX(q->wt, q->max);

	return OK;
}

static int dscr_polling(struct desc *dscr, int loop)
{
	do {
		if (dscr->chn_trans_len.bit.rf_chn_done)
			return 0;
	} while (loop--);

	return -1;
}

static int edma_hw_next_dscr(int chn, int inout, struct desc **next)
{
	int i;
	unsigned int ptr_l[2];
	struct desc *hw_next = NULL;
	union dma_dscr_ptr_high_reg local_chn_ptr_high;
	struct edma_info *edma = edma_info();

	if (inout == TX) {
		local_chn_ptr_high.reg =
		    edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg;
		SET_8_OF_40(hw_next,
			(local_chn_ptr_high.bit.rf_chn_tx_next_dscr_ptr_high &
			    (~(1 << 7))));
		SET_32_OF_40(hw_next,
		edma->dma_chn_reg[chn].dma_dscr.rf_chn_tx_next_dscr_ptr_low);
	} else {
		for (i = 0; i < 5; i++) {
			local_chn_ptr_high.reg =
			    edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg;
			SET_8_OF_40(hw_next,
			 (local_chn_ptr_high.bit.rf_chn_rx_next_dscr_ptr_high &
				     (~(1 << 7))));
			if (local_chn_ptr_high.bit
			    .rf_chn_rx_next_dscr_ptr_high) {
			}

			ptr_l[0] = edma->dma_chn_reg[chn]
				   .dma_dscr.rf_chn_rx_next_dscr_ptr_low;
			if ((ptr_l[0] == 0) || (ptr_l[0] == 0xFFFFFFFF)) {
				udelay(1);
				ptr_l[1] =
				    edma->dma_chn_reg[chn].dma_dscr
						.rf_chn_rx_next_dscr_ptr_low;
				PCIE_INFO(
				    "%s(%d,%d) err hw_next:0x%p, 0x%x, 0x%x\n",
					__func__, chn, inout, hw_next,
					ptr_l[0], ptr_l[1]);
			} else {
				SET_32_OF_40(hw_next, ptr_l[0]);
				break;
			}
		}
		if (i == 5) {
			PCIE_ERR(
				"%s(%d,%d) timeout hw_next:0x%p, 0x%x, 0x%x\n",
				__func__, chn, inout, hw_next,
				ptr_l[0], ptr_l[1]);
		}

	}
	*next = hw_next;
	if (hw_next == NULL) {
		PCIE_ERR("%s(%d, %d) err, hw_next == NULL\n",
			 __func__, chn, inout);
	}

	return 0;
}

int edma_sw_link_done_dscr(struct desc *head, struct desc **tail)
{
	struct desc *dscr;

	for (dscr = head;;) {
		if (dscr->chn_trans_len.bit.rf_chn_done == 0)
			break;
		dscr = dscr->next.p;
	}
	*tail = dscr;

	if (dscr == head)
		return ERROR;

	return 0;
}

static int dscr_ring_empty(int chn)
{
	struct desc *dscr;
	struct edma_info *edma = edma_info();

	edma_hw_next_dscr(chn, edma->chn_sw[chn].inout, &dscr);
	dscr = mpool_phy_to_vir(dscr);
	if (dscr == edma->chn_sw[chn].dscr_ring.head)
		return ERROR;
	return OK;
}

static int dscr_zero(struct desc *dscr)
{
	dscr->chn_trans_len.reg = 0;
	dscr->chn_trans_len.bit.rf_chn_tx_intr = 0;
	dscr->chn_trans_len.bit.rf_chn_rx_intr = 0;
	dscr->chn_ptr_high.bit.rf_chn_src_data_addr_high = 0xFF;
	dscr->chn_ptr_high.bit.rf_chn_dst_data_addr_high = 0xFF;
	dscr->rf_chn_data_src_addr_low = 0xFFFFFFFF;
	dscr->rf_chn_data_dst_addr_low = 0xFFFFFFFF;

	return 0;
}

static int dscr_link_mbuf(int inout, struct desc *dscr, struct mbuf_t *mbuf)
{
	if (inout == TX) {
		dscr->rf_chn_data_src_addr_low =
					(unsigned int)(mbuf->phy & 0xFFFFFFFF);
		dscr->chn_ptr_high.bit.rf_chn_src_data_addr_high =
							GET_8_OF_40(mbuf->phy);
		dscr->chn_trans_len.bit.rf_chn_trsc_len =
						(unsigned short)(mbuf->len);
	} else {
		dscr->rf_chn_data_dst_addr_low =
					(unsigned int)(mbuf->phy & 0xFFFFFFFF);
		dscr->chn_ptr_high.bit.rf_chn_dst_data_addr_high =
							GET_8_OF_40(mbuf->phy);
		dscr->chn_trans_len.bit.rf_chn_trsc_len = 0;
	}
	dscr->link.p = mbuf;

	return 0;
}

int dscr_link_cpdu(int inout, struct desc *dscr, struct cpdu_head *cpdu)
{
	unsigned char *buf = (unsigned char *)cpdu + sizeof(struct cpdu_head);
	int len = cpdu->len;

	if (inout == TX) {
		dscr->rf_chn_data_src_addr_low = GET_32_OF_40((buf));
		dscr->chn_ptr_high.bit.rf_chn_src_data_addr_high =
							GET_8_OF_40((buf));
		dscr->chn_trans_len.bit.rf_chn_trsc_len = (unsigned short)(len);
	} else {
		dscr->rf_chn_data_dst_addr_low = GET_32_OF_40((buf));
		dscr->chn_ptr_high.bit.rf_chn_dst_data_addr_high =
							GET_8_OF_40((buf));
		dscr->chn_trans_len.bit.rf_chn_trsc_len = 0;
	}
	dscr->link.p = cpdu;

	return 0;
}

static int edma_pop_link(int chn, struct desc *__head, struct desc *__tail,
		  void **head__, void **tail__, int *node)
{
	struct mbuf_t *mbuf = NULL;
	struct desc *dscr = __head;
	struct edma_info *edma = edma_info();

	if ((__head == NULL) || (__tail == NULL)) {
		PCIE_ERR("[+]%s(%d) dscr(0x%p--0x%p)\n", __func__,
			 chn, __head, __tail);
	}
	*head__ = *tail__ = NULL;
	(*node) = 0;
	spin_lock_irqsave(edma->chn_sw[chn].dscr_ring.lock.irq_spinlock_p,
			edma->chn_sw[chn].dscr_ring.lock.flag);
	do {
		if (dscr == NULL) {
			PCIE_ERR("%s(0x%p, 0x%p) dscr=NULL, error\n",
				 __func__, __head, __tail);
			spin_unlock_irqrestore(edma->chn_sw[chn].dscr_ring.lock
					       .irq_spinlock_p,
					       edma->chn_sw[chn].dscr_ring.lock
					       .flag);
			return -1;
		}
		if (dscr_polling(dscr, 500000)) {
			PCIE_ERR("%s(%d, 0x%p, 0x%p, 0x%p) polling err\n",
				 __func__, chn, __head, __tail, dscr);
			spin_unlock_irqrestore(edma->chn_sw[chn].dscr_ring.lock
					       .irq_spinlock_p,
					       edma->chn_sw[chn].dscr_ring.lock
					       .flag);
			return -1;
		}

		if (*head__) {
			mbuf->next = dscr->link.p;
			mbuf = mbuf->next;
		} else {
			mbuf = *head__ = dscr->link.p;
		}

		if (!mbuf) {
			PCIE_ERR("%s line:%d err\n", __func__, __LINE__);
			spin_unlock_irqrestore(edma->chn_sw[chn].dscr_ring.lock
					       .irq_spinlock_p,
					       edma->chn_sw[chn].dscr_ring.lock
					       .flag);

			return -1;
		}
		mbuf->len = dscr->chn_trans_len.bit.rf_chn_trsc_len;

		(*node)++;
		edma->chn_sw[chn].dscr_ring.head =
		    edma->chn_sw[chn].dscr_ring.head->next.p;
		edma->chn_sw[chn].dscr_ring.free++;
		dscr_zero(dscr);
		if (COMPARE_40_BIT(dscr->next.p, __tail))
			break;
		dscr = dscr->next.p;
	} while (1);

	mbuf->next = NULL;
	*tail__ = mbuf;

	spin_unlock_irqrestore(edma->chn_sw[chn].dscr_ring.lock.irq_spinlock_p,
					edma->chn_sw[chn].dscr_ring.lock.flag);

	return 0;
}

static int edma_hw_tx_req(int chn)
{
	struct edma_info *edma = edma_info();

	edma->dma_chn_reg[chn].dma_tx_req.reg = 1;

	return 0;
}

static int edma_hw_rx_req(int chn)
{
	struct edma_info *edma = edma_info();
	union dma_chn_rx_req_reg local_dma_rx_req;

	local_dma_rx_req.reg = edma->dma_chn_reg[chn].dma_rx_req.reg;
	local_dma_rx_req.bit.rf_chn_rx_req = 1;

	edma->dma_chn_reg[chn].dma_rx_req.reg = local_dma_rx_req.reg;

	return 0;
}

#ifdef __FOR_THREADX_H__
int edma_one_link_dscr_buf_bind(struct desc *dscr, unsigned char *dst,
				unsigned char *src, unsigned short len)
{
	addr_t dst__ = { 0 }, src__ = {
	0};
	struct edma_info *edma = edma_info();
	unsigned int tmp[2];

	PCIE_INFO("[+]%s(0x%x, 0x%x, 0x%x, %d)\n", __func__, dscr, dst,
		  src, len);

	AHB32_AXI40(&dst__, dst);
	AHB32_AXI40(&src__, src);

	tmp[0] = src__.l;
	tmp[1] = dst__.l;

	dscr->chn_trans_len.bit.rf_chn_trsc_len = len;
	dscr->chn_trans_len.bit.rf_chn_tx_intr = 0;

	memcpy((unsigned char *)(&(dscr->rf_chn_data_src_addr_low)),
	       (unsigned char *)(&tmp[0]), 4);
	memcpy((unsigned char *)(&(dscr->rf_chn_data_dst_addr_low)),
	       (unsigned char *)(&tmp[1]), 4);

	dscr->chn_ptr_high.bit.rf_chn_src_data_addr_high = src__.h;
	dscr->chn_ptr_high.bit.rf_chn_dst_data_addr_high = dst__.h;
	dscr->chn_trans_len.bit.rf_chn_eof = 0;

	if (sizeof(unsigned long) == sizeof(unsigned int)) {
		memcpy((unsigned char *)(&dscr->link.src),
		       (unsigned char *)(&src), 4);
		memcpy((unsigned char *)(&dscr->buf.dst),
		       (unsigned char *)(&dst), 4);
	} else {
		memcpy((unsigned char *)(&dscr->link.src),
		       (unsigned char *)(&src), 8);
		memcpy((unsigned char *)(&dscr->buf.dst),
		       (unsigned char *)(&dst), 8);
	}

	PCIE_INFO("[-]%s\n", __func__);

	return 0;
}

int edma_one_link_copy(int chn, struct desc *head, struct desc *tail, int num)
{
	union dma_chn_cfg_reg dma_cfg = { 0 };
	struct edma_info *edma = edma_info();
	union dma_dscr_ptr_high_reg local_chn_ptr_high;

	PCIE_INFO("[+]%s(%d, 0x%x, 0x%x, %d)\n", __func__, chn, head,
		  tail, num);
	tail->chn_trans_len.bit.rf_chn_eof = 1;

	dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
	local_chn_ptr_high.reg =
	    edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg;
	local_chn_ptr_high.bit.rf_chn_tx_next_dscr_ptr_high = GET_8_OF_40(head);
	dma_cfg.bit.rf_chn_en = 1;

	edma->dma_chn_reg[chn].dma_dscr.rf_chn_tx_next_dscr_ptr_low =
	    GET_32_OF_40((unsigned char *)(head));
	edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg =
	    local_chn_ptr_high.reg;
	edma->dma_chn_reg[chn].dma_cfg.reg = dma_cfg.reg;

	edma_hw_tx_req(chn);
	PCIE_INFO("[-]%s\n", __func__);

	return 0;
}

int edma_none_link_copy(int chn, addr_t *dst, addr_t *src, unsigned short len,
			int timeout)
{
	union dma_chn_cfg_reg dma_cfg = { 0 };
	union dma_dscr_trans_len_reg chn_trans_len = { 0 };
	union dma_dscr_ptr_high_reg chn_ptr_high = { 0 };
	struct edma_info *edma = edma_info();

	PCIE_INFO("[+]%s(%d, {0x%x,0x%x}, {0x%x,0x%x}, %d)\n",
		  __func__, chn, dst->h, dst->l, src->h, src->l, len);

	dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
	chn_trans_len.reg = edma->dma_chn_reg[chn].dma_dscr.chn_trans_len.reg;
	chn_ptr_high.reg = edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg;

	dma_cfg.bit.rf_chn_en = 1;
	chn_trans_len.bit.rf_chn_trsc_len = len;
	chn_ptr_high.bit.rf_chn_src_data_addr_high = src->h;
	chn_ptr_high.bit.rf_chn_dst_data_addr_high = dst->h;
	edma->dma_chn_reg[chn].dma_dscr.rf_chn_data_src_addr_low = src->l;
	edma->dma_chn_reg[chn].dma_dscr.rf_chn_data_dst_addr_low = dst->l;
	edma->dma_chn_reg[chn].dma_dscr.chn_trans_len.reg = chn_trans_len.reg;
	edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg = chn_ptr_high.reg;
	edma->dma_chn_reg[chn].dma_cfg.reg = dma_cfg.reg;
	edma_hw_tx_req(chn);
	while (timeout--) {
		if (edma->dma_chn_reg[chn].dma_int.bit
				.rf_chn_tx_complete_int_raw_status == 1) {
			edma->dma_chn_reg[chn].dma_int.bit
						.rf_chn_tx_complete_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.bit
						.rf_chn_tx_pop_int_clr = 1;
			PCIE_INFO("[-]%s\n", __func__);

			return 0;
		}
		udelay(1);
	}
	PCIE_INFO("[-]%s timeout\n", __func__);

	return -1;
}
#endif

int edma_push_link(int chn, void *head, void *tail, int num)
{
	int i, j, inout;
	struct mbuf_t *mbuf;
	struct cpdu_head *cpdu;
	struct desc *last = NULL;
	union dma_chn_cfg_reg dma_cfg;
	struct edma_info *edma = edma_info();

	inout = edma->chn_sw[chn].inout;
	if ((head == NULL) || (tail == NULL) || (num == 0)) {
		PCIE_ERR("%s(%d, 0x%p, 0x%p, %d) err\n", __func__,
				chn, head, tail, num);
		return -1;
	}
	if (num > edma->chn_sw[chn].dscr_ring.free) {
		PCIE_INFO("%s@%d err,chn:%d num:%d free:%d\n",
			  __func__, __LINE__, chn, num,
			  edma->chn_sw[chn].dscr_ring.free);
		/* dscr not enough */
		return -1;
	}

	PCIE_INFO("%s(chn=%d, head=0x%p, tail=0x%p, num=%d)\n",
		  __func__, chn, head, tail, num);

	spin_lock_irqsave(edma->chn_sw[chn].dscr_ring.lock.irq_spinlock_p,
			edma->chn_sw[chn].dscr_ring.lock.flag);

	for (i = 0, j = 0, mbuf = head, cpdu = head; i < num; i++) {
		dscr_zero(edma->chn_sw[chn].dscr_ring.tail);

		dscr_link_mbuf(inout,
				       edma->chn_sw[chn].dscr_ring.tail, mbuf);

		if ((edma->chn_sw[chn].interval) &&
			((++j) == edma->chn_sw[chn].interval)) {
			if (inout == TX)
				edma->chn_sw[chn].dscr_ring
				.tail->chn_trans_len.bit.rf_chn_tx_intr = 1;
			else
				edma->chn_sw[chn].dscr_ring
				.tail->chn_trans_len.bit.rf_chn_rx_intr = 1;
			j = 0;
		}
		last = edma->chn_sw[chn].dscr_ring.tail;
		edma->chn_sw[chn].dscr_ring.tail =
				edma->chn_sw[chn].dscr_ring.tail->next.p;
		edma->chn_sw[chn].dscr_ring.free--;

		mbuf = mbuf->next;
	}
	if (inout == TX) {
		last->chn_trans_len.bit.rf_chn_eof = 1;
	} else {
		last->chn_trans_len.bit.rf_chn_rx_intr = 1;
		last->chn_trans_len.bit.rf_chn_pause = 1;
	}
	spin_unlock_irqrestore(edma->chn_sw[chn].dscr_ring.lock.irq_spinlock_p,
				edma->chn_sw[chn].dscr_ring.lock.flag);

	if (inout == TX) {

		dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
		dma_cfg.bit.rf_chn_en = 1;
		edma->dma_chn_reg[chn].dma_cfg.reg = dma_cfg.reg;
		edma_hw_tx_req(chn);
	} else
		edma_hw_rx_req(chn);

	return 0;
}

static int edma_pending_q_buffer(int chn, void *head, void *tail, int num)
{
	struct edma_pending_q *q;
	struct edma_info *edma = edma_info();

	q = &(edma->chn_sw[chn].pending_q);

	if ((q->wt + 1) % (q->max) == q->rd) {
		PCIE_ERR("%s(%d) full\n", __func__, chn);
		return ERROR;
	}
	q->ring[q->wt].head = head;
	q->ring[q->wt].tail = tail;
	q->ring[q->wt].num  = num;
	q->wt = INCR_RING_BUFF_INDX(q->wt, q->max);

	return OK;
}

int edma_push_link_async(int chn, void *head, void *tail, int num)
{
	int ret;

	struct edma_info *edma = edma_info();
	struct edma_pending_q *q = &(edma->chn_sw[chn].pending_q);

	if (edma->chn_sw[chn].inout == 0) {
		ret = edma_push_link(chn, head, tail, num);
		return ret;
	}
	spin_lock_irqsave(q->lock.irq_spinlock_p, q->lock.flag);
	if (q->status) {
		ret = edma_pending_q_buffer(chn, head, tail, num);
		spin_unlock_irqrestore(q->lock.irq_spinlock_p, q->lock.flag);
		return ret;
	}
	edma->chn_sw[chn].pending_q.status = 1;
	spin_unlock_irqrestore(q->lock.irq_spinlock_p, q->lock.flag);
	ret = edma_push_link(chn, head, tail, num);

	return ret;
}

int edma_push_link_wait_complete(int chn, void *head, void *tail, int num,
				 int timeout)
{
	int ret;
	struct edma_info *edma = edma_info();

	edma->chn_sw[chn].wait = timeout;
	edma_push_link(chn, head, tail, num);
	ret = wait_wcnevent(&(edma->chn_sw[chn].event), timeout);

	return ret;
}


static int edma_pending_q_num(int chn)
{
	int ret;
	struct edma_pending_q *q;
	struct edma_info *edma = edma_info();

	q = &(edma->chn_sw[chn].pending_q);

	if (q->wt >= q->rd)
		ret = q->wt - q->rd;
	else
		ret = q->wt + (q->max - q->rd);
	return ret;
}

static int edma_pending_q_flush(int chn)
{
	int num, ret;
	void *head, *tail;
	struct edma_pending_q *q;
	struct edma_info *edma = edma_info();

	q = &(edma->chn_sw[chn].pending_q);
	spin_lock_irqsave(q->lock.irq_spinlock_p, q->lock.flag);
	if (edma_pending_q_num(chn) <= 0) {
		edma->chn_sw[chn].pending_q.status = 0;
		spin_unlock_irqrestore(q->lock.irq_spinlock_p, q->lock.flag);

		return OK;
	}
	head = q->ring[q->rd].head;
	tail = q->ring[q->rd].tail;
	num  = q->ring[q->rd].num;
	q->rd = INCR_RING_BUFF_INDX(q->rd, q->max);
	spin_unlock_irqrestore(q->lock.irq_spinlock_p, q->lock.flag);

	ret = edma_push_link(chn, head, tail, num);

	return ret;
}

int edma_tx_complete_isr(int chn, int mode)
{
	struct desc *start, *end;
	void *head, *tail;
	int node = 0, ret;
	struct edma_info *edma = edma_info();

	switch (mode) {
	case TWO_LINK_MODE:
		start = edma->chn_sw[chn].dscr_ring.head;
		edma_hw_next_dscr(chn, edma->chn_sw[chn].inout, &end);
		end = mpool_phy_to_vir(end);
		if (start != end) {
			edma_pop_link(chn, start, end, (void **)(&head),
				      (void **)(&tail), &node);
		}
		if (edma->chn_sw[chn].wait == 0) {
			if (node > 0)
				ret = mchn_hw_pop_link(chn, head, tail, node);

			if ((edma->chn_sw[chn].inout == TX) &&
			   (mchn_hw_max_pending(chn) > 0))
				ret = edma_pending_q_flush(chn);

			mchn_hw_tx_complete(chn, 0);
		} else {
			edma->chn_sw[chn].wait = 0;
			set_wcnevent(&(edma->chn_sw[chn].event));
		}
		break;
	case ONE_LINK_MODE:
		set_wcnevent(&(edma->chn_sw[chn].event));
		break;
	case NON_LINK_MODE:
	case 3:
		set_wcnevent(&(edma->chn_sw[chn].event));
		break;
	default:

		break;
	}
	return 0;
}

static int edma_tx_pop_isr(int chn)
{
	int node = 0;
	struct desc *start, *end;
	void *head, *tail;
	struct edma_info *edma = edma_info();

	start = edma->chn_sw[chn].dscr_ring.head;
	edma_hw_next_dscr(chn, edma->chn_sw[chn].inout, &end);
	end = mpool_phy_to_vir(end);
	if (start == end) {
		PCIE_INFO("[-]%s empty\n", __func__);
		return 0;
	}
	if (edma->chn_sw[chn].wait == 0) {
		edma_pop_link(chn, start, end, (void **)(&head),
			      (void **)(&tail), &node);
		if (node > 0)
			mchn_hw_pop_link(chn, head, tail, node);
	}

	return 0;
}

static int edma_rx_push_isr(int chn)
{
	int ret, node = 0;
	struct desc *end;
	void *head, *tail;

	struct edma_info *edma = edma_info();

	ret = dscr_ring_empty(chn);
	if (!ret) {
		edma_hw_next_dscr(chn, edma->chn_sw[chn].inout, &end);
		end = mpool_phy_to_vir(end);
		if (end != edma->chn_sw[chn].dscr_ring.head)
			edma_pop_link(chn, edma->chn_sw[chn].dscr_ring.head,
				      end, (void **)(&head), (void **)(&tail),
				      &node);
		if (node > 0)
			mchn_hw_pop_link(chn, head, tail, node);
	}
	if (edma->chn_sw[chn].dscr_ring.free > 0)
		mchn_hw_req_push_link(chn, edma->chn_sw[chn].dscr_ring.free);

	return 0;
}

static int edma_rx_pop_isr(int chn)
{
	int node;
	struct desc *end;
	void *head, *tail;
	struct edma_info *edma = edma_info();

	edma_hw_next_dscr(chn, edma->chn_sw[chn].inout, &end);
	end = mpool_phy_to_vir(end);
	if (end == edma->chn_sw[chn].dscr_ring.head)
		return 0;

	edma_pop_link(chn, edma->chn_sw[chn].dscr_ring.head, end,
		      (void **)(&head), (void **)(&tail), &node);
	if (node > 0)
		mchn_hw_pop_link(chn, head, tail, node);
	return 0;
}

static int hisrfunc(struct isr_msg_queue *msg)
{
	int chn;
	union dma_chn_int_reg dma_int;
	struct edma_info *edma = edma_info();

	chn = msg->chn;
	switch (msg->evt) {

	case ISR_MSG_INTx:
		dma_int.reg = msg->dma_int.reg;
		switch (edma->chn_sw[chn].mode) {
		case TWO_LINK_MODE:
			if (edma->chn_sw[chn].inout) {
				if (dma_int.bit.rf_chn_tx_pop_int_mask_status)
					edma_tx_pop_isr(chn);

				if (dma_int.bit
				    .rf_chn_tx_complete_int_mask_status)
					edma_tx_complete_isr(chn,
							     TWO_LINK_MODE);
			} else {
				if (dma_int.bit.rf_chn_rx_pop_int_mask_status)
					edma_rx_pop_isr(chn);
				if (dma_int.bit
					   .rf_chn_rx_push_int_mask_status)
					edma_rx_push_isr(chn);
			}
			/* fallthrough... */
		case ONE_LINK_MODE:
			break;
		case NON_LINK_MODE:
			break;
		default:
			PCIE_INFO("%s unknown mode\n", __func__);
			break;
		}
		break;
	case ISR_MSG_TX_POP:
		edma_tx_pop_isr(chn);
		break;
	case ISR_MSG_TX_COMPLETE:
		edma_tx_complete_isr(chn, TWO_LINK_MODE);
		break;
	case ISR_MSG_RX_POP:
		edma_rx_pop_isr(chn);
		break;
	case ISR_MSG_RX_PUSH:
		edma_rx_push_isr(chn);
		break;
	case ISR_MSG_EXIT_FUNC:
		break;
	default:
		pcie_hexdump("isr unknown msg", (unsigned char *)(msg),
			     sizeof(struct isr_msg_queue));
		break;
	}

	return 0;
}

int q_info(int debug)
{
	struct edma_info *edma = edma_info();
	struct msg_q *q = &(edma->isr_func.q);

	PCIE_INFO("seq(%d,%d), line:%d, sem:%d\n", hisrfunc_last_msg,
	       q->seq, hisrfunc_line, q->event.wait_sem.count);

	hisrfunc_debug = debug;
	set_wcnevent(&(edma->isr_func.q.event));

	return 0;
}

int legacy_irq_handle(int data)
{
	unsigned long irq_flags;
	int chn, discard;
	int ret = 0;
	unsigned int dma_int_mask_status;
	union dma_chn_int_reg dma_int;
	struct isr_msg_queue msg = { 0 };
	struct edma_info *edma = edma_info();

	local_irq_save(irq_flags);
	dma_int_mask_status = edma->dma_glb_reg->dma_int_mask_status;
	for (chn = 0; chn < 32; chn++) {
		if (!(dma_int_mask_status & (1 << chn)))
			continue;
		dma_int.reg = edma->dma_chn_reg[chn].dma_int.reg;
		if (dma_int.bit.rf_chn_cfg_err_int_mask_status) {
			PCIE_ERR("%s chn %d assert(0x%x)\n", __func__,
					chn, dma_int.reg);
			dma_int.bit.rf_chn_cfg_err_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			continue;
		}
		discard = 1;
		msg.chn = chn;
		msg.evt = ISR_MSG_INTx;
		msg.dma_int.reg = dma_int.reg;
		switch (edma->chn_sw[chn].mode) {
		case TWO_LINK_MODE:
			if (edma->chn_sw[chn].inout) {
				if (dma_int.bit.rf_chn_tx_pop_int_mask_status) {
					dma_int.bit.rf_chn_tx_pop_int_clr = 1;
					discard = 0;
				}
				if (dma_int.bit
					 .rf_chn_tx_complete_int_mask_status) {

					dma_int.bit
						.rf_chn_tx_complete_int_clr = 1;
					discard = 0;
				}
			} else {
				if (dma_int.bit.rf_chn_rx_pop_int_mask_status) {
					dma_int.bit.rf_chn_rx_pop_int_clr = 1;
					discard = 0;
				}
				if (dma_int.bit.rf_chn_rx_push_int_mask_status
									== 1) {
					dma_int.bit.rf_chn_rx_push_int_clr = 1;
					discard = 0;
				}
			}
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			if (!discard) {
				if (mchn_hw_cb_in_irq(chn) == 0) {
					enqueue(&(edma->isr_func.q),
						(unsigned char *)(&msg));
					set_wcnevent(&(edma->isr_func
								.q.event));
				} else if (mchn_hw_cb_in_irq(chn) == -1) {
					ret = -1;
					break;
				}

				hisrfunc(&msg);
			}
			break;
		case ONE_LINK_MODE:
			if (dma_int.bit.rf_chn_tx_complete_int_mask_status == 1)
				dma_int.bit.rf_chn_tx_complete_int_clr = 1;
			if (dma_int.bit.rf_chn_tx_pop_int_mask_status == 1)
				dma_int.bit.rf_chn_tx_pop_int_clr = 1;
			if (dma_int.bit.rf_chn_tx_complete_int_mask_status == 1)
				dma_int.bit.rf_chn_tx_complete_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			edma_tx_complete_isr(chn, ONE_LINK_MODE);
			break;
		case NON_LINK_MODE:
			if (dma_int.bit.rf_chn_tx_pop_int_mask_status == 1)
				dma_int.bit.rf_chn_tx_pop_int_clr = 1;
			if (dma_int.bit
				.rf_chn_tx_complete_int_mask_status == 1) {
				dma_int.bit.rf_chn_tx_complete_int_clr = 1;
				edma_tx_complete_isr(chn, NON_LINK_MODE);
			}
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			break;
		default:
			if (dma_int.bit.rf_chn_tx_pop_int_mask_status)
				dma_int.bit.rf_chn_tx_pop_int_clr = 1;
			if (dma_int.bit.rf_chn_tx_complete_int_mask_status)
				dma_int.bit.rf_chn_tx_complete_int_clr = 1;
			if (dma_int.bit.rf_chn_rx_pop_int_mask_status)
				dma_int.bit.rf_chn_rx_pop_int_clr = 1;
			if (dma_int.bit.rf_chn_rx_push_int_mask_status)
				dma_int.bit.rf_chn_rx_push_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			PCIE_INFO("%s chn %d not ready\n", __func__, chn);
			break;
		}
	}
	local_irq_restore(irq_flags);

	return ret;
}

int msi_irq_handle(int irq)
{
	int chn = 0;
	unsigned long irq_flags;
	union dma_chn_int_reg dma_int;
	struct isr_msg_queue msg = { 0 };
	struct edma_info *edma = edma_info();

	PCIE_INFO("irq msi handle=%d\n", irq);
	local_irq_save(irq_flags);
	chn = (irq - 0) / 2;
	dma_int.reg = edma->dma_chn_reg[chn].dma_int.reg;
	msg.chn = chn;
	if (edma->chn_sw[chn].inout == TX) {
		if (irq % 2 == 0) {
			dma_int.bit.rf_chn_tx_pop_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			msg.evt = ISR_MSG_TX_POP;
		} else {
			dma_int.bit.rf_chn_tx_complete_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			msg.evt = ISR_MSG_TX_COMPLETE;
		}
	} else {
		if (irq % 2 == 0) {
			do {
				dma_int.bit.rf_chn_rx_pop_int_clr = 1;
				edma->dma_chn_reg[chn].dma_int.reg =
								dma_int.reg;
			} while (edma->dma_chn_reg[chn].dma_int.reg & 0x040400);

			msg.evt = ISR_MSG_RX_POP;

		} else {
			dma_int.bit.rf_chn_rx_push_int_clr = 1;
			edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
			msg.evt = ISR_MSG_RX_PUSH;
		}
	}
	if (mchn_hw_cb_in_irq(chn) == 0) {
		enqueue(&(edma->isr_func.q), (unsigned char *)(&msg));
		PCIE_INFO(" callback not in irq\n");
		set_wcnevent(&(edma->isr_func.q.event));
		local_irq_restore(irq_flags);
		return 0;
	} else if (mchn_hw_cb_in_irq(chn) == -1) {
		local_irq_restore(irq_flags);
		return -1;
	}
	PCIE_INFO("callback in irq\n");
	hisrfunc(&msg);

	local_irq_restore(irq_flags);

	return 0;
}

int edma_task(void *a)
{
	struct isr_msg_queue msg = {};
	struct edma_info *edma = edma_info();
	struct msg_q *q = &(edma->isr_func.q);

	edma->isr_func.state = 1;
	PCIE_INFO("[+]%s\n", __func__);
	do {
		hisrfunc_line = __LINE__;
		wait_wcnevent(&(q->event), -1);
		hisrfunc_line = __LINE__;
		if (hisrfunc_debug)
			PCIE_INFO("#\n");
		while (dequeue(q, (unsigned char *)(&msg), -1) == OK) {
			hisrfunc_line = __LINE__;
			hisrfunc_last_msg = msg.seq;
			hisrfunc(&msg);
			hisrfunc_line = __LINE__;
			if (msg.evt == ISR_MSG_EXIT_FUNC)
				goto EXIT;
		}
	} while (1);
EXIT:
	edma->isr_func.state = 0;
	PCIE_INFO("[-]%s\n", __func__);

	return 0;
}

static void edma_tasklet(unsigned long data)
{
	struct isr_msg_queue msg = { 0 };
	struct edma_info *edma = edma_info();
	struct msg_q *q = &(edma->isr_func.q);

	while (dequeue(q, (unsigned char *)(&msg), -1) == OK)
		hisrfunc(&msg);


}

static int dscr_ring_init(struct dscr_ring *dscr_ring, int inout, int size,
		   unsigned char *mem)
{
	int i;
	unsigned int tmp;
	struct desc *dscr;

	PCIE_INFO("[+]%s(0x%p, 0x%p)\n", __func__, dscr_ring, mem);

	if (!mem)
		dscr_ring->mem =
		    (unsigned char *)mpool_malloc(sizeof(struct desc) *
						     (size + 1));
	else
		dscr_ring->mem = mem;
	dscr_ring->size = size;
	memset(dscr_ring->mem, 0x00, sizeof(struct desc) * (size + 1));

	edma_spin_lock_init(&(dscr_ring->lock));

	dscr_ring->head = dscr_ring->tail = dscr =
						(struct desc *) dscr_ring->mem;
	for (i = 0; i < size; i++) {
		if (inout == TX) {
			dscr[i].chn_ptr_high.bit.rf_chn_tx_next_dscr_ptr_high =
						GET_8_OF_40(&dscr[i + 1]);
			tmp = GET_32_OF_40((unsigned char *)(&dscr[i + 1]));
			memcpy((unsigned char *)(&dscr[i]
				.rf_chn_tx_next_dscr_ptr_low),
			       (unsigned char *)(&tmp), 4);
		} else {
			dscr[i].chn_ptr_high.bit.rf_chn_rx_next_dscr_ptr_high =
			    GET_8_OF_40(&dscr[i + 1]);
			tmp = GET_32_OF_40((unsigned char *)(&dscr[i + 1]));
			memcpy((unsigned char *)(&dscr[i]
						.rf_chn_rx_next_dscr_ptr_low),
			       (unsigned char *)(&tmp), 4);
		}
		PCIE_INFO("dscr(0x%p-->0x%p)\n",
			  mpool_vir_to_phy(&dscr[i]),
			  mpool_vir_to_phy(&dscr[i + 1]));
		dscr[i].next.p = &dscr[i + 1];
	}
	if (inout == TX) {
		dscr[i].chn_ptr_high.bit.rf_chn_tx_next_dscr_ptr_high =
		    GET_8_OF_40(&dscr[0]);
		tmp = GET_32_OF_40((unsigned char *)(&dscr[0]));
		memcpy((unsigned char *)(&dscr[i].rf_chn_tx_next_dscr_ptr_low),
		       (unsigned char *)(&tmp), 4);
		dscr[0].chn_trans_len.bit.rf_chn_eof = 1;

	} else {
		dscr[i].chn_ptr_high.bit.rf_chn_rx_next_dscr_ptr_high =
		    GET_8_OF_40(&dscr[0]);
		tmp = GET_32_OF_40((unsigned char *)(&dscr[0]));
		memcpy((unsigned char *)(&dscr[i].rf_chn_rx_next_dscr_ptr_low),
		       (unsigned char *)(&tmp), 4);
		dscr[0].chn_trans_len.bit.rf_chn_pause = 1;
	}
	PCIE_INFO("dscr(0x%p-->0x%p)\n",
		  mpool_vir_to_phy(&dscr[i]),
		  mpool_vir_to_phy(&dscr[0]));
	dscr[i].next.p = &dscr[0];
	dscr_ring->free = size;
	PCIE_INFO("[-]%s(0x%p, 0x%p, %d, %d)\n", __func__, dscr_ring,
		  dscr_ring->mem, dscr_ring->size, dscr_ring->free);
	return 0;
}

static int edma_pending_q_init(int chn, int max)
{
	struct edma_pending_q *q;
	struct edma_info *edma = edma_info();

	q = &(edma->chn_sw[chn].pending_q);
	memset((char *)q, 0x00, sizeof(struct edma_pending_q));
	q->max = max;
	q->chn = chn;
	edma_spin_lock_init(&(q->lock));

	return OK;
}

int edma_chn_init(int chn, int mode, int inout, int max_trans)
{
	int ret, dir = 0;
	struct dscr_ring *dscr_ring;
	struct edma_info *edma = edma_info();
	union dma_chn_int_reg dma_int = { 0 };
	union dma_chn_cfg_reg dma_cfg = { 0 };
	struct desc local_DSCR;

	if (inout == RX)
		/* int direction. int send to ap */
		dir = 1;
	PCIE_INFO("[+]%s(chn=%d,mode=%d,dir=%d,inout=%d,max_trans=%d)\n",
		  __func__, chn, mode, dir, inout, max_trans);

	dma_int.reg = edma->dma_chn_reg[chn].dma_int.reg;
	dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
	local_DSCR = edma->dma_chn_reg[chn].dma_dscr;

	switch (mode) {
	case TWO_LINK_MODE:
		dscr_ring = &(edma->chn_sw[chn].dscr_ring);
		ret = dscr_ring_init(dscr_ring, inout, max_trans, NULL);
		if (ret)
			return ERROR;
		/* 1:enable channel; 0:disable channel */
		if (dma_cfg.bit.rf_chn_en == 0) {
			dma_cfg.bit.rf_chn_en = 1;
			/* 0:trans done, 1:linklist done */
			dma_cfg.bit.rf_chn_req_mode = 1;
			dma_cfg.bit.rf_chn_list_mode = TWO_LINK_MODE;
			if (!inout)
			    /* source data from CP */
				dma_cfg.bit.rf_chn_dir = 1;
			else
				/* source data from AP */
				dma_cfg.bit.rf_chn_dir = 0;
		}
		if (inout == TX) {
			/* tx_list link point */
			local_DSCR.rf_chn_tx_next_dscr_ptr_low =
			    GET_32_OF_40((unsigned char *)(dscr_ring->head));
			/* tx_list link point */
			local_DSCR.chn_ptr_high.bit
						.rf_chn_tx_next_dscr_ptr_high =
					GET_8_OF_40(dscr_ring->head);
			dma_int.bit.rf_chn_tx_complete_int_en = 1;
			dma_int.bit.rf_chn_tx_pop_int_clr = 1;
			dma_int.bit.rf_chn_tx_complete_int_clr = 1;
		} else {
			local_DSCR.rf_chn_rx_next_dscr_ptr_low =
			    GET_32_OF_40((unsigned char *)(dscr_ring->head));
			local_DSCR.chn_ptr_high.bit
						.rf_chn_rx_next_dscr_ptr_high =
					GET_8_OF_40(dscr_ring->head);
			dma_int.bit.rf_chn_rx_push_int_en = 1;
			/* clear semaphore value */
			dma_cfg.bit.rf_chn_sem_value = 0xFF;
		}
		edma_pending_q_init(chn, mchn_hw_max_pending(chn));
		break;
	case ONE_LINK_MODE:
		/* 0:to cp . 1:to AP */
		dma_cfg.bit.rf_chn_int_out_sel = dir;
		/* 1:enable channel; 0:disable channel */
		dma_cfg.bit.rf_chn_en = 1;
		/* 0:trans done, 1:linklist done */
		dma_cfg.bit.rf_chn_req_mode = 1;
		dma_cfg.bit.rf_chn_list_mode = ONE_LINK_MODE;
		dma_int.bit.rf_chn_tx_complete_int_en = 1;
		dma_int.bit.rf_chn_tx_pop_int_en = 1;
		break;

	case NON_LINK_MODE:
	case 3:
		dma_int.bit.rf_chn_tx_complete_int_en = 0;
		dma_int.bit.rf_chn_tx_pop_int_en = 0;
		dma_int.bit.rf_chn_rx_push_int_en = 0;
		dma_int.bit.rf_chn_rx_pop_int_en = 0;
		/* 0:tx_int to CP; 1:rx_int to AP */
		dma_cfg.bit.rf_chn_int_out_sel = dir;
		dma_cfg.bit.rf_chn_en = 1;
		dma_cfg.bit.rf_chn_list_mode = NON_LINK_MODE;
		break;
	default:
		break;
	}

	switch (mode) {
	case TWO_LINK_MODE:
		if (inout) {
			edma->dma_chn_reg[chn].dma_dscr
					      .rf_chn_tx_next_dscr_ptr_low =
					local_DSCR.rf_chn_tx_next_dscr_ptr_low;
			edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.bit
						.rf_chn_tx_next_dscr_ptr_high =
									0x80;
		} else {
			edma->dma_chn_reg[chn].dma_dscr
					      .rf_chn_rx_next_dscr_ptr_low =
					local_DSCR.rf_chn_rx_next_dscr_ptr_low;
			edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.bit
						.rf_chn_rx_next_dscr_ptr_high =
									0x80;
		}
		edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg =
		    local_DSCR.chn_ptr_high.reg;

		break;
	default:
		break;
	}
	edma->chn_sw[chn].dir = dir;
	edma->chn_sw[chn].inout = inout;
	edma->chn_sw[chn].mode = mode;
	edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
	edma->dma_chn_reg[chn].dma_cfg.reg = dma_cfg.reg;
	dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
	PCIE_INFO("[-]%s\n", __func__);

	return 0;
}

int edma_tp_count(int chn, void *head, void *tail, int num)
{
	int i, dt;
	struct mbuf_t *mbuf;
	static int bytecount;
	static struct timeval start_time, time;
	struct timespec now;

	for (i = 0, mbuf = (struct mbuf_t *)head; i < num; i++) {
		getnstimeofday(&now);
		time.tv_sec = now.tv_sec;
		time.tv_usec = now.tv_nsec/1000;

		if (bytecount == 0)
			start_time = time;
		bytecount += mbuf->len;
		dt = time_sub_us(&start_time, &time);
		if (dt >= 1000000) {
			PCIE_INFO("edma-tp:%d/%d (byte/us)\n",
			       bytecount, dt);
			bytecount = 0;
		}
		mbuf = mbuf->next;
	}

	return 0;
}

int edma_init(struct wcn_pcie_info *pcie_info)
{
	unsigned int i, ret, *reg;
	struct edma_info *edma = edma_info();

	memset((char *)edma, 0x00, sizeof(struct edma_info));
	edma->pcie_info = pcie_info;

	PCIE_INFO("new edma(0x%p--0x%p)\n", edma,
		  (void *)virt_to_phys((void *)(edma)));
	ret = create_queue(&(edma->isr_func.q), sizeof(struct isr_msg_queue),
			   50);
	if (ret != 0) {
		PCIE_ERR("create_queue fail\n");
		return -1;
	}
#if CONFIG_TASKLET_SUPPORT
	edma->isr_func.q.event.tasklet = kmalloc(sizeof(struct tasklet_struct),
						 GFP_KERNEL);
	tasklet_init(edma->isr_func.q.event.tasklet, edma_tasklet, 0);
#else
	edma->isr_func.entity = kthread_create(edma_task, edma, "edma_task");
	if (edma->isr_func.entity == NULL) {
		PCIE_ERR("create isr_func fail\n");

		return -1;
	}
	do {
		struct sched_param param;

		param.sched_priority = 90;
		ret = sched_setscheduler((struct task_struct *)edma->isr_func
					  .entity, SCHED_FIFO, &param);
		PCIE_INFO("sched_setscheduler(SCHED_FIFO), prio:%d,ret:%d\n",
			param.sched_priority, ret);
	} while (0);

	wake_up_process(edma->isr_func.entity);
#endif
	reg = (unsigned int *)(pcie_bar_vmem(edma->pcie_info, 0) + 0x130004);
	*reg = ((*reg) | 1 << 7);
	edma->dma_glb_reg = (struct edma_glb_reg *)
			(pcie_bar_vmem(edma->pcie_info, 0) + 0x160000);
	edma->dma_chn_reg = (struct edma_chn_reg *)
			(pcie_bar_vmem(edma->pcie_info, 0) + 0X161000);
	PCIE_INFO("WCN dma_chn_reg size is %ld\n", sizeof(struct edma_chn_reg));
	for (i = 0; i < 16; i++) {
		PCIE_INFO("edma chn[%d] dma_int:0x%p, event:%p\n", i,
			  &(edma->dma_chn_reg[i].dma_int.reg),
			  &(edma->chn_sw[i].event));
		create_wcnevent(&(edma->chn_sw[i].event), i);
		edma->chn_sw[i].mode = -1;
	}
	PCIE_INFO("%s done\n", __func__);

	return 0;
}

int edma_deinit(void)
{
	struct isr_msg_queue msg = { 0 };
	struct edma_info *edma = edma_info();

	PCIE_INFO("[+]%s\n", __func__);
	do {
		usleep_range(10000, 11000);
		if (edma->isr_func.state == 0)
			break;

		msg.evt = ISR_MSG_EXIT_FUNC;
		enqueue(&(edma->isr_func.q), (unsigned char *)&msg);
		set_wcnevent(&(edma->isr_func.q.event));
	} while (edma->isr_func.state);
#if CONFIG_TASKLET_SUPPORT
	tasklet_disable(edma->isr_func.q.event.tasklet);
	tasklet_kill(edma->isr_func.q.event.tasklet);
#endif
	PCIE_INFO("[-]%s\n", __func__);

	return 0;
}
