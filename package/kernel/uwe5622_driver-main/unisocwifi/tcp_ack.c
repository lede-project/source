#include <uapi/linux/if_ether.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/in.h>
#include <linux/moduleparam.h>
#include <net/tcp.h>

#include "sprdwl.h"
#include "intf_ops.h"
#include "tcp_ack.h"


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static void sprdwl_tcp_ack_timeout(struct timer_list *t)
#else
static void sprdwl_tcp_ack_timeout(unsigned long data)
#endif
{
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_msg_buf *msg;
	struct sprdwl_tcp_ack_manage *ack_m = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	ack_info = (struct sprdwl_tcp_ack_info *)from_timer(ack_info, t, timer);
#else
	ack_info = (struct sprdwl_tcp_ack_info *)data;
#endif
	ack_m = container_of(ack_info, struct sprdwl_tcp_ack_manage,
				 ack_info[ack_info->ack_info_num]);

	write_seqlock_bh(&ack_info->seqlock);
	msg = ack_info->msgbuf;
	if (ack_info->busy && msg && !ack_info->in_send_msg) {
		ack_info->msgbuf = NULL;
		ack_info->drop_cnt = 0;
		ack_info->in_send_msg = msg;
		write_sequnlock_bh(&ack_info->seqlock);
		sprdwl_intf_tx(ack_m->priv, msg);
		return;
	}
	write_sequnlock_bh(&ack_info->seqlock);
}

void sprdwl_tcp_ack_init(struct sprdwl_priv *priv)
{
	int i;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_manage *ack_m = &priv->ack_m;

	memset(ack_m, 0, sizeof(struct sprdwl_tcp_ack_manage));
	ack_m->priv = priv;
	spin_lock_init(&ack_m->lock);
	atomic_set(&ack_m->max_drop_cnt, SPRDWL_TCP_ACK_DROP_CNT);
	ack_m->last_time = jiffies;
	ack_m->timeout = msecs_to_jiffies(SPRDWL_ACK_OLD_TIME);

	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
		ack_info = &ack_m->ack_info[i];
		ack_info->ack_info_num = i;
		seqlock_init(&ack_info->seqlock);
		ack_info->last_time = jiffies;
		ack_info->timeout = msecs_to_jiffies(SPRDWL_ACK_OLD_TIME);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
		timer_setup(&ack_info->timer, sprdwl_tcp_ack_timeout, 0);
#else
		setup_timer(&ack_info->timer, sprdwl_tcp_ack_timeout,
				(unsigned long)ack_info);
#endif
	}

	atomic_set(&ack_m->enable, 1);
	ack_m->ack_winsize = MIN_WIN;
}

void sprdwl_tcp_ack_deinit(struct sprdwl_priv *priv)
{
	int i;
	struct sprdwl_tcp_ack_manage *ack_m = &priv->ack_m;
	struct sprdwl_msg_buf *drop_msg = NULL;

	atomic_set(&ack_m->enable, 0);

	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
		drop_msg = NULL;

		write_seqlock_bh(&ack_m->ack_info[i].seqlock);
		del_timer(&ack_m->ack_info[i].timer);
		drop_msg = ack_m->ack_info[i].msgbuf;
		ack_m->ack_info[i].msgbuf = NULL;
		write_sequnlock_bh(&ack_m->ack_info[i].seqlock);

		if (drop_msg)
			sprdwl_intf_tcp_drop_msg(priv, drop_msg);
	}
}

static int sprdwl_tcp_check_quick_ack(unsigned char *buf,
					  struct sprdwl_tcp_ack_msg *msg)
{
	int ip_hdr_len;
	unsigned char *temp;
	struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;

	ethhdr = (struct ethhdr *)buf;
	if (ethhdr->h_proto != htons(ETH_P_IP))
		return 0;
	iphdr = (struct iphdr *)(ethhdr + 1);
	if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
		return 0;
	ip_hdr_len = iphdr->ihl * 4;
	temp = (unsigned char *)(iphdr) + ip_hdr_len;
	tcphdr = (struct tcphdr *)temp;
	/* TCP_FLAG_ACK */
	if (!(temp[13] & 0x10))
		return 0;

