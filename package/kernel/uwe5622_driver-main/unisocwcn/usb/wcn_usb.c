#include "wcn_usb.h"
#include "bus_common.h"
#include <wcn_bus.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/proc_fs.h>
#include "wcn_glb.h"
#include "../sdio/sdiohal.h"

#ifdef CONFIG_USB_EHCI_HCD
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/file.h>
#endif

/* for mdbg */
/* mdbg channel is not enough*/
/* so we virtual three channel from one */
#define virtual_offset 29
static struct mchn_ops_t *virtual_pop_ventor[4];
struct virtual_buf {
	unsigned char ventor_id;
	char buf[0];
};
#define virtual_to_head(x) ((char *)x - sizeof(unsigned char))

MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

static int mdbg_virtual_pop(int channel, struct mbuf_t *head,
		     struct mbuf_t *tail, int num)
{
	struct mbuf_t *mbuf;
	int ret = 0;

	while (head) {
		unsigned char ventor_id;
		struct mchn_ops_t *mchn_ops;

		mbuf = head;
		head = head->next;

		mbuf->next = NULL;

		/*mbuf->buf = | int tag | void buf | */
		/*shengming make it begin from 1*/
		/*we need it begin from 0*/
		ventor_id = ((struct virtual_buf *)mbuf->buf)->ventor_id - 1;
		mbuf->buf = ((struct virtual_buf *)mbuf->buf)->buf;

		mchn_ops = virtual_pop_ventor[ventor_id];
		if (mchn_ops)
			mchn_ops->pop_link(virtual_offset + ventor_id,
					mbuf, mbuf, 1);
		if (ret)
			wcn_usb_err("%s pop_link error[%d]", __func__, ret);
	}
	return 0;
}

struct mchn_ops_t mdbg_virtual_ops = {
	.channel = 23,
	.inout = 0,
	.hif_type = HW_TYPE_USB,
	.pool_size = 5,
	.pop_link = mdbg_virtual_pop,
};

static int channel_need_map(int chn)
{
	if (chn == 29 || chn == 30 || chn == 31)
		return 1;
	return 0;
}

static struct mchn_ops_t *channel_map_ops(struct mchn_ops_t *ops)
{
	if (ops->channel == 23)
		wcn_usb_err("ERROR 23 is not for you!\n");
	if (channel_need_map(ops->channel)) {
		virtual_pop_ventor[ops->channel - virtual_offset] = ops;
		return &mdbg_virtual_ops;
	}
	return ops;
}

static int channel_map_chn(int chn)
{
	if (channel_need_map(chn))
		return 23;
	return chn;
}

static void channel_map_mbuf(int *chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct mbuf_t *mbuf;
	int i;

	if (channel_need_map(*chn)) {
		*chn = 23;
		/*mbuf->buf = | int tag | void buf | */
		mbuf_list_iter(head, num, mbuf, i) {
			mbuf->buf = virtual_to_head(mbuf->buf);
		}
	}
}

static int wcn_usb_notifier_cb(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct wcn_usb_notifier *wn;

	wn = container_of(nb, struct wcn_usb_notifier, nb);

	if (wn->event == action)
		wn->cb(wn->data);

	return NOTIFY_OK;
}

/*
 * first we expect a state,
 * like we expect dev is pluged, or which interface is pluged,
 * IF state is not that we expect,
 * then we wait corresponding event
 */
static struct wcn_usb_notifier *wcn_usb_notifier_register(void (*cb)(void *),
		void *data, unsigned long event)
{
	struct wcn_usb_notifier *wn;
	/* expect a state */
	if (wcn_usb_state_get(event))
		cb(data);

	/* wait corresponding event */
	wn = kzalloc(sizeof(struct wcn_usb_notifier), GFP_KERNEL);
	if (!wn)
		return NULL;

	wn->nb.notifier_call = wcn_usb_notifier_cb;
	wn->cb = cb;
	wn->data = data;
	wn->event = event;

	wcn_usb_state_register(&wn->nb);

	return wn;
}

