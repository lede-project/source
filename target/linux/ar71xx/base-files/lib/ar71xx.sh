#!/bin/sh
#
# Copyright (C) 2009-2011 OpenWrt.org
#

AR71XX_BOARD_NAME=
AR71XX_MODEL=

ar71xx_get_mtd_offset_size_format() {
	local mtd="$1"
	local offset="$2"
	local size="$3"
	local format="$4"
	local dev

	dev=$(find_mtd_part $mtd)
	[ -z "$dev" ] && return

	dd if=$dev bs=1 skip=$offset count=$size 2>/dev/null | hexdump -v -e "1/1 \"$format\""
}

ar71xx_get_mtd_part_magic() {
	local mtd="$1"
	ar71xx_get_mtd_offset_size_format "$mtd" 0 4 %02x
}

wndr3700_board_detect() {
	local machine="$1"
	local magic
	local name

	name="wndr3700"

	magic="$(ar71xx_get_mtd_part_magic firmware)"
	case $magic in
	33373030)
		machine="NETGEAR WNDR3700"
		;;
	33373031)
		model="$(ar71xx_get_mtd_offset_size_format art 41 32 %c)"
		# Use awk to remove everything unprintable
		model_stripped="$(ar71xx_get_mtd_offset_size_format art 41 32 %c | LC_CTYPE=C awk -v 'FS=[^[:print:]]' '{print $1; exit}')"
		case $model in
		$'\xff'*)
			if [ "${model:24:1}" = 'N' ]; then
				machine="NETGEAR WNDRMAC"
			else
				machine="NETGEAR WNDR3700v2"
			fi
			;;
		'29763654+16+64'*)
			machine="NETGEAR ${model_stripped:14}"
			;;
		'29763654+16+128'*)
			machine="NETGEAR ${model_stripped:15}"
			;;
		*)
			# Unknown ID
			machine="NETGEAR ${model_stripped}"
		esac
	esac

	AR71XX_BOARD_NAME="$name"
	AR71XX_MODEL="$machine"
}

ubnt_get_mtd_part_magic() {
	ar71xx_get_mtd_offset_size_format EEPROM 4118 2 %02x
}

ubnt_xm_board_detect() {
	local model
	local magic

	magic="$(ubnt_get_mtd_part_magic)"
	case ${magic:0:3} in
	e00|\
	e01|\
	e80)
		model="Ubiquiti NanoStation M"
		;;
	e0a)
		model="Ubiquiti NanoStation loco M"
		;;
	e1b|\
	e1d)
		model="Ubiquiti Rocket M"
		;;
	e20|\
	e2d)
		model="Ubiquiti Bullet M"
		;;
	e30)
		model="Ubiquiti PicoStation M"
		;;
	esac

	[ -z "$model" ] || AR71XX_MODEL="${model}${magic:3:1}"
}

cybertan_get_hw_magic() {
	local part

	part=$(find_mtd_part firmware)
	[ -z "$part" ] && return 1

	dd bs=8 count=1 skip=0 if=$part 2>/dev/null | hexdump -v -n 8 -e '1/1 "%02x"'
}

dir505_board_detect() {
	local dev=$(find_mtd_part 'mac')
	[ -z "$dev" ] && return

	# The revision is stored at the beginning of the "mac" partition
	local rev="$(LC_CTYPE=C awk -v 'FS=[^[:print:]]' '{print $1; exit}' $dev)"
	AR71XX_MODEL="D-Link DIR-505 rev. $rev"
}

tplink_get_hwid() {
	local part

	part=$(find_mtd_part firmware)
	[ -z "$part" ] && return 1

	dd if=$part bs=4 count=1 skip=16 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"'
}

tplink_get_mid() {
	local part

	part=$(find_mtd_part firmware)
	[ -z "$part" ] && return 1

	dd if=$part bs=4 count=1 skip=17 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"'
}

