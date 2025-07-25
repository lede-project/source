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
#include <asm/byteorder.h>
#include <marlin_platform.h>
#include <wcn_bus.h>

#include "bufring.h"
#include "rdc_debug.h"
#include "wcn_txrx.h"
#include "wcn_log.h"
#include "wcn_misc.h"
#include "wcn_glb_reg.h"
#ifdef CONFIG_WCN_USB
#include "./usb_boot.h"
#endif

#if defined CONFIG_UWE5623 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_WCN_SWD
#include "wcn_swd_dp.h"
#endif
#endif

static int smp_calc_chsum(unsigned short *buf, unsigned int size)
{
	unsigned long int cksum = 0;
	unsigned short data;

	while (size > 1) {
		data = *buf;
		buf++;
		cksum += data;
		size -= sizeof(unsigned short);
	}

	if (size)
		cksum += *buf & 0xff;

	while (cksum >> 16)
		cksum = (cksum >> 16) + (cksum & 0xffff);

	return (unsigned short)(~cksum);
}

static int mdbg_write_smp_head(unsigned int len)
{
	struct smp_head *smp;
	unsigned char *smp_buf, *tmp;
	int smp_len;

	smp_len = sizeof(struct smp_head) + sizeof(struct sme_head_tag);
	smp_buf = kmalloc(smp_len, GFP_KERNEL);
	if (!smp_buf)
		return -ENOMEM;

	/* Smp header */
	smp = (struct smp_head *)smp_buf;
	smp->sync_code = SMP_HEADERFLAG;
	smp->length = smp_len + len - SYSNC_CODE_LEN;
	smp->channel_num = SMP_DSP_CHANNEL_NUM;
	smp->packet_type = SMP_DSP_TYPE;
	smp->reserved = SMP_RESERVEDFLAG;
	smp->check_sum = smp_calc_chsum(&smp->length, sizeof(struct smp_head)
		- SYSNC_CODE_LEN - CHKSUM_LEN);

	/*
	 * Diag header: Needs use these bytes for ARM log tool,
	 * And it need't 0x7e head and without 0x7e tail
	 */
	tmp = smp_buf + sizeof(struct smp_head);
	((struct sme_head_tag *)tmp)->seq_num = 0;
	((struct sme_head_tag *)tmp)->len = smp_len
		+ len - sizeof(struct smp_head);
	((struct sme_head_tag *)tmp)->type = SMP_DSP_TYPE;
	((struct sme_head_tag *)tmp)->subtype = SMP_DSP_DUMP_TYPE;

#ifdef CONFIG_WCND
	mdbg_ring_write(mdbg_dev->ring_dev->ring, smp_buf, smp_len);
#else
	dumpmem_rx_callback(smp_buf, smp_len);
#endif

	kfree(smp_buf);

	return 0;
}

static int mdbg_dump_data(unsigned int start_addr,
			  char *str, int len, int str_len)
{
	unsigned char *buf, *temp_buf;
	int count, trans_size, err = 0, i, prin_temp = 2;
	int temp_len;

	if (unlikely(!mdbg_dev->ring_dev)) {
		WCN_ERR("mdbg_dump ring_dev is NULL\n");
		return -1;
	}
	str = NULL;
	if (str) {
		/* msleep(20); */
		WCN_INFO("mdbg str_len:%d\n", str_len);
		if (mdbg_dev->ring_dev->flag_smp == 1)
			mdbg_write_smp_head(str_len);

		if ((mdbg_ring_free_space(mdbg_dev->ring_dev->ring) - 1)
			 < str_len) {
			wake_up_log_wait();
			temp_len
			= mdbg_ring_free_space(mdbg_dev->ring_dev->ring)
						- 1;
			if (temp_len > 0) {
#ifdef CONFIG_WCND
				mdbg_ring_write(mdbg_dev->ring_dev->ring,
						str, temp_len);
#else
				dumpmem_rx_callback(str, temp_len);
#endif
			}
			if (temp_len < 0) {
				WCN_ERR("ringbuf str error\n");
				return 0;
			}
			str += temp_len;
			str_len -= temp_len;
			wake_up_log_wait();
		}

		while ((mdbg_ring_free_space(mdbg_dev->ring_dev->ring)
			- 1 == 0) && (mdbg_dev->open_count != 0)) {
			WCN_ERR("no space to write mem, sleep...\n");
			wake_up_log_wait();
			 msleep(20);
		}

#ifdef CONFIG_WCND
		mdbg_ring_write(mdbg_dev->ring_dev->ring, str, str_len);
#else
		dumpmem_rx_callback(str, str_len);
#endif
		wake_up_log_wait();
	}

	if (len == 0)
		return 0;

	buf = kmalloc(DUMP_PACKET_SIZE, GFP_KERNEL);
	temp_buf = buf;
	if (!buf)
		return -ENOMEM;

	count = 0;
	while (count < len) {
		trans_size = (len - count) > DUMP_PACKET_SIZE ?
			DUMP_PACKET_SIZE : (len - count);
		temp_buf = buf;
#ifdef CONFIG_WCN_USB
		err = marlin_dump_read_usb(start_addr + count, buf, trans_size);
#else
		err = sprdwcn_bus_direct_read(start_addr + count, buf,
					      trans_size);
#endif
		if (err < 0) {
			WCN_ERR("%s dump memory error:%d\n", __func__, err);
			goto out;
		}
		if (prin_temp == 0) {
			prin_temp = 1;
			for (i = 0; i < 5; i++)
				WCN_ERR("mdbg *****buf[%d]:0x%x\n",
				       i, buf[i]);
		}
		if (mdbg_dev->ring_dev->flag_smp == 1)
			mdbg_write_smp_head(trans_size);

		temp_len
			= mdbg_ring_free_space(mdbg_dev->ring_dev->ring)
				- 1;
		if (temp_len < trans_size) {
			wake_up_log_wait();

			if (temp_len > 0) {
#ifdef CONFIG_WCND
				mdbg_ring_write(mdbg_dev->ring_dev->ring,
						temp_buf, temp_len);
#else
				dumpmem_rx_callback(temp_buf, temp_len);
#endif
			}
			if (temp_len < 0) {
				WCN_ERR("ringbuf data error\n");
				return 0;
			}
				temp_buf += temp_len;
				trans_size -= temp_len;
				count += temp_len;
				wake_up_log_wait();
		}
		while ((mdbg_ring_free_space(mdbg_dev->ring_dev->ring) - 1 == 0)
			&& (mdbg_dev->open_count != 0)) {
			WCN_ERR("no space buf to write mem, sleep...\n");
			wake_up_log_wait();
		    msleep(20);
		}

#ifdef CONFIG_WCND
		mdbg_ring_write(mdbg_dev->ring_dev->ring, temp_buf, trans_size);
#else
		dumpmem_rx_callback(temp_buf, trans_size);
#endif
		count += trans_size;
		wake_up_log_wait();
	}

out:
	kfree(buf);

	return count;
}

void mdbg_clear_log(void)
{
	if (mdbg_dev->ring_dev->ring->rp
		!= mdbg_dev->ring_dev->ring->wp) {
		WCN_INFO("log:%ld left in ringbuf not read\n",
			 (long int)(mdbg_dev->ring_dev->ring->wp -
			 mdbg_dev->ring_dev->ring->rp));
		mdbg_ring_clear(mdbg_dev->ring_dev->ring);
	}
}

