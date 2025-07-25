/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MDBG_TYPE_H
#define _MDBG_TYPE_H

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define WCN_DEBUG_ON 1
#define WCN_DEBUG_OFF 0

extern u32 wcn_print_level;

#define MDBG_HEADER		"MDBG: "
#define MDBG_HEADER_ERR		"WCN_MDBG_ERR: "
#define MDBG_DEBUG_MODE		0

#define WCN_HEADER		"WCN: "
#define WCN_HEADER_ERR		"WCN_ERR: "
#define WCN_HEADER_DEBUG		"WCN_DEBUG: "

#define WCN_INFO(fmt, args...) \
	pr_info(WCN_HEADER fmt, ## args)

#define WCN_ERR(fmt, args...) \
		pr_err(WCN_HEADER_ERR fmt,  ## args)

#define WCN_DEBUG(fmt, args...) do { \
	if (wcn_print_level ==  WCN_DEBUG_ON)\
		pr_info(WCN_HEADER_DEBUG"%s: [%d]:" fmt"\n",\
		__func__, __LINE__, ## args);\
} while (0)

#define MDBG_ERR(fmt, args...) \
	pr_info(MDBG_HEADER_ERR fmt, ## args)

#if MDBG_DEBUG_MODE
#define MDBG_LOG(fmt, args...)	pr_err(MDBG_HEADER"%s  %d:" fmt \
				"\n", __func__, __LINE__, ## args)
#else
#define MDBG_LOG(fmt, args...)
#endif

#ifdef CONFIG_PRINTK
#define wcn_printk_ratelimited(inter, burst, now, fmt, ...)	\
({								\
	static DEFINE_RATELIMIT_STATE(_wcn_rs,			\
				      (inter * HZ), burst);	\
	static unsigned int _wcn_last;				\
								\
	if (__ratelimit(&_wcn_rs)) {				\
		printk(fmt " [rate:%u]\n", ##__VA_ARGS__,	\
		       (now - _wcn_last) / inter);		\
		_wcn_last = now;				\
	}							\
})
#else
#define wcn_printk_ratelimited(inter, burst, now, fmt, ...)
#endif

#define wcn_pr_daterate(inter, burst, now, fmt, ...)		\
	wcn_printk_ratelimited(inter, burst, now,		\
			KERN_INFO "WCN: " pr_fmt(fmt), ##__VA_ARGS__)

#define MDBG_FUNC_ENTERY	MDBG_LOG("ENTER.")
#define MDBG_FUNC_EXIT		MDBG_LOG("EXIT.")

#define MDBG_SUCCESS		0
#define MDBG_ERR_RING_FULL	1
#define MDBG_ERR_MALLOC_FAIL 2
#define MDBG_ERR_BAD_PARAM	3
#define MDBG_ERR_SDIO_ERR	4
#define MDBG_ERR_TIMEOUT	5
#define MDBG_ERR_NO_FILE	6

#endif
