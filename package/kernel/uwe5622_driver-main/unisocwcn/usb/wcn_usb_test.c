#include "wcn_usb.h"
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <wcn_bus.h>

#define mbuf_list_iter(head, num, pos, posN) \
	for (pos = head, posN = 0; posN < num && pos; posN++, pos = pos->next)

#define WCN_USB_CHANNEL_MAX 32
#define TRANSF_LIST_MAX 200
#define POOL_SIZE TRANSF_LIST_MAX
#define pool_buf_size 1672
#define CHNMG_SHOW_BUF_MAX (WCN_USB_CHANNEL_MAX * 56)
#define BUF_LEN 128

#define wcn_usb_test_print(fmt, args...) \
	pr_info("wcn_usb_test " fmt, ## args)

/* ugly code !! */
static struct chnmg *this_chnmg;
static struct channel *chnmg_find_channel(struct chnmg *chnmg, int id);

struct channel {
	int id;
	int inout;
	int status;

	struct mbuf_t *rx_pool_head;
	struct mbuf_t *rx_pool_tail;
	int rx_pool_num;
	struct mutex  pool_lock;
	wait_queue_head_t wait_rx_data;
	struct mbuf_t *now;
	int now_offset;

	struct mchn_ops_t mchn_ops;
	struct proc_dir_entry *file; /* self */
	struct proc_dir_entry *dir;
	char *name;

	int lp_rx_head;
	int lp_tx_head;
};

static int get_channel_dir(int channel_id)
{
	if (channel_id <= 15)
		return 1;
	else
		return 0;
}

static int wcn_usb_channel_open(struct inode *inode, struct file *file)
{
	struct channel *channel;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19, 2))
	channel = (struct channel *)pde_data(inode);
else
	channel = (struct channel *)PDE_DATA(inode);
#endif

	if (!channel)
		return -EIO;

	file->private_data = channel;

	return 0;
}

static int wcn_usb_channel_release(struct inode *inode, struct file *file)
{
	return 0;
}

#define file_is_noblock(file) \
	((file->f_flags & O_NONBLOCK) == O_NONBLOCK)

static ssize_t wcn_usb_channel_read(struct file *file, char *buffer,
		size_t count, loff_t *ppos)
{
	struct channel *channel;
	ssize_t ret_size = 0;
	int cp_len;
	int ret;

	channel = file->private_data;

	if (!channel || !count || !buffer)
		return 0;

REFILL_BUF:
	if (channel->now && channel->now->len > channel->now_offset) {
		cp_len = min_t(long, count - ret_size,
				channel->now->len - channel->now_offset);
		ret = copy_to_user(buffer + ret_size,
					channel->now->buf + channel->now_offset,
					cp_len);
		if (ret) {
			ret_size = -EFAULT;
			goto READ_EXIT;
		}

		channel->now_offset += cp_len;
		ret_size += cp_len;
		*ppos += cp_len;
	}

	if (ret_size < count) {
		channel->now_offset = 0;
		if (channel->now) {
			ret = sprdwcn_bus_push_list(channel->id, channel->now,
					channel->now, 1);
			if (ret) {
				wcn_usb_test_print("%s push list error[%d]\n",
						__func__, ret);
			}
			channel->now = NULL;
		}

		/* get a new mbuf */
GET_NEW_MBUF:
		mutex_lock(&channel->pool_lock);
		if (channel->rx_pool_num != 0) {
			channel->rx_pool_num -= 1;
			channel->now = channel->rx_pool_head;
			channel->rx_pool_head = channel->rx_pool_head->next;
		}
		mutex_unlock(&channel->pool_lock);

		if  (!channel->now && !file_is_noblock(file) && !ret_size) {
			ret = wait_event_interruptible(channel->wait_rx_data,
					channel->rx_pool_num != 0);
			goto GET_NEW_MBUF;
		}

		if (channel->now)
			goto REFILL_BUF;
	}

READ_EXIT:
	return ret_size;
}

static ssize_t wcn_usb_channel_write(struct file *file, const char *buffer,
		size_t count, loff_t *ppos)
{
	int mbuf_num;
	struct mbuf_t *head;
	struct mbuf_t *tail;
	struct mbuf_t *mbuf;
	struct channel *channel;
	ssize_t buf_offset;
	int i;
	unsigned short buf_len;
	int ret;

	channel = (struct channel *)file->private_data;
	if (!get_channel_dir(channel->id)) {
		wcn_usb_test_print("%s not rx!\n", __func__);
		return 0;
	}

	mbuf_num = count / pool_buf_size + 1;
	mbuf_num = mbuf_num < TRANSF_LIST_MAX ? mbuf_num : TRANSF_LIST_MAX;

	ret = sprdwcn_bus_list_alloc(channel->id, &head, &tail, &mbuf_num);
	if (ret) {
		wcn_usb_test_print("%s list is full\n", __func__);
		return 0;
	}

	buf_offset = 0;
	mbuf = head;
	for (i = 0; i < mbuf_num; i++) {
		buf_len = count - buf_offset  < pool_buf_size ?
			count - buf_offset : pool_buf_size;
		mbuf->buf = kzalloc(buf_len, GFP_KERNEL);
		if (!mbuf->buf) {
			wcn_usb_test_print("%s no mem\n", __func__);
			return -ENOMEM;
		}

		if (copy_from_user(mbuf->buf, buffer + buf_offset, buf_len)) {
			wcn_usb_test_print("%s copy from user error\n",
					__func__);
			return -EFAULT;
		}

		mbuf->len = buf_len;
		mbuf = mbuf->next;
		buf_offset += buf_len;
	}

	if (!i)
		return 0;

	if (i < mbuf_num) {
		wcn_usb_test_print("%s creat list error i[%d] mbuf_num[%d]\n",
				__func__, i, mbuf_num);
		kfree(mbuf->buf);
		sprdwcn_bus_list_free(channel->id, mbuf, tail, mbuf_num - i);
		tail = head;
		while (tail->next != mbuf)
			tail = tail->next;

		tail->next = NULL;
	}
	wcn_usb_test_print("%s begin to push list\n", __func__);

	if (sprdwcn_bus_push_list(channel->id, head, tail, i)) {
		mbuf = head;
		while (!mbuf) {
			kfree(mbuf->buf);
			mbuf = mbuf->next;
		}
		sprdwcn_bus_list_free(channel->id, head, tail, i);
		return -EIO;
	}

	wcn_usb_test_print("%s i[%d] mbuf_num[%d] byte[%ld]\n",
			__func__, i, mbuf_num, buf_offset);

	*ppos += buf_offset;
	return buf_offset;
}

