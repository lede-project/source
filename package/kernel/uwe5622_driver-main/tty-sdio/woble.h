/*
 * btif_woble.h
 *
 *  Created on: 2020
 *      Author: unisoc
 */

#ifndef __WOBLE_H
#define __WOBLE_H

#define HCI_CMD			0x01
#define HCI_ACL			0x02
#define HCI_SCO			0x03
#define HCI_EVT			0x04

#define PACKET_TYPE         0
#define EVT_HEADER_TYPE     0
#define EVT_HEADER_EVENT    1
#define EVT_HEADER_SIZE     2
#define EVT_VENDOR_CODE_LSB 3
#define EVT_VENDOR_CODE_MSB 4
#define EVT_LE_META_SUBEVT  3
#define EVT_ADV_LENGTH      13

#define BT_HCI_EVT_CMD_COMPLETE    0x0e
#define BT_HCI_EVT_CMD_STATUS      0x0f

#define ACL_HEADER_SIZE_LB  3
#define ACL_HEADER_SIZE_HB  4
#define EVT_HEADER_STATUS   4

#define HCI_CMD_MAX_LEN     258
#define BD_ADDR_LEN         6

#define UINT8_TO_STREAM(p, u8)     {*(p)++ = (uint8_t)(u8); }
#define UINT16_TO_STREAM(p, u16)   {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8); }
#define ARRAY_TO_STREAM(p, a, len) {register int ijk; for (ijk = 0; ijk < len; ijk++) *(p)++ = (uint8_t) a[ijk]; }
#define STREAM_TO_UINT8(u8, p)     {u8 = (uint8_t)(*(p)); (p) += 1; }
#define STREAM_TO_UINT16(u16, p)   {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2; }
#define BDADDR_TO_STREAM(p, a)     {register int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk]; }
#define STREAM_TO_ARRAY(a, p, len) {register int ijk; for (ijk = 0; ijk < len; ijk++) ((uint8_t *) a)[ijk] = *p++; }

#define BT_HCI_OP_RESET            0x0c03
#define BT_HCI_OP_ENABLE           0xfca1
#define BT_HCI_OP_WOBLE            0xfd08
#define BT_HCI_OP_SET_SLEEPMODE    0xfd09
#define BT_HCI_OP_ADD_WAKEUPLIST   0xfd0a
#define BT_HCI_OP_SET_STARTSLEEP   0xfd0d

#define WOBLE_DEVICES_SIZE         10

struct HC_BT_HDR {
	unsigned short event;
	unsigned short len;
	unsigned short offset;
	unsigned short layer_specific;
	unsigned char data[];
};

struct hci_cmd_t {
	unsigned short opcode;
	struct semaphore wait;
	struct HC_BT_HDR response;
};

typedef enum {
	WOBLE_MOD_DISABLE = 0,
	WOBLE_MOD_ENABLE,
	WOBLE_MOD_UNDEFINE = 0xff,
} WOBLE_MOD;

typedef enum {
	WOBLE_SLEEP_MOD_COULD_KNOW = 0,
	WOBLE_SLEEP_MOD_COULD_NOT_KNOW
} WOBLE_SLEEP_MOD;

typedef enum {
	WOBLE_SLEEP_MOD_NOT_NEED_NOTITY = 0,
	WOBLE_SLEEP_MOD_NEED_NOTITY,
} WOBLE_NOFITY_MOD;

typedef enum {
	WOBLE_WAKE_MOD_ALL_ADV_DATA = (1 << 0),
	WOBLE_WAKE_MOD_ALL_ACL_DATA = (1 << 1),
	WOBLE_WAKE_MOD_SPECIAL_ADV_DATA = (1 << 2),
	WOBLE_WAKE_MOD_SPECIAL_ACL_DATA = (1 << 3),
} WOBLE_WAKE_MOD;

typedef enum {
	WOBLE_DISCONNECT_MOD_NOT = 0,
	WOBLE_DISCONNECT_MOD_WILL
} WOBLE_DISCONNECT_MOD;

typedef enum {
	WOBLE_ADV_WAKE_MOD_RAW_DATA = (1 << 0),
	WOBLE_ADV_WAKE_MOD_ADTYE = (1 << 1),
} WOBLE_ADV_WAKE_MOD;

typedef enum {
	WOBLE_IS_NOT_SHUTDOWN = 0,
	WOBLE_IS_SHUTDOWN,
} WOBLE_SHUTDOWN_MOD;

typedef enum {
	WOBLE_IS_NOT_RESUME = 0,
	WOBLE_IS_RESUME,
} WOBLE_RESUME_MOD;

typedef struct {
	uint8_t woble_mod;
	uint8_t sleep_mod;
	uint16_t timeout;
	uint8_t notify;
} woble_config_t;

typedef struct mtty_bt_wake_t {
	uint8_t addr[6];
	char *addr_str;
	uint8_t dev_tp;
	char *dev_tp_str;
	uint8_t addr_tp;
	char *addr_tp_str;
} mtty_bt_wake_t;

int woble_init(void);
void hci_set_ap_sleep_mode(int is_shutdown, int is_resume);

int mtty_bt_str_hex(char *str, uint8_t count, char *hex);
int mtty_bt_conf_prase(char *conf_str);
int mtty_bt_read_conf(void);
int woble_data_recv(const unsigned char *buf, int count);
int hci_cmd_send_sync(unsigned short opcode, struct HC_BT_HDR *py, struct HC_BT_HDR *rsp);

void hci_add_device_to_wakeup_list(mtty_bt_wake_t bt_wakeup_dev);
void hci_set_ap_start_sleep(void);
#endif

