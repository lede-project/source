/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * Filename : sdiohal.c
 * Abstract : This file is a implementation for wcn sdio hal function
 *
 * Authors	:
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

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>

#include "bus_common.h"
#include "sdiohal.h"
#include "wcn_glb.h"

#ifdef CONFIG_HISI_BOARD
#include "mach/hardware.h"
#endif

#ifdef CONFIG_AML_BOARD
#include <linux/amlogic/aml_gpio_consumer.h>

extern int wifi_irq_num(void);
extern int wifi_irq_trigger_level(void);
extern void sdio_reinit(void);
extern void sdio_clk_always_on(int on);
extern void sdio_set_max_reqsz(unsigned int size);
#endif

#ifdef CONFIG_RK_BOARD
extern int rockchip_wifi_set_carddetect(int val);
#endif

#ifdef CONFIG_AW_BOARD
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);
#endif

#ifndef MMC_CAP2_SDIO_IRQ_NOTHREAD
#define MMC_CAP2_SDIO_IRQ_NOTHREAD (1 << 17)
#endif

#define CP_GPIO1_REG 0x40840014
#define CP_PIN_FUNC_WPU BIT(8)

#define CP_GPIO1_DATA_BASE 0x40804000
#define CP_GPIO1_BIT BIT(1)

#ifndef IS_BYPASS_WAKE
#define IS_BYPASS_WAKE(addr) (false)
#endif

static int (*scan_card_notify)(void);
struct sdiohal_data_t *sdiohal_data;
static struct sdio_driver sdiohal_driver;

static int sdiohal_card_lock(struct sdiohal_data_t *p_data,
	const char *func)
{
	if ((atomic_inc_return(&p_data->xmit_cnt) >=
		SDIOHAL_REMOVE_CARD_VAL) ||
		!atomic_read(&p_data->xmit_start)) {
		atomic_dec(&p_data->xmit_cnt);
		sdiohal_err("%s xmit_cnt:%d xmit_start:%d,not have card\n",
			    func, atomic_read(&p_data->xmit_cnt),
			    atomic_read(&p_data->xmit_start));
		return -1;
	}

	return 0;
}

static void sdiohal_card_unlock(struct sdiohal_data_t *p_data)
{
	atomic_dec(&p_data->xmit_cnt);
}

struct sdiohal_data_t *sdiohal_get_data(void)
{
	return sdiohal_data;
}

unsigned char sdiohal_get_tx_mode(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return ((p_data->adma_tx_enable == true) ?
		SDIOHAL_ADMA : SDIOHAL_SDMA);
}

unsigned char sdiohal_get_rx_mode(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return ((p_data->adma_rx_enable == true) ?
		SDIOHAL_ADMA : SDIOHAL_SDMA);
}

unsigned char sdiohal_get_irq_type(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->irq_type;
}

unsigned int sdiohal_get_blk_size(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->blk_size ? 512 : 840;
}

void sdiohal_sdio_tx_status(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned char stbba0, stbba1, stbba2, stbba3;
	unsigned char apbrw0, apbrw1, apbrw2, apbrw3;
	unsigned char pubint_raw4;
	int err;

	sdiohal_resume_check();
	sdiohal_op_enter();
	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	stbba0 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA0, &err);
	stbba1 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA1, &err);
	stbba2 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA2, &err);
	stbba3 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA3, &err);
	pubint_raw4 = sdio_readb(p_data->sdio_func[FUNC_0],
				 SDIOHAL_FBR_PUBINT_RAW4, &err);
	apbrw0 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_APBRW0, &err);
	apbrw1 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_APBRW1, &err);
	apbrw2 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_APBRW2, &err);
	apbrw3 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_APBRW3, &err);
	sdio_release_host(p_data->sdio_func[FUNC_0]);
	sdiohal_op_leave();

	sdiohal_info("byte:[0x%x][0x%x][0x%x][0x%x];[0x%x]\n",
		stbba0, stbba1, stbba2, stbba3, pubint_raw4);
	sdiohal_info("byte:[0x%x][0x%x][0x%x][0x%x]\n",
		apbrw0, apbrw1, apbrw2, apbrw3);
}

static void sdiohal_abort(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int err;
	unsigned char val;

	sdiohal_info("sdiohal_abort\n");

#ifdef CONFIG_ARCH_SUNXI
	sprdwcn_bus_set_carddump_status(true);
	return;
#endif

	sdio_claim_host(p_data->sdio_func[FUNC_0]);

	val = sdio_readb(p_data->sdio_func[FUNC_0], 0x0, &err);
	sdiohal_info("before abort, SDIO_VER_CCCR:0x%x\n", val);

	sdio_writeb(p_data->sdio_func[FUNC_0], VAL_ABORT_TRANS,
		SDIOHAL_CCCR_ABORT, &err);

	val = sdio_readb(p_data->sdio_func[FUNC_0], 0x0, &err);
	sdiohal_info("after abort, SDIO_VER_CCCR:0x%x\n", val);

	sdio_release_host(p_data->sdio_func[FUNC_0]);
}

/* Get Success Transfer pac num Before Abort */
static void sdiohal_success_trans_pac_num(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned char stbba0;
	unsigned char stbba1;
	unsigned char stbba2;
	unsigned char stbba3;
	int err;

	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	stbba0 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA0, &err);
	stbba1 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA1, &err);
	stbba2 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA2, &err);
	stbba3 = sdio_readb(p_data->sdio_func[FUNC_0],
			    SDIOHAL_FBR_STBBA3, &err);
	p_data->success_pac_num = stbba0 | (stbba1 << 8) |
	    (stbba2 << 16) | (stbba3 << 24);
	sdio_release_host(p_data->sdio_func[FUNC_0]);

	sdiohal_info("success num:[%d]\n",
		p_data->success_pac_num);
}

unsigned int sdiohal_get_trans_pac_num(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->success_pac_num;
}

int sdiohal_sdio_pt_write(unsigned char *src, unsigned int datalen)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;
	struct timespec tm_begin, tm_end;
	static long time_total_ns;
	static int times_count;

	getnstimeofday(&tm_begin);
	if (unlikely(p_data->card_dump_flag == true)) {
		sdiohal_err("%s line %d dump happened\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (datalen % 4 != 0) {
		sdiohal_err("%s datalen not aligned to 4 byte\n", __func__);
		return -1;
	}

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

	sdiohal_resume_check();
	sdiohal_op_enter();
	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	ret = sdio_writesb(p_data->sdio_func[FUNC_1],
		SDIOHAL_PK_MODE_ADDR, src, datalen);
	sdio_release_host(p_data->sdio_func[FUNC_1]);
	if (ret != 0) {
		sdiohal_success_trans_pac_num();
		sdiohal_abort();
	}
	sdiohal_op_leave();
	sdiohal_card_unlock(p_data);

	getnstimeofday(&tm_end);
	time_total_ns += timespec_to_ns(&tm_end) - timespec_to_ns(&tm_begin);
	times_count++;
	if (!(times_count % PERFORMANCE_COUNT)) {
		sdiohal_pr_perf("tx avg time:%ld len=%d\n",
			(time_total_ns / PERFORMANCE_COUNT), datalen);
		time_total_ns = 0;
		times_count = 0;
	}

	return ret;
}

