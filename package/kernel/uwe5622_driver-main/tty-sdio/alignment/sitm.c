/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
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

#include <linux/slab.h>
#include <linux/kfifo.h>
#include "sitm.h"


static const uint8_t preamble_sizes[] = {
	HCI_COMMAND_PREAMBLE_SIZE,
	HCI_ACL_PREAMBLE_SIZE,
	HCI_SCO_PREAMBLE_SIZE,
	HCI_EVENT_PREAMBLE_SIZE
};

static struct packet_receive_data_t *rd;

int sitm_ini(void)
{
	rd = kmalloc(sizeof(struct packet_receive_data_t),
		GFP_KERNEL);
	memset(rd, 0, sizeof(struct packet_receive_data_t));
	if (kfifo_alloc(&rd->fifo,
		HCI_HAL_SERIAL_BUFFER_SIZE, GFP_KERNEL)) {
		pr_err("no memory for sitm ring buf");
	}
	return 0;
}

int sitm_cleanup(void)
{
	kfifo_free(&rd->fifo);
	kfree(rd);
	rd = NULL;
	return 0;
}



static int data_ready(uint8_t *buf, uint32_t count)
{
	int ret = kfifo_out(&rd->fifo, buf, count);
	return ret;
}

void parse_frame(data_ready_cb data_ready, frame_complete_cb frame_complete)
{
	uint8_t byte;
	size_t buffer_size, bytes_read;

	while (data_ready(&byte, 1) == 1) {
		switch (rd->state) {
		case BRAND_NEW:
			if (byte > DATA_TYPE_EVENT
				|| byte < DATA_TYPE_COMMAND) {
				pr_err("unknown head: 0x%02x\n", byte);
				break;
			}
			rd->type = byte;
			rd->bytes_remaining =
				preamble_sizes[PACKET_TYPE_TO_INDEX(rd->type)]
				+ 1;
			memset(rd->preamble, 0, PREAMBLE_BUFFER_SIZE);
			rd->index = 0;
			rd->state = PREAMBLE;
		case PREAMBLE:
			rd->preamble[rd->index] = byte;
			rd->index++;
			rd->bytes_remaining--;

			if (rd->bytes_remaining == 0) {
				rd->bytes_remaining =
					(rd->type == DATA_TYPE_ACL) ?
					RETRIEVE_ACL_LENGTH(rd->preamble)
					: byte;
				buffer_size = rd->index
					+ rd->bytes_remaining;
				memcpy(rd->buffer,
					rd->preamble,
					rd->index);
				rd->state =
					rd->bytes_remaining > 0 ?
					BODY : FINISHED;
			}
			break;
		case BODY:
			rd->buffer[rd->index] = byte;
			rd->index++;
			rd->bytes_remaining--;
			bytes_read = data_ready((rd->buffer
				+ rd->index),
				rd->bytes_remaining);
			rd->index += bytes_read;
			rd->bytes_remaining -= bytes_read;
			rd->state =
				rd->bytes_remaining == 0 ?
				FINISHED : rd->state;
			break;
		case IGNORE:
			pr_err("PARSE IGNORE\n");
			rd->bytes_remaining--;
			if (rd->bytes_remaining == 0) {
				rd->state = BRAND_NEW;
				return;
			}
			break;
		case FINISHED:
			pr_err("%s state.\n", __func__);
			break;
		default:
			pr_err("PARSE DEFAULT\n");
			break;
		}

		if (rd->state == FINISHED) {
			if (rd->type == DATA_TYPE_COMMAND
				|| rd->type == DATA_TYPE_ACL) {
				uint32_t tail = BYTE_ALIGNMENT
					- ((rd->index
					+ BYTE_ALIGNMENT)
					% BYTE_ALIGNMENT);

				while (tail--)
					rd->buffer[rd->index++] = 0;
			}
			frame_complete(rd->buffer,
				rd->index);
			rd->state = BRAND_NEW;
		}
	}
}

int sitm_write(const uint8_t *buf, int count, frame_complete_cb frame_complete)
{
	int ret;

	if (!rd) {
		pr_err("hci fifo no memory\n");
		return count;
	}

	ret = kfifo_avail(&rd->fifo);
	if (ret == 0) {
		pr_err("hci fifo no memory\n");
		return ret;
	} else if (ret < count) {
		pr_err("hci fifo low memory\n");
		count = ret;
	}

	kfifo_in(&rd->fifo, buf, count);
	parse_frame(data_ready, frame_complete);
	return count;
}


