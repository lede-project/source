#!/bin/sh
#
# Copyright 2015-2016 Traverse Technologies
#
layerscape_board_detect() {
	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"
	echo "layerscape" > /tmp/sysinfo/board_name
	echo "generic" > /tmp/sysinfo/model
}
layerscape_board_name() {
	echo "layerscape"
}
platform_do_upgrade() {
	bootsys=`fw_printenv bootsys | awk -F '=' '{{print $2}}'`
	newbootsys=2
	if [ "$bootsys" -eq "2" ]; then
		newbootsys=1;
	fi
	mkdir -p /tmp/image
	cd /tmp/image
	get_image "$1" > image.tar
	ls -la image.tar
	files=`tar -tf image.tar`
	echo "Files in image"
	echo $files
	for f in $files
	do
		stripped=`echo $f | awk -F '/' '{{print $2}}'`
		if [ -z $stripped ] || [ $stripped == "CONTROL" ]; then
			continue
		fi
		if [ $stripped == "root" ]; then
			stripped="rootfs"
		fi
		volume=$stripped
		if [ $stripped == "kernel" ] || [ $stripped == "rootfs" ]; then
			volume=$stripped$newbootsys
		fi
		volume_id=`ubinfo -d 0 --name $volume | grep 'Volume ID' | awk '{print $3}'`
		file_size=`tar -tvf image.tar $f | awk '{{print \$3}}'`
		echo "$f size $file_size"
		tar -xOf image.tar $f | ubiupdatevol -s $file_size /dev/ubi0_$volume_id -
		
		echo "$volume upgraded"
	done
	fw_setenv bootsys $newbootsys
	echo "Upgrade complete"
}
platform_copy_config() {
	bootsys=`fw_printenv bootsys | awk -F '=' '{{print $2}}'`
	rootvol=rootfs$bootsys
	volume_id=`ubinfo -d 0 --name $rootvol | grep 'Volume ID' | awk '{print $3}'`
	mount -t ubifs -o rw,noatime /dev/ubi0_$volume_id /mnt
	cp -af "$CONF_TAR" /mnt
	umount /mnt
}
platform_check_image() {
	echo "** platform_check_image **"
	local board=$(board_name)

	case "$board" in
	traverse,ls1043v | \
	traverse,five64)
		local tar_file="$1"
		local kernel_length=`(tar xf $tar_file sysupgrade-traverse-five64/kernel -O | wc -c) 2> /dev/null`
		local rootfs_length=`(tar xf $tar_file sysupgrade-traverse-five64/root -O | wc -c) 2> /dev/null`
		[ "$kernel_length" = 0 -o "$rootfs_length" = 0 ] && {
			echo "The upgarde image is corrupt."
			return 1
		}
		return 0
		;;
	esac

	echo "Sysupgrade is not yet supported on $board"

	return 1
}

platform_pre_upgrade() {
	echo "** platform_pre_upgrade **"
	
	echo "COPY_BIN: ${RAMFS_COPY_BIN} COPY_DATA: ${RAMFS_COPY_DATA}"
	
	# Force the creation of fw_printenv.lock
	mkdir -p /var/lock
	touch /var/lock/fw_printenv.lock
	
	export RAMFS_COPY_BIN="/usr/sbin/fw_printenv /usr/sbin/fw_setenv /usr/sbin/ubinfo /bin/echo ${RAMFS_COPY_BIN}"
	export RAMFS_COPY_DATA="/etc/fw_env.config /var/lock/fw_printenv.lock ${RAMFS_COPY_DATA}"
}