int sdiohal_sdio_pt_read(unsigned char *src, unsigned int datalen)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;
	struct timespec tm_begin, tm_end;
	static long time_total_ns;
	static int times_count;

	getnstimeofday(&tm_begin);

	if (unlikely(p_data->card_dump_flag == true)) {
		sdiohal_err("%s line %d dump happened\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

	sdiohal_resume_check();
	sdiohal_op_enter();
	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	ret = sdio_readsb(p_data->sdio_func[FUNC_1], src,
		SDIOHAL_PK_MODE_ADDR, datalen);
	sdio_release_host(p_data->sdio_func[FUNC_1]);
	if (ret != 0)
		sdiohal_abort();
	sdiohal_op_leave();
	sdiohal_card_unlock(p_data);

	getnstimeofday(&tm_end);
	time_total_ns += timespec_to_ns(&tm_end) - timespec_to_ns(&tm_begin);
	times_count++;
	if (!(times_count % PERFORMANCE_COUNT)) {
		sdiohal_pr_perf("rx avg time:%ld len=%d\n",
			(time_total_ns / PERFORMANCE_COUNT), datalen);
		time_total_ns = 0;
		times_count = 0;
	}

	return ret;
}

static int sdiohal_config_packer_chain(struct sdiohal_list_t *data_list,
	struct sdio_func *sdio_func, uint fix_inc, bool dir, uint addr)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct mmc_request mmc_req;
	struct mmc_command mmc_cmd;
	struct mmc_data mmc_dat;
	struct mmc_host *host = sdio_func->card->host;
	bool fifo = (fix_inc == SDIOHAL_DATA_FIX);
	uint fn_num = sdio_func->num;
	uint blk_num, blk_size, max_blk_count, max_req_size;
	struct mbuf_t *mbuf_node;
	unsigned int sg_count, sg_data_size;
	unsigned int i, ttl_len = 0, node_num;
	int err_ret = 0;

	node_num = data_list->node_num;
	if (node_num > MAX_CHAIN_NODE_NUM)
		node_num = MAX_CHAIN_NODE_NUM;

	sdiohal_list_check(data_list, __func__, dir);

	blk_size = SDIOHAL_BLK_SIZE;
	max_blk_count = min_t(unsigned int,
			      host->max_blk_count, (uint)MAX_IO_RW_BLK);
	max_req_size = min_t(unsigned int,
			     max_blk_count * blk_size, host->max_req_size);

	sg_count = 0;
	memset(&mmc_req, 0, sizeof(struct mmc_request));
	memset(&mmc_cmd, 0, sizeof(struct mmc_command));
	memset(&mmc_dat, 0, sizeof(struct mmc_data));
	sg_init_table(p_data->sg_list, ARRAY_SIZE(p_data->sg_list));

	mbuf_node = data_list->mbuf_head;
	for (i = 0; i < node_num; i++, mbuf_node = mbuf_node->next) {
		if (!mbuf_node) {
			sdiohal_err("%s tx config adma, mbuf ptr error:%p\n",
				__func__, mbuf_node);
			return -1;
		}

		if (sg_count >= ARRAY_SIZE(p_data->sg_list)) {
			sdiohal_err("%s:sg list exceed limit\n", __func__);
			return -1;
		}

		if (dir)
			sg_data_size = SDIOHAL_ALIGN_4BYTE(mbuf_node->len +
				sizeof(struct sdio_puh_t));
		else
			sg_data_size = MAX_PAC_SIZE;
		if (sg_data_size > MAX_PAC_SIZE) {
			sdiohal_err("pac size > cp buf size,len %d\n",
				sg_data_size);
			return -1;
		}

		if (sg_data_size > host->max_seg_size)
			sg_data_size = host->max_seg_size;

		sg_set_buf(&p_data->sg_list[sg_count++], mbuf_node->buf,
			sg_data_size);
		ttl_len += sg_data_size;
	}

	if (dir) {
		sg_data_size = SDIOHAL_ALIGN_BLK(ttl_len +
			SDIO_PUB_HEADER_SIZE) - ttl_len;
		if (sg_data_size > MAX_PAC_SIZE) {
			sdiohal_err("eof pac size > cp buf size,len %d\n",
				sg_data_size);
			return -1;
		}
		sg_set_buf(&p_data->sg_list[sg_count++],
			p_data->eof_buf, sg_data_size);
		ttl_len = SDIOHAL_ALIGN_BLK(ttl_len
			+ SDIO_PUB_HEADER_SIZE);
	} else {
		sg_data_size = SDIOHAL_DTBS_BUF_SIZE;
		sg_set_buf(&p_data->sg_list[sg_count++],
			p_data->dtbs_buf, sg_data_size);
		ttl_len += sg_data_size;
	}

	if (ttl_len % blk_size != 0) {
		sdiohal_err("ttl_len %d not aligned to blk size\n", ttl_len);
		return -1;
	}

	sdiohal_debug("ttl len:%d sg_count:%d\n", ttl_len, sg_count);

	blk_num = ttl_len / blk_size;
	mmc_dat.sg = p_data->sg_list;
	mmc_dat.sg_len = sg_count;
	mmc_dat.blksz = blk_size;
	mmc_dat.blocks = blk_num;
	mmc_dat.flags = dir ? MMC_DATA_WRITE : MMC_DATA_READ;
	mmc_cmd.opcode = 53; /* SD_IO_RW_EXTENDED */
	mmc_cmd.arg = dir ? 1<<31 : 0;
	mmc_cmd.arg |= (fn_num & 0x7) << 28;
	mmc_cmd.arg |= 1<<27;
	mmc_cmd.arg |= fifo ? 0 : 1<<26;
	mmc_cmd.arg |= (addr & 0x1FFFF) << 9;
	mmc_cmd.arg |= blk_num & 0x1FF;
	mmc_cmd.flags = MMC_RSP_SPI_R5 | MMC_RSP_R5 | MMC_CMD_ADTC;
	mmc_req.cmd = &mmc_cmd;
	mmc_req.data = &mmc_dat;
	if (!fifo)
		addr += ttl_len;

	sdio_claim_host(sdio_func);
	mmc_set_data_timeout(&mmc_dat, sdio_func->card);
	mmc_wait_for_req(host, &mmc_req);
	sdio_release_host(sdio_func);

	err_ret = mmc_cmd.error ? mmc_cmd.error : mmc_dat.error;
	if (err_ret != 0) {
		sdiohal_err("%s:CMD53 %s failed with code %d\n",
			__func__, dir ? "write" : "read", err_ret);
		print_hex_dump(KERN_WARNING, "sdio packer: ",
			       DUMP_PREFIX_NONE, 16, 1,
			       data_list->mbuf_head->buf,
			       SDIOHAL_PRINTF_LEN, true);
		return -1;
	}

	return 0;
}

