/*
 * --------------------------------------------------------------------
 * STNFC Daemon source code
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
#include <sys/ioctl.h>
#include <linux/tty.h>

enum digital_uart_driver{
	DIGITAL_UART_DRIVER_ST = 0,
	DIGITAL_UART_DRIVER_MAX,
};

#define DIGITALUARTSETDRIVER    _IOW('V', 0, char *)
#define N_DIGITAL       	26

int main(int argc, char *argv[])
{
	int fd;
	int ldtype = -1;
	int err;

	if (argc < 2) {
		printf("Missing device file name in program argument list\n");
		return 1;
	}

	printf("Device selected is %s\n", argv[1]);

	fd = open(argv[1], O_RDONLY | O_NOCTTY);
	if (fd == -1) {
		printf("Dev file open failed\n");
		return 1;
	}


	ioctl(fd, TIOCGETD, &ldtype);
	if (ldtype == N_TTY)
		printf("Initial LDisc on tty = %s matched with N_TTY\n", argv[1]);

	ldtype = N_DIGITAL;
	err = ioctl(fd, TIOCSETD, &ldtype);
	if (err == -1) {
		printf("LDisc change on tty = %s failed\n", argv[1]);
		return 1;
	} else
		printf("LDisc changed to N_DIGITAL on tty = %s\n", argv[1]);

	err = ioctl(fd, DIGITALUARTSETDRIVER, DIGITAL_UART_DRIVER_ST);
	if (err == -1) {
		printf("LDisc driver setting on tty = %s failed\n", argv[1]);
		return 1;
	} else
		printf("LDisc driver set to %d on tty = %s\n", DIGITAL_UART_DRIVER_ST,  argv[1]);

	printf("Process Going to Daemonize itself and then sleep forever... :)\n");

	err = daemon(0, 0);
	if (err) {
		printf("Cant start the Daemon :( Returning....\n");
		return err;
	}

	for (;;) pause();
}
