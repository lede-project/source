#
# Copyright (C) 2014-2016 OpenWrt.org
# Copyright (C) 2016 LEDE-Project.org
#

. /lib/mvebu.sh

RAMFS_COPY_DATA=/lib/mvebu.sh

platform_check_image() {
	local board=$(mvebu_board_name)
	local magic_long="$(get_magic_long "$1")"

	[ "$#" -gt 1 ] && return 1

	case "$board" in
	armada-385-linksys-caiman|armada-385-linksys-cobra|armada-385-linksys-rango|armada-385-linksys-shelby|armada-xp-linksys-mamba)
		[ "$magic_long" != "27051956" -a "$magic_long" != "73797375" ] && {
			echo "Invalid image type."
			return 1
		}
		return 0;
		;;
	armada-388-clearfog)
		platform_check_image_clearfog "$ARGV"
		return $?
		;;
	esac

	echo "Sysupgrade is not yet supported on $board."
	return 1
}

platform_do_upgrade() {
	local board=$(mvebu_board_name)

	case "$board" in
	armada-385-linksys-caiman|armada-385-linksys-cobra|armada-385-linksys-rango|armada-385-linksys-shelby|armada-xp-linksys-mamba)
		platform_do_upgrade_linksys "$ARGV"
		;;
	armada-388-clearfog)
		platform_do_upgrade_clearfog "$ARGV"
		;;
	*)
		default_do_upgrade "$ARGV"
		;;
	esac
}
platform_copy_config() {
	local board=$(mvebu_board_name)

	case "$board" in
	armada-388-clearfog)
		platform_copy_config_clearfog "$ARGV"
		;;
	esac
}

disable_watchdog() {
	killall watchdog
	( ps | grep -v 'grep' | grep '/dev/watchdog' ) && {
		echo 'Could not disable watchdog'
		return 1
	}
}

append sysupgrade_pre_upgrade disable_watchdog
