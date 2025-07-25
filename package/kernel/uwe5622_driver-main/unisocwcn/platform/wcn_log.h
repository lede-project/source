#ifndef _WCN_LOG
#define _WCN_LOG

#include "mdbg_type.h"

#define WCN_LOG_MAX_MINOR 2

struct mdbg_device_t {
	int			open_count;
	struct mutex		mdbg_lock;
	wait_queue_head_t	rxwait;
	struct wcnlog_dev *dev[WCN_LOG_MAX_MINOR];
	struct ring_device *ring_dev;
	bool exit_flag;
};

extern struct mdbg_device_t	*mdbg_dev;
extern wait_queue_head_t	mdbg_wait;
extern unsigned char flag_reset;
extern struct completion ge2_completion;

void wakeup_loopcheck_int(void);
int get_loopcheck_status(void);
void marlin_hold_cpu(void);
void wcnlog_clear_log(void);

int log_dev_init(void);
int log_dev_exit(void);
int wake_up_log_wait(void);
int log_cdev_exit(void);
int log_cdev_init(void);
#endif
