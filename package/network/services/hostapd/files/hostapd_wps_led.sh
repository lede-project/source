#!/bin/sh

. /lib/functions/leds.sh

IFNAME=$1
CMD=$2

PID_FILE=/var/run/hostapd_cli-wpsled-$IFNAME.pid

WPS_LED=$(ls /sys/class/leds | grep -E 'qss|wps|security' | head -n 1)

[ -z "$WPS_LED" ] && exit 1

case "$CMD" in
	WPS-PBC-ACTIVE)
		led_timer $WPS_LED 500 500
		;;
	WPS-PBC-DISABLE|CTRL-EVENT-EAP-FAILURE|WPS-TIMEOUT|WPS-SUCCESS)
		led_off "$WPS_LED"
		kill $(cat "$PID_FILE")
		rm -f "$PID_FILE"
		exit 1
		;;
	CTRL-EVENT-EAP-STARTED)
		led_timer "$WPS_LED" 150 150
		;;
esac
