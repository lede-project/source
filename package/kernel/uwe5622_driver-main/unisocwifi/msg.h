/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
 * Jingxiang Li <Jingxiang.li@spreadtrum.com>
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

#ifndef __SPRDWL_MSG_H__
#define __SPRDWL_MSG_H__

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/version.h>

/* 0 for cmd, 1 for event, 2 for data, 3 for mh data */
enum sprdwl_head_type {
	SPRDWL_TYPE_CMD,
	SPRDWL_TYPE_EVENT,
	SPRDWL_TYPE_DATA,
	SPRDWL_TYPE_DATA_SPECIAL,
	SPRDWL_TYPE_DATA_PCIE_ADDR,
	SPRDWL_TYPE_PKT_LOG,
};

enum sprdwl_head_rsp {
	/* cmd no rsp */
	SPRDWL_HEAD_NORSP,
	/* cmd need rsp */
	SPRDWL_HEAD_RSP,
};

/* bit[7][6][5] ctx_id: context id
 * bit[4] rsp: sprdwl_head_rsp
 * bit[3] reserv
 * bit[2][1][0] type: sprdwl_head_type
 */
struct sprdwl_common_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 type:3;
	__u8 reserv:1;
	__u8 rsp:1;
	__u8 ctx_id:3;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8 ctx_id:3;
	__u8 rsp:1;
	__u8 reserv:1;
	__u8 type:3;
#else
#error  "check <asm/byteorder.h> defines"
#endif
};

#define SPRDWL_CMD_STATUS_OK			0
#define SPRDWL_CMD_STATUS_ARG_ERROR		-1
#define SPRDWL_CMD_STATUS_GET_RESULT_ERROR	-2
#define SPRDWL_CMD_STATUS_EXEC_ERROR		-3
#define SPRDWL_CMD_STATUS_MALLOC_ERROR		-4
#define SPRDWL_CMD_STATUS_WIFIMODE_ERROR	-5
#define SPRDWL_CMD_STATUS_ERROR			-6
#define SPRDWL_CMD_STATUS_CONNOT_EXEC_ERROR	-7
#define SPRDWL_CMD_STATUS_NOT_SUPPORT_ERROR	-8
#define SPRDWL_CMD_STATUS_CRC_ERROR			-9
#define SPRDWL_CMD_STATUS_INI_INDEX_ERROR   -10
#define SPRDWL_CMD_STATUS_LENGTH_ERROR     -11
#define SPRDWL_CMD_STATUS_OTHER_ERROR		-127

#define SPRDWL_HEAD_GET_TYPE(common) \
	(((struct sprdwl_common_hdr *)(common))->type)

#define SPRDWL_HEAD_GET_CTX_ID(common) \
	(((struct sprdwl_common_hdr *)(common))->ctx_id)

#define SPRD_HEAD_GET_RESUME_BIT(common) \
	(((struct sprdwl_common_hdr *)(common))->reserv)

struct sprdwl_cmd_hdr {
	struct sprdwl_common_hdr common;
	u8 cmd_id;
	/* the payload len include the size of this struct */
	__le16 plen;
	__le32 mstime;
	s8 status;
	u8 rsp_cnt;
	u8 reserv[2];
	u8 paydata[0];
} __packed;

struct sprdwl_addr_hdr {
	struct sprdwl_common_hdr common;
	u8 paydata[0];
} __packed;

#define SPRDWL_GET_CMD_PAYDATA(msg) \
	    (((struct sprdwl_cmd_hdr *)((msg)->skb->data))->paydata)

struct sprdwl_data_hdr {
	struct sprdwl_common_hdr common;

#define WAPI_PN_SIZE                16
#define SPRDWL_DATA_OFFSET         2
	u8 info1; /*no used in marlin3*/
	/* the payload len include the size of this struct */
	__le16 plen;
	/* the flow contrl shared by sta and p2p */
	u8 flow0;
	/* the sta flow contrl */
	u8 flow1;
	/* the p2p flow contrl */
	u8 flow2;
	/* flow3 0: share, 1: self */
	u8 flow3;
} __packed;

struct sprdwl_pktlog_hdr {
	struct sprdwl_common_hdr common;
	u8 rsvd;
	/* the payload len include the size of this struct */
	__le16 plen;
} __packed;

struct sprdwl_msg_list {
	struct list_head freelist;
	struct list_head busylist;
	/*cmd to be free list*/
	struct list_head cmd_to_free;
	int maxnum;
	/* freelist lock */
	spinlock_t freelock;
	/* busylist lock */
	spinlock_t busylock;
	/*cmd_to_free lock*/
	spinlock_t complock;
	atomic_t ref;
	/* data flow contrl */
	atomic_t flow;
};

struct sprdwl_xmit_msg_list {
	/*merge qos queues to this list*/
	struct list_head to_send_list;
	/*data list sending by HIF, will be freed later*/
	struct list_head to_free_list;
	spinlock_t send_lock;
	spinlock_t free_lock;
	u8 mode;
	unsigned long failcount;
};

struct sprdwl_msg_buf {
	struct list_head list;
	struct sk_buff *skb;
	/* data just tx cmd use,not include the head */
	void *data;
	void *tran_data;
	u8 type;
	u8 mode;
	u16 len;
	unsigned long timeout;
	/* marlin 2 */
	unsigned int fifo_id;
	struct sprdwl_msg_list *msglist;
	/* marlin 3 */
	unsigned char buffer_type;
	struct peer_list *data_list;
	struct sprdwl_xmit_msg_list *xmit_msg_list;
	unsigned char msg_type;
#if defined(MORE_DEBUG)
	unsigned long tx_start_time;
#endif
};

static inline void sprdwl_fill_msg(struct sprdwl_msg_buf *msg,
				   struct sk_buff *skb, void *data, u16 len)
{
	msg->skb = skb;
	msg->tran_data = data;
	msg->len = len;
}

static inline int sprdwl_msg_ref(struct sprdwl_msg_list *msglist)
{
	return atomic_read(&msglist->ref);
}

static inline int sprdwl_msg_tx_pended(struct sprdwl_msg_list *msglist)
{
	return !list_empty(&msglist->busylist);
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)
#endif

int sprdwl_msg_init(int num, struct sprdwl_msg_list *list);
void sprdwl_msg_deinit(struct sprdwl_msg_list *list);
struct sprdwl_msg_buf *sprdwl_alloc_msg_buf(struct sprdwl_msg_list *list);
void sprdwl_free_msg_buf(struct sprdwl_msg_buf *msg_buf,
			 struct sprdwl_msg_list *list);
void sprdwl_queue_msg_buf(struct sprdwl_msg_buf *msg_buf,
			  struct sprdwl_msg_list *list);
struct sprdwl_msg_buf *sprdwl_peek_msg_buf(struct sprdwl_msg_list *list);
void sprdwl_dequeue_msg_buf(struct sprdwl_msg_buf *msg_buf,
			    struct sprdwl_msg_list *list);
struct sprdwl_msg_buf *sprdwl_get_msgbuf_by_data(void *data,
						 struct sprdwl_msg_list *list);
struct sprdwl_msg_buf *sprdwl_get_tail_msg_buf(struct sprdwl_msg_list *list);
#endif
