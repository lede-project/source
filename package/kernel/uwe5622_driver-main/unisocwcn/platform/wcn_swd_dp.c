/*
 * Copyright (C) 2019 Spreadtrum Communications Inc.
 *
 * Authors	: xiaodong.bi
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "wcn_swd_dp.h"

#ifdef CONFIG_WCN_PCIE
static void swd_ext_sel(bool enable)
{
	u32 *reg = (u32 *)(pcie_bar_vmem(4) + 0x1c);
	u32 *ahb_ctl = (u32 *)(pcie_bar_vmem(0) + 0x001303e8);/* dis sec */

	*ahb_ctl = 0x73e;
	if (enable)
		*reg |= (BIT(7) | BIT(6) | BIT(5));
	else
		*reg &= ~(BIT(7) | BIT(6) | BIT(5));
}

static void swclk_clr(void)
{
	u32 *reg = (u32 *)(pcie_bar_vmem(4) + 0x1c);

	*reg &= ~BIT(6);
}

static void swclk_set(void)
{
	u32 *reg = (u32 *)(pcie_bar_vmem(4) + 0x1c);

	*reg |= BIT(6);
}

static void sw_dio_out(u32 value)
{
	u32 *reg = (u32 *)(pcie_bar_vmem(4) + 0x1c);

	if (value & BIT(0))
		*reg |= BIT(5);
	else
		*reg &= ~BIT(5);
}

static u32 sw_dio_in(void)
{
	u32 *reg = (u32 *)(pcie_bar_vmem(4) + 0xc);
	u32 bit;

	bit = (*reg & BIT(7)) >> 7;

	return bit;
}
#elif defined CONFIG_WCN_USB
static void swd_ext_sel(bool enable)
{
}

static void swclk_clr(void)
{
}

static void swclk_set(void)
{
}

static void sw_dio_out(u32 value)
{
}

static u32 sw_dio_in(void)
{
	return 0;
}
#else
static void swd_ext_sel(bool enable)
{
	unsigned char reg;

	sdiohal_aon_readb(DAP_ADDR+0x0E, &reg);
	pr_info("reg value is:0x%x\n", reg);

	if (enable)
		reg |= (BIT(7) | BIT(6) | BIT(5));
	else
		reg &= ~(BIT(7) | BIT(6) | BIT(5));
	sdiohal_aon_writeb(DAP_ADDR+0x0E, reg);
	sdiohal_aon_readb(DAP_ADDR+0x0E, &reg);
	pr_info("reg value is:0x%x\n", reg);
}

static void swclk_clr(void)
{
	unsigned char reg;

	sdiohal_aon_readb(DAP_ADDR+0x0e, &reg);
	reg &= ~BIT(6);
	sdiohal_aon_writeb(DAP_ADDR+0x0E, reg);
}

static void swclk_set(void)
{
	unsigned char reg;

	sdiohal_aon_readb(DAP_ADDR+0x0E, &reg);
	reg |= BIT(6);
	sdiohal_aon_writeb(DAP_ADDR+0x0E, reg);
}

static void sw_dio_out(u32 value)
{
	unsigned char reg;

	sdiohal_aon_readb(DAP_ADDR+0x0E, &reg);
	if (value & BIT(0))
		reg |= BIT(5);
	else
		reg &= ~BIT(5);
	sdiohal_aon_writeb(DAP_ADDR+0x0E, reg);
}

static u32 sw_dio_in(void)
{
	unsigned char reg;
	u32 bit;

	sdiohal_aon_readb(DAP_ACK_ADDR, &reg);
	bit = (reg & BIT(7)) >> 7;

	return bit;
}
#endif

static void swd_clk_cycle(void)
{
	swclk_set();
	ndelay(100);
	swclk_clr();
	ndelay(100);
}

static void swd_write_bit(u32 bit)
{
	swclk_set();
	sw_dio_out(bit);
	ndelay(100);
	swclk_clr();
	ndelay(100);
}

static u32 swd_read_bit(void)
{
	u32 bit;

	swclk_set();
	ndelay(100);
	bit = sw_dio_in();
	swclk_clr();
	ndelay(100);

	return bit;
}

static void swd_insert_cycles(u32 n)
{
	u32 i;

	for (i = 0; i < n; i++)
		swd_clk_cycle();
}