	if (temp[13] & 0x8) {
		msg->saddr = iphdr->daddr;
		msg->daddr = iphdr->saddr;
		msg->source = tcphdr->dest;
		msg->dest = tcphdr->source;
		msg->seq = ntohl(tcphdr->seq);
		return 1;
	}

	return 0;
}

static int is_drop_tcp_ack(struct tcphdr *tcphdr, int tcp_tot_len,
				unsigned short *win_scale)
{
	int drop = 1;
	int len = tcphdr->doff * 4;
	unsigned char *ptr;

	if (tcp_tot_len > len) {
		drop = 0;
	} else {
		len -= sizeof(struct tcphdr);
		ptr = (unsigned char *)(tcphdr + 1);

		while ((len > 0) && drop) {
			int opcode = *ptr++;
			int opsize;

			switch (opcode) {
			case TCPOPT_EOL:
				break;
			case TCPOPT_NOP:
				len--;
				continue;
			default:
				opsize = *ptr++;
				if (opsize < 2)
					break;
				if (opsize > len)
					break;

				switch (opcode) {
				/* TODO: Add other ignore opt */
				case TCPOPT_TIMESTAMP:
					break;
				case TCPOPT_WINDOW:
					if (*ptr < 15)
						*win_scale = (1 << (*ptr));
					break;
				default:
					drop = 2;
				}

				ptr += opsize - 2;
				len -= opsize;
			}
		}
	}

	return drop;
}

/* flag:0 for not tcp ack
 *	1 for ack which can be drop
 *	2 for other ack whith more info
 */
static int sprdwl_tcp_check_ack(unsigned char *buf,
				struct sprdwl_tcp_ack_msg *msg,
				unsigned short *win_scale)
{
	int ret;
	int ip_hdr_len;
	int tcp_tot_len;
	unsigned char *temp;
	struct ethhdr *ethhdr;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;

	ethhdr = (struct ethhdr *)buf;
	if (ethhdr->h_proto != htons(ETH_P_IP))
		return 0;
	iphdr = (struct iphdr *)(ethhdr + 1);
	if (iphdr->version != 4 || iphdr->protocol != IPPROTO_TCP)
		return 0;
	ip_hdr_len = iphdr->ihl * 4;
	temp = (unsigned char *)(iphdr) + ip_hdr_len;
	tcphdr = (struct tcphdr *)temp;
	/* TCP_FLAG_ACK */
	if (!(temp[13] & 0x10))
		return 0;

	tcp_tot_len = ntohs(iphdr->tot_len) - ip_hdr_len;
	ret = is_drop_tcp_ack(tcphdr, tcp_tot_len, win_scale);

	if (ret > 0) {
		msg->saddr = iphdr->saddr;
		msg->daddr = iphdr->daddr;
		msg->source = tcphdr->source;
		msg->dest = tcphdr->dest;
		msg->seq = ntohl(tcphdr->ack_seq);
		msg->win = ntohs(tcphdr->window);
	}

	return ret;
}

/* return val: -1 for not match, others for match */
static int sprdwl_tcp_ack_match(struct sprdwl_tcp_ack_manage *ack_m,
				struct sprdwl_tcp_ack_msg *ack_msg)
{
	int i, ret = -1;
	unsigned start;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_msg *ack;

	for (i = 0; ((ret < 0) && (i < SPRDWL_TCP_ACK_NUM)); i++) {
		ack_info = &ack_m->ack_info[i];
		do {
			start = read_seqbegin(&ack_info->seqlock);
			ret = -1;

			ack = &ack_info->ack_msg;
			if (ack_info->busy &&
				ack->dest == ack_msg->dest &&
				ack->source == ack_msg->source &&
				ack->saddr == ack_msg->saddr &&
				ack->daddr == ack_msg->daddr)
				ret = i;
		} while (read_seqretry(&ack_info->seqlock, start));
	}

	return ret;
}

