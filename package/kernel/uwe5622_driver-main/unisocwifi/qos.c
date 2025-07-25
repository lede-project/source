/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * Authors	:
 * star.liu <star.liu@spreadtrum.com>
 * yifei.li <yifei.li@spreadtrum.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "qos.h"
#include "msg.h"
#include "sprdwl.h"

unsigned int g_qos_enable;
#if 0
/*initial array of dscp map to priority
 *map array will be changed by qos remap
 */
#define DSCP_MAX_VALUE 64
const unsigned char default_dscp2priomap[DSCP_MAX_VALUE] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 4, 0, 4, 0, 4, 0,
	4, 0, 4, 0, 4, 0, 4, 0,
	4, 0, 0, 0, 0, 0, 6, 0,
	6, 0, 0, 0, 0, 0, 0, 0,
	6, 0, 0, 0, 0, 0, 0, 0
};
#endif
#ifdef WMMAC_WFA_CERTIFICATION
bool g_wmmac_available[NUM_AC] = {false, false, false, false};
u32 g_wmmac_admittedtime[NUM_AC] = {0};
u32 g_wmmac_usedtime[NUM_AC] = {0};
struct wmm_ac_ts_t g_sta_ts_info[NUM_TID];
unsigned int wmmac_ratio = 10;
#endif

struct qos_map_set g_11u_qos_map;

void init_default_qos_map(void)
{
	u8 index;

	for (index = 0; index < QOS_MAP_MAX_DSCP_EXCEPTION; index++) {
		g_11u_qos_map.qos_exceptions[index].dscp = 0xFF;
		g_11u_qos_map.qos_exceptions[index].up = prio_0;
	}

	index = 0;
	g_11u_qos_map.qos_ranges[index].low = 0x0;	/*IP-PL0*/
	g_11u_qos_map.qos_ranges[index].high = 0x0;
	g_11u_qos_map.qos_ranges[index].up = prio_0;

	index++;
	g_11u_qos_map.qos_ranges[index].low = 0x3;	/*IP-PL3*/
	g_11u_qos_map.qos_ranges[index].high = 0x3;
	g_11u_qos_map.qos_ranges[index].up = prio_0;

	index++;
	g_11u_qos_map.qos_ranges[index].low = 0x1;	/*IP-PL1*/
	g_11u_qos_map.qos_ranges[index].high = 0x2;	/*IP-PL2*/
	g_11u_qos_map.qos_ranges[index].up = prio_1;

	index++;
	g_11u_qos_map.qos_ranges[index].low = 0x4;	/*IP-PL4*/
	g_11u_qos_map.qos_ranges[index].high = 0x5;	/*IP-PL5*/
	g_11u_qos_map.qos_ranges[index].up = prio_4;

	index++;
	g_11u_qos_map.qos_ranges[index].low = 0x6;	/*IP-PL6*/
	g_11u_qos_map.qos_ranges[index].high = 0x7;	/*IP-PL7*/
	g_11u_qos_map.qos_ranges[index].up = prio_6;

}

unsigned int pkt_get_prio(void *skb, int data_offset, unsigned char *tos)
{
	struct ether_header *eh;
	struct ethervlan_header *evh;
	unsigned char *pktdata;
	unsigned int priority = prio_6;

	pktdata = ((struct sk_buff *)(skb))->data + data_offset;
	eh = (struct ether_header *)pktdata;

	if (eh->ether_type == cpu_to_be16(ETHER_TYPE_8021Q)) {
		unsigned short vlan_tag;
		int vlan_prio;

		evh = (struct ethervlan_header *)eh;

		vlan_tag = be16_to_cpu(evh->vlan_tag);
		vlan_prio = (int)(vlan_tag >> VLAN_PRI_SHIFT) & VLAN_PRI_MASK;
		priority = vlan_prio;
	} else {
		unsigned char *ip_body = pktdata + sizeof(struct ether_header);
		unsigned char tos_tc = IP_TOS46(ip_body) & 0xE0;

		*tos = IP_TOS46(ip_body);
		switch (tos_tc) {
		case 0x00:
		case 0x60:
			priority = prio_0;/*BE*/
			break;
		case 0x20:
		case 0x40:
			priority = prio_1;/*BK*/
			break;
		case 0x80:
		case 0xA0:
			priority = prio_4;/*VI*/
			break;
		default:
			priority = prio_6;/*VO*/
			break;
		}
	}

	PKT_SET_PRIO(skb, priority);
	return priority;
}

