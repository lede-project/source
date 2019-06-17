#!/bin/sh

[ -n "$INCLUDE_ONLY" ] || {
	. /lib/functions.sh
	. ../netifd-proto.sh
	init_proto "$@"
}
#DBG=-v

proto_mbim_init_config() {
	available=1
	no_device=1
	proto_config_add_string "device:device"
	proto_config_add_string pdptype
	proto_config_add_string apn
	proto_config_add_string pincode
	proto_config_add_boolean ignore_pinfail
	proto_config_add_string delay
	proto_config_add_string auth
	proto_config_add_string username
	proto_config_add_string password
	proto_config_add_defaults
}

_proto_mbim_setup() {
	local interface="$1"
	local tid=2
	local ret

	local device pdptype apn pincode ignore_pinfail delay auth username password $PROTO_DEFAULT_OPTIONS
	json_get_vars device pdptype apn pincode ignore_pinfail delay auth username password $PROTO_DEFAULT_OPTIONS

	[ -n "$ctl_device" ] && device=$ctl_device

	pdptype=`echo "$pdptype" | awk '{print tolower($0)}'`
	[ "$pdptype" = "ip" ] && pdptype="ipv4"
	[ "$pdptype" = "ipv4" ] || [ "$pdptype" = "ipv6" ] || [ "$pdptype" = "ipv4v6" ] || pdptype="ipv4v6"

	[ -n "$device" ] || {
		echo "mbim[$$]" "No control device specified"
		proto_notify_error "$interface" NO_DEVICE
		proto_set_available "$interface" 0
		return 1
	}
	[ -c "$device" ] || {
		echo "mbim[$$]" "The specified control device does not exist"
		proto_notify_error "$interface" NO_DEVICE
		proto_set_available "$interface" 0
		return 1
	}

	devname="$(basename "$device")"
	devpath="$(readlink -f /sys/class/usbmisc/$devname/device/)"
	ifname="$( ls "$devpath"/net )"

	[ -n "$ifname" ] || {
		echo "mbim[$$]" "Failed to find matching interface"
		proto_notify_error "$interface" NO_IFNAME
		proto_set_available "$interface" 0
		return 1
	}

	[ -n "$apn" ] || {
		echo "mbim[$$]" "No APN specified"
		proto_notify_error "$interface" NO_APN
		return 1
	}

	[ -n "$delay" ] && sleep "$delay"

	echo "mbim[$$]" "Reading capabilities"
	umbim $DBG -n -d $device caps || {
		echo "mbim[$$]" "Failed to read modem caps"
		proto_notify_error "$interface" PIN_FAILED
		return 1
	}
	tid=$((tid + 1))

	[ "$pincode" ] && {
		echo "mbim[$$]" "Sending pin"
		umbim $DBG -n -t $tid -d $device unlock "$pincode" || {
			echo "mbim[$$]" "Unable to verify PIN"
			proto_notify_error "$interface" PIN_FAILED
			proto_block_restart "$interface"
			return 1
		}
	}
	tid=$((tid + 1))

	echo "mbim[$$]" "Checking pin"
	umbim $DBG -n -t $tid -d $device pinstate || {
		echo "mbim[$$]" "PIN required"
		if [ ! "$ignore_pinfail" ] || [ "$ignore_pinfail" = 0 ]; then
			proto_notify_error "$interface" PIN_FAILED
			proto_block_restart "$interface"
			return 1
		else
			echo "mbim[$$]" "Ignoring PIN failure"
		fi
	}
	tid=$((tid + 1))

	echo "mbim[$$]" "Checking subscriber"
	umbim $DBG -n -t $tid -d $device subscriber || {
		echo "mbim[$$]" "Subscriber init failed"
		proto_notify_error "$interface" NO_SUBSCRIBER
		return 1
	}
	tid=$((tid + 1))

	echo "mbim[$$]" "Register with network"
	umbim $DBG -n -t $tid -d $device registration || {
		echo "mbim[$$]" "Subscriber registration failed"
		proto_notify_error "$interface" NO_REGISTRATION
		return 1
	}
	tid=$((tid + 1))

	echo "mbim[$$]" "Attach to network"
	umbim $DBG -n -t $tid -d $device attach || {
		echo "mbim[$$]" "Failed to attach to network"
		proto_notify_error "$interface" ATTACH_FAILED
		return 1
	}
	tid=$((tid + 1))

	echo "mbim[$$]" "Connect to network"
	while ! umbim $DBG -n -t $tid -d $device connect "$pdptype:$apn" "$auth" "$username" "$password"; do
		tid=$((tid + 1))
		sleep 1;
	done
	tid=$((tid + 1))

	echo "mbim[$$]" "Connected, obtain IP address and configure interface"
	config="/tmp/mbim.$$.config"
	umbim $DBG -n -t $tid -d $device config > "$config"
	[ $? = 0 ] || {
		echo "mbim[$$]" "Failed to obtain IP address"
		proto_notify_error "$interface" CONFIG_FAILED
		rm -f "$config"
		return 1
	}
	cat "$config"
	tid=$((tid + 1))

	uci_set_state network $interface tid "$tid"

	local ip_4=`awk '$1=="ipv4address:" {print $2}' "$config"`
	local ip_6=`awk '$1=="ipv6address:" {print $2}' "$config"`
	[ "$ip_4" ] || ["$ip_6" ] || {
		echo "mbim[$$]" "Failed to obtain IP addresses"
		proto_notify_error "$interface" CONFIG_FAILED
		rm -f "$config"
		return 1
	}

	proto_init_update "$ifname" 1
	proto_set_keep 1
	[ "$ip_4" ] && {
		echo "mbim[$$]" "Configure IPv4 on $ifname"
		local ip=`echo "$ip_4" | cut -f1 -d/`
		local mask=`echo "$ip_4" | cut -f2 -d/`
		local gateway=`awk '$1=="ipv4gateway:" {print $2; exit}' "$config"`
		local mtu=`awk '$1=="ipv4mtu:" {print $2; exit}' "$config"`
		local dns1=`awk '$1=="ipv4dnsserver:" {print $2}' "$config" | sed -n "1p"`
		local dns2=`awk '$1=="ipv4dnsserver:" {print $2}' "$config" | sed -n "2p"`

		proto_add_ipv4_address "$ip" "$mask"
		[ "$defaultroute" = 0  ] || proto_add_ipv4_route 0.0.0.0 0 "$gateway" "$ip_4" "$metric"
		[ "$peerdns" = 0 ] || {
			[ "$dns1" ] && proto_add_dns_server "$dns1"
			[ "$dns2" ] && proto_add_dns_server "$dns2"
		}
		[ "$mtu" ] && ip link set "$ifname" mtu "${mtu:-1460}"
	}
	[ "$ip_6" ] && {
		echo "mbim[$$]" "Configure IPv6 on $ifname"
		local ip=`echo "$ip_6" | cut -f1 -d/`
		local mask=`echo "$ip_6" | cut -f2 -d/`
		local gateway=`awk '$1=="ipv6gateway:" {print $2; exit}' "$config"`
		local mtu=`awk '$1=="ipv6mtu:" {print $2; exit}' "$config"`
		local dns1=`awk '$1=="ipv6dnsserver:" {print $2}' "$config" | sed -n "1p"`
		local dns2=`awk '$1=="ipv6dnsserver:" {print $2}' "$config" | sed -n "2p"`

		proto_add_ipv6_address "$ip" "$mask"
		proto_add_ipv6_prefix "$ip_6"
		[ "$defaultroute" = 0  ] || proto_add_ipv6_route "::" 0 "$gateway" "" "" "$ip_6" "$metric"
		[ "$peerdns" = 0 ] || {
			[ "$dns1" ] && proto_add_dns_server "$dns1"
			[ "$dns2" ] && proto_add_dns_server "$dns2"
		}
		[ "$mtu" ] && ip link set "$ifname" mtu "${mtu:-1460}"
		# TODO: Don't know how to propagate IPv6 address to lan from here?
	}

	rm -f "$config"
	proto_send_update "$interface"
	echo "mbim[$$]" "Connection setup complete"
}

proto_mbim_setup() {
	local ret

	_proto_mbim_setup $@
	ret=$?

	[ "$ret" = 0 ] || {
		logger "mbim bringup failed, retry in 15s"
		sleep 15
	}

	return $rt
}

proto_mbim_teardown() {
	local interface="$1"

	local device
	json_get_vars device
	local tid=$(uci_get_state network $interface tid)

	[ -n "$ctl_device" ] && device=$ctl_device

	echo "mbim[$$]" "Stopping network"
	[ -n "$tid" ] && {
		umbim $DBG -t$tid -d "$device" disconnect
		uci_revert_state network $interface tid
	}

	proto_init_update "*" 0
	proto_send_update "$interface"
}

[ -n "$INCLUDE_ONLY" ] || add_protocol mbim