int sdiohal_adma_pt_write(struct sdiohal_list_t *data_list)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;
	struct timespec tm_begin, tm_end;
	static long time_total_ns;
	static int times_count;

	getnstimeofday(&tm_begin);

	if (unlikely(p_data->card_dump_flag == true)) {
		sdiohal_err("%s line %d dump happened\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

	sdiohal_resume_check();
	sdiohal_op_enter();
	ret = sdiohal_config_packer_chain(data_list,
					  p_data->sdio_func[FUNC_1],
					  SDIOHAL_DATA_FIX, SDIOHAL_WRITE,
					  SDIOHAL_PK_MODE_ADDR);
	if (ret != 0) {
		sdiohal_success_trans_pac_num();
		sdiohal_abort();
	}
	sdiohal_op_leave();
	sdiohal_card_unlock(p_data);

	getnstimeofday(&tm_end);
	time_total_ns += timespec_to_ns(&tm_end) - timespec_to_ns(&tm_begin);
	times_count++;
	if (!(times_count % PERFORMANCE_COUNT)) {
		sdiohal_pr_perf("tx avg time:%ld\n",
			(time_total_ns / PERFORMANCE_COUNT));
		time_total_ns = 0;
		times_count = 0;
	}

	return ret;
}

int sdiohal_adma_pt_read(struct sdiohal_list_t *data_list)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;
	struct timespec tm_begin, tm_end;
	static long time_total_ns;
	static int times_count;

	getnstimeofday(&tm_begin);

	if (unlikely(p_data->card_dump_flag == true)) {
		sdiohal_err("%s line %d dump happened\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

	sdiohal_resume_check();
	sdiohal_op_enter();
	ret = sdiohal_config_packer_chain(data_list,
					  p_data->sdio_func[FUNC_1],
					  SDIOHAL_DATA_FIX, SDIOHAL_READ,
					  SDIOHAL_PK_MODE_ADDR);
	if (ret != 0)
		sdiohal_abort();
	sdiohal_op_leave();
	sdiohal_card_unlock(p_data);

	getnstimeofday(&tm_end);
	time_total_ns += timespec_to_ns(&tm_end) - timespec_to_ns(&tm_begin);
	times_count++;
	if (!(times_count % PERFORMANCE_COUNT)) {
		sdiohal_pr_perf("rx avg time:%ld\n",
			(time_total_ns / PERFORMANCE_COUNT));
		time_total_ns = 0;
		times_count = 0;
	}

	return ret;
}

static int sdiohal_dt_set_addr(unsigned int addr)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned char address[4];
	int err = 0;
	int i;

	for (i = 0; i < 4; i++)
		address[i] = (addr >> (8 * i)) & 0xFF;

	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	sdio_writeb(p_data->sdio_func[FUNC_0], address[0],
		    SDIOHAL_FBR_SYSADDR0, &err);
	if (err != 0)
		goto exit;

	sdio_writeb(p_data->sdio_func[FUNC_0], address[1],
		    SDIOHAL_FBR_SYSADDR1, &err);
	if (err != 0)
		goto exit;

	sdio_writeb(p_data->sdio_func[FUNC_0], address[2],
		    SDIOHAL_FBR_SYSADDR2, &err);
	if (err != 0)
		goto exit;

	sdio_writeb(p_data->sdio_func[FUNC_0], address[3],
		    SDIOHAL_FBR_SYSADDR3, &err);
	if (err != 0)
		goto exit;

exit:
	sdio_release_host(p_data->sdio_func[FUNC_0]);

	return err;
}

struct debug_bus_t {
	char name[32];
	/*
	 * 0:[23:0]
	 * 1:[15:8],[7:0],[23:16]
	 * 2:[7:0],[23:16],[15:8]
	 * 3:[23:0]
	 */
	int mode;
	int sys;
	int size;
};

struct debug_bus_t bus_config[] = {
	//{"aon",			0x00, 0x0, 0x0d + 1}, /* 23 bit*/
	//{"pcie_usb",	0x00, 0x1, 0x17 + 1}, /* 23 bit*/
	{"wifi",		0x00, 0x2, 0x07 + 1}, /* 23 bit*/
	{"btwyf",		0x00, 0x3, 0x23 + 1}, /* 23 bit*/
	//{"ap",			0x00, 0x4, 0xac + 1}, /* 7 bit*/
	{"bt",			0x00, 0x5, 0x0f + 1}, /* 23 bit*/
	{"pmu",			0x00, 0x6, 0x33 + 1}, /* 16 bit*/
	{"bushang",		0x00, 0x7, 0x0d + 1}, /* 23 bit*/
	//{"aon_core",	0x00, 0x8, 0x0d}, /* 23 bit*/
	//{"wifi_axi_mtx",0x00, 0x9, 0x00},

};

static void sdiohal_debug_en(bool enable)
{
	unsigned char reg_val = 0;

	/* ctl en */
	sdiohal_aon_readb(WCN_DEBUG_CTL_REG, &reg_val);
	if (enable)
		reg_val |= WCN_CTL_EN;
	else
		reg_val &= ~WCN_CTL_EN;
	sdiohal_aon_writeb(WCN_DEBUG_CTL_REG, reg_val);
}

static void sdiohal_dump_sys_signal(int index, struct debug_bus_t *config)
{
	unsigned char reg_val = 0;
	unsigned char *data_buf;
	char prefix_str[64];
	int sig_offset;

	sdiohal_info("%s name:%s, mode:0x%x, sys:0x%x, size:0x%x\n",
		     __func__, config[index].name,
		     config[index].mode, config[index].sys,
		     config[index].size);

	sdiohal_aon_readb(WCN_DEBUG_MODE_SYS_REG, &reg_val);
	/* sel bus mode */
	reg_val &= ~WCN_MODE_MASK;
	reg_val |= (config[index].mode << 4);

	/* sel bus sys */
	reg_val &= ~WCN_SYS_MASK;
	reg_val |= config[index].sys;
	sdiohal_aon_writeb(WCN_DEBUG_MODE_SYS_REG, reg_val);

	data_buf = kzalloc(config[index].size * 4, GFP_KERNEL);
	for (sig_offset = 0; sig_offset < config[index].size; sig_offset++) {
		unsigned char *buf_p = &data_buf[sig_offset * 4];

		sdiohal_aon_writeb(WCN_SEL_SIG_REG, sig_offset);
		sdiohal_aon_readb(WCN_SIG_STATE + 0x0, &buf_p[0]);
		sdiohal_aon_readb(WCN_SIG_STATE + 0x1, &buf_p[1]);
		sdiohal_aon_readb(WCN_SIG_STATE + 0x2, &buf_p[2]);
	}

	sprintf(prefix_str, "%s: ", config[index].name);
	print_hex_dump(KERN_WARNING, prefix_str,
				   DUMP_PREFIX_OFFSET, 16, 4,
				   data_buf, config[index].size * 4, false);
}

void sdiohal_dump_debug_bus(void)
{
	unsigned char index;
	struct debug_bus_t *config = bus_config;
	int arry_size = sizeof(bus_config) / sizeof(struct debug_bus_t);

	sdiohal_info("%s entry\n", __func__);

	sdiohal_debug_en(true);
	for (index = 0; index < arry_size; index++)
		sdiohal_dump_sys_signal(index, config);
	sdiohal_debug_en(false);

	sdiohal_info("%s end\n", __func__);
}

static char *sdiohal_haddr[8] = {
	"cm4d",
	"cm4i",
	"cm4s",
	"dmaw",
	"dmar",
	"aon_to_ahb",
	"axi_to_ahb",
	"hready_status",
};

void sdiohal_dump_aon_reg(void)
{
	unsigned char reg_buf[16];
	unsigned char i, j, val = 0;

	sdiohal_info("sdio dump_aon_reg entry\n");

#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5623
	sdiohal_dump_debug_bus();
	return;
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() == WCN_CHIP_MARLIN3E) {
		sdiohal_dump_debug_bus();
		return;
	}
#endif

	for (i = 0; i <= CP_128BIT_SIZE; i++) {
		sdiohal_aon_readb(CP_PMU_STATUS + i, &reg_buf[i]);
		sdiohal_info("pmu sdio status:[0x%x]:0x%x\n",
			     CP_PMU_STATUS + i, reg_buf[i]);
	}

	for (i = 0; i < 8; i++) {
		sdiohal_aon_readb(CP_SWITCH_SGINAL, &val);
		val &= ~BIT(4);
		sdiohal_aon_writeb(CP_SWITCH_SGINAL, val);

		/* bit3:0 bt wifi sys, 1:gnss sys */
		val &= ~(BIT(0) | BIT(1) | BIT(2) | BIT(3));
		val |= i;
		sdiohal_aon_writeb(CP_SWITCH_SGINAL, val);

		val |= BIT(4);
		sdiohal_aon_writeb(CP_SWITCH_SGINAL, val);

		for (j = 0; j < CP_HREADY_SIZE; j++) {
			sdiohal_aon_readb(CP_BUS_HREADY + j, &reg_buf[j]);
			sdiohal_info("%s haddr %d:[0x%x]:0x%x\n",
				     sdiohal_haddr[i], i,
				     CP_BUS_HREADY + j, reg_buf[j]);
		}
	}

	/* check hready_status, if bt hung the bus, reset it.
	 * BIT(2):bt2 hready out
	 * BIT(3):bt2 hready
	 * BIT(4):bt1 hready out
	 * BIT(5):bt1 hready
	 */
	val = (reg_buf[0] & (BIT(2) | BIT(3) | BIT(4) | BIT(5)));
	sdiohal_info("val:0x%x\n", val);
	if ((val >> 2) != 0xf) {
		sdiohal_aon_readb(CP_RESET_SLAVE, &val);
		val |=  BIT(5) | BIT(6);
		sdiohal_aon_writeb(CP_RESET_SLAVE, val);

		for (i = 0; i < CP_HREADY_SIZE; i++) {
			sdiohal_aon_readb(CP_BUS_HREADY + i, &reg_buf[i]);
			sdiohal_info("after reset hready status:[0x%x]:0x%x\n",
				     CP_BUS_HREADY + i, reg_buf[i]);
		}
	}

	sdiohal_info("sdio dump_aon_reg end\n\n");
}
EXPORT_SYMBOL_GPL(sdiohal_dump_aon_reg);

#if SDIO_DUMP_CHANNEL_DATA
/* dump designated channel data when assert happened.
 * wifi cmd header struct as defined below:
 * struct sprdwl_cmd_hdr_t {
 *	u8 common;
 *	u8 cmd_id;
 *	__le16 plen;
 *	__le32 mstime;
 *	s8 status;
 *	u8 rsp_cnt;
 *	u8 reserv[2];
 *	u8 paydata[0];
 * } __packed;
 */
static void sdiohal_dump_channel_data(int channel,
	struct sdiohal_data_bak_t *chn_data, char *chn_str)
{
	char print_str[64];
	unsigned int mstime;

	mstime = chn_data->data_bk[SDIO_PUB_HEADER_SIZE + 4] +
		 (chn_data->data_bk[SDIO_PUB_HEADER_SIZE + 5] << 8) +
		 (chn_data->data_bk[SDIO_PUB_HEADER_SIZE + 6] << 16) +
		 (chn_data->data_bk[SDIO_PUB_HEADER_SIZE + 7] << 24);
	sdiohal_info("chn%d %s, cmdid=%d, mstime=%d, record_time=%d\n",
		     channel, chn_str,
		     chn_data->data_bk[SDIO_PUB_HEADER_SIZE + 1],
		     mstime,
		     chn_data->time);
	sprintf(print_str, "chn%d %s: ", channel, chn_str);
	print_hex_dump(KERN_WARNING, print_str, DUMP_PREFIX_NONE, 16,
		       1, chn_data->data_bk,
		       SDIOHAL_PRINTF_LEN, true);
}
#endif

int sdiohal_writel(unsigned int system_addr, void *buf)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

#ifdef CONFIG_ARCH_SUNXI
	if (p_data->dt_rw_fail)
		return -1;
#endif

	sdiohal_resume_check();
	sdiohal_cp_tx_wakeup(DT_WRITEL);
	sdiohal_op_enter();

	ret = sdiohal_dt_set_addr(system_addr);
	if (ret != 0) {
		sdiohal_op_leave();
		sdiohal_cp_tx_sleep(DT_WRITEL);
		sdiohal_card_unlock(p_data);
		return ret;
	}

	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	sdio_writel(p_data->sdio_func[FUNC_1],
		*(unsigned int *)buf, SDIOHAL_DT_MODE_ADDR, &ret);

	sdio_release_host(p_data->sdio_func[FUNC_1]);
	sdiohal_op_leave();
	sdiohal_cp_tx_sleep(DT_WRITEL);

	if (ret != 0) {
		sdiohal_err("dt writel fail ret:%d, system_addr=0x%x\n",
			    ret, system_addr);
		p_data->dt_rw_fail = 1;
		sdiohal_dump_aon_reg();
		sdiohal_abort();
	}
	sdiohal_card_unlock(p_data);

	return ret;
}

int sdiohal_readl(unsigned int system_addr, void *buf)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

#ifdef CONFIG_ARCH_SUNXI
	if (p_data->dt_rw_fail)
		return -1;
#endif

	sdiohal_resume_check();
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	/* If defined this macro, the driver will read chipid register firstly.
	 * Because sleep and wakeup function need to know register address
	 * by chipid. Because of getting slp_mgr.drv_slp_lock mutex lock,
	 * this logic will cause deadlock.
	 */
	if ((!IS_BYPASS_WAKE(system_addr)) &&
		(system_addr != CHIPID_REG_M3E) &&
		(system_addr != CHIPID_REG_M3_M3L))
#else
	if (!IS_BYPASS_WAKE(system_addr))
#endif
		sdiohal_cp_rx_wakeup(DT_READL);
	sdiohal_op_enter();
	ret = sdiohal_dt_set_addr(system_addr);
	if (ret != 0) {
		sdiohal_op_leave();
		sdiohal_cp_rx_sleep(DT_READL);
		sdiohal_card_unlock(p_data);
		return ret;
	}

	sdio_claim_host(p_data->sdio_func[FUNC_1]);

	*(unsigned int *)buf = sdio_readl(p_data->sdio_func[FUNC_1],
					  SDIOHAL_DT_MODE_ADDR, &ret);

	sdio_release_host(p_data->sdio_func[FUNC_1]);
	sdiohal_op_leave();