struct wcn_dump_mem_reg {
	/* some CP regs can't dump */
	bool do_dump;
	u32 addr;
	/* 4 btyes align */
	u32 len;
};

#define WCN_DUMP_END_STRING "marlin_memdump_finish"
/* magic number, not change it */
#define WCN_DUMP_VERSION_NAME "WCN_DUMP_HEAD__"
/* SUB_NAME len not more than 15 bytes */
#define WCN_DUMP_VERSION_SUB_NAME "SDIO_23xx"
/* CP2 iram start and end */
#define WCN_DUMP_CP2_IRAM_START 1
#define WCN_DUMP_CP2_IRAM_END 2
/* AP regs start and end */
#define WCN_DUMP_AP_REGS_START (WCN_DUMP_CP2_IRAM_END + 1)
#define WCN_DUMP_AP_REGS_END 9
/* CP2 regs start and end */
#define WCN_DUMP_CP2_REGS_START (WCN_DUMP_AP_REGS_END + 1)
#define WCN_DUMP_CP2_REGS_END (wcn_dump_array_size - 1)

#define WCN_DUMP_ALIGN(x) (((x) + 3) & ~3)
/* used for HEAD, so all dump mem in this array.
 * if new member added, please modify the macor XXX_START XXX_end above.
 */

static __attribute__((unused))
struct wcn_dump_mem_reg s_wcn_dump_regs_5621[] = {
	/* IRAM + DRAM */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x100000, FIRMWARE_MAX_SIZE_M3_M3E}, /*FIRMWARE_MAX_SIZE*/
#else
	{1, 0x100000, FIRMWARE_MAX_SIZE}, /*FIRMWARE_MAX_SIZE*/
#endif
	/* top */
	{1, 0x40880000, 0x54}, /* AON_AHB */
	{1, 0x4083C000, 0x354}, /* AON_APB */
	{1, 0x40130000, 0x400}, /* BTWF_AHB */
	{1, 0x40088000, 0x28c}, /* BTWF_APB */
	{1, 0x40844200, 0x144}, /* AON_CLK */
	{1, 0x40844000, 0x48}, /* PRE_DIV_CLK */
	/* SDIO regs */
	{1, 0x40140000, 0x10000}, /* SDIO regs */
	/* WIFI regs */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x400f0000, WIFI_AON_MAC_SIZE_M3_M3E}, /* WIFI_AON_MAC */
#else
	{1, 0x400f0000, WIFI_AON_MAC_SIZE}, /* WIFI_AON_MAC */
#endif
	{1, 0x400f1000, 0xD100}, /* WIFI_RTN_PD_MAC */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x40300000, WIFI_RAM_SIZE_M3_M3E}, /* WIFI_352K/298K_RAM */
	{1, 0x400a0000, WIFI_GLB_REG_SIZE_M3_M3E}, /* Wifi_glb_reg */
#else
	{1, 0x40300000, WIFI_RAM_SIZE}, /* WIFI_352K/298K_RAM */
	{1, 0x400a0000, WIFI_GLB_REG_SIZE}, /* Wifi_glb_reg */
#endif
	{1, 0x400b0000, 0x388}, /* Wifi_phy_top_reg */
	{1, 0x400b1000, 0x154}, /* Wifi_phy_tx11a_reg */
	{1, 0x400b2000, 0xa8c}, /* Wifi_phy_rx11a_reg */
	{1, 0x400b3000, 0xb0}, /* Wifi_phy_11b_reg */
	{1, 0x400b4000, 0xa70}, /* Wifi_rfif_reg */
	{1, 0x400b7000, 0x618}, /* Wifi_dfe_reg */
#ifdef CONFIG_WCN_FM
	/* FM regs */
	{1, 0x40098000, 0xabc}, /* fm + rds */
#endif
	/* Bluetooth (HW DEC and BB) Buffer regs */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x40240000, BT_ACC_SIZE_M3_M3E}, /* BT_ACC */
#else
	{1, 0x40240000, BT_ACC_SIZE}, /* BT_ACC */
#endif
	{1, 0x40246000, 0x738}, /* BT_JAL */
	{1, 0x40248000, 0xA0}, /* BT_HAB */
	{1, 0x4024A000, 0x21C},  /* BT_LEJAL */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x4024F000, BT_MODEM_SIZE_M3_M3E}, /* BT_MODEM */
#else
	{1, 0x4024F000, BT_MODEM_SIZE}, /* BT_MODEM */
#endif
	{1, 0x40200000, 0x200}, /* BT_CMD_BUF */
	{1, 0x40204000, 0x200}, /* BT_EVENT_BUF */
	{1, 0x40208000, 0x12A4},  /* BT_LMP_TX_BUF */
	{1, 0x40200C00, 0xB744},  /* BT_LMP_RX_BUF */
	{1, 0x40210000, 0x3000},  /* BT_ACL_TX_BUF */
	{1, 0x40214000, 0x3000},  /* BT_ACL_RX_BUF */
	{1, 0x40218000, 0x2D0},  /* BT_SCO_TX_BUF */
	{1, 0x4021C000, 0x5C0},  /* BT_SCO_RX_BUF */
	{1, 0x40241000, 0x400},  /* BT_BB_TX_BUF */
	{1, 0x40242000, 0x400}  /* BT_BB_RX_BUF */
};

static __attribute__((unused))
struct wcn_dump_mem_reg s_wcn_dump_regs_5622[] = {
	/* IRAM + DRAM */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x100000, FIRMWARE_MAX_SIZE_M3L}, /*FIRMWARE_MAX_SIZE*/
#else
	{1, 0x100000, FIRMWARE_MAX_SIZE}, /*FIRMWARE_MAX_SIZE*/
#endif
	/* top */
	{1, 0x40880000, 0x54}, /* AON_AHB */
	{1, 0x4083C000, 0x354}, /* AON_APB */
	{1, 0x40130000, 0x400}, /* BTWF_AHB */
	{1, 0x40088000, 0x28c}, /* BTWF_APB */
	{1, 0x40844200, 0x144}, /* AON_CLK */
	{1, 0x40844000, 0x48}, /* PRE_DIV_CLK */
	/* SDIO regs */
	{1, 0x40140000, 0x10000}, /* SDIO regs */
	/* WIFI regs */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x400f0000, WIFI_AON_MAC_SIZE_M3L}, /* WIFI_AON_MAC */
#else
	{1, 0x400f0000, WIFI_AON_MAC_SIZE}, /* WIFI_AON_MAC */
#endif
	{1, 0x400f1000, 0xD100}, /* WIFI_RTN_PD_MAC */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x40300000, WIFI_RAM_SIZE_M3L}, /* WIFI_352K/298K_RAM */
	{1, 0x400a0000, WIFI_GLB_REG_SIZE_M3L}, /* Wifi_glb_reg */
#else
	{1, 0x40300000, WIFI_RAM_SIZE}, /* WIFI_352K/298K_RAM */
	{1, 0x400a0000, WIFI_GLB_REG_SIZE}, /* Wifi_glb_reg */
#endif
	{1, 0x400b0000, 0x388}, /* Wifi_phy_top_reg */
	{1, 0x400b1000, 0x154}, /* Wifi_phy_tx11a_reg */
	{1, 0x400b2000, 0xa8c}, /* Wifi_phy_rx11a_reg */
	{1, 0x400b3000, 0xb0}, /* Wifi_phy_11b_reg */
	{1, 0x400b4000, 0xa70}, /* Wifi_rfif_reg */
	{1, 0x400b7000, 0x618}, /* Wifi_dfe_reg */
