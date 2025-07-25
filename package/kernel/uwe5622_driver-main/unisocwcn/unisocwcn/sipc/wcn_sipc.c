/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <wcn_bus.h>

#include "wcn_integrate.h"
#include "wcn_sipc.h"
#include "wcn_txrx.h"

#define SIPC_WCN_DST 3

#define SIPC_TYPE_SBUF 0
#define SIPC_TYPE_SBLOCK 1

#define SIPC_CHN_ATCMD 4
#define SIPC_CHN_LOOPCHECK 11
#define SIPC_CHN_ASSERT 12
#define SIPC_CHN_LOG 5
#define SIPC_CHN_BT 4
#define SIPC_CHN_FM 4
#define SIPC_CHN_WIFI_CMD 7
#define SIPC_CHN_WIFI_DATA0 8
#define SIPC_CHN_WIFI_DATA1 9

#define INIT_SIPC_CHN_SBUF(idx, channel, bid,\
		 blen, bnum, tbsize, rbsize)\
{.index = idx, .chntype = SIPC_TYPE_SBUF,\
.chn = channel, .dst = SIPC_WCN_DST, .sbuf.bufid = bid,\
.sbuf.len = blen, .sbuf.bufnum = bnum,\
.sbuf.txbufsize = tbsize, .sbuf.rxbufsize = rbsize}

#define INIT_SIPC_CHN_SBLOCK(idx, channel,\
		 tbnum, tbsize, rbnum, rbsize)\
{.index = idx, .chntype = SIPC_TYPE_SBLOCK,\
.chn = channel, .dst = SIPC_WCN_DST,\
.sblk.txblocknum = tbnum, .sblk.txblocksize = tbsize,\
.sblk.rxblocknum = rbnum, .sblk.rxblocksize = rbsize}

static struct wcn_sipc_info_t g_sipc_info = {0};

/* sipc channel info */
static struct sipc_chn_info g_sipc_chn[SIPC_CHN_NUM] = {
	INIT_SIPC_CHN_SBUF(SIPC_ATCMD_TX, SIPC_CHN_ATCMD,
		5, 128, 0, 0x2400, 0x2400),
	INIT_SIPC_CHN_SBUF(SIPC_ATCMD_RX, SIPC_CHN_ATCMD,
		5, 128, 0, 0x2400, 0x2400),
	INIT_SIPC_CHN_SBUF(SIPC_LOOPCHECK_RX, SIPC_CHN_LOOPCHECK,
		0, 128, 1, 0x400, 0x400),
	INIT_SIPC_CHN_SBUF(SIPC_ASSERT_RX, SIPC_CHN_ASSERT,
		0, 1024, 1, 0x400, 0x400),
	INIT_SIPC_CHN_SBUF(SIPC_LOG_RX, SIPC_CHN_LOG,
		0, 8*1024, 1, 0x8000, 0x30000),
	INIT_SIPC_CHN_SBUF(SIPC_BT_TX, SIPC_CHN_BT,
		11, 4096, 0, 0x2400, 0x2400),
	INIT_SIPC_CHN_SBUF(SIPC_BT_RX, SIPC_CHN_BT,
		10, 4096, 0, 0x2400, 0x2400),
	INIT_SIPC_CHN_SBUF(SIPC_FM_TX, SIPC_CHN_FM,
		14, 128, 0, 0x2400, 0x2400),
	INIT_SIPC_CHN_SBUF(SIPC_FM_RX, SIPC_CHN_FM,
		13, 128, 0, 0x2400, 0x2400),
	INIT_SIPC_CHN_SBLOCK(SIPC_WIFI_CMD_TX, SIPC_CHN_WIFI_CMD,
		4, 2048, 16, 2048),
	INIT_SIPC_CHN_SBLOCK(SIPC_WIFI_CMD_RX, SIPC_CHN_WIFI_CMD,
		4, 2048, 16, 2048),
	INIT_SIPC_CHN_SBLOCK(SIPC_WIFI_DATA0_TX, SIPC_CHN_WIFI_DATA0,
		64, 1664, 256, 1664),
	INIT_SIPC_CHN_SBLOCK(SIPC_WIFI_DATA0_RX, SIPC_CHN_WIFI_DATA0,
		64, 1664, 256, 1664),
	INIT_SIPC_CHN_SBLOCK(SIPC_WIFI_DATA1_TX, SIPC_CHN_WIFI_DATA1,
		64, 1664, 0, 0),
	INIT_SIPC_CHN_SBLOCK(SIPC_WIFI_DATA1_RX, SIPC_CHN_WIFI_DATA1,
		64, 1664, 0, 0),
};

