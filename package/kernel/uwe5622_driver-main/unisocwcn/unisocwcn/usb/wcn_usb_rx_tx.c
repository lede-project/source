#include "wcn_usb.h"
#include "bus_common.h"
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <linux/list.h>
#include <linux/atomic.h>

#define TRANSF_UNITS 16
#define TRANSF_TOTAL 10

#define WCN_USB_MEMCOPY 64
#define WIFIDATA_OUT_ALIGNMENT 1600
#define WIFIDATA_IN_ALIGNMENT 1620

#define TRANSF_ALIGNMENT_MAX 1672
#define WCN_USB_BT_CTRL_TYPE 0x21
/* 10 HZ */
#define WCN_USB_TIMEOUT (10 * HZ)
char dirty_buff[TRANSF_ALIGNMENT_MAX];

struct rx_tx_pool {
	spinlock_t		lock;
	int			pool_size;
	void			*buf;
	struct list_head	remain_head;
	struct list_head	wait_free_head;
} rx_tx_pool;

static void wcn_usb_print_mbuf(int chn, struct mbuf_t *mbuf_head, int num,
		const char *func_name)
{
	struct mbuf_t *mbuf;
	int i;

	if (!(get_wcn_usb_print_switch() & mbuf_info))
		return;

	wcn_usb_print(mbuf_info, "%s chn %d\n", func_name, chn);
	mbuf_list_iter(mbuf_head, num, mbuf, i) {
		wcn_usb_print(mbuf_info, "mbuf %p ->buf %p ->len %d\n",
				mbuf, mbuf->buf, mbuf->len);
		if (!mbuf->buf)
			continue;
		print_hex_dump(KERN_INFO, "mbuf ", 0, 32, 1, mbuf->buf, 32, 1);
	}

}

/* we usb skbuff.c to manager rx buf */
static void *wcn_usb_alloc_buf(unsigned int size, gfp_t gfp_mask)
{
	void *buf;

	size = SKB_DATA_ALIGN(size);
	buf = netdev_alloc_frag(size);
	if (buf)
		channel_debug_net_malloc(1);
	return buf;

}

static void wcn_usb_free_buf(void *buf)
{
	if (buf == NULL)
		return;
	channel_debug_net_free(1);
	put_page(virt_to_head_page(buf));
}

static void *wcn_usb_kzalloc(unsigned int size, gfp_t gfp_mask)
{
	void *buf;

	buf = kzalloc(size, gfp_mask);
	if (buf)
		channel_debug_kzalloc(1);
	return buf;
}

static void wcn_usb_kfree(void *buf)
{
	if (buf == NULL)
		return;
	kfree(buf);
}

int wcn_usb_rx_tx_pool_init(void)
{
	int i = 0;
	struct wcn_usb_rx_tx *rx_tx;

	spin_lock_init(&(rx_tx_pool.lock));
	rx_tx_pool.pool_size = 500;
	rx_tx_pool.buf
		= wcn_usb_kzalloc(rx_tx_pool.pool_size *
				sizeof(struct wcn_usb_rx_tx), GFP_KERNEL);
	if (!rx_tx_pool.buf)
		return -ENOMEM;
	INIT_LIST_HEAD(&rx_tx_pool.wait_free_head);
	INIT_LIST_HEAD(&rx_tx_pool.remain_head);

	for (i = 0; i < rx_tx_pool.pool_size; i++) {
		rx_tx = rx_tx_pool.buf + i * sizeof(struct wcn_usb_rx_tx);
		rx_tx->packet = wcn_usb_alloc_packet(GFP_KERNEL);
		list_add_tail(&rx_tx->list, &rx_tx_pool.remain_head);
	}

	return 0;
}

void wcn_usb_rx_tx_pool_deinit(void)
{
	struct wcn_usb_rx_tx *rx_tx;

	spin_lock_bh(&(rx_tx_pool.lock));
	list_for_each_entry(rx_tx, &rx_tx_pool.wait_free_head, list)
		wcn_usb_packet_free(rx_tx->packet);
	list_for_each_entry(rx_tx, &rx_tx_pool.remain_head, list)
		wcn_usb_packet_free(rx_tx->packet);

	INIT_LIST_HEAD(&rx_tx_pool.wait_free_head);
	INIT_LIST_HEAD(&rx_tx_pool.remain_head);
	wcn_usb_kfree(rx_tx_pool.buf);

	spin_unlock_bh(&(rx_tx_pool.lock));
}

void wcn_usb_rx_tx_pool_checkout_freed(void)
{
	struct wcn_usb_rx_tx *rx_tx, *n;

	list_for_each_entry_safe(rx_tx, n, &rx_tx_pool.wait_free_head, list) {
		if (wcn_usb_packet_is_freed(rx_tx->packet))
			list_move_tail(&rx_tx->list, &rx_tx_pool.remain_head);
	}
}

struct wcn_usb_rx_tx *wcn_usb_rx_tx_pool_zalloc(void)
{
	struct wcn_usb_rx_tx *rx_tx;

	spin_lock_bh(&rx_tx_pool.lock);
	if (list_empty(&rx_tx_pool.remain_head))
		wcn_usb_rx_tx_pool_checkout_freed();

	if (list_empty(&rx_tx_pool.remain_head)) {
		spin_lock_bh(&rx_tx_pool.lock);
		return NULL;
	}

	rx_tx = list_first_entry(&rx_tx_pool.remain_head,
			struct wcn_usb_rx_tx, list);
	list_del(&rx_tx->list);

	/* clean */
	rx_tx->head = rx_tx->tail = NULL;
	rx_tx->num = rx_tx->channel = rx_tx->packet_status = 0;
	wcn_usb_packet_clean(rx_tx->packet);

	wcn_usb_print_rx_tx(rx_tx);

	spin_unlock_bh(&rx_tx_pool.lock);

	return rx_tx;
}
void wcn_usb_rx_tx_pool_free(struct wcn_usb_rx_tx *rx_tx)
{
	if (!rx_tx)
		return;

	spin_lock_bh(&rx_tx_pool.lock);
	list_add_tail(&rx_tx->list, &rx_tx_pool.wait_free_head);

	wcn_usb_rx_tx_pool_checkout_freed();
	spin_unlock_bh(&rx_tx_pool.lock);
}

static inline int wcn_usb_channel_is_rx(int channel)
{
	return channel >= 16;
}

/* There is BUG in MUSB_SPRD's inturrpt */
#ifndef NO_EXCHANGE_CHANNEL_17
static const char report_num_map_chn[] = {18, 20, 21, 22, 23, 24, 17, 31};
#else
static const char report_num_map_chn[] = {18, 20, 21, 22, 23, 24, 29, 31};
#endif

static inline int wcn_usb_channel_is_apostle(int chn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(report_num_map_chn); i++)
		if (chn == report_num_map_chn[i])
			return 1;
	return 0;
}

static inline int alignment_comm(int channel)
{
	switch (channel) {
	case 6:
		return WIFIDATA_OUT_ALIGNMENT;
	case 22:
		return WIFIDATA_IN_ALIGNMENT;
	case 25:
		return 20;
	default:
		return TRANSF_ALIGNMENT_MAX;
	}
}

/* NOTE! wcn WIFI __ASK__ mbuf that they recive must more than 2048byte!!! */
static inline int alignment_protocol_stack(int channel)
{
	switch (channel) {
	case 22:
		return 2048 - alignment_comm(channel);
	default:
		return 0;
	}
}

