#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <wcn_bus.h>

#define mbuf_list_iter(head, num, pos, posN) \
	for (pos = head, posN = 0; posN < num && pos; posN++, pos = pos->next)

#define wcn_usb_dp(fmt, args...) \
		pr_info("wcn_usb_download " fmt, ## args)
struct wcn_usb_ddata {
	struct mchn_ops_t	tx_ops;
	struct mchn_ops_t	rx_ops;
	struct mutex		file_data_lock;
	/* config */
	int			in_id;
	int			out_id;
	/* mbuf pool */
	spinlock_t		lock;
	struct mbuf_t		*mbuf_head;
	/* file info */
	struct proc_dir_entry	*dir;
	struct proc_dir_entry	*download;
	/* sync wait queue */
	wait_queue_head_t	wait_rx_data;
	bool			rx_cb_called;
	wait_queue_head_t	wait_tx_data;
	/* only for rx */
	struct mbuf_t		*now;
	int			now_offset;
};

struct wcn_usb_ddata *ddthis;

/* we want to do a system for proc file */
static int rx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	struct mbuf_t *temp_tail;
	struct wcn_usb_ddata *data;

	data = ddthis;

	spin_lock(&data->lock);
	if (data->mbuf_head) {
		temp_tail = data->mbuf_head;
		while (temp_tail->next)
			temp_tail = temp_tail->next;
		temp_tail->next = head;
	} else {
		data->mbuf_head = head;
	}
	spin_unlock(&data->lock);

	data->rx_cb_called = 1;
	wake_up(&data->wait_rx_data);
	return 0;
}

static int tx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	struct wcn_usb_ddata *data;
	struct mbuf_t *mbuf;
	int i;
	int ret;

	data = ddthis;

	mbuf_list_iter(head, num, mbuf, i)
		kfree(mbuf->buf);

	ret = sprdwcn_bus_list_free(chn, head, tail, num);
	if (ret)
		wcn_usb_dp("%s list free error\n", __func__);

	wake_up(&data->wait_tx_data);
	return 0;
}

static int wcn_usb_dopen(struct inode *inode, struct file *file)
{
	struct wcn_usb_ddata *data;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19, 2))
	data = (struct wcn_usb_ddata *)pde_data(inode);
#else
	data = (struct wcn_usb_ddata *)PDE_DATA(inode);
#endif

	if (!data)
		return -EIO;

	mutex_lock(&data->file_data_lock);
	/* ops need updata */
	if (data->tx_ops.channel != data->out_id ||
			data->rx_ops.channel != data->in_id) {
		file->private_data = data;
		/* TODO channel init */
		data->tx_ops.channel = data->out_id;
		data->rx_ops.channel = data->in_id;
		data->tx_ops.hif_type = data->rx_ops.hif_type = HW_TYPE_USB;
		data->tx_ops.inout = 1;
		data->rx_ops.inout = 0;
		data->tx_ops.pool_size = data->rx_ops.pool_size = 5;
		data->rx_ops.pop_link = rx_cb;
		data->tx_ops.pop_link = tx_cb;


		if (sprdwcn_bus_chn_init(&data->rx_ops) ||
				sprdwcn_bus_chn_init(&data->tx_ops)) {
			wcn_usb_dp("%s chn_init rx_ops failed\n", __func__);
			wcn_usb_dp("%s chn_init rx_ops failed\n", __func__);
			data->tx_ops.channel = 0;
			data->rx_ops.channel = 0;
		}
	}
	mutex_unlock(&data->file_data_lock);

	return 0;
}

#define file_is_noblock(file) \
	((file->f_flags & O_NONBLOCK) == O_NONBLOCK)

static ssize_t wcn_usb_dwrite(struct file *file, const char __user *buf,
		size_t buflen, loff_t *offset)
{
	struct wcn_usb_ddata *data = file->private_data;
	int mbuf_num;
	struct mbuf_t *head, *tail;
	struct mbuf_t *mbuf;
	ssize_t ret_size = 0;
	int cp_len;
	int ret;
	int i, j;

	if (!data || !buflen || !buf)
		return 0;

	mutex_lock(&data->file_data_lock);
RETRY:
	mbuf_num = 1;
	ret = sprdwcn_bus_list_alloc(data->out_id, &head, &tail, &mbuf_num);
	if (ret) {
		wcn_usb_dp("%s mbuf_list_alloc error[%d]\n", __func__, ret);
		goto WRITE_EXIT;
	}

	mbuf_list_iter(head, mbuf_num, mbuf, i) {
		cp_len = buflen - ret_size;
		mbuf->len = cp_len;
		mbuf->buf = kmalloc(cp_len, GFP_KERNEL);
		if (!mbuf->buf) {
			wcn_usb_dp("%s kmalloc error\n", __func__);
			goto CLEAN_MBUF_EXIT;
		}
		if (copy_from_user(mbuf->buf, buf + ret_size, cp_len)) {
			wcn_usb_dp("%s copy_from_user error\n", __func__);
			goto CLEAN_MBUF_EXIT;
		}
		ret_size += cp_len;
		*offset += cp_len;
	}

	wcn_usb_dp("%s send mbuf[%p] len[%d] tail[%p]->next[%p]\n", __func__,
			head, head->len, tail, tail->next);
	ret = sprdwcn_bus_push_list(data->out_id, head, tail, mbuf_num);
	if (ret) {
		wcn_usb_dp("%s push_list error\n", __func__);
		goto CLEAN_MBUF_EXIT;
	}

	if (buflen > ret_size || !file_is_noblock(file)) {
		data->rx_cb_called = 0;
		ret = wait_event_interruptible(data->wait_tx_data,
				data->rx_cb_called == 1);
		if (ret < 0)
			goto WRITE_EXIT;

		goto RETRY;
	}

WRITE_EXIT:
	mutex_unlock(&data->file_data_lock);
	return ret_size;
CLEAN_MBUF_EXIT:
	mutex_unlock(&data->file_data_lock);
	mbuf_list_iter(head, i + 1, mbuf, j)
		kfree(mbuf->buf);
	sprdwcn_bus_list_free(data->out_id, head, tail, mbuf_num);
	*offset = 0;
	return -EIO;

}