static const struct file_operations wcn_usb_channel_fops = {
	.owner = THIS_MODULE,
	.read = wcn_usb_channel_read,
	.write = wcn_usb_channel_write,
	.open = wcn_usb_channel_open,
	.release = wcn_usb_channel_release,
	.llseek = noop_llseek,
};

int calculate_throughput(int channel_id, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	static struct timespec tm_begin;
	struct timespec tm_end;
	static int time_count;
	unsigned long time_total_ns;
	struct mbuf_t *mbuf;
	int i;

	if (time_count == 0)
		getnstimeofday(&tm_begin);

	if (!num)
		return 0;


	if (get_channel_dir(channel_id) &&
	    (chnmg_find_channel(this_chnmg, channel_id)->status)) {
		mbuf_list_iter(head, num, mbuf, i) {
			kfree(mbuf->buf);
			mbuf->buf = NULL;
			mbuf->len = 0;
		}
		sprdwcn_bus_list_free(channel_id, head, tail, num);
		return 0;
	}

	if (sprdwcn_bus_push_list(channel_id, head, tail, num))
		wcn_usb_test_print("%s push list error\n", __func__);

	time_count += num;
	if (time_count >= 1000) {
		getnstimeofday(&tm_end);
		time_total_ns = timespec_to_ns(&tm_end)
			- timespec_to_ns(&tm_begin);
		wcn_usb_test_print("%s avg time[%ld] in [%d]\n",
				__func__, time_total_ns, time_count);
		time_count = 0;
	}

	return 0;

}

static int tx_pop_link(int channel_id, struct mbuf_t *head, struct mbuf_t *tail,
		int num)
{
	int i;
	struct mbuf_t *mbuf;

	wcn_usb_test_print("%s is be called\n", __func__);
	mbuf_list_iter(head, num, mbuf, i) {
		kfree(mbuf->buf);
	}
	sprdwcn_bus_list_free(channel_id, head, tail, num);

	return 0;
}

static int rx_pop_link(int channel_id, struct mbuf_t *head, struct mbuf_t *tail,
		int num)
{
	struct channel *channel = chnmg_find_channel(this_chnmg, channel_id);

	if (!channel) {
		WARN_ON(1);
		return 0;
	}

	mutex_lock(&channel->pool_lock);
	if (channel->rx_pool_head) {
		channel->rx_pool_tail->next = head;
		channel->rx_pool_tail = tail;
		channel->rx_pool_num += num;
	} else {
		channel->rx_pool_head = head;
		channel->rx_pool_tail = tail;
		channel->rx_pool_num = num;
	}
	mutex_unlock(&channel->pool_lock);

	wake_up(&channel->wait_rx_data);
	return 0;
}

typedef int (*channel_callback)(int, struct mbuf_t *, struct mbuf_t*, int);
static struct channel *channel_init(int id, struct proc_dir_entry *dir,
				    channel_callback pop_link)
{
	struct channel *channel;

	channel = kzalloc(sizeof(struct channel), GFP_KERNEL);
	if (!channel)
		return NULL;

	/* 16 is magic that string max length */
	channel->name = kzalloc(32, GFP_KERNEL);
	if (!channel->name)
		goto CHANNEL_FREE;

	channel->id = id;
	channel->inout = get_channel_dir(id);
	init_waitqueue_head(&channel->wait_rx_data);
	mutex_init(&channel->pool_lock);
	sprintf(channel->name, "wcn_usb/channel_%d", id);
	channel->mchn_ops.channel = channel->id;
	channel->mchn_ops.hif_type = HW_TYPE_USB;
	channel->mchn_ops.inout = channel->inout;
	channel->mchn_ops.pool_size = POOL_SIZE;
	channel->mchn_ops.pop_link = pop_link;
	channel->file = proc_create_data(channel->name,
			0544, dir, &wcn_usb_channel_fops,
			channel);
	if (!channel->file)
		goto CHANNEL_NAME_FREE;


	return channel;

CHANNEL_NAME_FREE:
	kfree(channel->name);
CHANNEL_FREE:
	kfree(channel);
	return NULL;

}

