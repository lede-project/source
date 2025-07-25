#ifndef __WCN_BUS_H__
#define __WCN_BUS_H__

#define CHN_MAX_NUM 32

#ifdef CONFIG_WCN_SDIO
#define PUB_HEAD_RSV 4
#else
#define PUB_HEAD_RSV 0
#endif

enum wcn_hw_type {
	HW_TYPE_SDIO = 0,
	HW_TYPE_PCIE,
	HW_TYPE_SIPC,
	HW_TYPE_USB,
	HW_TYPE_UNKNOWN
};

enum wcn_vendor_id {
	/* NOT pull chipen, NOT reset sdio after resume. */
	WCN_VENDOR_DEFAULT = 0,
	/* PULL chipen after resume. */
	WCN_VENDOR_RESUME_POWER_DOWN,
	/* NOT pull chipen, reset sdio after resume. */
	WCN_VENDOR_RESUME_KEEPPWR_RESETSDIO,

	WCN_VENDOR_MAX = 8
};

enum slp_subsys {
	PACKER_TX = 0,
	PACKER_RX,
	PACKER_DT_TX,
	PACKER_DT_RX,
	DT_WRITEL,
	DT_READL,
	DT_WRITE,
	DT_READ,
	WIFI,
	DOWNLOAD,
	DBG_TOOL,
	SUBSYS_MAX,
};

struct mbuf_t {
	struct mbuf_t *next;
	unsigned char *buf;
	unsigned long  phy;
	unsigned short len;
	unsigned short rsvd;
	unsigned int   seq;
};

struct mchn_ops_t {
	int channel;
	/* hardware interface type */
	int hif_type;
	/* inout=1 tx side, inout=0 rx side */
	int inout;
	/* set callback pop_link/push_link frequency */
	int intr_interval;
	/* data buffer size */
	int buf_size;
	/* mbuf pool size */
	int pool_size;
	/* The large number of trans */
	int once_max_trans;
	/* rx side threshold */
	int rx_threshold;
	/* tx timeout */
	int timeout;
	/* callback in top tophalf */
	int cb_in_irq;
	/* pending link num */
	int max_pending;
	/*
	 * pop link list, (1)chn id, (2)mbuf link head
	 * (3) mbuf link tail (4)number of node
	 */
	int (*pop_link)(int, struct mbuf_t *, struct mbuf_t *, int);
	/* ap don't need to implementation */
	int (*push_link)(int, struct mbuf_t **, struct mbuf_t **, int *);
	/* (1)channel id (2)trans time, -1 express timeout */
	int (*tx_complete)(int, int);
	int (*power_notify)(int, int);
};

struct sdio_puh_t {
#ifdef CONFIG_SDIOM
	unsigned int pad:7;
#else
	unsigned int pad:6;
	unsigned int check_sum:1;
#endif
	unsigned int len:16;
	unsigned int eof:1;
	unsigned int subtype:4;
	unsigned int type:4;
}; /* 32bits public header */

struct bus_puh_t {
#ifdef CONFIG_SDIOM
	unsigned int pad:7;
#else
	unsigned int pad:6;
	unsigned int check_sum:1;
#endif
	unsigned int len:16;
	unsigned int eof:1;
	unsigned int subtype:4;
	unsigned int type:4;
}; /* 32bits public header */

struct sprdwcn_bus_ops {
	int (*preinit)(void);
	void (*deinit)(void);

	int (*chn_init)(struct mchn_ops_t *ops);
	int (*chn_deinit)(struct mchn_ops_t *ops);

	/*
	 * For sdio:
	 * list_alloc and list_free only tx available.
	 * TX: module manage buf, RX: SDIO unified manage buf
	 */
	int (*list_alloc)(int chn, struct mbuf_t **head,
			  struct mbuf_t **tail, int *num);
	int (*list_free)(int chn, struct mbuf_t *head,
			 struct mbuf_t *tail, int num);

	/* For sdio: TX(send data) and RX(give back list to SDIO) */
	int (*push_list)(int chn, struct mbuf_t *head,
			 struct mbuf_t *tail, int num);
	int (*push_list_direct)(int chn, struct mbuf_t *head,
			 struct mbuf_t *tail, int num);
	unsigned char (*get_tx_mode)(void);
	unsigned char (*get_rx_mode)(void);
	unsigned char (*get_irq_type)(void);
	unsigned int (*get_blk_size)(void);

