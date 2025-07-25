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

#ifdef CONFIG_WCN_PCIE
#include "pcie.h"
#else
#include "./../sdio/sdiohal.h"
#endif
#include "../include/wcn_glb_reg.h"

/* DAP Transfer Request */
#define DAP_TRANSFER_APnDP              (1<<0)  /* AP : 1 DP: 0 */
#define DAP_TRANSFER_RnW                (1<<1)	/* R:1 W:0 */
#define DAP_TRANSFER_A2                 (1<<2)
#define DAP_TRANSFER_A3                 (1<<3)
#define DAP_TRANSFER_MATCH_VALUE        (1<<4)
#define DAP_TRANSFER_MATCH_MASK         (1<<5)


/* DAP Transfer Response */
#define DAP_TRANSFER_OK                 (1<<0)
#define DAP_TRANSFER_WAIT               (1<<1)
#define DAP_TRANSFER_FAULT              (1<<2)
#define DAP_TRANSFER_ERROR              (1<<3)
#define DAP_TRANSFER_MISMATCH           (1<<4)


/* Debug Port Register Addresses */
#define DP_IDCODE	0x00 /* IDCODE Register (SW Read only) */
#define DP_ABORT	0x00 /* Abort Register (SW Write only) */
#define DP_CTRL_STAT	0x04 /* Control & Status */
#define DP_WCR		0x04 /* Wire Control Register (SW Only) */
#define DP_SELECT	0x08 /* Select Register (JTAG R/W & SW W) */
#define DP_RESEND	0x08 /* Resend (SW Read Only) */
#define DP_RDBUFF	0x0C /* Read Buffer (Read Only) */
#define DP_TARGETSEL	0x0C /* Targetsel (Read Only) */

#define AP_CTRL		0x00
#define AP_STAT		0x00
#define AP_IDCODE       0x0c /* APIDR 0xFC, bank:0xc */

#define TARGETSEL_AON	0X22000001
#define TARGETSEL_AP	0X12000001
#define TARGETSEL_CP	0X02000001
#define DAP_ADDR	0x1A0
#define DAP_ACK_ADDR	(0x140 + 0x0F)

void hold_btwf_core(void);
void set_debug_mode(void);
int swd_dump_arm_reg(void);