#ifdef CONFIG_CHECK_DRIVER_BY_CHIPID
	/* If defined this macro, the driver will read chipid register firstly.
	 * Because sleep and wakeup function need to know register address
	 * by chipid. Because of getting slp_mgr.drv_slp_lock mutex lock,
	 * this logic will cause deadlock.
	 */
	if ((!IS_BYPASS_WAKE(system_addr)) &&
		(system_addr != CHIPID_REG_M3E) &&
		(system_addr != CHIPID_REG_M3_M3L))
#else
	if (!IS_BYPASS_WAKE(system_addr))
#endif
		sdiohal_cp_rx_sleep(DT_READL);
	if (ret != 0) {
		sdiohal_err("dt readl fail ret:%d, system_addr=0x%x\n",
			    ret, system_addr);
		p_data->dt_rw_fail = 1;
		sdiohal_dump_aon_reg();
		sdiohal_abort();
	}
	sdiohal_card_unlock(p_data);

	return ret;
}

static int sdiohal_blksz_for_byte_mode(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
}

static int sdiohal_card_broken_byte_mode_512(
	const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BROKEN_BYTE_MODE_512;
}

static unsigned int max_bytes(struct sdio_func *func)
{
	unsigned int mval = func->card->host->max_blk_size;

	if (sdiohal_blksz_for_byte_mode(func->card))
		mval = min(mval, func->cur_blksize);
	else
		mval = min(mval, func->max_blksize);

	if (sdiohal_card_broken_byte_mode_512(func->card))
		return min(mval, 511u);

	/* maximum size for byte mode */
	return min(mval, 512u);
}

int sdiohal_dt_write(unsigned int system_addr,
			    void *buf, unsigned int len)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned int remainder = len;
	unsigned int trans_len;
	int ret = 0;

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

#ifdef CONFIG_ARCH_SUNXI
	if (p_data->dt_rw_fail)
		return -1;
#endif

	sdiohal_resume_check();
	sdiohal_cp_tx_wakeup(DT_WRITE);
	sdiohal_op_enter();

	ret = sdiohal_dt_set_addr(system_addr);
	if (ret != 0) {
		sdiohal_op_leave();
		sdiohal_cp_tx_sleep(DT_WRITE);
		sdiohal_card_unlock(p_data);
		return ret;
	}

	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	while (remainder > 0) {
		if (remainder >= p_data->sdio_func[FUNC_1]->cur_blksize)
			trans_len = p_data->sdio_func[FUNC_1]->cur_blksize;
		else
			trans_len = min(remainder,
					max_bytes(p_data->sdio_func[FUNC_1]));
		ret = sdio_memcpy_toio(p_data->sdio_func[FUNC_1],
				       SDIOHAL_DT_MODE_ADDR, buf, trans_len);
		if (ret)
			break;

		remainder -= trans_len;
		buf += trans_len;
	}
	sdio_release_host(p_data->sdio_func[FUNC_1]);
	sdiohal_op_leave();
	sdiohal_cp_tx_sleep(DT_WRITE);
	if (ret != 0) {
		sdiohal_err("dt write fail ret:%d, system_addr=0x%x\n",
			    ret, system_addr);
		p_data->dt_rw_fail = 1;
		sdiohal_dump_aon_reg();
		sdiohal_abort();
	}
	sdiohal_card_unlock(p_data);

	return ret;
}

int sdiohal_dt_read(unsigned int system_addr, void *buf,
			   unsigned int len)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned int remainder = len;
	unsigned int trans_len;
	int ret = 0;

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

#ifdef CONFIG_ARCH_SUNXI
	if (p_data->dt_rw_fail)
		return -1;
#endif

	sdiohal_resume_check();
	sdiohal_cp_rx_wakeup(DT_READ);
	sdiohal_op_enter();
	ret = sdiohal_dt_set_addr(system_addr);
	if (ret != 0) {
		sdiohal_op_leave();
		sdiohal_cp_rx_sleep(DT_READ);
		sdiohal_card_unlock(p_data);
		return ret;
	}

	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	while (remainder > 0) {
		if (remainder >= p_data->sdio_func[FUNC_1]->cur_blksize)
			trans_len = p_data->sdio_func[FUNC_1]->cur_blksize;
		else
			trans_len = min(remainder,
					max_bytes(p_data->sdio_func[FUNC_1]));
		ret = sdio_memcpy_fromio(p_data->sdio_func[FUNC_1],
					 buf, SDIOHAL_DT_MODE_ADDR, trans_len);
		if (ret)
			break;

		remainder -= trans_len;
		buf += trans_len;
	}
	sdio_release_host(p_data->sdio_func[FUNC_1]);
	sdiohal_op_leave();
	sdiohal_cp_rx_sleep(DT_READ);
	if (ret != 0) {
		sdiohal_err("dt read fail ret:%d, system_addr=0x%x\n",
			    ret, system_addr);
		p_data->dt_rw_fail = 1;
		sdiohal_dump_aon_reg();
		sdiohal_abort();
	}
	sdiohal_card_unlock(p_data);

	return ret;
}

int sdiohal_aon_readb(unsigned int addr, unsigned char *val)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int err = 0;
	unsigned char reg_val = 0;

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

	sdiohal_resume_check();
	sdiohal_op_enter();
	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	reg_val = sdio_readb(p_data->sdio_func[FUNC_0], addr, &err);
	if (val)
		*val = reg_val;
	sdio_release_host(p_data->sdio_func[FUNC_0]);
	sdiohal_op_leave();
	sdiohal_card_unlock(p_data);

	return err;
}

int sdiohal_aon_writeb(unsigned int addr, unsigned char val)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int err = 0;

	if (sdiohal_card_lock(p_data, __func__))
		return -1;

	sdiohal_resume_check();
	sdiohal_op_enter();
	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	sdio_writeb(p_data->sdio_func[FUNC_0], val, addr, &err);
	sdio_release_host(p_data->sdio_func[FUNC_0]);
	sdiohal_op_leave();
	sdiohal_card_unlock(p_data);

	return err;
}

unsigned long long sdiohal_get_rx_total_cnt(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->rx_packer_cnt;
}

void sdiohal_set_carddump_status(unsigned int flag)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	sdiohal_info("carddump flag set[%d]\n", flag);
	if (flag == true) {
		if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
			sdio_claim_host(p_data->sdio_func[FUNC_1]);
			sdio_release_irq(p_data->sdio_func[FUNC_1]);
			sdio_release_host(p_data->sdio_func[FUNC_1]);
		} else if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
			(p_data->irq_num > 0))
			disable_irq(p_data->irq_num);
		sdiohal_info("disable rx int for dump\n");
	}

#if SDIO_DUMP_CHANNEL_DATA
	sdiohal_dump_channel_data(SDIO_DUMP_TX_CHANNEL_NUM,
				  &p_data->chntx_push_old,
				  "tx push old");
	sdiohal_dump_channel_data(SDIO_DUMP_TX_CHANNEL_NUM,
				  &p_data->chntx_denq_old,
				  "tx denq old");
	sdiohal_dump_channel_data(SDIO_DUMP_RX_CHANNEL_NUM,
				  &p_data->chnrx_dispatch_old,
				  "rx dispatch old");
	sdiohal_dump_channel_data(SDIO_DUMP_TX_CHANNEL_NUM,
				  &p_data->chntx_push_new,
				  "tx push new");
	sdiohal_dump_channel_data(SDIO_DUMP_TX_CHANNEL_NUM,
				  &p_data->chntx_denq_new,
				  "tx denq new");
	sdiohal_dump_channel_data(SDIO_DUMP_RX_CHANNEL_NUM,
				  &p_data->chnrx_dispatch_new,
				  "rx dispatch new");
#endif
	p_data->card_dump_flag = flag;
}

unsigned int sdiohal_get_carddump_status(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->card_dump_flag;
}

static void sdiohal_disable_rx_irq(int irq)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	if (p_data->irq_type != SDIOHAL_RX_EXTERNAL_IRQ)
		return;

	sdiohal_atomic_add(1, &p_data->irq_cnt);
	disable_irq_nosync(irq);
}

void sdiohal_enable_rx_irq(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	if (p_data->irq_type != SDIOHAL_RX_EXTERNAL_IRQ)
		return;

	sdiohal_atomic_sub(1, &p_data->irq_cnt);
	if (p_data->irq_num > 0) {
		irq_set_irq_type(p_data->irq_num, p_data->irq_trigger_type);
		enable_irq(p_data->irq_num);
	}
}

static irqreturn_t sdiohal_irq_handler(int irq, void *para)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	sdiohal_debug("%s entry\n", __func__);

	sdiohal_lock_rx_ws();
	sdiohal_disable_rx_irq(irq);

	getnstimeofday(&p_data->tm_begin_irq);
	sdiohal_rx_up();

	return IRQ_HANDLED;
}

static int sdiohal_enable_slave_irq(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int err;
	unsigned char reg_val;

	/* set func1 dedicated0,1 int to ap enable */

	if (p_data->irq_type != SDIOHAL_RX_EXTERNAL_IRQ)
		return 0;

	sdiohal_resume_check();
	sdiohal_op_enter();
	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	reg_val = sdio_readb(p_data->sdio_func[FUNC_0],
			     SDIOHAL_FBR_DEINT_EN, &err);
	sdio_writeb(p_data->sdio_func[FUNC_0],
		    reg_val | VAL_DEINT_ENABLE, SDIOHAL_FBR_DEINT_EN, &err);
	reg_val = sdio_readb(p_data->sdio_func[FUNC_0],
			     SDIOHAL_FBR_DEINT_EN, &err);
	sdio_release_host(p_data->sdio_func[FUNC_0]);
	sdiohal_op_leave();

	return 0;
}