tplink_board_detect() {
	local model="$1"
	local hwid
	local hwver

	hwid=$(tplink_get_hwid)
	mid=$(tplink_get_mid)
	hwver=${hwid:6:2}
	hwver=" v${hwver#0}"

	case "$hwid" in
	001001*)
		model="TP-Link TL-MR10U"
		;;
	001101*)
		model="TP-Link TL-MR11U"
		;;
	001201*)
		model="TP-Link TL-MR12U"
		;;
	001301*)
		model="TP-Link TL-MR13U"
		;;
	007260*)
		model="TellStick ZNet Lite"
		;;
	015000*)
		model="EasyLink EL-M150"
		;;
	015300*)
		model="EasyLink EL-MINI"
		;;
	044401*)
		model="ANTMINER-S1"
		;;
	044403*)
		model="ANTMINER-S3"
		;;
	066601*)
		model="OMYlink OMY-G1"
		;;
	066602*)
		model="OMYlink OMY-X1"
		;;
	070100*)
		model="TP-Link TL-WA701N/ND"
		;;
	070301*)
		model="TP-Link TL-WR703N"
		;;
	071000*)
		model="TP-Link TL-WR710N"

		if [ "$hwid" = '07100002' -a "$mid" = '00000002' ]; then
			hwver=' v2.1'
		fi
		;;
	072001*)
		model="TP-Link TL-WR720N"
		;;
	073000*)
		model="TP-Link TL-WA730RE"
		;;
	074000*)
		model="TP-Link TL-WR740N/ND"
		;;
	074100*)
		model="TP-Link TL-WR741N/ND"
		;;
	074300*)
		model="TP-Link TL-WR743N/ND"
		;;
	075000*)
		model="TP-Link TL-WA750RE"
		;;
	080100*)
		model="TP-Link TL-WA801N/ND"
		;;
	080200*)
		model="TP-Link TL-WR802N"
		;;
	083000*)
		model="TP-Link TL-WA830RE"

		if [ "$hwver" = ' v10' ]; then
			hwver=' v1'
		fi
		;;
	084100*)
		model="TP-Link TL-WR841N/ND"

		if [ "$hwid" = '08410002' -a "$mid" = '00000002' ]; then
			hwver=' v1.5'
		fi
		;;
	084200*)
		model="TP-Link TL-WR842N/ND"
		;;
	084300*)
		model="TP-Link TL-WR843N/ND"
		;;
	085000*)
		model="TP-Link TL-WA850RE"
		;;
	086000*)
		model="TP-Link TL-WA860RE"
		;;
	090100*)
		model="TP-Link TL-WA901N/ND"
		;;
	094000*)
		model="TP-Link TL-WR940N"
		;;
	094100*)
		if [ "$hwid" = "09410002" -a "$mid" = "00420001" ]; then
			model="Rosewill RNX-N360RT"
			hwver=""
		else
			model="TP-Link TL-WR941N/ND"
		fi
		;;
	104100*)
		model="TP-Link TL-WR1041N/ND"
		;;
	104300*)
		model="TP-Link TL-WR1043N/ND"
		;;
	120000*)
		model="MERCURY MAC1200R"
		;;
	254300*)
		model="TP-Link TL-WR2543N/ND"
		;;
	302000*)
		model="TP-Link TL-MR3020"
		;;
	304000*)
		model="TP-Link TL-MR3040"
		;;
	322000*)
		model="TP-Link TL-MR3220"
		;;
	332000*)
		model="TP-Link TL-WDR3320"
		;;
	342000*)
		model="TP-Link TL-MR3420"
		;;
	350000*)
		model="TP-Link TL-WDR3500"
		;;
	360000*)
		model="TP-Link TL-WDR3600"
		;;
	3C0001*)
		model="OOLITE"
		;;
	3C0002*)
		model="MINIBOX_V1"
		;;
	430000*)
		model="TP-Link TL-WDR4300"
		;;
	430080*)
		iw reg set IL
		model="TP-Link TL-WDR4300 (IL)"
		;;
	431000*)
		model="TP-Link TL-WDR4310"
		;;
	44440101*)
		model="ANTROUTER-R1"
		;;
	453000*)
		model="Mercury MW4530R"
		;;
	49000002)
		model="TP-Link TL-WDR4900"
		;;
	65000002)
		model="TP-Link TL-WDR6500"
		;;
	721000*)
		model="TP-Link TL-WA7210N"
		;;
	750000*|\
	c70000*)
		model="TP-Link Archer C7"
		;;
	751000*)
		model="TP-Link TL-WA7510N"
		;;
	934100*)
		model="NC-LINK SMART-300"
		;;
	c50000*)
		model="TP-Link Archer C5"
		;;
	*)
		hwver=""
		;;
	esac

	AR71XX_MODEL="$model$hwver"
}