static void sprdwl_tcp_ack_update(struct sprdwl_tcp_ack_manage *ack_m)
{
	int i;
	struct sprdwl_tcp_ack_info *ack_info;

	if (time_after(jiffies, ack_m->last_time + ack_m->timeout)) {
		spin_lock_bh(&ack_m->lock);
		ack_m->last_time = jiffies;
		for (i = SPRDWL_TCP_ACK_NUM - 1; i >= 0; i--) {
			ack_info = &ack_m->ack_info[i];
			write_seqlock_bh(&ack_info->seqlock);
			if (ack_info->busy &&
				time_after(jiffies, ack_info->last_time +
					   ack_info->timeout)) {
				ack_m->free_index = i;
				ack_m->max_num--;
				ack_info->busy = 0;
			}
			write_sequnlock_bh(&ack_info->seqlock);
		}
		spin_unlock_bh(&ack_m->lock);
	}
}

/* return val: -1 for no index, others for index */
static int sprdwl_tcp_ack_alloc_index(struct sprdwl_tcp_ack_manage *ack_m)
{
	int i, ret = -1;
	struct sprdwl_tcp_ack_info *ack_info;
	unsigned start;

	spin_lock_bh(&ack_m->lock);
	if (ack_m->max_num == SPRDWL_TCP_ACK_NUM) {
		spin_unlock_bh(&ack_m->lock);
		return -1;
	}

	if (ack_m->free_index >= 0) {
		i = ack_m->free_index;
		ack_m->free_index = -1;
		ack_m->max_num++;
		spin_unlock_bh(&ack_m->lock);
		return i;
	}

	for (i = 0; ((ret < 0) && (i < SPRDWL_TCP_ACK_NUM)); i++) {
		ack_info = &ack_m->ack_info[i];
		do {
			start = read_seqbegin(&ack_info->seqlock);
			ret = -1;
			if (!ack_info->busy) {
				ack_m->free_index = -1;
				ack_m->max_num++;
				ret = i;
			}
		} while (read_seqretry(&ack_info->seqlock, start));
	}
	spin_unlock_bh(&ack_m->lock);

	return ret;
}

/* return val: 0 for not handle tx, 1 for handle tx */
int sprdwl_tcp_ack_handle(struct sprdwl_msg_buf *new_msgbuf,
			  struct sprdwl_tcp_ack_manage *ack_m,
			  struct sprdwl_tcp_ack_info *ack_info,
			  struct sprdwl_tcp_ack_msg *ack_msg,
			  int type)
{
	int quick_ack = 0;
	struct sprdwl_tcp_ack_msg *ack;
	int ret = 0;
	struct sprdwl_msg_buf *drop_msg = NULL;

	write_seqlock_bh(&ack_info->seqlock);

	ack_info->last_time = jiffies;
	ack = &ack_info->ack_msg;

	if (type == 2) {
		if (SPRDWL_U32_BEFORE(ack->seq, ack_msg->seq)) {
			ack->seq = ack_msg->seq;
			if (ack_info->psh_flag &&
				!SPRDWL_U32_BEFORE(ack_msg->seq,
						   ack_info->psh_seq)) {
				ack_info->psh_flag = 0;
			}

			if (ack_info->msgbuf) {
				drop_msg = ack_info->msgbuf;
				ack_info->msgbuf = NULL;
				del_timer(&ack_info->timer);
			}

			ack_info->in_send_msg = NULL;
			ack_info->drop_cnt = atomic_read(&ack_m->max_drop_cnt);
		} else {
			wl_err("%s before abnormal ack: %d, %d\n",
				   __func__, ack->seq, ack_msg->seq);
			drop_msg = new_msgbuf;
			ret = 1;
		}
	} else if (SPRDWL_U32_BEFORE(ack->seq, ack_msg->seq)) {
		if (ack_info->msgbuf) {
			drop_msg = ack_info->msgbuf;
			ack_info->msgbuf = NULL;
		}

		if (ack_info->psh_flag &&
			!SPRDWL_U32_BEFORE(ack_msg->seq, ack_info->psh_seq)) {
			ack_info->psh_flag = 0;
			quick_ack = 1;
		} else {
			ack_info->drop_cnt++;
		}

		ack->seq = ack_msg->seq;

		if (quick_ack || (!ack_info->in_send_msg &&
				  (ack_info->drop_cnt >=
				   atomic_read(&ack_m->max_drop_cnt)))) {
			ack_info->drop_cnt = 0;
			ack_info->in_send_msg = new_msgbuf;
			del_timer(&ack_info->timer);
		} else {
			ret = 1;
			ack_info->msgbuf = new_msgbuf;
			if (!timer_pending(&ack_info->timer))
				mod_timer(&ack_info->timer,
					  (jiffies + msecs_to_jiffies(5)));
		}
	} else {
		wl_err("%s before ack: %d, %d\n",
			   __func__, ack->seq, ack_msg->seq);
		drop_msg = new_msgbuf;
		ret = 1;
	}

	write_sequnlock_bh(&ack_info->seqlock);

	if (drop_msg)
		sprdwl_intf_tcp_drop_msg(ack_m->priv, drop_msg);

	return ret;
}