static void wcn_usb_notifier_unregister(struct wcn_usb_notifier *wn)
{
	wcn_usb_state_unregister(&wn->nb);
	kfree(wn);
}

static int wcn_usb_preinit(void)
{
	int ret;

	wcn_usb_chnmg_init();
	wcn_usb_init_copy_men();
	ret = wcn_usb_rx_tx_pool_init();
	if (ret != 0) {
		wcn_usb_err("%s wcn usb rx_tx_pool init failed!\n", __func__);
		return ret;
	}

	ret = wcn_usb_store_init();
	if (ret != 0) {
		wcn_usb_err("%s wcn usb store init failed!\n", __func__);
		return ret;
	}

	ret = wcn_usb_io_init();
	if (ret != 0) {
		wcn_usb_err("%s wcn usb io init failed!\n", __func__);
		return ret;
	}

	wcn_usb_notifier_register((void (*)(void *))wcn_usb_apostle_begin,
			(void *)25, download_over);
	return ret;
}

static void wcn_usb_deinit(void)
{
	/*TODO deinit all channel! */
	wcn_usb_store_delet();
	wcn_usb_io_delet();
	wcn_usb_rx_tx_pool_deinit();
}

static int wcn_usb_chn_init(struct mchn_ops_t *ops)
{
	struct wcn_usb_work_data *work_data;
	int ret;

	if (ops->hif_type != HW_TYPE_USB)
		wcn_usb_err("%s hif_type != HW_TYPE_USB[%d]\n", __func__,
				ops->hif_type);

	ops = channel_map_ops(ops);
	if (ops == NULL)
		return 0;

	work_data = wcn_usb_store_get_channel_info(ops->channel);
	if (!work_data)
		return -ENODEV;

	/*FIXME*/
	if (ops->channel == 23 && chn_ops(23))
		return 0;

	mutex_lock(&work_data->channel_lock);
	ret = bus_chn_init(ops, HW_TYPE_USB);
	mutex_unlock(&work_data->channel_lock);
	wcn_usb_mbuf_free_notif(ops->channel);
	if (ret)
		return ret;

	if (ops->inout == 0) {
		work_data->goon = 1;
		work_data->wn = wcn_usb_notifier_register(
				(void (*)(void *))wcn_usb_begin_poll_rx,
				(void *)(long)(ops->channel), download_over);
	}
	return 0;
}

static int wcn_usb_chn_deinit(struct mchn_ops_t *ops)
{
	struct wcn_usb_work_data *work_data;
	int ret;

	ops = channel_map_ops(ops);
	if (ops == NULL)
		return 0;

	work_data = wcn_usb_store_get_channel_info(ops->channel);
	if (!work_data)
		return -ENODEV;

	wcn_usb_wait_channel_stop(ops->channel);

	mutex_lock(&work_data->channel_lock);
	ret = bus_chn_deinit(ops);
	mutex_unlock(&work_data->channel_lock);
	if (ret)
		return ret;
	if (ops->inout == 0)
		wcn_usb_notifier_unregister(work_data->wn);

	return 0;
}

int wcn_usb_list_alloc(int chn, struct mbuf_t **head,
		struct mbuf_t **tail, int *num)
{
	int ret;
	struct mbuf_t *mbuf;
	int i;
	struct mchn_ops_t *ops;

	ops = chn_ops(chn);
	if (!ops) {
		wcn_usb_err("%s chn_ops get error\n", __func__);
		return -ENODEV;
	}
	chn = channel_map_chn(chn);

	ret = buf_list_alloc(chn, head, tail, num);
	if (ret) {
		/* FIXME buf_list_alloc return errorno is error!!! */
		ret = -ENOMEM;
		return ret;
	}

