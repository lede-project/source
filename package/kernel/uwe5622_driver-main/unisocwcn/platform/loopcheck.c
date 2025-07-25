#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

#include "wcn_glb.h"
#include "wcn_procfs.h"

#define LOOPCHECK_TIMER_INTERVAL      5
#define WCN_LOOPCHECK_INIT	1
#define WCN_LOOPCHECK_OPEN	2

#ifdef CONFIG_WCN_LOOPCHECK
struct wcn_loopcheck {
	unsigned long status;
	struct completion completion;
	struct delayed_work work;
	struct workqueue_struct *workqueue;
};

static struct wcn_loopcheck loopcheck;
#endif
static struct completion atcmd_completion;
static struct mutex atcmd_lock;

int at_cmd_send(char *buf, unsigned int len)
{
	unsigned char *send_buf = NULL;
	struct mbuf_t *head, *tail;
	int num = 1;

	WCN_DEBUG("%s len=%d\n", __func__, len);
	if (unlikely(marlin_get_module_status() != true)) {
		WCN_ERR("WCN module have not open\n");
		return -EIO;
	}

	send_buf = kzalloc(len + PUB_HEAD_RSV + 1, GFP_KERNEL);
	if (!send_buf)
		return -ENOMEM;
	memcpy(send_buf + PUB_HEAD_RSV, buf, len);

	if (!sprdwcn_bus_list_alloc(mdbg_proc_ops[MDBG_AT_TX_OPS].channel,
				    &head, &tail, &num)) {
		head->buf = send_buf;
		head->len = len;
		head->next = NULL;
		sprdwcn_bus_push_list(mdbg_proc_ops[MDBG_AT_TX_OPS].channel,
				      head, tail, num);
	}
	return 0;
}

#ifdef CONFIG_WCN_LOOPCHECK
static void loopcheck_work_queue(struct work_struct *work)
{
	int ret;
	unsigned long timeleft;
	char a[] = "at+loopcheck\r\n";

	if (!test_bit(WCN_LOOPCHECK_OPEN, &loopcheck.status))
		return;
	mutex_lock(&atcmd_lock);
	at_cmd_send(a, sizeof(a));

	timeleft = wait_for_completion_timeout(&loopcheck.completion,
					       (3 * HZ));
	mutex_unlock(&atcmd_lock);
	if (!test_bit(WCN_LOOPCHECK_OPEN, &loopcheck.status))
		return;
	if (!timeleft) {
		stop_loopcheck();
		WCN_ERR("didn't get loopcheck ack\n");
		WCN_INFO("start dump CP2 mem\n");
		mdbg_assert_interface("loopcheck fail");
		return;
	}

	ret = queue_delayed_work(loopcheck.workqueue, &loopcheck.work,
				 LOOPCHECK_TIMER_INTERVAL * HZ);
}
#endif

void switch_cp2_log(bool flag)
{
	unsigned long timeleft;
	char a[32];
	unsigned char ret;

	WCN_INFO("%s - %s entry!\n", __func__, (flag ? "open" : "close"));
	mutex_lock(&atcmd_lock);
	sprintf(a, "at+armlog=%d\r\n", (flag ? 1 : 0));
	ret = at_cmd_send(a, sizeof(a));
	if (ret) {
		mutex_unlock(&atcmd_lock);
		WCN_ERR("%s fail!\n", __func__);
		return;
	}
	timeleft = wait_for_completion_timeout(&atcmd_completion, (3 * HZ));
	mutex_unlock(&atcmd_lock);
	if (!timeleft)
		WCN_ERR("didn't get %s ack\n", __func__);
}

int get_board_ant_num(void)
{
	unsigned long timeleft;
	char a[] = "at+getantnum\r\n";
	unsigned char *at_cmd_buf;
	unsigned char ret;

	WCN_DEBUG("%s entry!\n", __func__);

	/* 1. uwe5621 module on RK board(rk3368):
	 * Antenna num is fixed to one.
	 */
#ifdef CONFIG_RK_BOARD
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5621
	WCN_INFO("%s [one_ant]\n", __func__);
	return MARLIN_ONE_ANT;
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3) {
		WCN_INFO("%s [one_ant]\n", __func__);
		return MARLIN_ONE_ANT;
	}