static inline int wcn_usb_channel_is_sg(int chn)
{
	return 0;
}

static inline int wcn_usb_channel_is_copy(int chn)
{
	return chn == 22 || chn == 6;
}

static inline int wcn_usb_rx_tx_need_resent(struct wcn_usb_rx_tx *rx_tx)
{
	return !rx_tx->packet_status && wcn_usb_channel_is_rx(rx_tx->channel);
}

struct wcn_usb_copy_kthread {
	struct task_struct	*wcn_usb_thread;
	spinlock_t		lock;
	struct completion	callback_complete;
	struct list_head	rx_tx_head;
	struct mbuf_t		*tx_mbuf_head;
	struct mbuf_t		*tx_mbuf_tail;
} copy_work[2];


static void wcn_usb_callback(struct wcn_usb_packet *packet)
{
	struct wcn_usb_rx_tx *rx_tx;
	struct list_head *rx_tx_head;
	struct completion *callback_complete;
	spinlock_t *list_lock;

	rx_tx = wcn_usb_packet_get_pdata(packet);
	channel_debug_rx_tx_from_controller(rx_tx->channel, 1);

	rx_tx->packet_status = wcn_usb_packet_get_status(packet);

	if (wcn_usb_channel_is_copy(rx_tx->channel) &&
			wcn_usb_channel_is_rx(rx_tx->channel)) {
		struct wcn_usb_copy_kthread *rx_copy;

		rx_copy = &copy_work[1];
		rx_tx_head = &rx_copy->rx_tx_head;
		callback_complete = &rx_copy->callback_complete;
		list_lock = &rx_copy->lock;
	} else {
		struct wcn_usb_work_data *work_data;

		work_data = wcn_usb_store_get_channel_info(rx_tx->channel);
		if (!work_data) {
			wcn_usb_err("%s channel[%d] work_data miss\n",
					__func__, rx_tx->channel);
			return;
		}
		rx_tx_head = &work_data->rx_tx_head;
		callback_complete = &work_data->callback_complete;
		list_lock = &work_data->lock;
	}

	spin_lock(list_lock);
	list_add_tail(&rx_tx->list, rx_tx_head);
	spin_unlock(list_lock);

	complete(callback_complete);
#if 0
	ret = schedule_work(&work_data->wcn_usb_work);
#endif
}

static struct scatterlist *wcn_usb_mbuf2sgs(struct mbuf_t *head,
		struct mbuf_t *tail, int num, int align,
		unsigned int *total_len, int *sgs_num)
{
	struct mbuf_t *mbuf;
	unsigned short mbuf_len;
	int i = 0, j = 0;
	int extra_sg = 0;
	struct scatterlist *sgs;

	mbuf_list_iter(head, num, mbuf, i) {
		if (mbuf->len < align)
			extra_sg++;
	}

	*sgs_num = extra_sg + num;
	sgs = wcn_usb_kzalloc((*sgs_num) * sizeof(struct scatterlist),
			GFP_KERNEL);
	if (!sgs)
		return NULL;

	sg_init_table(sgs, *sgs_num);

	mbuf_list_iter(head, num, mbuf, i) {
		mbuf_len = min_t(unsigned short, mbuf->len, align);
		sg_set_buf(sgs + j++, mbuf->buf, mbuf_len);
		*total_len += mbuf_len;
		if (mbuf_len < align) {
			sg_set_buf(sgs + j++, dirty_buff, align - mbuf_len);
			*total_len += align - mbuf_len;
		}
	}
	return sgs;
}


static int wcn_usb_sent_mbuf_sg_all(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct wcn_usb_rx_tx *rx_tx;
	struct wcn_usb_ep *ep;
	int ret = 0;
	struct scatterlist *sgs;
	int sgs_num;
	unsigned int total_len = 0;
	int align;

	ep = wcn_usb_store_get_epFRchn(chn);
	if (!ep)
		return -ENODEV;

	rx_tx = wcn_usb_rx_tx_pool_zalloc();
	channel_debug_rx_tx_alloc(chn, 1);
	if (!rx_tx)
		return -ENOMEM;

	ret = wcn_usb_packet_bind(rx_tx->packet, ep, GFP_KERNEL);
	if (ret)
		goto SG_FREE_RX_TX;

	align = alignment_comm(chn);
	sgs = wcn_usb_mbuf2sgs(head, tail, num, align, &total_len, &sgs_num);
	if (!sgs) {
		ret = -ENOMEM;
		goto SG_FREE_RX_TX;
	}

	rx_tx->channel = chn;
	rx_tx->head = head;
	rx_tx->tail = tail;
	rx_tx->num = num;

	ret = wcn_usb_packet_set_sg(rx_tx->packet, sgs, sgs_num, total_len);
	if (ret)
		goto SG_FREE_RX_TX;

	wcn_usb_print_rx_tx(rx_tx);
	channel_debug_rx_tx_to_controller(chn, 1);
	ret = wcn_usb_packet_submit(rx_tx->packet, wcn_usb_callback,
			rx_tx, GFP_KERNEL);
	if (!ret)
		return ret;

	wcn_usb_kfree(sgs);
SG_FREE_RX_TX:
	wcn_usb_rx_tx_pool_free(rx_tx);
	channel_debug_rx_tx_free(chn, 1);
	return ret;
}

static int wcn_usb_mbuf_list_destroy(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num);

/**
 * This is a secondary function.
 * It first try to pop_link mbuf to user,
 * Or try to free mbuf list and buf of mbuf->buf
 */
static void wcn_usb_deal_partial_fail(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct mchn_ops_t *mchn_ops;
	int ret;

	mutex_lock(&wcn_usb_store_get_channel_info(chn)->channel_lock);
	mchn_ops = chn_ops(chn);
	if (!wcn_usb_channel_is_rx(chn) && mchn_ops && mchn_ops->pop_link) {
		channel_debug_mbuf_to_user(chn, num);
		ret = mchn_ops->pop_link(chn, head, tail, num);
		if (ret)
			wcn_usb_err("%s:%d channel[%d] pop_link error[%d]\n",
					__func__, __LINE__, chn, ret);
	} else {
		if (!wcn_usb_channel_is_rx(chn))
			wcn_usb_err("%s %d pop_link mis\n", __func__, __LINE__);
		wcn_usb_mbuf_list_destroy(chn, head, tail, num);
	}
	mutex_unlock(&wcn_usb_store_get_channel_info(chn)->channel_lock);
}

/**
 * C is difficult to deal with "partial failure"
 * so this function and `wcn_usb_sent_mbuf` function
 * only return 0: successs or other_value: "total failure"
 * if we at partial failure, We will try to fix it by self,
 * AND RETURN 0
 */
static int wcn_usb_sent_mbuf_sg_dispack(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num, int unit_size)
{
	struct mbuf_t *mbuf;
	struct mbuf_t *temp_head, *temp_tail;
	int ret;
	int i;
	int temp_num;

	temp_head = head;
	for (; num > 0; num -= unit_size) {
		temp_num = min(num, unit_size);
		/* find last */
		mbuf_list_iter(temp_head, temp_num - 1, mbuf, i);
		if (mbuf == NULL) {
			wcn_usb_err("%s error tail\n", __func__);
			wcn_usb_deal_partial_fail(chn, temp_head, tail, num);
		}
		temp_tail = mbuf;
		mbuf = mbuf->next; /* keep next head */
		temp_tail->next = NULL; /* cut list */
		ret = wcn_usb_sent_mbuf_sg_all(chn, temp_head, temp_tail,
				temp_num);
		if (ret) {
			wcn_usb_err("%s submit is error %d\n", __func__, ret);
			wcn_usb_deal_partial_fail(chn, temp_head, tail, num);
		}
		temp_head = mbuf;
	}
	return 0;
}

