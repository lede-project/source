#include "debug.h"
#include <linux/spinlock.h>
#include <linux/jiffies.h>

struct debug_ctrl {
	spinlock_t debug_ctrl_lock;
	bool start;
};

static struct debug_ctrl debug_ctrl;

void debug_ctrl_init(void)
{
	spin_lock_init(&debug_ctrl.debug_ctrl_lock);
	debug_ctrl.start = false;
}

static bool check_debug_ctrl(void)
{
	bool value = false;

	spin_lock_bh(&debug_ctrl.debug_ctrl_lock);
	if (debug_ctrl.start)
		value = true;
	spin_unlock_bh(&debug_ctrl.debug_ctrl_lock);

	return value;
}

#define MAX_TS_NUM 20
struct debug_time_stamp {
	unsigned long ts_enter;
	unsigned int pos;
	unsigned int ts_record[MAX_TS_NUM];
	unsigned int max_ts;
};

static struct debug_time_stamp g_debug_ts[MAX_DEBUG_TS_INDEX];

void debug_ts_enter(enum debug_ts_index index)
{
	if (!check_debug_ctrl())
		return;

	g_debug_ts[index].ts_enter = jiffies;
}

void debug_ts_leave(enum debug_ts_index index)
{
	struct debug_time_stamp *ts = &g_debug_ts[index];

	if (!check_debug_ctrl() || (ts->ts_enter == 0))
		return;

	ts->ts_record[ts->pos] =
		jiffies_to_usecs(jiffies - ts->ts_enter);

	if (ts->ts_record[ts->pos] > ts->max_ts)
		ts->max_ts = ts->ts_record[ts->pos];

	(ts->pos < (MAX_TS_NUM - 1)) ? ts->pos++ : (ts->pos = 0);
}

void debug_ts_show(struct seq_file *s, enum debug_ts_index index)
{
	unsigned int i = 0;
	unsigned int avr_time = 0, avr_cnt = 0;
	struct debug_time_stamp *ts = &g_debug_ts[index];

	if (!check_debug_ctrl())
		return;

	seq_printf(s, "%s(us):", ts_index2str(index));
	for (i = 0; i < MAX_TS_NUM; i++) {
		seq_printf(s, " %d", ts->ts_record[i]);
		if (ts->ts_record[i] != 0) {
			avr_time += ts->ts_record[i];
			avr_cnt++;
		}
	}
	seq_printf(s, "\n%s average time(us): %d\n",
			ts_index2str(index), avr_time/avr_cnt);
	seq_printf(s, "%s max time(us): %d\n",
			ts_index2str(index), ts->max_ts);
}

struct debug_cnt {
	int cnt;
};

static struct debug_cnt g_debug_cnt[MAX_DEBUG_CNT_INDEX];

void debug_cnt_inc(enum debug_cnt_index index)
{
	if (!check_debug_ctrl())
		return;

	g_debug_cnt[index].cnt++;
}

void debug_cnt_dec(enum debug_cnt_index index)
{
	if (!check_debug_ctrl())
		return;

	g_debug_cnt[index].cnt--;
}

void debug_cnt_add(enum debug_cnt_index index, int num)
{
	if (!check_debug_ctrl())
		return;

	g_debug_cnt[index].cnt += num;
}

void debug_cnt_sub(enum debug_cnt_index index, int num)
{
	if (!check_debug_ctrl())
		return;

	g_debug_cnt[index].cnt -= num;
}

void debug_cnt_show(struct seq_file *s, enum debug_cnt_index index)
{
	if (!check_debug_ctrl())
		return;

	seq_printf(s, "%s: %d\n",
		cnt_index2str(index), g_debug_cnt[index].cnt);
}

#define MAX_RECORD_NUM 20
struct debug_record {
	unsigned int pos;
	int record[MAX_TS_NUM];
};

static struct debug_record g_debug_record[MAX_RECORD_NUM];

void debug_record_add(enum debug_record_index index, int num)
{
	struct debug_record *record = &g_debug_record[index];

	if (!check_debug_ctrl())
		return;

	record->record[record->pos] = num;
	(record->pos < (MAX_RECORD_NUM - 1)) ?
		record->pos++ : (record->pos = 0);
}

void debug_record_show(struct seq_file *s, enum debug_record_index index)
{
	struct debug_record *record = &g_debug_record[index];
	unsigned int i = 0;

	if (!check_debug_ctrl())
		return;

	seq_printf(s, "%s:", record_index2str(index));
	for (i = 0; i < MAX_RECORD_NUM; i++)
		seq_printf(s, " %d", record->record[i]);
	seq_puts(s, "\n");
}

void adjust_ts_cnt_debug(char *buf, unsigned char offset)
{
	int level = buf[offset] - '0';

	spin_lock_bh(&debug_ctrl.debug_ctrl_lock);
	if (level == 0) {
		debug_ctrl.start = false;
		spin_unlock_bh(&debug_ctrl.debug_ctrl_lock);
	} else {
		memset(g_debug_ts, 0,
			   (MAX_DEBUG_TS_INDEX *
			sizeof(struct debug_time_stamp)));
		memset(g_debug_cnt, 0,
			   (MAX_DEBUG_CNT_INDEX * sizeof(struct debug_cnt)));
		memset(g_debug_record, 0,
			   (MAX_RECORD_NUM * sizeof(struct debug_record)));
		debug_ctrl.start = true;
		spin_unlock_bh(&debug_ctrl.debug_ctrl_lock);
	}
}

