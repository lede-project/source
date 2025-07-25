#include <wcn_bus.h>

#include "bus_common.h"
#include "sdiohal.h"

static int sdio_get_hif_type(void)
{
	return HW_TYPE_SDIO;
}

int sdiohal_driver_register(void)
{
	return 0;
}

void sdiohal_driver_unregister(void)
{

}

static int sdio_preinit(void)
{
	sdiohal_init();
	return 0;
}

static void sdio_preexit(void)
{
	sdiohal_exit();
}

static int sdio_buf_list_alloc(int chn, struct mbuf_t **head,
			       struct mbuf_t **tail, int *num)
{
	return buf_list_alloc(chn, head, tail, num);
}

static int sdio_buf_list_free(int chn, struct mbuf_t *head,
			      struct mbuf_t *tail, int num)
{
	return buf_list_free(chn, head, tail, num);
}

static int sdio_list_push(int chn, struct mbuf_t *head,
			  struct mbuf_t *tail, int num)
{
	return sdiohal_list_push(chn, head, tail, num);
}

static int sdio_list_push_direct(int chn, struct mbuf_t *head,
				 struct mbuf_t *tail, int num)
{
	return sdiohal_list_direct_write(chn, head, tail, num);
}

static int sdio_chn_init(struct mchn_ops_t *ops)
{
	return bus_chn_init(ops, HW_TYPE_SDIO);
}

static int sdio_chn_deinit(struct mchn_ops_t *ops)
{
	return bus_chn_deinit(ops);
}

static int sdio_direct_read(unsigned int addr,
				void *buf, unsigned int len)
{
	return sdiohal_dt_read(addr, buf, len);
}

static int sdio_direct_write(unsigned int addr,
				void *buf, unsigned int len)
{
	return sdiohal_dt_write(addr, buf, len);
}

static int sdio_readbyte(unsigned int addr, unsigned char *val)
{
	return sdiohal_aon_readb(addr, val);
}

static int sdio_writebyte(unsigned int addr, unsigned char val)
{
	return sdiohal_aon_writeb(addr, val);
}

static unsigned int sdio_get_carddump_status(void)
{
	return sdiohal_get_carddump_status();
}

static void sdio_set_carddump_status(unsigned int flag)
{
	return sdiohal_set_carddump_status(flag);
}

static unsigned long long sdio_get_rx_total_cnt(void)
{
	return sdiohal_get_rx_total_cnt();
}

static int sdio_runtime_get(void)
{
	return sdiohal_runtime_get();
}

static int sdio_runtime_put(void)
{
	return sdiohal_runtime_put();
}

static int sdio_rescan(void)
{
	return sdiohal_scan_card();
}

static void sdio_register_rescan_cb(void *func)
{
	return sdiohal_register_scan_notify(func);
}

static void sdio_remove_card(void)
{
	return sdiohal_remove_card();
}

static void sdiohal_cp_allow_sleep(enum slp_subsys subsys)
{
#ifdef CONFIG_WCN_SLP
//#ifdef CONFIG_CPLOG_DEBUG
//	sdiohal_info("%s entry\n", __func__);
//#endif
	slp_mgr_drv_sleep(subsys, true);
#endif
}

static void sdiohal_cp_sleep_wakeup(enum slp_subsys subsys)
{
#ifdef CONFIG_WCN_SLP
//#ifdef CONFIG_CPLOG_DEBUG
//	sdiohal_info("%s entry\n", __func__);
//#endif
	slp_mgr_drv_sleep(subsys, false);
	slp_mgr_wakeup(subsys);
#endif
}

static struct sprdwcn_bus_ops sdiohal_bus_ops = {
	.preinit = sdio_preinit,
	.deinit = sdio_preexit,
	.chn_init = sdio_chn_init,
	.chn_deinit = sdio_chn_deinit,
	.list_alloc = sdio_buf_list_alloc,
	.list_free = sdio_buf_list_free,
	.push_list = sdio_list_push,
	.push_list_direct = sdio_list_push_direct,
	.direct_read = sdio_direct_read,
	.direct_write = sdio_direct_write,
	.readbyte = sdio_readbyte,
	.writebyte = sdio_writebyte,
	.read_l = sdiohal_readl,
	.write_l = sdiohal_writel,

	.get_carddump_status = sdio_get_carddump_status,
	.set_carddump_status = sdio_set_carddump_status,
	.get_rx_total_cnt = sdio_get_rx_total_cnt,

	.runtime_get = sdio_runtime_get,
	.runtime_put = sdio_runtime_put,

	.register_rescan_cb = sdio_register_rescan_cb,
	.rescan = sdio_rescan,
	.remove_card = sdio_remove_card,
	.get_tx_mode = sdiohal_get_tx_mode,
	.get_rx_mode = sdiohal_get_rx_mode,
	.get_irq_type = sdiohal_get_irq_type,
	.get_blk_size = sdiohal_get_blk_size,
	.get_hif_type = sdio_get_hif_type,
	.driver_register = sdiohal_driver_register,
	.driver_unregister = sdiohal_driver_unregister,
	.allow_sleep = sdiohal_cp_allow_sleep,
	.sleep_wakeup = sdiohal_cp_sleep_wakeup,
};

void module_bus_init(void)
{
	module_ops_register(&sdiohal_bus_ops);
}
EXPORT_SYMBOL(module_bus_init);

void module_bus_deinit(void)
{
	module_ops_unregister();
}
EXPORT_SYMBOL(module_bus_deinit);
