/*
 * btif_woble.c
 *
 *  Created on: 2020
 *      Author: Unisoc
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

#include <linux/file.h>
#include <linux/string.h>
#include "woble.h"
#include "tty.h"
#include "alignment/sitm.h"
#include <marlin_platform.h>

#define CMD_TIMEOUT 5000
#define MAX_WAKE_DEVICE_MAX_NUM 36
#define CONFIG_FILE_PATH "/data/misc/bluedroid/bt_config.conf"

static struct hci_cmd_t hci_cmd;
uint8_t device_count_db;
const woble_config_t s_woble_config_cust = {WOBLE_MOD_ENABLE, WOBLE_SLEEP_MOD_COULD_NOT_KNOW, 0, WOBLE_SLEEP_MOD_NOT_NEED_NOTITY};

mtty_bt_wake_t bt_wake_dev_db[MAX_WAKE_DEVICE_MAX_NUM];

int woble_init(void)
{
	memset(&hci_cmd, 0, sizeof(struct hci_cmd_t));
	sema_init(&hci_cmd.wait, 0);

	return 0;
}

int mtty_bt_str_hex(char *str, uint8_t count, char *hex)
{
	uint8_t data_buf[12] = {0x00};
	uint8_t i = 0;
	while ((*str != '\0') && (i < count * 2)) {
		if ((*str >= '0') && (*str <= '9')) {
			data_buf[i] = *str - '0' + 0x00;
			i++;
		} else if ((*str >= 'A') && (*str <= 'F')) {
			data_buf[i] = *str - 'A' + 0x0A;
			i++;
		} else if ((*str >= 'a') && (*str <= 'f')) {
			data_buf[i] = *str - 'a' + 0x0a;
			i++;
		} else {
			;
		}
		str++;
	}
	if (count == 1) {
		*hex = data_buf[0];
	} else {
		for (i = 0; i < count; i++) {
			hex[i] = (data_buf[0 + i * 2] << 4) + data_buf[1 + i * 2];
		}
	}
	for (i = 0; i < count; i++) {
		pr_info("%s data[%d] = %02x\n", __func__, i, hex[i]);
	}
	return 0;
}

int mtty_bt_conf_prase(char *conf_str)
{
	char *tok_str = conf_str;
	char *str1, *str2, *str3;
	uint8_t device_count = 0;
	uint8_t loop_count = 0;
	if (conf_str) {
		while (tok_str != NULL) {
			tok_str = strsep(&conf_str, "\r\n");
			if (!tok_str)
				continue;
			if ((strpbrk(tok_str, "[") != NULL) &&
				(strpbrk(tok_str, "]") != NULL) &&
				(strpbrk(tok_str, ":") != NULL)) {
				bt_wake_dev_db[device_count].addr_str = tok_str;
				continue;
			}
			if (strstr(tok_str, "DevType") != NULL) {
				bt_wake_dev_db[device_count].dev_tp_str = tok_str;
				continue;
			}
			if (strstr(tok_str, "AddrType") != NULL) {
				bt_wake_dev_db[device_count].addr_tp_str = tok_str;
				device_count++;
			}
		}
	} else {
		pr_info("%s conf_str is NULL\n", __func__);
	}
	if (device_count) {
		for (; loop_count < device_count; loop_count++) {
			str1 = strchr(bt_wake_dev_db[loop_count].addr_str, '[');
			str2 = strstrip(bt_wake_dev_db[loop_count].dev_tp_str);
			str2 = strchr(str2, '=');
			str3 = strstrip(bt_wake_dev_db[loop_count].addr_tp_str);
			str3 = strchr(str3, '=');
			mtty_bt_str_hex(str1, 6, bt_wake_dev_db[loop_count].addr);
			mtty_bt_str_hex(str2, 1, &(bt_wake_dev_db[loop_count].dev_tp));
			mtty_bt_str_hex(str3, 1, &(bt_wake_dev_db[loop_count].addr_tp));
		}
	}
	return device_count;
}

int mtty_bt_read_conf(void)
{
	char *buffer = NULL;
	unsigned char *p_buf = NULL;
	struct file *bt_conf_fp;
	unsigned int read_len, buffer_len;
	uint8_t device_count = 0;
	loff_t file_size = 0;
	loff_t file_offset = 0;
	memset(bt_wake_dev_db, 0, sizeof(mtty_bt_wake_t) * MAX_WAKE_DEVICE_MAX_NUM);
	bt_conf_fp = filp_open(CONFIG_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(bt_conf_fp)) {
		pr_info("%s open file %s error %ld \n",
				__func__, CONFIG_FILE_PATH, PTR_ERR(bt_conf_fp));
		return device_count;
	}
	file_size = vfs_llseek(bt_conf_fp, 0, SEEK_END);
	buffer_len = 0;
	buffer = vmalloc(file_size);
	p_buf = buffer;
	if (!buffer) {
		fput(bt_conf_fp);
		pr_info("%s no memory\n", __func__);
		return device_count;
	}

	do {
		read_len = kernel_read(bt_conf_fp, p_buf, file_size, &file_offset);
		if (read_len > 0) {
			buffer_len += read_len;
			file_size -= read_len;
			p_buf += read_len;
		}
	} while ((read_len > 0) && (file_size > 0));

	fput(bt_conf_fp);
	pr_info("%s read %s data_len:0x%x\n",
			__func__, CONFIG_FILE_PATH, buffer_len);
	device_count = mtty_bt_conf_prase(buffer);
	return device_count;
}

int woble_data_recv(const unsigned char *buf, int count)
{
	unsigned short opcode = 0;
	unsigned char status = 0;
	const unsigned char *p = 0;
	const unsigned char *rxmsg = buf;
	int left_length = count;
	int pack_length = 0;
	int last = 0;
	int ret = -1;

	if (count < 0) {
		pr_err("%s count < 0!!!", __func__);
	}

	do {
		rxmsg = buf + (count - left_length);
		switch (rxmsg[PACKET_TYPE]) {
		case HCI_EVT:
			if (left_length < 3) {
				pr_err("%s left_length <3 !!!!!", __func__);
			}
			pack_length = rxmsg[EVT_HEADER_SIZE];
			pack_length += 3;

			if (left_length - pack_length < 0) {
				pr_err("%s left_length - pack_length <0 !!!!!", __func__);

			}
			switch (rxmsg[EVT_HEADER_EVENT]) {
			default:
			case BT_HCI_EVT_CMD_COMPLETE:
				p = rxmsg + 4;
				STREAM_TO_UINT16(opcode, p);
				STREAM_TO_UINT8(status, p);
				break;

			case BT_HCI_EVT_CMD_STATUS:
				p = rxmsg + 5;
				STREAM_TO_UINT16(opcode, p);
				status = rxmsg[3];
				break;
			}
			last = left_length;
			left_length -= pack_length;
			break;
		default:
			left_length = 0;
			break;
		}
	} while (left_length);

	if (hci_cmd.opcode == opcode && hci_cmd.opcode) {
		pr_info("%s opcode: 0x%04X, status: %d\n", __func__, opcode, status);
		up(&hci_cmd.wait);
		ret = 0;
	}
	return ret;
}

int hci_cmd_send_sync(unsigned short opcode, struct HC_BT_HDR *py,
		struct HC_BT_HDR *rsp)
{
	unsigned char msg_req[HCI_CMD_MAX_LEN + BYTE_ALIGNMENT] = {0}, *p;
	int ret = 0;

	p = msg_req;
	UINT8_TO_STREAM(p, HCI_CMD);
	UINT16_TO_STREAM(p, opcode);

	if (py == NULL) {
		UINT8_TO_STREAM(p, 0);
	} else {
		UINT8_TO_STREAM(p, py->len);
		ARRAY_TO_STREAM(p, py->data, py->len);
	}

	hci_cmd.opcode = opcode;
	ret = marlin_sdio_write(msg_req, (p - msg_req) + (BYTE_ALIGNMENT - ((p - msg_req) % BYTE_ALIGNMENT)));
	if (!ret) {
		hci_cmd.opcode = 0;
		pr_err("%s marlin_sdio_write fail", __func__);
		return 0;
	}

	if (down_timeout(&hci_cmd.wait, msecs_to_jiffies(CMD_TIMEOUT))) {
		pr_err("%s CMD_TIMEOUT for CMD: 0x%04X", __func__, opcode);
		mdbg_assert_interface("hci cmd timeout");
	}
	hci_cmd.opcode = 0;

	return 0;
}

void hci_set_ap_sleep_mode(int is_shutdown, int is_resume)
{
	struct HC_BT_HDR *payload = (struct HC_BT_HDR *)vmalloc(sizeof(struct HC_BT_HDR) + 3);
	unsigned char *p;
	p = payload->data;

	payload->len = 6;
	if (is_resume && !is_shutdown) {
		UINT8_TO_STREAM(p, 0);
	} else {
		UINT8_TO_STREAM(p, s_woble_config_cust.woble_mod);
	}

	if (is_shutdown) {
		UINT8_TO_STREAM(p, WOBLE_SLEEP_MOD_COULD_KNOW);
	} else {
		UINT8_TO_STREAM(p, WOBLE_SLEEP_MOD_COULD_NOT_KNOW);
	}

	UINT16_TO_STREAM(p, s_woble_config_cust.timeout);
	UINT8_TO_STREAM(p, s_woble_config_cust.notify);
	UINT8_TO_STREAM(p, 0);

	hci_cmd_send_sync(BT_HCI_OP_SET_SLEEPMODE, payload, NULL);
	vfree(payload);
	return;
}

void hci_add_device_to_wakeup_list(mtty_bt_wake_t bt_wakeup_dev)
{
	struct HC_BT_HDR *payload = (struct HC_BT_HDR *)vmalloc(sizeof(struct HC_BT_HDR) + 3);
	unsigned char *p;
	p = payload->data;

	payload->len = 10;
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr_tp);
	//ARRAY_TO_STREAM(p, bt_wakeup_dev.addr,6);
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr[5]);
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr[4]);
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr[3]);
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr[2]);
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr[1]);
	UINT8_TO_STREAM(p, bt_wakeup_dev.addr[0]);
	UINT16_TO_STREAM(p, 3);
	UINT8_TO_STREAM(p, 0);

	hci_cmd_send_sync(BT_HCI_OP_ADD_WAKEUPLIST, payload, NULL);
	vfree(payload);
	return;
}

void hci_set_ap_start_sleep(void)
{
	struct HC_BT_HDR *payload = (struct HC_BT_HDR *)vmalloc(sizeof(struct HC_BT_HDR) + 3);

	hci_cmd_send_sync(BT_HCI_OP_SET_STARTSLEEP, NULL, NULL);
	vfree(payload);
	return;
}
