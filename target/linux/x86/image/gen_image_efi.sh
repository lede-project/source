#!/usr/bin/env bash
# Copyright (C) 2006-2012 OpenWrt.org
set -x
[ $# == 5 -o $# == 6 ] || {
    echo "SYNTAX: $0 <file> <kernel size> <kernel directory> <rootfs size> <rootfs image> [<align>]"
    exit 1
}

OUTPUT="$1"
KERNELSIZE="$2"
KERNELDIR="$3"
ROOTFSSIZE="$4"
ROOTFSIMAGE="$5"
ALIGN="$6"

rm -f "$OUTPUT"

head=16
sect=63
cyl=$(( ($KERNELSIZE + $ROOTFSSIZE) * 1024 * 1024 / ($head * $sect * 512)))

# create partition table
# the first partition is FAT/FAT32 formatted, can be either 0x0b (chs) or 0x0c (lba).
# also explicitly state 0x83 for rootfs partition choice 
set `ptgen -o "$OUTPUT" -h $head -s $sect -t 0x0b -p ${KERNELSIZE}m -t 0x83 -p ${ROOTFSSIZE}m ${ALIGN:+-l $ALIGN} ${SIGNATURE:+-S 0x$SIGNATURE}`

KERNELOFFSET="$(($1 / 512))"
KERNELSIZE="$2"
ROOTFSOFFSET="$(($3 / 512))"
ROOTFSSIZE="$(($4 / 512))"

[ -n "$PADDING" ] && dd if=/dev/zero of="$OUTPUT" bs=512 seek="$ROOTFSOFFSET" conv=notrunc count="$ROOTFSSIZE"
dd if="$ROOTFSIMAGE" of="$OUTPUT" bs=512 seek="$ROOTFSOFFSET" conv=notrunc

[ -n "$NOGRUB" ] && exit 0

#make_ext4fs -J -l "$KERNELSIZE" "$OUTPUT.kernel" "$KERNELDIR"
echo "Creating FAT formatted boot image partition"
dd if=/dev/zero bs=512 count=0 seek="$(($KERNELSIZE/512))" of="$OUTPUT.kernel" &&
mkfs.fat -n BOOT "$OUTPUT.kernel" &&
echo "FAT formatted boot image partition created"
echo "Copying boot and EFI directory into boot image partition"
mcopy -s -i "$OUTPUT.kernel" "$KERNELDIR/boot" "$KERNELDIR/EFI" ::/ || echo "Copying boot and EFI directory failed!"
dd if="$OUTPUT.kernel" of="$OUTPUT" bs=512 seek="$KERNELOFFSET" conv=notrunc || echo "Merging boot partition to disk image failed!"
rm -f "$OUTPUT.kernel"