struct chnmg {
	struct proc_dir_entry *file; /* self */
	struct proc_dir_entry *defile;
	struct proc_dir_entry *dir;
	struct proc_dir_entry *print_level;
	struct proc_dir_entry *channel_debug;
	int num_channels;
	struct channel *channel[0];
};

static void channel_destroy(struct channel *channel)
{
	proc_remove(channel->file);
	kfree(channel->name);
	kfree(channel);
}

static struct channel *channel_register(struct chnmg *chnmg,
					int channel_id,
					channel_callback pop_link)
{
	struct channel *channel;
	int i;

	channel = chnmg_find_channel(chnmg, channel_id);
	if (!channel) {
		channel = channel_init(channel_id, chnmg->dir, pop_link);
		for (i = 0; i < WCN_USB_CHANNEL_MAX; i++) {
			if (!chnmg->channel[i]) {
				chnmg->channel[i] = channel;
				break;
			}
		}
		if (sprdwcn_bus_chn_init(&channel->mchn_ops)) {
			channel_destroy(channel);
			chnmg->channel[i] = NULL;
			channel = NULL;
		}
	}

	return channel;
}

static void channel_unregister(struct channel *channel)
{
	sprdwcn_bus_chn_deinit(&channel->mchn_ops);
}

static ssize_t channel_show(struct channel *channel, char *kbuf,
		size_t buf_size)
{
	int ret;

	if (!channel)
		return 0;

	ret = snprintf(kbuf, buf_size, "[%d]\t[%s]\t[%d]\n", channel->id,
			channel->inout ? "tx" : "rx", channel->status);
	if (ret < 0) {
		wcn_usb_test_print("%s channel print error id[%d] errno[%d]\n",
				__func__, channel->id, ret);
		return 0;
	}

	/* cut the \0 */
	return strlen(kbuf) - 1;
}

static struct channel *chnmg_find_channel(struct chnmg *chnmg, int id)
{
	int i;

	for (i = 0; i < WCN_USB_CHANNEL_MAX; i++) {
		if (chnmg->channel[i]) {
			if ((chnmg->channel[i])->id == id)
				return chnmg->channel[i];
		}
	}
	return NULL;
}

static struct channel *chnmg_find_channel_destroy(struct chnmg *chnmg, int id)
{
	int i;
	struct channel *channel = NULL;

	for (i = 0; i < WCN_USB_CHANNEL_MAX; i++) {
		if (chnmg->channel[i]) {
			if ((chnmg->channel[i])->id == id) {
				channel = chnmg->channel[i];
				chnmg->channel[i] = NULL;
			}
		}
	}
	return channel;
}

/* chnmg channel manager */
static int wcn_usb_chnmg_open(struct inode *inode, struct file *file)
{
	struct chnmg *chnmg;
	/* get channel_list head */
	chnmg = (struct chnmg *)pde_data(inode);

	file->private_data = chnmg;
	return 0;
}

static int wcn_usb_chnmg_release(struct inode *indoe, struct file *file)
{
	return 0;
}

static ssize_t wcn_usb_chnmg_show(struct file *file, char *buffer,
		size_t count, loff_t *ppos)
{
	size_t ret;
	size_t buf_len;
	int i;
	struct chnmg *chnmg;
	char *kbuf;
	size_t remain_length;

	kbuf = kzalloc(CHNMG_SHOW_BUF_MAX, GFP_KERNEL);
	if (!kbuf)
		return 0;

	chnmg = file->private_data;

	for (i = 0, buf_len = 0; i < WCN_USB_CHANNEL_MAX &&
			buf_len < CHNMG_SHOW_BUF_MAX; i++) {
		if (chnmg->channel[i]) {
			ret = channel_show(chnmg->channel[i], kbuf+buf_len,
					CHNMG_SHOW_BUF_MAX - buf_len);
			buf_len += ret;
		}
	}

	if (*ppos > buf_len) {
		kfree(kbuf);
		return 0;
	}

	remain_length = buf_len - *ppos;
	count = remain_length < count ? remain_length : count;
	if (copy_to_user(buffer, kbuf + *ppos, count)) {
		wcn_usb_test_print("%s copy error\n", __func__);
		kfree(kbuf);
		return 0;
	}

	kfree(kbuf);
	*ppos += count;
	return count;
}

static int atoi(const char *str)
{
	int value = 0;

	while (*str >= '0' && *str <= '9') {
		value *= 10;
		value += *str - '0';
		str++;
	}
	return value;
}

static int string_is_num(char *string)
{
	if (string[0] >= '0' && string[0] <= '9')
		return 1;
	return 0;
}