u32 swd_transfer(u8 cmd, u32 *data)
{
	u32 ack = 0;
	u32 bit;
	u32 val;
	u32 parity;
	u32 n;

	parity = 0;
	/* Start Bit */
	swd_write_bit(1);
	bit = ((cmd >> 0) & BIT(0));
	/* APnDP Bit */
	swd_write_bit(bit);
	parity += bit;
	bit = ((cmd >> 1) & BIT(0));
	/* RnW Bit */
	swd_write_bit(bit);
	parity += bit;
	bit = ((cmd >> 2) & BIT(0));
	/* A2 Bit */
	swd_write_bit(bit);
	parity += bit;
	bit = ((cmd >> 3) & BIT(0));
	/* A3 Bit */
	swd_write_bit(bit);
	parity += bit;
	/* Parity Bit */
	swd_write_bit(parity & BIT(0));
	/* Stop Bit */
	swd_write_bit(0);
	/* Park Bit */
	swd_write_bit(1);

	/* Turnaround */
	swd_insert_cycles(1);

	/* Acknowledge response */
	bit = swd_read_bit();
	ack |= bit << 0;
	bit = swd_read_bit();
	ack |= bit << 1;
	bit = swd_read_bit();
	ack |= bit << 2;

	if (ack == DAP_TRANSFER_OK) {
		/* Data transfer */
		if (cmd & DAP_TRANSFER_RnW) {
			val = 0;
			parity = 0;
			/* Read DATA[0:31] */
			for (n = 0; n < 32; n++) {
				bit = swd_read_bit();
				parity += bit;
				val |= (bit << n);
			}

			/* Read Parity */
			bit = swd_read_bit();
			if ((parity ^ bit) & 1)
				ack = DAP_TRANSFER_ERROR;

			if (data)
				*data = val;

			/* Turnaround */
			swd_insert_cycles(1);
		} else {
			/* Turnaround */
			swd_insert_cycles(1);
			sw_dio_out(0);

			val = *data;
			parity = 0;
			/* Write WDATA[0:31] */
			for (n = 0; n < 32; n++) {
				bit = val & BIT(0);
				swd_write_bit(bit);
				parity += bit;
				val >>= 1;
			}
			/* Write Parity Bit */
			swd_write_bit(parity & BIT(0));
			/* Turnaround */
			swd_insert_cycles(1);
		}

		/* Idle cycles >= 8 */
		sw_dio_out(0);
		swd_insert_cycles(8);

		return ack;
	}

	if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
		if ((cmd & DAP_TRANSFER_RnW) != 0)
			swd_insert_cycles(32+1);
		else {
			/* Turnaround */
			swd_insert_cycles(1);
			if ((cmd & DAP_TRANSFER_RnW) == 0)
				swd_insert_cycles(32 + 1);
		}

		/* Idle cycles >= 8 */
		sw_dio_out(0);
		swd_insert_cycles(8);

		return ack;
	}

	/* Turnaround */
	swd_insert_cycles(1);
	swd_insert_cycles(32+1);

	/* Idle cycles >= 8 */
	sw_dio_out(0);
	swd_insert_cycles(8);

	return ack;
}

static void swd_send_nbytes(u8 *buf, int nbytes)
{
	u8 i, j;
	u8 dat;

	for (i = 0; i < nbytes; i++) {
		dat = buf[i];
		for (j = 0; j < 8; j++) {
			if ((dat & 0x80) == 0x80)
				swd_write_bit(1);
			else
				swd_write_bit(0);
			dat <<= 1;
		}
	}
}


static u32 swd_dap_read(u8 reg, u32 *data)
{
	u32 ack;

	reg &= ~DAP_TRANSFER_APnDP;
	reg |= DAP_TRANSFER_RnW;
	ack = swd_transfer(reg, data);

	return ack;
}

static u32 swd_dap_write(u8 reg, u32 *data)
{
	u32 ack;

	reg &= ~(DAP_TRANSFER_APnDP | DAP_TRANSFER_RnW);
	ack = swd_transfer(reg, data);

	return ack;
}

static u32 swd_ap_read(u8 reg, u32 *data)
{
	u32 ack;

	reg |= DAP_TRANSFER_APnDP | DAP_TRANSFER_RnW;
	ack = swd_transfer(reg, data);

	return ack;
}

static u32 swd_ap_write(u32 reg, u32 *data)
{
	u32 ack;

	reg &= ~(DAP_TRANSFER_RnW);
	reg |= DAP_TRANSFER_APnDP;
	ack = swd_transfer(reg, data);

	return ack;
}