#define SIPC_INVALID_CHN(index) ((index >= SIPC_CHN_NUM) ? 1 : 0)
#define SIPC_TYPE(index) (g_sipc_chn[index].chntype)
#define SIPC_CHN(index) (&g_sipc_chn[index])

static inline char *sipc_chn_tostr(int chn, int bufid)
{
	switch (chn) {
	case SIPC_CHN_ATCMD:
		if (bufid == 5)
			return "ATCMD";
		else if (bufid == 10 || bufid == 11)
			return "BT";
		else if (bufid == 13 || bufid == 14)
			return "FM";
	case SIPC_CHN_LOG:
		return "LOG";
	case SIPC_CHN_LOOPCHECK:
		return "LOOPCHECK";
	case SIPC_CHN_ASSERT:
		return "ASSERT";
	case SIPC_CHN_WIFI_CMD:
		return "WIFICMD";
	case SIPC_CHN_WIFI_DATA0:
		return "WIFIDATA0";
	case SIPC_CHN_WIFI_DATA1:
		return "WIFIDATA1";
	default:
		return "Unknown Channel";
	}
}

static inline int wcn_sipc_buf_list_alloc(int chn,
	struct mbuf_t **head, struct mbuf_t **tail, int *num)
{
	return buf_list_alloc(chn, head, tail, num);
}

static inline int wcn_sipc_buf_list_free(int chn,
	struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	return buf_list_free(chn, head, tail, num);
}

static int wcn_sipc_recv(struct sipc_chn_info *sipc_chn, void *buf, int len)
{
	struct mbuf_t *head, *tail;
	struct mchn_ops_t *wcn_sipc_ops = NULL;

	wcn_sipc_ops = chn_ops(sipc_chn->index);
	if (unlikely(!wcn_sipc_ops))
		return -E_NULLPOINT;

	head = kzalloc(sizeof(struct mbuf_t), GFP_KERNEL);
	if (unlikely(!head))
		return -E_NOMEM;

	head->buf = buf;
	head->len = len;
	head->next = NULL;
	tail = head;
	wcn_sipc_ops->pop_link(sipc_chn->index, head, tail, 1);

	return 0;
}

static int wcn_sipc_sbuf_write(u8 index, void *buf, int len)
{
	int cnt = -1;
	struct sipc_chn_info *sipc_chn;

	if (SIPC_INVALID_CHN(index))
		return -E_INVALIDPARA;
	sipc_chn = SIPC_CHN(index);
	cnt = sbuf_write(sipc_chn->dst, sipc_chn->chn,
			sipc_chn->sbuf.bufid, buf + PUB_HEAD_RSV, len, -1);
	WCN_INFO("sbuf chn[%s] write cnt=%d\n",
		sipc_chn_tostr(sipc_chn->chn, sipc_chn->sbuf.bufid), cnt);

	return cnt;
}

