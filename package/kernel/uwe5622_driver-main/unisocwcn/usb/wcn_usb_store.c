#include "wcn_usb.h"
/**
 * struct wcn_usb_store_chn2ep - Chn and ep map table.
 * @ep:	ep. chn map to.
 * @mutex: every entry map only allow one thread touch self when one time.
 */
struct wcn_usb_store_chn2ep {
	struct wcn_usb_work_data *work_data;
	struct wcn_usb_ep *ep;
	__u8 epAddress;
};

#define ep_get_chn2ep(x) container_of(x, struct wcn_usb_store_chn2ep, ep)

#define FILL_CHN2EP_MAP(channel_id, ep_address)\
	.ep = NULL, \
	.epAddress = ep_address

/* we need explicitly define the chn2ep table */
/* channel_id must be Continuous and begin with zero */
/* then we can say index == channel */
static struct wcn_usb_store_chn2ep chn2ep_table[] = {
	{FILL_CHN2EP_MAP(0, 0x00)},/* no use */
	{FILL_CHN2EP_MAP(1, 0x01)},
	{FILL_CHN2EP_MAP(2, 0x02)},
	{FILL_CHN2EP_MAP(3, 0x03)},
	{FILL_CHN2EP_MAP(4, 0x04)},
	{FILL_CHN2EP_MAP(5, 0x05)},
	{FILL_CHN2EP_MAP(6, 0x06)},
	{FILL_CHN2EP_MAP(7, 0x07)},
	{FILL_CHN2EP_MAP(8, 0x08)},
	{FILL_CHN2EP_MAP(9, 0x09)},
	{FILL_CHN2EP_MAP(10, 0x0A)},
	{FILL_CHN2EP_MAP(11, 0x0B)},
	{FILL_CHN2EP_MAP(12, 0x0C)},
	{FILL_CHN2EP_MAP(13, 0x0D)},
	{FILL_CHN2EP_MAP(14, 0x0E)},
	{FILL_CHN2EP_MAP(15, 0x0F)},
	{FILL_CHN2EP_MAP(16, 0x00)},/* no use */
	/* There is BUG in MUSB_SPRD's inturrpt */
#ifndef NO_EXCHANGE_CHANNEL_17
	{FILL_CHN2EP_MAP(17, 0x8A)},
#else
	{FILL_CHN2EP_MAP(17, 0x81)},
#endif
	{FILL_CHN2EP_MAP(18, 0x82)},
	{FILL_CHN2EP_MAP(19, 0x83)},
	{FILL_CHN2EP_MAP(20, 0x84)},
	{FILL_CHN2EP_MAP(21, 0x85)},
	{FILL_CHN2EP_MAP(22, 0x86)},
	{FILL_CHN2EP_MAP(23, 0x87)},
	{FILL_CHN2EP_MAP(24, 0x88)},
	{FILL_CHN2EP_MAP(25, 0x89)},
#ifndef NO_EXCHANGE_CHANNEL_17
	{FILL_CHN2EP_MAP(26, 0x81)},
#else
	{FILL_CHN2EP_MAP(26, 0x8A)},
#endif
	{FILL_CHN2EP_MAP(27, 0x8B)},
	{FILL_CHN2EP_MAP(28, 0x8C)},
	{FILL_CHN2EP_MAP(29, 0x8D)},
	{FILL_CHN2EP_MAP(30, 0x8E)},
	{FILL_CHN2EP_MAP(31, 0x8F)},
};

#ifndef array_size
#define array_size(x)	(sizeof(x) / sizeof((x)[0]))
#endif

#define chn2ep_table_size()\
	array_size(chn2ep_table)

static struct wcn_usb_store_chn2ep *wcn_usb_store_get_chn2ep(int index)
{
	struct wcn_usb_store_chn2ep *ret;
	int table_size = chn2ep_table_size();

	/* May be we can call unlike */
	if (index >= table_size) {
		wcn_usb_err("%s index[%d] is invalid(table size %d)\n",
			       __func__, index, table_size);
		return NULL;
	}

	ret = chn2ep_table + index;