void sprdwl_filter_rx_tcp_ack(struct sprdwl_priv *priv,
				  unsigned char *buf, unsigned plen)
{
	int index;
	struct sprdwl_tcp_ack_msg ack_msg;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_manage *ack_m = &priv->ack_m;

	if (!atomic_read(&ack_m->enable))
		return;

	if ((plen > MAX_TCP_ACK) ||
		!sprdwl_tcp_check_quick_ack(buf, &ack_msg))
		return;

	index = sprdwl_tcp_ack_match(ack_m, &ack_msg);
	if (index >= 0) {
		ack_info = ack_m->ack_info + index;
		write_seqlock_bh(&ack_info->seqlock);
		ack_info->psh_flag = 1;
		ack_info->psh_seq = ack_msg.seq;
		write_sequnlock_bh(&ack_info->seqlock);
	}
}

/* return val: 0 for not filter, 1 for filter */
int sprdwl_filter_send_tcp_ack(struct sprdwl_priv *priv,
				   struct sprdwl_msg_buf *msgbuf,
				   unsigned char *buf, unsigned int plen)
{
	int ret = 0;
	int index, drop;
	unsigned short win_scale = 0;
	unsigned int win = 0;
	struct sprdwl_tcp_ack_msg ack_msg;
	struct sprdwl_tcp_ack_msg *ack;
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_manage *ack_m = &priv->ack_m;

	if (!atomic_read(&ack_m->enable))
		return 0;

	if (plen > MAX_TCP_ACK)
		return 0;

	sprdwl_tcp_ack_update(ack_m);
	drop = sprdwl_tcp_check_ack(buf, &ack_msg, &win_scale);
	if (!drop && (0 == win_scale))
		return 0;

	index = sprdwl_tcp_ack_match(ack_m, &ack_msg);
	if (index >= 0) {
		ack_info = ack_m->ack_info + index;
		if ((0 != win_scale) &&
			(ack_info->win_scale != win_scale)) {
			write_seqlock_bh(&ack_info->seqlock);
			ack_info->win_scale = win_scale;
			write_sequnlock_bh(&ack_info->seqlock);
		}

		if (drop > 0) {
			win = ack_info->win_scale * ack_msg.win;
			if (win < (ack_m->ack_winsize * SIZE_KB))
				drop = 2;

			ret = sprdwl_tcp_ack_handle(msgbuf, ack_m, ack_info,
						&ack_msg, drop);
		}

		goto out;
	}

	index = sprdwl_tcp_ack_alloc_index(ack_m);
	if (index >= 0) {
		write_seqlock_bh(&ack_m->ack_info[index].seqlock);
		ack_m->ack_info[index].busy = 1;
		ack_m->ack_info[index].psh_flag = 0;
		ack_m->ack_info[index].last_time = jiffies;
		ack_m->ack_info[index].drop_cnt =
			atomic_read(&ack_m->max_drop_cnt);
		ack_m->ack_info[index].win_scale =
			(win_scale != 0) ? win_scale : 1;

		ack = &ack_m->ack_info[index].ack_msg;
		ack->dest = ack_msg.dest;
		ack->source = ack_msg.source;
		ack->saddr = ack_msg.saddr;
		ack->daddr = ack_msg.daddr;
		ack->seq = ack_msg.seq;
		write_sequnlock_bh(&ack_m->ack_info[index].seqlock);
	}

out:
	return ret;
}