static int wcn_usb_chnmg_get_intFRuser(const char *user_buffer, size_t count)
{
	char *kbuf;
	int channel_id;

	if (count > 10) {
		wcn_usb_test_print("%s error count\n", __func__);
		return -EINVAL;
	}

	kbuf = kzalloc(count, GFP_KERNEL);
	if (!kbuf) {
		wcn_usb_test_print("%s no memory\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(kbuf, user_buffer, count)) {
		kfree(kbuf);
		wcn_usb_test_print("%s copy error\n", __func__);
		return -EIO;
	}

	if (!string_is_num(kbuf)) {
		kfree(kbuf);
		wcn_usb_test_print("%s we only want number!\n", __func__);
		return -EINVAL;
	}

	channel_id = atoi(kbuf);
	kfree(kbuf);
	return channel_id;
}

struct usb_test_cmd_desc {
	int type;
	int channel;
	int command;
	int mbuf_num;
	int mbuf_len;
};
#define TEST_COMMAND_CHAN 8

static int wcn_usb_test_tp(struct chnmg *chnmg, struct usb_test_cmd_desc *cmd)
{
	struct channel *channel;
	int num = cmd->mbuf_num;
	int i;
	struct mbuf_t *head;
	struct mbuf_t *tail;
	struct mbuf_t *mbuf;
	int ret = 0;

	channel = channel_register(chnmg, cmd->channel, calculate_throughput);
	if (!channel) {
		wcn_usb_test_print("%s channel init error\n", __func__);
		return -EIO;
	}
	channel->status = cmd->command;
	if (channel->status)
		return 0;

	if (get_channel_dir(cmd->channel) &&
	    !(chnmg_find_channel(this_chnmg, cmd->channel)->status)) {
		ret = sprdwcn_bus_list_alloc(cmd->channel, &head, &tail, &num);
		if (ret || !head || num != cmd->mbuf_num) {
			sprdwcn_bus_list_free(cmd->channel, head, tail, num);
			return -ENOMEM;
		}
		mbuf_list_iter(head, cmd->mbuf_num, mbuf, i) {
			mbuf->buf = kzalloc(cmd->mbuf_len, GFP_KERNEL);
			if (!mbuf->buf) {
				int j;

				mbuf_list_iter(head, i, mbuf, j)
					kfree(mbuf->buf);
				return -ENOMEM;
			}
			mbuf->len = cmd->mbuf_len;
		}
		ret = sprdwcn_bus_push_list(cmd->channel, head, tail,
					      cmd->mbuf_num);
		if (ret) {
			mbuf_list_iter(head, num, mbuf, i)
				kfree(mbuf->buf);
			sprdwcn_bus_list_free(cmd->channel, head, tail, num);
		}
	}
	return ret;
}

static int channel_loopback_enable[] = {3, 4, 5, 6};
static void *tx_buf[10];
static int checkdata_loopback_rx(int channel_id, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct channel *channel = chnmg_find_channel(this_chnmg, channel_id);
	int i;
	struct mbuf_t *mbuf;
	int rx_head;
	int ret;

	if (!channel || channel->status) {
		sprdwcn_bus_list_free(channel_id, head, tail, num);
		return 0;
	}
	mutex_lock(&channel->pool_lock);
	rx_head = channel->lp_rx_head;
	channel->lp_rx_head = rx_head + num;
	mutex_unlock(&channel->pool_lock);

	mbuf_list_iter(head, num, mbuf, i) {
		int j;
		char *check_buf = tx_buf[(i + rx_head) % ARRAY_SIZE(tx_buf)];

		for (j = 0; j < mbuf->len; j++) {
			if (((char *)mbuf->buf)[j] != check_buf[j])
				wcn_usb_test_print("%s check is not ok!\n",
						   __func__);
		}
	}
	ret = sprdwcn_bus_push_list(channel_id, head, tail, num);
	return ret;
}

static int checkdata_loopback_tx(int channel_id, struct mbuf_t *head,
		struct mbuf_t *tail, int num)
{
	struct channel *channel = chnmg_find_channel(this_chnmg, channel_id);
	int i;
	struct mbuf_t *mbuf;
	int ret = 0;

	if (!channel || channel->status) {
		sprdwcn_bus_list_free(channel_id, head, tail, num);
		return 0;
	}

	mutex_lock(&channel->pool_lock);
	mbuf_list_iter(head, num, mbuf, i) {
		mbuf->buf =
			tx_buf[(i + channel->lp_tx_head) % ARRAY_SIZE(tx_buf)];
	}
	ret = sprdwcn_bus_push_list(channel_id, head, tail, num);
	if (ret) {
		mbuf_list_iter(head, num, mbuf, i)
		sprdwcn_bus_list_free(channel_id, head, tail, num);
	} else {
		channel->lp_tx_head += num;
	}

	mutex_unlock(&channel->pool_lock);
	return ret;
}

static int wcn_usb_test_lp(struct chnmg *chnmg, struct usb_test_cmd_desc *cmd)
{
	struct channel *channel_tx;
	struct channel *channel_rx;
	int num = 1;
	int i;
	struct mbuf_t *head;
	struct mbuf_t *tail;
	struct mbuf_t *mbuf;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(channel_loopback_enable); i++) {
		if (i == channel_loopback_enable[i])
			break;
	}

	if (tx_buf[0] == NULL) {
		for (i = 0; i < ARRAY_SIZE(tx_buf); i++) {
			tx_buf[i] = kmalloc(2048, GFP_KERNEL);
			if (tx_buf[i] == NULL) {
				int j;

				for (j = 0; j < i; j++) {
					kfree(tx_buf[j]);
					tx_buf[j] = NULL;
				}
				return -ENOMEM;
			}
			memset(tx_buf[i], i+1, 2048);
		}
	}

	if (i == ARRAY_SIZE(channel_loopback_enable))
		return -EINVAL;

	channel_tx = channel_register(chnmg, cmd->channel,
					checkdata_loopback_tx);
	if (!channel_tx) {
		wcn_usb_test_print("%s channel tx init error\n", __func__);
		return -EIO;
	}
	channel_rx = channel_register(chnmg, cmd->channel + 16,
				   checkdata_loopback_rx);
	if (!channel_rx) {
		wcn_usb_test_print("%s channel rx init error\n", __func__);
		return -EIO;
	}
	channel_tx->status = cmd->command;
	if (channel_tx->status)
		return 0;

	/* every time sent one mbuf */
	ret = sprdwcn_bus_list_alloc(cmd->channel, &head, &tail, &num);
	if (ret || !head) {
		sprdwcn_bus_list_free(cmd->channel, head, tail, num);
		return -ENOMEM;
	}

	mutex_lock(&channel_tx->pool_lock);
	mbuf_list_iter(head, num, mbuf, i) {
		int index = (i + channel_tx->lp_tx_head) % ARRAY_SIZE(tx_buf);

		mbuf->buf = tx_buf[index];
		mbuf->len = cmd->mbuf_len;
	}
	ret = sprdwcn_bus_push_list(cmd->channel, head, tail, num);
	if (ret) {
		mbuf_list_iter(head, num, mbuf, i)
			kfree(mbuf->buf);
		sprdwcn_bus_list_free(cmd->channel, head, tail, num);
	} else {
		channel_tx->lp_tx_head += num;
	}

	mutex_unlock(&channel_tx->pool_lock);

	return ret;
}

