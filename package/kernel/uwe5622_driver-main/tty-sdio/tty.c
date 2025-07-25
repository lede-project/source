/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/atomic.h>
#ifdef CONFIG_OF
#include <linux/of_device.h>
#include <linux/of.h>
#endif
#include <linux/compat.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/version.h>

#include <marlin_platform.h>
#include "tty.h"
#include "lpm.h"
#include "rfkill.h"
#include "dump.h"
#include "woble.h"
#include "alignment/sitm.h"

static unsigned int log_level = MTTY_LOG_LEVEL_NONE;

#define BT_VER(fmt, ...)						\
	do {										\
		if (log_level == MTTY_LOG_LEVEL_VER)	\
			pr_err(fmt, ##__VA_ARGS__);			\
	} while (0)


static struct semaphore sem_id;

struct rx_data {
	unsigned int channel;
	struct mbuf_t *head;
	struct mbuf_t *tail;
	unsigned int num;
	struct list_head entry;
};

struct mtty_device {
	struct mtty_init_data   *pdata;
	struct tty_port *port;
	struct tty_struct   *tty;
	struct tty_driver   *driver;

	/* mtty state */
	atomic_t state;
	/*spinlock_t    rw_lock;*/
	struct mutex    rw_mutex;
	struct list_head rx_head;
	/*struct tasklet_struct rx_task;*/
	struct work_struct bt_rx_work;
	struct workqueue_struct *bt_rx_workqueue;
};

static struct mtty_device *mtty_dev;
static unsigned int que_task = 1;
static int que_sche = 1;

static bool is_dumped;
static bool is_user_debug;
bt_host_data_dump *data_dump;

static ssize_t dumpmem_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == 2) {
		pr_info("Set is_user_debug true!\n");
		is_user_debug = true;
		return 0;
	}

	if (is_dumped == false) {
		pr_info("mtty BT start dump cp mem !\n");
		//mdbg_assert_interface("BT command timeout assert !!!");
		bt_host_data_printf();
		if (data_dump != NULL) {
			data_dump = NULL;
			vfree(data_dump);
		}
	} else {
		pr_info("mtty BT has dumped cp mem, pls restart phone!\n");
	}
	is_dumped = true;

	return 0;
}

static DEVICE_ATTR_WO(dumpmem);

static ssize_t chipid_show(struct device *dev,
	   struct device_attribute *attr, char *buf)
{
	int i = 0, id;
	const char *id_str = NULL;

	id = wcn_get_chip_type();
	id_str = wcn_get_chip_name();
	pr_info("%s: chipid: %d, chipid_str: %s", __func__, id, id_str);

	i = scnprintf(buf, PAGE_SIZE, "%d/", id);
	pr_info("%s: buf: %s, i = %d", __func__, buf, i);
	strcat(buf, id_str);
	i += scnprintf(buf + i, PAGE_SIZE - i, buf + i);
	pr_info("%s: buf: %s, i = %d", __func__, buf, i);
	return i;
}

static DEVICE_ATTR_RO(chipid);

static ssize_t ant_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int num = 2;

	num = marlin_get_ant_num();
	pr_err("%s: %d", __func__, num);

	return sprintf(buf, "%d", num);
}

static DEVICE_ATTR_RO(ant_num);

static struct attribute *bluetooth_attrs[] = {
	&dev_attr_dumpmem.attr,
	&dev_attr_chipid.attr,
	&dev_attr_ant_num.attr,
	NULL,
};

static struct attribute_group bluetooth_group = {
	.name = NULL,
	.attrs = bluetooth_attrs,
};

static void hex_dump(unsigned char *bin, size_t binsz)
{
	char *str, hex_str[] = "0123456789ABCDEF";
	size_t i;

	str = (char *)vmalloc(binsz * 3);
	if (!str) {
		return;
	}

	for (i = 0; i < binsz; i++) {
		str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
		str[(i * 3) + 1] = hex_str[(bin[i]) & 0x0F];
		str[(i * 3) + 2] = ' ';
	}
	str[(binsz * 3) - 1] = 0x00;
	pr_info("%s\n", str);
	vfree(str);
}

static void hex_dump_block(unsigned char *bin, size_t binsz)
{
#define HEX_DUMP_BLOCK_SIZE 20
	int loop = binsz / HEX_DUMP_BLOCK_SIZE;
	int tail = binsz % HEX_DUMP_BLOCK_SIZE;
	int i;

	if (!loop) {
		hex_dump(bin, binsz);
		return;
	}

	for (i = 0; i < loop; i++) {
		hex_dump(bin + i * HEX_DUMP_BLOCK_SIZE, HEX_DUMP_BLOCK_SIZE);
	}

	if (tail)
		hex_dump(bin + i * HEX_DUMP_BLOCK_SIZE, tail);
}