static const u8 up_to_ac[] = {
	0,	/*SPRDWL_AC_BE*/
	1,	/*SPRDWL_AC_BK*/
	1,	/*SPRDWL_AC_BK*/
	0,	/*SPRDWL_AC_BE*/
	4,	/*SPRDWL_AC_VI*/
	4,	/*SPRDWL_AC_VI*/
	6,	/*SPRDWL_AC_VO*/
	6	/*SPRDWL_AC_VO*/
};

#if 0
/* up range from low to high with up value */
static bool
qos_up_table_set(u8 *dscp2up_table, u8 up, struct dscp_range *dscp_range)
{
	int i;

	if (up > 7 || dscp_range->low > dscp_range->high ||
		dscp_range->low >= DSCP_MAX_VALUE ||
		dscp_range->high >= DSCP_MAX_VALUE) {
		return false;
	}

	for (i = dscp_range->low; i <= dscp_range->high; i++)
		dscp2up_table[i] = up_to_ac[up];

	return true;
}

/* set user priority table */
void qos_set_dscp2up_table(unsigned char *dscp2up_table,
			   struct qos_capab_info *qos_map_ie)
{
	unsigned char len;
	struct dscp_range dscp_range;
	int i;
	struct dscp_range *range;
	unsigned char except_len;
	u8 *except_ptr;
	u8 *range_ptr;

	if (dscp2up_table == NULL || qos_map_ie == NULL)
		return;

	/* length of QoS Map IE must be 16+n*2, n is number of exceptions */
	if (qos_map_ie != NULL && qos_map_ie->id == DOT11_MNG_QOS_MAP_ID &&
			qos_map_ie->len >= QOS_MAP_FIXED_LENGTH &&
			(qos_map_ie->len % 2) == 0) {
		except_ptr = (u8 *)qos_map_ie->qos_info;
		len = qos_map_ie->len;
		except_len = len - QOS_MAP_FIXED_LENGTH;
		range_ptr = except_ptr + except_len;

		/* fill in ranges */
		for (i = 0; i < QOS_MAP_FIXED_LENGTH; i += 2) {
			range = (struct dscp_range *)(&range_ptr[i]);

			if (range->low == 255 && range->high == 255)
				continue;
			if (!qos_up_table_set(dscp2up_table, i / 2, range)) {
				/* reset the table on failure */
				memcpy(dscp2up_table, default_dscp2priomap,
					   DSCP_MAX_VALUE);
				return;
			}
		}

		/* update exceptions */
		for (i = 0; i < except_len; i += 2) {
			struct dscp_exception *exception =
				(struct dscp_exception *)(&except_ptr[i]);
			unsigned char dscp = exception->dscp;
			unsigned char up = exception->up;

			dscp_range.high = dscp;
			dscp_range.low = dscp;
			/* exceptions with invalid dscp/up are ignored */
			qos_up_table_set(dscp2up_table, up, &dscp_range);
		}
	}
	wl_hex_dump(L_DBG, "qos up table: ", DUMP_PREFIX_OFFSET,
				 16, 1, dscp2up_table, DSCP_MAX_VALUE, 0);
}

/*Todo*/
int fq_table[5][4] = {
	{0,  0,  0,  0},
	{30,  0,  0,  0},
	{30,  20,  0,  0},
	{40,  25,  20,  0},
	{40,  30,  20,  10}
};

