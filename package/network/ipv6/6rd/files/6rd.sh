#!/bin/sh
# 6rd.sh - IPv6-in-IPv4 tunnel backend
# Copyright (c) 2010-2012 OpenWrt.org

[ -n "$INCLUDE_ONLY" ] || {
	. /lib/functions.sh
	. /lib/functions/network.sh
	. ../netifd-proto.sh
	init_proto "$@"
}

get_cidr_netbit() {
	eval $1=$(echo $2 | cut -d/ -f2)
}

ip6addr_to_hex() {
	eval $1=$(echo $2 | cut -d/ -f1 | sed "s/::/$(echo $2 |
		awk -F':' '{for(i=1;i<=10-NF;i++)printf ":"}')/" |
		awk -F':' '{for(i=1;i<=NF;i++)x=x""sprintf("%04s", $i);gsub(/ /,"0",x);printf x}')
}

hex_to_ip6addr() {
	eval $1=$(echo $2 | sed 's/.\{4\}/&:/g;s/:$//;s/:0\+/:/g;s/:::*/::/g')
}

ip4addr_to_hex() {
	eval $1=$(echo $2 | cut -d/ -f1 | awk -F'\.' '{for(i=1;i<=NF;i++)printf "%02x",$i}')
}

hex_to_bin() {
	eval $1=$(echo $2 | sed '
	s/0/0000/g;s/1/0001/g;s/2/0010/g;s/3/0011/g;s/4/0100/g;s/5/0101/g;s/6/0110/g;s/7/0111/g;s/8/1000/g;
	s/9/1001/g;s/a/1010/g;s/b/1011/g;s/c/1100/g;s/d/1101/g;s/e/1110/g;s/f/1111/g')
}

bin_to_hex() {
	padding=$(printf "%$(((${#2}+3)/4*4-${#2}))s")
	eval $1=$(echo ${padding// /0}$2 | sed 's/.\{4\}/& /g;
	s/0000/0/g;s/0001/1/g;s/0010/2/g;s/0011/3/g;s/0100/4/g;s/0101/5/g;s/0110/6/g;s/0111/7/g;s/1000/8/g;
	s/1001/9/g;s/1010/a/g;s/1011/b/g;s/1100/c/g;s/1101/d/g;s/1110/e/g;s/1111/f/g;s/ //g')
}

# $1 | ($2 << $3)
hex_or_lshift() {
	local __dst __src __padding __res
	hex_to_bin __dst $2
	hex_to_bin __src $3
	__padding=$(printf "%$4s")
	__src=$__src${__padding// /0}
	if [ ${#__src} -gt ${#__dst} ]; then
		__padding=$(printf "%$((${#__src}-${#__dst}))s")
		__dst=${__padding// /0}$__dst
	elif [ ${#__dst} -gt ${#__src} ]; then
		__padding=$(printf "%$((${#__dst}-${#__src}))s")
		__src=${__padding// /0}$__src
	fi
	__res=
	for i in `seq 0 $((${#__dst}-1))`; do
		if [ ${__dst:$i:1} = '0' -a ${__src:$i:1} = '0' ]; then
			__res=${__res}0
		else
			__res=${__res}1
		fi
	done
	bin_to_hex __res $__res
	eval $1=$__res
}

sixrd_calc() {
	local __ip6prefix_hex __ip6prefixlen __ipaddr_hex __ip4hostlen __ip6subnet __ip6hostlen
	ip6addr_to_hex __ip6prefix_hex $2
	__ip6prefixlen=$3
	ip4addr_to_hex __ipaddr_hex $4
	__ip4hostlen=$((32-$5))
	__ipaddr_hex=$(printf "%x" $((0x$__ipaddr_hex&((1<<$__ip4hostlen)-1))))
	__ip6hostlen=$((128-$__ip6prefixlen-$__ip4hostlen))
	hex_or_lshift __ip6subnet $__ip6prefix_hex $__ipaddr_hex $__ip6hostlen
	hex_to_ip6addr __ip6subnet $__ip6subnet
	eval $1=$__ip6subnet/$((128-__ip6hostlen))
}

proto_6rd_setup() {
	local cfg="$1"
	local iface="$2"
	local link="6rd-$cfg"

	local mtu df ttl tos ipaddr peeraddr ip6prefix ip6prefixlen ip4prefixlen tunlink zone
	json_get_vars mtu df ttl tos ipaddr peeraddr ip6prefix ip6prefixlen ip4prefixlen tunlink zone

	[ -z "$ip6prefix" -o -z "$peeraddr" ] && {
		proto_notify_error "$cfg" "MISSING_ADDRESS"
		proto_block_restart "$cfg"
		return
	}

	( proto_add_host_dependency "$cfg" "$peeraddr" "$tunlink" )

	[ -z "$ipaddr" ] && {
		local wanif="$tunlink"
		if [ -z $wanif ] && ! network_find_wan wanif; then
			proto_notify_error "$cfg" "NO_WAN_LINK"
			return
		fi

		if ! network_get_ipaddr ipaddr "$wanif"; then
			proto_notify_error "$cfg" "NO_WAN_LINK"
			return
		fi
	}

	# Determine the relay prefix.
	local ip4prefixlen="${ip4prefixlen:-0}"
	local ip4prefix=$(ipcalc.sh "$ipaddr/$ip4prefixlen" | grep NETWORK)
	ip4prefix="${ip4prefix#NETWORK=}"

	# Determine our IPv6 address.
	local ip6subnet
	sixrd_calc ip6subnet $ip6prefix $ip6prefixlen $ipaddr $ip4prefixlen
	local ip6addr="${ip6subnet%%::*}::1"

	# Determine the IPv6 prefix
	local ip6lanprefix="$ip6subnet/$(($ip6prefixlen + 32 - $ip4prefixlen))"

	proto_init_update "$link" 1
	proto_add_ipv6_address "$ip6addr" "$ip6prefixlen"
	proto_add_ipv6_prefix "$ip6lanprefix"

	proto_add_ipv6_route "::" 0 "::$peeraddr" 4096 "" "$ip6addr/$ip6prefixlen"
	proto_add_ipv6_route "::" 0 "::$peeraddr" 4096 "" "$ip6lanprefix"

	proto_add_tunnel
	json_add_string mode sit
	json_add_int mtu "${mtu:-1280}"
	json_add_boolean df "${df:-1}"
	json_add_int ttl "${ttl:-64}"
	[ -n "$tos" ] && json_add_string tos "$tos"
	json_add_string local "$ipaddr"
	[ -n "$tunlink" ] && json_add_string link "$tunlink"

	json_add_object 'data'
	json_add_string prefix "$ip6prefix/$ip6prefixlen"
	json_add_string relay-prefix "$ip4prefix/$ip4prefixlen"
	json_close_object

	proto_close_tunnel

	proto_add_data
	[ -n "$zone" ] && json_add_string zone "$zone"
	proto_close_data

	proto_send_update "$cfg"
}

proto_6rd_teardown() {
	local cfg="$1"
}

proto_6rd_init_config() {
	no_device=1
	available=1

	proto_config_add_int "mtu"
	proto_config_add_boolean "df"
	proto_config_add_int "ttl"
	proto_config_add_string "tos"
	proto_config_add_string "ipaddr"
	proto_config_add_string "peeraddr"
	proto_config_add_string "ip6prefix"
	proto_config_add_string "ip6prefixlen"
	proto_config_add_string "ip4prefixlen"
	proto_config_add_string "tunlink"
	proto_config_add_string "zone"
}

[ -n "$INCLUDE_ONLY" ] || {
	add_protocol 6rd
}
