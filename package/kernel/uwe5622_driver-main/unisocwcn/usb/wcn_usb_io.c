#include "wcn_usb.h"
#define INTF_LOCK_TIMEOUT 10000

static const struct usb_device_id wcn_usb_io_id_table[] = {
	{ USB_DEVICE(0x1782, 0x4d00) },
	{ USB_DEVICE(0x0403, 0x6011) },
	{ USB_DEVICE(0x04b4, 0x1004) },
	{ USB_DEVICE(0x1782, 0x5d35) },
	{ }					/* Terminating entry */
};

static inline int intf_hold(struct wcn_usb_intf *intf)
{
	unsigned long irq;

	spin_lock_irqsave(&intf->lock, irq);
	if (intf->change_flag == 1) {
		spin_unlock_irqrestore(&intf->lock, irq);
		return -EBUSY;
	}

	intf->num_users++;
	spin_unlock_irqrestore(&intf->lock, irq);
	return 0;
}

static inline void intf_release(struct wcn_usb_intf *intf)
{
	unsigned long irq;

	spin_lock_irqsave(&intf->lock, irq);

	intf->num_users--;
	wake_up(&intf->wait_user);

	spin_unlock_irqrestore(&intf->lock, irq);
}

/* if flags == 1, this function will block! */
static int intf_set_unavailable(struct wcn_usb_intf *intf, int flags)
{
	unsigned long irq;

	spin_lock_irqsave(&intf->lock, irq);

	if (flags) {
		mutex_lock(&intf->flag_lock);
	} else {
		if (!mutex_trylock(&intf->flag_lock)) {
			spin_unlock_irqrestore(&intf->lock, irq);
			return -EBUSY;
		}
	}

	intf->change_flag = 1;
	spin_unlock_irqrestore(&intf->lock, irq);

	if (intf->num_users != 0)
		if (!wait_event_timeout(intf->wait_user, !(intf->num_users),
				INTF_LOCK_TIMEOUT))
			return -ETIMEDOUT;
	return 0;
}

static void intf_set_available(struct wcn_usb_intf *intf)
{
	unsigned long irq;

	spin_lock_irqsave(&intf->lock, irq);
	intf->change_flag = 0;
	spin_unlock_irqrestore(&intf->lock, irq);
	mutex_unlock(&intf->flag_lock);
}

static int ep_hold_intf(struct wcn_usb_ep *ep)
{
	unsigned long irq;
	int ret = -ENODEV;

	spin_lock_irqsave(&ep->intf_lock, irq);

	if (ep->intf)
		ret = intf_hold(ep->intf);

	spin_unlock_irqrestore(&ep->intf_lock, irq);
	return ret;
}

static void ep_release_intf(struct wcn_usb_ep *ep)
{
	intf_release(ep->intf);
}

/* please guarantee intf is not NULL! */
struct usb_endpoint_descriptor *wcn_usb_intf2endpoint(struct wcn_usb_intf *intf,
					__u8 numEp)
{
	struct usb_endpoint_descriptor *endpoint;
	struct usb_host_interface *iface_desc;

	if (numEp > wcn_usb_intf_epNum(intf))
		return NULL;

	iface_desc = intf->interface->cur_altsetting;
	endpoint = &iface_desc->endpoint[numEp].desc;

	return endpoint;
}

/* we can make it faster, If we have to */
static int __intf_fill_store_callback(struct wcn_usb_ep *ep, void *data)
{
	struct wcn_usb_intf *intf;
	__u8 epAddress;
	int epNum;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	unsigned long irq;

	intf = (struct wcn_usb_intf *)data;
	epAddress = wcn_usb_store_chn2addr(ep->channel);
	epNum = wcn_usb_intf_epNum(intf);

	for (i = 0; i < epNum; i++) {
		endpoint = wcn_usb_intf2endpoint(intf, i);
		if (endpoint->bEndpointAddress == epAddress) {
			wcn_usb_info("%s ep will be register %x\n", __func__,
					epAddress);
			spin_lock_irqsave(&ep->intf_lock, irq);
			ep->intf = intf;
			spin_unlock_irqrestore(&ep->intf_lock, irq);
			ep->numEp = i;
			break;
		}
	}
	return 0;
}

