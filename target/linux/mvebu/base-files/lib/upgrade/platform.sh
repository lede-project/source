#
# Copyright (C) 2014-2016 OpenWrt.org
# Copyright (C) 2016 LEDE-Project.org
#

. /lib/mvebu.sh

RAMFS_COPY_BIN='/usr/sbin/fw_printenv /usr/sbin/fw_setenv'
RAMFS_COPY_DATA='/lib/mvebu.sh /etc/fw_env.config /var/lock/fw_printenv.lock'
REQUIRE_IMAGE_METADATA=1

platform_check_image() {
	return 0
}

platform_do_upgrade() {
	local board=$(mvebu_board_name)

	case "$board" in
	armada-385-linksys-caiman|armada-385-linksys-cobra|armada-385-linksys-rango|armada-385-linksys-shelby|armada-xp-linksys-mamba)
		platform_do_upgrade_linksys "$ARGV"
		;;
	armada-388-clearfog-pro)
		platform_do_upgrade_clearfog "$ARGV"
		;;
	armada-388-clearfog-base)
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
	armada-385-linksys-caiman|armada-385-linksys-cobra|armada-385-linksys-rango|armada-385-linksys-shelby|armada-xp-linksys-mamba)
		platform_copy_config_linksys
		;;
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
