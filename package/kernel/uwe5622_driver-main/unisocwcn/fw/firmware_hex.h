#ifndef _FIRMWARE_HEX_H
#define _FIRMWARE_HEX_H

#define firmware_hexcode "wcnmodem.bin.hex"

static unsigned char firmware_hex_buf[] = {
#include firmware_hexcode
};

#define FIRMWARE_HEX_SIZE sizeof(firmware_hex_buf)

#endif /* _FIRMWARE_HEX_H */