void wcn_usb_ep_stop(struct wcn_usb_ep *ep)
{
	int ret;
	unsigned long flags;

	ret = ep_hold_intf(ep);
	if (ret) {
		wcn_usb_err("%s get lock error\n", __func__);
		return;
	}

	/* the lock of kernel is Unreliable!! so we do it */
	spin_lock_irqsave(&ep->submit_lock, flags);
	usb_kill_anchored_urbs(&ep->submitted);
	spin_unlock_irqrestore(&ep->submit_lock, flags);

	ep_release_intf(ep);
}

static int __intf_erase_store_callback(struct wcn_usb_ep *ep, void *data)
{
	struct wcn_usb_intf *intf;
	__u8 epAddress;
	int epNum;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	unsigned long irq;

	intf = (struct wcn_usb_intf *)data;
	epAddress = wcn_usb_store_chn2addr(ep->channel);
	epNum = wcn_usb_intf_epNum(intf);

	for (i = 0; i < epNum; i++) {
		endpoint = wcn_usb_intf2endpoint(intf, i);
		if (endpoint->bEndpointAddress == epAddress) {
			wcn_usb_info("%s ep will be unregister %x\n", __func__,
					epAddress);
			spin_lock_irqsave(&ep->intf_lock, irq);
			ep->intf = NULL;
			spin_unlock_irqrestore(&ep->intf_lock, irq);
			ep->numEp = -1;
			wcn_usb_ep_stop(ep);
			break;
		}
	}
	return 0;
}

static int wcn_usb_intf_fill_store(struct wcn_usb_intf *intf)
{
	return wcn_usb_store_travel_ep(__intf_fill_store_callback, intf);
}

static int wcn_usb_intf_erase_store(struct wcn_usb_intf *intf)
{
	return wcn_usb_store_travel_ep(__intf_erase_store_callback, intf);
}

int wcn_usb_ep_can_dma(struct wcn_usb_ep *ep)
{
	int ret;

	ret = ep_hold_intf(ep);
	if (ret) {
		wcn_usb_err("%s get lock error\n", __func__);
		return 0;
	}

	if (wcn_usb_ep_is_isoc(ep) || !wcn_usb_ep_no_sg_constraint(ep))
		ret = 0;
	else
		ret = 1;

	ep_release_intf(ep);
	return 1;
}

unsigned int wcn_usb_packet_recv_len(struct wcn_usb_packet *packet)
{
	int ret;
	unsigned int len = 0;
	int i;

	ret = ep_hold_intf(packet->ep);
	if (ret) {
		wcn_usb_err("%s ep lock set failed\n", __func__);
		return 0;
	}

	if (wcn_usb_ep_is_isoc(packet->ep)) {
		for (i = 0; i < packet->urb->number_of_packets; i++)
			len += packet->urb->iso_frame_desc[i].actual_length;
	} else {
		len = packet->urb->actual_length;
	}

	ep_release_intf(packet->ep);
	return len;
}