/* static void mtty_rx_task(unsigned long data) */
static void mtty_rx_work_queue(struct work_struct *work)
{
	int i, ret = 0;
	/*struct mtty_device *mtty = (struct mtty_device *)data;*/
	struct mtty_device *mtty;
	struct rx_data *rx = NULL;

	que_task = que_task + 1;
	if (que_task > 65530)
		que_task = 0;
	pr_info("mtty que_task= %d\n", que_task);
	que_sche = que_sche - 1;

	mtty = container_of(work, struct mtty_device, bt_rx_work);
	if (unlikely(!mtty)) {
		pr_err("mtty_rx_task mtty is NULL\n");
		return;
	}

	if (atomic_read(&mtty->state) == MTTY_STATE_OPEN) {
		do {
			mutex_lock(&mtty->rw_mutex);
			if (list_empty_careful(&mtty->rx_head)) {
				pr_err("mtty over load queue done\n");
				mutex_unlock(&mtty->rw_mutex);
				break;
			}
			rx = list_first_entry_or_null(&mtty->rx_head,
				struct rx_data, entry);
			if (!rx) {
				pr_err("mtty over load queue abort\n");
				mutex_unlock(&mtty->rw_mutex);
				break;
			}
			list_del(&rx->entry);
			mutex_unlock(&mtty->rw_mutex);

			pr_err("mtty over load working at channel: %d, len: %d\n",
					rx->channel, rx->head->len);
			for (i = 0; i < rx->head->len; i++) {
				ret = tty_insert_flip_char(mtty->port,
						*(rx->head->buf+i), TTY_NORMAL);
				if (ret != 1) {
					i--;
					continue;
				} else {
					tty_flip_buffer_push(mtty->port);
				}
			}
			pr_err("mtty over load cut channel: %d\n", rx->channel);
			kfree(rx->head->buf);
			kfree(rx);

		} while (1);
	} else {
		pr_info("mtty status isn't open, status:%d\n", atomic_read(&mtty->state));
	}
}

static int mtty_rx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	int ret = 0, block_size;
	struct rx_data *rx;
#ifndef WOBLE_FUN
	bt_wakeup_host();
#endif
	block_size = ((head->buf[2] & 0x7F) << 9) + (head->buf[1] << 1) + (head->buf[0] >> 7);

	if (log_level == MTTY_LOG_LEVEL_VER) {
		BT_VER("%s dump head: %d, channel: %d, num: %d\n", __func__, BT_SDIO_HEAD_LEN, chn, num);
		hex_dump_block((unsigned char *)head->buf, BT_SDIO_HEAD_LEN);
		BT_VER("%s dump block %d\n", __func__, block_size);
		hex_dump_block((unsigned char *)head->buf + BT_SDIO_HEAD_LEN, block_size);
	}
	if (is_user_debug) {
		bt_host_data_save((unsigned char *)head->buf + BT_SDIO_HEAD_LEN, block_size, BT_DATA_IN);
	}

	woble_data_recv((unsigned char *)head->buf + BT_SDIO_HEAD_LEN, block_size);

	if (atomic_read(&mtty_dev->state) == MTTY_STATE_CLOSE) {
		pr_err("%s mtty bt is closed abnormally\n", __func__);
		sprdwcn_bus_push_list(chn, head, tail, num);
		return -1;
	}

	if (mtty_dev != NULL) {
		if (!work_pending(&mtty_dev->bt_rx_work)) {
			BT_VER("%s tty_insert_flip_string", __func__);
			ret = tty_insert_flip_string(mtty_dev->port,
				(unsigned char *)head->buf + BT_SDIO_HEAD_LEN,
				block_size);   // -BT_SDIO_HEAD_LEN
			BT_VER("%s ret: %d, len: %d\n", __func__, ret, block_size);
			if (ret)
				tty_flip_buffer_push(mtty_dev->port);
			if (ret == (block_size)) {
				BT_VER("%s send success", __func__);
				sprdwcn_bus_push_list(chn, head, tail, num);
				return 0;
			}
		}

		rx = kmalloc(sizeof(struct rx_data), GFP_KERNEL);
		if (rx == NULL) {
			pr_err("%s rx == NULL\n", __func__);
			sprdwcn_bus_push_list(chn, head, tail, num);
			return -ENOMEM;
		}

		rx->head = head;
		rx->tail = tail;
		rx->channel = chn;
		rx->num = num;
		rx->head->len = (block_size) - ret;
		rx->head->buf = kmalloc(rx->head->len, GFP_KERNEL);
		if (rx->head->buf == NULL) {
			pr_err("mtty low memory!\n");
			kfree(rx);
			sprdwcn_bus_push_list(chn, head, tail, num);
			return -ENOMEM;
		}

		memcpy(rx->head->buf, (unsigned char *)head->buf + BT_SDIO_HEAD_LEN + ret, rx->head->len);
		sprdwcn_bus_push_list(chn, head, tail, num);
		mutex_lock(&mtty_dev->rw_mutex);
		pr_err("mtty over load push %d -> %d, channel: %d len: %d\n",
			block_size, ret, rx->channel, rx->head->len);
		list_add_tail(&rx->entry, &mtty_dev->rx_head);
		mutex_unlock(&mtty_dev->rw_mutex);
		if (!work_pending(&mtty_dev->bt_rx_work)) {
			pr_err("work_pending\n");
			queue_work(mtty_dev->bt_rx_workqueue,
				&mtty_dev->bt_rx_work);
		}
		return 0;
	}
	pr_err("mtty_rx_cb mtty_dev is NULL!!!\n");

	return -1;
}