#ifdef CONFIG_WCN_FM
	/* FM regs */
	{1, 0x40098000, 0xabc}, /* fm + rds */
#endif
	/* Bluetooth (HW DEC and BB) Buffer regs */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x40240000, BT_ACC_SIZE_M3L}, /* BT_ACC */
#else
	{1, 0x40240000, BT_ACC_SIZE}, /* BT_ACC */
#endif
	{1, 0x40246000, 0x738}, /* BT_JAL */
	{1, 0x40248000, 0xA0}, /* BT_HAB */
	{1, 0x4024A000, 0x21C},  /* BT_LEJAL */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	{1, 0x4024F000, BT_MODEM_SIZE_M3L}, /* BT_MODEM */
#else
	{1, 0x4024F000, BT_MODEM_SIZE}, /* BT_MODEM */
#endif
	{1, 0x40200000, 0x200}, /* BT_CMD_BUF */
	{1, 0x40204000, 0x200}, /* BT_EVENT_BUF */
	{1, 0x40208000, 0x12A4},  /* BT_LMP_TX_BUF */
	{1, 0x40200C00, 0xB744},  /* BT_LMP_RX_BUF */
	{1, 0x40210000, 0x3000},  /* BT_ACL_TX_BUF */
	{1, 0x40214000, 0x3000},  /* BT_ACL_RX_BUF */
	{1, 0x40218000, 0x2D0},  /* BT_SCO_TX_BUF */
	{1, 0x4021C000, 0x5C0},  /* BT_SCO_RX_BUF */
	{1, 0x40241000, 0x400},  /* BT_BB_TX_BUF */
	{1, 0x40242000, 0x400}  /* BT_BB_RX_BUF */
};

#ifdef CONFIG_WCN_USB
#define RAM_SECTION_NUM 4
#endif
static __attribute__((unused))
struct wcn_dump_mem_reg s_wcn_dump_regs_5623[] = {
	/* IRAM + DRAM */
	{1, 0x40500000, 0x7ac00}, /* CP IRAM */
	{1, 0x40580000, 0x1a800}, /* CP DRAM */
	{1, 0x406A0000, 0x54000}, /* AON AHB RAM */
	{1, 0x40F00000, 0x70000}, /* AON AXI RAM */
	{1, 0x42000000, 0x8a800}, /* CP ROM */
	/* top */
	{1, 0x40930000, 0xE4},  /* AON_AHB regs */
	{1, 0x4082C000, 0x3B4}, /* AON_APB regs */
	{1, 0x40610000, 0x190}, /* AON_CP_APB regs */
	{1, 0x40828000, 0x200}, /* PMU_APB regs */
	{1, 0x40834220, 0x138}, /* AON_CLK_RF regs */
	{1, 0x40834000, 0x54}, /* AON_PRE_DIV_CLK regs */
	{1, 0x40130000, 0x29C}, /* BTWF_AHB regs */
#ifdef CONFIG_WCN_SDIO
	{1, 0x40970000, 0x10000}, /* SDIO regs */
#endif
	/* WIFI regs */
	{1, 0x400f0000, 0xFF00}, /* WIFI MAC regs */
	{1, 0x40300000, 0x28000}, /* WIFI MAC SHARE RAM */
	{1, 0x400B0000, 0x8000}, /* WIFI PHY */
	/* BT regs */
	{1, 0X40200000, 0X7FFFF}, /* BT CFG */
	{1, 0X40280000, 0X7FFFF}, /* BT_ACC */
};

static struct wcn_dump_mem_reg s_wcn_dump_regs[36];
static unsigned int wcn_dump_array_size;

struct wcn_dump_section_info {
	/* cp load start addr */
	__le32 start;
	/* cp load end addr */
	__le32 end;
	/* load from file offset */
	__le32 off;
	__le32 reserv;
} __packed;

struct wcn_dump_head_info {
	/* WCN_DUMP_VERSION_NAME */
	u8 version[16];
	/* WCN_DUMP_VERSION_SUB_NAME */
	u8 sub_version[16];
	/* numbers of wcn_dump_section_info */
	__le32 n_sec;
	/* used to check if dump is full */
	__le32 file_size;
	u8 reserv[8];
	struct wcn_dump_section_info section[0];
} __packed;

static int wcn_fill_dump_head_info(struct wcn_dump_mem_reg *mem_cfg, int cnt)
{
	int i, len, head_len;
	struct wcn_dump_mem_reg *mem;
	struct wcn_dump_head_info *head;
	struct wcn_dump_section_info *sec;

	head_len = sizeof(*head) + sizeof(*sec) * cnt;
	head = kzalloc(head_len, GFP_KERNEL);
	if (unlikely(!head)) {
		WCN_ERR("system has no mem for dump mem\n");
		return -1;
	}

	strncpy(head->version, WCN_DUMP_VERSION_NAME,
		strlen(WCN_DUMP_VERSION_NAME));
	strncpy(head->sub_version, WCN_DUMP_VERSION_SUB_NAME,
		strlen(WCN_DUMP_VERSION_SUB_NAME));
	head->n_sec = cpu_to_le32(cnt);
	len = head_len;
	for (i = 0; i < cnt; i++) {
		sec = head->section + i;
		mem = mem_cfg + i;
		sec->off = cpu_to_le32(WCN_DUMP_ALIGN(len));
		sec->start = cpu_to_le32(mem->addr);
		sec->end = cpu_to_le32(sec->start + mem->len - 1);
		len += mem->len;
		WCN_INFO("section[%d] [0x%x 0x%x 0x%x]\n",
			 i, le32_to_cpu(sec->start),
			 le32_to_cpu(sec->end), le32_to_cpu(sec->off));
	}
	head->file_size = cpu_to_le32(len + strlen(WCN_DUMP_END_STRING));

#ifdef CONFIG_WCND
	mdbg_ring_write(mdbg_dev->ring_dev->ring, head, head_len);
#else
	dumpmem_rx_callback(head, head_len);
#endif
	wake_up_log_wait();
	kfree(head);

	return 0;
}

static void mdbg_dump_str(char *str, int str_len)
{
	if (!str)
		return;

#ifdef CONFIG_WCND
	mdbg_ring_write(mdbg_dev->ring_dev->ring, str, str_len);
#else
	dumpmem_rx_callback(str, str_len);
#endif
	wake_up_log_wait();
	WCN_INFO("dump str finish!");
}

/*
 * dump cp wifi phy reg, wifi phy start[11,17]
 */
static void wcn_dump_cp_register(struct wcn_dump_mem_reg *mem)
{
	int i;

	for (i = 11; i <= 17; i++) {
		mdbg_dump_data(mem[i].addr, NULL, mem[i].len, 0);
		WCN_INFO("dump cp reg section[%d] ok!\n", i);
	}
}

/* IF CONFIG_UWE5623 IS NOT DEFINE, THIS FUNCTION WILL NOT BE USED!
 * THIS IS A ERROR BY COMPLIE. SO IT NOT BE STATIC
 */
void wcn_dump_cp_data(struct wcn_dump_mem_reg *mem, int start, int end)
{
	int i;

	for (i = start; i <= end; i++) {
		mdbg_dump_data(mem[i].addr, NULL, mem[i].len, 0);
		WCN_INFO("dump cp reg section[%d] ok!\n", i);
	}
}