/* unsigned int  *pip is ret value */
unsigned int wcn_usb_ep_pipe(struct wcn_usb_ep *ep, unsigned int *pip)
{
	struct usb_endpoint_descriptor *endpoint;
	int ret;

	ret = ep_hold_intf(ep);
	if (ret) {
		wcn_usb_err("%s hold intf fail %d\n", __func__, ret);
		return ret;
	}

	endpoint = wcn_usb_intf2endpoint(ep->intf, ep->numEp);
	if (usb_endpoint_is_bulk_in(endpoint))
		*pip = usb_rcvbulkpipe(ep->intf->udev,
				endpoint->bEndpointAddress);
	if (usb_endpoint_is_bulk_out(endpoint))
		*pip = usb_sndbulkpipe(ep->intf->udev,
				endpoint->bEndpointAddress);
	if (usb_endpoint_is_int_in(endpoint))
		*pip = usb_rcvintpipe(ep->intf->udev,
				endpoint->bEndpointAddress);
	if (usb_endpoint_is_int_out(endpoint))
		*pip = usb_sndintpipe(ep->intf->udev,
				endpoint->bEndpointAddress);
	if (usb_endpoint_is_isoc_in(endpoint))
		*pip = usb_rcvisocpipe(ep->intf->udev,
				endpoint->bEndpointAddress);
	if (usb_endpoint_is_isoc_out(endpoint))
		*pip = usb_sndisocpipe(ep->intf->udev,
				endpoint->bEndpointAddress);

	if (usb_endpoint_xfer_control(endpoint) &&
			usb_endpoint_dir_in(endpoint))
		*pip = usb_rcvctrlpipe(ep->intf->udev,
				endpoint->bEndpointAddress);
	if (usb_endpoint_xfer_control(endpoint) &&
			usb_endpoint_dir_out(endpoint))
		*pip = usb_sndctrlpipe(ep->intf->udev,
				endpoint->bEndpointAddress);

	ep_release_intf(ep);
	return 0;
}

void wcn_usb_io_urb_callback(struct urb *urb)
{
	struct wcn_usb_packet *packet;

	packet = urb->context;

	wcn_usb_print_packet(packet);
	packet->callback(packet);
}

static int wcn_usb_ep_set_packet(struct wcn_usb_ep *ep,
		struct wcn_usb_packet *packet,
		gfp_t mem_flags)
{
	int ret;

	packet->ep = ep;
	packet->urb->dev = ep->intf->udev;
	ret = wcn_usb_ep_pipe(ep, &packet->urb->pipe);
	if (ret != 0)
		return ret;

	packet->urb->context = packet;
	packet->urb->complete = wcn_usb_io_urb_callback;
	if (wcn_usb_ep_is_isoc(ep) || wcn_usb_ep_is_int(ep))
		wcn_usb_packet_interval(packet, wcn_usb_ep_interval(ep));

	return 0;
}

struct wcn_usb_packet *wcn_usb_alloc_packet(gfp_t mem_flags)
{
	struct wcn_usb_packet *packet;

	packet = kzalloc(sizeof(struct wcn_usb_packet), mem_flags);
	if (packet == NULL)
		return NULL;

	packet->urb = usb_alloc_urb(0, mem_flags);
	if (packet->urb == NULL)
		goto PACKET_FREE;

	return packet;

PACKET_FREE:
	kfree(packet);
	return NULL;
}

int wcn_usb_packet_bind(struct wcn_usb_packet *packet, struct wcn_usb_ep *ep,
		gfp_t mem_flags)
{
	int ret;

	ret = ep_hold_intf(ep);
	if (ret) {
		wcn_usb_err("%s hold intf fail %d\n", __func__, ret);
		return ret;
	}

	ret = wcn_usb_ep_set_packet(ep, packet, mem_flags);
	if (ret)
		wcn_usb_err("%s hold ep set packet failed %d\n", __func__, ret);

	ep_release_intf(ep);
	return ret;
}

/* return number_of_packets */
int wcn_usb_frame_fill(struct usb_iso_packet_descriptor *iso_frame_desc,
				int len, int mtu, int max_number_of_packets)
{
	int i, offset = 0;

	for (i = 0; i < max_number_of_packets && len >= mtu;
					i++, offset += mtu, len -= mtu) {
		iso_frame_desc[i].offset = offset;
		iso_frame_desc[i].length = mtu;
	}

	if (len && i < max_number_of_packets) {
		iso_frame_desc[i].offset = offset;
		iso_frame_desc[i].length = len;
		i++;
	}

	return i;
}

int wcn_usb_packet_is_freed(struct wcn_usb_packet *packet)
{
	if (atomic_read(&(&packet->urb->kref)->refcount) != 1)
		return 0;
	return 1;
}

void wcn_usb_packet_clean(struct wcn_usb_packet *packet)
{
	/* keep urb */
	/* clean other field */
	memset((void *)packet + sizeof(void *), 0,
			sizeof(struct wcn_usb_packet) - sizeof(void *));
	if (packet->urb)
		usb_init_urb(packet->urb);
}