static int mtty_tx_cb(int chn, struct mbuf_t *head, struct mbuf_t *tail, int num)
{
	int i;
	struct mbuf_t *pos = NULL;
	BT_VER("%s channel: %d, head: %p, tail: %p num: %d\n", __func__, chn, head, tail, num);
	pos = head;
	for (i = 0; i < num; i++, pos = pos->next) {
		kfree(pos->buf);
		pos->buf = NULL;
	}
	if ((sprdwcn_bus_list_free(chn, head, tail, num)) == 0) {
		BT_VER("%s sprdwcn_bus_list_free() success\n", __func__);
		up(&sem_id);
	} else
		pr_err("%s sprdwcn_bus_list_free() fail\n", __func__);

	return 0;
}

static int mtty_open(struct tty_struct *tty, struct file *filp)
{
	struct mtty_device *mtty = NULL;
	struct tty_driver *driver = NULL;
	data_dump = (bt_host_data_dump *)vmalloc(sizeof(bt_host_data_dump));
	memset(data_dump, 0, sizeof(bt_host_data_dump));
	if (tty == NULL) {
		pr_err("mtty open input tty is NULL!\n");
		return -ENOMEM;
	}
	driver = tty->driver;
	mtty = (struct mtty_device *)driver->driver_state;

	if (mtty == NULL) {
		pr_err("mtty open input mtty NULL!\n");
		return -ENOMEM;
	}

	mtty->tty = tty;
	tty->driver_data = (void *)mtty;

	atomic_set(&mtty->state, MTTY_STATE_OPEN);
	que_task = 0;
	que_sche = 0;
	sitm_ini();
	pr_info("mtty_open device success!\n");
	return 0;
}

static void mtty_close(struct tty_struct *tty, struct file *filp)
{
	struct mtty_device *mtty = NULL;

	if (tty == NULL) {
		pr_err("mtty close input tty is NULL!\n");
		return;
	}
	mtty = (struct mtty_device *) tty->driver_data;
	if (mtty == NULL) {
		pr_err("mtty close s tty is NULL!\n");
		return;
	}

	atomic_set(&mtty->state, MTTY_STATE_CLOSE);
	sitm_cleanup();

	if (data_dump != NULL) {
		vfree(data_dump);
		data_dump = NULL;
	}
	pr_info("mtty_close device success !\n");
}

