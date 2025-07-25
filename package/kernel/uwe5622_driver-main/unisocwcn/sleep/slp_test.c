#include <linux/kthread.h>
#include <linux/delay.h>
#include <wcn_bus.h>

#include "slp_mgr.h"
#include "slp_sdio.h"
static int test_cnt;
int sleep_test_thread(void *data)
{
	unsigned int ram_val;

	while (1) {
		if (test_cnt)
			msleep(5000);
		else
			msleep(30000);

		sprdwcn_bus_reg_read(0x40500000, &ram_val, 0x4);
		SLP_MGR_INFO("ram_val is 0x%x\n", ram_val);
		msleep(5000);
		test_cnt++;
	}
}

static struct task_struct *slp_test_task;
int slp_test_init(void)
{
	SLP_MGR_INFO("create slp_mgr test thread\n");
	if (!slp_test_task)
		slp_test_task = kthread_create(sleep_test_thread,
			NULL, "sleep_test_thread");
	if (slp_test_task != 0) {
		wake_up_process(slp_test_task);
		return 0;
	}

	SLP_MGR_ERR("create sleep_test_thread fail\n");

	return -1;
}