static int wcn_usb_test_command(struct chnmg *chnmg, char *buf, int buf_len)
{
	struct usb_test_cmd_desc *test_cmd;
	struct wcn_usb_ep *ep;
	int actual_len;
	int ret = 0;

	ep = wcn_usb_store_get_epFRchn(TEST_COMMAND_CHAN);
	if (!ep)
		return -EIO;

	test_cmd = kzalloc(sizeof(struct usb_test_cmd_desc), GFP_KERNEL);
	if (test_cmd == NULL)
		return -ENOMEM;

	ret = sscanf(buf+3, "%d %d %d %d",
		     &test_cmd->channel,
		     &test_cmd->command,
		     &test_cmd->mbuf_num,
		     &test_cmd->mbuf_len);
	if (ret < 0) {
		ret = -EINVAL;
		goto ERROR;
	}
	if (strncmp(buf, "tp", strlen("tp")) == 0)
		test_cmd->type = 1;
	else if (strncmp(buf, "lb", strlen("lb")) == 0)
		test_cmd->type = 0;
	ret = wcn_usb_msg(ep, (void *) test_cmd,
			  sizeof(struct usb_test_cmd_desc),
			  &actual_len, 3000);
	if (ret || actual_len != sizeof(struct usb_test_cmd_desc)) {
		wcn_usb_test_print("%s usb_msg error ret%d len%d\n",
				   __func__, ret, actual_len);
		ret = -EIO;
		goto ERROR;
	}

	if (test_cmd->type == 1)
		wcn_usb_test_tp(chnmg, test_cmd);
	else
		wcn_usb_test_lp(chnmg, test_cmd);


ERROR:
	kfree(test_cmd);
	return ret;
}

static ssize_t wcn_usb_chnmg_build(struct file *file, const char *buffer,
		size_t count, loff_t *ppos)
{
	int channel_id;
	struct channel *channel;
	struct chnmg *chnmg;
	channel_callback pop_link;
	char buf[BUF_LEN];
	int ret;

	chnmg = file->private_data;

	memset(buf, 0, BUF_LEN);
	if (copy_from_user(buf, buffer, BUF_LEN))
		return -EFAULT;

	if (strncmp(buf, "tp", strlen("tp")) == 0
			|| strncmp(buf, "lb", strlen("lb")) == 0) {
		ret = wcn_usb_test_command(chnmg, buf, BUF_LEN);
		if (ret)
			return ret;
		*ppos = sizeof(struct channel) * (++chnmg->num_channels);
		return sizeof(struct channel);
	}

	channel_id = wcn_usb_chnmg_get_intFRuser(buffer, count);
	pop_link = get_channel_dir(channel_id) ? tx_pop_link : rx_pop_link;
	if (channel_id < 0 ||  channel_id > 32) {
		wcn_usb_test_print("%s channel_id overflow %d\n", __func__,
					channel_id);
		return -EINVAL;
	}

	channel = channel_register(chnmg, channel_id, pop_link);
	if (!channel) {
		wcn_usb_test_print("%s channel init error\n", __func__);
		return -EIO;
	}

	*ppos = sizeof(struct channel) * (++chnmg->num_channels);
	return sizeof(struct channel);
}

static const struct file_operations wcn_usb_chnmg_fops = {
	.owner = THIS_MODULE,
	.read = wcn_usb_chnmg_show,
	.write = wcn_usb_chnmg_build,
	.open = wcn_usb_chnmg_open,
	.release = wcn_usb_chnmg_release,
	.llseek = noop_llseek,
};