static inline struct mbuf_t *wcn_usb_mbuf_stack_pop(struct mbuf_t **head)
{
	struct mbuf_t *mbuf;

	if (*head == NULL)
		return NULL;
	mbuf = *head;
	*head = (*head)->next;
	return mbuf;
}

static inline void wcn_usb_mbuf_stack_push(struct mbuf_t **head,
		struct mbuf_t *mbuf)
{
	mbuf->next = *head;
	*head = mbuf;
}

static int wcn_usb_sent_mbuf(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct wcn_usb_rx_tx *rx_tx;
	struct wcn_usb_ep *ep;
	struct mbuf_t *mbuf;
	int ret;

	ep = wcn_usb_store_get_epFRchn(chn);
	if (!ep)
		return -ENODEV;

	/* we need carefull in this loop */
	/* this mbuf and this rx_tx is not belong to us when we submit it */
	/* so we cant do mbuf = mbuf->next after we submit it!!!*/
	while ((mbuf = wcn_usb_mbuf_stack_pop(&head)) != NULL && num-- >= 0) {
		rx_tx = wcn_usb_rx_tx_pool_zalloc();
		if (!rx_tx)
			goto FREE_RX_TX;
		channel_debug_rx_tx_alloc(chn, 1);
		ret = wcn_usb_packet_bind(rx_tx->packet, ep, GFP_KERNEL);
		if (ret)
			goto FREE_RX_TX;

		rx_tx->head = mbuf;
		rx_tx->tail = mbuf;
		rx_tx->num = 1;
		rx_tx->channel = chn;

		ret = wcn_usb_packet_set_buf(rx_tx->packet, mbuf->buf,
				mbuf->len, GFP_ATOMIC);
		if (ret)
			goto FREE_RX_TX;

		wcn_usb_print_rx_tx(rx_tx);
		channel_debug_rx_tx_to_controller(chn, 1);
		ret = wcn_usb_packet_submit(rx_tx->packet,
				wcn_usb_callback, rx_tx, GFP_ATOMIC);
		if (ret) {
			/* if submit is failed, rx_tx_temp is nobody belonged,
			 * so we collect back it
			 */
			wcn_usb_mbuf_stack_push(&head, mbuf);
			wcn_usb_err("%s channel[%d] submit error[%d]\n",
					__func__, chn, ret);
			goto FREE_RX_TX;
		}
	}

	return 0;
FREE_RX_TX:
	channel_debug_rx_tx_free(chn, 1);
	wcn_usb_rx_tx_pool_free(rx_tx);
	wcn_usb_err("%s channel[%d] not sent all list, remain mbuf[%d],ret:%d\n",
			__func__, chn, num + 1, ret);
	wcn_usb_deal_partial_fail(chn, mbuf, tail, num + 1);

	return 0;
}

struct wcn_ctrl {
	struct usb_ctrlrequest dr;
	struct mbuf_t *head;
};

void ctrl_callback(struct urb *urb)
{
	struct mchn_ops_t *mchn_ops;
	struct wcn_ctrl *wcn_c = (struct wcn_ctrl *)(urb->context);

	mchn_ops = chn_ops(0);
	if (urb->status || !mchn_ops || !mchn_ops->pop_link) {
		wcn_usb_err("%s ctrl channel is error\n", __func__);
		goto CTRL_OUT;
	}
	channel_debug_mbuf_to_user(0, 1);
	mchn_ops->pop_link(0, wcn_c->head, wcn_c->head, 1);
CTRL_OUT:
	usb_free_urb(urb);
	wcn_usb_kfree(wcn_c);
}

struct {
	struct list_head big_men_head;
	spinlock_t list_lock;
} big_men_head;

struct wcn_usb_big_men {
	struct list_head list;
	char buf[0];
};

#define LOCK_FREE_BIG_BUF(at) \
	__atomic_add_unless(at, 1, 1)

static void *wcn_usb_get_big_men(int chn)
{
	void *buf = NULL;
	struct wcn_usb_big_men *big_men;

	spin_lock_irq(&big_men_head.list_lock);
	big_men = list_first_entry_or_null(&big_men_head.big_men_head,
			struct wcn_usb_big_men, list);
	if (!big_men) {
		buf = NULL;
		wcn_usb_err("%s no big buf!!\n", __func__);
	} else {
		buf = big_men->buf;
		list_del(&big_men->list);
		channel_debug_alloc_big_men(chn);
	}
	spin_unlock_irq(&big_men_head.list_lock);
	return buf;
}

static void wcn_usb_put_big_men(void *buf, int chn)
{
	struct wcn_usb_big_men *big_men = container_of(buf,
			struct wcn_usb_big_men, buf);

	spin_lock_irq(&big_men_head.list_lock);
	list_add(&big_men->list,  &big_men_head.big_men_head);
	spin_unlock_irq(&big_men_head.list_lock);
	channel_debug_free_big_men(chn);
}

static int rx_copy_work_func(void *work)
{
	struct wcn_usb_copy_kthread *copy_kthread;
	struct wcn_usb_rx_tx *rx_tx, *n;
	int recv_len, total_len;
	void *buf;
	struct mbuf_t *mbuf;
	int i;
	struct wcn_usb_work_data *work_data;
	struct list_head rx_tx_head;

	do {
		struct sched_param param;

		param.sched_priority = -20;
		sched_setscheduler(current, SCHED_FIFO, &param);
	} while (0);

	copy_kthread = (struct wcn_usb_copy_kthread *)work;


GET_RX_TX_HEAD:
	reinit_completion(&copy_kthread->callback_complete);
	INIT_LIST_HEAD(&rx_tx_head);

	spin_lock_irq(&copy_kthread->lock);
	list_splice_init(&copy_kthread->rx_tx_head, &rx_tx_head);
	spin_unlock_irq(&copy_kthread->lock);

	list_for_each_entry_safe(rx_tx, n, &rx_tx_head, list) {
		total_len = recv_len = wcn_usb_packet_recv_len(rx_tx->packet);
		buf = wcn_usb_packet_pop_buf(rx_tx->packet);
		if (!rx_tx->packet_status) {
			mbuf_list_iter(rx_tx->head, rx_tx->num, mbuf, i) {
				if (recv_len <= 0)
					break;
				memcpy(mbuf->buf, buf + (total_len - recv_len),
					alignment_comm(rx_tx->channel));
				recv_len -= alignment_comm(rx_tx->channel);
			}
		}

		wcn_usb_put_big_men(buf, rx_tx->channel);
		work_data = wcn_usb_store_get_channel_info(rx_tx->channel);
		if (work_data) {
			spin_lock_irq(&work_data->lock);
			list_move_tail(&rx_tx->list, &work_data->rx_tx_head);
			spin_unlock_irq(&work_data->lock);
			complete(&work_data->callback_complete);
		} else {
			wcn_usb_err("%s channel[%d] work_data miss\n",
					__func__, rx_tx->channel);
			WARN_ON(1);
		}
	}

	wait_for_completion(&copy_kthread->callback_complete);
	goto GET_RX_TX_HEAD;

	return 0;
}