	/*
	 * for pcie
	 * push link list, Using a blocking mode,
	 * Timeout wait for tx_complete
	 */
	int (*push_link_wait_complete)(int chn, struct mbuf_t *head,
				       struct mbuf_t *tail, int num,
				       int timeout);
	int (*hw_pop_link)(int chn, void *head, void *tail, int num);
	int (*hw_tx_complete)(int chn, int timeout);
	int (*hw_req_push_link)(int chn, int need);

	int (*direct_read)(unsigned int addr, void *buf, unsigned int len);
	int (*direct_write)(unsigned int addr, void *buf, unsigned int len);

	int (*readbyte)(unsigned int addr, unsigned char *val);
	int (*writebyte)(unsigned int addr, unsigned char val);

	int (*write_l)(unsigned int system_addr, void *buf);
	int (*read_l)(unsigned int system_addr, void *buf);

	unsigned int (*get_carddump_status)(void);
	void (*set_carddump_status)(unsigned int flag);
	unsigned long long (*get_rx_total_cnt)(void);

	/* for runtime */
	int (*runtime_get)(void);
	int (*runtime_put)(void);

	int (*rescan)(void);
	void (*register_rescan_cb)(void *);
	void (*remove_card)(void);
	/* for module to know hif_type */
	int (*get_hif_type)(void);
	int (*driver_register)(void);
	void (*driver_unregister)(void);
	void (*allow_sleep)(enum slp_subsys subsys);
	void (*sleep_wakeup)(enum slp_subsys subsys);

	/* for usb check cp status */
	int (*check_cp_ready)(unsigned int addr, int timeout);
};

extern void module_bus_init(void);
extern void module_bus_deinit(void);
extern struct sprdwcn_bus_ops *get_wcn_bus_ops(void);

static inline
int sprdwcn_bus_get_hif_type(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_hif_type)
		return 0;

	return bus_ops->get_hif_type();
}

static inline
int sprdwcn_bus_preinit(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->preinit)
		return 0;

	return bus_ops->preinit();
}

static inline
void sprdwcn_bus_deinit(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->deinit)
		return;

	bus_ops->deinit();
}

static inline
int sprdwcn_bus_chn_init(struct mchn_ops_t *ops)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->chn_init)
		return 0;

	return bus_ops->chn_init(ops);
}

static inline
int sprdwcn_bus_chn_deinit(struct mchn_ops_t *ops)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->chn_deinit)
		return 0;

	return bus_ops->chn_deinit(ops);
}

static inline
int sprdwcn_bus_list_alloc(int chn, struct mbuf_t **head,
			   struct mbuf_t **tail, int *num)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->list_alloc)
		return 0;

	return bus_ops->list_alloc(chn, head, tail, num);
}

static inline
int sprdwcn_bus_list_free(int chn, struct mbuf_t *head,
			  struct mbuf_t *tail, int num)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->list_free)
		return 0;

	return bus_ops->list_free(chn, head, tail, num);
}

static inline
int sprdwcn_bus_push_list(int chn, struct mbuf_t *head,
			  struct mbuf_t *tail, int num)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->push_list)
		return 0;

	return bus_ops->push_list(chn, head, tail, num);
}

static inline
int sprdwcn_bus_push_list_direct(int chn, struct mbuf_t *head,
				 struct mbuf_t *tail, int num)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->push_list_direct)
		return 0;

	return bus_ops->push_list_direct(chn, head, tail, num);
}

static inline
int sprdwcn_bus_push_link_wait_complete(int chn, struct mbuf_t *head,
					struct mbuf_t *tail, int num,
					int timeout)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->push_link_wait_complete)
		return 0;

	return bus_ops->push_link_wait_complete(chn, head,
						    tail, num, timeout);
}

static inline
int sprdwcn_bus_hw_pop_link(int chn, void *head, void *tail, int num)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->hw_pop_link)
		return 0;

	return bus_ops->hw_pop_link(chn, head, tail, num);
}

static inline
int sprdwcn_bus_hw_tx_complete(int chn, int timeout)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->hw_tx_complete)
		return 0;

	return bus_ops->hw_tx_complete(chn, timeout);
}