/*Todo*/
int wfq_table[5][4] = {
	{0,  0,  0,  0},
	{10, 0,  0,  0},
	{20, 10, 0,  0},
	{30, 20, 10, 0},
	{40, 30, 20, 10}
};

/*Todo*/
int fd_special_table[2][2] = {
	{30, 10},
	{30, 10}
};

/*time slot ratio based on WFA spec*/
int fd_ratio_table[3] = {7, /*vo: 87%, vi:13%*/
				  9, /*vi:90%, be:10%*/
				  5};/*be:81%, bk:19%*/
#endif
void qos_enable(int flag)
{
	g_qos_enable = flag;
}

void qos_init(struct tx_t *tx_t_list)
{
	int i, j;

	/*tx_t_list->index = SPRDWL_AC_VO;*/
	for (i = 0; i < SPRDWL_AC_MAX; i++) {
		for (j = 0; j < MAX_LUT_NUM; j++) {
			INIT_LIST_HEAD(&tx_t_list->q_list[i].p_list[j].head_list);
			spin_lock_init(&tx_t_list->q_list[i].p_list[j].p_lock);
			atomic_set(&tx_t_list->q_list[i].p_list[j].l_num, 0);
		}
	}
#if 0
	if (tx_t_list->dscp2up_table == NULL) {
		tx_t_list->dscp2up_table = kzalloc(DSCP_MAX_VALUE, GFP_KERNEL);
		if (tx_t_list->dscp2up_table == NULL)
			wl_err("%s malloc dscp2up_table fail\n", __func__);
		else
			memcpy(tx_t_list->dscp2up_table, default_dscp2priomap,
				   DSCP_MAX_VALUE);
	}
#endif
}
#if 0
void qos_deinit(struct tx_t *qos)
{
	if (qos->dscp2up_table != NULL) {
		kfree(qos->dscp2up_table);
		qos->dscp2up_table = NULL;
	}
}

struct qos_capab_info *qos_parse_capab_info(void *buf, int buflen, uint key)
{
	struct qos_capab_info *capab_info;
	int totlen;

	capab_info = (struct qos_capab_info *)buf;
	if (capab_info == NULL)
		return NULL;

	totlen = buflen;

	/* find tagged parameter */
	while (totlen >= TLV_HDR_LEN) {
		int len = capab_info->len;

		/* validate remaining totlen */
		if ((capab_info->id == key) &&
		   (totlen >= (int)(len + TLV_HDR_LEN)))
			return capab_info;

		capab_info = (struct qos_capab_info *)
			((u8 *)capab_info + (len + TLV_HDR_LEN));
		totlen -= (len + TLV_HDR_LEN);
	}

	return NULL;
}
#endif
unsigned int qos_match_q(void *skb, int data_offset)
{
	int priority;
	struct ether_header *eh;
	qos_head_type_t data_type = SPRDWL_AC_BE;
	unsigned char tos = 0;

	if (0 == g_qos_enable)
		return SPRDWL_AC_BE;
	/* vo vi bk be*/
	eh =
	(struct ether_header *)(((struct sk_buff *)(skb))->data + data_offset);

	if (cpu_to_be16(ETHER_TYPE_IP) != eh->ether_type &&
	   cpu_to_be16(ETHER_TYPE_IPV6) != eh->ether_type) {
		goto OUT;
	}
	priority = pkt_get_prio(skb, data_offset, &tos);
	switch (priority) {
	case prio_1:
		data_type = SPRDWL_AC_BK;
		break;
	case prio_4:
		data_type = SPRDWL_AC_VI;
		break;
	case prio_6:
		data_type = SPRDWL_AC_VO;
		break;
	default:
		data_type = SPRDWL_AC_BE;
		break;
	}
OUT:
	/*return data_type as qos queue index*/
	return data_type;
}