int  wcn_usb_packet_set_buf(struct wcn_usb_packet *packet,
				void *buf, ssize_t buf_size,  gfp_t mem_flags)
{
	int ret;
	int max_number_of_packets;

	ret = ep_hold_intf(packet->ep);
	if (ret) {
		wcn_usb_err("%s hold intf fail %d\n", __func__, ret);
		return ret;
	}

	if (wcn_usb_ep_is_isoc(packet->ep)) {
		if (!wcn_usb_ep_packet_max(packet->ep)) {
			ep_release_intf(packet->ep);
			if (wcn_usb_ep_set(packet->ep, 1)) {
				wcn_usb_err("%s maxSize is zero, can't set\n",
						__func__);
				return -EIO;
			}
			ret = ep_hold_intf(packet->ep);
			if (ret) {
				wcn_usb_err("%s hold intf fail %d\n",
						__func__, ret);
				return ret;
			}
		}

		usb_free_urb(packet->urb);

		max_number_of_packets = buf_size /
			wcn_usb_ep_packet_max(packet->ep) + 1;
		packet->urb = usb_alloc_urb(max_number_of_packets, mem_flags);
		if (!packet->urb) {
			wcn_usb_err("%s usb_alloc_urb failed\n", __func__);
			ep_release_intf(packet->ep);
			return -ENOMEM;
		}

		if (wcn_usb_ep_set_packet(packet->ep, packet, mem_flags)) {
			ep_release_intf(packet->ep);
			return -ENODEV;
		}

		packet->urb->transfer_flags = URB_ISO_ASAP;
		packet->urb->number_of_packets = wcn_usb_frame_fill(
					packet->urb->iso_frame_desc,
					buf_size,
					wcn_usb_ep_packet_max(packet->ep),
					max_number_of_packets
					);
	}

	packet->urb->transfer_buffer = buf;
	packet->urb->transfer_buffer_length = buf_size;

	ep_release_intf(packet->ep);
	return 0;
}

void *wcn_usb_packet_pop_buf(struct wcn_usb_packet *packet)
{
	void *ret;

	if (!packet && !packet->urb)
		return NULL;
	ret = packet->urb->transfer_buffer;
	packet->urb->transfer_buffer = NULL;
	return ret;
}


#if 0
/* reserved this function */
void *wcn_usb_packet_alloc_buf(struct wcn_usb_packet *packet,
				ssize_t buf_size,  gfp_t mem_flags)
{
	/* alloc a urb */
	/* call a usb_alloc_coherent to get buf */
	/* set packet->is_usb_dma */
	/* fill urb */
	return NULL;
}

/* only for wcn_usb_packet_alloc_buf */
void wcn_usb_packet_free_buf(struct wcn_usb_packet *packet)
{
	/* free alloc_buf */
	/* free urb */
}
#endif

int wcn_usb_packet_set_setup_packet(struct wcn_usb_packet *packet,
					struct usb_ctrlrequest *setup_packet)
{
	if (!packet && !packet->urb)
		return -EINVAL;
	packet->urb->setup_packet = (char *)setup_packet;
	return 0;
}

struct usb_ctrlrequest *wcn_usb_packet_pop_setup_packet(
						struct wcn_usb_packet *packet)
{
	struct usb_ctrlrequest *ret;

	if (!packet && !packet->urb)
		return NULL;
	ret = (struct usb_ctrlrequest *)packet->urb->setup_packet;
	packet->urb->setup_packet = NULL;
	return ret;
}

int wcn_usb_packet_set_sg(struct wcn_usb_packet *packet, struct scatterlist *sg,
				int num_sgs, unsigned int buf_len)
{
	int ret;

	if (!packet && !packet->urb)
		return -EINVAL;

	/* that mean clean sg arg */
	if (!sg) {
		packet->urb->sg = NULL;
		packet->urb->num_sgs = 0;
		packet->urb->transfer_buffer_length = 0;
		return 0;
	}

	ret = ep_hold_intf(packet->ep);
	if (ret) {
		wcn_usb_err("%s hold intf fail %d\n", __func__, ret);
		return ret;
	}

	if (wcn_usb_ep_is_isoc(packet->ep)) {
		wcn_usb_err("%s sg can be used in isoc\n", __func__);
		ep_release_intf(packet->ep);
		return -EINVAL;
	}

	ep_release_intf(packet->ep);

	packet->urb->sg = sg;
	packet->urb->num_sgs = num_sgs;
	packet->urb->transfer_buffer_length = buf_len;

	return 0;
}