tplink_pharos_get_model_string() {
	local part
	part=$(find_mtd_part 'product-info')
	[ -z "$part" ] && return 1

	# The returned string will end with \r\n, but we don't remove it here
	# to simplify matching against it in the sysupgrade image check
	dd if=$part bs=1 skip=4360 2>/dev/null | head -n 1
}

tplink_pharos_board_detect() {
	local model_string="$(tplink_pharos_get_model_string | tr -d '\r')"
	local oIFS="$IFS"; IFS=":"; set -- $model_string; IFS="$oIFS"

	local model="${1%%\(*}"

	AR71XX_MODEL="TP-Link $model v$2"
}

gl_inet_board_detect() {
	local size="$(mtd_get_part_size 'firmware')"

	case "$size" in
	16580608)
		AR71XX_MODEL='GL-iNet 6416A v1'
		;;
	8192000)
		AR71XX_MODEL='GL-iNet 6408A v1'
		;;
	esac
}

ar71xx_board_detect() {
	local machine
	local name

	machine=$(awk 'BEGIN{FS="[ \t]+:[ \t]"} /machine/ {print $2}' /proc/cpuinfo)

	case "$machine" in
	Abicom*)
		case "$machine" in
		*SC1750)
			name="sc1750"
			;;
		*SC300M)
			name="sc300m"
			;;
		*SC450)
			name="sc450"
			;;
		esac
		;;
	*AC1750DB)
		name="f9k1115v2"
		;;
	ALFA*)
		case "$machine" in
		*AP120C)
			name="alfa-ap120c"
			;;
		*AP96)
			name="alfa-ap96"
			;;
		*Hornet-UB)
			local size
			size=$(awk '/firmware/ { print $2 }' /proc/mtd)

			if [ "x$size" = "x00790000" ]; then
				name="hornet-ub"
			fi

			if [ "x$size" = "x00f90000" ]; then
				name="hornet-ub-x2"
			fi
			;;
		*N2/N5)
			name="alfa-nx"
			;;
		*Tube2H)
			name="tube2h"
			;;
		esac
		;;
	Allnet*)
		case "$machine" in
		*ALL0258N)
			name="all0258n"
			;;
		*ALL0305)
			name="all0305"
			;;
		*ALL0315N)
			name="all0315n"
			;;
		esac
		;;
	*Antminer-S1)
		name="antminer-s1"
		;;
	*Antminer-S3)
		name="antminer-s3"
		;;
	*Atheros*)
		case "$machine" in
		*AP121*)
			name="ap121"
			;;
		*AP121-MINI)
			name="ap121-mini"
			;;
		*AP132*)
			name="ap132"
			;;
		*AP135-020*)
			name="ap135-020"
			;;
		*AP136-010*)
			name="ap136-010"
			;;
		*AP136-020*)
			name="ap136-020"
			;;
		*AP143*)
			name="ap143"
			;;
		*AP147-010*)
			name="ap147-010"
			;;
		*AP152*)
			name="ap152"
			;;
		*AP96)
			name="ap96"
			;;
		*DB120*)
			name="db120"
			;;
		*PB42)
			name="pb42"
			;;
		*PB44*)
			name="pb44"
			;;
		esac
		;;
	*AW-NR580)
		name="aw-nr580"
		;;
	*"Black Swift board"*)
		name="bsb"
		;;
	Buffalo*)
		case "$machine" in
		*BHR-4GRV2)
			name="bhr-4grv2"
			;;
		*WHR-G301N)
			name="whr-g301n"
			;;
		*WHR-HP-G300N)
			name="whr-hp-g300n"
			;;
		*WHR-HP-GN)
			name="whr-hp-gn"
			;;
		*WLAE-AG300N)
			name="wlae-ag300n"
			;;
		*WZR-450HP2)
			name="wzr-450hp2"
			;;
		*WZR-HP-AG300H/WZR-600DHP)
			name="wzr-hp-ag300h"
			;;
		*WZR-HP-G300NH)
			name="wzr-hp-g300nh"
			;;
		*WZR-HP-G300NH2)
			name="wzr-hp-g300nh2"
			;;
		*WZR-HP-G450H)
			name="wzr-hp-g450h"
			;;
		esac
		;;
	*"BXU2000n-2 rev. A1")
		name="bxu2000n-2-a1"
		;;
	*C-55)
		name="c-55"
		;;
	*C-60)
		name="c-60"
		;;
	*CAP324)
		name="cap324"
		;;
	*CAP4200AG)
		name="cap4200ag"
		;;
	*Carambola2*)
		name="carambola2"
		;;
	COMFAST*)
		case "$machine" in
		*"E316N v2")
			name="cf-e316n-v2"
			;;
		*"E320N v2")
			name="cf-e320n-v2"
			;;
		*"E380AC v1")
			name="cf-e380ac-v1"
			;;
		*"E380AC v2")
			name="cf-e380ac-v2"
			;;
		*E520N)
			name="cf-e520n"
			;;
		*E530N)
			name="cf-e530n"
			;;
		esac
		;;
	Compex*)
		case "$machine" in
		*WP543)
			name="wp543"
			;;
		*WPE72)
			name="wpe72"
			;;
		*WPJ342)
			name="wpj342"
			;;
		*WPJ344)
			name="wpj344"
			;;
		*WPJ531)
			name="wpj531"
			;;
		*WPJ558)
			name="wpj558"
			;;
		esac
		;;
	*CR3000)
		name="cr3000"
		;;
	*CR5000)
		name="cr5000"
		;;
	D-Link*)
		case "$machine" in
		*"DAP-2695 rev. A1")
			name="dap-2695-a1"
			;;
		*"DGL-5500 rev. A1")
			name="dgl-5500-a1"
			;;
		*"DHP-1565 rev. A1")
			name="dhp-1565-a1"
			;;
		*"DIR-505 rev. A1")
			name="dir-505-a1"
			dir505_board_detect
			;;
		*"DIR-600 rev. A1")
			name="dir-600-a1"
			;;
		*"DIR-615 rev. C1")
			name="dir-615-c1"
			;;
		*"DIR-615 rev. E1")
			name="dir-615-e1"
			;;
		*"DIR-615 rev. E4")
			name="dir-615-e4"
			;;
		*"DIR-615 rev. I1")
			name="dir-615-i1"
			;;
		*"DIR-825 rev. B1")
			name="dir-825-b1"
			;;
		*"DIR-825 rev. C1")
			name="dir-825-c1"
			;;
		*"DIR-835 rev. A1")
			name="dir-835-a1"
			;;
		*"DIR-869 rev. A1")
			name="dir-869-a1"
			;;
		esac
		;;
	*"dLAN Hotspot")
		name="dlan-hotspot"
		;;
	*"dLAN pro 1200+ WiFi ac")
		name="dlan-pro-1200-ac"
		;;
	*"dLAN pro 500 Wireless+")
		name="dlan-pro-500-wp"
		;;
	*"Domino Pi")
		name="gl-domino"
		;;
	*DR344)
		name="dr344"
		;;
	*DR531)
		name="dr531"
		;;
	*"Dragino v2")
		name="dragino2"
		;;
	*DW33D)
		name="dw33d"
		;;
	*E2100L)
		name="e2100l"
		;;
	*EAP7660D)
		name="eap7660d"
		;;
	*EL-M150)
		name="el-m150"
		;;
	*EL-MINI)
		name="el-mini"
		;;
	*EmbWir-Dorin)
		name="ew-dorin"
		;;
	*EmbWir-Dorin-Router)
		name="ew-dorin-router"
		;;
	EnGenius*)
		case "$machine" in
		*"EAP300 v2")
			name="eap300v2"
			;;
		*EPG5000)
			name="epg5000"
			;;
		*ESR1750)
			name="esr1750"
			;;
		*ESR900)
			name="esr900"
			;;
		esac
		;;
	*"GL AR150")
		name="gl-ar150"
		;;
	*"GL AR300")
		name="gl-ar300"
		;;
	*GL-AR300M)
		name="gl-ar300m"
		;;
	*"GL-CONNECT INET v1")
		name="gl-inet"
		gl_inet_board_detect
		;;
	*GL-MIFI)
		name="gl-mifi"
		;;
	*HC6361)
		name="hiwifi-hc6361"
		;;
	jjPlus*)
		case "$machine" in
		*JA76PF)
			name="ja76pf"
			;;
		*JA76PF2)
			name="ja76pf2"
			;;
		*JWAP003)
			name="jwap003"
			;;
		*JWAP230)
			name="jwap230"
			;;
		esac
		;;
	*MAC1200R)
		name="mc-mac1200r"
		;;
	Meraki*)
		case "$machine" in
		*MR12)
			name="mr12"
			;;
		*MR16)
			name="mr16"
			;;
		*MR18)
			name="mr18"
			;;
		*Z1)
			name="z1"
			;;
		esac
		;;
	MikroTik*)
		case "$machine" in
		*2011L)
			name="rb-2011l"
			;;
		*2011UAS)
			name="rb-2011uas"
			;;
		*2011UAS-2HnD)
			name="rb-2011uas-2hnd"
			;;
		*2011UiAS)
			name="rb-2011uias"
			;;
		*2011UiAS-2HnD)
			name="rb-2011uias-2hnd"
			;;
		*411/A/AH)
			name="rb-411"
			;;
		*411U)
			name="rb-411u"
			;;
		*433/AH)
			name="rb-433"
			;;
		*433UAH)
			name="rb-433u"
			;;
		*435G)
			name="rb-435g"
			;;
		*450)
			name="rb-450"
			;;
		*450G)
			name="rb-450g"
			;;
		*493/AH)
			name="rb-493"
			;;
		*493G)
			name="rb-493g"
			;;
		*750)
			name="rb-750"
			;;
		*750GL)
			name="rb-750gl"
			;;
		*751)
			name="rb-751"
			;;
		*751G)
			name="rb-751g"
			;;
		*911G-2HPnD)
			name="rb-911g-2hpnd"
			;;
		*911G-5HPacD)
			name="rb-911g-5hpacd"
			;;
		*911G-5HPnD)
			name="rb-911g-5hpnd"
			;;
		*912UAG-2HPnD)
			name="rb-912uag-2hpnd"
			;;
		*912UAG-5HPnD)
			name="rb-912uag-5hpnd"
			;;
		*941-2nD)
			name="rb-941-2nd"
			;;
		*951G-2HnD)
			name="rb-951g-2hnd"
			;;
		*951Ui-2HnD)
			name="rb-951ui-2hnd"
			;;
		*"SXT Lite2")
			name="rb-sxt2n"
			;;
		*"SXT Lite5")
			name="rb-sxt5n"
			;;
		esac
		;;
	*"MiniBox V1.0")
		name="minibox-v1"
		;;
	*MZK-W04NU)
		name="mzk-w04nu"
		;;
	*MZK-W300NH)
		name="mzk-w300nh"
		;;
	NETGEAR*)
		case "$machine" in
		*R6100)
			name="r6100"
			;;
		*WNDAP360)
			name="wndap360"
			;;
		*WNDR3700/WNDR3800/WNDRMAC)
			wndr3700_board_detect "$machine"
			;;
		*WNDR3700v4)
			name="wndr3700v4"
			;;
		*WNDR4300)
			name="wndr4300"
			;;
		*"WNR1000 V2")
			name="wnr1000-v2"
			;;
		*WNR2000)
			name="wnr2000"
			;;
		*"WNR2000 V3")
			name="wnr2000-v3"
			;;
		*"WNR2000 V4")
			name="wnr2000-v4"
			;;
		*WNR2200)
			name="wnr2200"
			;;
		*"WNR612 V2")
			name="wnr612-v2"
			;;
		*WPN824N)
			name="wpn824n"
			;;
		esac
		;;
	*OMY-G1)
		name="omy-g1"
		;;
	*OMY-X1)
		name="omy-x1"
		;;
	*"Onion Omega")
		name="onion-omega"
		;;
	*"Oolite V1.0")
		name="oolite"
		;;
	OpenMesh*)
		case "$machine" in
		*MR1750)
			name="mr1750"
			;;
		*MR1750v2)
			name="mr1750v2"
			;;
		*MR600)
			name="mr600"
			;;
		*MR600v2)
			name="mr600v2"
			;;
		*MR900)
			name="mr900"
			;;
		*MR900v2)
			name="mr900v2"
			;;
		*OM2P)
			name="om2p"
			;;
		*"OM2P HS")
			name="om2p-hs"
			;;
		*"OM2P HSv2")
			name="om2p-hsv2"
			;;
		*"OM2P HSv3")
			name="om2p-hsv3"
			;;
		*"OM2P LC")
			name="om2p-lc"
			;;
		*OM2Pv2)
			name="om2pv2"
			;;
		*OM5P)
			name="om5p"
			;;
		*"OM5P AC")
			name="om5p-ac"
			;;
		*"OM5P ACv2")
			name="om5p-acv2"
			;;
		*"OM5P AN")
			name="om5p-an"
			;;
		esac
		;;
	*"PQI Air Pen")
		name="pqi-air-pen"
		;;
	*"Qihoo 360 C301")
		name="qihoo-c301"
		;;
	*RW2458N)
		name="rw2458n"
		;;
	*SMART-300)
		name="smart-300"
		;;
	*SOM9331)
		name="som9331"
		;;
	*"TellStick ZNet Lite")
		name="tellstick-znet-lite"
		;;
	TP-LINK*)
		case "$machine" in
		*C5)
			name="archer-c5"
			;;
		*"C59 v1")
			name="archer-c59-v1"
			;;
		*"C60 v1")
			name="archer-c60-v1"
			;;
		*C7)
			name="archer-c7"
			;;
		*CPE210/220)
			name="cpe210"
			tplink_pharos_board_detect
			;;
		*CPE510/520)
			name="cpe510"
			tplink_pharos_board_detect
			;;
		*EAP120)
			name="eap120"
			tplink_pharos_board_detect
			;;
		*MR10U)
			name="tl-mr10u"
			;;
		*MR11U)
			name="tl-mr11u"
			;;
		*MR12U)
			name="tl-mr12u"
			;;
		*"MR13U v1")
			name="tl-mr13u"
			;;
		*MR3020)
			name="tl-mr3020"
			;;
		*MR3040)
			name="tl-mr3040"
			;;
		*"MR3040 v2")
			name="tl-mr3040-v2"
			;;
		*MR3220)
			name="tl-mr3220"
			;;
		*"MR3220 v2")
			name="tl-mr3220-v2"
			;;
		*MR3420)
			name="tl-mr3420"
			;;
		*"MR3420 v2")
			name="tl-mr3420-v2"
			;;
		*RE450)
			name="re450"
			;;
		*"WA701ND v2")
			name="tl-wa701nd-v2"
			;;
		*"WA7210N v2")
			name="tl-wa7210n-v2"
			;;
		*WA750RE)
			name="tl-wa750re"
			;;
		*"WA7510N v1")
			name="tl-wa7510n"
			;;
		*"WA801ND v2")
			name="tl-wa801nd-v2"
			;;
		*"WA801ND v3")
			name="tl-wa801nd-v3"
			;;
		*"WA830RE v2")
			name="tl-wa830re-v2"
			;;
		*WA850RE)
			name="tl-wa850re"
			;;
		*WA860RE)
			name="tl-wa860re"
			;;
		*WA901ND)
			name="tl-wa901nd"
			;;
		*"WA901ND v2")
			name="tl-wa901nd-v2"
			;;
		*"WA901ND v3")
			name="tl-wa901nd-v3"
			;;
		*"WA901ND v4")
			name="tl-wa901nd-v4"
			;;
		*WBS210)
			name="wbs210"
			tplink_pharos_board_detect
			;;
		*WBS510)
			name="wbs510"
			tplink_pharos_board_detect
			;;
		*"WDR3320 v2")
			name="tl-wdr3320-v2"
			;;
		*WDR3500)
			name="tl-wdr3500"
			;;
		*WDR3600/4300/4310)
			name="tl-wdr4300"
			;;
		*"WDR4900 v2")
			name="tl-wdr4900-v2"
			;;
		*"WDR6500 v2")
			name="tl-wdr6500-v2"
			;;
		*WPA8630)
			name="tl-wpa8630"
			;;
		*"WR1041N v2")
			name="tl-wr1041n-v2"
			;;
		*WR1043ND)
			name="tl-wr1043nd"
			;;
		*"WR1043ND v2")
			name="tl-wr1043nd-v2"
			;;
		*"WR1043ND v4")
			name="tl-wr1043nd-v4"
			;;
		*WR2543N*)
			name="tl-wr2543n"
			;;
		*"WR703N v1")
			name="tl-wr703n"
			;;
		*"WR710N v1")
			name="tl-wr710n"
			;;
		*WR720N*)
			name="tl-wr720n-v3"
			;;
		*WR741ND)
			name="tl-wr741nd"
			;;
		*"WR741ND v4")
			name="tl-wr741nd-v4"
			;;
		*"WR802N v1")
			name="tl-wr802n-v1"
			;;
		*WR810N)
			name="tl-wr810n"
			;;
		*"WR841N v1")
			name="tl-wr841n-v1"
			;;
		*"WR841N/ND v11")
			name="tl-wr841n-v11"
			;;
		*"WR841N/ND v7")
			name="tl-wr841n-v7"
			;;
		*"WR841N/ND v8")
			name="tl-wr841n-v8"
			;;
		*"WR841N/ND v9")
			name="tl-wr841n-v9"
			;;
		*"WR842N/ND v2")
			name="tl-wr842n-v2"
			;;
		*"WR842N/ND v3")
			name="tl-wr842n-v3"
			;;
		*"WR940N v4")
			name="tl-wr940n-v4"
			;;
		*"WR941N/ND v5")
			name="tl-wr941nd-v5"
			;;
		*"WR941N/ND v6")
			name="tl-wr941nd-v6"
			;;
		*WR941ND)
			name="tl-wr941nd"
			;;
		esac

		[ -z "$AR71XX_MODEL" ] && tplink_board_detect "$machine"
		;;
	TRENDnet*)
		case "$machine" in
		*632BRP)
			name="tew-632brp"
			;;
		*673GRU)
			name="tew-673gru"
			;;
		*712BR)
			name="tew-712br"
			;;
		*732BR)
			name="tew-732br"
			;;
		*823DRU)
			name="tew-823dru"
			;;
		esac
		;;
	Ubiquiti*)
		case "$machine" in
		*AirGateway)
			name="airgateway"
			;;
		*"AirGateway Pro")
			name="airgatewaypro"
			;;
		*AirRouter)
			name="airrouter"
			;;
		*"Bullet M")
			name="bullet-m"
			ubnt_xm_board_detect
			;;
		*"Loco M XW")
			name="loco-m-xw"
			;;
		*LS-SR71)
			name="ls-sr71"
			;;
		*"Nanostation M")
			name="nanostation-m"
			ubnt_xm_board_detect
			;;
		*"Nanostation M XW")
			name="nanostation-m-xw"
			;;
		*"Rocket M")
			name="rocket-m"
			ubnt_xm_board_detect
			;;
		*"Rocket M TI")
			name="rocket-m-ti"
			;;
		*"Rocket M XW")
			name="rocket-m-xw"
			;;
		*RouterStation)
			name="routerstation"
			;;
		*"RouterStation Pro")
			name="routerstation-pro"
			;;
		*UniFi)
			name="unifi"
			;;
		*"UniFi AP Pro")
			name="uap-pro"
			;;
		*UniFi-AC-LITE)
			name="unifiac-lite"
			;;
		*UniFi-AC-PRO)
			name="unifiac-pro"
			;;
		*"UniFiAP Outdoor")
			name="unifi-outdoor"
			;;
		*"UniFiAP Outdoor+")
			name="unifi-outdoor-plus"
			;;
		esac
		;;
	"WD My Net"*)
		case "$machine" in
		*N600)
			name="mynet-n600"
			;;
		*N750)
			name="mynet-n750"
			;;
		*"Wi-Fi Range"*)
			name="mynet-rext"
			;;
		esac
		;;
	WeIO*)
		name="weio"
		;;
	*WLR-8100)
		name="wlr8100"
		;;
	*WRT160NL)
		name="wrt160nl"
		;;
	*WRT400N)
		name="wrt400n"
		;;
	*WRTnode2Q*)
		name="wrtnode2q"
		;;
	*Yun)
		name="arduino-yun"
		;;
	YunCore*)
		case "$machine" in
		*AP90Q)
			name="ap90q"
			;;
		*CPE830)
			name="cpe830"
			;;
		*CPE870)
			name="cpe870"
			;;
		*SR3200)
			name="sr3200"
			;;
		*XD3200)
			name="xd3200"
			;;
		esac
		;;
	*ZBT-WE1526)
		name="zbt-we1526"
		;;
	*ZCN-1523H-2)
		name="zcn-1523h-2"
		;;
	*ZCN-1523H-5)
		name="zcn-1523h-5"
		;;
	Zyxel*)
		case "$machine" in
		*NBG460N/550N/550NH)
			name="nbg460n_550n_550nh"
			;;
		*NBG6616)
			name="nbg6616"
			;;
		*NBG6716)
			name="nbg6716"
			;;
		esac
		;;
	esac

	[ -z "$name" ] && name="unknown"

	[ -z "$AR71XX_BOARD_NAME" ] && AR71XX_BOARD_NAME="$name"
	[ -z "$AR71XX_MODEL" ] && AR71XX_MODEL="$machine"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$AR71XX_BOARD_NAME" > /tmp/sysinfo/board_name
	echo "$AR71XX_MODEL" > /tmp/sysinfo/model
}

ar71xx_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}
