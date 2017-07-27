#
# Copyright (C) 2016 OpenWrt.org
#

platform_do_upgrade() {
	local tar_file="$1"
	local board="$(board_name)"

	echo "flashing kernel"
	tar xf $tar_file sysupgrade-$board/kernel -O | mtd write - kernel

	echo "flashing rootfs"
	tar xf $tar_file sysupgrade-$board/root -O | mtd write - rootfs

	return 0
}

platform_check_image() {
	local tar_file="$1"
	local board=$(board_name)

	case "$board" in
	mediatek,mt7623-rfb-nand-ephy |\
	mediatek,mt7623-rfb-nand)
		nand_do_platform_check $board $1
		return $?
		;;
	mediatek,mt7623-rfb-emmc)
		local kernel_length=`(tar xf $tar_file sysupgrade-$board/kernel -O | wc -c) 2> /dev/null`
		local rootfs_length=`(tar xf $tar_file sysupgrade-$board/root -O | wc -c) 2> /dev/null`
		;;

	*)
		echo "Sysupgrade is not supported on your board yet."
		return 1
		;;
	esac

	[ "$kernel_length" = 0 -o "$rootfs_length" = 0 ] && {
		echo "The upgarde image is corrupt."
		return 1
	}

	return 0
}

platform_pre_upgrade() {
	case "$(board_name)" in
	mediatek,mt7623-rfb-nand-ephy |\
	mediatek,mt7623-rfb-nand)
		nand_do_upgrade $1
		;;
	esac
}