	return ret;
}

/**
 * wcn_usb_store_get_epFRchn - get a ep describe from a channel id
 * @id: The key we looking for.
 *
 * return: if we find, we return. else NULL.
 */
struct wcn_usb_ep *wcn_usb_store_get_epFRchn(int channel)
{
	struct wcn_usb_store_chn2ep *chn2ep;

	chn2ep = wcn_usb_store_get_chn2ep(channel);
	if (chn2ep == NULL)
		return NULL;
	return chn2ep->ep;
}

struct wcn_usb_work_data *wcn_usb_store_get_channel_info(int channel)
{
	struct wcn_usb_store_chn2ep *chn2ep;

	chn2ep = wcn_usb_store_get_chn2ep(channel);
	if (chn2ep == NULL)
		return NULL;
	return chn2ep->work_data;
}

/**
 * wcn_usb_store_addr2chn() - get a chn describe from a ep
 * @address: The key we looking for.
 *
 * return: if we find, we return a chnnal id. else 0;
 *
 * NOTE: If we take long time in this function, we need build a table for
 * epAddress to chn id.
 */
int wcn_usb_store_addr2chn(__u8 epAddress)
{
	int i;
	int table_size;
	struct wcn_usb_store_chn2ep *chn2ep;

	table_size = chn2ep_table_size();

	for (i = 0; i < table_size; i++) {
		chn2ep = wcn_usb_store_get_chn2ep(i);
		if (chn2ep->epAddress == epAddress)
			return i;
	}
	return -1;
}

__u8 wcn_usb_store_chn2addr(int channel)
{
	struct wcn_usb_store_chn2ep *chn2ep;

	chn2ep = wcn_usb_store_get_chn2ep(channel);
	if (!chn2ep)
		return 0;

	return chn2ep->epAddress;
}


int wcn_usb_store_travel_ep(ep_handle_cb cb, void *pdata)
{
	int i;
	size_t table_size;
	struct wcn_usb_store_chn2ep *chn2ep;
	int ret;

	table_size = chn2ep_table_size();

	for (i = 0; i < table_size; i++) {
		chn2ep = wcn_usb_store_get_chn2ep(i);
		ret = cb(chn2ep->ep, pdata);
		if (ret)
			return ret;
	}

	return ret;
}

static void wcn_usb_state_init(void);
/**
 * wcn_usb_store_init() - init wcn_usb_store memory.
 * @void: void.
 *
 * return: zero for success, or a error number reutrn.
 *
 * Note: This function must be called before the interface driver probe that
 * interface belong to sprd wcn bus usb.
 */
int wcn_usb_store_init(void)
{
	struct wcn_usb_store_chn2ep *chn2ep;
	int table_size = chn2ep_table_size();
	int i;

	for (i = 0; i < table_size; i++) {
		chn2ep = wcn_usb_store_get_chn2ep(i);
		WARN_ON(chn2ep->ep != NULL);

		chn2ep->ep = kzalloc(sizeof(struct wcn_usb_ep), GFP_KERNEL);
		if (!chn2ep->ep)
			return -ENOMEM;

		wcn_usb_ep_init(chn2ep->ep, i);

		chn2ep->work_data = kzalloc(sizeof(struct wcn_usb_work_data),
					GFP_KERNEL);
		if (!chn2ep->work_data)
			return -ENOMEM;

		wcn_usb_work_data_init(chn2ep->work_data, i);
	}

	wcn_usb_state_init();
	wcn_usb_info("%s success\n", __func__);
	return 0;
}

static int wcn_usb_work_data_reset(void)
{
	struct wcn_usb_store_chn2ep *chn2ep;
	int table_size = chn2ep_table_size();
	int i;

	for (i = 0; i < table_size; i++) {
		chn2ep = wcn_usb_store_get_chn2ep(i);

		chn2ep->work_data->report_num_last = 0;
		chn2ep->work_data->transfer_remains = 0;
		chn2ep->work_data->report_num = 0;
	}

	wcn_usb_info("%s success\n", __func__);
	return 0;
}


