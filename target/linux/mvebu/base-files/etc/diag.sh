#!/bin/sh
# Copyright (C) 2014-2016 OpenWrt.org
# Copyright (C) 2016 LEDE-Project.org

. /lib/functions/leds.sh
. /lib/mvebu.sh

get_status_led() {
	case $(mvebu_board_name) in
	armada-385-linksys-caiman)
		status_led="caiman:white:power"
		;;
	armada-385-linksys-cobra)
		status_led="cobra:white:power"
		;;
	armada-385-linksys-rango)
		status_led="rango:white:power"
		;;
	armada-385-linksys-shelby)
		status_led="shelby:white:power"
		;;
	armada-xp-linksys-mamba)
		status_led="mamba:white:power"
		;;
	esac
}

set_state() {
	get_status_led

	case "$1" in
	preinit)
		status_led_blink_preinit
		;;
	failsafe)
		status_led_blink_failsafe
		;;
	preinit_regular)
		status_led_blink_preinit_regular
		;;
	done)
		status_led_on
		;;
	esac
}
