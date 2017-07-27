#!/bin/sh
#
# This script is called by dsl_cpe_control whenever there is a DSL event,
# we only actually care about the DSL_INTERFACE_STATUS events as these
# tell us the line has either come up or gone down.
#
# The rest of the code is basically the same at the atm hotplug code
#

[ "$DSL_NOTIFICATION_TYPE" = "DSL_INTERFACE_STATUS" ] || exit 0

. /usr/share/libubox/jshn.sh
. /lib/functions.sh
. /lib/functions/leds.sh

led_dsl_up() {
	case "$(config_get led_dsl trigger)" in
	"netdev")
		led_set_attr $1 "trigger" "netdev"
		led_set_attr $1 "device_name" "$(config_get led_dsl dev)"
		led_set_attr $1 "mode" "$(config_get led_dsl mode)"
		;;
	*)
		led_on $1
		;;
	esac
}

include /lib/network
scan_interfaces

config_load system
config_get led led_dsl sysfs
if [ -n "$led" ]; then
	case "$DSL_INTERFACE_STATUS" in
	  "HANDSHAKE")  led_timer $led 500 500;;
	  "TRAINING")   led_timer $led 200 200;;
	  "UP")		led_dsl_up $led;;
	  *)		led_off $led
	esac
fi

interfaces=`ubus list network.interface.\* | cut -d"." -f3`
for ifc in $interfaces; do

	json_load "$(ifstatus $ifc)"

	json_get_var proto proto
	if [ "$proto" != "pppoa" ]; then
		continue
	fi

	json_get_var up up
	config_get_bool auto "$ifc" auto 1
	if [ "$DSL_INTERFACE_STATUS" = "UP" ]; then
		if [ "$up" != 1 ] && [ "$auto" = 1 ]; then
			( sleep 1; ifup "$ifc" ) &
		fi
	else
		if [ "$up" = 1 ] && [ "$auto" = 1 ]; then
			( sleep 1; ifdown "$ifc" ) &
		else
			json_get_var autostart autostart
			if [ "$up" != 1 ] && [ "$autostart" = 1 ]; then
				( sleep 1; ifdown "$ifc" ) &
			fi
		fi
	fi
done