static int cp_dcache_clean_invalid_all(void)
{
	int ret;
	unsigned int reg_val = 0;

	/*
	 * 1.AP write DCACHE REG CMD by sdio dt mode
	 * 2.delay little time for dcache clean excuting and polling done raw
	 * 3.clear done raw
	 * 4.if sdio dt mode is breaked,
	 *   cp cpu reset and dcache REG is default.
	 *   cache_debug mode must be set normal mode.
	 *   cache_size set 32K
	 */
	marlin_hold_cpu();
	ret = sprdwcn_bus_reg_read(DCACHE_REG_ENABLE, &reg_val, 4);
	if (!(ret == 0)) {
		pr_info("Marlin3_Dcache REG sdiohal_dt_read error !\n");
		return ret;
	}
	if (!(reg_val & DCACHE_ENABLE_MASK)) {
		WCN_INFO("CP DCACHE DISENABLE\n");
		return ret;
	}
	WCN_INFO("CP DCACHE ENABLE\n");
	ret = sprdwcn_bus_reg_read(DCACHE_CFG0, &reg_val, 4);
	if (!(ret == 0)) {
		pr_info("Marlin3_Dcache REG sdiohal_dt_read error !\n");
		return ret;
	}
	if (reg_val & DCACHE_DEBUG_EN) {
		reg_val &= ~(DCACHE_DEBUG_EN);
		/* dcache set normal mode */
		ret = sprdwcn_bus_reg_write(DCACHE_CFG0, &reg_val, 4);
		if (!(ret == 0)) {
			pr_info("Marlin3_Dcache REG sdiohal_dt_write error !\n");
			return ret;
		}
	}
	ret = sprdwcn_bus_reg_read(DCACHE_CFG0, &reg_val, 4);
	if ((reg_val & DCACHE_SIZE_SEL_MASK) != DCACHE_SIZE_SEL_MASK) {
		reg_val |= ((DCACHE_SIZE_32K<<28)&DCACHE_SIZE_SEL_MASK);
		/* cache size set 32K */
		ret = sprdwcn_bus_reg_write(DCACHE_CFG0, &reg_val, 4);
	}
	reg_val = (
		(DCACHE_CMD_ISSUE_START | DCACHE_CMD_CLEAN_INVALID_ALL)&
		DCACHE_CMD_CFG2_MASK);
	ret = sprdwcn_bus_reg_write(DCACHE_CMD_CFG2, &reg_val, 4);
	/* cmd excuting */
	msleep(20);
	ret = sprdwcn_bus_reg_read(DCACHE_INT_RAW_STS, &reg_val, 4);
	/* read raw */
	if ((reg_val & 0X00000001) == 0) {
		pr_info("Marlin3_Dcache clear cost time not enough !\n");
		//return ret;
	}
	reg_val = (DCACHE_CMD_IRQ_CLR);
	/* clear raw */
	ret = sprdwcn_bus_reg_write(DCACHE_INT_CLR, &reg_val, 4);
	return ret;
}