static u32 swd_memap_read(u32 addr, u32 *data)
{
	u8 reg;
	u32 ack;

	reg = DAP_TRANSFER_A2;
	ack = swd_ap_write(reg, &addr);

	reg = DAP_TRANSFER_A2 | DAP_TRANSFER_A3;
	ack = swd_ap_read(reg, data);
	ack = swd_ap_read(reg, data);

	return ack;
}

static u32 swd_memap_write(u32 addr, u32 *data)
{
	u8 reg;
	u32 ack;

	reg = DAP_TRANSFER_A2;
	ack = swd_ap_write(reg, &addr);

	reg = DAP_TRANSFER_A2 | DAP_TRANSFER_A3;
	ack = swd_ap_write(reg, data);

	return ack;
}

static void swd_read_arm_core(void)
{
	u32 value;
	u32 index;
	u32 addr;
	static const char *core_reg_name[19] = {
		"R0 ", "R1 ", "R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ", "R8 ",
		"R9 ", "R10", "R11", "R12", "R13", "R14", "R15", "PSR", "MSP",
		"PSP",
	};

/* reg arm reg */
	for (index = 0; index < 19; index++) {
		addr = 0xe000edf4;
		swd_memap_write(addr, &index);
		addr = 0xe000edf8;
		swd_memap_read(addr, &value);
		pr_info("%s %s:0x%x\n", __func__, core_reg_name[index], value);
	}
}

/* MSB first */
u8 swd_wakeup_seq[16] = {
	0x49, 0xCF, 0x90, 0x46,
	0xA9, 0xB4, 0xA1, 0x61,
	0x97, 0xF5, 0xBB, 0xC7,
	0x45, 0x70, 0x3D, 0x98,
};

/* LSB first */
u8 swd_wakeup_seq2[16] = {
	0x19, 0xBC, 0x0E, 0xA2,
	0xE3, 0xDD, 0xAF, 0xE9,
	0x86, 0x85, 0x2D, 0x95,
	0x62, 0x09, 0xF3, 0x92,
};

u8 swd_to_ds_seq[2] = {
	0x3d, 0xc7,
};

static void swd_dormant_to_wake(void)
{
	u8  data;

	/* Send at least eight SWCLKTCK cycles with SWDIOTMS HIGH */
	sw_dio_out(1);
	swd_insert_cycles(8);

	/* 128 bit Selection Alert sequence */
	swd_send_nbytes(swd_wakeup_seq, 16);

	/* four SWCLKTCK cycles with SWDIOTMS LOW */
	sw_dio_out(0);
	swd_insert_cycles(4);

	/* Send the activation code */
	data = 0x58;
	swd_send_nbytes(&data, 1);

	swd_insert_cycles(1);
	sw_dio_out(0);
}

static void swd_wake_to_dormant(void)
{

	/* Send at least eight SWCLKTCK cycles with SWDIOTMS HIGH */
	sw_dio_out(1);
	swd_insert_cycles(8);

	/* 16-bit SWD-to-DS select sequence */
	swd_send_nbytes(swd_to_ds_seq, 2);
	swd_insert_cycles(1);
}

static void switch_jtag_to_swd(void)
{

	u8 data[2];

	/* Send at least 50 SWCLKTCK cycles with SWDIOTMS HIGH */
	sw_dio_out(1);
	swd_insert_cycles(50);

	/* send the 16-bit JTAG-to-SWD select sequence */
	data[0] = 0x79;
	data[1] = 0xe7;
	swd_send_nbytes(data, 2);

	/* Send at least 50 SWCLKTCK cycles with SWDIOTMS HIGH */
	swd_insert_cycles(50);
	sw_dio_out(0);
}

static void swd_line_reset(void)
{
	/* Complete SWD reset sequence
	 *(50 cycles high followed by 2 or more idle cycles)
	 */
	sw_dio_out(1);
	swd_insert_cycles(50);
	sw_dio_out(0);
	swd_insert_cycles(2);
}

static void swd_read_dpidr(void)
{
	u32 ack, data = 0;

	/* the dp idr is 0x0be12477*/
	ack = swd_dap_read(DP_IDCODE, &data);

	pr_info("%s idcode:0x%x\n", __func__, data);
}

static void swd_read_apidr(void)
{
	u32 ack, data = 0;

	data = (0xf << 4);
	ack = swd_dap_write(DP_SELECT, &data);

	/* the dp idr is 0x14770015*/
	ack = swd_ap_read(AP_IDCODE, &data);
	ack = swd_ap_read(AP_IDCODE, &data);
	pr_info("%s idcode:0x%x\n", __func__, data);

	data = (0x0 << 4);
	ack = swd_dap_write(DP_SELECT, &data);
}