#endif
#endif /* CONFIG_RK_BOARD */

	/* 2. uwe5622 module:
	 * Antenna num is fixed to one.
	 */
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
	WCN_INFO("%s [one_ant]\n", __func__);
	return MARLIN_ONE_ANT;
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
		WCN_INFO("%s [one_ant]\n", __func__);
		return MARLIN_ONE_ANT;
	}
#endif

	/* 3. Other situations:
	 * send at cmd to get antenna num.
	 */
	mutex_lock(&atcmd_lock);
	ret = at_cmd_send(a, sizeof(a));
	if (ret) {
		mutex_unlock(&atcmd_lock);
		WCN_ERR("%s fail!\n", __func__);
		return ret;
	}
	timeleft = wait_for_completion_timeout(&atcmd_completion, (3 * HZ));
	if (!timeleft) {
		mutex_unlock(&atcmd_lock);
		WCN_ERR("didn't get board ant num, default:[three_ant]\n");
		return MARLIN_THREE_ANT;
	}
	at_cmd_buf = mdbg_get_at_cmd_buf();
	mutex_unlock(&atcmd_lock);
	if (at_cmd_buf[0] == '2') {
		WCN_INFO("%s [two_ant]\n", __func__);
		return MARLIN_TWO_ANT;
	} else if (at_cmd_buf[0] == '3') {
		WCN_INFO("%s [three_ant]\n", __func__);
		return MARLIN_THREE_ANT;
	}
	WCN_ERR("%s read err:%s, default:[three_ant]\n",
		__func__, at_cmd_buf);
	return MARLIN_THREE_ANT;
}

void get_cp2_version(void)
{
	unsigned long timeleft;
	char a[] = "at+spatgetcp2info\r\n";
	unsigned char ret;

	WCN_INFO("%s entry!\n", __func__);
	mutex_lock(&atcmd_lock);
	ret = at_cmd_send(a, sizeof(a));
	if (ret) {
		mutex_unlock(&atcmd_lock);
		WCN_ERR("%s fail!\n", __func__);
		return;
	}
	timeleft = wait_for_completion_timeout(&atcmd_completion, (3 * HZ));
	mutex_unlock(&atcmd_lock);
	if (!timeleft)
		WCN_ERR("didn't get CP2 version\n");
}

#ifdef CONFIG_WCN_LOOPCHECK
void start_loopcheck(void)
{
	if (!test_bit(WCN_LOOPCHECK_INIT, &loopcheck.status) ||
	    test_and_set_bit(WCN_LOOPCHECK_OPEN, &loopcheck.status))
		return;
	WCN_INFO("%s\n", __func__);
	reinit_completion(&loopcheck.completion);
	queue_delayed_work(loopcheck.workqueue, &loopcheck.work, HZ);
}

void stop_loopcheck(void)
{
	if (!test_bit(WCN_LOOPCHECK_INIT, &loopcheck.status) ||
	    !test_and_clear_bit(WCN_LOOPCHECK_OPEN, &loopcheck.status))
		return;
	WCN_INFO("%s\n", __func__);
	complete_all(&loopcheck.completion);
	cancel_delayed_work_sync(&loopcheck.work);
}

void complete_kernel_loopcheck(void)
{
	complete(&loopcheck.completion);
}
#endif

void complete_kernel_atcmd(void)
{
	complete(&atcmd_completion);
}

int loopcheck_init(void)
{
#ifdef CONFIG_WCN_LOOPCHECK
	WCN_DEBUG("loopcheck_init\n");
	loopcheck.status = 0;
	init_completion(&loopcheck.completion);
	loopcheck.workqueue =
			create_singlethread_workqueue("WCN_LOOPCHECK_QUEUE");
	if (!loopcheck.workqueue) {
		WCN_ERR("WCN_LOOPCHECK_QUEUE create failed");
		return -ENOMEM;
	}
	set_bit(WCN_LOOPCHECK_INIT, &loopcheck.status);
	INIT_DELAYED_WORK(&loopcheck.work, loopcheck_work_queue);
#endif
	init_completion(&atcmd_completion);
	mutex_init(&atcmd_lock);

	return 0;
}

int loopcheck_deinit(void)
{
#ifdef CONFIG_WCN_LOOPCHECK
	stop_loopcheck();
	destroy_workqueue(loopcheck.workqueue);
	loopcheck.status = 0;
#endif
	mutex_destroy(&atcmd_lock);

	return 0;
}