void sprdwl_move_tcpack_msg(struct sprdwl_priv *priv,
				struct sprdwl_msg_buf *msg)
{
	struct sprdwl_tcp_ack_info *ack_info;
	struct sprdwl_tcp_ack_manage *ack_m = &priv->ack_m;
	int i = 0;

	if (!atomic_read(&ack_m->enable))
		return;

	if (msg->len > MAX_TCP_ACK)
		return;

	for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
		ack_info = &ack_m->ack_info[i];
		write_seqlock_bh(&ack_info->seqlock);
		if (ack_info->busy && (ack_info->in_send_msg == msg))
			ack_info->in_send_msg = NULL;
		write_sequnlock_bh(&ack_info->seqlock);
	}
}

extern struct sprdwl_priv *g_sprdwl_priv;

void enable_tcp_ack_delay(char *buf, unsigned char offset)
{
	int enable = buf[offset] - '0';
	int i;
	struct sprdwl_tcp_ack_manage *ack_m = NULL;
	struct sprdwl_msg_buf *drop_msg = NULL;

	if (!g_sprdwl_priv)
		return ;

	ack_m = &g_sprdwl_priv->ack_m;

	if (enable == 0) {
		atomic_set(&ack_m->enable, 0);
		for (i = 0; i < SPRDWL_TCP_ACK_NUM; i++) {
			drop_msg = NULL;

			write_seqlock_bh(&ack_m->ack_info[i].seqlock);
			drop_msg = ack_m->ack_info[i].msgbuf;
			ack_m->ack_info[i].msgbuf = NULL;
			del_timer(&ack_m->ack_info[i].timer);
			write_sequnlock_bh(&ack_m->ack_info[i].seqlock);

			if (drop_msg)
				sprdwl_intf_tcp_drop_msg(g_sprdwl_priv,
							 drop_msg);
		}
	} else {
		atomic_set(&ack_m->enable, 1);
	}
}

void adjust_tcp_ack_delay(char *buf, unsigned char offset)
{
#define MAX_LEN 2
	unsigned int cnt = 0;
	unsigned int i = 0;
	struct sprdwl_tcp_ack_manage *ack_m = NULL;

	if (!g_sprdwl_priv)
		return ;

	for (i = 0; i < MAX_LEN; (cnt *= 10), i++) {

		if ((buf[offset + i] >= '0') &&
		   (buf[offset + i] <= '9')) {
			cnt += (buf[offset + i] - '0');
		} else {
			cnt /= 10;
			break;
		}
	}

	ack_m = &g_sprdwl_priv->ack_m;
	wl_err("cnt: %d\n", cnt);

	if (cnt >= 100)
		cnt = SPRDWL_TCP_ACK_DROP_CNT;

	atomic_set(&ack_m->max_drop_cnt, cnt);
	wl_err("drop time: %d, atomic drop time: %d\n", cnt, atomic_read(&ack_m->max_drop_cnt));
#undef MAX_LEN
}

void adjust_tcp_ack_delay_win(char *buf, unsigned char offset)
{
	unsigned int value = 0;
	unsigned int i = 0;
	unsigned int len = strlen(buf) - strlen("tcpack_delay_win=");
	struct sprdwl_tcp_ack_manage *ack_m = NULL;

	if (!g_sprdwl_priv)
		return;

	for (i = 0; i < len; (value *= 10), i++) {
		if ((buf[offset + i] >= '0') &&
		   (buf[offset + i] <= '9')) {
			value += (buf[offset + i] - '0');
		} else {
			value /= 10;
			break;
		}
	}
	ack_m = &g_sprdwl_priv->ack_m;
	ack_m->ack_winsize = value;
	wl_err("%s, change tcpack_delay_win to %dKB\n", __func__, value);
}
