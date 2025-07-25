/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __EDMA_ENGIN_H__
#define __EDMA_ENGIN_H__

#include "pcie_dbg.h"
#include "pcie.h"

enum ISR_TASK_MSG_ID {
	ISR_MSG_NULL = 0,
	ISR_MSG_TX_POP,
	ISR_MSG_TX_COMPLETE,
	ISR_MSG_RX_POP,
	ISR_MSG_RX_PUSH,
	ISR_MSG_INTx,
	ISR_MSG_EXIT_FUNC,
};

enum LINK_MODE_TYPE {
	TWO_LINK_MODE = 0,
	ONE_LINK_MODE,
	NON_LINK_MODE,
};

enum ADDR_REGION_TYPE {
	CPU64 = 0x0,
	AXI40 = 0x1,
	AHB32 = 0x2,
};

#define CONFIG_TASKLET_SUPPORT (1)
#define INCR_RING_BUFF_INDX(indx, max_num) \
	((((indx) + 1) < (max_num)) ? ((indx) + 1) : (0))

#define GET_32_OF_40(a) ((unsigned int)((unsigned long)	\
				(mpool_vir_to_phy(a)) & 0xFFFFFFFF))
#define GET_8_OF_40(a) (0x80)
#define SET_32_OF_40(a, v) do {	\
	unsigned long l = (unsigned long)(a);	\
	if (sizeof(unsigned long) == sizeof(unsigned int)) {	\
		a = (void *)((l&0x00000000)|(v));	\
	} else {	\
		a = (void *)((l&0xFFFFFFFF00000000)|(v));	\
	}	\
} while (0)
#define SET_8_OF_40(a, v) do {	\
	unsigned long l = (unsigned long)(a);	\
	if (sizeof(unsigned long) != sizeof(unsigned int))	\
		a = (void *)((l&0xFFFFFF00FFFFFFFF) |\
			     ((unsigned long)(v) << 32));\
} while (0)

#define COMPARE_40_BIT(a, b) ((sizeof(unsigned int) ==	\
	sizeof(unsigned long)) ? \
	(!((unsigned long)(a) ^ (unsigned long)(b))) :	\
	(!(((unsigned long)(a) ^ (unsigned long)(b)) & 0xFFFFFFFFFF)))

struct cpdu_head {
	struct cpdu_head *next;
	unsigned int __next:8;
	unsigned int    len:16;
	unsigned int offset:8;
	unsigned int   rsvd;
};

#ifdef __FOR_THREADX_H__
#define GET_32_OF_40(a) ((unsigned int)(a))
#define GET_8_OF_40(a) (0)
#define SET_32_OF_40(a, v) (a = (struct desc *)(v))
#define SET_8_OF_40(a, v)

#define COMPARE_40_BIT(a, b) ((sizeof(unsigned int) ==	\
	sizeof(unsigned long)) ? \
	(!((unsigned long)(a) ^ (unsigned long)(b))) :	\
	(!(((unsigned long)(a) ^ (unsigned long)(b)) & 0xFFFFFFFFFF)))

struct mbuf_t {
	struct mbuf_t *next;
	unsigned char *buf;
	unsigned short len;
	unsigned short rsvd;
	unsigned int seq;
};
#endif

union dma_glb_pause_reg {
	unsigned int reg;
	struct {
		unsigned int rf_dma_pause:1;
		unsigned int rsvd0:1;
		unsigned int rf_dma_pause_status:1;
		unsigned int rsvd1:5;
		unsigned int rf_dma_dst_outstanding_num:4;
		unsigned int rf_dma_src_outstanding_num:4;

		unsigned int edma_reg_rclk_cg_en:1;
		unsigned int edma_glb_cfg_wclk_cg_en:1;
		unsigned int edma_chn_cfg_wclk_cg_en:1;
		unsigned int edma_req_cid_wclk_cg_en:1;
		unsigned int edma_chn_int_clk_cg_en:1;
		unsigned int edma_axi_clk_cg_en:1;
		unsigned int rsvd2:2;
		unsigned int rf_dma_pcie_legacy_int_en:1;
		unsigned int rsvd3:7;
	} bit;
};

