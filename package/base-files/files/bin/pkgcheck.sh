#!/bin/sh

OPKG_INFO_DIR=/usr/lib/opkg/info/
OPKG_ROM_INFO_DIR=/rom/usr/lib/opkg/info/

[ ! -d "${OPKG_ROM_INFO_DIR}" ] && {
	echo "No ROM filesystem found!"
	exit 1
}

package_diff="$(diff -q "${OPKG_INFO_DIR}" "${OPKG_ROM_INFO_DIR}")"

echo "$package_diff" | \
	sed -e "s|Only in ${OPKG_INFO_DIR}: \(.*\)\..*|Added/Modified: \1|" | \
	sed -e "s|Files ${OPKG_INFO_DIR}\(.*\)\..* and ${OPKG_ROM_INFO_DIR}.* differ|Added/Modified: \1|" | \
	sed -e "s|Only in ${OPKG_ROM_INFO_DIR}: \(.*\)\..*|Deleted:        \1|" | \
	sort | uniq