static int wcn_usb_sent_mbuf_copy(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct wcn_usb_copy_kthread *copy_kthread;

	copy_kthread = &copy_work[0];

	spin_lock(&copy_kthread->lock);
	if (!copy_kthread->tx_mbuf_head) {
		copy_kthread->tx_mbuf_head = head;
		copy_kthread->tx_mbuf_tail = tail;
	} else {
		copy_kthread->tx_mbuf_tail->next = head;
		copy_kthread->tx_mbuf_tail = tail;
	}

	copy_kthread->tx_mbuf_tail->next = NULL;
	spin_unlock(&copy_kthread->lock);

	complete(&copy_kthread->callback_complete);
	return 0;
}

static void wcn_usb_callback_copy_tx(struct wcn_usb_packet *packet)
{
	struct wcn_usb_rx_tx *rx_tx;

	rx_tx = wcn_usb_packet_get_pdata(packet);
	wcn_usb_put_big_men(wcn_usb_packet_pop_buf(packet), rx_tx->channel);
	return wcn_usb_callback(packet);
}

static int wcn_usb_copy_submit_rx_tx(struct wcn_usb_rx_tx *rx_tx,
		void *buf, int buf_size);
static int tx_copy_work_func(void *work)
{
	struct wcn_usb_copy_kthread *copy_kthread;
	struct wcn_usb_rx_tx *rx_tx = NULL;
	struct mbuf_t *mbuf;
	void *buf = NULL;
	int buf_size = 0;
	int ret;
	int mbuf_num;


	do {
		struct sched_param param;

		param.sched_priority = -20;
		sched_setscheduler(current, SCHED_FIFO, &param);
	} while (0);

	copy_kthread = (struct wcn_usb_copy_kthread *)work;

GET_NEXT_MBUF:
	reinit_completion(&copy_kthread->callback_complete);
	spin_lock(&copy_kthread->lock);
	mbuf = wcn_usb_mbuf_stack_pop(&copy_kthread->tx_mbuf_head);
	spin_unlock(&copy_kthread->lock);

	if (!mbuf) {
		if (rx_tx) {
			ret = wcn_usb_copy_submit_rx_tx(rx_tx, buf, buf_size);
			if (ret) {
				wcn_usb_err("%s %d submit error %d\n",
						__func__, __LINE__, ret);
				wcn_usb_put_big_men(buf, rx_tx->channel);
				wcn_usb_rx_tx_pool_free(rx_tx);
			}
			rx_tx = NULL;
			buf = NULL;
			buf_size = 0;
		}
		wait_for_completion(&copy_kthread->callback_complete);
		goto GET_NEXT_MBUF;
	}

	while (!rx_tx) {
		rx_tx = wcn_usb_rx_tx_pool_zalloc();
		if (!rx_tx) {
			wcn_usb_err("%s %d no rx_tx memory\n",
					__func__, __LINE__);
			usleep_range(100, 300);
		}
	}

	rx_tx->channel = 6;

	while (!buf) {
		buf = wcn_usb_get_big_men(rx_tx->channel);
		if (!buf)
			msleep(200);
	}

	memcpy(buf + buf_size, mbuf->buf, mbuf->len);
	buf_size += alignment_comm(rx_tx->channel);
	if (!rx_tx->head) {
		rx_tx->head = rx_tx->tail = mbuf;
	} else {
		rx_tx->tail->next = mbuf;
		rx_tx->tail = mbuf;
	}
	rx_tx->tail->next = NULL;
	rx_tx->num += 1;

	/**
	 * We need to avoid that send ZERO_LENGTH_PACKET(ZLP)
	 * because sprd-musb can't send URB_ZERO_PACKET FLAG
	 * and it will accidentally lose the packet that len == 0.
	 *
	 * Since mbuf->len == 1600 then it need send ZLP when
	 * rx_tx->num == 8 or rx_tx->num == 16. (num * len % 512 == 0)
	 * So we need to avoid rx_tx->num == 8 or rx_tx->num == 16.
	 */
	if (rx_tx->num == 7) {
		spin_lock(&copy_kthread->lock);
		mbuf_list_iter(copy_kthread->tx_mbuf_head, 2, mbuf, mbuf_num);
		spin_unlock(&copy_kthread->lock);
	}

	if (rx_tx->num >= 15 || (rx_tx->num == 7 && mbuf_num <= 1)) {
		ret = wcn_usb_copy_submit_rx_tx(rx_tx, buf, buf_size);
		if (ret) {
			wcn_usb_put_big_men(buf, rx_tx->channel);
			wcn_usb_err("%s %d submit error %d\n",
					__func__, __LINE__, ret);
			wcn_usb_rx_tx_pool_free(rx_tx);
		}
		rx_tx = NULL;
		buf = NULL;
		buf_size = 0;
	}

	goto GET_NEXT_MBUF;
	return 0;
}

static int wcn_usb_copy_submit_rx_tx(struct wcn_usb_rx_tx *rx_tx, void *buf,
		int buf_size)
{
	struct wcn_usb_ep *ep;
	int ret;

	ep = wcn_usb_store_get_epFRchn(rx_tx->channel);
	if (!ep)
		return -ENODEV;

	ret = wcn_usb_packet_bind(rx_tx->packet, ep, GFP_KERNEL);
	if (ret)
		return ret;

	ret = wcn_usb_packet_set_buf(rx_tx->packet, buf, buf_size, GFP_ATOMIC);
	if (ret)
		return ret;

	ret = wcn_usb_packet_submit(rx_tx->packet, wcn_usb_callback_copy_tx,
			rx_tx, GFP_ATOMIC);
	if (ret)
		return ret;

	return 0;
}

void wcn_usb_init_copy_men(void)
{
	int i;

	INIT_LIST_HEAD(&big_men_head.big_men_head);
	spin_lock_init(&big_men_head.list_lock);
	for (i = 0; i < WCN_USB_MEMCOPY; i++) {
		struct wcn_usb_big_men *big_men;

		big_men = wcn_usb_kzalloc(sizeof(struct wcn_usb_big_men) +
			TRANSF_ALIGNMENT_MAX * TRANSF_UNITS, GFP_KERNEL);
		list_add(&big_men->list, &big_men_head.big_men_head);
	}

	copy_work[0].wcn_usb_thread = kthread_create(tx_copy_work_func,
			&copy_work[0], "wcn_usb_thread_tx");
	copy_work[1].wcn_usb_thread = kthread_create(rx_copy_work_func,
			&copy_work[1], "wcn_copy_rx");
	for (i = 0; i < 2; i++) {
		spin_lock_init(&copy_work[i].lock);
		init_completion(&copy_work[i].callback_complete);
		INIT_LIST_HEAD(&copy_work[i].rx_tx_head);
		if (copy_work[i].wcn_usb_thread)
			wake_up_process(copy_work[i].wcn_usb_thread);
	}
}