struct scatterlist *wcn_usb_packet_pop_sg(struct wcn_usb_packet *packet,
					int *num_sgs)
{
	struct scatterlist *sg;

	if (num_sgs)
		*num_sgs = packet->urb->num_sgs;
	packet->urb->num_sgs = 0;
	sg = packet->urb->sg;
	packet->urb->sg = NULL;
	return sg;
}

void wcn_usb_packet_free(struct wcn_usb_packet *packet)
{
	usb_unanchor_urb(packet->urb);
	usb_free_urb(packet->urb);
	kfree(packet);
}

/* wcn_usb_packet_set_buf and wcn_usb_packet_alloc_buf will fill
 * interval and iso_frame_desc as default. (dependent on ep descriptive char)
 * and they didn't fill setup_packet for contrl transfer.
 * so, If you need change setup_packet or iso_frame_desc or interval, you need
 * function as follow.
 */
int wcn_usb_packet_frame_desc(struct wcn_usb_packet *packet,
				const struct usb_iso_packet_descriptor *frame,
				 int framNum)
{
	if (framNum > packet->urb->number_of_packets)
		return -EINVAL;
	memcpy(packet->urb->iso_frame_desc, frame,
			sizeof(struct usb_iso_packet_descriptor) * framNum);
	packet->urb->number_of_packets = framNum;
	return 0;
}

void wcn_usb_packet_interval(struct wcn_usb_packet *packet, int interval)
{
	if (packet->urb->dev->speed == USB_SPEED_HIGH ||
		packet->urb->dev->speed == USB_SPEED_SUPER) {
		interval = clamp(interval, 1, 16);
		packet->urb->interval = 1 << (interval - 1);
	} else {
		packet->urb->interval = interval;
	}

	packet->urb->start_frame = -1;
}

static int wcn_usb_need_zlp(struct wcn_usb_ep *ep, ssize_t buf_size)
{
	if ((wcn_usb_ep_is_int(ep) || wcn_usb_ep_is_bulk(ep)) &&
		!wcn_usb_ep_dir(ep) && !(buf_size % wcn_usb_ep_packet_max(ep)))
		return 1;
	return 0;
}

static void zlp_callback(struct urb *urb)
{
	kfree(urb->transfer_buffer);
	usb_free_urb(urb);
}

/*
 * because musb_sprd cant sent zero length packet!
 * we sent a 1 byte length packet instead of zlp.
 */
int wcn_usb_send_zlp(struct wcn_usb_ep *ep, gfp_t mem_flags)
{
	struct urb *urb;
	struct usb_endpoint_descriptor *endpoint;
	void *buf;

	buf = kzalloc(1, GFP_ATOMIC);
	if (buf == NULL)
		return -ENOMEM;

	endpoint = wcn_usb_intf2endpoint(ep->intf, ep->numEp);
	urb = usb_alloc_urb(0, mem_flags);
	if (urb == NULL) {
		kfree(buf);
		return -ENOMEM;
	}
	usb_fill_bulk_urb(urb, ep->intf->udev,
	usb_sndbulkpipe(ep->intf->udev, endpoint->bEndpointAddress),
			buf, 1, zlp_callback, NULL);
	usb_anchor_urb(urb, &ep->submitted);
	return usb_submit_urb(urb, mem_flags);
}