static int sdiohal_host_irq_init(unsigned int irq_gpio_num)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;

	sdiohal_debug("%s enter\n", __func__);

#ifdef CONFIG_AML_BOARD
	/* As for amlogic platform, gpio trigger type low will request fail. */
	p_data->irq_num = wifi_irq_num();
	if (wifi_irq_trigger_level() == GPIO_IRQ_LOW)
		p_data->irq_trigger_type = IRQF_TRIGGER_LOW;
	else
		p_data->irq_trigger_type = IRQF_TRIGGER_HIGH;
	sdiohal_info("%s sdio gpio irq num:%d, trigger_type:%s\n",
		     __func__, p_data->irq_num,
		     ((p_data->irq_trigger_type == IRQF_TRIGGER_LOW) ?
		     "low" : "high"));
#else
	if (irq_gpio_num == 0)
		return ret;

	ret = gpio_request(irq_gpio_num, "sdiohal_gpio");
	if (ret < 0) {
		sdiohal_err("req gpio irq = %d fail!!!", irq_gpio_num);
		return ret;
	}

	ret = gpio_direction_input(irq_gpio_num);
	if (ret < 0) {
		sdiohal_err("gpio:%d input set fail!!!", irq_gpio_num);
		return ret;
	}

	p_data->irq_num = gpio_to_irq(irq_gpio_num);
	p_data->irq_trigger_type = IRQF_TRIGGER_HIGH;
#endif

	return ret;
}

static int sdiohal_get_dev_func(struct sdio_func *func)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	if (func->num >= SDIOHAL_MAX_FUNCS) {
		sdiohal_err("func num err!!! func num is %d!!!",
			func->num);
		return -1;
	}
	sdiohal_debug("func num is %d.", func->num);

	if (func->num == 1) {
		p_data->sdio_func[FUNC_0] = kmemdup(func, sizeof(*func),
							 GFP_KERNEL);
		p_data->sdio_func[FUNC_0]->num = 0;
		p_data->sdio_func[FUNC_0]->max_blksize = SDIOHAL_BLK_SIZE;
	}

	p_data->sdio_func[FUNC_1] = func;

	return 0;
}

#ifdef CONFIG_WCN_PARSE_DTS
static struct mmc_host *sdiohal_dev_get_host(struct device_node *np_node)
{
	void *drv_data;
	struct mmc_host *host_mmc;
	struct platform_device *pdev;

	pdev = of_find_device_by_node(np_node);
	if (pdev == NULL) {
		sdiohal_err("sdio dev get platform device failed!!!");
		return NULL;
	}

	drv_data = platform_get_drvdata(pdev);
	if (drv_data == NULL) {
		sdiohal_err("sdio dev get drv data failed!!!");
		return NULL;
	}

	host_mmc = drv_data;
	sdiohal_info("host_mmc:%p private data:0x%lx containerof:%p\n",
		     host_mmc, *(host_mmc->private),
		     container_of(drv_data, struct mmc_host, private));

	if (*(host_mmc->private) == (unsigned long)host_mmc)
		return host_mmc;
	else
		return container_of(drv_data, struct mmc_host, private);
}
#endif

static int sdiohal_parse_dt(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
#ifdef CONFIG_WCN_PARSE_DTS
	struct device_node *np;
	struct device_node *sdio_node;

	np = of_find_node_by_name(NULL, "uwe-bsp");
	if (!np) {
		sdiohal_err("dts node not found");
		return -1;
	}
#endif

	/* adma_tx_enable and adma_rx_enable */
#ifdef CONFIG_SDIO_TX_ADMA_MODE
	p_data->adma_tx_enable = true;
#endif
#ifdef CONFIG_WCN_PARSE_DTS
	if (of_get_property(np, "adma-tx", NULL))
		p_data->adma_tx_enable = true;
#endif

#ifdef CONFIG_SDIO_RX_ADMA_MODE
	p_data->adma_rx_enable = true;
#endif
#ifdef CONFIG_WCN_PARSE_DTS
	if (of_get_property(np, "adma-rx", NULL))
		p_data->adma_rx_enable = true;
#endif

	/* power seq */
#ifdef CONFIG_SDIO_PWRSEQ
	p_data->pwrseq = true;
#endif
#ifdef CONFIG_WCN_PARSE_DTS
	if (of_get_property(np, "pwrseq", NULL))
		p_data->pwrseq = true;
#endif

	/* irq type */
#ifdef CONFIG_SDIO_INBAND_INT
	p_data->irq_type = SDIOHAL_RX_INBAND_IRQ;
#elif defined(CONFIG_SDIO_INBAND_POLLING)
	p_data->irq_type = SDIOHAL_RX_POLLING;
#else
	p_data->irq_type = SDIOHAL_RX_EXTERNAL_IRQ;
#endif
#ifdef CONFIG_WCN_PARSE_DTS
	if (of_get_property(np, "data-irq", NULL))
		p_data->irq_type = SDIOHAL_RX_INBAND_IRQ;
	else if (of_get_property(np, "rx-polling", NULL))
		p_data->irq_type = SDIOHAL_RX_POLLING;
	else {
		p_data->irq_type = SDIOHAL_RX_EXTERNAL_IRQ;
		p_data->gpio_num =
			of_get_named_gpio(np, "sdio-ext-int-gpio", 0);
		if (!gpio_is_valid(p_data->gpio_num)) {
			sdiohal_err("can not get sdio int gpio%d\n",
				    p_data->gpio_num);
			p_data->gpio_num = 0;
		}
	}
#else /* else of CONFIG_WCN_PARSE_DTS */
	p_data->gpio_num = 0;
#endif

	/* block size */
#ifdef CONFIG_SDIO_BLKSIZE_512
	p_data->blk_size = true;
#endif
#ifdef CONFIG_WCN_PARSE_DTS
	if (of_get_property(np, "blksz-512", NULL))
		p_data->blk_size = true;
#endif

	sdiohal_info("%s adma_tx:%d, adma_rx:%d, pwrseq:%d, irq type:%s, "
		     "gpio_num:%d, blksize:%d\n",
		     __func__, p_data->adma_tx_enable,
		     p_data->adma_rx_enable, p_data->pwrseq,
		     ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) ? "gpio" :
		     (((p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) ?
		     "data" : "polling"))), p_data->gpio_num,
		     sprdwcn_bus_get_blk_size());

#ifdef CONFIG_WCN_PARSE_DTS
	sdio_node = of_parse_phandle(np, "sdhci-name", 0);
	if (sdio_node == NULL)
		sdiohal_info("not config sdio host node.");

#if (defined(CONFIG_RK_BOARD) || defined(CONFIG_AW_BOARD))
	/* will get host at sdiohal_probe */
	return 0;
#endif

	p_data->sdio_dev_host = sdiohal_dev_get_host(sdio_node);
	if (p_data->sdio_dev_host == NULL) {
		sdiohal_err("get host failed!!!");
		return -1;
	}
	sdiohal_info("get host ok!!!");
#endif /* end of CONFIG_WCN_PARSE_DTS */

	return 0;
}

static int sdiohal_set_cp_pin_status(void)
{
	int reg_value;

	/* cp pin pull down on default except uwe5621 */
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifndef CONFIG_UWE5621
	return 0;
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	if (wcn_get_chip_model() != WCN_CHIP_MARLIN3)
		return 0;
#endif
	/*
	 * Because of cp pin pull up on default, It's lead to
	 * the sdio mistaken interruption before cp run,
	 * So set the pin to no pull up on init.
	 */
	sdiohal_readl(CP_GPIO1_REG, &reg_value);
	sdiohal_info("reg_value: 0x%x\n", reg_value);
	reg_value &= ~(CP_PIN_FUNC_WPU);
	reg_value |= (1<<7);
	sdiohal_writel(CP_GPIO1_REG, &reg_value);

	sdiohal_readl(CP_GPIO1_REG, &reg_value);
	sdiohal_info("reg_value: 0x%x\n", reg_value);

	/* gpio set low*/
	sdiohal_readl(CP_GPIO1_DATA_BASE + 0x04, &reg_value);
	sdiohal_info("reg_value 0x04: 0x%x\n", reg_value);
	reg_value |= (1<<1);
	sdiohal_writel(CP_GPIO1_DATA_BASE + 0x04, &reg_value);

	sdiohal_readl(CP_GPIO1_DATA_BASE + 0x04, &reg_value);
	sdiohal_info("reg_value 0x04: 0x%x\n", reg_value);

	sdiohal_readl(CP_GPIO1_DATA_BASE + 0x08, &reg_value);
	sdiohal_info("reg_value 0x08: 0x%x\n", reg_value);
	reg_value |= (1<<1);
	sdiohal_writel(CP_GPIO1_DATA_BASE + 0x08, &reg_value);

	sdiohal_readl(CP_GPIO1_DATA_BASE, &reg_value);
	sdiohal_info("reg_value 0x08: 0x%x\n", reg_value);

	sdiohal_readl(CP_GPIO1_DATA_BASE + 0x0, &reg_value);
	sdiohal_info("reg_value 0x0: 0x%x\n", reg_value);
	reg_value &= ~(CP_PIN_FUNC_WPU);
	reg_value |= (1<<1);
	sdiohal_writel(CP_GPIO1_DATA_BASE, &reg_value);

	sdiohal_readl(CP_GPIO1_DATA_BASE, &reg_value);
	sdiohal_info("reg_value 0x0: 0x%x\n", reg_value);
	return 0;
}

