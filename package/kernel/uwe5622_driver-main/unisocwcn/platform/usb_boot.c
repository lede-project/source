#include "usb_boot.h"

#define DOWNLOAD_TX_CHN 2
#define DOWNLOAD_RX_CHN 18

static int ackncmp(const char *cs, const char *ct, size_t count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		count--;
	}
	return 0;
}

static int usb_download_sent_command(const char *command,
		unsigned int command_len, const char *ack, unsigned int ack_len)
{
	struct wcn_usb_ep *tx_ep = wcn_usb_store_get_epFRchn(DOWNLOAD_TX_CHN);
	struct wcn_usb_ep *rx_ep = wcn_usb_store_get_epFRchn(DOWNLOAD_RX_CHN);
	int actual_len;
	int ret;
	char *command_temp;
	void *data;
	int data_size = 128;

	if (!tx_ep || !rx_ep) {
		wcn_usb_err("no ep\n");
		return -EIO;
	}

	data = kzalloc(data_size, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	command_temp = kmalloc(command_len, GFP_KERNEL);
	if (!command_temp) {
		ret = -ENOMEM;
		goto OUT_FREE_DATA;
	}

	memcpy(command_temp, command, command_len);
	/* sent command */
	ret = wcn_usb_msg(tx_ep, command_temp, command_len, &actual_len, 3000);
	if (ret || actual_len != command_len) {
		wcn_usb_err("wcn %s sent msg is error [ret %d actual_len %d command_len %d\n",
				__func__, ret, actual_len, command_len);
		ret = -EIO;
		goto OUT;
	}

	if (ack == NULL)
		goto OUT;

	/* get ack and compare */
	ret = wcn_usb_msg(rx_ep, data, data_size, &actual_len, 3000);
	if (ret || ack_len > actual_len || ackncmp(ack, data, ack_len)) {
		wcn_usb_err("wcn %s ack is not ok\n", __func__);
		print_hex_dump(KERN_DEBUG, "WCN ack", 0, 16, 1,
				data, data_size, 1);
		print_hex_dump(KERN_DEBUG, "WCN command", 0, 16, 1,
				command, command_len, 1);
		ret = -EIO;
		goto OUT;
	}

OUT:
	kfree(command_temp);
OUT_FREE_DATA:
	kfree(data);

	return ret;
}

static unsigned short crc16_xmodem_cal(unsigned char *buf, unsigned int buf_len)
{
	unsigned int i;
	unsigned short crc = 0;

	while (buf_len-- != 0) {
		for (i = 0x80; i != 0; i = i >> 1) {
			if ((crc & 0x8000) != 0) {
				crc = crc << 1;
				crc = crc ^ 0x1021;
			} else {
				crc = crc << 1;
			}

			if ((*buf & i) != 0)
				crc = crc ^ 0x1021;
		}

		buf++;
	}

	return crc;
}

static const char get_version[] = { 0x7E };
static const char get_version_ack[] = {
	0x7E, 0x00, 0x81, 0x00, 0x06, 0x53, 0x50,
	0x52, 0x44, 0x33, 0x00, 0x57, 0x0A, 0x7E};
static const char connect[] = {
	0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E};
static const char common_ack[] = {
	0x7E, 0x00, 0x80, 0x00, 0x00, 0x3B, 0x5A, 0x7E};
static const char end_download[] = {
	0x7E, 0x00, 0x03, 0x00, 0x00, 0x59, 0x50, 0x7E};

unsigned short usb_download_command_replace(char *command, unsigned short len)
{
	unsigned short tail = len - 1;
	unsigned short i;
	unsigned short j;

	/* ho head and no tail */
	for (i = len - 2; i > 0; i--) {
		if (command[i] == 0x7E || command[i] == 0x7D) {
			tail += 1;
			for (j = tail; j > i; j--)
				command[j] = command[j - 1];
			command[i + 1] = 0x50 | (command[i + 1] & 0x0F);
			command[i] = 0x7D;
		}
	}

	return tail + 1;
}

int usb_download_command_start_download(unsigned int addr, unsigned int len)
{
	unsigned short command_len = 16;
	char command[20] = {0x7E,/* head */
		0x00, 0x01, /*command type */
		0x00, 0x08, /*for command data len */
		0x00, 0x00, 0x00, 0x00,/*addr*/
		0x00, 0x00, 0x00, 0x00,/*data len*/
		0x00, 0x00, /*for crc*/
		0x7E /*tail*/};


	*((u32 *)&command[5]) = cpu_to_be32(addr);
	*((u32 *)&command[9]) = cpu_to_be32(len);

	*((u16 *)&command[command_len - 3]) =
		cpu_to_be16(crc16_xmodem_cal(&command[1], command_len - 4));

	command_len = usb_download_command_replace(command, 16);

	return usb_download_sent_command(command, command_len,
			common_ack, sizeof(common_ack));
}

int usb_download_command_exec(unsigned int addr)
{
	unsigned short command_len = 12;
	char command[16] = {0x7E,/* head */
		0x00, 0x04, /*command type */
		0x00, 0x04, /*command len */
		0x00, 0x00, 0x00, 0x00, /*addr*/
		0x00, 0x00, /*for crc*/
		0x7E /*tail*/};

	*((u32 *)&command[5]) = cpu_to_be32(addr);

	*((u16 *)&command[command_len - 3]) =
		cpu_to_be16(crc16_xmodem_cal(&command[1], command_len - 4));

	command_len = usb_download_command_replace(command, 12);

	return usb_download_sent_command(command, command_len,
			common_ack, sizeof(common_ack));
}

#define usb_download_command_get_version()				\
	usb_download_sent_command(get_version, sizeof(get_version),	\
			get_version_ack, sizeof(get_version_ack))
#define usb_download_command_connect()				\
	usb_download_sent_command(connect, sizeof(connect),	\
			common_ack, sizeof(common_ack))
#define usb_download_command_end_download()				\
	usb_download_sent_command(end_download, sizeof(end_download),	\
			common_ack, sizeof(common_ack))

int marlin_get_version(void)
{
	return usb_download_command_get_version();
}

int marlin_connet(void)
{
	return usb_download_command_connect();
}

int marlin_firmware_download_start_usb(void)
{
	int ret;

	ret = usb_download_command_get_version();
	if (ret) {
		wcn_usb_err("%s start command is error\n", __func__);
		return ret;
	}

	ret = usb_download_command_connect();
	if (ret) {
		wcn_usb_err("%s connect command is error\n", __func__);
		return ret;
	}

	return 0;
}

int marlin_firmware_download_exec_usb(unsigned int addr)
{
	int ret;

	ret = usb_download_command_exec(addr);
	if (ret) {
		wcn_usb_err("%s exec command is error\n", __func__);
		return ret;
	}

	return 0;
}

int marlin_firmware_download_usb(unsigned int addr, const void *buf,
		unsigned int len, unsigned int packet_max)
{
	int ret;

	ret = usb_download_command_start_download(addr, len);
	if (ret) {
		wcn_usb_err("start download command is error\n");
		return ret;
	}

	ret = usb_download_sent_command(buf, len,
			common_ack, sizeof(common_ack));
	if (ret) {
		wcn_usb_err("donwload img is error\n");
		return ret;
	}

	ret = usb_download_command_end_download();
	if (ret) {
		wcn_usb_err("end donwload command is error\n");
		return ret;
	}

	return 0;
}

int marlin_firmware_get_chip_id(void *buf, unsigned int buf_size)
{
	int ret;
	unsigned short command_len = 8;
	char command[16] = {0x7E,/* head */
		0x00, 0x08, /*command type */
		0x00, 0x00, /*command len */
		0x00, 0x00, /*for crc*/
		0x7E /*tail*/};
	struct wcn_usb_ep *rx_ep = wcn_usb_store_get_epFRchn(DOWNLOAD_RX_CHN);
	int actual_len;
	char *temp_buf;
#define temp_buf_size 16

	if (!rx_ep) {
		wcn_usb_err("%s no rx_ep\n", __func__);
		return -EIO;
	}

	temp_buf = kzalloc(temp_buf_size, GFP_KERNEL);
	if (!temp_buf)
		return -ENOMEM;

	*((u16 *)&command[command_len - 3]) =
		cpu_to_be16(crc16_xmodem_cal(&command[1], command_len - 4));

	command_len = usb_download_command_replace(command, 8);

	ret = usb_download_sent_command(command, command_len, NULL, 0);
	if (ret) {
		wcn_usb_err("%s send command error\n", __func__);
		kfree(temp_buf);
		return -EIO;
	}

	ret = wcn_usb_msg(rx_ep, temp_buf, temp_buf_size, &actual_len, 3000);
	if (ret) {
		wcn_usb_err("%s get chip id error\n", __func__);
		kfree(temp_buf);
		return ret;
	}

	if (temp_buf[0] != 0x7e || temp_buf[2] != 0x08) {
		wcn_usb_err("%s get chip id error\n", __func__);
		print_hex_dump(KERN_ERR, "WCN ack", 0, 16, 1,
				temp_buf, temp_buf_size, 1);
		kfree(temp_buf);
		return ret;
	}

	memcpy(buf, &temp_buf[5], buf_size);

	kfree(temp_buf);
	return ret;

}


int marlin_dump_from_romcode_usb(unsigned int addr, void *buf, int len)
{
	int ret;
	unsigned short command_len = 16;
	char command[20] = {0x7E,/* head */
		0x00, 0x09, /*command type */
		0x00, 0x08, /*for command data len */
		0x00, 0x00, 0x00, 0x00,/*addr*/
		0x00, 0x00, 0x00, 0x00,/*data len*/
		0x00, 0x00, /*for crc*/
		0x7E /*tail*/};
	struct wcn_usb_ep *rx_ep = wcn_usb_store_get_epFRchn(DOWNLOAD_RX_CHN);
	int actual_len;

	if (!rx_ep) {
		wcn_usb_err("%s no rx_ep\n", __func__);
		return -EIO;
	}

	wcn_usb_info("%s addr is 0x%x len is 0x%x\n", __func__, addr, len);
	*((u32 *)&command[5]) = cpu_to_be32(addr);
	*((u32 *)&command[9]) = cpu_to_be32(len);

	*((u16 *)&command[command_len - 3]) =
		cpu_to_be16(crc16_xmodem_cal(&command[1], command_len - 4));

	command_len = usb_download_command_replace(command, 16);

	ret = usb_download_sent_command(command, command_len, NULL, 0);
	if (ret) {
		wcn_usb_err("%s send command error\n", __func__);
		return -EIO;
	}

	ret = wcn_usb_msg(rx_ep, buf, len, &actual_len, 3000);
	if (ret)
		wcn_usb_err("%s dump memory error\n", __func__);

	return ret;
}

int marlin_dump_read_usb(unsigned int addr, char *buf, int len)
{
	int ret;
	static int first_pac;

	if (first_pac == 0) {
		first_pac = 1;
		marlin_firmware_download_start_usb();
	}

	ret = marlin_dump_from_romcode_usb(addr, buf, len);

	return ret;
}
