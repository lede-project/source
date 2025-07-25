/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _PCIE_DBG_H
#define _PCIE_DBGE_H

#include <linux/kernel.h>

#define PCIE_HEADER		"WCN_PCIE: "
#define PCIE_HEADER_ERR		"WCN_PCIE_ERR: "

#define PCIE_INFO(fmt, args...) \
	pr_info(PCIE_HEADER fmt, ## args)

#define PCIE_ERR(fmt, args...) \
		pr_err(PCIE_HEADER_ERR fmt,  ## args)

int pcie_hexdump(char *name, char *buf, int len);

#endif
