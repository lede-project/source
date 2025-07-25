#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <marlin_platform.h>
#include <wcn_bus.h>
#include "sdio_int.h"
#include "slp_mgr.h"
#include "slp_sdio.h"
#include "wcn_glb.h"
#include "../sdio/sdiohal.h"

struct sdio_int_t sdio_int = {0};

atomic_t flag_pub_int_done;
bool sdio_power_notify = FALSE;

static inline int sdio_pub_int_clr0(unsigned char int_sts0)
{
	return sprdwcn_bus_aon_writeb(sdio_int.pub_int_clr0,
			int_sts0);
}

bool sdio_get_power_notify(void)
{
	return sdio_power_notify;
}

void sdio_record_power_notify(bool notify_cb_sts)
{
	sdio_power_notify = notify_cb_sts;
}

void sdio_wait_pub_int_done(void)
{
	struct slp_mgr_t *slp_mgr;
	int wait_cnt = 0;

	if (sdio_int.pub_int_num <= 0)
		return;

	slp_mgr = slp_get_info();

	if (sdio_power_notify) {
		/* enter suspend, means no tx data to cp2, so set sleep*/
		mutex_lock(&(slp_mgr->drv_slp_lock));
		if (atomic_read(&(slp_mgr->cp2_state)) == STAY_AWAKING) {
			SLP_MGR_INFO("allow sleep1\n");
			slp_allow_sleep();
			atomic_set(&(slp_mgr->cp2_state), STAY_SLPING);
		}
		mutex_unlock(&(slp_mgr->drv_slp_lock));

		/* wait pub_int handle finish*/
		while ((atomic_read(&flag_pub_int_done) == 0) &&
			   (wait_cnt < 10)) {
			wait_cnt++;
			SLP_MGR_INFO("wait pub_int_done:%d", wait_cnt);
			usleep_range(1500, 3000);
		}
		SLP_MGR_INFO("flag_pub_int_done-%d",
			atomic_read(&flag_pub_int_done));
	} else
		SLP_MGR_INFO("sdio power_notify is NULL");
}
EXPORT_SYMBOL(sdio_wait_pub_int_done);

int pub_int_handle_thread(void *data)
{
	union PUB_INT_STS0_REG pub_int_sts0 = {0};
	int bit_num, ret;

	while (!kthread_should_stop()) {
		/* wait_for_completion may cause hung_task_timeout_secs
		 * with message of task blocked for more than 120 seconds.
		 */
		wait_for_completion_interruptible(
			&(sdio_int.pub_int_completion));

		ret = sprdwcn_bus_aon_readb(sdio_int.pub_int_sts0,
			&(pub_int_sts0.reg));
		/* sdio cmd52 fail, it should be chip power off */
		if (ret < 0)
			SLP_MGR_INFO("sdio cmd52 fail, ret-%d", ret);
		else {
			SLP_MGR_INFO("PUB_INT_STS0-0x%x\n", pub_int_sts0.reg);
			sdio_pub_int_clr0(pub_int_sts0.reg);

			bit_num = 0;
			do {
				if ((pub_int_sts0.reg & BIT(bit_num)) &&
					sdio_int.pub_int_cb[bit_num]) {
					sdio_int.pub_int_cb[bit_num]();
				}
				bit_num++;
			} while (bit_num < PUB_INT_MAX);
		}

		if (sdio_power_notify)
			atomic_set(&flag_pub_int_done, 1);
		else {
			__pm_relax((sdio_int.pub_int_ws));
		}

		/* enable interrupt, balance with disable in pub_int_isr */
		if (atomic_read(&(sdio_int.chip_power_on)))
			enable_irq(sdio_int.pub_int_num);
	}

	return 0;
}

static int irq_cnt;
static irqreturn_t pub_int_isr(int irq, void *para)
{
	disable_irq_nosync(irq);
	/*
	 * for wifi powersave special handle, when wifi driver send
	 * power save cmd to cp2, then pub int can't take wakelock,
	 * or ap can't enter deep sleep.
	 */
	if (sdio_power_notify)
		atomic_set(&flag_pub_int_done, 0);
	else {
		__pm_stay_awake((sdio_int.pub_int_ws));
	}

	irq_cnt++;
	SLP_MGR_INFO("irq_cnt%d!!", irq_cnt);

	complete(&(sdio_int.pub_int_completion));

	return IRQ_HANDLED;
}

static struct task_struct *pub_int_handle_task;
static int sdio_isr_handle_init(void)
{
	if (!pub_int_handle_task)
		pub_int_handle_task = kthread_create(pub_int_handle_thread,
			NULL, "pub_int_handle_thread");
	if (pub_int_handle_task != 0) {
		wake_up_process(pub_int_handle_task);
		return 0;
	}

	SLP_MGR_INFO("%s ok!\n", __func__);

	return -1;
}

static int sdio_pub_int_register(int irq)
{
	int ret = 0;

	SLP_MGR_INFO("public_int, gpio-%d\n", irq);

	if (irq <= 0)
		return ret;

	ret = gpio_direction_input(irq);
	if (ret < 0) {
		SLP_MGR_ERR("public_int, gpio-%d input set fail!!!", irq);
		return ret;
	}

	sdio_int.pub_int_num = gpio_to_irq(irq);
	SLP_MGR_INFO("public_int, intnum-%d\n", sdio_int.pub_int_num);

	ret = request_irq(sdio_int.pub_int_num,
			pub_int_isr,
			IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND,
			"pub_int_isr",
			NULL);
	if (ret != 0) {
		SLP_MGR_ERR("req irq-%d err!!!", sdio_int.pub_int_num);
		return ret;
	}

	/* enable interrupt when chip power on */
	disable_irq(sdio_int.pub_int_num);

	return ret;
}

