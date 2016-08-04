/*
 * --------------------------------------------------------------------
 * STNFC (CR95HF) standalone wake-up test application!
 * --------------------------------------------------------------------
 * Copyright (C) 2016 STMicroelectronics Pvt. Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

int main(int argc, char *argv[])
{
	int fd;
	int i;
	unsigned char send_byte = 0x55;
	unsigned char recv_byte = 0;
	struct termios config, config1;

	if (argc < 2) {
		printf("Missing device file name in program argument list\n");
		return 1;
	}

	printf("Device selected is %s\n", argv[1]);

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd == -1) {
		printf("Dev file open failed\n");
		return 1;
	}

	memset(&config, 0, sizeof(config));
	memset(&config1, 0, sizeof(config1));

	config.c_cflag = CSTOPB;
	config.c_cflag |= B57600 | CS8 | CREAD | CLOCAL;
	config.c_cc[VMIN] = 1;
	config.c_cc[VTIME] = 0;

	tcflush(fd, TCIFLUSH);
	if(tcsetattr(fd, TCSANOW, &config)) 
        {
                printf("Error in setting terminal configs\n");
		return 1;
        }

	tcgetattr(fd, &config1);

	printf("Requested config c_cflag is 0x%x\n", config.c_cflag);
	printf("Actual set c_cflag is 0x%x\n", config1.c_cflag);

	/* first wake-up CR95HF */
	write(fd, &send_byte, 1);
	write(fd, &send_byte, 1);
	write(fd, &send_byte, 1);
	write(fd, &send_byte, 1);
	
	tcflush(fd, TCIOFLUSH);

	for(i = 0; i <= 10; i++) {
		
		write(fd, &send_byte, 1);

		read(fd, &recv_byte, 1);

		printf("Send => 0x%x\n", send_byte);
		printf("Recv <= 0x%x\n", recv_byte);
		
		tcflush(fd, TCIOFLUSH);
	}

	if (recv_byte == 0x55)
		printf("Success: ST NFC Device woken up !!\n");
	else
		printf("Error: ST NFC Device NOT woken !!\n");

	close(fd);

	return 0;
}
