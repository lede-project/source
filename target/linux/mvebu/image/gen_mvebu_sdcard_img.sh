#!/usr/bin/env bash
#
# Copyright (C) 2016 Josua Mayer
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

usage() {
	echo "$0 <sectors> <outfile> <bootloader> [<type_partitionN> <sectors_partitionN> <img_partitionN>]?"
}

# always require first 3 arguments
# then in pairs up to 8 more for a total of up to 4 partitions
if [ $# -lt 3 ] || [ $# -gt 15 ] || [ $(($#%3)) -ne 0 ]; then
	usage
	exit 1
fi

set -e

# parameters
IMGSIZE=$1; shift
OUTFILE="$1"; shift
BOOTLOADER="$1"; shift

# generate image file
printf "Creating $OUTFILE from /dev/zero: "
dd if=/dev/zero of="$OUTFILE" bs=512 count=1 >/dev/null
printf "Done\n"

while [ "$#" -ge 3 ]; do
	ptgen_args="$ptgen_args -p $(($2 / 2)) -S 0x$1"
	parts="$parts$3 "
	shift; shift; shift
done

head=16
sect=63

# create real partition table using fdisk
printf "Creating partition table: "
set `ptgen -o "$OUTFILE" -h $head -s $sect -l 1024 $ptgen_args`
printf "Done\n"

# install bootloader
printf "Writing bootloader: "
dd of="$OUTFILE" if="$BOOTLOADER" bs=512 seek=1 conv=notrunc 2>/dev/null
printf "Done\n"

i=1
while [ "$#" -ge 2 ]; do
	img="${parts%% *}"
	parts="${parts#* }"

	printf "Writing %s to partition %i: " "$img" $i
	dd if="$img" of="$OUTFILE" bs=512 seek=$(($1 / 512)) conv=notrunc 2>/dev/null
	printf "Done\n"

	let i=i+1
	shift; shift
done
