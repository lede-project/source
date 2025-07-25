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

#include "sprdwl.h"
#include "msg.h"
#include "wcn_wrapper.h"
/* static struct sprdwl_msg_list msg_list */

int sprdwl_msg_init(int num, struct sprdwl_msg_list *list)
{
	int i;
	struct sprdwl_msg_buf *msg_buf;
	struct sprdwl_msg_buf *pos;

	if (!list)
		return -EPERM;
	INIT_LIST_HEAD(&list->freelist);
	INIT_LIST_HEAD(&list->busylist);
	INIT_LIST_HEAD(&list->cmd_to_free);
	list->maxnum = num;
	spin_lock_init(&list->freelock);
	spin_lock_init(&list->busylock);
	spin_lock_init(&list->complock);
	atomic_set(&list->ref, 0);
	atomic_set(&list->flow, 0);
	for (i = 0; i < num; i++) {
		msg_buf = kzalloc(sizeof(*msg_buf), GFP_KERNEL);
		if (msg_buf) {
			INIT_LIST_HEAD(&msg_buf->list);
			list_add_tail(&msg_buf->list, &list->freelist);
		} else {
			wl_err("%s failed to alloc msg_buf!\n", __func__);
			goto err_alloc_buf;
		}
	}

	return 0;

err_alloc_buf:
	list_for_each_entry_safe(msg_buf, pos, &list->freelist, list) {
		list_del(&msg_buf->list);
		kfree(msg_buf);
	}
	return -ENOMEM;
}

#define SPRDWL_MSG_EXIT_VAL 0x8000
void sprdwl_msg_deinit(struct sprdwl_msg_list *list)
{
	struct sprdwl_msg_buf *msg_buf;
	struct sprdwl_msg_buf *pos;
	struct timespec txmsgftime1, txmsgftime2;

	atomic_add(SPRDWL_MSG_EXIT_VAL, &list->ref);
	if (atomic_read(&list->ref) > SPRDWL_MSG_EXIT_VAL)
		wl_err("%s ref not ok! wait for pop!\n", __func__);

	getnstimeofday(&txmsgftime1);
	while (atomic_read(&list->ref) > SPRDWL_MSG_EXIT_VAL) {
		getnstimeofday(&txmsgftime2);
		if (((unsigned long)(timespec_to_ns(&txmsgftime2) -
			timespec_to_ns(&txmsgftime1))/1000000) > 3000)
			break;
		usleep_range(2000, 2500);
	}

	wl_info("%s list->ref ok!\n", __func__);

	if (!list_empty(&list->busylist))
		WARN_ON(1);

	list_for_each_entry_safe(msg_buf, pos, &list->freelist, list) {
		list_del(&msg_buf->list);
		kfree(msg_buf);
	}
}

struct sprdwl_msg_buf *sprdwl_alloc_msg_buf(struct sprdwl_msg_list *list)
{
	struct sprdwl_msg_buf *msg_buf = NULL;

	if (atomic_inc_return(&list->ref) >= SPRDWL_MSG_EXIT_VAL) {
		atomic_dec(&list->ref);
		return NULL;
	}
	spin_lock_bh(&list->freelock);
	if (!list_empty(&list->freelist)) {
		msg_buf = list_first_entry(&list->freelist,
					   struct sprdwl_msg_buf, list);
		list_del(&msg_buf->list);
	}
	spin_unlock_bh(&list->freelock);

	if (!msg_buf)
		atomic_dec(&list->ref);
	return msg_buf;
}

void sprdwl_free_msg_buf(struct sprdwl_msg_buf *msg_buf,
			 struct sprdwl_msg_list *list)
{
	spin_lock_bh(&list->freelock);
	list_add_tail(&msg_buf->list, &list->freelist);
	atomic_dec(&list->ref);
	spin_unlock_bh(&list->freelock);
}

void sprdwl_queue_msg_buf(struct sprdwl_msg_buf *msg_buf,
			  struct sprdwl_msg_list *list)
{
	spin_lock_bh(&list->busylock);
	list_add_tail(&msg_buf->list, &list->busylist);
	spin_unlock_bh(&list->busylock);
}

struct sprdwl_msg_buf *sprdwl_peek_msg_buf(struct sprdwl_msg_list *list)
{
	struct sprdwl_msg_buf *msg_buf = NULL;

	spin_lock_bh(&list->busylock);
	if (!list_empty(&list->busylist))
		msg_buf = list_first_entry(&list->busylist,
				   struct sprdwl_msg_buf, list);
	spin_unlock_bh(&list->busylock);

	return msg_buf;
}

void sprdwl_dequeue_msg_buf(struct sprdwl_msg_buf *msg_buf,
				struct sprdwl_msg_list *list)
{
	spin_lock_bh(&list->busylock);
	list_del(&msg_buf->list);
	spin_unlock_bh(&list->busylock);
	sprdwl_free_msg_buf(msg_buf, list);
}

struct sprdwl_msg_buf *sprdwl_get_msgbuf_by_data(void *data,
						 struct sprdwl_msg_list *list)
{
	int find = 0;
	struct sprdwl_msg_buf *pos;
	struct sprdwl_msg_buf *msg_buf;

	spin_lock_bh(&list->busylock);
	list_for_each_entry_safe(msg_buf, pos,  &list->busylist, list) {
		if (data == msg_buf->tran_data) {
			list_del(&msg_buf->list);
			find = 1;
			break;
		}
	}
	spin_unlock_bh(&list->busylock);

	return find ? msg_buf : NULL;
}

struct sprdwl_msg_buf *sprdwl_get_tail_msg_buf(struct sprdwl_msg_list *list)
{
	struct sprdwl_msg_buf *msg_buf = NULL;

	spin_lock_bh(&list->busylock);
	if (!list_empty(&list->busylist))
		msg_buf = list_last_entry(&list->busylist,
					struct sprdwl_msg_buf, list);
	spin_unlock_bh(&list->busylock);
	return msg_buf;
}