	channel_debug_mbuf_alloc(chn, *num);
	/* buf_list_alloc not sure this list is clean!!! so we do this */
	mbuf_list_iter(*head, *num, mbuf, i) {
		mbuf->buf = NULL;
		mbuf->len = 0;
	}

	return ret;
}

int wcn_usb_list_free(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	int ret;
	struct mchn_ops_t *ops;

	ops = chn_ops(chn);
	if (!ops) {
		wcn_usb_err("%s chn_ops get error\n", __func__);
		return -ENODEV;
	}
	chn = channel_map_chn(chn);

	ret = buf_list_free(chn, head, tail, num);
	if (ret)
		return ret;

	channel_debug_mbuf_free(chn, num);
	wcn_usb_mbuf_free_notif(chn);
	return ret;
}

static int wcn_usb_push_list(int chn, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct mchn_ops_t *ops;
	struct wcn_usb_work_data *work_data;
	int inout;

	channel_map_mbuf(&chn, head, tail, num);

	work_data = wcn_usb_store_get_channel_info(chn);
	if (!work_data)
		return -ENODEV;

	ops = chn_ops(chn);
	if (!ops) {
		wcn_usb_err("%s chn_ops get error\n", __func__);
		return -ENODEV;
	}
	inout = ops->inout;

	channel_debug_mbuf_from_user(chn, num);
	/* if this is a rx channel */
	if (inout == 0) { /* rx */
		/* free each buf */
		/* free mbuf list */
		return wcn_usb_push_list_rx(chn, head, tail, num);
	}
	/* if this is a tx channel */
	/* submit this mbuf list */
	return wcn_usb_push_list_tx(chn, head, tail, num);
}

static int wcn_usb_get_hif_type(void)
{
	return HW_TYPE_USB;
}

static unsigned long long wcn_usb_get_rx_total_cnt(void)
{
	return wcn_usb_get_rx_tx_cnt();
}

static void wcn_usb_insert_call_fn(void *data)
{
	((void (*)(void))data)();
}

static void wcn_usb_register_rescan_cb(void *data)
{
	wcn_usb_notifier_register((void (*)(void *))wcn_usb_insert_call_fn,
			(void *)data, dev_plug_fully);
}

#ifdef CONFIG_USB_EHCI_HCD
#define READ_SIZE PAGE_SIZE
char ehci_dbg_buf[PAGE_SIZE];
static int wcn_mount_debugfs(void)
{
	return 0;
}

static int wcn_print_async(void)
{
	struct file *filp;
	int ret;
	static char ehci_async_path[128] =
			"/sys/kernel/debug/usb/ehci/f9890000.ehci/async";

	filp = filp_open(ehci_async_path, O_RDONLY, 0644);
	if (IS_ERR(filp))
		wcn_usb_err("%s open %s error no:%ld\n",
			    __func__, ehci_async_path, PTR_ERR(filp));

	memset(ehci_dbg_buf, 0x0, READ_SIZE);
	ret = kernel_read(filp, 0, ehci_dbg_buf, READ_SIZE);
	wcn_usb_info("%s async read:%d\n%s\n\n", __func__, ret, ehci_dbg_buf);

	fput(filp);

	return 0;
}

static int wcn_print_periodic(void)
{
	struct file *filp;
	int ret;
	static char ehci_periodic_path[128] =
			"/sys/kernel/debug/usb/ehci/f9890000.ehci/periodic";

	filp = filp_open(ehci_periodic_path, O_RDONLY, 0644);
	if (IS_ERR(filp))
		wcn_usb_err("%s open %s error no:%ld\n",
			    __func__, ehci_periodic_path, PTR_ERR(filp));

	memset(ehci_dbg_buf, 0x0, READ_SIZE);
	ret = kernel_read(filp, 0, ehci_dbg_buf, READ_SIZE);
	wcn_usb_info("%s periodic read:%d\n%s\n\n", __func__, ret,
		     ehci_dbg_buf);

	fput(filp);

	return 0;
}