static void sdiohal_irq_handler_data(struct sdio_func *func)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int err;

	sdiohal_debug("%s entry\n", __func__);

	if (!WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		sdiohal_err("%s line %d not have card\n", __func__, __LINE__);
		return;
	}

	sdiohal_resume_check();

	/* send cmd to clear cp int status */
	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	sdio_f0_readb(p_data->sdio_func[FUNC_0], SDIO_CCCR_INTx, &err);
	sdio_release_host(p_data->sdio_func[FUNC_0]);
	if (err < 0)
		sdiohal_err("%s error %d\n", __func__, err);

	sdiohal_lock_rx_ws();
	sdiohal_rx_up();
}

static int sdiohal_suspend(struct device *dev)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct mchn_ops_t *sdiohal_ops;
	struct sdio_func *func;
	int chn, ret = 0;

	sdiohal_info("[%s]enter\n", __func__);

#ifdef CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
	/* After resume will reset sdio reg */
	ret = sprdwcn_bus_reg_read(SDIO_CP_INT_EN, &p_data->sdio_int_reg, 4);
	sdiohal_info("%s SDIO_CP_INT_EN(0x58):0x%x ret:%d\n", __func__,
		     p_data->sdio_int_reg, ret);
#endif

	atomic_set(&p_data->flag_suspending, 1);
	for (chn = 0; chn < SDIO_CHANNEL_NUM; chn++) {
		sdiohal_ops = chn_ops(chn);
		if (sdiohal_ops && sdiohal_ops->power_notify) {
#ifdef CONFIG_WCN_SLP
			sdio_record_power_notify(true);
#endif
			ret = sdiohal_ops->power_notify(chn, false);
			if (ret != 0) {
				sdiohal_info("[%s] chn:%d suspend fail\n",
					     __func__, chn);
				atomic_set(&p_data->flag_suspending, 0);
				return ret;
			}
		}
	}

#ifdef CONFIG_WCN_SLP
	sdio_wait_pub_int_done();
	sdio_record_power_notify(false);
#endif

	if (marlin_get_bt_wl_wake_host_en()) {
		/* Inform CP side that AP will enter into suspend status. */
		sprdwcn_bus_aon_writeb(REG_AP_INT_CP0, (1 << AP_SUSPEND));
	}

	atomic_set(&p_data->flag_suspending, 0);
	atomic_set(&p_data->flag_resume, 0);
	if (atomic_read(&p_data->irq_cnt))
		sdiohal_lock_rx_ws();

	if (WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		func = container_of(dev, struct sdio_func, dev);
		func->card->host->pm_flags |= MMC_PM_KEEP_POWER;
		sdiohal_info("%s pm_flags=0x%x, caps=0x%x\n", __func__,
			     func->card->host->pm_flags,
			     func->card->host->caps);
	}

	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
		sdio_claim_host(p_data->sdio_func[FUNC_1]);
		sdio_release_irq(p_data->sdio_func[FUNC_1]);
		sdio_release_host(p_data->sdio_func[FUNC_1]);
	} else if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
		(p_data->irq_num > 0))
		disable_irq(p_data->irq_num);

	if (WCN_CARD_EXIST(&p_data->xmit_cnt))
		atomic_add(SDIOHAL_REMOVE_CARD_VAL, &p_data->xmit_cnt);

	return 0;
}

static int sdiohal_resume(struct device *dev)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct mchn_ops_t *sdiohal_ops;
	struct sdio_func *func = p_data->sdio_func[FUNC_1];
	int chn;
	int ret = 0;
#ifdef CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
	int init_state = 0;
#endif

	sdiohal_info("[%s]enter\n", __func__);

#if (defined(CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO) ||\
	defined(CONFIG_WCN_RESUME_POWER_DOWN))
	/*
	 * For hisi board, sdio host will power down.
	 * So sdio slave need to reset and reinit.
	 */
	mmc_power_save_host(p_data->sdio_dev_host);
	mdelay(5);
	mmc_power_restore_host(p_data->sdio_dev_host);

	if (!p_data->pwrseq) {
		/* Enable Function 1 */
		sdio_claim_host(p_data->sdio_func[FUNC_1]);
		ret = sdio_enable_func(p_data->sdio_func[FUNC_1]);
		sdio_set_block_size(p_data->sdio_func[FUNC_1],
				    SDIOHAL_BLK_SIZE);
		p_data->sdio_func[FUNC_1]->max_blksize = SDIOHAL_BLK_SIZE;
		sdio_release_host(p_data->sdio_func[FUNC_1]);
		if (ret < 0) {
			sdiohal_err("enable func1 err!!! ret is %d\n", ret);
			return ret;
		}
		sdiohal_info("enable func1 ok\n");

		atomic_set(&p_data->flag_resume, 1);
		sdiohal_enable_slave_irq();
	} else
		pm_runtime_put_noidle(&func->dev);
#endif

	atomic_set(&p_data->flag_resume, 1);
	if (!WCN_CARD_EXIST(&p_data->xmit_cnt))
		atomic_sub(SDIOHAL_REMOVE_CARD_VAL, &p_data->xmit_cnt);

#ifdef CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
	/* After resume will reset sdio reg, re-enable sdio int. */
	ret = sprdwcn_bus_reg_write(SDIO_CP_INT_EN, &p_data->sdio_int_reg, 4);
	sdiohal_info("%s SDIO_CP_INT_EN(0x58):0x%x ret:%d\n", __func__,
		     p_data->sdio_int_reg, ret);
#endif

	if (marlin_get_bt_wl_wake_host_en()) {
		/* Inform CP side that AP reset sdio done during resume. */
		sprdwcn_bus_aon_writeb(REG_AP_INT_CP0, (1 << AP_RESUME));
	}

#ifdef CONFIG_WCN_RESUME_POWER_DOWN
	marlin_schedule_download_wq();
#endif

#if (defined(CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO) ||\
	defined(CONFIG_WCN_RESUME_POWER_DOWN))
	sdiohal_set_cp_pin_status();
#endif

	if (WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		func = container_of(dev, struct sdio_func, dev);
		func->card->host->pm_flags &= ~MMC_PM_KEEP_POWER;
		sdiohal_info("%s pm_flags=0x%x, caps=0x%x\n", __func__,
			     func->card->host->pm_flags,
			     func->card->host->caps);
	}

#ifdef CONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
	/*
	 * polling sync_addr,
	 * If equal to SYNC_SDIO_REINIT_DONE, cp receive sdio int (ap resume);
	 * Then write sync_addr to SYNC_SDIO_IS_READY,
	 * and enable sdio rx int.
	 */
	do {
		ret = sprdwcn_bus_reg_read(SYNC_ADDR, &init_state, 4);
		sdiohal_info("%s init_state:0x%x ret:%d\n", __func__,
			     init_state, ret);
		if (init_state == SYNC_SDIO_REINIT_DONE) {
			init_state = SYNC_SDIO_IS_READY;
			ret = sprdwcn_bus_reg_write(SYNC_ADDR, &init_state, 4);
			if (ret < 0)
				sdiohal_err("write SDIO_READY err:%d\n", ret);
			else
				break;
		}
		msleep(20);
	} while (1);
#endif

#ifndef CONFIG_WCN_RESUME_POWER_DOWN
	/* If CONFIG_WCN_RESUME_POWER_DOWN,
	 * will enable irq at sdiohal_runtime_get function.
	 */
	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
		sdio_claim_host(p_data->sdio_func[FUNC_1]);
		if (sdio_claim_irq(p_data->sdio_func[FUNC_1],
			sdiohal_irq_handler_data)) {
			sdiohal_err("%s: Failed to request IRQ\n",
				    __func__);
			sdio_release_host(p_data->sdio_func[FUNC_1]);
			return -1;
		}
		sdio_release_host(p_data->sdio_func[FUNC_1]);
	} else if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
		(p_data->irq_num > 0))
		enable_irq(p_data->irq_num);
#endif

	for (chn = 0; chn < SDIO_CHANNEL_NUM; chn++) {
		sdiohal_ops = chn_ops(chn);
		if (sdiohal_ops && sdiohal_ops->power_notify) {
			ret = sdiohal_ops->power_notify(chn, true);
			if (ret != 0)
				sdiohal_info("[%s] chn:%d resume fail\n",
					     __func__, chn);
		}
	}

	return 0;
}

int sdiohal_runtime_get(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	int ret;

	sdiohal_info("%s entry\n", __func__);
	if (!p_data)
		return -ENODEV;

	if (!WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		sdiohal_err("%s line %d not have card\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (!p_data->pwrseq) {
		if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
			sdio_claim_host(p_data->sdio_func[FUNC_1]);
			if (sdio_claim_irq(p_data->sdio_func[FUNC_1],
				sdiohal_irq_handler_data)) {
				sdiohal_err("%s: Failed to request IRQ\n",
					    __func__);
				sdio_release_host(p_data->sdio_func[FUNC_1]);
				return -1;
			}
			sdio_release_host(p_data->sdio_func[FUNC_1]);
		} else if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
			(p_data->irq_num > 0))
			enable_irq(p_data->irq_num);

		return 0;
	}

	ret = pm_runtime_get_sync(&p_data->sdio_func[FUNC_1]->dev);
	if (ret < 0) {
		sdiohal_err("sdiohal_rumtime_get err: %d", ret);
		return ret;
	}

	/* Enable Function 1 */
	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	ret = sdio_enable_func(p_data->sdio_func[FUNC_1]);
	sdio_set_block_size(p_data->sdio_func[FUNC_1], SDIOHAL_BLK_SIZE);
	p_data->sdio_func[FUNC_1]->max_blksize = SDIOHAL_BLK_SIZE;
	sdio_release_host(p_data->sdio_func[FUNC_1]);
	if (ret < 0) {
		sdiohal_err("%s enable func1 err!!! ret is %d", __func__, ret);
		return ret;
	}
	sdiohal_info("enable func1 ok!!!");
	sdiohal_set_cp_pin_status();
	sdiohal_enable_slave_irq();
	if (p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ)
		enable_irq(p_data->irq_num);
	sdiohal_info("sdihal: %s ret:%d\n", __func__, ret);

	return ret;
}

