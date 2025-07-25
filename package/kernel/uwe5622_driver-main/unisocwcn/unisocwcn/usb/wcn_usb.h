#ifndef WCN_USB_H
#define WCN_USB_H

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/bitops.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <wcn_bus.h>

#define wcn_usb_info(fmt, args...) \
	pr_info("wcn_usb info " fmt, ## args)

#define wcn_usb_info_ratelimited(fmt, args...) \
	printk_ratelimited("wcn_usb info " fmt, ## args)

#define wcn_usb_debug(fmt, args...) \
	pr_debug("wcn_usb debug " fmt, ## args)

#define wcn_usb_err(fmt, args...) \
	pr_err("wcn_usb err " fmt, ## args)

char get_wcn_usb_print_switch(void);
#define wcn_usb_print(mask, fmt, args...)	\
	do {					\
		if (get_wcn_usb_print_switch() & mask)\
			pr_info("wcn_usb info " fmt, ## args); \
	} while (0)
#define rx_tx_info 0x08
#define packet_info 0x04
#define mbuf_info 0x02
#define extern_info 0x01

#define wcn_usb_print_packet(packet) \
	wcn_usb_print(packet_info, \
			"%s+%d packet %p ->chn %d ->urb %p\n", \
			__func__, __LINE__, packet, \
		packet->ep ? packet->ep->channel : -1, packet->urb)

/**
 * struct wcn_usb_store_intf - The object inherited from struct usb_interface.
 * @interface: We inherited from it.
 * @udev: interface->usb_device.
 * @lock: keep { num_users change_flags is a atomic }.
 * @num_users: How many object need this intf now.
 * @flags_lock: keep { change_flags } only one can access;
 * @change_flag: Is this intf need be changed? (if it is, don't add num_users!)
 */
struct wcn_usb_intf {
	struct usb_interface *interface;
	struct usb_device *udev;
	spinlock_t lock;
	char num_users;
	struct mutex flag_lock;
	char change_flag;
	wait_queue_head_t wait_user;
};

/**
 * struct wcn_usb_ep - The object descrise a endpoint for wcn usb.
 * @intf_lock: keep { intf intf->users } is a atomic.
 * @intf: This endpoint must belong to a interface.
 * @epNum: This endpoint's index in interface's cur_altsetting->endpoint[]
 *	that it belong to.
 * @submitted: for retract urb;
 */
struct wcn_usb_ep {
	int channel;
	spinlock_t intf_lock;
	struct wcn_usb_intf *intf;
	__u8 numEp;
	struct usb_anchor submitted;
	spinlock_t submit_lock;
};

/**
 * struct wcn_usb_packet - The object for broke talk with io.
 * @urb: from usb_alloc_urb. user don't touch it!
 * @ep: record ep point.
 * @is_usb_anchor: to indicate this buf is alloc with usb_alloc_coherent.
 * @callback: be called after the packet transfer ok.
 * @pdata: to callback.
 *
 * NOTE: user don't touch any feild in this struct, unless you sure it is right
 * that what you do!
 */
struct wcn_usb_packet {
	struct urb *urb;
	struct wcn_usb_ep *ep;
	bool is_usb_anchor;
	void (*callback)(struct wcn_usb_packet *packet);
	void *pdata;
};

struct wcn_usb_rx_tx {
	struct wcn_usb_packet *packet;
	struct mbuf_t *head;
	struct mbuf_t *tail;
	int num;
	int channel;
	int packet_status;
	struct list_head list;
};

#define wcn_usb_print_rx_tx(rx_tx)		\
	wcn_usb_print(rx_tx_info, "%s+%d rx_tx %p ->packet %p ->head"\
		" %p ->tail %p ->num %d ->channel %d ->packet_status %d\n", \
		__func__, __LINE__, rx_tx, rx_tx->packet, \
		rx_tx->head, rx_tx->tail, rx_tx->num, \
		rx_tx->channel, rx_tx->packet_status)

struct wcn_usb_notifier {
	struct notifier_block nb;
	void (*cb)(void *);
	void *data;
	unsigned long event;
};

struct wcn_usb_work_data {
	struct task_struct	*wcn_usb_thread;
	wait_queue_head_t	wait_mbuf;
	wait_queue_head_t	work_completion;
	struct mutex		channel_lock;
	spinlock_t		lock;
	struct list_head	rx_tx_head;
	int			channel;
	int			report_num;
	int			report_num_last;
	int			transfer_remains;
	int			goon;
	struct completion	callback_complete;
	struct wcn_usb_notifier	*wn;
};


struct wcn_usb_work_data *wcn_usb_store_get_channel_info(int channel);
int wcn_usb_store_register_channel_info(int channel, void *info);
struct wcn_usb_ep *wcn_usb_store_get_epFRchn(int channel);
int wcn_usb_store_init(void);
void wcn_usb_store_delet(void);
__u8 wcn_usb_store_chn2addr(int channel);
struct usb_endpoint_descriptor *wcn_usb_intf2endpoint(struct wcn_usb_intf *intf,
					__u8 numEp);
typedef int (*ep_handle_cb)(struct wcn_usb_ep *ep, void *pdata);
int wcn_usb_store_travel_ep(ep_handle_cb cb, void *pdata);

int wcn_usb_packet_get_status(struct wcn_usb_packet *packet);
void wcn_usb_ep_stop(struct wcn_usb_ep *ep);
void wcn_usb_packet_free(struct wcn_usb_packet *packet);
void wcn_usb_io_delet(void);
int wcn_usb_io_init(void);
int wcn_usb_packet_set_buf(struct wcn_usb_packet *packet,
				void *buf, ssize_t buf_size,  gfp_t mem_flags);
void wcn_usb_packet_free_buf(struct wcn_usb_packet *packet);
int wcn_usb_packet_set_sg(struct wcn_usb_packet *packet, struct scatterlist *sg,
				int num_sgs, unsigned int buf_len);
struct wcn_usb_packet *wcn_usb_alloc_packet(gfp_t mem_flags);
struct scatterlist *wcn_usb_packet_pop_sg(struct wcn_usb_packet *packet,
						int *num_sgs);
int wcn_usb_msg(struct wcn_usb_ep *ep, void *data, int len,
		int *actual_length, int timeout);
int wcn_usb_packet_submit(struct wcn_usb_packet *packet,
		void (*callback)(struct wcn_usb_packet *packet),
		void *pdata, gfp_t mem_flags);
int wcn_usb_ep_can_dma(struct wcn_usb_ep *ep);
void wcn_usb_packet_interval(struct wcn_usb_packet *packet, int interval);
unsigned int wcn_usb_packet_recv_len(struct wcn_usb_packet *packet);
int wcn_usb_ep_set(struct wcn_usb_ep *ep, int setting_id);
void wcn_usb_wait_channel_stop(int chn);
int wcn_usb_packet_bind(struct wcn_usb_packet *packet, struct wcn_usb_ep *ep,
		gfp_t mem_flags);
void wcn_usb_packet_clean(struct wcn_usb_packet *packet);
int wcn_usb_packet_set_setup_packet(struct wcn_usb_packet *packet,
					struct usb_ctrlrequest *setup_packet);
struct usb_ctrlrequest *wcn_usb_packet_pop_setup_packet(
						struct wcn_usb_packet *packet);

#define wcn_usb_packet_get_buf(packet) \
	(packet->buf)
#define wcn_usb_ep_init(ep, id) \
	do { \
		ep->channel = id; \
		ep->intf = NULL; \
		ep->numEp = -1; \
		spin_lock_init(&ep->intf_lock); \
		init_usb_anchor(&ep->submitted); \
		spin_lock_init(&ep->submit_lock); \
	} while (0)

#if 0
#define wcn_usb_work_data_init(work_data, id) \
	do { \
		work_data->channel = id; \
		mutex_init(&work_data->channel_lock); \
		spin_lock_init(&work_data->lock); \
		init_waitqueue_head(&work_data->wait_mbuf); \
		INIT_WORK(&work_data->wcn_usb_work, wcn_usb_work_func); \
		init_waitqueue_head(&work_data->work_completion); \
		init_completion(&work_data->callback_complete);\
	} while (0)
#endif

void wcn_usb_work_data_init(struct wcn_usb_work_data *work_data, int id);

/* follow macro call wcn_usb_intf2endpoint
 * So we need hold intf, before we call them
 */
#define wcn_usb_ep_address(ep) \
		(wcn_usb_intf2endpoint(ep->intf, ep->numEp)->bEndpointAddress)
#define wcn_usb_ep_interval(ep) \
		(wcn_usb_intf2endpoint(ep->intf, ep->numEp)->bInterval)
#define wcn_usb_ep_packet_max(ep) \
		usb_endpoint_maxp(wcn_usb_intf2endpoint(ep->intf, ep->numEp))
#define wcn_usb_ep_is_isoc(ep) \
	usb_endpoint_xfer_isoc(wcn_usb_intf2endpoint(ep->intf, ep->numEp))
#define wcn_usb_ep_is_int(ep) \
	usb_endpoint_xfer_int(wcn_usb_intf2endpoint(ep->intf, ep->numEp))
#define wcn_usb_ep_is_ctrl(ep) \
	usb_endpoint_xfer_control(wcn_usb_intf2endpoint(ep->intf, ep->numEp))
#define wcn_usb_ep_is_bulk(ep) \
	usb_endpoint_xfer_bulk(wcn_usb_intf2endpoint(ep->intf, ep->numEp))
#define wcn_usb_ep_no_sg_constraint(ep) \
		(ep->intf->udev->bus->no_sg_constraint)
#define wcn_usb_ep_dir(ep) \
	usb_endpoint_dir_in(wcn_usb_intf2endpoint(ep->intf, ep->numEp))

#define wcn_usb_packet_get_pdata(packet) \
		(packet->pdata)

#define wcn_usb_ep_is_stop(ep) \
	usb_anchor_empty(&ep->submitted)

#define wcn_usb_packet_setup_packet(packet, setup_packet) \
		(packet->urb->setup_packet = setup_packet)

#define wcn_usb_intf_epNum(intf) \
	intf->interface->cur_altsetting->desc.bNumEndpoints

/* @head: list's head
 * @num: list's num
 * @pos: current node, if eq null then we done all list with success. if eq is
 *	not null, then it point the node that error one.
 * @posN: current node's index, if pos eq null then it eq num.
 */
#define mbuf_list_iter(head, num, pos, posN) \
	for (pos = head, posN = 0; posN < num && pos; posN++, pos = pos->next)

int  wcn_usb_push_list_tx(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num);
int wcn_usb_push_list_rx(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num);
void wcn_usb_begin_poll_rx(int chn);
void wcn_usb_mbuf_free_notif(int chn);
int wcn_usb_list_alloc(int chn, struct mbuf_t **head,
		struct mbuf_t **tail, int *num);
int wcn_usb_list_free(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num);
int wcn_usb_rx_tx_pool_init(void);
void wcn_usb_rx_tx_pool_deinit(void);

unsigned long long wcn_usb_get_rx_tx_cnt(void);

enum wcn_usb_event {
	interface_plug_base,
	interface_0_plug = interface_plug_base,
	interface_1_plug,
	interface_2_plug,
	dev_plug_fully,

	interface_unplug_base,
	interface_0_unplug = interface_unplug_base,
	interface_1_unplug,
	interface_2_unplug,
	dev_unplug_fully,

	download_over,
	cp_ready,
	pwr_on,
	pwr_off,
	pwr_state,
	error_happen,
	error_clean,
};


int wcn_usb_apostle_begin(int chn);
int wcn_usb_store_addr2chn(__u8 epAddress);

int wcn_usb_packet_is_freed(struct wcn_usb_packet *packet);
void *wcn_usb_packet_pop_buf(struct wcn_usb_packet *packet);
void wcn_usb_init_copy_men(void);

void channel_debug_mbuf_from_user(int chn, int num);
void channel_debug_mbuf_to_user(int chn, int num);
void channel_debug_mbuf_free(int chn, int num);
void channel_debug_mbuf_alloc(int chn, int num);
void channel_debug_rx_tx_from_controller(int chn, int num);
void channel_debug_rx_tx_to_controller(int chn, int num);
void channel_debug_rx_tx_free(int chn, int num);
void channel_debug_rx_tx_alloc(int chn, int num);
void channel_debug_to_accept(int chn, int num);
void channel_debug_report_num(int chn, int num);
void channel_debug_kzalloc(int times);
void channel_debug_net_free(int times);
void channel_debug_net_malloc(int times);
void channel_debug_free_big_men(int chn);
void channel_debug_alloc_big_men(int chn);
#if 0
void channel_debug_push_list_no_null(int times);
#endif
void channel_debug_packet_no_full(int times);
void channel_debug_free_with_accient(int times);
void channel_debug_mbuf_10(int times);
void channel_debug_mbuf_4(int times);
void channel_debug_mbuf_1(int times);
void channel_debug_mbuf_8(int times);
void channel_debug_interrupt_callback(int times);
void channel_debug_cp_num(int times);
int wcn_usb_chnmg_init(void);


int wcn_usb_state_sent_event(enum wcn_usb_event event);
int wcn_usb_state_register(struct notifier_block *nb);
int wcn_usb_state_unregister(struct notifier_block *nb);
int wcn_usb_state_get(enum wcn_usb_event event);

/* for test dump */
unsigned int marlin_get_wcn_chipid(void);
int marlin_dump_from_romcode_usb(unsigned int addr, void *buf, int len);
int marlin_get_version(void);
int marlin_connet(void);
#endif
