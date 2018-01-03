#!/bin/sh
# Copyright (C) 2010-2015 OpenWrt.org

. /lib/functions/leds.sh

boot="$(get_dt_led boot)"
failsafe="$(get_dt_led failsafe)"
running="$(get_dt_led running)"

set_state() {
	status_led="$boot"

	case "$1" in
	preinit)
		status_led_blink_preinit
		;;
	failsafe)
		status_led_off
		[ -n "$running" ] && {
			status_led="$running"
			status_led_off
		}
		status_led="$failsafe"
		status_led_blink_failsafe
		;;
	preinit_regular)
		status_led_blink_preinit_regular
		;;
	done)
		status_led_off
		[ -n "$running" ] && {
			status_led="$running"
			status_led_on
		}
		;;
	esac
}