static int wcn_usb_poll_copy_one(int chn)
{
	struct wcn_usb_rx_tx *rx_tx;
	struct wcn_usb_ep *ep;
	int ret;
	void *buf;
	struct mbuf_t *mbuf;
	int i;

	ep = wcn_usb_store_get_epFRchn(chn);
	if (!ep)
		return -ENODEV;

	channel_debug_rx_tx_alloc(chn, 1);
	rx_tx = wcn_usb_rx_tx_pool_zalloc();
	if (!rx_tx)
		return -ENOMEM;

	rx_tx->num = TRANSF_UNITS;
	ret = wcn_usb_list_alloc(chn, &rx_tx->head, &rx_tx->tail, &rx_tx->num);
	if (ret)
		goto FREE_RX_TX;

	rx_tx->channel = chn;

	if (rx_tx->num != TRANSF_UNITS) {
		ret = -ENOMEM;
		goto FREE_MBUF;
	}

	mbuf_list_iter(rx_tx->head, rx_tx->num, mbuf, i) {
		mbuf->len = alignment_comm(chn) + alignment_protocol_stack(chn);
		mbuf->buf = wcn_usb_alloc_buf(mbuf->len, GFP_KERNEL);
		if (!mbuf->buf) {
			ret = -ENOMEM;
			goto FREE_MBUF;
		}
	}

	ret = wcn_usb_packet_bind(rx_tx->packet, ep, GFP_KERNEL);
	if (ret)
		goto FREE_MBUF;

	buf = wcn_usb_get_big_men(chn);
	if (!buf) {
		ret = -ENOMEM;
		goto FREE_MBUF;
	}

	ret = wcn_usb_packet_set_buf(rx_tx->packet, buf,
		alignment_comm(chn) * rx_tx->num, GFP_ATOMIC);
	if (ret)
		goto FREE_BIG_MEN;

	channel_debug_rx_tx_to_controller(chn, 1);
	ret = wcn_usb_packet_submit(rx_tx->packet, wcn_usb_callback,
			rx_tx, GFP_ATOMIC);
	if (ret)
		goto FREE_BIG_MEN;

	return 0;

FREE_BIG_MEN:
	wcn_usb_put_big_men(buf, chn);
FREE_MBUF:
	wcn_usb_mbuf_list_destroy(chn, rx_tx->head, rx_tx->tail, rx_tx->num);
FREE_RX_TX:
	channel_debug_rx_tx_free(chn, 1);
	wcn_usb_rx_tx_pool_free(rx_tx);
	return ret;
}

static int wcn_usb_poll_copy(int chn, int urbs)
{
	int i;
	int ret = 0;

	for (i = 0; i < urbs; i++) {
		ret = wcn_usb_poll_copy_one(chn);
		if (ret)
			break;
	}

	if (i != 0)
		return urbs - i;
	else
		return ret;
}

inline int wcn_usb_push_list_tx(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct wcn_usb_ep *ep;

	wcn_usb_print_mbuf(chn, head, num, __func__);

	if (chn == 0)
		ep = wcn_usb_store_get_epFRchn(23);
	else
		ep = wcn_usb_store_get_epFRchn(chn);

	if (!ep || !ep->intf)
		return -ENODEV;

	if (chn == 0) {
		struct urb *urb = usb_alloc_urb(0, GFP_KERNEL);
		struct wcn_ctrl *dr;
		unsigned int pipe;
		struct usb_device *udev;

		udev = ep->intf->udev;
		dr = wcn_usb_kzalloc(sizeof(struct wcn_ctrl), GFP_KERNEL);
		if (!dr)
			return -ENOMEM;
		dr->dr.bRequestType = 0x21;
		dr->dr.bRequest = 0;
		dr->dr.wIndex = 0;
		dr->dr.wValue = 0;
		dr->dr.wLength = __cpu_to_le16(head->len);
		dr->head = head;

		pipe = usb_sndctrlpipe(udev, 0x00);

		usb_fill_control_urb(urb, udev, pipe, (void *)(&(dr->dr)),
				head->buf, head->len, ctrl_callback, dr);

		return usb_submit_urb(urb, GFP_KERNEL);
	}

	if (wcn_usb_channel_is_sg(chn))
		return wcn_usb_sent_mbuf_sg_dispack(chn, head, tail,
				num, TRANSF_UNITS);
	else if (wcn_usb_channel_is_copy(chn))
		return wcn_usb_sent_mbuf_copy(chn, head, tail, num);
	else
		return wcn_usb_sent_mbuf(chn, head, tail, num);
}

static int wcn_usb_mbuf_list_destroy(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct mbuf_t *mbuf;
	int i;

	mbuf_list_iter(head, num, mbuf, i) {
		wcn_usb_free_buf(mbuf->buf);
		mbuf->buf = NULL;
		mbuf->len = 0;
	}
	return wcn_usb_list_free(chn, head, tail, num);
}

inline int wcn_usb_push_list_rx(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	wcn_usb_print_mbuf(chn, head, num, __func__);
	return wcn_usb_mbuf_list_destroy(chn, head, tail, num);
}

static int wcn_usb_poll_sg(int chn, int urbs)
{
	struct mbuf_t *head, *tail;
	int num;
	int ret;
	struct mbuf_t *mbuf;
	int i, j;
	int err;


	/* TODO maybe we can call wcn_usb_sent_mbuf_sg_dispatch */
	for (i = 0; i < urbs; i++) {
		num = TRANSF_UNITS;
		ret = wcn_usb_list_alloc(chn, &head, &tail, &num);
		if (ret || num != TRANSF_UNITS) {
			if (ret)
				ret = -ENOMEM;
			else
				ret = urbs - i;
			if (head != NULL)
				goto POLL_FREE_MBUF_SG;
			else
				return ret;
		}

		mbuf_list_iter(head, num, mbuf, j) {
			mbuf->len = alignment_comm(chn) +
				alignment_protocol_stack(chn);
			mbuf->buf = wcn_usb_alloc_buf(mbuf->len, GFP_ATOMIC);
			if (!mbuf->buf)
				goto POLL_FREE_MBUF_SG;
		}

		ret = wcn_usb_sent_mbuf_sg_all(chn, head, tail, num);
		if (ret)
			goto POLL_FREE_MBUF_SG;
	}
	return 0;

POLL_FREE_MBUF_SG:
	err = wcn_usb_mbuf_list_destroy(chn, head, tail, num);
	if (err)
		wcn_usb_err("%s:%d channel[%d] list_destroy error[%d]\n",
					__func__, __LINE__, chn, err);
	wcn_usb_err("%s channel[%d] poll sg error, in urb[%d]\n",
			__func__, chn, i);
	if (i != 0)
		ret = urbs - i;
	return ret;
}

/*
 * if ret_value > 0 : ret_value = number not process yet (maybe no mbuf)
 * if ret_value < 0 : error
 * if ret_value = 0 : success all
 */
static int wcn_usb_poll(int chn, int urbs)
{
	struct mbuf_t *head, *tail;
	int num;
	int ret;
	struct mbuf_t *mbuf;
	int i;
	int err;

	num = urbs;
	ret = wcn_usb_list_alloc(chn, &head, &tail, &num);
	if (ret)
		return -ENOMEM;

	mbuf_list_iter(head, num, mbuf, i) {
		mbuf->len = alignment_comm(chn) + alignment_protocol_stack(chn);
		mbuf->buf = wcn_usb_alloc_buf(mbuf->len, GFP_KERNEL);
		if (!mbuf->buf) {
			ret = -ENOMEM;
			goto POLL_FREE_MBUF_BUF;
		}
	}

	ret = wcn_usb_sent_mbuf(chn, head, tail, num);
	if (ret)
		goto POLL_FREE_MBUF_BUF;

	return urbs - num;
POLL_FREE_MBUF_BUF:
	err = wcn_usb_mbuf_list_destroy(chn, head, tail, num);
	if (err)
		wcn_usb_err("%s:%d channel[%d] list_destroy error[%d]\n",
				__func__, __LINE__, chn, err);
	wcn_usb_err("%s channel[%d] poll error, in mbuf[%d]\n",
			__func__, chn, i);
	return ret;
}