static void wcn_sipc_sbuf_notifer(int event, void *data)
{
	int cnt = -1;
	int ret = -1;
	u8 *buf;
	struct bus_puh_t *puh = NULL;
	struct sipc_chn_info *sipc_chn = (struct sipc_chn_info *)data;

	if (unlikely(!sipc_chn))
		return;

	switch (event) {
	case SBUF_NOTIFY_WRITE:
		break;
	case SBUF_NOTIFY_READ:
		buf = kzalloc(sipc_chn->sbuf.len + PUB_HEAD_RSV, GFP_KERNEL);
		if (unlikely(!buf)) {
			WCN_ERR("[%s]:mem alloc fail!\n", __func__);
			return;
		}
		cnt = sbuf_read(sipc_chn->dst,
				sipc_chn->chn,
				sipc_chn->sbuf.bufid,
				(void *)(buf + PUB_HEAD_RSV),
				sipc_chn->sbuf.len, 0);
		puh = (struct bus_puh_t *)buf;
		puh->len = cnt;
		WCN_DEBUG("sbuf chn[%s] read cnt=%d\n",
				sipc_chn_tostr(sipc_chn->chn, 0), cnt);
		if (cnt < 0) {
			WCN_ERR("sbuf read cnt[%d] invalid\n", cnt);
			kfree(buf);
			return;
		}
		ret = wcn_sipc_recv(sipc_chn, buf, cnt);
		if (ret < 0) {
			WCN_ERR("sbuf recv fail[%d]\n", ret);
			kfree(buf);
			return;
		}
		break;
	default:
		WCN_ERR("sbuf read event[%d] invalid\n", event);
	}
}

static int wcn_sipc_sblk_write(u8 index, void *buf, int len)
{
	int ret = -1;
	u8 *addr = NULL;
	struct sblock blk;
	struct sipc_chn_info *sipc_chn;

	if (SIPC_INVALID_CHN(index))
		return -E_INVALIDPARA;
	sipc_chn = SIPC_CHN(index);
	/* Get a free swcnblk. */
	ret = sblock_get(sipc_chn->dst, sipc_chn->chn, &blk, 0);
	if (ret) {
		WCN_ERR("[%s]:Failed to get free swcnblk(%d)!\n",
		       sipc_chn_tostr(sipc_chn->chn, 0), ret);
		return -ENOMEM;
	}
	if (blk.length < len) {
		WCN_ERR("[%s]:The size of swcnblk is so tiny!\n",
		       sipc_chn_tostr(sipc_chn->chn, 0));
		sblock_put(sipc_chn->dst, sipc_chn->chn, &blk);
		return E_INVALIDPARA;
	}
	addr = (u8 *)blk.addr + SIPC_SBLOCK_HEAD_RESERV;
	blk.length = len + SIPC_SBLOCK_HEAD_RESERV;
	memcpy(((u8 *)addr), buf, len);
	ret = sblock_send(sipc_chn->dst, sipc_chn->chn, &blk);
	if (ret) {
		WCN_ERR("[%s]:err:%d\n",
		       sipc_chn_tostr(sipc_chn->chn, 0), ret);
		sblock_put(sipc_chn->dst, sipc_chn->chn, &blk);
	}

	return ret;
}

static void wcn_sipc_sblk_recv(struct sipc_chn_info *sipc_chn)
{
	u32 length = 0;
	int ret = -1;
	struct sblock blk;

	WCN_DEBUG("[%s]:recv sblock msg",
		       sipc_chn_tostr(sipc_chn->chn, 0));

	while (!sblock_receive(sipc_chn->dst, sipc_chn->chn, &blk, 0)) {
		length = blk.length - SIPC_SBLOCK_HEAD_RESERV;
		WCN_DEBUG("sblk length %d", length);
		wcn_sipc_recv(sipc_chn,
			(u8 *)blk.addr + SIPC_SBLOCK_HEAD_RESERV, length);
		ret = sblock_release(sipc_chn->dst, sipc_chn->chn, &blk);
		if (ret)
			WCN_ERR("release swcnblk[%d] err:%d\n",
			       sipc_chn->chn, ret);
	}
}