static ssize_t wcn_usb_chnmg_destroy(struct file *file, const char *buffer,
		size_t count, loff_t *ppos)
{
	int channel_id;
	struct channel *channel;
	struct chnmg *chnmg;

	chnmg = file->private_data;

	channel_id = wcn_usb_chnmg_get_intFRuser(buffer, count);
	if (channel_id < 0 ||  channel_id > 32) {
		wcn_usb_test_print("%s channel_id overflow %d\n", __func__,
				channel_id);
		return -EINVAL;
	}

	channel = chnmg_find_channel_destroy(chnmg, channel_id);
	if (!channel) {
		wcn_usb_test_print("%s channel is not existed!\n", __func__);
		return sizeof(struct channel);
	}

	channel_unregister(channel);
	channel_destroy(channel);

	*ppos = sizeof(struct channel) * (--chnmg->num_channels);
	return sizeof(struct channel);
}

static const struct file_operations wcn_usb_chnmg_defile_fops = {
	.owner = THIS_MODULE,
	.read = wcn_usb_chnmg_show,
	.write = wcn_usb_chnmg_destroy,
	.open = wcn_usb_chnmg_open,
	.release = wcn_usb_chnmg_release,
	.llseek = noop_llseek,
};

static int print_level_open(struct inode *inode, struct file *file)
{
	struct chnmg *chnmg;
	/* get channel_list head */
	chnmg = (struct chnmg *)pde_data(inode);

	file->private_data = chnmg;
	return 0;
}

static char wcn_usb_print_switch;
char get_wcn_usb_print_switch(void)
{
	return wcn_usb_print_switch;
}

static ssize_t print_level_write(struct file *file, const char *buffer,
		size_t count, loff_t *ppos)
{
	int level = wcn_usb_chnmg_get_intFRuser(buffer, count);

	wcn_usb_test_print("%s get level %d->%d\n",
			__func__, wcn_usb_print_switch, level);
	if (level < 0 || level > 16)
		return -EINVAL;

	wcn_usb_print_switch = level;
	return count;
}

static ssize_t print_level_read(struct file *file, char *buffer,
		size_t count, loff_t *ppos)
{
	void *kbuf;

	kbuf = kzalloc(16, GFP_KERNEL);
	if (kbuf == NULL)
		return -EIO;
	if ((*ppos)++ != 0) {
		kfree(kbuf);
		return 0;
	}
	snprintf(kbuf, 16, "%d\n", wcn_usb_print_switch | 0x0);
	if (copy_to_user(buffer, kbuf, 16)) {
		kfree(kbuf);
		return -EIO;
	}

	kfree(kbuf);
	return 16;
}

const struct file_operations print_level = {
	.owner = THIS_MODULE,
	.read = print_level_read,
	.write = print_level_write,
	.open = print_level_open,
	.release = wcn_usb_chnmg_release,
	.llseek = noop_llseek,
};


#define wcn_usb_channel_debug
#ifdef wcn_usb_channel_debug
struct channel_debug_info {
	atomic_t rx_tx_give_to_controller;
	atomic_t rx_tx_get_from_controller;
	atomic_t rx_tx_alloc;
	atomic_t rx_tx_free;

	atomic_t mbuf_alloc;
	atomic_t mbuf_free;
	atomic_t mbuf_give_to_user;
	atomic_t mbuf_get_from_user;

	atomic_t report_num;
	atomic_t to_accept;
	spinlock_t lock;
};

/*last channel is special */
struct channel_debug_info g_channel_debug[33];

void channel_debug_rx_tx_alloc(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].rx_tx_alloc));
}

void channel_debug_rx_tx_free(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].rx_tx_free));
}

void channel_debug_rx_tx_to_controller(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].rx_tx_give_to_controller));
}

void channel_debug_rx_tx_from_controller(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].rx_tx_get_from_controller));
}

void channel_debug_mbuf_alloc(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].mbuf_alloc));
}

void channel_debug_mbuf_free(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].mbuf_free));
}

void channel_debug_to_accept(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].to_accept));
}

void channel_debug_report_num(int chn, int num)
{
	atomic_set(&(g_channel_debug[chn].report_num), num);
}

void channel_debug_mbuf_to_user(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].mbuf_give_to_user));
}

void channel_debug_mbuf_from_user(int chn, int num)
{
	atomic_add(num, &(g_channel_debug[chn].mbuf_get_from_user));
}

void channel_debug_net_malloc(int times)
{
	channel_debug_rx_tx_alloc(32, times);
}

void channel_debug_net_free(int times)
{
	channel_debug_rx_tx_free(32, times);
}

void channel_debug_kzalloc(int times)
{
	channel_debug_rx_tx_to_controller(32, times);
}

void channel_debug_interrupt_callback(int times)
{
	channel_debug_rx_tx_from_controller(32, times);
}

void channel_debug_cp_num(int times)
{
	atomic_set(&(g_channel_debug[32].mbuf_alloc), times);
}

void channel_debug_packet_no_full(int times)
{
	channel_debug_mbuf_free(32, times);
}

void channel_debug_mbuf_8(int times)
{
#if 0
	channel_debug_mbuf_from_user(32, times);
#endif
}

void channel_debug_mbuf_10(int times)
{
#if 0
	channel_debug_mbuf_to_user(32, times);
#endif
}

void channel_debug_mbuf_4(int times)
{
	channel_debug_report_num(32, times);
}

void channel_debug_mbuf_1(int times)
{
	channel_debug_to_accept(32, times);
}