union dma_glb_prot_reg {
	unsigned int reg;
	struct {
		unsigned int rf_dma_pause_prot:1;
		unsigned int rf_dma_int_raw_status_prot:1;
		unsigned int rf_dma_int_mask_status_prot:1;
		unsigned int rf_dma_req_status_prot:1;
		unsigned int rf_dma_debug_status_prot:1;
		unsigned int rf_dma_arb_sel_prot:1;
		unsigned int rf_dma_sync_sec_normal_prot:1;
		unsigned int rsvd:25;
	} bit;
};

union dma_glb_msix_value_reg {
	unsigned int reg;
	struct {
		unsigned int rf_dma_pcie_msix_value:22;
		unsigned int rsvd:2;
		unsigned int rf_dma_pcie_msix_reg_addr_hi:8;
	} bit;
};

union dma_chn_int_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_tx_pop_int_en:1;
		unsigned int rf_chn_tx_complete_int_en:1;
		unsigned int rf_chn_rx_pop_int_en:1;
		unsigned int rf_chn_rx_push_int_en:1;
		unsigned int rf_chn_cfg_err_int_en:1;
		unsigned int rsvd0:3;
		unsigned int rf_chn_tx_pop_int_raw_status:1;
		unsigned int rf_chn_tx_complete_int_raw_status:1;
		unsigned int rf_chn_rx_pop_int_raw_status:1;
		unsigned int rf_chn_rx_push_int_raw_status:1;
		unsigned int rf_chn_cfg_err_int_raw_status:1;
		unsigned int rsvd1:3;
		unsigned int rf_chn_tx_pop_int_mask_status:1;
		unsigned int rf_chn_tx_complete_int_mask_status:1;
		unsigned int rf_chn_rx_pop_int_mask_status:1;
		unsigned int rf_chn_rx_push_int_mask_status:1;
		unsigned int rf_chn_cfg_err_int_mask_status:1;
		unsigned int rsvd2:3;
		unsigned int rf_chn_tx_pop_int_clr:1;
		unsigned int rf_chn_tx_complete_int_clr:1;
		unsigned int rf_chn_rx_pop_int_clr:1;
		unsigned int rf_chn_rx_push_int_clr:1;
		unsigned int rf_chn_cfg_err_int_clr:1;
		unsigned int rsvd3:3;
	} bit;
};

union dma_chn_tx_req_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_tx_req:1;
		unsigned int rsvd:31;
	} bit;
};

union dma_chn_rx_req_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_rx_req:1;
		unsigned int rsvd:31;
	} bit;
};

union dma_chn_cfg_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_en:1;
		unsigned int rsvd0:3;
		unsigned int rf_chn_list_mode:2;
		unsigned int rf_chn_int_to_ap_type:1;
		unsigned int rf_chn_dir:1;
		unsigned int rf_chn_swt_mode:2;
		unsigned int rf_chn_priority:2;
		unsigned int rf_dont_wait_ddone:1;
		unsigned int rf_chn_req_mode:1;
		unsigned int rf_chn_int_out_sel:1;
		unsigned int rsvd1:1;
		unsigned int rf_chn_sem_value:8;
		unsigned int rf_chn_err_status:3;
		unsigned int rsvd2:5;

	} bit;
};

union dma_dscr_trans_len_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_trsc_len:16;
		unsigned int rf_rsvd0:8;
		unsigned int rf_chn_done:1;
		unsigned int rf_chn_pause:1;
		unsigned int rf_chn_tx_intr:1;
		unsigned int rf_chn_rx_intr:1;
		unsigned int rf_chn_eof:1;
		unsigned int rf_rsvd1:3;
	} bit;
};

union dma_dscr_ptr_high_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_src_data_addr_high:8;
		unsigned int rf_chn_dst_data_addr_high:8;
		unsigned int rf_chn_tx_next_dscr_ptr_high:8;
		unsigned int rf_chn_rx_next_dscr_ptr_high:8;
	} bit;
};

union dma_dscr_req1_cid_reg {
	unsigned int reg;
	struct {
		unsigned int rf_chn_req1_cid:6;
		unsigned int rf_rsvd2:26;
	} bit;
};

