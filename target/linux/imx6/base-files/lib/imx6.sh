#!/bin/sh
#
# Copyright (C) 2010-2013 OpenWrt.org
#

IMX6_BOARD_NAME=
IMX6_MODEL=

imx6_board_detect() {
	local machine
	local name

	machine=$(cat /proc/device-tree/model)

	case "$machine" in
	"Gateworks Ventana i.MX6 DualLite/Solo GW51XX" |\
	"Gateworks Ventana i.MX6 Dual/Quad GW51XX")
		name="gw51xx"
		;;

	"Gateworks Ventana i.MX6 DualLite/Solo GW52XX" |\
	"Gateworks Ventana i.MX6 Dual/Quad GW52XX")
		name="gw52xx"
		;;

	"Gateworks Ventana i.MX6 DualLite/Solo GW53XX" |\
	"Gateworks Ventana i.MX6 Dual/Quad GW53XX")
		name="gw53xx"
		;;

	"Gateworks Ventana i.MX6 DualLite/Solo GW54XX" |\
	"Gateworks Ventana i.MX6 Dual/Quad GW54XX" |\
	"Gateworks Ventana GW5400-A")
		name="gw54xx"
		;;

	"Gateworks Ventana i.MX6 Dual/Quad GW551X" |\
	"Gateworks Ventana i.MX6 Solo/DualLite GW551X")
		name="gw551x"
		;;

	"Gateworks Ventana i.MX6 DualLite/Solo GW552X" |\
	"Gateworks Ventana i.MX6 Dual/Quad GW552X")
		name="gw552x"
		;;

	"Gateworks Ventana i.MX6 DualLite/Solo GW5904" |\
	"Gateworks Ventana i.MX6 Dual/Quad GW5904")
		name="gw5904"
		;;

	"Wandboard i.MX6 Dual Lite Board")
		name="wandboard"
		;;

	*)
		name="generic"
		;;
	esac

	[ -z "$IMX6_BOARD_NAME" ] && IMX6_BOARD_NAME="$name"
	[ -z "$IMX6_MODEL" ] && IMX6_MODEL="$machine"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$IMX6_BOARD_NAME" > /tmp/sysinfo/board_name
	echo "$IMX6_MODEL" > /tmp/sysinfo/model
}

imx6_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] || imx6_board_detect
	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}