int sdiohal_runtime_put(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int xmit_cnt;

	sdiohal_info("%s entry\n", __func__);

	if (!p_data)
		return -ENODEV;

	if (!WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		sdiohal_err("%s line %d not have card\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
		sdio_claim_host(p_data->sdio_func[FUNC_1]);
		sdio_release_irq(p_data->sdio_func[FUNC_1]);
		sdio_release_host(p_data->sdio_func[FUNC_1]);
	} else if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
		(p_data->irq_num > 0))
		disable_irq(p_data->irq_num);

#ifndef CONFIG_AML_BOARD
	/* As for amlogic platform, NOT remove card
	 * after chip power off. So won't probe again.
	 */
	atomic_set(&p_data->xmit_start, 0);
#endif
	xmit_cnt = atomic_read(&p_data->xmit_cnt);
	while ((xmit_cnt > 0) &&
		(xmit_cnt < SDIOHAL_REMOVE_CARD_VAL)) {
		usleep_range(1000, 2000);
		xmit_cnt = atomic_read(&p_data->xmit_cnt);
		sdiohal_info("%s wait xmit_cnt:%d\n",
			     __func__, xmit_cnt);
	}

	sdiohal_info("%s wait xmit_cnt end\n", __func__);

	if (!p_data->pwrseq)
		return 0;

	return pm_runtime_put_sync(&p_data->sdio_func[FUNC_1]->dev);
}

#ifdef SDIO_RESET_DEBUG
int sdiohal_disable_apb_reset(void)
{
#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef SDIO_RESET_ENABLE
	int reg_value;

	sdiohal_readl(SDIO_RESET_ENABLE, &reg_value);
	sdiohal_info("0x40930040: 0x%x\n", reg_value);
	reg_value &= ~BIT(4);
	sdiohal_writel(SDIO_RESET_ENABLE, &reg_value);
#endif
#else /*CONFIG_CHECK_DRIVER_BY_CHIPID*/
	int ret_value;
	unsigned int sdio_reset_enable = 0x40930040;

	sdiohal_readl(sdio_reset_enable, &reg_value);
	sdiohal_info("0x40930040: 0x%x\n", reg_value);
	reg_value &= ~BIT(4);
	sdiohal_writel(sdio_reset_enable, &reg_value);
#endif

	return 0;
}

/*
 * full_reset: 1, reset sdio and apb;
 * full_reset: 0, only reset sdio.
 */
void sdiohal_reset(bool full_reset)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret;
	u8 val;

	sdio_claim_host(p_data->sdio_func[FUNC_0]);
	val = sdio_readb(p_data->sdio_func[FUNC_0], SDIOHAL_CCCR_ABORT, &ret);
	if (ret)
		val = 0x08;
	else
		val |= 0x08;
	sdio_writeb(p_data->sdio_func[FUNC_0], val, SDIOHAL_CCCR_ABORT, &ret);
	sdio_release_host(p_data->sdio_func[FUNC_0]);

	sdio_reset_comm((p_data->sdio_dev_host->card));

	/* rst apb */
	if (full_reset) {
		sdiohal_aon_writeb(0x02, 0xf);
		sdiohal_aon_writeb(0x02, 0x0);
	}

	/* Enable Function 1 */
	sdio_claim_host(p_data->sdio_func[FUNC_1]);
	ret = sdio_enable_func(p_data->sdio_func[FUNC_1]);
	sdio_set_block_size(p_data->sdio_func[FUNC_1],
			    SDIOHAL_BLK_SIZE);
	p_data->sdio_func[FUNC_1]->max_blksize = SDIOHAL_BLK_SIZE;
	sdio_release_host(p_data->sdio_func[FUNC_1]);
	if (ret < 0) {
		sdiohal_err("enable func1 err!!! ret is %d\n", ret);
		return;
	}
	sdiohal_info("enable func1 ok\n");

	sdiohal_enable_slave_irq();
}
#endif

#ifdef CONFIG_HISI_BOARD
#define REG_BASE_CTRL __io_address(0xf8a20008)
void sdiohal_set_card_present(bool enable)
{
	u32 regval;

	sdiohal_info("%s enable:%d\n", __func__, enable);

	/*set card_detect low to detect card*/
	regval = readl(REG_BASE_CTRL);
	if (enable)
		regval |= 0x1;
	else
		regval &= ~0x1;
	writel(regval, REG_BASE_CTRL);
}
#endif

static int sdiohal_probe(struct sdio_func *func,
	const struct sdio_device_id *id)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret;
	struct mmc_host *host = func->card->host;

	sdiohal_info("%s: func->class=%x, vendor=0x%04x, device=0x%04x, "
		     "func_num=0x%04x, clock=%d\n",
		     __func__, func->class, func->vendor, func->device,
		     func->num, host->ios.clock);

#ifdef CONFIG_AML_BOARD
	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
		/* disable auto clock, sdio clock will be always on. */
		sdio_clk_always_on(1);
	}
	/*
	 * setting sdio max request size to 512kB
	 * to improve transmission efficiency.
	 */
	sdio_set_max_reqsz(0x80000);
#endif

	ret = sdiohal_get_dev_func(func);
	if (ret < 0) {
		sdiohal_err("get func err\n");
		return ret;
	}

	sdiohal_debug("get func ok:0x%p card:0x%p host_mmc:0x%p\n",
		      p_data->sdio_func[FUNC_1],
		      p_data->sdio_func[FUNC_1]->card,
		      p_data->sdio_func[FUNC_1]->card->host);
	p_data->sdio_dev_host = p_data->sdio_func[FUNC_1]->card->host;
	if (p_data->sdio_dev_host == NULL) {
		sdiohal_err("get host failed!!!");
		return -1;
	}
	sdiohal_debug("get host ok!!!");

	atomic_set(&p_data->xmit_start, 1);

	if (!p_data->pwrseq) {
		/* Enable Function 1 */
		sdio_claim_host(p_data->sdio_func[FUNC_1]);
		ret = sdio_enable_func(p_data->sdio_func[FUNC_1]);
		sdio_set_block_size(p_data->sdio_func[FUNC_1],
				    SDIOHAL_BLK_SIZE);
		p_data->sdio_func[FUNC_1]->max_blksize = SDIOHAL_BLK_SIZE;
		sdio_release_host(p_data->sdio_func[FUNC_1]);
		if (ret < 0) {
			sdiohal_err("enable func1 err!!! ret is %d\n", ret);
			return ret;
		}
		sdiohal_debug("enable func1 ok\n");

		sdiohal_enable_slave_irq();
	} else
		pm_runtime_put_noidle(&func->dev);

	if (!WCN_CARD_EXIST(&p_data->xmit_cnt))
		atomic_sub(SDIOHAL_REMOVE_CARD_VAL, &p_data->xmit_cnt);

	p_data->card_dump_flag = false;

	sdiohal_set_cp_pin_status();

	if (p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) {
		ret = request_irq(p_data->irq_num, sdiohal_irq_handler,
				  p_data->irq_trigger_type | IRQF_NO_SUSPEND,
				  "sdiohal_irq", &func->dev);
		if (ret != 0) {
			sdiohal_err("request irq err gpio is %d\n",
				    p_data->irq_num);
			return ret;
		}

		disable_irq(p_data->irq_num);
	}
	complete(&p_data->scan_done);

	/* the card is nonremovable */
	p_data->sdio_dev_host->caps |= MMC_CAP_NONREMOVABLE;
#ifdef CONFIG_RK_BOARD
	/* Some RK platform, if config caps with MMC_CAP_SDIO_IRQ, will set
	 * caps2 with MMC_CAP2_SDIO_IRQ_NOTHREAD at the same time.
	 * This is unexpected. So clear this status.
	 */
	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ)
		p_data->sdio_dev_host->caps2 &= ~MMC_CAP2_SDIO_IRQ_NOTHREAD;
#endif

	/* calling rescan callback to inform download */
	if (scan_card_notify != NULL)
		scan_card_notify();

	sdiohal_debug("rescan callback:%p\n", scan_card_notify);
	sdiohal_info("probe ok\n");

	return 0;
}

static void sdiohal_remove(struct sdio_func *func)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	sdiohal_info("[%s]enter\n", __func__);

#ifdef CONFIG_HISI_BOARD
	sdiohal_set_card_present(0);
#endif

	if (WCN_CARD_EXIST(&p_data->xmit_cnt))
		atomic_add(SDIOHAL_REMOVE_CARD_VAL, &p_data->xmit_cnt);

	complete(&p_data->remove_done);

	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
		sdio_claim_host(p_data->sdio_func[FUNC_1]);
		sdio_release_irq(p_data->sdio_func[FUNC_1]);
		sdio_release_host(p_data->sdio_func[FUNC_1]);
	} else if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
		(p_data->irq_num > 0))
		free_irq(p_data->irq_num, &func->dev);
}