void channel_debug_alloc_big_men(int chn)
{
	if (chn == 6)
		channel_debug_mbuf_to_user(32, 1);
	else if (chn == 22)
		channel_debug_mbuf_from_user(32, 1);
}

void channel_debug_free_big_men(int chn)
{
	if (chn == 6)
		channel_debug_mbuf_to_user(32, -1);
	else if (chn == 22)
		channel_debug_mbuf_from_user(32, -1);
}

int channel_debug_snprint_tableinfo(char *buf, int buf_size)
{
	int ret;

	ret = snprintf(buf, buf_size, "chn\trx_tx_alloc\trx_tx_free");
	ret += snprintf(buf + ret, buf_size - ret,
			"\trx_tx_to_h\trx_tx_from_h");
	ret += snprintf(buf + ret, buf_size - ret,
			"\tmbuf_alloc\tmbuf_free");
	ret += snprintf(buf + ret, buf_size - ret,
			"\tmbuf_to_user\tmbuf_from_user");
	ret += snprintf(buf + ret, buf_size - ret,
			"\treport_num\tto_accept\n");
	return ret;
}

int channel_debug_snprint_special_info(char *buf, int buf_size)
{
	int ret;

	ret = snprintf(buf, buf_size, "sp\tnet_alloc\tnet_free");
	ret += snprintf(buf + ret, buf_size - ret,
			"\tdev_kmalloc\tinterrupt_cb");
	ret += snprintf(buf + ret, buf_size - ret,
			"\tint_22_cp_num\tPacketNoFull");
	ret += snprintf(buf + ret, buf_size - ret,
			"\tbig_men_for_tx\tbig_men_for_rx");
	ret += snprintf(buf + ret, buf_size - ret,
			"\tmbuf_more_4\tmbuf_eq_1\n");
	return ret;
}

int channel_debug_snprint(char *buf, int buf_size, int chn)
{
	int ret;

	ret = snprintf(buf, buf_size, "%.2d ", chn);
	ret += snprintf(buf + ret, buf_size - ret,
		"\t%.8d\t%.8d\t%.8d\t%.8d",
		atomic_read(&(g_channel_debug[chn].rx_tx_alloc)),
		atomic_read(&(g_channel_debug[chn].rx_tx_free)),
		atomic_read(&(g_channel_debug[chn].rx_tx_give_to_controller)),
		atomic_read(&(g_channel_debug[chn].rx_tx_get_from_controller)));
	ret += snprintf(buf + ret, buf_size - ret,
		"\t%.8d\t%.8d\t%.8d\t%.8d\t%.8d\t%.8d\n",
		atomic_read(&(g_channel_debug[chn].mbuf_alloc)),
		atomic_read(&(g_channel_debug[chn].mbuf_free)),
		atomic_read(&(g_channel_debug[chn].mbuf_give_to_user)),
		atomic_read(&(g_channel_debug[chn].mbuf_get_from_user)),
		atomic_read(&(g_channel_debug[chn].report_num)),
		atomic_read(&(g_channel_debug[chn].to_accept)));
	return ret;
}