/* We have not touch packet after we submit it!!! */
int wcn_usb_packet_submit(struct wcn_usb_packet *packet,
		void (*callback)(struct wcn_usb_packet *packet),
		void *pdata, gfp_t mem_flags)
{
	int ret;
	struct wcn_usb_ep *ep;
	ssize_t buf_size;
	unsigned long flags;

	if (packet == NULL || callback == NULL || packet->ep == NULL)
		return -EINVAL;

	wcn_usb_debug("%s be called\n", __func__);
	packet->pdata = pdata;
	packet->callback = callback;

	ret = ep_hold_intf(packet->ep);
	if (ret) {
		wcn_usb_err("%s lock error\n", __func__);
		return ret;
	}

	/*debug*/
	if (wcn_usb_state_get(error_happen)) {
		ep_release_intf(packet->ep);
		return -EIO;
	}

	spin_lock_irqsave(&packet->ep->submit_lock, flags);
	ep = packet->ep;
	buf_size = packet->urb->transfer_buffer_length;

	usb_anchor_urb(packet->urb, &packet->ep->submitted);

	wcn_usb_print_packet(packet);
	ret = usb_submit_urb(packet->urb, mem_flags);
	if (ret)
		wcn_usb_err("%s send urb error %d\n", __func__, ret);

	if (wcn_usb_need_zlp(ep, buf_size)) {
		ret = wcn_usb_send_zlp(ep, mem_flags);
		if (ret)
			wcn_usb_err("%s send zlp error %d\n", __func__, ret);
	}

	spin_unlock_irqrestore(&packet->ep->submit_lock, flags);
	ep_release_intf(ep);

	return ret;
}

int wcn_usb_msg(struct wcn_usb_ep *ep, void *data, int len,
		int *actual_length, int timeout)
{
	int ret;
	unsigned int pip;

	ret = ep_hold_intf(ep);
	if (ret)
		return ret;

	ret = wcn_usb_ep_pipe(ep, &pip);
	if (ret)
		goto OUT;

	if (wcn_usb_ep_is_int(ep)) {
		ret = usb_interrupt_msg(ep->intf->udev, pip, data,
				len, actual_length, timeout);
	} else if (wcn_usb_ep_is_bulk(ep)) {
		ret = usb_bulk_msg(ep->intf->udev, pip, data,
				len, actual_length, timeout);
	} else {
		wcn_usb_err("%s only int bulk can be sent with msg", __func__);
		ret = -EIO;
	}

OUT:
	ep_release_intf(ep);
	if (ret)
		wcn_usb_err("%s error!\n", __func__);
	return ret;
}

int wcn_usb_packet_get_status(struct wcn_usb_packet *packet)
{
	int i;
	int ret = 0;

	ret = ep_hold_intf(packet->ep);
	if (ret) {
		wcn_usb_err("%s hold intf fail %d\n", __func__, ret);
		return ret;
	}

	if (!wcn_usb_ep_is_isoc(packet->ep) || packet->urb->status < 0) {
		ret = packet->urb->status;
		goto RETURN_STATUS;
	}

	for (i = 0; i < packet->urb->number_of_packets; i++) {
		ret = packet->urb->iso_frame_desc[i].status;
		if (ret < 0) {
			wcn_usb_err("%s packet %d status %d\n",
					__func__, i, ret);
			goto RETURN_STATUS;
		}
	}

RETURN_STATUS:
	ep_release_intf(packet->ep);
	return ret;
}

/* This function couldn't be called in context that blow intf_hold !*/
int wcn_usb_ep_set(struct wcn_usb_ep *ep, int setting_id)
{
	int ret;
	struct wcn_usb_intf *intf;
	__u8 intrNumber;
	struct usb_device *udev;

	ret = ep_hold_intf(ep);
	if (ret) {
		wcn_usb_err("%s intf get failed\n", __func__);
		return -EBUSY;
	}

	intf = ep->intf;
	if (!intf) {
		ep_release_intf(ep);
		wcn_usb_err("%s intf is null\n", __func__);
		return -ENODEV;
	}

	intrNumber = intf->interface->cur_altsetting->desc.bInterfaceNumber;

	udev = usb_get_dev(ep->intf->udev);

	ep_release_intf(ep);

	ret = intf_set_unavailable(intf, 0);
	if (ret) {
		wcn_usb_err("%s intf_set_unavailable failed\n", __func__);
		usb_put_dev(udev);
		return ret;
	}

	wcn_usb_err("%s %d\n", __func__, __LINE__);
	wcn_usb_intf_erase_store(intf);

	/* TODO check this */
	ret = usb_set_interface(udev, intrNumber, setting_id);

	wcn_usb_intf_fill_store(intf);

	intf_set_available(intf);
	usb_put_dev(udev);
	return ret;
}

