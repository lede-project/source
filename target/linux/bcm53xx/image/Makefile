#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Image/Prepare
	rm -f $(KDIR)/fs_mark
	echo -ne '\xde\xad\xc0\xde' > $(KDIR)/fs_mark
	$(call prepare_generic_squashfs,$(KDIR)/fs_mark)

	# For UBI we want only one extra block
	rm -f $(KDIR)/ubi_mark
	echo -ne '\xde\xad\xc0\xde' > $(KDIR)/ubi_mark
endef

define Build/lzma-d16
	$(STAGING_DIR_HOST)/bin/lzma e $@ -d16 $(1) $@.new
	mv $@.new $@
endef

# Similar to Build/tplink-safeloader but uses TRX instead of clean kernel
define Build/bcm53xx-tplink-safeloader
	$(STAGING_DIR_HOST)/bin/trx \
		-o $@.trx \
		-m 33554432 \
		-f $(IMAGE_KERNEL) -a 1024
	$(STAGING_DIR_HOST)/bin/tplink-safeloader \
		-B $(TPLINK_BOARD) \
		-k $@.trx \
		-r $@ \
		-j \
		-o $@.new
	mv $@.new $@
	rm $@.trx
endef

define Build/buffalo-wzr-header
	$(eval product=$(word 1,$(1)))
	$(eval region=$(word 2,$(1)))
	( \
		echo $(product)_$(BUFFALO_TAG_VERSION)_$(BUFFALO_TAG_MINOR)_$(region)_$(BUFFALO_TAG_PLATFORM); \
		echo filelen=$$(stat -c%s $@); \
		cat $@ \
	) > $@.new
	mv $@.new $@
endef

# TRX with only one (kernel) partition
define Build/trx
	$(STAGING_DIR_HOST)/bin/trx \
		-o $@.new \
		-m 33554432 \
		-f $@
	mv $@.new $@
endef

define Build/trx-serial
	$(STAGING_DIR_HOST)/bin/trx \
		-o $@.new \
		-m 33554432 \
		-f $(IMAGE_KERNEL) -a 1024 \
		-f $@ -a 0x10000 -A $(KDIR)/fs_mark
	mv $@.new $@
endef

define Build/trx-nand
	# kernel: always use 4 MiB (-28 B or TRX header) to allow upgrades even
	#	  if it grows up between releases
	# root: UBI with one extra block containing UBI mark to trigger erasing
	#	rest of partition
	$(STAGING_DIR_HOST)/bin/trx \
		-o $@.new \
		-m 33554432 \
		-f $(IMAGE_KERNEL) -a 0x20000 -b 0x400000 \
		-f $@ \
		-A $(KDIR)/ubi_mark -a 0x20000
	mv $@.new $@
endef

define Build/asus-trx
	$(STAGING_DIR_HOST)/bin/asustrx \
		-p $(PRODUCTID) -i $@ -o $@.new
	mv $@.new $@
endef

define Build/seama-nand
	# Seama entity
	$(STAGING_DIR_HOST)/bin/oseama \
		entity $@.entity \
		-m "dev=/dev/mtdblock/7" \
		-m "type=firmware" \
		-f $(IMAGE_KERNEL) \
		-b 0x400000 \
		-f $@ \
		-f $(KDIR)/ubi_mark
	# Seama container
	$(STAGING_DIR_HOST)/bin/seama \
		-s $@ \
		-m "signature=$(SIGNATURE)" \
		-i $@.entity
endef

DEVICE_VARS += PRODUCTID SIGNATURE NETGEAR_BOARD_ID NETGEAR_REGION TPLINK_BOARD
DEVICE_VARS += BUFFALO_TAG_PLATFORM BUFFALO_TAG_VERSION BUFFALO_TAG_MINOR

IEEE8021X := wpad-mini
B43 := $(IEEE8021X) kmod-b43
BRCMFMAC_43602A1 := $(IEEE8021X) kmod-brcmfmac brcmfmac-firmware-43602a1-pcie
BRCMFMAC_4366B1 := $(IEEE8021X) kmod-brcmfmac brcmfmac-firmware-4366b1-pcie
BRCMFMAC_4366C0 := $(IEEE8021X) kmod-brcmfmac
USB2_PACKAGES := kmod-usb-ohci kmod-usb2 kmod-phy-bcm-ns-usb2
USB2_PACKAGES += kmod-usb-ledtrig-usbport
USB3_PACKAGES := $(USB2_PACKAGES) kmod-usb3 kmod-phy-bcm-ns-usb3

define Device/Default
  # .dtb files are prefixed by SoC type, e.g. bcm4708- which is not included in device/image names
  # extract the full dtb name based on the device info
  DEVICE_DTS := $(patsubst %.dtb,%,$(notdir $(wildcard $(if $(IB),$(KDIR),$(DTS_DIR))/*-$(1).dtb)))
  KERNEL := kernel-bin | append-dtb | lzma-d16
  KERNEL_DEPENDS = $$(wildcard $(DTS_DIR)/$$(DEVICE_DTS).dts)
  KERNEL_INITRAMFS_SUFFIX := .trx
  KERNEL_INITRAMFS := kernel-bin | append-dtb | lzma-d16 | trx
  FILESYSTEMS := squashfs
  KERNEL_NAME := zImage
  IMAGE_NAME = $$(IMAGE_PREFIX)-By-ypjalt-$$(REVISION)-$$(1).$$(2)
  IMAGES := trx
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE/trx := append-ubi | trx-nand
endef


# Buffalo devices have TFTP recovery mode which can work nicely with initramfs
# kernels.
# We should have two initramfs images for Buffalo: plain initramfs kernel and
# TRX with initramfs kernel. It's not possible right now so let's just build
# plain initramfs kernel as it may be more useful.


# Linksys devices are disabled due to problem with 2 TRX partitions


define Device/phicomm-k3
  DEVICE_TITLE := PHICOMM K3
  DEVICE_PACKAGES := $(BRCMFMAC_4366C0) $(USB3_PACKAGES)
  IMAGES := trx
endef
TARGET_DEVICES += phicomm-k3


$(eval $(call BuildImage))