/* select aon_apb_dap DAP(Debug Access Port) */
#if defined CONFIG_UWE5622 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
void dap_sel_btwf_lite(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(DAP_CTRL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read DJTAG_DAP_SEL error:%d\n", __func__, ret);
		WCN_INFO("dt fail,start reset pin!\n");
		ret = marlin_reset_reg();
		if (ret < 0) {
			WCN_ERR("dt fail,reset pin fail!\n");
			return;
		}
		ret = sprdwcn_bus_reg_read(DAP_CTRL, &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("after reset,dt read still fail!\n");
			return;
		}
	}
	WCN_INFO("%s DJTAG_DAP_SEL:0x%x\n", __func__, reg_val);

	reg_val |= CM4_DAP_SEL_BTWF_LITE;
	ret = sprdwcn_bus_reg_write(DAP_CTRL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write DJTAG_DAP_SEL error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s DJTAG_DAP_SEL:0x%x\n", __func__, reg_val);

	ret = sprdwcn_bus_reg_read(DAP_CTRL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read2 DJTAG_DAP_SEL error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s 2:DJTAG_DAP_SEL:0x%x\n", __func__, reg_val);
}

/* select aon_apb_dap DAP(Debug Access Port) */
void dap_sel_default_lite(void)
{
	int ret;
	unsigned int reg_val;

	reg_val = 0;
	ret = sprdwcn_bus_reg_write(DAP_CTRL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write DJTAG_DAP_SEL error:%d\n", __func__, ret);
		return;
	}
}

/* enable aon_apb_dap_en */
void apb_eb_lite(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(APB_ENB1, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read APB_EB error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s APB_EB:0x%x\n", __func__, reg_val);

	reg_val |= DBG_CM4_EB;
	ret = sprdwcn_bus_reg_write(APB_ENB1, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write APB_EB error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s APB_EB:0x%x\n", __func__, reg_val);

	ret = sprdwcn_bus_reg_read(APB_ENB1, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read2 APB_EB error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s 2:APB_EB:0x%x\n", __func__, reg_val);
}
#endif

/* select aon_apb_dap DAP(Debug Access Port) */
void dap_sel_btwf(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(DJTAG_DAP_SEL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read DJTAG_DAP_SEL error:%d\n", __func__, ret);
		WCN_INFO("dt fail,start reset pin!\n");
		ret = marlin_reset_reg();
		if (ret < 0) {
			WCN_ERR("dt fail,reset pin fail!\n");
			return;
		}
		ret = sprdwcn_bus_reg_read(DJTAG_DAP_SEL, &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("after reset,dt read still fail!\n");
			return;
		}
	}
	WCN_INFO("%s DJTAG_DAP_SEL:0x%x\n", __func__, reg_val);

	reg_val |= CM4_DAP_SEL_BTWF | CM4_DAP_SEL_GNSS;
	ret = sprdwcn_bus_reg_write(DJTAG_DAP_SEL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write DJTAG_DAP_SEL error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s DJTAG_DAP_SEL:0x%x\n", __func__, reg_val);

	ret = sprdwcn_bus_reg_read(DJTAG_DAP_SEL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read2 DJTAG_DAP_SEL error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s 2:DJTAG_DAP_SEL:0x%x\n", __func__, reg_val);
}

/* select aon_apb_dap DAP(Debug Access Port) */
void dap_sel_default(void)
{
	int ret;
	unsigned int reg_val;

	reg_val = 0;
	ret = sprdwcn_bus_reg_write(DJTAG_DAP_SEL, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write DJTAG_DAP_SEL error:%d\n", __func__, ret);
		return;
	}
}

/* disable aon_apb_dap_rst */
void apb_rst(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(APB_RST, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read APB_RST error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s APB_RST:0x%x\n", __func__, reg_val);

	reg_val &= ~CM4_DAP0_SOFT_RST & ~CM4_DAP1_SOFT_RST;
	ret = sprdwcn_bus_reg_write(APB_RST, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write APB_RST error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s APB_RST:0x%x\n", __func__, reg_val);

	ret = sprdwcn_bus_reg_read(APB_RST, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read2 APB_RST error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s 2:APB_RST:0x%x\n", __func__, reg_val);
}

/* enable aon_apb_dap_en */
void apb_eb(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(APB_EB, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read APB_EB error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s APB_EB:0x%x\n", __func__, reg_val);

	reg_val |= CM4_DAP0_EB | CM4_DAP1_EB;
	ret = sprdwcn_bus_reg_write(APB_EB, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s write APB_EB error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s APB_EB:0x%x\n", __func__, reg_val);

	ret = sprdwcn_bus_reg_read(APB_EB, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read2 APB_EB error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s 2:APB_EB:0x%x\n", __func__, reg_val);
}

void check_dap_is_ok(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(BTWF_STATUS_REG, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s read error:%d\n", __func__, ret);
		return;
	}
	WCN_INFO("%s :0x%x\n", __func__, reg_val);

	if (reg_val == BTWF_OK_VALUE)
		WCN_INFO("btwf dap is ready\n");
}

/*
 * Debug Halting Control status Register
 * (0xe000edf0) = 0xa05f0003
 */
void hold_btwf_core(void)
{
	int ret, i;
	unsigned int reg_val;
	unsigned int a[][2] = {
			{ARM_DAP_REG1, 0x22000012},
			{ARM_DAP_REG2, 0xe000edf0},
			{ARM_DAP_REG3, 0xa05f0003} }; /* 0xa05f0007 try */

	for (i = 0; i < 3; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}
}

/*
 * Debug Halting Control status Register
 * (0xe000edf0) = 0xa05f0003
 */
void release_btwf_core(void)
{
	int ret, i;
	unsigned int reg_val;
	unsigned int a[][2] = {
			{ARM_DAP_REG1, 0x22000012},
			{ARM_DAP_REG2, 0xe000edf0},
			{ARM_DAP_REG3, 0xa05f0000} }; /* 0xa05f is a key */

	for (i = 0; i < 3; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}
}

/* Debug Exception and Monitor Control Register */
void set_debug_mode(void)
{
	int ret, i;
	unsigned int reg_val;
	unsigned int a[][2] = {
			{ARM_DAP_REG1, 0x22000012},
			{ARM_DAP_REG2, 0xe000edfC},
			{ARM_DAP_REG3, 0x010007f1} };

	for (i = 0; i < 3; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}
}

/*
 * Debug core Register Selector Register
 * The index R0 is 0, R1 is 1
 */
void set_core_reg(unsigned int index)
{
	int ret, i;
	unsigned int reg_val;
	unsigned int a[][2] = {
			{ARM_DAP_REG1, 0x22000012},
			{ARM_DAP_REG2, 0xe000edf4},
			{ARM_DAP_REG3, index} };

	for (i = 0; i < 3; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}
}

/*
 * write_core_reg_value - write arm reg = value.
 * Example: write PC(R15)=0x12345678
 * reg_index = 15, value = 0x12345678
 */
void write_core_reg_value(unsigned int reg_index, unsigned int value)
{
	int ret, i;
	unsigned int reg_val;
	unsigned int a[][3] = {
			{ARM_DAP_REG1, 0x22000012, 0x22000012},
			{ARM_DAP_REG2, 0xe000edf8, 0xe000edf4},
			{ARM_DAP_REG3, value, 0x10000+reg_index} };

	for (i = 0; i < 3; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}

	for (i = 0; i < 2; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}

	sprdwcn_bus_reg_read(a[2][0], &reg_val, 4);
	WCN_INFO("%s value: 0x%x, reg_value:0x%x\n", __func__, value, reg_val);

	for (i = 0; i < 3; i++) {
		reg_val = a[i][2];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}
}

void sprdwcn_bus_armreg_write(unsigned int reg_index, unsigned int value)
{
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
	dap_sel_btwf_lite();
	apb_eb_lite();
#else
	dap_sel_btwf();
	apb_rst();
	apb_eb();
#endif
#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
		dap_sel_btwf_lite();
		apb_eb_lite();
	} else {
		dap_sel_btwf();
		apb_rst();
		apb_eb();
	}
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

	check_dap_is_ok();
	hold_btwf_core();
	set_debug_mode();
	write_core_reg_value(reg_index, value);

	/* make sure btwf core can run */
	release_btwf_core();
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifndef CONFIG_UWE5622
	/* make sure JTAG can connect dap */
	dap_sel_default();
#endif
#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() != WCN_CHIP_MARLIN3L)
		dap_sel_default();
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
}

/* Debug Core register Data Register */
void read_core_reg(unsigned int value, unsigned int *p)
{
	int ret, i;
	unsigned int reg_val;
	unsigned int a[][2] = {
			{ARM_DAP_REG1, 0x22000012},
			{ARM_DAP_REG2, 0xe000edf8},
			{ARM_DAP_REG3, 0x00000000} };

	for (i = 0; i < 2; i++) {
		reg_val = a[i][1];
		ret = sprdwcn_bus_reg_write(a[i][0], &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s  error:%d\n", __func__, ret);
			return;
		}
	}

	sprdwcn_bus_reg_read(a[2][0], &reg_val, 4);
	p[value] = reg_val;

	WCN_INFO("%s ****R[%d]: 0x%x****\n", __func__, value, reg_val);
}


int dump_arm_reg(void)
{
	unsigned int i;
	static const char *core_reg_name[19] = {
		"R0 ", "R1 ", "R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ", "R8 ",
		"R9 ", "R10", "R11", "R12", "R13", "R14", "R15", "PSR", "MSP",
		"PSP",
		};
	unsigned int *p;

	p = kzalloc(19 * 4, GFP_KERNEL);
	if (!p) {
		WCN_ERR("Can not allocate ARM REG Buffer\n");
		return -ENOMEM;
	}

	memset(p, 0, 19*4);
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5622
	dap_sel_btwf_lite();
	apb_eb_lite();
#else
	dap_sel_btwf();
	apb_rst();
	apb_eb();
#endif
#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L) {
		dap_sel_btwf_lite();
		apb_eb_lite();
	} else {
		dap_sel_btwf();
		apb_rst();
		apb_eb();
	}
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/

	check_dap_is_ok();
	hold_btwf_core();
	set_debug_mode();
	for (i = 0; i < 19; i++) {
		set_core_reg(i);
		read_core_reg(i, p);
	}
	WCN_INFO("------------[ ARM REG ]------------\n");
	for (i = 0; i < 19; i++)
		WCN_INFO("[%s] = \t0x%08x\n", core_reg_name[i], p[i]);

	WCN_INFO("------------[ ARM END ]------------\n");
	kfree(p);
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifndef CONFIG_UWE5622
	/* make sure JTAG can connect dap */
	dap_sel_default();
#endif
#else /*ELOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() != WCN_CHIP_MARLIN3L)
		dap_sel_default();
#endif /*EOF CONFIG_CHECK_DRIVER_BY_CHIPID*/
	return 0;
}

static int check_bt_buffer_RW(void)
{
	int ret = -1;
	unsigned int temp;

	ret = sprdwcn_bus_reg_read(HCI_ARM_WR_RD_MODE, &temp, 4);
	if (ret < 0) {
		WCN_ERR("read HCI_ARM_WR_RD_MODE reg error:%d\n", ret);
		return ret;
	}
	WCN_INFO("%s HCI_ARM_WR_RD_MODE reg val:0x%x\n", __func__, temp);

	temp = HCI_ARM_WR_RD_VALUE;
	ret = sprdwcn_bus_reg_write(HCI_ARM_WR_RD_MODE, &temp, 4);

	return ret;
}

static int enable_cp_pll(void)
{
	int ret;
	unsigned int temp;

	ret = sprdwcn_bus_reg_read(CLK_CTRL0, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s read CLK_CTRL0 reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s rd CLK_CTRL0 reg val:0x%x\n", __func__, temp);

	temp = temp | APLL_PDN;
	ret = sprdwcn_bus_reg_write(CLK_CTRL0, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s write CLK_CTRL0 reg error:%d\n", __func__, ret);
		return ret;
	}
	udelay(200);
	temp = temp | APLL_PDN | BPLL_PDN;
	WCN_INFO("%s enable CLK_CTRL0 val:0x%x\n", __func__, temp);
	ret = sprdwcn_bus_reg_write(CLK_CTRL0, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s write CLK_CTRL0 reg err:%d\n", __func__, ret);
		return ret;
	}
	udelay(200);

	return ret;
}

static int check_wifi_power_domain_ison(void)
{
	int ret;
	unsigned int temp;

	ret = enable_cp_pll();
	if (ret < 0) {
		WCN_ERR("wifi enable cp pll err\n");
		return ret;
	}

	ret = sprdwcn_bus_reg_read(CHIP_SLP, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s read CHIP_SLP reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s CHIP_SLP reg val:0x%x\n", __func__, temp);

	if ((temp & WIFI_ALL_PWRON) != WIFI_ALL_PWRON) {
		/* WIFI WRAP */
		if ((temp & WIFI_WRAP_PWRON) != WIFI_WRAP_PWRON) {
			WCN_INFO("WIFI WRAP have power down\n");
			/* WRAP power on */
			WCN_INFO("WIFI WRAP start power on\n");
			ret = sprdwcn_bus_reg_read(PD_WIFI_AON_CFG4, &temp, 4);
			temp = temp & (~WIFI_WRAP_PWR_DOWN);
			ret = sprdwcn_bus_reg_write(PD_WIFI_AON_CFG4, &temp, 4);
			udelay(200);
			/* MAC power on */
			WCN_INFO("WIFI MAC start power on\n");
			ret = sprdwcn_bus_reg_read(PD_WIFI_MAC_AON_CFG4,
						   &temp, 4);
			temp = temp & (~WIFI_MAC_PWR_DOWN);
			ret = sprdwcn_bus_reg_write(PD_WIFI_MAC_AON_CFG4,
						    &temp, 4);
			udelay(200);
			/* PHY power on */
			WCN_INFO("WIFI PHY start power on\n");
			ret = sprdwcn_bus_reg_read(PD_WIFI_PHY_AON_CFG4,
						   &temp, 4);
			temp = temp & (~WIFI_PHY_PWR_DOWN);
			ret = sprdwcn_bus_reg_write(PD_WIFI_PHY_AON_CFG4,
						    &temp, 4);
			/* retention */
			WCN_INFO("WIFI retention start power on\n");
			ret = sprdwcn_bus_reg_read(PD_WIFI_AON_CFG4, &temp, 4);
			temp = temp | WIFI_RETENTION;
			ret = sprdwcn_bus_reg_write(PD_WIFI_AON_CFG4, &temp, 4);
		}
		/* WIFI MAC */
		else if ((temp & WIFI_MAC_PWRON) != WIFI_MAC_PWRON) {
			WCN_INFO("WIFI MAC have power down\n");
			/* MAC_AON_WIFI_DOZE_CTL [bit1 =0] */
			ret = sprdwcn_bus_reg_read(DUMP_WIFI_AON_MAC_ADDR,
						   &temp, 4);
			temp = temp & (~(1 << 1));
			ret = sprdwcn_bus_reg_write(DUMP_WIFI_AON_MAC_ADDR,
						    &temp, 4);
			udelay(300);
			/* WIFI_MAC_RTN_SLEEPPS_CTL [bit0] =0 */
			ret = sprdwcn_bus_reg_read(WIFI_MAC_RTN_SLEEPPS_CTL,
						   &temp, 4);
			temp = temp & (~(1 << 0));
			ret = sprdwcn_bus_reg_write(WIFI_MAC_RTN_SLEEPPS_CTL,
						    &temp, 4);
		}

	}

	ret = sprdwcn_bus_reg_read(WIFI_ENABLE, &temp, 4);
	if (ret < 0) {
		WCN_ERR("%s read WIFI_ENABLE reg error:%d\n", __func__, ret);
		return ret;
	}
	WCN_INFO("%s WIFI_ENABLE reg val:0x%x\n", __func__, temp);

	if ((temp & WIFI_ALL_EN) == WIFI_ALL_EN)
		return 0;

	WCN_INFO("WIFI_en and wifi_mac_en is disable\n");
	ret = sprdwcn_bus_reg_read(WIFI_ENABLE, &temp, 4);
	temp = temp | WIFI_EN;
	temp = temp | WIFI_MAC_EN;
	ret = sprdwcn_bus_reg_write(WIFI_ENABLE, &temp, 4);

	return 0;
}

void dump_dummy_read(void)
{
	int ret;
	unsigned int reg_val;

	ret = sprdwcn_bus_reg_read(CHIPID_REG, &reg_val, 4);
	if (ret < 0) {
		WCN_ERR("%s dummy read err:%d\n", __func__, ret);
		WCN_INFO("%s dt fail,start reset!\n", __func__);
		ret = marlin_reset_reg();
		if (ret < 0) {
			WCN_ERR("%s dt fail,reset fail!\n", __func__);
			return;
		}
		ret = sprdwcn_bus_reg_read(CHIPID_REG, &reg_val, 4);
		if (ret < 0) {
			WCN_ERR("%s after reset,dummy read still fail!\n",
				__func__);
			return;
		}
	}
}

/*
 * 0x400F0000 - 0x400F0108 MAC AON
 * check 1:
 * AON APB status Reg(0x4083C00C
 * AON APB Control Reg(0x4083C088   bit1 wrap pwr on(0)/down(1))
 * AON APB Control Reg(0x4083C0A8  bit2 Mac Pwr on(0)/dwn(1))
 * AON APB Control Reg(0x4083C0B8 bit2 Phy pwr on(0)/dwn (1))
 * check 2:
 * Wifi EB : 0x40130004 Wifi EB(bit5)  wifi mac  enable:1
 *
 * 0x40300000 - 0x40358000  wifi 352k share RAM
 * 0x400f1000 - 0x400fe100  wifi reg
 */
int mdbg_dump_mem(void)
{
	long int count;
	int ret;
	static char mdbg_dump_mem_cnt;

	if (mdbg_dump_mem_cnt) {
		WCN_ERR("Not dump again!\n");
		return 0;
	}
	mdbg_dump_mem_cnt++;

	/* init dump array pointer */
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3)
#endif
#if defined CONFIG_UWE5621 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
	{
		wcn_dump_array_size = ARRAY_SIZE(s_wcn_dump_regs_5621);
		memcpy(s_wcn_dump_regs, s_wcn_dump_regs_5621,
		       sizeof(s_wcn_dump_regs_5621));
	}
#endif

#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3L)
#endif
#if defined CONFIG_UWE5622 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
	{
		wcn_dump_array_size = ARRAY_SIZE(s_wcn_dump_regs_5622);
		memcpy(s_wcn_dump_regs, s_wcn_dump_regs_5622,
		       sizeof(s_wcn_dump_regs_5622));
	}
#endif

#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E)
#endif
#if defined CONFIG_UWE5623 || defined CONFIG_CHECK_DRIVER_BY_CHIPID
	{
		wcn_dump_array_size = ARRAY_SIZE(s_wcn_dump_regs_5623);
		memcpy(s_wcn_dump_regs, s_wcn_dump_regs_5623,
		       sizeof(s_wcn_dump_regs_5623));
	}
#endif

	/* DUMP ARM REG */
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5623
	#ifdef CONFIG_WCN_SWD
	swd_dump_arm_reg();
	#endif
#else
	dump_arm_reg();
#endif

#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E)
		swd_dump_arm_reg();
	else
		dump_arm_reg();
#endif

	dump_dummy_read();
	mdbg_clear_log();
	mdbg_atcmd_clean();
	cp_dcache_clean_invalid_all();

	if (wcn_fill_dump_head_info(s_wcn_dump_regs, wcn_dump_array_size))
		return -1;

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5623
	#ifdef CONFIG_WCN_USB
	if (marlin_reset_reg() < 0) {
		WCN_INFO("%s reset marlin fail!\n", __func__);
		goto end;
	}

	wcn_dump_cp_data(s_wcn_dump_regs, 0, RAM_SECTION_NUM - 1);
	#else
	wcn_dump_cp_data(s_wcn_dump_regs, 0, wcn_dump_array_size - 1);
	#endif

	goto end;
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E) {
		wcn_dump_cp_data(s_wcn_dump_regs, 0, wcn_dump_array_size - 1);
		goto end;
	}
#endif /*CONFIG_CHECK_DRIVER_BY_CHIPID*/

	count = mdbg_dump_data(CP_START_ADDR, NULL, FIRMWARE_MAX_SIZE, 0);
	if (count <= 0) {
		WCN_INFO("mdbg start reset marlin reg!\n");
		ret = marlin_reset_reg();
		if (ret < 0)
			return 0;
		cp_dcache_clean_invalid_all();
		count = mdbg_dump_data(CP_START_ADDR, NULL,
				       FIRMWARE_MAX_SIZE, 0);

		WCN_INFO("mdbg only dump ram %ld ok!\n", count);

		goto end;
	}
	WCN_INFO("mdbg dump ram %ld ok!\n", count);

	if (AON_AHB_ADDR) {
		count = mdbg_dump_data(AON_AHB_ADDR, "start_dump_aon_ahb_reg",
		AON_AHB_SIZE, strlen("start_dump_aon_ahb_reg"));
		WCN_INFO("mdbg dump aon ahb %ld ok!\n", count);
	}
	if (AON_APB_ADDR) {
		count = mdbg_dump_data(AON_APB_ADDR, "start_dump_aon_apb_reg",
		AON_APB_SIZE, strlen("start_dump_aon_aph_reg"));
		WCN_INFO("mdbg dump aon_apb %ld ok!\n", count);
	}
	if (BTWF_AHB_ADDR) {
		count = mdbg_dump_data(BTWF_AHB_ADDR, "start_dump_btwf_ahb_reg",
		BTWF_AHB_SIZE, strlen("start_dump_btwf_ahb_reg"));
		WCN_INFO("mdbg dump btwfahb %ld ok!\n", count);
	}
	if (BTWF_APB_ADDR) {
		count = mdbg_dump_data(BTWF_APB_ADDR, "start_dump_btwf_apb_reg",
		BTWF_APB_SIZE, strlen("start_dump_btwf_apb_reg"));
		WCN_INFO("mdbg dump btwfapb %ld ok!\n", count);
	}
	if (AON_CLK_ADDR) {
		count = mdbg_dump_data(AON_CLK_ADDR, "start_dump_aon_clk_reg",
		AON_CLK_SIZE, strlen("start_dump_aon_clk_reg"));
		WCN_INFO("mdbg dump aonclk %ld ok!\n", count);
	}
	if (PRE_DIV_CLK_ADDR) {
		count = mdbg_dump_data(PRE_DIV_CLK_ADDR,
				       "start_dump_pre_div_clk_reg",
				       PRE_DIV_CLK_SIZE,
				       strlen("start_dump_pre_div_clk_reg"));
		WCN_INFO("mdbg dump predivclk %ld ok!\n", count);
	}


	count = mdbg_dump_data(DUMP_SDIO_ADDR, "start_dump_sdio_reg",
			       DUMP_SDIO_ADDR_SIZE,
			      strlen("start_dump_sdio_reg"));
	WCN_INFO("mdbg dump sdio %ld ok!\n", count);

	/* for dump wifi reg */
	ret = check_wifi_power_domain_ison();
	if (ret) {
		WCN_ERR("********:-) :-) :-) :-)*********\n");
		WCN_ERR("!!!mdbg wifi power domain is down!!\n");
		goto next;
	}

#ifdef DUMP_WIFI_AON_MAC_ADDR_SIZE
	if (DUMP_WIFI_AON_MAC_ADDR)
		count = mdbg_dump_data(DUMP_WIFI_AON_MAC_ADDR,
						"start_dump_wifi_aon_reg",
		DUMP_WIFI_AON_MAC_ADDR_SIZE, strlen("start_dump_wifi_aon_reg"));
#endif


	if (DUMP_WIFI_RTN_PD_MAC_ADDR)
		count = mdbg_dump_data(DUMP_WIFI_RTN_PD_MAC_ADDR,
				       "start_dump_wifi_RTN+PD_reg",
				       DUMP_WIFI_RTN_PD_MAC_ADDR_SIZE,
				       strlen("start_dump_wifi_RTN+PD_reg"));

#ifdef DUMP_WIFI_352K_RAM_ADDR_SIZE
	if (DUMP_WIFI_352K_RAM_ADDR) {
		count = mdbg_dump_data(DUMP_WIFI_352K_RAM_ADDR,
				       "start_dump_wifi_352K_RAM_reg",
				       DUMP_WIFI_352K_RAM_ADDR_SIZE,
				       strlen("start_dump_wifi_352K_RAM_reg"));
		WCN_INFO("mdbg dump wifi %ld ok!\n", count);
	}
#endif

	wcn_dump_cp_register(s_wcn_dump_regs);
next:
	if (DUMP_INTC_ADDR) {
		count = mdbg_dump_data(DUMP_INTC_ADDR, "start_dump_intc_reg",
			       DUMP_REG_SIZE,
			       strlen("start_dump_intc_reg"));
		WCN_INFO("mdbg dump intc %ld ok!\n", count);
	}

	if (DUMP_SYSTIMER_ADDR) {
		count = mdbg_dump_data(DUMP_SYSTIMER_ADDR,
					"start_dump_systimer_reg",
			       DUMP_REG_SIZE,
			       strlen("start_dump_systimer_reg"));
		WCN_INFO("mdbg dump systimer %ld ok!\n", count);
	}

	if (DUMP_WDG_ADDR) {
		count = mdbg_dump_data(DUMP_WDG_ADDR, "start_dump_wdg_reg",
			DUMP_REG_SIZE, strlen("start_dump_wdg_reg"));
		WCN_INFO("mdbg dump wdg %ld ok!\n", count);
	}

	if (DUMP_APB_ADDR) {
		count = mdbg_dump_data(DUMP_APB_ADDR, "start_dump_apb_reg",
		DUMP_REG_SIZE, strlen("start_dump_apb_reg"));
		WCN_INFO("mdbg dump apb %ld ok!\n", count);
	}

	if (DUMP_DMA_ADDR) {
		count = mdbg_dump_data(DUMP_DMA_ADDR, "start_dump_dma_reg",
		DUMP_REG_SIZE, strlen("start_dump_dma_reg"));
		WCN_INFO("mdbg dump dma %ld ok!\n", count);
	}

	if (DUMP_AHB_ADDR) {
		count = mdbg_dump_data(DUMP_AHB_ADDR, "start_dump_ahb_reg",
			DUMP_REG_SIZE, strlen("start_dump_ahb_reg"));
		WCN_INFO("mdbg dump ahb %ld ok!\n", count);
	}

	count = mdbg_dump_data(DUMP_FM_ADDR, "start_dump_fm_reg",
		DUMP_FM_ADDR_SIZE, strlen("start_dump_fm_reg"));
	WCN_INFO("mdbg dump fm %ld ok!\n", count);

	if (DUMP_WIFI_ADDR) {
		count = mdbg_dump_data(DUMP_WIFI_ADDR, "start_dump_wifi_reg",
			DUMP_WIFI_ADDR_SIZE, strlen("start_dump_wifi_reg"));
		WCN_INFO("mdbg dump wifi %ld ok!\n", count);
	}

	if (DUMP_BT_CMD_ADDR != 0) {
		count = mdbg_dump_data(DUMP_BT_CMD_ADDR,
			"start_dump_bt_cmd_buf",
		DUMP_BT_CMD_ADDR_SIZE, strlen("start_dump_bt_cmd_buf"));
		WCN_INFO("mdbg dump bt cmd %ld ok!\n", count);
	}

	if (DUMP_BT_ADDR) {
		count = mdbg_dump_data(DUMP_BT_ADDR, "start_dump_bt_reg",
		DUMP_BT_ADDR_SIZE, strlen("start_dump_bt_reg"));
		WCN_INFO("mdbg dump bt %ld ok!\n", count);
	}
	if (BT_ACC_ADDR) {
		count = mdbg_dump_data(BT_ACC_ADDR, "start_dump_bt_acc_reg",
		BT_ACC_SIZE, strlen("start_dump_bt_acc_reg"));
		WCN_INFO("mdbg dump btacc %ld ok!\n", count);
	}
	if (BT_JAL_ADDR) {
		count = mdbg_dump_data(BT_JAL_ADDR, "start_dump_bt_jal_reg",
		BT_JAL_SIZE, strlen("start_dump_bt_jal_reg"));
		WCN_INFO("mdbg dump btjal %ld ok!\n", count);
	}
	if (BT_HAB_ADDR) {
		count = mdbg_dump_data(BT_HAB_ADDR, "start_dump_bt_hab_reg",
		BT_HAB_SIZE, strlen("start_dump_bt_hab_reg"));
		WCN_INFO("mdbg dump bthab %ld ok!\n", count);
	}
	if (BT_LEJAL_ADDR) {
		count = mdbg_dump_data(BT_LEJAL_ADDR, "start_dump_bt_lejal_reg",
		BT_LEJAL_SIZE, strlen("start_dump_bt_lejal_reg"));
		WCN_INFO("mdbg dump btlejal %ld ok!\n", count);
	}
	if (BT_MODEM_ADDR) {
		count = mdbg_dump_data(BT_MODEM_ADDR, "start_dump_bt_modem_reg",
		BT_MODEM_SIZE, strlen("start_dump_bt_modem_reg"));
		WCN_INFO("mdbg dump bt modem %ld ok!\n", count);
	}

	check_bt_buffer_RW();

	if (BT_CMD_BUF_ADDR) {
		count = mdbg_dump_data(BT_CMD_BUF_ADDR,
				       "start_dump_bt_cmd_buf_reg",
				       BT_CMD_BUF_SIZE,
				       strlen("start_dump_bt_cmd_buf_reg"));
		WCN_INFO("mdbg dump bt_cmd buf %ld ok!\n", count);
	}
	if (BT_EVENT_BUF_ADDR) {
		count = mdbg_dump_data(BT_EVENT_BUF_ADDR,
				       "start_dump_bt_event_buf_reg",
				       BT_EVENT_BUF_SIZE,
				       strlen("start_dump_bt_event_buf_reg"));
		WCN_INFO("mdbg dump btevent buf %ld ok!\n", count);
	}
	if (BT_LMP_TX_BUF_ADDR) {
		count = mdbg_dump_data(BT_LMP_TX_BUF_ADDR,
				       "start_dump_bt_lmp_tx_buf_reg",
				       BT_LMP_TX_BUF_SIZE,
				       strlen("start_dump_bt_lmp_tx_buf_reg"));
		WCN_INFO("mdbg dump bt_lmp_tx_buf %ld ok!\n", count);
	}
	if (BT_LMP_RX_BUF_ADDR) {
		count = mdbg_dump_data(BT_LMP_RX_BUF_ADDR,
				       "start_dump_bt_lmp_rx_buf_reg",
				       BT_LMP_RX_BUF_SIZE,
				       strlen("start_dump_bt_lmp_rx_buf_reg"));
		WCN_INFO("mdbg dump bt_lmp_rx_buf %ld ok!\n", count);
	}
	if (BT_ACL_TX_BUF_ADDR) {
		count = mdbg_dump_data(BT_ACL_TX_BUF_ADDR,
				       "start_dump_bt_acl_tx_buf_reg",
				       BT_ACL_TX_BUF_SIZE,
				       strlen("start_dump_bt_acl_tx_buf_reg"));
		WCN_INFO("mdbg dump bt_acl_tx_buf%ld ok!\n", count);
	}
	if (BT_ACL_RX_BUF_ADDR) {
		count = mdbg_dump_data(BT_ACL_RX_BUF_ADDR,
				       "start_dump_bt_acl_rx_buf_reg",
				       BT_ACL_RX_BUF_SIZE,
				       strlen("start_dump_bt_acl_rx_buf_reg"));
		WCN_INFO("mdbg dump bt_acl_rx_buf %ld ok!\n", count);
	}
	if (BT_SCO_TX_BUF_ADDR) {
		count = mdbg_dump_data(BT_SCO_TX_BUF_ADDR,
				       "start_dump_bt_sco_tx_buf_reg",
				       BT_SCO_TX_BUF_SIZE,
				       strlen("start_dump_bt_sco_tx_buf_reg"));
		WCN_INFO("mdbg dump bt_sco_tx_buf %ld ok!\n", count);
	}
	if (BT_SCO_RX_BUF_ADDR) {
		count = mdbg_dump_data(BT_SCO_RX_BUF_ADDR,
				       "start_dump_bt_sco_rx_buf_reg",
				       BT_SCO_RX_BUF_SIZE,
				       strlen("start_dump_bt_sco_rx_buf_reg"));
		WCN_INFO("mdbg dump bt_sco_rx_buf %ld ok!\n", count);
	}
	if (BT_BB_TX_BUF_ADDR) {
		count = mdbg_dump_data(BT_BB_TX_BUF_ADDR,
				       "start_dump_bt_bb_tx_buf_reg",
				       BT_BB_TX_BUF_SIZE,
				       strlen("start_dump_bt_bb_tx_buf_reg"));
		WCN_INFO("mdbg dump bt_bb_tx_buf %ld ok!\n", count);
	}
	if (BT_BB_RX_BUF_ADDR) {
		count = mdbg_dump_data(BT_BB_RX_BUF_ADDR,
				       "start_dump_bt_bb_rx_buf_reg",
				       BT_BB_RX_BUF_SIZE,
				       strlen("start_dump_bt_bb_rx_buf_reg"));
		WCN_INFO("mdbg dump bt_bb_rx_buf %ld ok!\n", count);
	}

end:
	/* Make sure only string "marlin_memdump_finish" to slog one time */
	msleep(40);

	mdbg_dump_str(WCN_DUMP_END_STRING, strlen(WCN_DUMP_END_STRING));
	WCN_INFO("mdbg dump memory finish\n");

	return 0;
}

