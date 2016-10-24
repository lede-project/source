#!/bin/sh
#
# Copyright (C) 2013-2015 OpenWrt.org
# Copyright (C) 2016 LEDE-Project.org
#

MVEBU_BOARD_NAME=
MVEBU_MODEL=

mvebu_board_detect() {
	local machine
	local name

	machine=$(cat /proc/device-tree/model)

	case "$machine" in
	*"Marvell Armada 370 Evaluation Board")
		name="armada-370-db"
		;;
	*"Globalscale Mirabox")
		name="mirabox"
		;;
	*"Marvell Armada 370 Reference Design")
		name="armada-370-rd"
		;;
	*"Marvell Armada XP Evaluation Board")
		name="armada-xp-db"
		;;
	*"PlatHome OpenBlocks AX3-4 board")
		name="openblocks-ax3-4"
		;;
	*"Marvell Armada XP GP Board")
		name="armada-xp-gp"
		;;
	*"Linksys WRT1200AC")
		name="armada-385-linksys-caiman"
		;;
	*"Linksys WRT1900AC")
		name="armada-xp-linksys-mamba"
		;;
	*"Linksys WRT1900ACv2")
		name="armada-385-linksys-cobra"
		;;
	*"Linksys WRT1900ACS")
		name="armada-385-linksys-shelby"
		;;
	*"Linksys WRT3200ACM")
		name="armada-385-linksys-rango"
		;;
	*"Marvell Armada 385 Access Point Development Board")
		name="armada-385-db-ap"
		;;
	*"Marvell Armada XP Development Board DB-MV784MP-GP")
		name="armada-xp-gp"
		;;
	*"SolidRun Clearfog A1")
		name="armada-388-clearfog"
		;;
	esac

	[ -z "$name" ] && name="unknown"

	[ -z "$MVEBU_BOARD_NAME" ] && MVEBU_BOARD_NAME="$name"
	[ -z "$MVEBU_MODEL" ] && MVEBU_MODEL="$machine"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$MVEBU_BOARD_NAME" > /tmp/sysinfo/board_name
	echo "$MVEBU_MODEL" > /tmp/sysinfo/model
}

mvebu_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] || mvebu_board_detect
	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}