int sdio_ap_int_cp0(enum AP_INT_CP_BIT bit)
{
	union AP_INT_CP0_REG reg_int_cp0 = {0};

	switch (bit) {

	case ALLOW_CP_SLP:
		reg_int_cp0.bit.allow_cp_slp = 1;
		break;

	case WIFI_BIN_DOWNLOAD:
		reg_int_cp0.bit.wifi_bin_download = 1;
		break;

	case BT_BIN_DOWNLOAD:
		reg_int_cp0.bit.bt_bin_download = 1;
		break;
	case SAVE_CP_MEM:
		reg_int_cp0.bit.save_cp_mem = 1;
		break;
	case TEST_DEL_THREAD:
		reg_int_cp0.bit.test_delet_thread = 1;
		break;

	default:
		SLP_MGR_INFO("ap_int_cp bit error");
		break;
	}

	return sprdwcn_bus_aon_writeb(sdio_int.ap_int_cp0,
			reg_int_cp0.reg);
}
EXPORT_SYMBOL(sdio_ap_int_cp0);

int sdio_pub_int_RegCb(enum PUB_INT_BIT bit,
		PUB_INT_ISR isr_handler)
{
	if (sdio_int.pub_int_num <= 0)
		return 0;

	if (isr_handler == NULL) {
		SLP_MGR_ERR("pub_int_RegCb error !!");
		return -1;
	}

	sdio_int.pub_int_cb[bit] = isr_handler;

	SLP_MGR_INFO("0X%x pub_int_RegCb", bit);

	return 0;
}
EXPORT_SYMBOL(sdio_pub_int_RegCb);

int sdio_pub_int_btwf_en0(void)
{
	union PUB_INT_EN0_REG reg_int_en = {0};

	if (sdio_int.pub_int_num <= 0)
		return 0;

	sprdwcn_bus_aon_readb(sdio_int.pub_int_en0, &(reg_int_en.reg));

	reg_int_en.bit.req_slp = 1;
	reg_int_en.bit.mem_save_bin = 1;
	reg_int_en.bit.wifi_open = 1;
	reg_int_en.bit.bt_open = 1;
	reg_int_en.bit.wifi_close = 1;
	reg_int_en.bit.bt_close = 1;
	sprdwcn_bus_aon_writeb(sdio_int.pub_int_en0, reg_int_en.reg);

	SLP_MGR_INFO("%s ok!\n", __func__);

	return 0;
}
EXPORT_SYMBOL(sdio_pub_int_btwf_en0);

int sdio_pub_int_gnss_en0(void)
{
	union PUB_INT_EN0_REG reg_int_en = {0};

	if (sdio_int.pub_int_num <= 0)
		return 0;

	sprdwcn_bus_aon_readb(sdio_int.pub_int_en0, &(reg_int_en.reg));

	reg_int_en.bit.gnss_cali_done = 1;

	sprdwcn_bus_aon_writeb(sdio_int.pub_int_en0, reg_int_en.reg);

	SLP_MGR_INFO("%s ok!\n", __func__);

	return 0;
}
EXPORT_SYMBOL(sdio_pub_int_gnss_en0);


void sdio_pub_int_poweron(bool state)
{
	if (sdio_int.pub_int_num <= 0)
		return;

	atomic_set(&(sdio_int.chip_power_on), state);

	if (state)
		enable_irq(sdio_int.pub_int_num);
	else {
		disable_irq(sdio_int.pub_int_num);
		reinit_completion(&(sdio_int.pub_int_completion));
	}
}
EXPORT_SYMBOL(sdio_pub_int_poweron);

int sdio_pub_int_init(int irq)
{
	if (irq <= 0) {
		sdio_int.pub_int_num = 0;
		return 0;
	}

	sdio_int.cp_slp_ctl = REG_CP_SLP_CTL;
	sdio_int.ap_int_cp0 = REG_AP_INT_CP0;
	sdio_int.pub_int_en0 = REG_PUB_INT_EN0;
	sdio_int.pub_int_clr0 = REG_PUB_INT_CLR0;
	sdio_int.pub_int_sts0 = REG_PUB_INT_STS0;

	atomic_set(&flag_pub_int_done, 1);

	/*wakeup_source pointer*/
	sdio_int.pub_int_ws = wakeup_source_create("pub_int_ws");
	wakeup_source_add(sdio_int.pub_int_ws);

	init_completion(&(sdio_int.pub_int_completion));

	sdio_pub_int_register(irq);

	sdio_isr_handle_init();

	SLP_MGR_INFO("%s ok!\n", __func__);

	return 0;
}
EXPORT_SYMBOL(sdio_pub_int_init);

int sdio_pub_int_deinit(void)
{
	if (sdio_int.pub_int_num <= 0)
		return 0;

	atomic_set(&flag_pub_int_done, 1);
	if (pub_int_handle_task) {
		disable_irq(sdio_int.pub_int_num);
		complete(&(sdio_int.pub_int_completion));
		kthread_stop(pub_int_handle_task);
		pub_int_handle_task = NULL;
	}

	sdio_power_notify = FALSE;
	disable_irq(sdio_int.pub_int_num);
	free_irq(sdio_int.pub_int_num, NULL);

	/*wakeup_source pointer*/
	wakeup_source_remove(sdio_int.pub_int_ws);
	wakeup_source_destroy(sdio_int.pub_int_ws);

	SLP_MGR_INFO("%s ok!\n", __func__);

	return 0;
}
EXPORT_SYMBOL(sdio_pub_int_deinit);

