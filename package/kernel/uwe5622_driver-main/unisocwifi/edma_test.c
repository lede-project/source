#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/kernel.h>

#include "sprdwl.h"
#include "edma_test.h"
#include "wl_intf.h"

#define PCIE_CHANNEL_PAIR (8)
#define RUN 1
#define STOP 0
#define RAW_DATA_LEN 10

#define WL_SEQ_NUM_MASK			0xfff /* allow 12 bit */
#define WL_SEQ_NUM_SHIFT		0
#define WL_SEQ_SET_NUM(x, val)	((x) = \
	((x) & ~(WL_SEQ_NUM_MASK << WL_SEQ_NUM_SHIFT)) | \
	(((val) & WL_SEQ_NUM_MASK) << WL_SEQ_NUM_SHIFT))
#define WL_SEQ_GET_NUM(x)	(((x) >> WL_SEQ_NUM_SHIFT) & \
	WL_SEQ_NUM_MASK)

extern struct sprdwl_intf_ops g_intf_ops;
extern int if_tx_one(struct sprdwl_intf *intf, unsigned char *data,
			 int len, int chn);

unsigned long loop;

static struct dentry *chn_tx_dentry[8];
static unsigned int chn_tx_num[8];
static unsigned int chn_tx_success[8];
static unsigned int chn_tx_fail[8];

struct edma_test_cmd_header {
	u16 subcmd;
	u16 len;
	u8 data[0];
} __packed;

struct task_struct *task_array[PCIE_CHANNEL_PAIR];
int task_status;

void edma_transceive_test_init(void)
{
	int i, err;

	wl_err("Enter %s\n", __func__);

	for (i = 0; i < PCIE_CHANNEL_PAIR; i++) {
		task_array[i] = kthread_create(edma_transceive_test_exec,
					NULL, "edma_transceive_task");
		if (IS_ERR(task_array[i])) {
			err = PTR_ERR(task_array[i]);
			wl_err("%s, Fail, thread_num=%d,error_num=%d.",
				__func__, i, err);
		}
	}

	wl_err("Exit %s\n", __func__);
}

void edma_transceive_test_deinit(void)
{
	int i;

	wl_err("Enter %s\n", __func__);

	for (i = 0; i < PCIE_CHANNEL_PAIR; i++) {
		if (!IS_ERR_OR_NULL(task_array[i])) {
			wl_err(" %s, Destory thread, thread_num=%d,\n",
				   __func__, i);
			kthread_stop(task_array[i]);
			task_array[i] = NULL;
		}
	}

	wl_err("Exit %s\n", __func__);
}

int do_tx(int channel)
{
	int ret = 0;
	struct sprdwl_msg_buf *msg;
	struct edma_test_cmd_header *p;
	unsigned char raw_data[RAW_DATA_LEN];
	struct sprdwl_intf *intf = (struct sprdwl_intf *)g_intf_ops.intf;
	struct sprdwl_priv *priv = intf->priv;

	memset(raw_data, channel, RAW_DATA_LEN);

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + RAW_DATA_LEN,
				0, SPRDWL_HEAD_RSP,
				channel);
	if (!msg) {
		wl_err(" %s, No mem for msg,\n", __func__);
		return -ENOMEM;
	}

	p = (struct edma_test_cmd_header *)msg->data;
	p->subcmd = channel;
	p->len = RAW_DATA_LEN;
	memcpy(p->data, raw_data, RAW_DATA_LEN);

	sprdwl_queue_msg_buf(msg, msg->msglist);

	ret = if_tx_one(intf, msg->tran_data, msg->len, channel);

	if (ret) {
		wl_err("%s send cmd failed (ret=%d)\n", __func__, ret);
		kfree(msg->tran_data);
		msg->tran_data = NULL;
	}

	sprdwl_dequeue_msg_buf(msg, msg->msglist);

	return 0;
}