static inline
int sprdwcn_bus_hw_req_push_link(int chn, int need)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->hw_req_push_link)
		return 0;

	return bus_ops->hw_req_push_link(chn, need);
}

static inline
int sprdwcn_bus_direct_read(unsigned int addr,
			    void *buf, unsigned int len)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->direct_read)
		return 0;

	return bus_ops->direct_read(addr, buf, len);
}

static inline
int sprdwcn_bus_direct_write(unsigned int addr,
			     void *buf, unsigned int len)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->direct_write)
		return 0;

	return bus_ops->direct_write(addr, buf, len);
}

static inline
int sprdwcn_bus_reg_read(unsigned int addr,
			    void *buf, unsigned int len)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->read_l)
		return 0;

	return bus_ops->read_l(addr, buf);
}

static inline
int sprdwcn_bus_reg_write(unsigned int addr,
			     void *buf, unsigned int len)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->write_l)
		return 0;

	return bus_ops->write_l(addr, buf);
}

static inline
int sprdwcn_bus_aon_readb(unsigned int addr, unsigned char *val)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->readbyte)
		return 0;

	return bus_ops->readbyte(addr, val);
}

static inline
int sprdwcn_bus_aon_writeb(unsigned int addr, unsigned char val)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->writebyte)
		return 0;

	return bus_ops->writebyte(addr, val);
}

static inline
unsigned int sprdwcn_bus_get_carddump_status(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_carddump_status)
		return 0;

	return bus_ops->get_carddump_status();
}

static inline
void sprdwcn_bus_set_carddump_status(unsigned int flag)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->set_carddump_status)
		return;

	bus_ops->set_carddump_status(flag);
}

static inline
unsigned long long sprdwcn_bus_get_rx_total_cnt(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_rx_total_cnt)
		return 0;

	return bus_ops->get_rx_total_cnt();
}

static inline
int sprdwcn_bus_runtime_get(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->runtime_get)
		return 0;

	return bus_ops->runtime_get();
}

static inline
int sprdwcn_bus_runtime_put(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->runtime_put)
		return 0;

	return bus_ops->runtime_put();
}

static inline
int sprdwcn_bus_rescan(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->rescan)
		return 0;

	return bus_ops->rescan();
}

static inline
void sprdwcn_bus_register_rescan_cb(void *func)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->register_rescan_cb)
		return;

	bus_ops->register_rescan_cb(func);
}

static inline
void sprdwcn_bus_remove_card(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->remove_card)
		return;

	bus_ops->remove_card();
}

static inline
unsigned char sprdwcn_bus_get_tx_mode(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_tx_mode)
		return 0;

	return bus_ops->get_tx_mode();
}

static inline
unsigned char sprdwcn_bus_get_rx_mode(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_rx_mode)
		return 0;

	return bus_ops->get_rx_mode();
}

static inline
unsigned char sprdwcn_bus_get_irq_type(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_irq_type)
		return 0;

	return bus_ops->get_irq_type();
}

static inline
unsigned int sprdwcn_bus_get_blk_size(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->get_blk_size)
		return 0;

	return bus_ops->get_blk_size();
}

static inline
void sprdwcn_bus_allow_sleep(enum slp_subsys subsys)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->allow_sleep)
		return;

	bus_ops->allow_sleep(subsys);
}

static inline
void sprdwcn_bus_sleep_wakeup(enum slp_subsys subsys)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->sleep_wakeup)
		return;

	bus_ops->sleep_wakeup(subsys);
}

static inline
int sprdwcn_bus_driver_register(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->driver_register)
		return 0;

	return bus_ops->driver_register();
}

static inline
void sprdwcn_bus_driver_unregister(void)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->driver_unregister)
		return;

	bus_ops->driver_unregister();
}

static inline
int sprdwcn_check_cp_ready(unsigned int addr, int timeout)
{
	struct sprdwcn_bus_ops *bus_ops = get_wcn_bus_ops();

	if (!bus_ops || !bus_ops->check_cp_ready)
		return 0;

	return bus_ops->check_cp_ready(addr, timeout);
}

static inline
int wcn_bus_init(void)
{
	module_bus_init();
	return 0;
}

static inline
void wcn_bus_deinit(void)
{
	module_bus_deinit();
}

#endif