static int mtty_write(struct tty_struct *tty,
		const unsigned char *buf, int count)
{
	int num = 1, ret;
	struct mbuf_t *tx_head = NULL, *tx_tail = NULL;
	unsigned char *block = NULL;

	BT_VER("%s +++\n", __func__);

	if (is_user_debug) {
		bt_host_data_save(buf, count, BT_DATA_OUT);
	}
	if (log_level == MTTY_LOG_LEVEL_VER) {
		BT_VER("%s dump size: %d\n", __func__, count);
		hex_dump_block((unsigned char *)buf, count);
	}

	block = kmalloc(count + BT_SDIO_HEAD_LEN, GFP_KERNEL);

	if (!block) {
		pr_err("%s kmalloc failed\n", __func__);
		return -ENOMEM;
	}
	memset(block, 0, count + BT_SDIO_HEAD_LEN);
	memcpy(block + BT_SDIO_HEAD_LEN, buf, count);
	down(&sem_id);
	ret = sprdwcn_bus_list_alloc(BT_TX_CHANNEL, &tx_head, &tx_tail, &num);
	if (ret) {
		pr_err("%s sprdwcn_bus_list_alloc failed: %d\n", __func__, ret);
		up(&sem_id);
		kfree(block);
		block = NULL;
		return -ENOMEM;
	}
	tx_head->buf = block;
	tx_head->len = count;
	tx_head->next = NULL;

	ret = sprdwcn_bus_push_list(BT_TX_CHANNEL, tx_head, tx_tail, num);
	if (ret) {
		pr_err("%s sprdwcn_bus_push_list failed: %d\n", __func__, ret);
		kfree(tx_head->buf);
		tx_head->buf = NULL;
		sprdwcn_bus_list_free(BT_TX_CHANNEL, tx_head, tx_tail, num);
		return -EBUSY;
	}

	BT_VER("%s ---\n", __func__);
	return count;
}

static  int sdio_data_transmit(uint8_t *data, size_t count)
{
	return mtty_write(NULL, data, count);
}

static ssize_t mtty_write_plus(struct tty_struct *tty, const u8 *buf,
			     size_t count)
{
	return sitm_write(buf, count, sdio_data_transmit);
}

static void mtty_flush_chars(struct tty_struct *tty)
{
}

static unsigned int mtty_write_room(struct tty_struct *tty)
{
	return UINT_MAX;
}

static const struct tty_operations mtty_ops = {
	.open  = mtty_open,
	.close = mtty_close,
	.write = mtty_write_plus,
	.flush_chars = mtty_flush_chars,
	.write_room  = mtty_write_room,
};

static struct tty_port *mtty_port_init(void)
{
	struct tty_port *port = NULL;

	port = kzalloc(sizeof(struct tty_port), GFP_KERNEL);
	if (port == NULL)
		return NULL;
	tty_port_init(port);

	return port;
}

static int mtty_tty_driver_init(struct mtty_device *device)
{
	struct tty_driver *driver;
	int ret = 0;

	device->port = mtty_port_init();
	if (!device->port)
		return -ENOMEM;

	driver = tty_alloc_driver(MTTY_DEV_MAX_NR * 2, 0);
	if (!driver)
		return -ENOMEM;

	/*
	* Initialize the tty_driver structure
	* Entries in mtty_driver that are NOT initialized:
	* proc_entry, set_termios, flush_buffer, set_ldisc, write_proc
	*/
	driver->owner = THIS_MODULE;
	driver->driver_name = device->pdata->name;
	driver->name = device->pdata->name;
	driver->major = 0;
	driver->type = TTY_DRIVER_TYPE_SYSTEM;
	driver->subtype = SYSTEM_TYPE_TTY;
	driver->init_termios = tty_std_termios;
	driver->driver_state = (void *)device;
	device->driver = driver;
	device->driver->flags = TTY_DRIVER_REAL_RAW;
	/* initialize the tty driver */
	tty_set_operations(driver, &mtty_ops);
	tty_port_link_device(device->port, driver, 0);
	ret = tty_register_driver(driver);
	if (ret) {
		tty_driver_kref_put(driver);
		tty_port_destroy(device->port);
		return ret;
	}
	return ret;
}

static void mtty_tty_driver_exit(struct mtty_device *device)
{
	struct tty_driver *driver = device->driver;

	tty_unregister_driver(driver);
	tty_driver_kref_put(driver);
	tty_port_destroy(device->port);
}

__attribute__((unused))
static int mtty_parse_dt(struct mtty_init_data **init, struct device *dev)
{
#ifdef CONFIG_OF
	struct device_node *np = dev->of_node;
	struct mtty_init_data *pdata = NULL;
	int ret;

	pdata = kzalloc(sizeof(struct mtty_init_data), GFP_KERNEL);
	if (!pdata)
	return -ENOMEM;

	ret = of_property_read_string(np,
		"sprd,name",
		(const char **)&pdata->name);
	if (ret)
	goto error;
	*init = pdata;

	return 0;
error:
	kfree(pdata);
	*init = NULL;
	return ret;
#else
	return -ENODEV;
#endif
}