unsigned int tid_map_to_qosindex(unsigned char tid)
{
	qos_head_type_t qos_index = SPRDWL_AC_BE;

	switch (tid) {
	case prio_1:
		qos_index = SPRDWL_AC_BK;
		break;
	case prio_4:
		qos_index = SPRDWL_AC_VI;
		break;
	case prio_6:
		qos_index = SPRDWL_AC_VO;
		break;
	default:
		qos_index = SPRDWL_AC_BE;
		break;
	}
	/*return data_type as qos queue index*/
	return qos_index;
}

unsigned int get_tid_qosindex(void *skb, int data_offset, unsigned char *tid, unsigned char *tos)
{
	int priority;
	struct ether_header *eh;

	if (0 == g_qos_enable)
		return SPRDWL_AC_BE;
	/* vo vi bk be*/
	eh =
	(struct ether_header *)(((struct sk_buff *)(skb))->data + data_offset);

	/*if (cpu_to_be16(ETHER_TYPE_IP) != eh->ether_type &&
	   cpu_to_be16(ETHER_TYPE_IPV6) != eh->ether_type) {
		goto OUT;
	}*/
	priority = pkt_get_prio(skb, data_offset, tos);
	*tid = priority;

	/*return data_type as qos queue index*/
	return tid_map_to_qosindex(*tid);
}
#if 0
void qos_wfq(struct tx_t *qos)
{
	int t, i, j, weight, q[4] = {0}, list_num[4] = {0, 0, 0, 0};

	for (i = 0, t = 0, weight = 0; i < 4; i++) {
		for (j = 0; j < MAX_LUT_NUM; j++)
			list_num[i] += get_list_num(&qos->q_list[i].p_list[j].head_list);
		if (list_num[i] > 0) {
			q[t] = i;
			t++;
		}
	}
	if (0 == t)
		return;
	for (i = 0; i < t; i++)
		weight += wfq_table[t][i];
	for (i = 0; i < t; i++)
		qos->going[q[i]] = wfq_table[t][i] *
				   list_num[i] / weight;
}

void qos_fq(struct tx_t *qos)
{
	int i, j, t, k, q[4] = {0}, list_num[4] = {0, 0, 0, 0};

	for (i = 0, t = 0; i < 4; i++) {
		for (j = 0; j < MAX_LUT_NUM; j++)
			list_num[i] += get_list_num(&qos->q_list[i].p_list[j].head_list);
		if (list_num[i] > 0) {
			q[t] = i;
			t++;
		}
	}
	if (0 == t)
		return;
	/* vi & bk*/
	if ((2 == t) && (1 == q[0]) && (2 == q[1])) {
		qos->going[SPRDWL_AC_VI] = fd_special_table[0][0];
		qos->going[SPRDWL_AC_BE] = fd_special_table[0][1];

		if (list_num[SPRDWL_AC_VI] < qos->going[1])
			qos->going[SPRDWL_AC_VI] =
				list_num[SPRDWL_AC_VI];
		if (list_num[SPRDWL_AC_BE] < qos->going[2])
			qos->going[SPRDWL_AC_BE] =
				list_num[SPRDWL_AC_BE];
		return;
	}
	/*bk & be*/
	if ((2 == t) && (2 == q[0]) && (3 == q[1])) {
		qos->going[2] = fd_special_table[1][0];
		qos->going[3] = fd_special_table[1][1];

		if (list_num[SPRDWL_AC_BE] < qos->going[2])
			qos->going[SPRDWL_AC_BE] =
				list_num[SPRDWL_AC_BE];
		if (list_num[SPRDWL_AC_BK] < qos->going[3])
			qos->going[SPRDWL_AC_BK] =
				list_num[SPRDWL_AC_BK];
		return;
	}

	for (i = 0; i < t; i++) {
		k = 0;
		qos->going[q[i]] = fq_table[t][i];
		if (list_num[q[i]]  < qos->going[q[i]]) {
			k = list_num[q[i]];
			qos->going[q[i]] = k;
		}
	}
}

