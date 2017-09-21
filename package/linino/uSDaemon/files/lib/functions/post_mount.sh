#!/bin/sh

SD=/mnt/sd
WWW=/www/sd

_symlink_if_special() {
	local special_folder=$1
	local source=$2
	local destination=$3

	if [ -d $special_folder ]; then
		if [ ! -L $destination ] || [ ! -d $destination ]; then
			ln -s $source $destination
			logger -t "automount" "$special_folder folder found: $source is now available at $destination"
		fi
	fi
}

_remove_symlink_if_invalid() {
	local symlink=$1
	if [ -L $symlink ] && [ ! -d $symlink ]; then
		logger -t "automount" "Removing $symlink"
		rm $symlink
	fi
}

post_auto_mount() {
	local mnt_path=$1

	_symlink_if_special "$mnt_path/arduino" $mnt_path $SD
	_symlink_if_special "$mnt_path/arduino/www" "$mnt_path/arduino/www" $WWW
}

post_auto_umount() {
	_remove_symlink_if_invalid $SD
	_remove_symlink_if_invalid $WWW
}