static void print_ehci_info(void)
{
	wcn_usb_info("%s start print ehci info\n", __func__);
	wcn_mount_debugfs();
	wcn_print_async();
	wcn_print_periodic();
	wcn_usb_info("%s end print ehci info\n", __func__);
}
#endif

static void wcn_usb_set_carddump_status(unsigned int status)
{
	if (status) {
		#ifdef CONFIG_USB_EHCI_HCD
		print_ehci_info();
		#endif
		wcn_usb_state_sent_event(error_happen);
	} else
		wcn_usb_state_sent_event(error_clean);
}

static unsigned int wcn_usb_get_carddump_status(void)
{
	return wcn_usb_state_get(error_happen);
}

static int wcn_usb_runtime_get(void)
{
	/* wcn_usb_state_sent_event(pwr_on); */

	return 0;
}

static int wcn_usb_runtime_put(void)
{
	wcn_usb_state_sent_event(pwr_off);

	return 0;
}

static int wcn_usb_rescan(void)
{
	if (!wcn_usb_state_get(error_happen)) {
		if (wcn_usb_state_get(dev_plug_fully))
			wcn_usb_state_sent_event(dev_plug_fully);

	}

	return 0;
}

static void wcn_usb_cp_ready_event(void *data)
{
	struct completion *sync_complete;

	if (data) {
		sync_complete = (struct completion *)data;
		complete(sync_complete);
	}
}

static int wcn_usb_check_cp_ready(unsigned int addr, int timout)
{
	unsigned long timeleft;
	static struct completion *sync_complete;
	static struct wcn_usb_notifier *usb_notifier;
	int ret = 0;

	sync_complete = kzalloc(sizeof(struct completion), GFP_KERNEL);
	if (!sync_complete) {
		ret = -ENOMEM;
		goto OUT;
	}

	init_completion(sync_complete);
	usb_notifier = wcn_usb_notifier_register(
			(void (*)(void *))wcn_usb_cp_ready_event,
			(void *)sync_complete, cp_ready);
	wcn_usb_state_sent_event(pwr_on);
	wcn_usb_state_sent_event(download_over);

	timeleft = wait_for_completion_timeout(sync_complete, 2*HZ);
	if (!timeleft)
		ret = -ETIMEDOUT;

	wcn_usb_notifier_unregister(usb_notifier);
	kfree(sync_complete);
OUT:
	if (ret)
		wcn_usb_err("%s wait cp sync err %d\n", __func__, ret);

	return ret;
}

struct sprdwcn_bus_ops wcn_usb_bus_ops = {
	.preinit = wcn_usb_preinit,
	.deinit = wcn_usb_deinit,
	.chn_init = wcn_usb_chn_init,
	.chn_deinit = wcn_usb_chn_deinit,
	.list_alloc = wcn_usb_list_alloc,
	.list_free = wcn_usb_list_free,
	.push_list = wcn_usb_push_list,
	.get_hif_type = wcn_usb_get_hif_type,
	/* other ops not implemented temporarily */
	.register_rescan_cb = wcn_usb_register_rescan_cb,
	.get_carddump_status = wcn_usb_get_carddump_status,
	.set_carddump_status = wcn_usb_set_carddump_status,
	.get_rx_total_cnt = wcn_usb_get_rx_total_cnt,
	.rescan = wcn_usb_rescan,
	.runtime_get = wcn_usb_runtime_get,
	.runtime_put = wcn_usb_runtime_put,
	.check_cp_ready = wcn_usb_check_cp_ready,
};

void module_bus_init(void)
{
	wcn_usb_info("%s register wcn_usb_bus_ops\n", __func__);

	if (module_ops_register(&wcn_usb_bus_ops))
		wcn_usb_err("%s wcn_usb_bus_ops register error!\n", __func__);
}
EXPORT_SYMBOL(module_bus_init);

void module_bus_deinit(void)
{
	module_ops_unregister();
}
