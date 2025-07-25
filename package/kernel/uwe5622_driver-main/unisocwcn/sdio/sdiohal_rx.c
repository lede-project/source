#include "sdiohal.h"

static unsigned int sdiohal_rx_adapt_get(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->dtbs;
}

static void sdiohal_rx_adapt_set_dtbs(unsigned int len)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned int off;

	if (len == 0) {
		p_data->dtbs = MAX_PAC_SIZE;
		return;
	}

	off = (len >> 10) + 1;
	len = SDIOHAL_ALIGN_BLK(len + 8 * off + 64);
#if (BITS_PER_LONG > 32) || (PAGE_SIZE >= 65536)
	p_data->dtbs = (len >= SDIOHAL_RX_RECVBUF_LEN) ?
			SDIOHAL_RX_RECVBUF_LEN : len;
#else
	p_data->dtbs = (len >= SDIOHAL_32_BIT_RX_RECVBUF_LEN) ?
			SDIOHAL_32_BIT_RX_RECVBUF_LEN : len;
#endif
}

static unsigned int sdiohal_rx_adapt_get_pac_num(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	return p_data->remain_pac_num;
}

static void sdiohal_rx_adapt_set_pac_num(unsigned int num)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	if (num == 0) {
		p_data->remain_pac_num = 1;
		return;
	}

	p_data->remain_pac_num =
		(num >= MAX_CHAIN_NODE_NUM) ? MAX_CHAIN_NODE_NUM : num;
}

static int sdiohal_data_list_assignment(struct mbuf_t *mbuf_node,
	struct sdio_puh_t *puh, int channel)
{
	struct sdiohal_list_t *rx_channel_list;

	rx_channel_list = sdiohal_get_rx_channel_list(channel);
	if (rx_channel_list->node_num == 0)
		rx_channel_list->mbuf_head = mbuf_node;
	else
		rx_channel_list->mbuf_tail->next = mbuf_node;

	mbuf_node->next = NULL;
	rx_channel_list->mbuf_tail = mbuf_node;
	rx_channel_list->type = puh->type;
	rx_channel_list->subtype = puh->subtype;
	rx_channel_list->node_num++;

	return 0;
}

/* for adma */
static int sdiohal_rx_list_parser(struct sdiohal_list_t *data_list,
				int valid_len)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct sdio_puh_t *puh = NULL;
	struct mbuf_t *mbuf_node, *mbuf_next;
	unsigned int node_num, i;
	int inout = 0, channel = 0;
	unsigned int parse_len;

	sdiohal_list_check(data_list, __func__, SDIOHAL_READ);

	node_num = data_list->node_num;
	mbuf_next = data_list->mbuf_head;
	parse_len = 0;
	for (i = 0; i < node_num; i++) {
		mbuf_node = mbuf_next;
		mbuf_next = mbuf_next->next;
		puh = (struct sdio_puh_t *)mbuf_node->buf;
		if ((puh->eof == 0) && (puh->type != 0xF)) {
			channel = sdiohal_hwtype_to_channel(inout,
				puh->type, puh->subtype);
			parse_len += puh->len;
			if (puh->check_sum)
				puh->len += 2;
			if ((channel >= SDIO_CHANNEL_NUM) || (puh->len >
				(MAX_PAC_SIZE - SDIO_PUB_HEADER_SIZE)) ||
				(puh->len == 0)) {
				sdiohal_rx_list_free(mbuf_node, mbuf_node, 1);
				sdiohal_err("%s skip type[%d]sub[%d]len[%d]\n",
						__func__, puh->type, puh->subtype,
						puh->len);
				continue;
			}
			p_data->rx_packer_cnt++;
			mbuf_node->len = MAX_MBUF_SIZE;
			sdiohal_data_list_assignment(mbuf_node, puh, channel);
		} else {
			sdiohal_debug("%s eof pac:%d,parse[%d]%s valid[%d]\n",
					  __func__, puh->eof, parse_len,
					  (parse_len < valid_len ? "<":">="),
					  valid_len);
			sdiohal_debug("%s type[%d]sub[%d]len[%d]\n", __func__,
					  puh->type, puh->subtype, puh->len);

			sdiohal_rx_list_free(mbuf_node, mbuf_node, 1);
		}
	}

	return 0;
}