static struct mbuf_t *wcn_usb_dget_new_mbuf(struct wcn_usb_ddata *data,
		int is_noblock)
{
	struct mbuf_t *mbuf = NULL;
	int ret;

GET_MBUF:
	spin_lock(&data->lock);
	if (data->mbuf_head) {
		mbuf = data->mbuf_head;
		data->mbuf_head = data->mbuf_head->next;
		mbuf->next = NULL;
	}
	spin_unlock(&data->lock);

	/* Are we get data? */
	if (mbuf || is_noblock)
		return mbuf;
	/* we not get mbuf, and user asked block */
	ret = wait_event_interruptible(data->wait_rx_data, data->mbuf_head);
	if (ret < 0)
		return NULL;
	goto GET_MBUF;
}

static ssize_t wcn_usb_dread(struct file *file, char __user *buf,
		size_t buflen, loff_t *offset)
{
	struct wcn_usb_ddata *data = file->private_data;
	ssize_t ret_size = 0;
	int cp_len;
	int ret;

	if (!data || !buflen || !buf)
		return 0;

	mutex_lock(&data->file_data_lock);
REFILL_BUF:
	/* we have info */
	if (data->now && data->now->len > data->now_offset) {
		cp_len = min_t(long, buflen - ret_size,
				data->now->len - data->now_offset);

		ret = copy_to_user(buf + ret_size,
					data->now->buf + data->now_offset,
					cp_len);
		if (ret) {
			ret_size = -EFAULT;
			goto READ_EXIT;
		}

		data->now_offset += cp_len;
		ret_size += cp_len;
		*offset += cp_len;
	}

	/* our info is not enough */
	if (ret_size < buflen) {
		data->now_offset = 0;
		/* release our info */
		if (data->now) {
			ret = sprdwcn_bus_push_list(data->in_id, data->now,
					data->now, 1);
			if (ret) {
				wcn_usb_dp("%s push list error[%d]\n",
						__func__, ret);
			}
		}

		/* get new */
		data->now = wcn_usb_dget_new_mbuf(data, file_is_noblock(file));
		if (!data->now)
			goto READ_EXIT;

		goto REFILL_BUF;
	}

READ_EXIT:
	mutex_unlock(&data->file_data_lock);
	return ret_size;
}

static int wcn_usb_drelease(struct inode *inode, struct file *file)
{
	struct wcn_usb_ddata *data = file->private_data;
	struct mbuf_t *head, *tail;
	int ret;
	int mbuf_num = 0;

	if (!data)
		return 0;
	mutex_lock(&data->file_data_lock);
	/* Is this any data in now? */
	if (data->now) {
		data->now->next = data->mbuf_head;
		data->mbuf_head = data->now;
		data->now = NULL;
	}

	/* Is this any data in head? */
	if (data->mbuf_head) {
		mbuf_num = 1;
		head = tail = data->mbuf_head;
		while (tail->next) {
			tail = tail->next;
			mbuf_num += 1;
		}

		/* If there any mbuf not be read, maybe We need warming user */
		ret = sprdwcn_bus_push_list(data->in_id, head, tail, mbuf_num);
		if (ret)
			wcn_usb_dp("%s push list error[%d]\n", __func__, ret);
	}

	/* channel deinit */
	mutex_unlock(&data->file_data_lock);
	return 0;
}

static const struct file_operations wcn_usb_dops = {
	.owner = THIS_MODULE,
	.read = wcn_usb_dread,
	.write = wcn_usb_dwrite,
	.open = wcn_usb_dopen,
	.release = wcn_usb_drelease,
	.llseek = noop_llseek,
};

int wcn_usb_dinit(void)
{
	struct wcn_usb_ddata *this;

	this = kzalloc(sizeof(struct wcn_usb_ddata), GFP_KERNEL);
	if (!this) {
		wcn_usb_dp("%s[%d] no memory\n", __func__, __LINE__);
		return -ENOMEM;
	}

	spin_lock_init(&this->lock);
	mutex_init(&this->file_data_lock);
	this->out_id = 7;
	this->in_id = 23;
	init_waitqueue_head(&this->wait_tx_data);
	init_waitqueue_head(&this->wait_rx_data);

	this->download = proc_create_data("wcn_usb/download", 0544, this->dir,
			&wcn_usb_dops, this);
	if (!this->download)
		wcn_usb_dp("%s create file[download] failed\n", __func__);

	ddthis = this;
	return 0;
}