/*get time slot ratio between higher priority stream and lower*/
int qos_fq_ratio(struct tx_t *qos)
{
	int i, j, t, q[4] = {0};

	for (i = 0, t = 0; i < 4; i++) {
		for (j = 0; j < MAX_LUT_NUM; j++) {
			if (!list_empty(&qos->q_list[i].p_list[j].head_list)) {
				q[t] = i;
				t++;
				break;
			}
		}
	}
	if (0 == t)
		return t;
	/*vi & vo, two streams coexist based on WFA spec*/
	if ((2 == t) && (0 == q[0]) && (1 == q[1])) {
		qos->ratio = fd_ratio_table[0];
		qos->ac_index = SPRDWL_AC_VO;
		return t;
	}
	/* vi & be*/
	if ((2 == t) && (1 == q[0]) && (2 == q[1])) {
		qos->ratio = fd_ratio_table[1];
		qos->ac_index = SPRDWL_AC_VI;
		return t;
	}
	/*be & bk*/
	if ((2 == t) && (2 == q[0]) && (3 == q[1])) {
		qos->ratio = fd_ratio_table[2];
		qos->ac_index = SPRDWL_AC_BE;
		return t;
	}
	/*ac_index indicate which two qos streams coexist*/
	qos->ac_index = SPRDWL_AC_MAX;
	qos->ratio = 0;
	return t;
}

void qos_sched(struct tx_t *qos, struct qos_list **q, int *num)
{
	int round, j;

	if (0 == g_qos_enable) {
		*q = &qos->q_list[SPRDWL_AC_BE];
		for (j = 0; j < MAX_LUT_NUM; j++)
			*num += get_list_num(&qos->q_list[SPRDWL_AC_BE].p_list[j].head_list);
		return;
	}

	for (round = 0;  round < 4; round++) {
		if ((SPRDWL_AC_VO == qos->index) &&
			(0 == qos->going[SPRDWL_AC_VO]))
			/*qos_fq(qos);*/
			qos_wfq(qos);
		if (qos->going[qos->index] > 0)
			break;
		qos->index = INCR_RING_BUFF_INDX(qos->index, 4);
	}
	*q = &qos->q_list[qos->index];
	*num = qos->going[qos->index];
}
#endif
int get_list_num(struct list_head *list)
{
	int num = 0;
	struct list_head *pos;
	struct list_head *n_list;

	if (list_empty(list))
		return 0;
	list_for_each_safe(pos, n_list, list)
		num++;
	return num;
}