/* for normal dma */
static int sdiohal_rx_buf_parser(char *data_buf, int valid_len)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct sdio_puh_t *puh = NULL;
	struct sdiohal_list_t *data_list = NULL;
	int inout = 0, channel;
	unsigned char *p = NULL;
	unsigned int parse_len;

	puh = (struct sdio_puh_t *)data_buf;
	for (parse_len = 0; parse_len < valid_len;) {
		if (puh->eof != 0)
			break;

		p = (unsigned char *)puh;
		if (puh->type != 0xF) {
			channel = sdiohal_hwtype_to_channel(inout,
				puh->type, puh->subtype);
			parse_len += puh->len;
			if (puh->check_sum)
				puh->len += 2;
			if ((channel >= SDIO_CHANNEL_NUM) || (puh->len >
				(MAX_PAC_SIZE - SDIO_PUB_HEADER_SIZE)) ||
				(puh->len == 0)) {
				sdiohal_err("%s skip type[%d]sub[%d]len[%d]\n",
						__func__, puh->type, puh->subtype,
						puh->len);
				continue;
			}
			p_data->rx_packer_cnt++;

			data_list = sdiohal_get_rx_mbuf_node(1);
			if (!data_list)
				return -ENOMEM;
			data_list->mbuf_head = data_list->mbuf_tail;
			data_list->mbuf_head->buf = (unsigned char *)puh;
			data_list->mbuf_head->len = puh->len;
			data_list->node_num = 1;
			p_data->frag_ctl.pagecnt_bias--;

			sdiohal_list_check(data_list, __func__, SDIOHAL_READ);
			sdiohal_print_list_data(channel, data_list, __func__,
						SDIOHAL_DATA_LEVEL);

			sdiohal_data_list_assignment(data_list->mbuf_head,
							 puh, channel);
			kfree(data_list);
		}
		/* pointer to next packet */
		p += sizeof(struct sdio_puh_t)
			+ SDIOHAL_ALIGN_4BYTE(puh->len);
		puh = (struct sdio_puh_t *)p;
	}

	return 0;
}

static void sdiohal_rx_wait(void)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();

	while (1) {
		msleep(300);
		if (WCN_CARD_EXIST(&p_data->xmit_cnt))
			break;
	}
}