static inline void mtty_destroy_pdata(struct mtty_init_data **init)
{
#if (defined CONFIG_OF) && !(defined OTT_UWE)
	struct mtty_init_data *pdata = *init;

	kfree(pdata);

	*init = NULL;
#else
	return;
#endif
}

#ifdef WOBLE_FUN
static int bt_tx_powerchange(int channel, int is_resume)
{
	unsigned long power_state = marlin_get_power_state();
	pr_info("%s is_resume =%d", __func__, is_resume);
	if (test_bit(MARLIN_BLUETOOTH, &power_state)) {
		if (!is_resume) {
			hci_set_ap_sleep_mode(WOBLE_IS_NOT_SHUTDOWN, WOBLE_IS_NOT_RESUME);
			hci_set_ap_start_sleep();
		}
		if (is_resume) {
			hci_set_ap_sleep_mode(WOBLE_IS_NOT_SHUTDOWN, WOBLE_IS_RESUME);
		}
	}

	return 0;
}
#endif

struct mchn_ops_t bt_rx_ops = {
	.channel = BT_RX_CHANNEL,
	.hif_type = HW_TYPE_SDIO,
	.inout = BT_RX_INOUT,
	.pool_size = BT_RX_POOL_SIZE,
	.pop_link = mtty_rx_cb,
};

struct mchn_ops_t bt_tx_ops = {
	.channel = BT_TX_CHANNEL,
	.hif_type = HW_TYPE_SDIO,
	.inout = BT_TX_INOUT,
	.pool_size = BT_TX_POOL_SIZE,
	.pop_link = mtty_tx_cb,
#ifdef WOBLE_FUN
	.power_notify = bt_tx_powerchange,
#endif
};

static int  mtty_probe(struct platform_device *pdev)
{
	struct mtty_init_data *pdata = (struct mtty_init_data *)
								pdev->dev.platform_data;
	struct mtty_device *mtty;
	int rval = 0;

#ifdef OTT_UWE
	static struct mtty_init_data mtty_driver_data = {
		.name = "ttyBT",
	};

	pdata = &mtty_driver_data;
#else
	if (pdev->dev.of_node && !pdata) {
		rval = mtty_parse_dt(&pdata, &pdev->dev);
		if (rval) {
			pr_err("failed to parse mtty device tree, ret=%d\n",
					rval);
			return rval;
		}
	}
#endif

	mtty = kzalloc(sizeof(struct mtty_device), GFP_KERNEL);
	if (mtty == NULL) {
		mtty_destroy_pdata(&pdata);
		pr_err("mtty Failed to allocate device!\n");
		return -ENOMEM;
	}

	mtty->pdata = pdata;
	rval = mtty_tty_driver_init(mtty);
	if (rval) {
		mtty_tty_driver_exit(mtty);
		kfree(mtty->port);
		kfree(mtty);
		mtty_destroy_pdata(&pdata);
		pr_err("regitster notifier failed (%d)\n", rval);
		return rval;
	}

	pr_info("mtty_probe init device addr: 0x%p\n", mtty);
	platform_set_drvdata(pdev, mtty);

	/*spin_lock_init(&mtty->rw_lock);*/
	atomic_set(&mtty->state, MTTY_STATE_CLOSE);
	mutex_init(&mtty->rw_mutex);
	INIT_LIST_HEAD(&mtty->rx_head);
	/*tasklet_init(&mtty->rx_task, mtty_rx_task, (unsigned long)mtty);*/
	mtty->bt_rx_workqueue =
		create_singlethread_workqueue("SPRDBT_RX_QUEUE");
	if (!mtty->bt_rx_workqueue) {
		pr_err("%s SPRDBT_RX_QUEUE create failed", __func__);
		return -ENOMEM;
	}
	INIT_WORK(&mtty->bt_rx_work, mtty_rx_work_queue);

	mtty_dev = mtty;

//#ifdef KERNEL_VERSION_414
	if (sysfs_create_group(&pdev->dev.kobj,
			&bluetooth_group)) {
		pr_err("%s failed to create bluetooth tty attributes.\n", __func__);
	}
//#endif

	rfkill_bluetooth_init(pdev);
	bluesleep_init();
	woble_init();
	sprdwcn_bus_chn_init(&bt_rx_ops);
	sprdwcn_bus_chn_init(&bt_tx_ops);
	sema_init(&sem_id, BT_TX_POOL_SIZE - 1);

	return 0;
}