static void wcn_sipc_sblk_notifer(int event, void *data)
{
	struct sipc_chn_info *sipc_chn = (struct sipc_chn_info *)data;

	if (unlikely(!sipc_chn))
		return;
	switch (event) {
	case SBLOCK_NOTIFY_RECV:
		wcn_sipc_sblk_recv(sipc_chn);
		break;
	/* SBLOCK_NOTIFY_GET cmd not need process it */
	case SBLOCK_NOTIFY_GET:
		break;
	default:
		WCN_ERR("Invalid event swcnblk notify:%d\n", event);
		break;
	}
}

struct wcn_sipc_data_ops  sipc_data_ops[] = {
	{
		.sipc_write = wcn_sipc_sbuf_write,
		.sipc_notifer = wcn_sipc_sbuf_notifer,
	},
	{
		.sipc_write = wcn_sipc_sblk_write,
		.sipc_notifer = wcn_sipc_sblk_notifer,
	},
};

static int wcn_sipc_buf_push(int index, struct mbuf_t *head,
			  struct mbuf_t *tail, int num)
{
	struct mchn_ops_t *wcn_sipc_ops = NULL;

	wcn_sipc_ops = chn_ops(index);
	if (unlikely(!wcn_sipc_ops))
		return -E_NULLPOINT;

	if (wcn_sipc_ops->inout == WCNBUS_TX) {
		sipc_data_ops[SIPC_TYPE(index)].sipc_write(
					index, (void *)(head->buf), head->len);
		wcn_sipc_ops->pop_link(index, head, tail, num);
	} else if (wcn_sipc_ops->inout == WCNBUS_RX) {
		/* free buf mem */
		if (SIPC_TYPE(index) == SIPC_TYPE_SBUF)
			kfree(head->buf);
		/* free buf head */
		kfree(head);
	} else
		return -E_INVALIDPARA;

	return 0;
}

static inline unsigned int wcn_sipc_get_status(void)
{
	return g_sipc_info.sipc_chn_status;
}

static inline void wcn_sipc_set_status(unsigned int flag)
{
	mutex_lock(&g_sipc_info.status_lock);
	g_sipc_info.sipc_chn_status = flag;
	mutex_unlock(&g_sipc_info.status_lock);
}

static unsigned long long wcn_sipc_get_rxcnt(void)
{
	return wcn_get_cp2_comm_rx_count();
}

