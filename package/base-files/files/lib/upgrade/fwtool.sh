fwtool_pre_upgrade() {
	fwtool -q -i /dev/null "$1"
}

fwtool_check_image() {
	[ $# -gt 1 ] && return 1

	. /usr/share/libubox/jshn.sh

	if ! fwtool -q -i /tmp/sysupgrade.meta "$1"; then
		echo "Image metadata not found"
		[ "$REQUIRE_IMAGE_METADATA" = 1 -a "$FORCE" != 1 ] && {
			echo "Use sysupgrade -F to override this check when downgrading or flashing to vendor firmware"
		}
		[ "$REQUIRE_IMAGE_METADATA" = 1 ] && return 1
		return 0
	fi

	json_load "$(cat /tmp/sysupgrade.meta)" || {
		echo "Invalid image metadata"
		return 1
	}

	device="$(cat /tmp/sysinfo/board_name)"

	json_select supported_devices || return 1

	json_get_keys dev_keys
	for k in $dev_keys; do
		json_get_var dev "$k"
		[ "$dev" = "$device" ] && return 0
	done

	echo "Device $device not supported by this image"
	echo -n "Supported devices:"
	for k in $dev_keys; do
		json_get_var dev "$k"
		echo -n " $dev"
	done
	echo

	return 1
}

fwtool_append_meta_config() {
	[ $# -gt 1 ] && return 1

	. /usr/share/libubox/jshn.sh
	. /etc/openwrt_release

	local sha256_file board

	sha256_file=$(sha256sum "$1")
	sha256_file="${sha256_file%% *}"

	if [ -f "/tmp/sysinfo/board_name" ]; then
		board="$(cat /tmp/sysinfo/board_name)"
	else
		board="unknown"
	fi

	json_init
	json_add_object "version"
	json_add_string "dist" "$DISTRIB_ID"
	json_add_string "version" "$DISTRIB_RELEASE"
	json_add_string "revision" "$DISTRIB_REVISION"
	json_close_object
	json_add_object "device"
	json_add_string "target" "${DISTRIB_TARGET%%/*}"
	json_add_string "subtarget" "${DISTRIB_TARGET##*/}"
	json_add_string "board" "$board"
	json_add_string "hostname" "$(hostname)"
	json_close_object
	json_add_object "checksum"
	json_add_string "sha256sum" "$sha256_file"
	json_close_object
	json_dump | fwtool -I - "$1"

	return 0
}

fwtool_check_meta_config() {
	[ $# -gt 1 ] && return 1

	. /usr/share/libubox/jshn.sh
	. /etc/openwrt_release

	local meta board_info sha256_file
	local sha256sum board

	meta=$(fwtool -t -i - "$1" 2>/dev/null)
	[ -z "$meta" ] && return 2

	sha256_file=$(sha256sum "$1")
	sha256_file="${sha256_file%% *}"

	echo "$meta" | fwtool -I - "$1"

	if [ -f "/tmp/sysinfo/board_name" ]; then
		board_info="$(cat /tmp/sysinfo/board_name)"
	else
		board_info="unknown"
	fi

	json_load "$meta"
	json_select checksum
	json_get_var sha256sum sha256sum
	json_select ..
	json_select device
	json_get_var board board
	json_select ..

	[ "$sha256sum" != "$sha256_file" ] && return 3
	[ "$board" != "$board_info" ] && return 4

	return 0
}