u32 swd_sel_target(u8 cmd, u32 *data)
{
	u32 bit, parity;
	u32 n, val;

	sw_dio_out(1);
	swd_insert_cycles(50);

	sw_dio_out(0);
	swd_insert_cycles(2);

	parity = 0;
	/* Start Bit */
	swd_write_bit(1);
	bit = ((cmd >> 0) & BIT(0));
	/* APnDP Bit */
	swd_write_bit(bit);
	parity += bit;
	bit = ((cmd >> 1) & BIT(0));
	/* RnW Bit */
	swd_write_bit(bit);
	parity += bit;
	bit = ((cmd >> 2) & BIT(0));
	/* A2 Bit */
	swd_write_bit(bit);
	parity += bit;
	bit = ((cmd >> 3) & BIT(0));
	/* A3 Bit */
	swd_write_bit(bit);
	parity += bit;
	/* Parity Bit */
	swd_write_bit(parity & BIT(0));
	/* Stop Bit */
	swd_write_bit(0);
	/* Park Bit */
	swd_write_bit(1);

	/* Turnaround */
	swd_insert_cycles(5);

	val = *data;
	parity = 0;
	/* Write WDATA[0:31] */
	for (n = 0; n < 32; n++) {
		bit = (val & BIT(0));
		swd_write_bit(bit);
		parity += bit;
		val >>= 1;
	}
	/* Write Parity Bit */
	swd_write_bit(parity & BIT(0));

	sw_dio_out(0);
	swd_insert_cycles(3);

	return 0;
}

static void btwf_sys_dap_sel(void)
{
	u32 data = 0;

	swd_line_reset();
	data = TARGETSEL_CP;
	swd_sel_target(DP_TARGETSEL, &data);
	swd_read_dpidr();
}

static int swd_power_up(void)
{
	u32 data;

	pr_info("%s entry\n", __func__);

	data = 0x50000000;
	swd_dap_write(DP_CTRL_STAT, &data);
	swd_dap_read(DP_CTRL_STAT, &data);

	pr_info("%s read ctrl stat:0x%x\n", __func__, data);

	return 0;
}

static void swd_device_en(void)
{
	u32 data = 0;

	swd_ap_read(AP_CTRL, &data);
	swd_ap_read(AP_CTRL, &data);
	data = (data & 0xffffff88) |  BIT(1) | BIT(6);
	swd_ap_write(AP_STAT, &data);
}

/* Debug Exception and Monitor Control Register */
/* (0xe000edfC) = 0x010007f1 */
void swd_set_debug_mode(void)
{
	int ret;
	int addr;
	unsigned int reg_val;

	reg_val = 0x010007f1;
	addr = 0xe000edfC;
	ret = swd_memap_write(addr, &reg_val);
	if (ret < 0) {
		pr_info("%s  error:%d\n", __func__, ret);
		return;
	}

/*test */
	ret = swd_memap_read(addr, &reg_val);
	if (ret < 0) {
		pr_info("%s  error:%d\n", __func__, ret);
		return;
	}
	pr_info("%s arm debug reg value is 0x%x:\n", __func__, reg_val);
}

/*
 * Debug Halting Control status Register
 * (0xe000edf0) = 0xa05f0003
 */
void swd_hold_btwf_core(void)
{
	int ret;
	int addr;
	unsigned int reg_val;

	addr = 0xe000edf0;
	reg_val = 0xa05f0003;

	ret = swd_memap_write(addr, &reg_val);
	if (ret < 0) {
		pr_info("%s  error:%d\n", __func__, ret);
		return;
	}
/*test */
	ret = swd_memap_read(addr, &reg_val);
	if (ret < 0) {
		pr_info("%s  error:%d\n", __func__, ret);
		return;
	}
	pr_info("%s arm hold btwf reg value is 0x%x:\n", __func__, reg_val);
}

int swd_dump_arm_reg(void)
{
	pr_info("%s entry\n", __func__);

	swd_ext_sel(true);
	swd_line_reset();

	switch_jtag_to_swd();
	swd_dormant_to_wake();
	btwf_sys_dap_sel();

	swd_power_up();
	swd_read_apidr();
	swd_device_en();
	swd_hold_btwf_core();
	swd_set_debug_mode();
	swd_read_arm_core();

	/* release swd */
	swd_wake_to_dormant();
	swd_ext_sel(false);

	pr_info("%s end\n", __func__);

	return 0;
}