static ssize_t wcn_usb_channel_debug_read(struct file *file, char *buffer,
		size_t count, loff_t *ppos)
{
	void *kbuf;
	int kbuf_size = 4096;
	int info_size = 0;
	int ret;
	int i;
	int copy_size;

	kbuf = kzalloc(kbuf_size, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	ret = channel_debug_snprint_tableinfo(kbuf, kbuf_size);
	if (ret < 0) {
		kfree(kbuf);
		return ret;
	}
	info_size += ret;

	for (i = 0; i < 32; i++) {
		ret = channel_debug_snprint(kbuf + info_size,
				kbuf_size - info_size, i);
		if (ret < 0) {
			kfree(kbuf);
			return ret;
		}
		info_size += ret;
	}

	ret = channel_debug_snprint_special_info(kbuf + info_size,
			kbuf_size - info_size);
	if (ret < 0) {
		kfree(kbuf);
		return ret;
	}
	info_size += ret;

	ret = channel_debug_snprint(kbuf + info_size,
			kbuf_size - info_size, 32);
	if (ret < 0) {
		kfree(kbuf);
		return ret;
	}
	info_size += ret;


	info_size += 1;
	if (*ppos >= info_size) {
		kfree(kbuf);
		return 0;
	}

	copy_size = count > info_size ? info_size : count;
	if (copy_to_user(buffer, kbuf, copy_size)) {
		kfree(kbuf);
		return -EIO;
	}

	kfree(kbuf);
	*ppos += copy_size;
	return copy_size;
}


static ssize_t wcn_usb_channel_debug_write(struct file *file,
		const char *buffer, size_t count, loff_t *ppos)
{
	return count;
}

static int wcn_usb_channel_debug_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int wcn_usb_channel_debug_release(struct inode *indoe, struct file *file)
{
	return 0;
}

const struct file_operations channel_debug = {
	.owner = THIS_MODULE,
	.read = wcn_usb_channel_debug_read,
	.write = wcn_usb_channel_debug_write,
	.open = wcn_usb_channel_debug_open,
	.release = wcn_usb_channel_debug_release,
	.llseek = noop_llseek,
};

#endif

struct dump_addr {
	u32 addr;
	u32 len;
};
static void test_dump(void)
{
	struct dump_addr dump_addr[] = {
	{0x40500000, 0x7ac00}, /* CP IRAM */
	{0x40580000, 0x1a800}, /* CP DRAM */
	{0x406a0000, 0x54000}, /* AON AHB RAM */
	{0x40f00000, 0x70000}, /* AON AXI RAM */
	{0x400f0000, 100}, /* AON AXI RAM */
	};
	unsigned int buf_size = 0x8000;
	void *buf;
	int ret;
	int i;
	int offset;
	unsigned int read_size;

	buf = kmalloc(buf_size, GFP_KERNEL);
	if (buf == NULL)
		return;

	for (i = 0; i < ARRAY_SIZE(dump_addr); i++) {
		offset = 0;
		while (offset < dump_addr[i].len) {
			read_size = dump_addr[i].len - offset;
			read_size = min(buf_size, read_size);
			ret = marlin_dump_from_romcode_usb(dump_addr[i].addr +
					offset, buf, read_size);
			if (ret) {
				wcn_usb_err("%s dump error\n", __func__);
				kfree(buf);
				return;
			}
			offset += read_size;
		}
	}
	kfree(buf);
	return;

}

static ssize_t wcn_usb_channel_romcode_write(struct file *file,
		const char *buffer, size_t count, loff_t *ppos)
{
	int action;
	int ret;

	action = wcn_usb_chnmg_get_intFRuser(buffer, count);
	switch (action) {
	case 1:
		ret = marlin_get_version();
		if (ret) {
			wcn_usb_err("%s start command is error\n", __func__);
			return ret;
		}
		break;
	case 2:
		ret = marlin_connet();
		if (ret) {
			wcn_usb_err("%s connect command is error\n", __func__);
			return ret;
		}
		break;
	case 3:
		marlin_get_wcn_chipid();
		break;
	case 4:
		test_dump();
		break;
	}
	return count;
}
static ssize_t wcn_usb_channel_romcode_read(struct file *file, char *buffer,
		size_t count, loff_t *ppos)
{
	void *buf;
	size_t buf_size = 0x7ABFF;
	int ret;

	if (*ppos >= buf_size)
		return 0;

	buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return 0;

	ret = marlin_dump_from_romcode_usb(0x40500000, buf, buf_size);
	if (ret) {
		kfree(buf);
		return 0;
	}

	buf_size = min_t(size_t, buf_size - *ppos, count);
	if (copy_to_user(buffer, buf + *ppos, buf_size)) {
		kfree(buf);
		wcn_usb_test_print("%s copy error\n", __func__);
		return -EIO;
	}

	*ppos += buf_size;
	return buf_size;
}

const struct file_operations romcode_test = {
	.owner = THIS_MODULE,
	.read = wcn_usb_channel_romcode_read,
	.write = wcn_usb_channel_romcode_write,
	.open = wcn_usb_channel_debug_open,
	.release = wcn_usb_channel_debug_release,
	.llseek = noop_llseek,
};

struct proc_dir_entry *proc_sys;
int wcn_usb_chnmg_init(void)
{
	struct chnmg *chnmg;
	int i;

#if 0
	/* do something to register wcn_bus */
	wcn_bus_init();
	sprdwcn_bus_preinit();
#endif
	proc_sys = proc_mkdir("wcn_usb", NULL);
	if (!proc_sys)
		wcn_usb_err("%s build proc sys error!\n", __func__);


	wcn_usb_test_print("%s into\n", __func__);
	chnmg = kzalloc(sizeof(struct chnmg) +
			sizeof(struct channel *) * WCN_USB_CHANNEL_MAX,
			GFP_KERNEL);
	if (!chnmg)
		return -ENOMEM;

	chnmg->dir = NULL;

	chnmg->file = proc_create_data("wcn_usb/chnmg", 0544, chnmg->dir,
			&wcn_usb_chnmg_fops, chnmg);
	if (!chnmg->file)
		goto CHNMG_FILE_ERROR;

	chnmg->defile = proc_create_data("wcn_usb/chnmg_destroy",
			0544, chnmg->dir, &wcn_usb_chnmg_defile_fops, chnmg);
	if (!chnmg->defile)
		goto CHNMG_FILE_ERROR;

	chnmg->print_level = proc_create_data("wcn_usb/print",
			0544, chnmg->dir, &print_level, chnmg);
	if (!chnmg->print_level)
		goto CHNMG_FILE_ERROR;

	chnmg->channel_debug = proc_create_data("wcn_usb/channel_debug",
			0544, chnmg->dir, &channel_debug, chnmg);
	if (!chnmg->channel_debug)
		goto CHNMG_FILE_ERROR;

	for (i = 0; i < 33; i++)
		spin_lock_init(&g_channel_debug[i].lock);

	chnmg->channel_debug = proc_create_data("wcn_usb/romcode_test",
			0544, chnmg->dir, &romcode_test, chnmg);

	wcn_usb_test_print("%s init success!\n", __func__);
	this_chnmg = chnmg;
	return 0;

CHNMG_FILE_ERROR:
	kfree(chnmg);
	return -ENOMEM;
}
#if 0
module_init(wcn_usb_chnmg_init);

static void wcn_usb_chnmg_exit(void)
{
	kfree(this_chnmg);
}
module_exit(wcn_usb_chnmg_exit);
#endif
