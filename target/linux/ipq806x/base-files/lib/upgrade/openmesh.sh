# The U-Boot loader of the OpenMesh devices requires image sizes and
# checksums to be provided in the U-Boot environment.
# The OpenMesh devices come with 2 main partitions - while one is active
# sysupgrade will flash the other. The boot order is changed to boot the
# newly flashed partition. If the new partition can't be booted due to
# upgrade failures the previously used partition is loaded.

platform_do_upgrade_openmesh() {
	local tar_file="$1"
	local restore_backup
	local primary_kernel_mtd

	local setenv_script="/tmp/fw_env_upgrade"

	local kernel_mtd="$(find_mtd_index $PART_NAME)"
	local kernel_offset="$(cat /sys/class/mtd/mtd${kernel_mtd}/offset)"
	local total_size="$(cat /sys/class/mtd/mtd${kernel_mtd}/size)"

	# switch to second kernel when flashing to secondary flash region
	local next_boot_part="1"
	case "$(board_name)" in
	openmesh,a42)
		primary_kernel_mtd=8
		;;
	*)
		echo "failed to detect primary kernel mtd partition for board"
		return 1
		;;
	esac
	[ "$kernel_mtd" = "$primary_kernel_mtd" ] || next_boot_part="2"

	local board_dir=$(tar tf $tar_file | grep -m 1 '^sysupgrade-.*/$')
	board_dir=${board_dir%/}

	local kernel_length=$(tar xf $tar_file ${board_dir}/kernel -O | wc -c)
	local rootfs_length=$(tar xf $tar_file ${board_dir}/root -O | wc -c)
	# rootfs without EOF marker
	rootfs_length=$((rootfs_length-4))

	local kernel_md5=$(tar xf $tar_file ${board_dir}/kernel -O | md5sum); kernel_md5="${kernel_md5%% *}"
	# md5 checksum of rootfs with EOF marker
	local rootfs_md5=$(tar xf $tar_file ${board_dir}/root -O | dd bs=1 count=$rootfs_length | md5sum); rootfs_md5="${rootfs_md5%% *}"

	#
	# add tar support to get_image() to use default_do_upgrade() instead?
	#

	# take care of restoring a saved config
	[ "$SAVE_CONFIG" -eq 1 ] && restore_backup="${MTD_CONFIG_ARGS} -j ${CONF_TAR}"

	# write concatinated kernel + rootfs to flash
	tar xf $tar_file ${board_dir}/kernel ${board_dir}/root -O | \
		mtd $restore_backup write - $PART_NAME

	# prepare new u-boot env
	if [ "$next_boot_part" = "1" ]; then
		echo "bootseq 1,2" > $setenv_script
	else
		echo "bootseq 2,1" > $setenv_script
	fi

	printf "kernel_size_%i 0x%08x\n" $next_boot_part $kernel_length >> $setenv_script
	printf "vmlinux_start_addr 0x%08x\n" ${kernel_offset} >> $setenv_script
	printf "vmlinux_size 0x%08x\n" ${kernel_length} >> $setenv_script
	printf "vmlinux_checksum %s\n" ${kernel_md5} >> $setenv_script

	printf "rootfs_size_%i 0x%08x\n" $next_boot_part $((total_size-kernel_length)) >> $setenv_script
	printf "rootfs_start_addr 0x%08x\n" $((kernel_offset+kernel_length)) >> $setenv_script
	printf "rootfs_size 0x%08x\n" ${rootfs_length} >> $setenv_script
	printf "rootfs_checksum %s\n" ${rootfs_md5} >> $setenv_script

	# store u-boot env changes
	fw_setenv -s $setenv_script || {
		echo "failed to update U-Boot environment"
		return 1
	}
}

# make sure we got uboot-envtools and fw_env.config copied over to the ramfs
# create /var/lock for the lock "fw_setenv.lock" of fw_setenv
platform_add_ramfs_ubootenv()
{
        [ -e /usr/sbin/fw_printenv ] && install_bin /usr/sbin/fw_printenv /usr/sbin/fw_setenv
        [ -e /etc/fw_env.config ] && install_file /etc/fw_env.config
        mkdir -p $RAM_ROOT/var/lock
}
append sysupgrade_pre_upgrade platform_add_ramfs_ubootenv