#ifdef WMMAC_WFA_CERTIFICATION
/*init wmmac params, include timer and ac params*/
void wmm_ac_init(struct sprdwl_priv *priv)
{
	u8 ac;

	for (ac = 0; ac < NUM_AC; ac++) {
		g_wmmac_usedtime[ac] = 0;
		g_wmmac_available[ac] = false;
		g_wmmac_admittedtime[ac] = 0;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	timer_setup(&priv->wmmac.wmmac_edcaf_timer, update_wmmac_edcaftime_timeout, 0);
#else
	setup_timer(&priv->wmmac.wmmac_edcaf_timer, update_wmmac_edcaftime_timeout,
		    (unsigned long)priv);
#endif
	memset(&priv->wmmac.ac[0], 0, 4*sizeof(struct wmm_ac_params));
}
void reset_wmmac_parameters(struct sprdwl_priv *priv)
{
	u8 ac;

	for (ac = 0; ac < NUM_AC; ac++) {
		g_wmmac_usedtime[ac] = 0;
		g_wmmac_available[ac] = false;
		g_wmmac_admittedtime[ac] = 0;
	}
	if (timer_pending(&priv->wmmac.wmmac_edcaf_timer))
		del_timer_sync(&priv->wmmac.wmmac_edcaf_timer);

	memset(&priv->wmmac.ac[0], 0, 4*sizeof(struct wmm_ac_params));
}

void reset_wmmac_ts_info(void)
{
	u8 tsid;

	for (tsid = 0; tsid < NUM_TID; tsid++)
		remove_wmmac_ts_info(tsid);
}

unsigned int priority_map_to_qos_index(int priority)
{
	qos_head_type_t qos_index = SPRDWL_AC_BE;

	switch (up_to_ac[priority]) {
	case prio_1:
		qos_index = SPRDWL_AC_BK;
		break;
	case prio_4:
		qos_index = SPRDWL_AC_VI;
		break;
	case prio_6:
		qos_index = SPRDWL_AC_VO;
		break;
	default:
		qos_index = SPRDWL_AC_BE;
		break;
	}
	/*return data_type as qos queue index*/
	return qos_index;
}

unsigned int map_priority_to_edca_ac(int priority)
{
	int ac;

	switch (priority) {
	case 01:
	case 02:
		ac = AC_BK;
	break;

	case 04:
	case 05:
		ac = AC_VI;
	break;

	case 06:
	case 07:
		ac = AC_VO;
	break;

	case 00:
	case 03:
	default:
		ac = AC_BE;
	break;
	}
	/*return data_type as qos queue index*/
	return ac;
}

unsigned int map_edca_ac_to_priority(u8 ac)
{
	unsigned int priority;

	switch (ac) {
	case AC_BK:
		priority = prio_1;
	break;
	case AC_VI:
		priority = prio_4;
	break;
	case AC_VO:
		priority = prio_6;
	break;
	case AC_BE:
	default:
		priority = prio_0;
	break;
	}
	return priority;
}
void update_wmmac_ts_info(u8 tsid, u8 up, u8 ac, bool status, u16 admitted_time)
{
	g_sta_ts_info[tsid].exist = status;
	g_sta_ts_info[tsid].ac = ac;
	g_sta_ts_info[tsid].up = up;
	g_sta_ts_info[tsid].admitted_time = admitted_time;
}

u16 get_wmmac_admitted_time(u8 tsid)
{
	u16 value = 0;

	if (g_sta_ts_info[tsid].exist == true)
		value = g_sta_ts_info[tsid].admitted_time;

	return value;
}

void remove_wmmac_ts_info(u8 tsid)
{
	memset(&(g_sta_ts_info[tsid]), 0, sizeof(struct wmm_ac_ts_t));
}
void update_admitted_time(struct sprdwl_priv *priv, u8 tsid, u16 medium_time, bool increase)
{
	u8 ac = g_sta_ts_info[tsid].ac;

	if (true == increase) {
		/*mediumtime is in unit of 32 us, admittedtime is in unit of us*/
		g_wmmac_admittedtime[ac] += (medium_time<<5);
		mod_timer(&priv->wmmac.wmmac_edcaf_timer,
				jiffies + WMMAC_EDCA_TIMEOUT_MS * HZ / 1000);
	} else {
		if (g_wmmac_admittedtime[ac] > (medium_time<<5))
			g_wmmac_admittedtime[ac] -= (medium_time<<5);
		else {
			g_wmmac_admittedtime[ac] = 0;
			if (timer_pending(&priv->wmmac.wmmac_edcaf_timer))
				del_timer_sync(&priv->wmmac.wmmac_edcaf_timer);
		}
	}

	g_wmmac_available[ac] = (g_wmmac_usedtime[ac] < g_wmmac_admittedtime[ac]);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
void update_wmmac_edcaftime_timeout(struct timer_list *t)
{
	struct sprdwl_priv *priv = from_timer(priv, t, wmmac.wmmac_edcaf_timer);
#else
void update_wmmac_edcaftime_timeout(unsigned long data)
{
	struct sprdwl_priv *priv = (struct sprdwl_priv *)data;
#endif
	/*restart edcaf timer per second*/
	mod_timer(&priv->wmmac.wmmac_edcaf_timer, jiffies + WMMAC_EDCA_TIMEOUT_MS * HZ / 1000);

	if (g_wmmac_admittedtime[AC_VO] > 0) {
		g_wmmac_usedtime[AC_VO] = 0;
		g_wmmac_available[AC_VO] = true;
	}
	if (g_wmmac_admittedtime[AC_VI] > 0) {
		g_wmmac_usedtime[AC_VI] = 0;
		g_wmmac_available[AC_VI] = true;
	}
}

/*change priority according to the g_wmmac_available value */
unsigned int change_priority_if(struct sprdwl_priv *priv, unsigned char *tid, unsigned char *tos, u16 len)
{
	unsigned int qos_index, ac;
	int match_index = 0;
	unsigned char priority = *tos;

	if (1 == g_qos_enable) {
		ac = map_priority_to_edca_ac(*tid);
		while (ac != 0) {
			if (!!(priv->wmmac.ac[ac].aci_aifsn & WMM_AC_ACM)) {
				/*current ac is available, use it directly*/
				if (true == g_wmmac_available[ac]) {
					/* use wmmac_ratio to adjust ac used time */
					/* it is rough calc method: (data_len * 8) * ratio / data_rate, here , use 54Mbps as common usage */
					g_wmmac_usedtime[ac] += (len + 4) * 8 * wmmac_ratio / 10 / 54;
					g_wmmac_available[ac] = (g_wmmac_usedtime[ac] < g_wmmac_admittedtime[ac]);
					break;
				}
				if ((g_wmmac_available[ac] == false) && (g_wmmac_usedtime[ac] != 0))
					return SPRDWL_AC_MAX;
				/*current ac is not available, maybe usedtime > admitted time*/
				/*downgrade to lower ac, then try again*/
				ac--;
			} else {
				break;
			}
		}

		*tid = map_edca_ac_to_priority(ac);
	}

	priority >>= 2;

	for (match_index = 0; match_index < QOS_MAP_MAX_DSCP_EXCEPTION; match_index++) {
		if (priority == g_11u_qos_map.qos_exceptions[match_index].dscp) {
			*tid = g_11u_qos_map.qos_exceptions[match_index].up;
			break;
		}
	}

	if (match_index >= QOS_MAP_MAX_DSCP_EXCEPTION) {
		for (match_index = 0; match_index < 8; match_index++) {
			if ((priority >= g_11u_qos_map.qos_ranges[match_index].low) &&
			   (priority <= g_11u_qos_map.qos_ranges[match_index].high)) {
				*tid = g_11u_qos_map.qos_ranges[match_index].up;
				break;
			}
		}
	}
	switch (*tid) {
	case prio_1:
		qos_index = SPRDWL_AC_BK;
		break;
	case prio_4:
		qos_index = SPRDWL_AC_VI;
		break;
	case prio_6:
		qos_index = SPRDWL_AC_VO;
		break;
	default:
		qos_index = SPRDWL_AC_BE;
		break;
	}

	/*return data_type as qos queue index*/
	return qos_index;
}

const u8 *get_wmm_ie(u8 *res, u16 ie_len, u8 ie, uint oui, uint oui_type)
{
	const u8 *end, *pos;

	pos = res;
	end = pos + ie_len;
	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		/*try to find VENDOR_SPECIFIC ie, which wmm ie located*/
		if (pos[0] == ie) {
			/*match the OUI_MICROSOFT 0x0050f2 ie, and WMM ie*/
			if ((((pos[2] << 16) | (pos[3] << 8) | pos[4]) == oui) &&
				(pos[5] == WMM_OUI_TYPE)) {
				pos += 2;
				return pos;
			}
			break;
		}
		pos += 2 + pos[1];
	}
	return NULL;
}
#endif

