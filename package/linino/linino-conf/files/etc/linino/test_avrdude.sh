# (c)  Copyright 2013 dog hunter, LLC - All righs reserved
# GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
# Domenico La Fauci
# Federico Musto

echo "Testing AVRDUDE on ATMEL 32U4 ..."

#!/bin/sh

OE_PATH=/sys/class/gpio/gpio21/value
OE_VALUE=$(cat $OE_PATH)

# remove SPI driver to avoid conflicts while AVR programming
TTY_DS=$(lsmod | grep spi_tty_ds)
if [ "x$TTY_DS" != "x" ]; then
	rmmod spi-tty-ds
	if [ "$?" != "0" ]; then
		exit
	fi
fi

GPIO_DS=$(lsmod | grep spi_gpio)
if [ "x$GPIO_DS" != "x" ]; then
	rmmod spi-gpio
	if [ "$?" != "0" ]; then
		exit
	fi
fi


# program AVR
echo 1 > $OE_PATH
avrdude -c linuxgpio -v -C /etc/avrdude.conf -p m32u4 -U lfuse:w:0xFF:m -U hfuse:w:0xD8:m -U efuse:w:0xFB:m 


# Restore previous status
echo $OE_VALUE > $OE_PATH
if [ "x$GPIO_DS" != "x" ]; then
	insmod spi-gpio
fi

if [ "x$TTY_DS" != "x" ]; then
	insmod spi-tty-ds
fi