struct edma_glb_reg {
	union dma_glb_pause_reg dma_pause;
	unsigned int dma_int_raw_status;
	unsigned int dma_int_mask_status;
	unsigned int dma_req_status;
	unsigned int dma_debug_status;
	unsigned int dma_arb_sel_status;
	unsigned int dma_chn_arprot;
	unsigned int dma_chn_awprot;
	unsigned int dma_chn_prot_flag;
	union dma_glb_prot_reg dma_glb_prot;
	unsigned int dma_req_cid_prot;
	unsigned int dma_sync_sec_nomal;
	unsigned int dma_pcie_msix_reg_addr_low;
	union dma_glb_msix_value_reg dma_pcie_msix_value;
};

struct desc {
	union dma_dscr_trans_len_reg chn_trans_len;
	union dma_dscr_ptr_high_reg chn_ptr_high;
	unsigned int rf_chn_tx_next_dscr_ptr_low;
	unsigned int rf_chn_rx_next_dscr_ptr_low;
	unsigned int rf_chn_data_src_addr_low;
	unsigned int rf_chn_data_dst_addr_low;

	union {
		struct desc *p;
		unsigned int t[2];
	} next;
	union {
		void *p;
		unsigned char *src;
		unsigned int t[2];
	} link;
	union {
		unsigned char *p;
		unsigned char *dst;
		unsigned int t[2];
	} buf;
};

struct edma_chn_reg {
	union dma_chn_int_reg dma_int;
	union dma_chn_tx_req_reg dma_tx_req;
	union dma_chn_rx_req_reg dma_rx_req;
	union dma_chn_cfg_reg dma_cfg;
	struct desc dma_dscr;
};

enum ERROR_CODE {
	OK = 0,
	ERROR = -1,
	TIMEOUT = -2,
};

struct event_t {
	int id;
	int flag;
	struct semaphore wait_sem;
	struct timeval time;
	struct tasklet_struct *tasklet;
};

struct lock_sw {
	void *entity;
	unsigned long flag;
	char *name;
};

struct irq_lock_t {
	spinlock_t  *irq_spinlock_p;
	unsigned long flag;
	char *name;
};

struct dscr_ring {
	int size;
	int free;
	struct desc *head;
	struct desc *tail;
	unsigned char *mem;
	struct irq_lock_t lock;
};

struct edma_pending_q {
	struct {
		void *head;
		void *tail;
		int   num;
	} ring[32];
	int wt;
	int rd;
	int max;
	int status;
	int chn;
	struct irq_lock_t lock;
};

struct isr_msg_queue {
	unsigned int seq;
	unsigned short chn;
	unsigned short evt;
	union dma_chn_int_reg dma_int;
};

struct msg_q {
	unsigned int seq;
	unsigned int wt;
	unsigned int rd;
	unsigned int max;
	unsigned int size;
	unsigned char *mem;
	struct irq_lock_t lock;
	struct event_t event;
};

struct edma_info {
	struct edma_glb_reg *dma_glb_reg;
	struct edma_chn_reg *dma_chn_reg;
	int (*enable)(void);
	int (*reset)(void);
	int ap;
	struct {
		int mode;
		int wait;
		int interval;
		int tx_complete_verify;
		struct event_t event;
		struct dscr_ring dscr_ring;
		unsigned char dir;
		unsigned char inout;
		unsigned char state;
		struct edma_pending_q pending_q;
	} chn_sw[32];
	struct {
		int state;
		void *entity;
		struct msg_q q;
	} isr_func;
	struct wcn_pcie_info *pcie_info;
};

int edma_init(struct wcn_pcie_info *pcie_info);
int edma_deinit(void);

int edma_chn_init(int chn, int mode, int inout, int max_trans);
int edma_one_link_dscr_buf_bind(struct desc *dscr, unsigned char *dst,
				       unsigned char *src, unsigned short len);
struct edma_info *edma_info(void);
int msi_irq_handle(int irq);
int legacy_irq_handle(int data);
int edma_push_link(int chn, void *head, void *tail, int num);
int edma_push_link_async(int chn, void *head, void *tail, int num);
int edma_push_link_wait_complete(int chn, void *head, void *tail,
				 int num, int timeout);
int mchn_hw_pop_link(int chn, void *head, void *tail, int num);
int mchn_hw_cb_in_irq(int chn);
int mchn_hw_max_pending(int chn);
int edma_tp_count(int chn, void *head, void *tail, int num);
void *mpool_malloc(int len);
int mpool_free(void);
void *pcie_alloc_memory(int len);
int delete_queue(struct msg_q *q);
#endif