static inline int wcn_usb_poll_rx(int chn, int urbs)
{
	if (wcn_usb_channel_is_sg(chn))
		return wcn_usb_poll_sg(chn, urbs);
	else if (wcn_usb_channel_is_copy(chn))
		return wcn_usb_poll_copy(chn, urbs);
	else
		return wcn_usb_poll(chn, urbs);
}

void wcn_usb_begin_poll_rx(int chn)
{
	struct wcn_usb_work_data *work_data;
	struct mchn_ops_t *mchn_ops;
	int ret;
	int urbs = 1;
	int total;

	if (wcn_usb_channel_is_apostle(chn))
		return;

	work_data = wcn_usb_store_get_channel_info(chn);
	if (!work_data) {
		wcn_usb_err("%s channel[%d] work_data is miss\n",
				__func__, chn);
		return;
	}

	mutex_lock(&work_data->channel_lock);
	mchn_ops = chn_ops(chn);
	if (!mchn_ops) {
		mutex_unlock(&work_data->channel_lock);
		wcn_usb_err("%s channel[%d] chn_ops is miss\n",
				__func__, chn);
		return;
	}
	total = mchn_ops->pool_size;
	mutex_unlock(&work_data->channel_lock);

	if (wcn_usb_channel_is_sg(chn))
		urbs = total / TRANSF_UNITS;
	else
		urbs = total;

	urbs = min(urbs, TRANSF_TOTAL);
	urbs = max(urbs, 1);
	ret = wcn_usb_poll_rx(chn, urbs);
	if (ret)
		wcn_usb_err("%s channel[%d] begin poll error[%d]\n",
				__func__, chn, ret);
}

void wcn_usb_mbuf_free_notif(int chn)
{
	struct wcn_usb_work_data *work_data;

	work_data = wcn_usb_store_get_channel_info(chn);
	if (!work_data) {
		wcn_usb_err("%s channel[%d] work_data is miss\n",
				__func__, chn);
		return;
	}

	complete(&work_data->callback_complete);
#if 0
	wake_up(&work_data->wait_mbuf);
#endif
}

void wcn_usb_wait_channel_stop(int chn)
{
	struct wcn_usb_work_data *work_data;
	struct wcn_usb_ep *ep;
	long time;
	int i = 3;

	work_data = wcn_usb_store_get_channel_info(chn);
	if (!work_data)
		return;

	ep = wcn_usb_store_get_epFRchn(chn);
	if (!ep)
		return;
STOP_AGAIN:
	work_data->goon = 0;
	wcn_usb_ep_stop(ep);
#if 0
	if (work_data->head)
		schedule_work(&work_data->wcn_usb_work);
#endif
	time = wait_event_timeout(work_data->work_completion,
			buf_list_is_full(chn), WCN_USB_TIMEOUT);
	if (!time) {
		wcn_usb_err("%s chn:%d wait work_completion timeout!\n",
			    __func__, chn);

		if (--i <= 0 && !wcn_usb_state_get(error_happen))
			return;
	}
	if (!buf_list_is_full(chn) && !wcn_usb_state_get(error_happen))
		goto STOP_AGAIN;
}

static int wcn_usb_rx_tx_parse(struct wcn_usb_rx_tx *rx_tx,
		struct mbuf_t **head, struct mbuf_t **tail, int *num)
{
	/* struct wcn_usb_ep *ep; */
	struct mbuf_t *mbuf;
	int i = 0;
	int recv_len, total_len;
	int ret;
	int recv_mbuf_num;

	*head = NULL;
	*tail = NULL;
	*num = 0;

	/* split list */
	rx_tx->tail->next = NULL;
	if (wcn_usb_channel_is_rx(rx_tx->channel) && !rx_tx->packet_status) {
		total_len = recv_len = wcn_usb_packet_recv_len(rx_tx->packet);
		if (!recv_len) {
			ret = -ENOENT;
			goto OUT;
		}

		if (wcn_usb_channel_is_copy(rx_tx->channel) ||
				wcn_usb_channel_is_sg(rx_tx->channel)) {
			recv_mbuf_num = DIV_ROUND_UP(recv_len,
					alignment_comm(rx_tx->channel));
			mbuf_list_iter(rx_tx->head, recv_mbuf_num - 1, mbuf, i);
		} else {
			recv_mbuf_num = 1;
			mbuf = rx_tx->head;
			mbuf->len = recv_len;
		}
		if (!mbuf) {
			wcn_usb_err("%s chn[%d] recv_len longer than we give\n",
					__func__, rx_tx->channel);
		} else {
			*head = rx_tx->head;
			*tail = mbuf;
			*num = i + 1;

			rx_tx->head = mbuf->next;
			mbuf->next = NULL;
			rx_tx->num -= *num;
			if (!rx_tx->num)
				rx_tx->tail = NULL;
		}

		switch (*num) {
		case 16:
		case 15:
		case 14:
		case 13:
		case 12:
		case 11:
		case 10:
			channel_debug_mbuf_10(*num);
			break;
		case 9:
		case 8:
			channel_debug_mbuf_8(*num);
			break;
		case 7:
		case 6:
		case 5:
		case 4:
			channel_debug_mbuf_4(*num);
			break;
		case 3:
		case 2:
		case 1:
			channel_debug_mbuf_1(*num);
			break;
		default:
			break;
		}

	} else if (!wcn_usb_channel_is_rx(rx_tx->channel)) {
		*head = rx_tx->head;
		*tail = rx_tx->tail;
		*num = rx_tx->num;

		rx_tx->head = NULL;
		rx_tx->tail = NULL;
		rx_tx->num = 0;
	}
	ret = rx_tx->packet_status;

OUT:
	if (rx_tx->num) {
		channel_debug_packet_no_full(rx_tx->num);
		if (wcn_usb_mbuf_list_destroy(rx_tx->channel,
					      rx_tx->head, rx_tx->tail,
					      rx_tx->num))
			wcn_usb_err("%s chn[%d] list_destroy error%d\n",
				    __func__, rx_tx->channel, ret);
	}

	return ret;
}

static void wcn_usb_rx_tx_list_parse(struct list_head *rx_tx_head,
		int *rx_tx_num, int *rx_tx_resent,
		struct mbuf_t **head, struct mbuf_t **tail, int *num)
{
	struct wcn_usb_rx_tx *rx_tx, *n;
	struct mbuf_t *temp_head, *temp_tail;
	int temp_num;
	int ret;