/* we don't need usb usb major number to find interface */
#if 0
#define USB_SWCN_MINOR_BASE 123
/* TODO this fops need fill! Plan is that: fill it with io_dbg_fop */
static struct usb_class_driver wcn_usb_io_class = {
	.name = "sprd_wcn_io_%d",
	.fops = NULL,
	.minor_base = USB_SWCN_MINOR_BASE,
};
#endif

static int wcn_usb_io_probe(struct usb_interface *interface,
				const struct usb_device_id *id)
{
	/* init a struct wcn_usb_intf and fill it! */
	struct wcn_usb_intf *intf;

	intf = kzalloc(sizeof(struct wcn_usb_intf), GFP_KERNEL);
	if (!intf)
		return -ENOMEM;

	usb_set_intfdata(interface, intf);

	spin_lock_init(&intf->lock);
	mutex_init(&intf->flag_lock);
	init_waitqueue_head(&intf->wait_user);

	intf->interface = usb_get_intf(interface);
	intf->udev = usb_get_dev(interface_to_usbdev(interface));
	/* register struct wcn_usb_intf */
	wcn_usb_intf_fill_store(intf);

	wcn_usb_info("interface[%x] is register\n",
			interface->cur_altsetting->desc.bInterfaceNumber);

	wcn_usb_state_sent_event(interface_plug_base +
			interface->cur_altsetting->desc.bInterfaceNumber);

	return 0;
}

static void wcn_usb_io_disconnect(struct usb_interface *interface)
{
	/* we must clean all thing!
	 * this driver may be probe and disconnect so much times!
	 */
	struct wcn_usb_intf *intf;

	wcn_usb_state_sent_event(interface_unplug_base +
			interface->cur_altsetting->desc.bInterfaceNumber);

	wcn_usb_info("interface[%x] will be unregister\n",
			interface->cur_altsetting->desc.bInterfaceNumber);
	intf = usb_get_intfdata(interface);

	/* this lock must give me! */
	intf_set_unavailable(intf, 1);

	usb_set_intfdata(interface, NULL);
	wcn_usb_intf_erase_store(intf);
	usb_put_dev(intf->udev);
	usb_put_intf(interface);

	kfree(intf);
}

static int wcn_usb_io_pre_reset(struct usb_interface *interface)
{
	/* there is a lock to prevent we reset a interface when
	 * urb submit
	 */
	struct wcn_usb_intf *intf;

	intf = usb_get_intfdata(interface);
	intf_set_unavailable(intf, 1);

	return 0;
}

static int wcn_usb_io_post_reset(struct usb_interface *interface)
{
	/* free the lock that we take in wcn_usb_io_pre_reset */
	struct wcn_usb_intf *intf;

	intf = usb_get_intfdata(interface);
	intf_set_available(intf);
	return 0;
}

struct usb_driver wcn_usb_io_driver = {
	.name = "wcn_usb_io",
	.probe = wcn_usb_io_probe,
	.disconnect = wcn_usb_io_disconnect,
	.pre_reset = wcn_usb_io_pre_reset,
	.post_reset = wcn_usb_io_post_reset,
	.id_table = wcn_usb_io_id_table,
	.supports_autosuspend = 1,
};


/**
 * wcn_usb_io_init() - init wcn_usb_io's memory and register this driver.
 * @void: void.
 *
 * return: zero for success, or a error number return.
 * NOTE: we can't use module_usb_driver to register our driver.
 * Because this driver have a connection order with other
 */
int wcn_usb_io_init(void)
{
	return usb_register(&wcn_usb_io_driver);
}

void wcn_usb_io_delet(void)
{
}
