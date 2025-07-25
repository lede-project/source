#ifndef __WCN_USB_BOOT_H__
#define __WCN_USB_BOOT_H__
#include "../usb/wcn_usb.h"

int marlin_firmware_download_exec_usb(unsigned int addr);
int marlin_firmware_download_start_usb(void);
int marlin_firmware_download_usb(unsigned int addr, const void *buf,
		unsigned int len, unsigned int packet_max);
int marlin_dump_read_usb(unsigned int addr, char *buf, int len);
int marlin_firmware_get_chip_id(void *buf, unsigned int buf_size);

#define WCN_USB_FDL_ADDR 0x40f00000
#define WCN_USB_FW_ADDR 0x100000
#define WCN_USB_FDL_PATH \
	"/dev/block/platform/soc/soc:ap-ahb/50430000.sdio/by-name/wcnfdl"

#define WCN_USB_FDL_SIZE 1024

#endif