	*head = NULL;
	*tail = NULL;
	*num = 0;
	*rx_tx_num = 0;
	*rx_tx_resent = 0;
	list_for_each_entry_safe(rx_tx, n, rx_tx_head, list) {
		wcn_usb_print_rx_tx(rx_tx);
		(*rx_tx_num)++;
		ret = wcn_usb_rx_tx_parse(rx_tx, &temp_head,
					  &temp_tail, &temp_num);
		if (ret) {
			/* There fix rx urb only received a zero len packer,
			 * usb can't continue read data.
			 */
			if (ret == -ENOENT && wcn_usb_rx_tx_need_resent(rx_tx)
				&& wcn_usb_channel_is_apostle(rx_tx->channel))
				(*rx_tx_resent)++;
			else
				wcn_usb_err("%s channel[%d] rx_tx_parse error[%d]\n",
					__func__, rx_tx->channel, ret);
		} else if (wcn_usb_rx_tx_need_resent(rx_tx) &&
			!wcn_usb_channel_is_apostle(rx_tx->channel))
			(*rx_tx_resent)++;

		if (!*head) {
			*head = temp_head;
			*tail = temp_tail;
			*num = temp_num;
		} else if (temp_head) {
			(*tail)->next = temp_head;
			*tail = temp_tail;
			*num += temp_num;
		}
	}
}

static void wcn_usb_work_rx_tx_free(struct list_head *rx_tx_head)
{
	struct wcn_usb_rx_tx *rx_tx, *n;

	list_for_each_entry_safe(rx_tx, n, rx_tx_head, list) {
		channel_debug_rx_tx_free(rx_tx->channel, 1);
		kfree(wcn_usb_packet_pop_sg(rx_tx->packet, NULL));
		kfree(wcn_usb_packet_pop_setup_packet(rx_tx->packet));
		wcn_usb_rx_tx_pool_free(rx_tx);
	}
}

static unsigned long long wcn_usb_rx_tx_cnt;
unsigned long long wcn_usb_get_rx_tx_cnt(void)
{
	return wcn_usb_rx_tx_cnt;
}

static void wcn_usb_rx_trash(int chn, int num);
int wcn_usb_work_func(void *work)
{
	struct wcn_usb_work_data *work_data;
	struct list_head rx_tx_head;
	struct mbuf_t *head, *tail;
	struct mchn_ops_t *mchn_ops;
	int num = 0, rx_tx_num;
	int ret;

#if 0
	work_data = container_of(work, struct wcn_usb_work_data, wcn_usb_work);
#endif
	work_data = (struct wcn_usb_work_data *)work;

#if 0
	do {
		struct sched_param param;

		param.sched_priority = 1;
		sched_setscheduler(current, SCHED_FIFO, &param);
	} while (0);
#endif

GET_RX_TX_HEAD:
	reinit_completion(&work_data->callback_complete);
	INIT_LIST_HEAD(&rx_tx_head);

	spin_lock_irq(&work_data->lock);
	list_splice_init(&work_data->rx_tx_head, &rx_tx_head);
	if (work_data->report_num >= work_data->report_num_last)
		work_data->transfer_remains += work_data->report_num -
					       work_data->report_num_last;
	else
		work_data->transfer_remains +=
			USHRT_MAX - work_data->report_num_last +
			work_data->report_num + 1;
	work_data->report_num_last = work_data->report_num;
	spin_unlock_irq(&work_data->lock);

	wcn_usb_rx_tx_list_parse(&rx_tx_head, &rx_tx_num, &ret,
			&head, &tail, &num);

	work_data->transfer_remains += ret;

	if (head == NULL)
		goto RX_TX_LIST_HANDLE;

	mutex_lock(&work_data->channel_lock);
	mchn_ops = chn_ops(work_data->channel);
	if (mchn_ops && mchn_ops->pop_link) {
		wcn_usb_print_mbuf(work_data->channel, head, num, __func__);
		channel_debug_mbuf_to_user(work_data->channel, num);
		ret = mchn_ops->pop_link(work_data->channel, head, tail, num);
		if (ret)
			wcn_usb_err("%s channel[%d] pop_link error[%d]\n",
					__func__, work_data->channel, ret);
	} else {
		wcn_usb_err("%s channel[%d] pop_link miss\n",
				__func__, work_data->channel);
		ret = wcn_usb_mbuf_list_destroy(work_data->channel, head,
				tail, num);
		if (ret)
			wcn_usb_err("%s channel[%d] list_destroy error[%d]\n",
					__func__, work_data->channel, ret);
	}
	mutex_unlock(&work_data->channel_lock);

	if (wcn_usb_channel_is_rx(work_data->channel))
		wcn_usb_rx_tx_cnt += rx_tx_num;
RX_TX_LIST_HANDLE:
	wcn_usb_work_rx_tx_free(&rx_tx_head);
	if (work_data->transfer_remains) {
		if (chn_ops(work_data->channel) && work_data->goon) {
			ret = wcn_usb_poll_rx(work_data->channel,
					work_data->transfer_remains);
			if (ret < 0) {
				//wcn_usb_err("%s chn[%d] poll rx error[%d]\n",
				//	__func__, work_data->channel, ret);
				/*
				 * if ret == -ENOMEN then we can wait it,
				 * if ret != -ENOMEN that mean that is a
				 * serous error, then we drop all info
				 */
				if (ret != -ENOMEM)
					work_data->transfer_remains = 0;

			} else {
				channel_debug_to_accept(work_data->channel,
					work_data->transfer_remains - ret);
				work_data->transfer_remains = ret;
			}
		} else {
			wcn_usb_rx_trash(work_data->channel,
					 work_data->transfer_remains);
			work_data->transfer_remains = 0;
		}
	}

	wake_up(&work_data->work_completion);
	if (!work_data->transfer_remains)
		wait_for_completion(&work_data->callback_complete);
	else
		msleep(100);
	goto GET_RX_TX_HEAD;

	return 0;
}

void wcn_usb_work_data_init(struct wcn_usb_work_data *work_data, int id)
{
	work_data->channel = id;
	mutex_init(&work_data->channel_lock);
	spin_lock_init(&work_data->lock);
	init_waitqueue_head(&work_data->wait_mbuf);
	work_data->wcn_usb_thread = kthread_create(wcn_usb_work_func, work_data,
			"wcn_thread%d", id);
	init_waitqueue_head(&work_data->work_completion);
	init_completion(&work_data->callback_complete);
	INIT_LIST_HEAD(&work_data->rx_tx_head);
	if (work_data->wcn_usb_thread)
		wake_up_process(work_data->wcn_usb_thread);
	else
		wcn_usb_err("%s create a new thread failed\n", __func__);
}

struct wcn_usb_rx_apostle {
	struct wcn_usb_packet *packet;
	void *buf;
	int buf_size;
	int chn;
};

