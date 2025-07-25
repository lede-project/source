#ifndef __SDIOHAL_H__
#define __SDIOHAL_H__

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#ifndef timespec
#define timespec timespec64
#define timespec_to_ns timespec64_to_ns
#define getnstimeofday ktime_get_real_ts64
#define timeval __kernel_old_timeval
#define rtc_time_to_tm rtc_time64_to_tm
#define timeval_to_ns ktime_to_ns
#endif

#include <linux/version.h>
#if KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE
#include <uapi/linux/sched/types.h>
#else
#include <linux/sched.h>
#endif
#include <wcn_bus.h>
#ifdef CONFIG_WCN_SLP
#include "../sleep/sdio_int.h"
#include "../sleep/slp_mgr.h"
#endif

#define PERFORMANCE_COUNT 100
#define SDIOHAL_PRINTF_LEN (16)
#define SDIOHAL_NORMAL_LEVEL (0x01)
#define SDIOHAL_DEBUG_LEVEL (0x02)
#define SDIOHAL_LIST_LEVEL (0x04)
#define SDIOHAL_DATA_LEVEL (0x08)
#define SDIOHAL_PERF_LEVEL (0x10)

#define sdiohal_info(fmt, args...) \
	pr_info("sdiohal:" fmt, ## args)
#define sdiohal_err(fmt, args...) \
	pr_err("sdiohal err:" fmt, ## args)

#ifdef CONFIG_DEBUG_FS
extern long int sdiohal_log_level;

#define sdiohal_normal(fmt, args...) \
	do { if (sdiohal_log_level & SDIOHAL_NORMAL_LEVEL) \
		sdiohal_info(fmt, ## args); \
	} while (0)
#define sdiohal_debug(fmt, args...) \
	do { if (sdiohal_log_level & SDIOHAL_DEBUG_LEVEL) \
		sdiohal_info(fmt, ## args); \
	} while (0)
#define sdiohal_pr_list(loglevel, fmt, args...) \
	do { if (sdiohal_log_level & loglevel) \
		sdiohal_info(fmt, ## args); \
	} while (0)
#define sdiohal_pr_data(level, prefix_str, prefix_type, \
			rowsize, groupsize, buf, len, ascii, loglevel) \
	do { if (sdiohal_log_level & loglevel) \
		print_hex_dump(level, prefix_str, prefix_type, \
			       rowsize, groupsize, buf, len, ascii); \
	} while (0)
#define sdiohal_pr_perf(fmt, args...) \
	do { if (sdiohal_log_level & SDIOHAL_PERF_LEVEL) \
		printk(fmt, ## args); \
	} while (0)
#else
#define sdiohal_normal(fmt, args...)
#define sdiohal_debug(fmt, args...)
#define sdiohal_pr_list(loglevel, fmt, args...)
#define	sdiohal_pr_data(level, prefix_str, prefix_type, \
			rowsize, groupsize, buf, len, ascii, loglevel)
#define sdiohal_pr_perf(fmt, args...)
#endif

#define SDIOHAL_SDMA	0
#define SDIOHAL_ADMA	1

#define SDIOHAL_RX_EXTERNAL_IRQ	0
#define SDIOHAL_RX_INBAND_IRQ	1
#define SDIOHAL_RX_POLLING	2

#ifdef CONFIG_UWE5623
#define SDIO_RESET_ENABLE 0x40930040
#endif

/* channel numbers */
#define SDIO_CHN_TX_NUM 12
#define SDIO_CHN_RX_NUM 14

#define SDIO_CHANNEL_NUM (SDIO_CHN_TX_NUM + SDIO_CHN_RX_NUM)

/* dump designated channel data when assert happened */
#define SDIO_DUMP_CHANNEL_DATA		1
#define SDIO_DUMP_TX_CHANNEL_NUM	8
#define SDIO_DUMP_RX_CHANNEL_NUM	22
#define SDIO_DUMP_RX_WIFI_EVENT_MIN	(0x80)

/* list bumber */
#define SDIO_TX_LIST_NUM SDIO_CHN_TX_NUM
#define SDIO_RX_LIST_NUM SDIO_CHN_RX_NUM
#define MAX_CHAIN_NODE_NUM 100

/* task prio */
#define SDIO_TX_TASK_PRIO 89
#define SDIO_RX_TASK_PRIO 90

/* mbuf max size */
#define MAX_MBUF_SIZE (2 << 10)

#ifdef CONFIG_SDIO_BLKSIZE_512
/* cp blk size */
#define SDIOHAL_BLK_SIZE 512
/* each pac data max size,cp align size */
#define MAX_PAC_SIZE (SDIOHAL_BLK_SIZE * 4)
#elif defined(CONFIG_WCN_PARSE_DTS)
/* cp blk size */
#define SDIOHAL_BLK_SIZE (sprdwcn_bus_get_blk_size())
/* each pac data max size,cp align size */
#define MAX_PAC_SIZE ((SDIOHAL_BLK_SIZE == 512) ? \
	(SDIOHAL_BLK_SIZE * 4) : (SDIOHAL_BLK_SIZE * 2))
#else
/* cp blk size */
#define SDIOHAL_BLK_SIZE 840
/* each pac data max size,cp align size */
#define MAX_PAC_SIZE (SDIOHAL_BLK_SIZE * 2)
#endif

/* pub header size */
#define SDIO_PUB_HEADER_SIZE (4)
#define SDIOHAL_DTBS_BUF_SIZE SDIOHAL_BLK_SIZE

/* for rx buf */
#define SDIOHAL_RX_NODE_NUM (12 << 10)

/* for 64 bit sys */
#define SDIOHAL_RX_RECVBUF_LEN (MAX_CHAIN_NODE_NUM * MAX_MBUF_SIZE)
#define SDIOHAL_FRAG_PAGE_MAX_ORDER \
	get_order(SDIOHAL_RX_RECVBUF_LEN)

/* for 32 bit sys */
#define SDIOHAL_32_BIT_RX_RECVBUF_LEN (16 << 10)
#define SDIOHAL_FRAG_PAGE_MAX_ORDER_32_BIT \
	get_order(SDIOHAL_32_BIT_RX_RECVBUF_LEN)

#define SDIOHAL_FRAG_PAGE_MAX_SIZE \
	(PAGE_SIZE << SDIOHAL_FRAG_PAGE_MAX_ORDER)
#define SDIOHAL_PAGECNT_MAX_BIAS SDIOHAL_FRAG_PAGE_MAX_SIZE

/* tx buf size for normal dma mode */
#define SDIOHAL_TX_SENDBUF_LEN (MAX_CHAIN_NODE_NUM * MAX_MBUF_SIZE)

/* temp for marlin2 */
#define WIFI_MIN_RX 8
#define WIFI_MAX_RX 9

/* for adma */
#define SDIOHAL_READ 0 /* Read request */
#define SDIOHAL_WRITE 1 /* Write request */
#define SDIOHAL_DATA_FIX 0 /* Fixed addressing */
#define SDIOHAL_DATA_INC 1 /* Incremental addressing */
#define MAX_IO_RW_BLK 511

/* fun num */
#define FUNC_0  0
#define FUNC_1  1
#define SDIOHAL_MAX_FUNCS 2

/* cp sdio reg addr */
#define SDIOHAL_DT_MODE_ADDR	0x0f
#define SDIOHAL_PK_MODE_ADDR	0x20
#define SDIOHAL_CCCR_ABORT		0x06
#define VAL_ABORT_TRANS			0x01
#define SDIOHAL_FBR_SYSADDR0	0x15c
#define SDIOHAL_FBR_SYSADDR1	0x15d
#define SDIOHAL_FBR_SYSADDR2	0x15e
#define SDIOHAL_FBR_SYSADDR3	0x15f
#define SDIOHAL_FBR_APBRW0		0x180
#define SDIOHAL_FBR_APBRW1		0x181
#define SDIOHAL_FBR_APBRW2		0x182
#define SDIOHAL_FBR_APBRW3		0x183
#define SDIOHAL_FBR_STBBA0		0x1bc
#define SDIOHAL_FBR_STBBA1		0x1bd
#define SDIOHAL_FBR_STBBA2		0x1be
#define SDIOHAL_FBR_STBBA3		0x1bf
#define SDIOHAL_FBR_DEINT_EN	0x1ca
#define VAL_DEINT_ENABLE		0x3
#define SDIOHAL_FBR_PUBINT_RAW4	0x1e8

#define SDIOHAL_ALIGN_4BYTE(a)  (((a)+3)&(~3))
#ifdef CONFIG_SDIO_BLKSIZE_512
#define SDIOHAL_ALIGN_BLK(a)  (((a)+511)&(~511))
#elif defined(CONFIG_WCN_PARSE_DTS)
#define SDIOHAL_ALIGN_BLK(a) ((SDIOHAL_BLK_SIZE == 512) ? \
	(((a)+511)&(~511)) : (((a)%SDIOHAL_BLK_SIZE) ? \
	(((a)/SDIOHAL_BLK_SIZE + 1)*SDIOHAL_BLK_SIZE) : (a)))
#else
#define SDIOHAL_ALIGN_BLK(a) (((a)%SDIOHAL_BLK_SIZE) ? \
	(((a)/SDIOHAL_BLK_SIZE + 1)*SDIOHAL_BLK_SIZE) : (a))
#endif

#define WCN_128BIT_CTL_BASE 0x1a0
#define WCN_128BIT_STAT_BASE 0x140
#define CP_128BIT_SIZE	(0x0f)

#define WCN_CTL_REG_0 (WCN_128BIT_CTL_BASE + 0X00)
#define WCN_CTL_REG_1 (WCN_128BIT_CTL_BASE + 0X01)
#define WCN_CTL_REG_2 (WCN_128BIT_CTL_BASE + 0X02)
#define WCN_CTL_REG_3 (WCN_128BIT_CTL_BASE + 0X03)
#define WCN_CTL_REG_4 (WCN_128BIT_CTL_BASE + 0X04)
#define WCN_CTL_REG_5 (WCN_128BIT_CTL_BASE + 0X05)
#define WCN_CTL_REG_6 (WCN_128BIT_CTL_BASE + 0X06)
#define WCN_CTL_REG_7 (WCN_128BIT_CTL_BASE + 0X07)
#define WCN_CTL_REG_8 (WCN_128BIT_CTL_BASE + 0X08)
#define WCN_CTL_REG_9 (WCN_128BIT_CTL_BASE + 0X09)
#define WCN_CTL_REG_10 (WCN_128BIT_CTL_BASE + 0X0a)
#define WCN_CTL_REG_11 (WCN_128BIT_CTL_BASE + 0X0b)
#define WCN_CTL_REG_12 (WCN_128BIT_CTL_BASE + 0X0c)
#define WCN_CTL_REG_13 (WCN_128BIT_CTL_BASE + 0X0d)
#define WCN_CTL_REG_14 (WCN_128BIT_CTL_BASE + 0X0e)
#define WCN_CTL_REG_15 (WCN_128BIT_CTL_BASE + 0X0f)

#define WCN_STATE_REG_0 (WCN_128BIT_STAT_BASE + 0X00)
#define WCN_STATE_REG_1 (WCN_128BIT_STAT_BASE + 0X01)
#define WCN_STATE_REG_2 (WCN_128BIT_STAT_BASE + 0X02)
#define WCN_STATE_REG_3 (WCN_128BIT_STAT_BASE + 0X03)
#define WCN_STATE_REG_4 (WCN_128BIT_STAT_BASE + 0X04)
#define WCN_STATE_REG_5 (WCN_128BIT_STAT_BASE + 0X05)
#define WCN_STATE_REG_6 (WCN_128BIT_STAT_BASE + 0X06)
#define WCN_STATE_REG_7 (WCN_128BIT_STAT_BASE + 0X07)
#define WCN_STATE_REG_8 (WCN_128BIT_STAT_BASE + 0X08)
#define WCN_STATE_REG_9 (WCN_128BIT_STAT_BASE + 0X09)
#define WCN_STATE_REG_10 (WCN_128BIT_STAT_BASE + 0X0a)
#define WCN_STATE_REG_11 (WCN_128BIT_STAT_BASE + 0X0b)
#define WCN_STATE_REG_12 (WCN_128BIT_STAT_BASE + 0X0c)
#define WCN_STATE_REG_13 (WCN_128BIT_STAT_BASE + 0X0d)
#define WCN_STATE_REG_14 (WCN_128BIT_STAT_BASE + 0X0e)
#define WCN_STATE_REG_15 (WCN_128BIT_STAT_BASE + 0X0f)

#define SDIO_VER_CCCR	(0X0)

#define SDIO_CP_INT_EN (DUMP_SDIO_ADDR + 0x58)

/* for marlin3 */
#define CP_PMU_STATUS	(WCN_STATE_REG_0)
#define CP_SWITCH_SGINAL (WCN_CTL_REG_4)
#define CP_RESET_SLAVE  (WCN_CTL_REG_8)
#define CP_BUS_HREADY	(WCN_STATE_REG_4)
#define CP_HREADY_SIZE	(0x04)

/* for malrin3e */
#define WCN_CTL_EN	BIT(7)
#define WCN_SYS_MASK	(0xf)
#define WCN_MODE_MASK	(0x3 << 4)
#define WCN_DEBUG_CTL_REG (WCN_CTL_REG_2)
#define WCN_DEBUG_MODE_SYS_REG (WCN_CTL_REG_1)
#define WCN_SEL_SIG_REG	(WCN_CTL_REG_0)
#define WCN_SIG_STATE	(WCN_STATE_REG_0)

#define SDIOHAL_REMOVE_CARD_VAL 0x8000
#define WCN_CARD_EXIST(xmit) \
	(atomic_read(xmit) < SDIOHAL_REMOVE_CARD_VAL)

struct sdiohal_frag_mg {
	struct page_frag frag;
	unsigned int pagecnt_bias;
};

struct sdiohal_list_t {
	struct list_head head;
	struct mbuf_t *mbuf_head;
	struct mbuf_t *mbuf_tail;
	unsigned int type;
	unsigned int subtype;
	unsigned int node_num;
};

struct buf_pool_t {
	int size;
	int free;
	int payload;
	void *head;
	char *mem;
	spinlock_t lock;
};

struct sdiohal_sendbuf_t {
	unsigned int used_len;
	unsigned char *buf;
	unsigned int retry_len;
	unsigned char *retry_buf;
};

struct sdiohal_data_bak_t {
	unsigned int time;
	unsigned char data_bk[SDIOHAL_PRINTF_LEN];
};

struct sdiohal_data_t {
	struct task_struct *tx_thread;
	struct task_struct *rx_thread;
	struct completion tx_completed;
	struct completion rx_completed;
	/*wakeup_source pointer*/
	struct wakeup_source *tx_ws;
	struct wakeup_source *rx_ws;

	atomic_t tx_wake_flag;
	atomic_t rx_wake_flag;
#ifdef CONFIG_WCN_SLP
	atomic_t tx_wake_cp_count[SUBSYS_MAX];
	atomic_t rx_wake_cp_count[SUBSYS_MAX];
#endif
	struct mutex xmit_lock;
	struct mutex xmit_sdma;
	spinlock_t tx_spinlock;
	spinlock_t rx_spinlock;
	atomic_t flag_resume;
	atomic_t tx_mbuf_num;
	atomic_t xmit_cnt;
	atomic_t xmit_start;
	bool exit_flag;
	/* adma enable:1, disable:0 */
	bool adma_tx_enable;
	bool adma_rx_enable;
	bool pwrseq;
	/* blk_size: 0 840, 1 512 */
	bool blk_size;
	/* dt mode read or write fail flag */
	bool dt_rw_fail;
	/* EXTERNAL_IRQ 0, INBAND_IRQ 1, POLLING 2. */
	unsigned char irq_type;

	/* tx data list for send */
	struct sdiohal_list_t tx_list_head;
	/* tx data list for pop */
	struct sdiohal_list_t *list_tx[SDIO_CHN_TX_NUM];
	/* rx data list for dispatch */
	struct sdiohal_list_t *list_rx[SDIO_CHN_RX_NUM];
	/* mbuf list */
	struct sdiohal_list_t list_rx_buf;
	/* frag data buf */
	struct sdiohal_frag_mg frag_ctl;
	struct mchn_ops_t *ops[SDIO_CHANNEL_NUM];
	struct mutex callback_lock[SDIO_CHANNEL_NUM];
	struct buf_pool_t pool[SDIO_CHANNEL_NUM];

	bool flag_init;
	atomic_t flag_suspending;
	int gpio_num;
	unsigned int irq_num;
	unsigned int irq_trigger_type;
	atomic_t irq_cnt;
	unsigned int card_dump_flag;
	struct sdio_func *sdio_func[SDIOHAL_MAX_FUNCS];
	struct mmc_host *sdio_dev_host;
	struct scatterlist sg_list[MAX_CHAIN_NODE_NUM + 1];

	unsigned int success_pac_num;
	struct sdiohal_sendbuf_t send_buf;
	unsigned char *eof_buf;

	unsigned int dtbs;
	unsigned int remain_pac_num;
	unsigned long long rx_packer_cnt;
	char *dtbs_buf;

	/* for performance statics */
	struct timespec tm_begin_sch;
	struct timespec tm_end_sch;
	struct timespec tm_begin_irq;
	struct timespec tm_end_irq;

	/*wakeup_source pointer*/
	struct wakeup_source *scan_ws;

	struct completion scan_done;
	struct completion remove_done;
	unsigned int sdio_int_reg;
#if SDIO_DUMP_CHANNEL_DATA
	struct sdiohal_data_bak_t chntx_push_old;
	struct sdiohal_data_bak_t chntx_push_new;
	struct sdiohal_data_bak_t chntx_denq_old;
	struct sdiohal_data_bak_t chntx_denq_new;
	struct sdiohal_data_bak_t chnrx_dispatch_old;
	struct sdiohal_data_bak_t chnrx_dispatch_new;
#endif
	int printlog_txchn;
	int printlog_rxchn;
};

struct sdiohal_data_t *sdiohal_get_data(void);
unsigned char sdiohal_get_tx_mode(void);
unsigned char sdiohal_get_rx_mode(void);
unsigned char sdiohal_get_irq_type(void);
unsigned int sdiohal_get_blk_size(void);

/* for list manger */
void sdiohal_atomic_add(int count, atomic_t *value);
void sdiohal_atomic_sub(int count, atomic_t *value);

/* seam for thread */
void sdiohal_tx_down(void);
void sdiohal_tx_up(void);
void sdiohal_rx_down(void);
void sdiohal_rx_up(void);
int sdiohal_tx_thread(void *data);
int sdiohal_rx_thread(void *data);

/* for wakup event */
void sdiohal_lock_tx_ws(void);
void sdiohal_unlock_tx_ws(void);
void sdiohal_lock_rx_ws(void);
void sdiohal_unlock_rx_ws(void);
void sdiohal_lock_scan_ws(void);
void sdiohal_unlock_scan_ws(void);

/* for api mutex */
void sdiohal_callback_lock(struct mutex *mutex);
void sdiohal_callback_unlock(struct mutex *mutex);

/* for sleep */
#ifdef CONFIG_WCN_SLP
void sdiohal_cp_tx_sleep(enum slp_subsys subsys);
void sdiohal_cp_tx_wakeup(enum slp_subsys subsys);
void sdiohal_cp_rx_sleep(enum slp_subsys subsys);
void sdiohal_cp_rx_wakeup(enum slp_subsys subsys);
#else
#define sdiohal_cp_tx_sleep(args...) do {} while (0)
#define sdiohal_cp_tx_wakeup(args...) do {} while (0)
#define sdiohal_cp_rx_sleep(args...) do {} while (0)
#define sdiohal_cp_rx_wakeup(args...) do {} while (0)
#endif

void sdiohal_resume_check(void);
void sdiohal_resume_wait(void);
void sdiohal_op_enter(void);
void sdiohal_op_leave(void);
void sdiohal_sdma_enter(void);
void sdiohal_sdma_leave(void);
void sdiohal_channel_to_hwtype(int inout, int chn,
			       unsigned int *type, unsigned int *subtype);
int sdiohal_hwtype_to_channel(int inout, unsigned int type,
			      unsigned int subtype);

/* for list manger */
bool sdiohal_is_tx_list_empty(void);
int sdiohal_tx_packer(struct sdiohal_sendbuf_t *send_buf,
		      struct sdiohal_list_t *data_list,
		      struct mbuf_t *mbuf_node);
int sdiohal_tx_set_eof(struct sdiohal_sendbuf_t *send_buf,
		       unsigned char *eof_buf);
void sdiohal_tx_list_enq(int channel, struct mbuf_t *head,
			 struct mbuf_t *tail, int num);
void sdiohal_tx_find_data_list(struct sdiohal_list_t *data_list);
int sdiohal_tx_list_denq(struct sdiohal_list_t *data_list);
int sdiohal_rx_list_free(struct mbuf_t *mbuf_head,
			 struct mbuf_t *mbuf_tail, int num);
struct sdiohal_list_t *sdiohal_get_rx_mbuf_list(int num);
struct sdiohal_list_t *sdiohal_get_rx_mbuf_node(int num);
int sdiohal_rx_list_dispatch(void);
struct sdiohal_list_t *sdiohal_get_rx_channel_list(int channel);
void *sdiohal_get_rx_free_buf(unsigned int *alloc_size, unsigned int read_len);
void sdiohal_tx_init_retrybuf(void);
int sdiohal_misc_init(void);
void sdiohal_misc_deinit(void);

/* sdiohal main.c */
void sdiohal_sdio_tx_status(void);
unsigned int sdiohal_get_trans_pac_num(void);
int sdiohal_sdio_pt_write(unsigned char *src, unsigned int datalen);
int sdiohal_sdio_pt_read(unsigned char *src, unsigned int datalen);
int sdiohal_adma_pt_write(struct sdiohal_list_t *data_list);
int sdiohal_adma_pt_read(struct sdiohal_list_t *data_list);
int sdiohal_tx_data_list_send(struct sdiohal_list_t *data_list,
			      bool pop_flag);
void sdiohal_enable_rx_irq(void);

/* for debugfs */
#ifdef CONFIG_DEBUG_FS
void sdiohal_debug_init(void);
void sdiohal_debug_deinit(void);
#endif


void sdiohal_print_list_data(int channel, struct sdiohal_list_t *data_list,
			     const char *func, int loglevel);
void sdiohal_print_mbuf_data(int channel, struct mbuf_t *head,
			     struct mbuf_t *tail, int num, const char *func,
			     int loglevel);

void sdiohal_list_check(struct sdiohal_list_t *data_list,
			const char *func, bool dir);
void sdiohal_mbuf_list_check(int channel, struct mbuf_t *head,
			     struct mbuf_t *tail, int num,
			     const char *func, bool dir, int loglevel);

int sdiohal_init(void);
void sdiohal_exit(void);
int sdiohal_list_push(int chn, struct mbuf_t *head,
		      struct mbuf_t *tail, int num);
int sdiohal_list_direct_write(int channel, struct mbuf_t *head,
			      struct mbuf_t *tail, int num);

/* driect mode,reg access.etc */
int sdiohal_dt_read(unsigned int addr, void *buf, unsigned int len);
int sdiohal_dt_write(unsigned int addr, void *buf, unsigned int len);
int sdiohal_aon_readb(unsigned int addr, unsigned char *val);
int sdiohal_aon_writeb(unsigned int addr, unsigned char val);
int sdiohal_writel(unsigned int system_addr, void *buf);
int sdiohal_readl(unsigned int system_addr, void *buf);
void sdiohal_dump_aon_reg(void);

/* for dumpmem */
unsigned int sdiohal_get_carddump_status(void);
void sdiohal_set_carddump_status(unsigned int flag);

/* for loopcheck */
unsigned long long sdiohal_get_rx_total_cnt(void);

/* for power on/off */
int sdiohal_runtime_get(void);
int sdiohal_runtime_put(void);

void sdiohal_register_scan_notify(void *func);
int sdiohal_scan_card(void);
void sdiohal_remove_card(void);

#ifdef SDIO_RESET_DEBUG
/* Some custome platform not export sdio_reset_comm symbol. */
extern int sdio_reset_comm(struct mmc_card *card);
int sdiohal_disable_apb_reset(void);
void sdiohal_reset(bool full_reset);
#endif
#endif