int marlin_sdio_write(const unsigned char *buf, int count)
{
	int num = 1, ret;
	struct mbuf_t *tx_head = NULL, *tx_tail = NULL;
	unsigned char *block = NULL, *tmp_buf = (unsigned char *)buf + 1;
	unsigned short op = 0;

	STREAM_TO_UINT16(op, tmp_buf);

	if (buf[0] == 0x1) {
		pr_info("%s +++ op 0x%04X\n", __func__, op);
	}

	block = kmalloc(count + BT_SDIO_HEAD_LEN, GFP_KERNEL);

	if (!block) {
		pr_err("%s kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	if (log_level == MTTY_LOG_LEVEL_VER) {
		pr_info("%s dump size: %d\n", __func__, count);
		hex_dump_block((unsigned char *)buf, count);
	}

	memset(block, 0, count + BT_SDIO_HEAD_LEN);
	memcpy(block + BT_SDIO_HEAD_LEN, buf, count);

	down(&sem_id);
	ret = sprdwcn_bus_list_alloc(BT_TX_CHANNEL, &tx_head, &tx_tail, &num);

	if (ret) {
		pr_err("%s sprdwcn_bus_list_alloc failed: %d\n", __func__, ret);
		up(&sem_id);
		kfree(block);
		block = NULL;
		return -ENOMEM;
	}

	tx_head->buf = block;
	tx_head->len = count;
	tx_head->next = NULL;

	ret = sprdwcn_bus_push_list(BT_TX_CHANNEL, tx_head, tx_tail, num);

	if (ret) {
		pr_err("%s sprdwcn_bus_push_list failed: %d\n", __func__, ret);
		kfree(tx_head->buf);
		tx_head->buf = NULL;
		sprdwcn_bus_list_free(BT_TX_CHANNEL, tx_head, tx_tail, num);
		return -EBUSY;
	}

	BT_VER("%s ---\n", __func__);

	return count;
}

#ifdef WOBLE_FUN
static void  mtty_shutdown(struct platform_device *pdev)
{
	unsigned long int power_state = marlin_get_power_state();
	pr_info("%s ---\n", __func__);
	if (test_bit(MARLIN_BLUETOOTH, &power_state)) {
		pr_info("set bluetooth into sleep mode\n");
		hci_set_ap_sleep_mode(1, 0);
		hci_set_ap_start_sleep();
	}
	return;
}
#endif

static void mtty_remove(struct platform_device *pdev)
{
	struct mtty_device *mtty = platform_get_drvdata(pdev);

	mtty_tty_driver_exit(mtty);
	sprdwcn_bus_chn_deinit(&bt_rx_ops);
	sprdwcn_bus_chn_deinit(&bt_tx_ops);
	kfree(mtty->port);
	mtty_destroy_pdata(&mtty->pdata);
	flush_workqueue(mtty->bt_rx_workqueue);
	destroy_workqueue(mtty->bt_rx_workqueue);
	/*tasklet_kill(&mtty->rx_task);*/
	kfree(mtty);
	platform_set_drvdata(pdev, NULL);
//#ifdef KERNEL_VERSION_414
	sysfs_remove_group(&pdev->dev.kobj, &bluetooth_group);
//#endif
	bluesleep_exit();

	return;
}

static const struct of_device_id mtty_match_table[] = {
	{ .compatible = "sprd,mtty", },
	{ },
};

static struct platform_driver mtty_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "mtty",
		.of_match_table = mtty_match_table,
	},
	.probe = mtty_probe,
	.remove = mtty_remove,
#ifdef WOBLE_FUN
	.shutdown = mtty_shutdown,
#endif
};

#ifdef OTT_UWE
static struct platform_device *mtty_pdev;
static int __init mtty_pdev_init(void)
{
	int ret;
	ret = platform_driver_register(&mtty_driver);
	if (!ret) {
		mtty_pdev = platform_device_alloc("mtty", -1);
		if (platform_device_add(mtty_pdev) != 0)
			pr_err("register platform device unisoc mtty failed\n");
	}

	return ret;
}

static void __exit mtty_pdev_exit(void)
{
	platform_driver_unregister(&mtty_driver);
	platform_device_del(mtty_pdev);
}

module_init(mtty_pdev_init);
module_exit(mtty_pdev_exit);
#else
module_platform_driver(mtty_driver);
#endif

MODULE_AUTHOR("Unisoc wcn bt");
MODULE_DESCRIPTION("Unisoc marlin tty driver");
