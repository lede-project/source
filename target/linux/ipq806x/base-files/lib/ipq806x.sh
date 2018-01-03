#!/bin/sh
#
# Copyright (c) 2014 The Linux Foundation. All rights reserved.
# Copyright (C) 2011 OpenWrt.org
#

IPQ806X_BOARD_NAME=
IPQ806X_MODEL=

ipq806x_board_detect() {
	local machine
	local name

	machine=$(cat /proc/device-tree/model)

	case "$machine" in
	*"AP-DK01.1-C1")
		name="ap-dk01.1-c1"
		;;
	*"AP148")
		name="ap148"
		;;
	*"4040")
		name="fritz4040"
		;;
	*"C2600")
		name="c2600"
		;;
	*"D7800")
		name="d7800"
		;;
	*"DB149")
		name="db149"
		;;
	*"NBG6817")
		name="nbg6817"
		;;
	*"R7500")
		name="r7500"
		;;
	*"R7500v2")
		name="r7500v2"
		;;
	*"Linksys EA8500"*)
		name="ea8500"
		;;
	*"R7800")
		name="r7800"
		;;
	*"VR2600v")
		name="vr2600v"
		;;
	esac

	[ -z "$name" ] && name="unknown"

	[ -z "$IPQ806X_BOARD_NAME" ] && IPQ806X_BOARD_NAME="$name"
	[ -z "$IPQ806X_MODEL" ] && IPQ806X_MODEL="$machine"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$IPQ806X_BOARD_NAME" > /tmp/sysinfo/board_name
	echo "$IPQ806X_MODEL" > /tmp/sysinfo/model
}