int sdiohal_rx_thread(void *data)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct sched_param param;
	int read_len, mbuf_num;
	int ret = 0;
	unsigned int rx_dtbs = 0;
	unsigned int valid_len = 0;
	static char *rx_buf;
	struct sdiohal_list_t *data_list = NULL;
	struct timespec tm_begin, tm_end;
	static long time_total_ns;
	static int times_count;

	param.sched_priority = SDIO_RX_TASK_PRIO;
	sched_setscheduler(current, SCHED_FIFO, &param);
	sdiohal_rx_adapt_set_dtbs(0);
	sdiohal_rx_adapt_set_pac_num(1);

	while (1) {
		/* Wait the semaphore */
		if (p_data->irq_type == SDIOHAL_RX_POLLING)
			sdiohal_rx_wait();
		else
			sdiohal_rx_down();
		if (p_data->exit_flag)
			break;
		if (!WCN_CARD_EXIST(&p_data->xmit_cnt)) {
			sdiohal_err("%s line %d not have card\n",
					__func__, __LINE__);
			continue;
		}

		getnstimeofday(&p_data->tm_end_irq);
		sdiohal_pr_perf("rx sch time:%ld\n",
				(long)(timespec_to_ns(&p_data->tm_end_irq)
				- timespec_to_ns(&p_data->tm_begin_irq)));

		sdiohal_resume_wait();
		sdiohal_cp_rx_wakeup(PACKER_RX);

read_again:
		getnstimeofday(&tm_begin);

		if (p_data->adma_rx_enable) {
			/* read len is packet num */
			mbuf_num = sdiohal_rx_adapt_get_pac_num();
			sdiohal_debug("%s mbuf_num:%d adma_rx_enable:%d\n",
					  __func__, mbuf_num,
					  p_data->adma_rx_enable);

			data_list = sdiohal_get_rx_mbuf_list(mbuf_num);
			if (!data_list) {
				sdiohal_err("sdiohal_get_rx_mbuf_list fail\n");
				msleep(100);
				goto submit_list;
			}
			if (p_data->irq_type == SDIOHAL_RX_POLLING)
				memset(p_data->dtbs_buf, 0x0,
					   SDIOHAL_DTBS_BUF_SIZE);
			ret = sdiohal_adma_pt_read(data_list);
			if (ret != 0) {
				sdiohal_err("adma read fail ret:%d\n", ret);
				rx_dtbs = 0;
				if (p_data->irq_type != SDIOHAL_RX_POLLING) {
					sdiohal_rx_list_free(
							data_list->mbuf_head,
							data_list->mbuf_tail,
							data_list->node_num);
					kfree(data_list);
					data_list = NULL;
					goto submit_list;
				}
			}
			rx_dtbs =  *((unsigned int *)(p_data->dtbs_buf
				   + (SDIOHAL_DTBS_BUF_SIZE - 4)));
			valid_len = *((unsigned int *)(p_data->dtbs_buf
					+ (SDIOHAL_DTBS_BUF_SIZE - 8)));
			sdiohal_debug("%s rx_pac_num:%d, valid len:%d\n",
					  __func__, rx_dtbs, valid_len);
			sdiohal_rx_list_parser(data_list, valid_len);
			kfree(data_list);
			data_list = NULL;
		} else {
			unsigned int alloc_size;

			/* read len is packet data len */
			read_len = sdiohal_rx_adapt_get();
			sdiohal_debug("%s read_len:%d adma_rx_enable:%d\n",
					  __func__, read_len,
					  p_data->adma_rx_enable);

			/*get buf by readlen 1024 aligned */
			rx_buf = sdiohal_get_rx_free_buf(&alloc_size, read_len);
			if (!rx_buf) {
				sdiohal_err("get_rx_free_buf fail, rlen=%d\n",
						read_len);
				msleep(100);
				goto submit_list;
			}
			if (alloc_size < read_len) {
				read_len = alloc_size;
				sdiohal_debug("alloc_size=%d < read_len=%d\n",
						  alloc_size, read_len);
			}

			ret = sdiohal_sdio_pt_read(rx_buf, read_len);
			if (ret != 0) {
				sdiohal_err("sdio pt read fail ret:%d\n", ret);
				rx_dtbs = 0;
				if (p_data->irq_type != SDIOHAL_RX_POLLING)
					goto submit_list;
			}
			rx_dtbs = *((unsigned int *)(rx_buf + (read_len - 4)));
			valid_len =
				*((unsigned int *)(rx_buf + (read_len - 8)));
			sdiohal_debug("%s rx_dtbs:%d,valid len:%d\n",
					  __func__, rx_dtbs, valid_len);
			sdiohal_rx_buf_parser(rx_buf, valid_len);
		}

submit_list:
		getnstimeofday(&tm_end);
		time_total_ns += timespec_to_ns(&tm_end)
			- timespec_to_ns(&tm_begin);
		times_count++;
		if (!(times_count % PERFORMANCE_COUNT)) {
			sdiohal_pr_perf("rx list avg time:%ld\n",
					(time_total_ns / PERFORMANCE_COUNT));
			time_total_ns = 0;
			times_count = 0;
		}

		sdiohal_rx_list_dispatch();
		if (p_data->adma_rx_enable)
			sdiohal_rx_adapt_set_pac_num(rx_dtbs);
		else
			sdiohal_rx_adapt_set_dtbs(rx_dtbs);
		if (rx_dtbs > 0)
			goto read_again;

		sdiohal_cp_rx_sleep(PACKER_RX);
		sdiohal_unlock_rx_ws();
		if (p_data->irq_type != SDIOHAL_RX_POLLING)
			sdiohal_enable_rx_irq();
	}

	return 0;
}
