/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Authors	: jinglong.chen
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MCHN_H__
#define __MCHN_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#define HW_TYPE_SDIO 0
#define HW_TYPE_PCIE 1
#define MCHN_MAX_NUM 32

struct mbuf_t {
	struct __mbuf *next;
	unsigned char *buf;
	unsigned long  phy;
	unsigned short len;
	unsigned short rsvd;
	unsigned int   seq;
};

struct buffer_pool_t {
	int size;
	int free;
	int payload;
	void *head;
	char *mem;
	spinlock_t lock;
	unsigned long irq_flags;
};

struct mchn_ops_t {
	/*channel id*/
	int channel;
	/*hardware interface type*/
	int hif_type;
	/*inout=1 tx side, inout=0 rx side */
	int inout;
	/*set callback pop_link/push_link frequency */
	int intr_interval;
	/*data buffer size */
	int buf_size;
	/*mbuf pool size */
	int pool_size;
	/*The large number of trans */
	int once_max_trans;
	/*rx side threshold */
	int rx_threshold;
	/*tx timeout */
	int timeout;
	/*callback in top tophalf*/
	int cb_in_irq;
	/*pending link num*/
	int max_pending;
	/*pop link list, (1)chn id, (2)mbuf link head
	 *(3) mbuf link tail (4)number of node
	 */
	int (*pop_link)(int, mbuf_t *, mbuf_t *, int);
	/*ap don't need to implementation*/
	int (*push_link)(int, mbuf_t **, mbuf_t **, int *);
	/*(1)channel id (2)trans time, -1 express timeout*/
	int (*tx_complete)(int, int);
	int (*power_notify)(int, int);
};

struct mchn_info_t {
	mchn_ops_t *ops[MCHN_MAX_NUM];
	struct {
		buffer_pool_t pool;
	} chn_public[MCHN_MAX_NUM];
};

/*configuration channel*/
int mchn_init(struct mchn_ops_t *ops);
/*cancellation channel*/
int mchn_deinit(struct mchn_ops_t *ops);
/*push link list*/
int mchn_push_link(int channel, struct mbuf_t *head, struct mbuf_t *tail,
	int num);
/*push link list, Using a blocking mode, Timeout wait for tx_complete*/
int mchn_push_link_wait_complete(int chn, struct mbuf_t *head,
	struct mbuf_t *tail, int num, int timeout);
int mchn_hw_pop_link(int chn, void *head, void *tail, int num);
int mchn_hw_tx_complete(int chn, int timeout);
int mchn_hw_req_push_link(int chn, int need);
int mbuf_link_alloc(int chn, struct mbuf_t **head, struct mbuf_t **tail,
	int *num);
int mbuf_link_free(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num);
int mchn_hw_max_pending(int chn);
struct mchn_info_t *mchn_info(void);
struct mchn_ops_t *mchn_ops(int channel);
#endif

