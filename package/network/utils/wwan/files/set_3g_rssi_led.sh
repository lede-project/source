#!/bin/sh

. /lib/ramips.sh

set_led_rssi_off() {
	case $board in
	dwr-512-b)
		echo '0' > /sys/class/leds/$board\:green\:sigstrength/brightness
		echo '0' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'none' >   /sys/class/leds/$board\:red\:sigstrength/trigger
		;;
	mr200)
		echo '0' > /sys/class/leds/$board:white:signal1/brightness
		echo '0' > /sys/class/leds/$board:white:signal2/brightness
		echo '0' > /sys/class/leds/$board:white:signal3/brightness
		echo '0' > /sys/class/leds/$board:white:signal4/brightness
		echo 'none' > /sys/class/leds/$board:white:signal4/trigger
		;;
	esac
}
set_led_rssi_1() {
        case $board in
        dwr-512-b)
                echo '0' > /sys/class/leds/$board\:green\:sigstrength/brightness
                echo '255' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'none' >   /sys/class/leds/$board\:red\:sigstrength/trigger
                ;;
	mr200)
		echo '255' > /sys/class/leds/$board:white:signal1/brightness
		echo '0' > /sys/class/leds/$board:white:signal2/brightness
		echo '0' > /sys/class/leds/$board:white:signal3/brightness
		echo '0' > /sys/class/leds/$board:white:signal4/brightness
		echo 'none' > /sys/class/leds/$board:white:signal4/trigger
		;;
        esac
}
set_led_rssi_2() {
        case $board in
        dwr-512-b)
                echo '255' > /sys/class/leds/$board\:green\:sigstrength/brightness
                echo '255' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'none' >   /sys/class/leds/$board\:red\:sigstrength/trigger
                ;;
	mr200)
		echo '255' > /sys/class/leds/$board:white:signal1/brightness
		echo '255' > /sys/class/leds/$board:white:signal2/brightness
		echo '0' > /sys/class/leds/$board:white:signal3/brightness
		echo '0' > /sys/class/leds/$board:white:signal4/brightness
		echo 'none' > /sys/class/leds/$board:white:signal4/trigger
		;;
        esac
}
set_led_rssi_3() {
        case $board in
        dwr-512-b)
                echo '255' > /sys/class/leds/$board\:green\:sigstrength/brightness
                echo '128' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'none' >   /sys/class/leds/$board\:red\:sigstrength/trigger
                ;;
	mr200)
		echo '255' > /sys/class/leds/$board:white:signal1/brightness
		echo '255' > /sys/class/leds/$board:white:signal2/brightness
		echo '255' > /sys/class/leds/$board:white:signal3/brightness
		echo '0' > /sys/class/leds/$board:white:signal4/brightness
		echo 'none' > /sys/class/leds/$board:white:signal4/trigger
		;;
        esac
}
set_led_rssi_4() {
        case $board in
        dwr-512-b)
                echo '255' > /sys/class/leds/$board\:green\:sigstrength/brightness
                echo '0' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'none' >   /sys/class/leds/$board\:red\:sigstrength/trigger
                ;;
	mr200)
		echo '255' > /sys/class/leds/$board:white:signal1/brightness
		echo '255' > /sys/class/leds/$board:white:signal2/brightness
		echo '255' > /sys/class/leds/$board:white:signal3/brightness
		echo '255' > /sys/class/leds/$board:white:signal4/brightness
		echo 'none' > /sys/class/leds/$board:white:signal4/trigger
		;;
        esac
}
set_led_rssi_5() {
        case $board in
        dwr-512-b)
                echo '255' > /sys/class/leds/$board\:green\:sigstrength/brightness
                echo '0' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'none' >   /sys/class/leds/$board\:red\:sigstrength/trigger
                ;;
	mr200)
		echo '255' > /sys/class/leds/$board:white:signal1/brightness
		echo '255' > /sys/class/leds/$board:white:signal2/brightness
		echo '255' > /sys/class/leds/$board:white:signal3/brightness
		echo '255' > /sys/class/leds/$board:white:signal4/brightness
		echo 'none' > /sys/class/leds/$board:white:signal4/trigger
		;;
        esac
}
set_led_rssi_err() {
        case $board in
        dwr-512-b)
                echo '0' > /sys/class/leds/$board\:green\:sigstrength/brightness
                echo '255' >   /sys/class/leds/$board\:red\:sigstrength/brightness
                echo 'timer' >   /sys/class/leds/$board\:red\:sigstrength/trigger
                ;;
	mr200)
		echo '0' > /sys/class/leds/$board:white:signal1/brightness
		echo '0' > /sys/class/leds/$board:white:signal2/brightness
		echo '0' > /sys/class/leds/$board:white:signal3/brightness
		echo '255' > /sys/class/leds/$board:white:signal4/brightness
		echo 'timer' > /sys/class/leds/$board:white:signal4/trigger
		;;
        esac
}
set_lte_led_on() {
	case $board in
	mr200)
		echo '255' > /sys/class/leds/$board:white:4g/brightness
		;;
	esac
}
set_lte_led_off() {
	case $board in
		mr200)
		echo '0' > /sys/class/leds/$board:white:4g/brightness
	;;
	esac
}

board=$(ramips_board_name)

case "$1" in
start)
	until [ 0 -gt 1 ]; do
		rssi3g=$(gcom -d /dev/ttyUSB0 -s /etc/gcom/getstrength.gcom|awk '/[CSQ]/ { print $2 }'|sed -e 's/,//')
		sleep 2
		lte=$(gcom -d /dev/ttyUSB1 reg |awk -F[,] '/^\Registered/ {print $2}')

		if [ $rssi3g -eq 99 ]; then
			set_led_rssi_err
		elif [ $rssi3g -gt 24 ]; then
			set_led_rssi_5
		elif [ $rssi3g -gt 19 ]; then
			set_led_rssi_4
		elif [ $rssi3g -gt 14 ]; then
			set_led_rssi_3
		elif [ $rssi3g -gt 9 ]; then
			set_led_rssi_2
		elif [ $rssi3g -gt 1 ]; then
			set_led_rssi_1
		else
			set_led_rssi_err;
		fi

		if [ $lte -eq 7 ]; then
			set_lte_led_on
		else
			set_lte_led_off;
		fi

		sleep 15;
	done
	;;
stop)
	set_led_rssi_off
	;;
test)
	set_led_rssi_1
	sleep 2;
	set_led_rssi_2
	sleep 2;
	set_led_rssi_3
	sleep 2;
	set_led_rssi_4
	sleep 2;
	set_led_rssi_5
	sleep 2;
	set_led_rssi_err
	sleep 2;
	set_lte_led_on
	sleep 2;
	set_lte_led_off
	sleep 2;
	set_led_rssi_off
esac