static int wcn_sipc_chn_init(struct mchn_ops_t *ops)
{
	int ret = -1;
	u8 chntype = 0;
	struct sipc_chn_info *sipc_chn;

	if (SIPC_INVALID_CHN(ops->channel))
		return -E_INVALIDPARA;
	sipc_chn = SIPC_CHN(ops->channel);
	WCN_INFO("[%s]:index[%d] chn[%d]\n",
				__func__,
				ops->channel,
				sipc_chn->chn);

	chntype = sipc_chn->chntype;
	if (chntype == SIPC_TYPE_SBUF) {
		/* sbuf */
		WCN_DEBUG("bufid[%d] len[%d] bufnum[%d]\n",
				sipc_chn->sbuf.bufid,
				sipc_chn->sbuf.len,
				sipc_chn->sbuf.bufnum);
		if (sipc_chn->sbuf.bufnum) {
			ret = sbuf_create(sipc_chn->dst, sipc_chn->chn,
					sipc_chn->sbuf.bufnum,
					sipc_chn->sbuf.txbufsize,
					sipc_chn->sbuf.rxbufsize);
			if (ret < 0) {
				WCN_ERR("sbuf chn[%d] create fail!\n",
					ops->channel);
				return ret;
			}
		}
		if (ops->inout == WCNBUS_RX) {
			ret = sbuf_register_notifier(
					sipc_chn->dst,
					sipc_chn->chn,
					sipc_chn->sbuf.bufid,
					sipc_data_ops[chntype].sipc_notifer,
					sipc_chn);
			if (ret < 0) {
				WCN_ERR("sbuf chn[%d] registerfail!\n",
						ops->channel);
				return ret;
			}
		}
		WCN_INFO("sbuf chn[%d] create success!\n", ops->channel);
	} else if (chntype == SIPC_TYPE_SBLOCK) {
		WCN_DEBUG("tbnum[%d] tbsz[%d] rbnum[%d] rbsz[%d]\n",
					sipc_chn->sblk.txblocknum,
					sipc_chn->sblk.txblocksize,
					sipc_chn->sblk.rxblocknum,
					sipc_chn->sblk.rxblocksize);
		/* sblock */
		if (ops->inout == WCNBUS_TX) {
			ret = sblock_create(
					sipc_chn->dst,
					sipc_chn->chn,
					sipc_chn->sblk.txblocknum,
					sipc_chn->sblk.txblocksize,
					sipc_chn->sblk.rxblocknum,
					sipc_chn->sblk.rxblocksize);
			if (ret < 0) {
				WCN_ERR("sblock chn[%d] create fail!\n",
					ops->channel);
				return ret;
			}
		}
		if (ops->inout == WCNBUS_RX) {
			ret = sblock_register_notifier(
					sipc_chn->dst,
					sipc_chn->chn,
					sipc_data_ops[chntype].sipc_notifer,
					sipc_chn);
			if (ret < 0) {
				WCN_ERR("sblock chn[%d] register fail!\n",
					ops->channel);
				sblock_destroy(sipc_chn->dst, sipc_chn->chn);
				return ret;
			}
		}
		WCN_INFO("sblock chn[%d] create success!\n", ops->channel);
	} else {
		WCN_ERR("invalid sipc type!");
		return -E_INVALIDPARA;
	}

	bus_chn_init(ops, HW_TYPE_SIPC);

	return ret;
}

static int wcn_sipc_chn_deinit(struct mchn_ops_t *ops)
{
	struct sipc_chn_info *sipc_chn;

	bus_chn_deinit(ops);

	if (SIPC_INVALID_CHN(ops->channel))
		return -E_INVALIDPARA;
	sipc_chn = SIPC_CHN(ops->channel);
	/* sbuf */
	if (sipc_chn->chntype == SIPC_TYPE_SBUF) {
		if (sipc_chn->sbuf.bufnum)
			sbuf_destroy(sipc_chn->dst, sipc_chn->chn);
	} else if (sipc_chn->chntype == SIPC_TYPE_SBLOCK) {
		if (ops->inout == WCNBUS_TX)
			sblock_destroy(sipc_chn->dst, sipc_chn->chn);
	}
	WCN_INFO("sipc chn[%d] deinit success!\n", ops->channel);

	return 0;
}

static void wcn_sipc_module_init(void)
{
	mutex_init(&g_sipc_info.status_lock);
	WCN_INFO("sipc module init success\n");
}

static void wcn_sipc_module_deinit(void)
{
	mutex_destroy(&g_sipc_info.status_lock);
	WCN_INFO("sipc module deinit success\n");
}

static struct sprdwcn_bus_ops sipc_bus_ops = {
	.chn_init = wcn_sipc_chn_init,
	.chn_deinit = wcn_sipc_chn_deinit,
	.list_alloc = wcn_sipc_buf_list_alloc,
	.list_free = wcn_sipc_buf_list_free,
	.push_list = wcn_sipc_buf_push,
	.get_carddump_status = wcn_sipc_get_status,
	.set_carddump_status = wcn_sipc_set_status,
	.get_rx_total_cnt = wcn_sipc_get_rxcnt,

};

void module_bus_init(void)
{
	wcn_sipc_module_init();
	module_ops_register(&sipc_bus_ops);
	WCN_INFO("sipc bus init success\n");
}
EXPORT_SYMBOL(module_bus_init);

void module_bus_deinit(void)
{
	module_ops_unregister();
	wcn_sipc_module_deinit();
	WCN_INFO("sipc bus deinit success\n");
}
EXPORT_SYMBOL(module_bus_deinit);