/**
 * wcn_usb_store_delet() - free wcn_usb_store memory.
 * @void: void.
 *
 * free wcn_usb_store memory.
 *
 * return void.
 *
 * Note: This function must be called later the interface driver free!
 */
void wcn_usb_store_delet(void)
{
	struct wcn_usb_store_chn2ep *chn2ep;
	int table_size = chn2ep_table_size();
	int i;

	for (i = 0; i < table_size; i++) {
		chn2ep = wcn_usb_store_get_chn2ep(i);
		kfree(chn2ep->ep);
	}
}


static ATOMIC_NOTIFIER_HEAD(wcn_usb_state_list);

int wcn_usb_state_sent_event(enum wcn_usb_event event)
{
	wcn_usb_info("%s event:0x%x\n", __func__, event);

	return atomic_notifier_call_chain(&wcn_usb_state_list, event, NULL);
}

int wcn_usb_state_register(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&wcn_usb_state_list, nb);
}

int wcn_usb_state_unregister(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&wcn_usb_state_list, nb);
}

static struct wcn_usb_state {
	struct notifier_block nb;
	unsigned int interface_pluged:3;
	unsigned int downloaded:1;
	unsigned int pwr_state:1;
	unsigned int cp_ready:1;
	unsigned int errored:1;
	unsigned int :0;
} wcn_usb_state;

int wcn_usb_state_get(enum wcn_usb_event event)
{
	struct wcn_usb_state *state = &wcn_usb_state;

	unsigned int interface_id;

	switch (event) {
	case interface_0_plug:
	case interface_1_plug:
	case interface_2_plug:
		interface_id = event - interface_plug_base;
		return (state->interface_pluged & (1 << interface_id)) != 0;
	case dev_plug_fully:
		return state->interface_pluged == 0x7;
	case interface_0_unplug:
	case interface_1_unplug:
	case interface_2_unplug:
		interface_id = event - interface_unplug_base;
		return (state->interface_pluged & (1 << interface_id)) == 0;
	case dev_unplug_fully:
		return state->interface_pluged == 0x7;
	case download_over:
		return state->downloaded == 1;
	case pwr_state:
		return state->pwr_state == 1;
	case cp_ready:
		return state->cp_ready == 1;
	case error_happen:
		return state->errored == 1;
	case error_clean:
		return state->errored == 0;
	default:
		break;
	}
	return 0;
}

static int wcn_usb_state_nb_cb(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct wcn_usb_state *state =
		container_of(nb, struct wcn_usb_state, nb);
	unsigned int interface_id;
	unsigned int interface_pluged_old;

	switch (action) {
	case interface_0_plug:
	case interface_1_plug:
	case interface_2_plug:
		interface_id = action - interface_plug_base;
		interface_pluged_old = state->interface_pluged;
		state->interface_pluged |= 1 << interface_id;
		if (state->interface_pluged == 7 && interface_pluged_old != 7)
			wcn_usb_state_sent_event(dev_plug_fully);
		break;
	case dev_plug_fully:
		break;
	case interface_0_unplug:
	case interface_1_unplug:
	case interface_2_unplug:
		interface_id = action - interface_unplug_base;
		interface_pluged_old = state->interface_pluged;
		state->interface_pluged &= ~(1 << interface_id);
		if (state->interface_pluged == 0 && interface_pluged_old != 0)
			wcn_usb_state_sent_event(dev_unplug_fully);
		break;
	case dev_unplug_fully:
		state->downloaded = 0;
		break;
	case download_over:
		state->downloaded = 1;
		break;
	case pwr_on:
		state->pwr_state = 1;
		break;
	case pwr_off:
		state->pwr_state = 0;
		wcn_usb_work_data_reset();
		break;
	case cp_ready:
		state->cp_ready = 1;
		break;
	case error_happen:
		state->errored = 1;
		break;
	case error_clean:
		state->errored = 0;
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static void wcn_usb_state_init(void)
{
	wcn_usb_state.nb.notifier_call = wcn_usb_state_nb_cb;
	wcn_usb_state_register(&wcn_usb_state.nb);
}
