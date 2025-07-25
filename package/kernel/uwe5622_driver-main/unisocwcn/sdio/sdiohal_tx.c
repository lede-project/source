#include "sdiohal.h"

#define SDIOHAL_TX_RETRY_MAX 3
#define SDIOHAL_TX_NO_RETRY

static void sdiohal_tx_retrybuf_left(unsigned int suc_pac_cnt)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	unsigned int cnt = 0;
	struct sdio_puh_t *puh = NULL;
	unsigned char *p = NULL;

	if (suc_pac_cnt == 0)
		return;

	puh = (struct sdio_puh_t *)p_data->send_buf.retry_buf;
	for (cnt = 0; cnt < suc_pac_cnt;) {
		if (puh->eof == 0) {
			p = (unsigned char *)puh;
			cnt++;
			p_data->send_buf.retry_len =
				p_data->send_buf.retry_len -
				sizeof(struct sdio_puh_t) -
				SDIOHAL_ALIGN_4BYTE(puh->len);

			/* pointer to next packet */
			p += sizeof(struct sdio_puh_t)
				+ SDIOHAL_ALIGN_4BYTE(puh->len);
			puh = (struct sdio_puh_t *)p;
			p_data->send_buf.retry_buf = (unsigned char *)p;
		} else
			break;
	}

	sdiohal_debug("sdiohal_tx_retrybuf_left [%p] retry_len[%d]\n",
			  p_data->send_buf.retry_buf,
			  p_data->send_buf.retry_len);
}

static int sdiohal_send_try(struct sdiohal_sendbuf_t *send_buf)
{
	unsigned int tx_pac_cnt = 0;
	unsigned int try_cnt = 0;
	int ret = 0;

#ifdef SDIOHAL_TX_NO_RETRY
	return 0;
#endif

	sdiohal_tx_init_retrybuf();
try_send:
	try_cnt++;
	if (try_cnt < SDIOHAL_TX_RETRY_MAX) {
		tx_pac_cnt = sdiohal_get_trans_pac_num();

		/* get the buf ptr and length right now */
		sdiohal_tx_retrybuf_left(tx_pac_cnt);
		usleep_range(4000, 6000);

		ret = sdiohal_sdio_pt_write(send_buf->retry_buf,
			SDIOHAL_ALIGN_BLK(send_buf->retry_len));
		if (ret != 0)
			goto try_send;
	}

	return 0;
}

static int sdiohal_send(struct sdiohal_sendbuf_t *send_buf,
	struct sdiohal_list_t *data_list)
{
	int ret = 0;

	if ((!send_buf) || (!data_list))
		return -EINVAL;

	ret = sdiohal_sdio_pt_write(send_buf->buf,
		SDIOHAL_ALIGN_BLK(send_buf->used_len));
	if (ret != 0) {
		sdiohal_err("tyr send,type:%d subtype:%d node_num:%d\n",
				data_list->type, data_list->subtype,
				data_list->node_num);
		ret = sdiohal_send_try(send_buf);
	}

	return ret;
}

int sdiohal_tx_data_list_send(struct sdiohal_list_t *data_list,
				  bool pop_flag)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct mbuf_t *mbuf_node;
	unsigned int i;
	int ret = 0;

	sdiohal_sdma_enter();
	sdiohal_list_check(data_list, __func__, SDIOHAL_WRITE);
	mbuf_node = data_list->mbuf_head;
	for (i = 0; i < data_list->node_num;
		i++, mbuf_node = mbuf_node->next) {
		if (p_data->send_buf.used_len +
			sizeof(struct sdio_puh_t) +
			SDIOHAL_ALIGN_4BYTE(mbuf_node->len)
				> SDIOHAL_TX_SENDBUF_LEN) {
			sdiohal_tx_set_eof(&p_data->send_buf,
					   p_data->eof_buf);
			ret = sdiohal_send(&p_data->send_buf, data_list);
			if (ret)
				sdiohal_err("err1,type:%d subtype:%d num:%d\n",
					data_list->type, data_list->subtype,
					data_list->node_num);
			p_data->send_buf.used_len = 0;
		}
		sdiohal_tx_packer(&p_data->send_buf,
				  data_list, mbuf_node);
	}
	sdiohal_tx_set_eof(&p_data->send_buf, p_data->eof_buf);

	if (pop_flag == true)
		sdiohal_tx_list_denq(data_list);
	ret = sdiohal_send(&p_data->send_buf, data_list);
	if (ret)
		sdiohal_err("err2,type:%d subtype:%d num:%d\n",
			data_list->type, data_list->subtype,
			data_list->node_num);

	p_data->send_buf.used_len = 0;
	sdiohal_sdma_leave();

	return ret;
}

int sdiohal_tx_thread(void *data)
{
	struct sdiohal_data_t *p_data = sdiohal_get_data();
	struct sdiohal_list_t data_list;
	struct sched_param param;
	struct timespec tm_begin, tm_end;
	static long time_total_ns;
	static int times_count;

	param.sched_priority = SDIO_TX_TASK_PRIO;
	sched_setscheduler(current, SCHED_FIFO, &param);

	while (1) {
		/* Wait the semaphore */
		sdiohal_tx_down();
		if (p_data->exit_flag)
			break;
		if (!WCN_CARD_EXIST(&p_data->xmit_cnt)) {
			sdiohal_err("%s line %d not have card\n",
					__func__, __LINE__);
			continue;
		}

		getnstimeofday(&p_data->tm_end_sch);
		sdiohal_pr_perf("tx sch time:%ld\n",
			(long)(timespec_to_ns(&p_data->tm_end_sch)
			- timespec_to_ns(&p_data->tm_begin_sch)));
		sdiohal_lock_tx_ws();
		sdiohal_resume_wait();

		/* wakeup cp */
		sdiohal_cp_tx_wakeup(PACKER_TX);

		while (!sdiohal_is_tx_list_empty()) {
			getnstimeofday(&tm_begin);

			sdiohal_tx_find_data_list(&data_list);
			if (p_data->adma_tx_enable) {
				sdiohal_adma_pt_write(&data_list);
				sdiohal_tx_list_denq(&data_list);
			} else
				sdiohal_tx_data_list_send(&data_list, true);

			getnstimeofday(&tm_end);
			time_total_ns += timespec_to_ns(&tm_end)
				- timespec_to_ns(&tm_begin);
			times_count++;
			if (!(times_count % PERFORMANCE_COUNT)) {
				sdiohal_pr_perf("tx list avg time:%ld\n",
					(time_total_ns / PERFORMANCE_COUNT));
				time_total_ns = 0;
				times_count = 0;
			}
		}

		sdiohal_cp_tx_sleep(PACKER_TX);
		sdiohal_unlock_tx_ws();
	}

	return 0;
}

