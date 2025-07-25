/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Filename : slp_mgr.c
 * Abstract : This file is a implementation for  sleep manager
 *
 * Authors	: sam.sun
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/version.h>
//#include <linux/wakelock.h>
#include <wcn_bus.h>
#include "../sdio/sdiohal.h"
#include "slp_mgr.h"
#include "slp_sdio.h"
#include "wcn_glb.h"

struct slp_mgr_t slp_mgr;

struct slp_mgr_t *slp_get_info(void)
{
	return &slp_mgr;
}

void slp_mgr_drv_sleep(enum slp_subsys subsys, bool enable)
{
	mutex_lock(&(slp_mgr.drv_slp_lock));
	if (enable)
		slp_mgr.active_module &= ~(BIT(subsys));
	else
		slp_mgr.active_module |= (BIT(subsys));
	if (slp_mgr.active_module == 0) {
		/* If pubint pin is valid (not used as bt_wake_host pin),
		 * packet mode will enter sleep when pub int irq coming.
		 */
		if (marlin_get_bt_wl_wake_host_en()) {
			slp_allow_sleep();
			atomic_set(&(slp_mgr.cp2_state), STAY_SLPING);
		} else if (subsys > PACKER_DT_RX) {
			slp_allow_sleep();
			atomic_set(&(slp_mgr.cp2_state), STAY_SLPING);
		}
	}
	mutex_unlock(&(slp_mgr.drv_slp_lock));
}

int slp_mgr_wakeup(enum slp_subsys subsys)
{
	unsigned char slp_sts;
	int ret;
	int do_dump = 0;
	ktime_t time_end;
#if KERNEL_VERSION(3, 16, 75) > LINUX_VERSION_CODE
	ktime_t time_cmp;
#endif
	mutex_lock(&(slp_mgr.wakeup_lock));
	if (STAY_SLPING == (atomic_read(&(slp_mgr.cp2_state)))) {
		ap_wakeup_cp();
		time_end = ktime_add_ms(ktime_get(), 30);
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5623
		/*select btwf_slp_status*/
		sprdwcn_bus_aon_writeb(REG_CP_PMU_SEL_CTL, 6);
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
		if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E)
			sprdwcn_bus_aon_writeb(REG_CP_PMU_SEL_CTL, 6);
#endif
		while (1) {
			ret = sprdwcn_bus_aon_readb(REG_BTWF_SLP_STS, &slp_sts);
			if (ret < 0) {
				SLP_MGR_ERR("read slp sts err:%d", ret);
				usleep_range(40, 80);
				goto try_timeout;
			}

			slp_sts &= SLEEP_STATUS_FLAG;
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
			if ((slp_sts != BTWF_IN_DEEPSLEEP) &&
			   (slp_sts != BTWF_IN_DEEPSLEEP_XLT_ON) &&
#if defined(CONFIG_UWE5622) || defined(CONFIG_UWE5623)
			   (slp_sts != BTWF_PLL_PWR_WAIT) &&
			   (slp_sts != BTWF_XLT_WAIT) &&
			   (slp_sts != BTWF_XLTBUF_WAIT))
#else
			   (slp_sts != BTWF_IN_DEEPSLEEP_XLT_ON))
#endif
			{
#if defined(CONFIG_UWE5623)
				unsigned int reg_val = 0;

				sprdwcn_bus_reg_read(CP_WAKE_STATUS,
							 &reg_val, 4);
				if ((reg_val & BIT(31)) == 0)
#endif
					break;
			}
#else
			if ((slp_sts != BTWF_IN_DEEPSLEEP) &&
			   (slp_sts != BTWF_IN_DEEPSLEEP_XLT_ON) &&
			   ((((wcn_get_chip_model() == WCN_CHIP_MARLIN3E) ||
			   (wcn_get_chip_model() == WCN_CHIP_MARLIN3L)) &&
			   (slp_sts != BTWF_PLL_PWR_WAIT) &&
			   (slp_sts != BTWF_XLT_WAIT) &&
			   (slp_sts != BTWF_XLTBUF_WAIT)) ||
			   (wcn_get_chip_model() == WCN_CHIP_MARLIN3))) {
				if (wcn_get_chip_model() ==
					WCN_CHIP_MARLIN3E) {
					unsigned int reg_val = 0;

					sprdwcn_bus_reg_read(CP_WAKE_STATUS,
								 &reg_val, 4);
					if ((reg_val & BIT(31)) == 0)
						break;
				} else
					break;
			}
#endif
try_timeout:
			//SLP_MGR_INFO("slp_sts-0x%x", slp_sts);
			if (do_dump) {
				atomic_set(&(slp_mgr.cp2_state), STAY_AWAKING);
				SLP_MGR_INFO("wakeup fail, slp_sts-0x%x",
						 slp_sts);
				sdiohal_dump_aon_reg();
				mutex_unlock(&(slp_mgr.wakeup_lock));
				return -1;
			}
#if KERNEL_VERSION(3, 16, 75) <= LINUX_VERSION_CODE
			/* kernelv3.16.75 add ktime_after function. */
			if (ktime_after(ktime_get(), time_end))
				do_dump = 1;
#else
			time_cmp = ktime_get();
			if (time_cmp.tv64 > time_end.tv64)
				do_dump = 1;
#endif
		}

		atomic_set(&(slp_mgr.cp2_state), STAY_AWAKING);
	}
	mutex_unlock(&(slp_mgr.wakeup_lock));

	return 0;
}

/* called after chip power on, and reset sleep status */
void slp_mgr_reset(void)
{
	atomic_set(&(slp_mgr.cp2_state), STAY_AWAKING);
	reinit_completion(&(slp_mgr.wakeup_ack_completion));
}

int slp_mgr_init(void)
{
	SLP_MGR_DBG("%s enter\n", __func__);

	atomic_set(&(slp_mgr.cp2_state), STAY_AWAKING);
	mutex_init(&(slp_mgr.drv_slp_lock));
	mutex_init(&(slp_mgr.wakeup_lock));
	init_completion(&(slp_mgr.wakeup_ack_completion));
	slp_pub_int_RegCb();
#ifdef SLP_MGR_TEST
	slp_test_init();
#endif

	SLP_MGR_DBG("%s ok!\n", __func__);

	return 0;
}
EXPORT_SYMBOL(slp_mgr_init);

int slp_mgr_deinit(void)
{
	SLP_MGR_DBG("%s enter\n", __func__);
	atomic_set(&(slp_mgr.cp2_state), STAY_SLPING);
	slp_mgr.active_module = 0;
	mutex_destroy(&(slp_mgr.drv_slp_lock));
	mutex_destroy(&(slp_mgr.wakeup_lock));
	SLP_MGR_DBG("%s ok!\n", __func__);

	return 0;
}
EXPORT_SYMBOL(slp_mgr_deinit);
