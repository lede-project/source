/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Filename : marlin_rfkill.h
 * Abstract : This file is a implementation for itm sipc command/event function
 *
 * Authors	: yufeng.yang
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
#ifndef __MARLIN_RFKILL_H__
#define __MARLIN_RFKILL_H__

int rfkill_bluetooth_init(struct platform_device *pdev);
int rfkill_bluetooth_remove(struct platform_device *pdev);

#endif
