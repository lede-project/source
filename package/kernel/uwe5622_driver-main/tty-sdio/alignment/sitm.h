#ifndef __SITM_H
#define __SITM_H

#include <linux/types.h>
#include <linux/kfifo.h>

#define PREAMBLE_BUFFER_SIZE 5
#define PACKET_TYPE_TO_INDEX(type) ((type) - 1)
#define HCI_COMMAND_PREAMBLE_SIZE 3
#define HCI_ACL_PREAMBLE_SIZE 4
#define HCI_SCO_PREAMBLE_SIZE 3
#define HCI_EVENT_PREAMBLE_SIZE 2
#define RETRIEVE_ACL_LENGTH(preamble) \
	((((preamble)[4] & 0xFFFF) << 8) | (preamble)[3])
#define HCI_HAL_SERIAL_BUFFER_SIZE 1026

#define BYTE_ALIGNMENT 4

enum receive_state_t {
	BRAND_NEW,
	PREAMBLE,
	BODY,
	IGNORE,
	FINISHED
};

enum serial_data_type_t {
	DATA_TYPE_COMMAND = 1,
	DATA_TYPE_ACL     = 2,
	DATA_TYPE_SCO     = 3,
	DATA_TYPE_EVENT   = 4
};


struct packet_receive_data_t {
	enum receive_state_t state;
	uint16_t bytes_remaining;
	uint8_t type;
	uint8_t preamble[PREAMBLE_BUFFER_SIZE];
	uint16_t index;
	struct kfifo fifo;
	uint8_t buffer[HCI_HAL_SERIAL_BUFFER_SIZE + BYTE_ALIGNMENT];
};

typedef int (*frame_complete_cb)(uint8_t *data, size_t len);
typedef int (*data_ready_cb)(uint8_t *data, uint32_t len);


int sitm_write(const uint8_t *buf, int count, frame_complete_cb frame_complete);
int sitm_ini(void);
int sitm_cleanup(void);
#endif