static void sdiohal_launch_thread(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	p_data->tx_thread =
		kthread_create(sdiohal_tx_thread, NULL, "sdiohal_tx_thread");
	if (p_data->tx_thread)
		wake_up_process(p_data->tx_thread);
	else {
		sdiohal_err("create sdiohal_tx_thread fail\n");
		return;
	}

	p_data->rx_thread =
	    kthread_create(sdiohal_rx_thread, NULL, "sdiohal_rx_thread");
	if (p_data->rx_thread)
		wake_up_process(p_data->rx_thread);
	else
		sdiohal_err("creat sdiohal_rx_thread fail\n");
}

static void sdiohal_stop_thread(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	sdiohal_info("[%s]enter\n", __func__);
	atomic_set(&p_data->flag_resume, 1);
	p_data->exit_flag = 1;
	if (p_data->tx_thread) {
		sdiohal_tx_up();
		kthread_stop(p_data->tx_thread);
		p_data->tx_thread = NULL;
	}
	if (p_data->rx_thread) {
		sdiohal_rx_up();
		kthread_stop(p_data->rx_thread);
		p_data->rx_thread = NULL;
	}
}

static const struct dev_pm_ops sdiohal_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdiohal_suspend, sdiohal_resume)
};

static const struct sdio_device_id sdiohal_ids[] = {
	{SDIO_DEVICE(0, 0)},
	{},
};

static struct sdio_driver sdiohal_driver = {
	.probe = sdiohal_probe,
	.remove = sdiohal_remove,
	.name = "sdiohal",
	.id_table = sdiohal_ids,
	.drv = {
		.pm = &sdiohal_pm_ops,
	},
};

#define WCN_SDIO_CARD_REMOVED	BIT(4)
void sdiohal_remove_card(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
#ifdef CONFIG_AW_BOARD
//	int wlan_bus_index = sunxi_wlan_get_bus_index();
	/* don't need to remove sdio card. */
	return;
#endif

#ifdef CONFIG_AML_BOARD
	/* As for amlogic platform, don't need to remove sdio card. */
	return;
#endif

	if (!WCN_CARD_EXIST(&p_data->xmit_cnt))
		return;

	atomic_add(SDIOHAL_REMOVE_CARD_VAL, &p_data->xmit_cnt);
	sdiohal_lock_scan_ws();
	sdiohal_resume_check();
	while (atomic_read(&p_data->xmit_cnt) > SDIOHAL_REMOVE_CARD_VAL)
		usleep_range(4000, 6000);

	init_completion(&p_data->remove_done);

#ifdef CONFIG_HISI_BOARD
	sdiohal_set_card_present(0);
#endif

#ifdef CONFIG_RK_BOARD
	//rockchip_wifi_set_carddetect(0);
	return ;
#endif

#ifdef CONFIG_AW_BOARD
//	sunxi_mmc_rescan_card(wlan_bus_index);
	return;
#endif

	p_data->sdio_dev_host->card->state |= WCN_SDIO_CARD_REMOVED;

	/* enable remove the card */
	p_data->sdio_dev_host->caps &= ~MMC_CAP_NONREMOVABLE;

	if (wait_for_completion_timeout(&p_data->remove_done,
					msecs_to_jiffies(5000)) == 0)
		sdiohal_err("remove card time out\n");
	else
		sdiohal_info("remove card end\n");

	sdio_unregister_driver(&sdiohal_driver);
	sdiohal_unlock_scan_ws();
}

int sdiohal_scan_card(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	int ret = 0;
#ifdef CONFIG_AML_BOARD
	struct sdio_func *func = p_data->sdio_func[FUNC_1];
#endif
#if 0
	int wlan_bus_index;
#endif

	sdiohal_info("sdiohal_scan_card\n");

#ifdef CONFIG_AML_BOARD
	if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ)
		sdio_clk_always_on(0);
	/* As for amlogic platform, Not remove sdio card.
	 * When system is booting up, amlogic platform will power
	 * up and get wifi module sdio id to know which vendor.
	 * Then power down. In order to not rescan sdio card,
	 * reset and reinit sdio host and slave is needed.
	 */
	sdio_reinit();
#endif

	if (WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		sdiohal_info("Already exist card!, xmit_cnt=0x%x\n",
			     atomic_read(&p_data->xmit_cnt));
#ifdef CONFIG_AML_BOARD
		/* As for amlogic platform, Not remove sdio card.
		 * But will reset sdio host, sdio slave need to be reset.
		 * If reset pin NC, don't need to reset sdio slave.
		 */
		if (p_data->sdio_dev_host != NULL)
			mmc_power_restore_host(p_data->sdio_dev_host);
		if (p_data->irq_type == SDIOHAL_RX_INBAND_IRQ) {
			/* disable auto clock, sdio clock will be always on. */
			sdio_clk_always_on(1);
		}
		/*
		 * setting sdio max request size to 512kB
		 * to improve transmission efficiency.
		 */
		sdio_set_max_reqsz(0x80000);

		if (!p_data->pwrseq) {
			/* Enable Function 1 */
			sdio_claim_host(p_data->sdio_func[FUNC_1]);
			ret = sdio_enable_func(p_data->sdio_func[FUNC_1]);
			sdio_set_block_size(p_data->sdio_func[FUNC_1],
					SDIOHAL_BLK_SIZE);
			p_data->sdio_func[FUNC_1]->max_blksize =
				SDIOHAL_BLK_SIZE;
			sdio_release_host(p_data->sdio_func[FUNC_1]);
			if (ret < 0) {
				sdiohal_err("enable func1 err!!! ret is %d\n",
					    ret);
				return ret;
			}
			sdiohal_info("enable func1 ok\n");
		} else
			pm_runtime_put_noidle(&func->dev);
		/* calling rescan callback to inform download */
		if (scan_card_notify != NULL)
			scan_card_notify();
		sdiohal_info("scan end!\n");
		return 0;
#endif

		sdiohal_remove_card();
		msleep(100);
	}

#ifdef CONFIG_HISI_BOARD
	/* only for hisi mv300 scan card mechanism */
	sdiohal_set_card_present(1);
#endif

#ifdef CONFIG_RK_BOARD
	//rockchip_wifi_set_carddetect(1);
#endif

#if 0
	wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0) {
		ret = wlan_bus_index;
		sdiohal_err("%s sunxi_wlan_get_bus_index=%d err!",
			    __func__, ret);
		return ret;
	}
	sunxi_mmc_rescan_card(wlan_bus_index);
#endif

	sdiohal_lock_scan_ws();
	sdiohal_resume_check();
	init_completion(&p_data->scan_done);
	ret = sdio_register_driver(&sdiohal_driver);
	if (ret != 0) {
		sdiohal_err("sdio_register_driver error :%d\n", ret);
		return ret;
	}
	if (wait_for_completion_timeout(&p_data->scan_done,
		msecs_to_jiffies(2500)) == 0) {
		sdiohal_unlock_scan_ws();
		sdio_unregister_driver(&sdiohal_driver);
		sdiohal_err("wait scan card time out\n");
		return -ENODEV;
	}
	if (!p_data->sdio_dev_host) {
		sdiohal_unlock_scan_ws();
		sdio_unregister_driver(&sdiohal_driver);
		sdiohal_err("sdio_dev_host is NULL!\n");
		return -ENODEV;
	}

	sdiohal_unlock_scan_ws();
	sdiohal_info("scan end!\n");

	return ret;
}

void sdiohal_register_scan_notify(void *func)
{
	scan_card_notify = func;
}

int sdiohal_init(void)
{
	struct sdiohal_data_t *p_data;
	int ret = 0;

	sdiohal_debug("sdiohal_init entry\n");

	p_data = kzalloc(sizeof(struct sdiohal_data_t), GFP_KERNEL);
	if (!p_data) {
		WARN_ON(1);
		return -ENOMEM;
	}
	p_data->printlog_txchn = SDIO_CHANNEL_NUM;
	p_data->printlog_rxchn = SDIO_CHANNEL_NUM;
	/* card not ready */
	atomic_set(&p_data->xmit_cnt, SDIOHAL_REMOVE_CARD_VAL);
	sdiohal_data = p_data;

	if (sdiohal_parse_dt() < 0)
		return -1;

	ret = sdiohal_misc_init();
	if (ret != 0) {
		sdiohal_err("sdiohal_misc_init error :%d\n", ret);
		return -1;
	}

	sdiohal_launch_thread();
	if (p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ)
		sdiohal_host_irq_init(p_data->gpio_num);
	p_data->flag_init = true;

#ifdef CONFIG_DEBUG_FS
#ifndef USB_SDIO_DT
	sdiohal_debug_init();
#endif
#endif
	sdiohal_info("sdiohal_init ok\n");

	return 0;
}

void sdiohal_exit(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	sdiohal_info("sdiohal_exit entry\n");

	p_data->flag_init = false;
#ifdef CONFIG_DEBUG_FS
	sdiohal_debug_deinit();
#endif
	if (WCN_CARD_EXIST(&p_data->xmit_cnt)) {
		sdiohal_info("Already exist card!\n");
		sdiohal_remove_card();
	}
	if ((p_data->irq_type == SDIOHAL_RX_EXTERNAL_IRQ) &&
		(p_data->irq_num > 0))
		gpio_free(p_data->gpio_num);
	sdiohal_stop_thread();
	sdiohal_misc_deinit();
	if (sdiohal_data) {
		sdiohal_data->sdio_dev_host = NULL;
		kfree(sdiohal_data);
		sdiohal_data = NULL;
	}

	sdiohal_info("sdiohal_exit ok\n");
}