#define SPRDWL_SDIO_DEBUG_BUFLEN 128
static ssize_t pcie_read_info(struct file *file,
				  char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	size_t ret = 0;
	unsigned int buflen, len;
	unsigned char *buf;
	unsigned long chn = (unsigned long)file->private_data;

	buflen = SPRDWL_SDIO_DEBUG_BUFLEN;
	buf = kzalloc(buflen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len = 0;
	len += scnprintf(buf, buflen, "chn: %d, num: %d, success: %d, fail: %d\n",
			 ((int)chn)*2, chn_tx_num[chn], chn_tx_success[chn], chn_tx_fail[chn]);
	if (len > buflen)
		len = buflen;

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return ret;
}

static const struct file_operations pcie_debug_fops = {
	.read = pcie_read_info,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

const unsigned char *tx_name[] = {
	"edma_transceive_0",
	"edma_transceive_2",
	"edma_transceive_4",
	"edma_transceive_6",
	"edma_transceive_8",
	"edma_transceive_10",
	"edma_transceive_12",
	"edma_transceive_14",
};

void edma_transceive_test_run(int pairs)
{
	int i, err;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)g_intf_ops.intf;
	struct sprdwl_priv *priv = intf->priv;

	wl_info("Enter %s\n", __func__);

	memset(task_array, 0, sizeof(struct task_struct *)*PCIE_CHANNEL_PAIR);
	memset(chn_tx_dentry, 0, sizeof(struct dentry *)*PCIE_CHANNEL_PAIR);

	loop = 0;
	for (i = 0; i < pairs; i++) {
		chn_tx_num[i] = 0;
		chn_tx_success[i] = 0;
		chn_tx_fail[i] = 0;

		task_array[i] = kthread_create(edma_transceive_test_exec,
								(unsigned long *)loop,
								"edma_transceive_%d", i);
		chn_tx_dentry[i] = debugfs_create_file(tx_name[i], S_IRUSR, priv->debugfs, (unsigned long *)loop, &pcie_debug_fops);
		loop++;
		if (IS_ERR(task_array[i])) {
			err = PTR_ERR(task_array[i]);
			wl_err("%s, Fail, thread_num=%d,error_num=%d.",
				   __func__, i, err);
		} else {
			wl_info(" %s, Start thread, thread_num=%d,\n",
				__func__, i);
			wake_up_process(task_array[i]);
		}
	}

	wl_info("Exit %s\n", __func__);
}

void edma_transceive_test_stop(void)
{
	int i;

	wl_info("Enter %s\n", __func__);

	for (i = 0; i < PCIE_CHANNEL_PAIR; i++) {
		if (!IS_ERR_OR_NULL(task_array[i])) {
			wl_err(" %s, Stop thread, thread_num=%d,\n",
				   __func__, i);
			kthread_stop(task_array[i]);
			task_array[i] = NULL;
		}

		if (!IS_ERR_OR_NULL(chn_tx_dentry[i])) {
			wl_err(" %s, deinit tx dentry: %d\n", __func__, i);
			debugfs_remove(chn_tx_dentry[i]);
			chn_tx_dentry[i] = NULL;
		}
	}

	wl_info("Exit %s\n", __func__);
}

int edma_transceive_test_exec(void *args)
{
	unsigned long chn = (unsigned long)args;
	int ret = 0;
	//struct sched_param param;

	wl_err("Enter %s\n", __func__);

	while (1) {
		if (kthread_should_stop()) {
			wl_err(" %s, Exec thread, stopped.\n", __func__);
			break;
		}

		__set_current_state(TASK_RUNNING);
		wl_info(" %s, Exec thread, thread_num=%ld.\n", __func__, chn);
		/* do tx */
		chn_tx_num[chn]++;
		ret = do_tx((int)chn * 2);
		if (ret)
			chn_tx_fail[chn]++;
		else
			chn_tx_success[chn]++;
		msleep(1);
	}

	wl_err("Exit %s\n", __func__);

	return 0;
}

char *
__strtok(char **string, const char *delimiters, char *tokdelim)
{
	unsigned char *str;
	unsigned long map[8];
	int count;
	char *nextoken;

	if (tokdelim != NULL) {
		/* Prime the token delimiter */
		*tokdelim = '\0';
	}

	/* Clear control map */
	for (count = 0; count < 8; count++)
		map[count] = 0;

	/* Set bits in delimiter table */
	do {
		map[*delimiters >> 5] |= (1 << (*delimiters & 31));
	} while (*delimiters++);

	str = (unsigned char *)*string;

	/* Find beginning of token (skip over leading delimiters). Note that
	 * there is no token iff this loop sets str to point to the terminal
	 * null (*str == '\0')
	 */
	while (((map[*str >> 5] & (1 << (*str & 31))) &&
		*str) || (*str == ' '))
		str++;

	nextoken = (char *)str;

	/* Find the end of the token. If it is not the end of the string,
	 * put a null there.
	 */
	for (; *str; str++) {
		if (map[*str >> 5] & (1 << (*str & 31))) {
			if (tokdelim != NULL)
				*tokdelim = *str;

			*str++ = '\0';
			break;
		}
	}

	*string = (char *)str;

	/* Determine if a token has been found. */
	if (nextoken == (char *)str)
		return NULL;
	else
		return nextoken;
}

void edma_transceive_test_trigger(char *cmd)
{
	long oper, pairs;
	char *delim_1 = "=";
	char *delim_2 = ",";

	__strtok(&cmd, delim_1, 0);
	/* oper = (int)simple_strtol(cmd, NULL, 0); */
	if (kstrtol(cmd, 10, &oper) < 0) {
		wl_err("%s, %s is invalid\n", __func__, cmd);
		return;
	}


	__strtok(&cmd, delim_2, 0);
	/* pairs = (int)simple_strtol(cmd, NULL, 0); */
	if (kstrtol(cmd, 10, &pairs) < 0) {
		wl_err("%s, %s is invalid\n", __func__, cmd);
		return;
	}

	wl_err("Enter %s\n", __func__);

	if (pairs <= 0 || pairs > PCIE_CHANNEL_PAIR) {
		wl_err("%s, Invalid chn pairs(%ld), use default val(8)\n",
			   __func__, pairs);
		pairs = PCIE_CHANNEL_PAIR;
	}

	switch (oper) {
	case RUN:
		wl_err("%s, Run task\n", __func__);
		if (task_status == RUN) {
			wl_err("%s, Already running\n", __func__);
			return;
		}

		edma_transceive_test_run((int)pairs);
		task_status = RUN;
		break;
	case STOP:
		wl_err("%s, Stop task\n", __func__);
		if (task_status == STOP) {
			wl_err("%s, Already stopped\n", __func__);
			return;
		}

		edma_transceive_test_stop();
		task_status = STOP;
		break;
	default:
		wl_err("%s, Unknown trigger\n", __func__);
		break;
	}

	wl_err("Exit %s\n", __func__);
}
