#!/bin/sh

[ -n "$INCLUDE_ONLY" ] || {
	. /lib/functions.sh
	. ../netifd-proto.sh
	init_proto "$@"
}

proto_ncm_init_config() {
	no_device=1
	available=1
	proto_config_add_string "device:device"
	proto_config_add_string apn
	proto_config_add_string auth
	proto_config_add_string username
	proto_config_add_string password
	proto_config_add_string pincode
	proto_config_add_string delay
	proto_config_add_string mode
	proto_config_add_string pdptype
	proto_config_add_boolean ipv6
	proto_config_add_defaults
}

proto_ncm_setup() {
	local interface="$1"

	local manufacturer initialize setmode connect ifname devname devpath

	local device apn auth username password pincode delay mode pdptype ipv6 $PROTO_DEFAULT_OPTIONS
	json_get_vars device apn auth username password pincode delay mode pdptype ipv6 $PROTO_DEFAULT_OPTIONS
	
	if [ "$ipv6" = 0 ]; then
		ipv6=""
	else
		ipv6=1
	fi
	
	[ -z "$pdptype" ] && {
		if [ -n "$ipv6" ]; then
			pdptype="IPV4V6"
		else
			pdptype="IP"
		fi
	}

	[ -n "$ctl_device" ] && device=$ctl_device

	[ -n "$device" ] || {
		echo "No control device specified"
		proto_notify_error "$interface" NO_DEVICE
		proto_set_available "$interface" 0
		return 1
	}
	[ -e "$device" ] || {
		echo "Control device not valid"
		proto_set_available "$interface" 0
		return 1
	}
	[ -n "$apn" ] || {
		echo "No APN specified"
		proto_notify_error "$interface" NO_APN
		return 1
	}

	devname="$(basename "$device")"
	case "$devname" in
	'tty'*)
		devpath="$(readlink -f /sys/class/tty/$devname/device)"
		ifname="$( ls "$devpath"/../../*/net )"
		;;
	*)
		devpath="$(readlink -f /sys/class/usbmisc/$devname/device/)"
		ifname="$( ls "$devpath"/net )"
		;;
	esac
	[ -n "$ifname" ] || {
		echo "The interface could not be found."
		proto_notify_error "$interface" NO_IFACE
		proto_set_available "$interface" 0
		return 1
	}

	[ -n "$delay" ] && sleep "$delay"

	manufacturer=`gcom -d "$device" -s /etc/gcom/getcardinfo.gcom | awk '/Manufacturer/ { print tolower($2) }'`
	[ -z $manufacturer ] && {
		manufacturer=`gcom -d "$device" -s /etc/gcom/getcardinfo.gcom | head -2 | tail -1 | tr -d "\r" | tr [A-Z] [a-z]`
	}
	[ $? -ne 0 ] && {
		echo "Failed to get modem information"
		proto_notify_error "$interface" GETINFO_FAILED
		return 1
	}

	json_load "$(cat /etc/gcom/ncm.json)"
	json_select "$manufacturer"
	[ $? -ne 0 ] && {
		echo "Unsupported modem"
		proto_notify_error "$interface" UNSUPPORTED_MODEM
		proto_set_available "$interface" 0
		return 1
	}
	json_get_values initialize initialize
	for i in $initialize; do
		eval COMMAND="$i" gcom -d "$device" -s /etc/gcom/runcommand.gcom || {
			echo "Failed to initialize modem"
			proto_notify_error "$interface" INITIALIZE_FAILED
			return 1
		}
	done

	[ -n "$pincode" ] && {
		PINCODE="$pincode" gcom -d "$device" -s /etc/gcom/setpin.gcom || {
			echo "Unable to verify PIN"
			proto_notify_error "$interface" PIN_FAILED
			proto_block_restart "$interface"
			return 1
		}
	}

        json_get_values configure configure
        for i in $configure; do
                eval COMMAND="$i" gcom -d "$device" -s /etc/gcom/runcommand.gcom || {
                        echo "Failed to configure modem"
                        proto_notify_error "$interface" INITIALIZE_FAILED
                        return 1
                }
        done

	[ -n "$mode" ] && {
		json_select modes
		json_get_var setmode "$mode"
		eval COMMAND="$setmode" gcom -d "$device" -s /etc/gcom/runcommand.gcom || {
			echo "Failed to set operating mode"
			proto_notify_error "$interface" SETMODE_FAILED
			return 1
		}
		json_select ..
	}

#	json_get_vars connect
#	eval COMMAND="$connect" gcom -d "$device" -s /etc/gcom/runcommand.gcom || {
#		echo "Failed to connect"
#		proto_notify_error "$interface" CONNECT_FAILED
#		return 1
#	}

	echo "Connected, starting DHCP on $ifname"
	
	proto_init_update "$ifname" 1
	proto_send_update "$interface"

	json_load "$(cat /etc/gcom/ncm.json)"
	json_select "$manufacturer"
	json_get_vars connect
	eval COMMAND="$connect" gcom -d "$device" -s /etc/gcom/runcommand.gcom || {
               echo "Failed to connect"
               proto_notify_error "$interface" CONNECT_FAILED
               return 1
	}

	json_init
	json_add_string name "${interface}_4"
	json_add_string ifname "@$interface"
	json_add_string proto "dhcp"
	proto_add_dynamic_defaults
	ubus call network add_dynamic "$(json_dump)"

	[ -n "$ipv6" ] && {
		json_init
		json_add_string name "${interface}_6"
		json_add_string ifname "@$interface"
		json_add_string proto "dhcpv6"
		json_add_string extendprefix 1
		proto_add_dynamic_defaults
		ubus call network add_dynamic "$(json_dump)"
	}
}

proto_ncm_teardown() {
	local interface="$1"

	local manufacturer disconnect

	local device
	json_get_vars device

	echo "Stopping network device:$device"

	manufacturer=`gcom -d "$device" -s /etc/gcom/getcardinfo.gcom | awk '/Manufacturer/ { print tolower($2) }'`
	[ -z $manufacturer ] && {
		manufacturer=`gcom -d "$device" -s /etc/gcom/getcardinfo.gcom | sed '2q;d' | tr -d "\n\r" | tr [A-Z] [a-z]`
	}
	[ $? -ne 0 ] && {
		echo "Failed to get modem information"
		proto_notify_error "$interface" GETINFO_FAILED
		return 1
	}

	json_load "$(cat /etc/gcom/ncm.json)"
	json_select "$manufacturer" || {
		echo "Unsupported modem"
		proto_notify_error "$interface" UNSUPPORTED_MODEM
		return 1
	}

	json_get_vars disconnect
	COMMAND="$disconnect" gcom -d "$device" -s /etc/gcom/runcommand.gcom || {
		echo "Failed to disconnect"
		proto_notify_error "$interface" DISCONNECT_FAILED
		return 1
	}

	proto_init_update "*" 0
	proto_send_update "$interface"
}
[ -n "$INCLUDE_ONLY" ] || {
	add_protocol ncm
}