int wcn_usb_apostle_fire(int chn, void (*fn)(struct wcn_usb_packet *packet))
{
	struct wcn_usb_rx_apostle *apostle;
	int ret = 0;
	struct wcn_usb_ep *ep;
	int i;

	wcn_usb_info("%s report num map is ", __func__);
	for (i = 0; i < ARRAY_SIZE(report_num_map_chn); i++)
		wcn_usb_info("%d ", report_num_map_chn[i]);
	wcn_usb_info("\n");

	ep = wcn_usb_store_get_epFRchn(chn);
	if (!ep)
		return -ENODEV;

	apostle = wcn_usb_kzalloc(sizeof(struct wcn_usb_rx_apostle),
			GFP_KERNEL);
	if (!apostle)
		return -ENOMEM;

	apostle->chn = chn;
	apostle->packet = wcn_usb_alloc_packet(GFP_KERNEL);
	if (!apostle->packet) {
		ret = -ENOMEM;
		goto FREE_APOSTLE;
	}

	ret = wcn_usb_packet_bind(apostle->packet, ep, GFP_KERNEL);
	if (ret)
		goto FREE_APOSTLE_PACKET;

	apostle->buf_size = alignment_comm(chn) + alignment_protocol_stack(chn);
	/* this code is unnecessary, if apostle chn is not sg mode */
	if (wcn_usb_channel_is_sg(chn) || wcn_usb_channel_is_copy(chn))
		apostle->buf_size = apostle->buf_size * TRANSF_UNITS + 1;

	apostle->buf = wcn_usb_kzalloc(apostle->buf_size, GFP_KERNEL);
	if (!apostle->buf) {
		ret = -ENOMEM;
		goto FREE_APOSTLE_PACKET;
	}

	ret = wcn_usb_packet_set_buf(apostle->packet, apostle->buf,
			apostle->buf_size, GFP_KERNEL);
	if (ret)
		goto FREE_APOSTLE_BUF;

	ret = wcn_usb_packet_submit(apostle->packet, fn, apostle, GFP_ATOMIC);
	if (ret)
		goto FREE_APOSTLE_BUF;
	return ret;

FREE_APOSTLE_BUF:
	wcn_usb_kfree(apostle->buf);
FREE_APOSTLE_PACKET:
	wcn_usb_packet_free(apostle->packet);
FREE_APOSTLE:
	wcn_usb_kfree(apostle);
	return ret;
}

static void wcn_usb_rx_apostle_free(struct wcn_usb_rx_apostle *apostle)
{
	wcn_usb_kfree(apostle->buf);
	wcn_usb_packet_free(apostle->packet);
	wcn_usb_kfree(apostle);
}

static void wcn_usb_rx_trash_callback(struct wcn_usb_packet *packet)
{
	struct wcn_usb_rx_apostle *apostle;

	apostle = wcn_usb_packet_get_pdata(packet);
	wcn_usb_rx_apostle_free(apostle);
}

static void wcn_usb_rx_trash(int chn, int num)
{
	int i;

	for (i = 0; i < num; i++)
		wcn_usb_apostle_fire(chn, wcn_usb_rx_trash_callback);
}

struct int_info {
	unsigned int count;
	unsigned short report_num[6];
};

#define loop_check_cmd "at+loopcheck\r"
static void loop_check_callback(struct urb *urb)
{
	kfree(urb->transfer_buffer);
	usb_free_urb(urb);
}

static void start_loop_check(void)
{
	void *buf = kzalloc(strlen(loop_check_cmd) + 1, GFP_ATOMIC);
	struct urb *urb = usb_alloc_urb(0, GFP_ATOMIC);
	struct wcn_usb_ep *ep = wcn_usb_store_get_epFRchn(7);
	unsigned int pipe;
	struct usb_device *udev;
	struct usb_endpoint_descriptor *endpoint;

	if (!ep || !ep->intf || !ep->numEp) {
		wcn_usb_err("%s start loopcheck ep is error\n", __func__);
		return;
	}

	endpoint = wcn_usb_intf2endpoint(ep->intf, ep->numEp);
	if (!endpoint) {
		wcn_usb_err("%s start loopcheck endpoint is error\n", __func__);
		return;
	}

	udev = ep->intf->udev;
	pipe = usb_sndbulkpipe(udev, endpoint->bEndpointAddress);
	strncpy(buf, loop_check_cmd, strlen(loop_check_cmd));
	usb_fill_bulk_urb(urb, udev, pipe, buf, strlen(loop_check_cmd) + 1,
				     loop_check_callback, buf);
	if (usb_submit_urb(urb, GFP_KERNEL))
		wcn_usb_err("%s start loopcheck error\n", __func__);
}

static void wcn_usb_rx_apostle_callback(struct wcn_usb_packet *packet)
{
	struct wcn_usb_rx_apostle *apostle;
	int i;
	int total_len;
	int ret;
	struct wcn_usb_work_data *work_data;
	static unsigned int interrupt_count;
	char log_info[64];
	struct int_info *apostle_info = NULL;

	channel_debug_interrupt_callback(1);
	apostle = wcn_usb_packet_get_pdata(packet);
	ret = wcn_usb_packet_get_status(packet);
	if (ret) {
		wcn_usb_info_ratelimited("%s get apostle error[%d]\n",
					 __func__, ret);
		goto RESUBMIT_PACKET;
	}
	total_len = wcn_usb_packet_recv_len(packet);
	if (total_len != apostle->buf_size) {
		wcn_usb_err("%s apostle->buf_size[0x%x] total_len[0x%x]\n",
				__func__, apostle->buf_size, total_len);
		if (get_wcn_usb_print_switch() & packet_info) {
			snprintf(log_info, 32, "wcn usb_interrupt_msg %d ",
					interrupt_count);
			print_hex_dump(KERN_ERR, log_info, 0, apostle->buf_size,
				       1, apostle->buf, apostle->buf_size, 0);
		}

		start_loop_check();
		goto RESUBMIT_PACKET;
	}

	interrupt_count++;
	if (get_wcn_usb_print_switch() & packet_info) {
		snprintf(log_info, 32, "wcn usb_interrupt_msg %d ",
				interrupt_count);
		print_hex_dump(KERN_ERR, log_info, 0, apostle->buf_size, 1,
				apostle->buf, apostle->buf_size, 0);
	}

	apostle_info = (struct int_info *)(apostle->buf);
	for (i = 0; i < ARRAY_SIZE(report_num_map_chn) - 1; i++) {
		int channel;

		channel = report_num_map_chn[i];
		work_data = wcn_usb_store_get_channel_info(channel);
		spin_lock(&work_data->lock);
		work_data->report_num = apostle_info->report_num[i];
		spin_unlock(&work_data->lock);

		channel_debug_report_num(channel, apostle_info->report_num[i]);
		complete(&work_data->callback_complete);
	}
	channel_debug_cp_num(apostle_info->count);
RESUBMIT_PACKET:
	if (wcn_usb_state_get(error_happen)) {
		wcn_usb_rx_apostle_free(apostle);
		wcn_usb_err("%s assert is happen!!!\n", __func__);
		return;
	}

	if (apostle_info && apostle_info->report_num[ARRAY_SIZE(
						report_num_map_chn) - 1] > 0) {
		wcn_usb_info("%s recv sync 0x%x\n", __func__,
			apostle_info->report_num[ARRAY_SIZE(
						report_num_map_chn) - 1]);
		wcn_usb_state_sent_event(cp_ready);
	} else if (!wcn_usb_state_get(pwr_state)
		   && wcn_usb_state_get(cp_ready)) {
		wcn_usb_rx_apostle_free(apostle);
		wcn_usb_err("%s power off!!!\n", __func__);
		return;
	}

	ret = wcn_usb_packet_submit(apostle->packet,
			wcn_usb_rx_apostle_callback, apostle, GFP_ATOMIC);
	if (ret) {
		wcn_usb_rx_apostle_free(apostle);
		wcn_usb_err("%s submit error %d\n", __func__, ret);
	}
}

int wcn_usb_apostle_begin(int chn)
{
	return wcn_usb_apostle_fire(chn, wcn_usb_rx_apostle_callback);
}

