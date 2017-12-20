#
# Copyright (C) 2017 Tomasz Maciej Nowak <tomek_n@o2.pl>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

SIGNATURE := $(shell perl -e 'printf("%08x", rand(0xFFFFFFFF))')

define Build/tegra-sdcard
	rm -fR $@.boot
	mkdir -p $@.boot
	$(CP) $(KDIR)/$(KERNEL_NAME) $@.boot
	$(CP) $(DTS_DIR)/$(DEVICE_DTS).dtb $@.boot
	sed -e 's#@ROOT@#$(SIGNATURE)#g' \
		./$(DEVICE_NAME)-bootscript > bootscript.new
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 \
		-n '$(DEVICE_TITLE) bootscript' \
		-d ./bootscript.new \
		$@.boot/boot.scr
	rm -f ./bootscript.new

	PADDING="$(CONFIG_TARGET_IMAGES_PAD)" SIGNATURE="$(SIGNATURE)" $(SCRIPT_DIR)/gen_image_generic.sh \
		$@ \
		$(CONFIG_TEGRA_SD_BOOT_PARTSIZE) $@.boot \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) $(IMAGE_ROOTFS) \
		256
	rm -fR $@.boot
endef

define Device/Default
  PROFILES := Default
  KERNEL_NAME := zImage
  KERNEL := kernel-bin
  IMAGES := sdcard.img.gz
  IMAGE/sdcard.img.gz := tegra-sdcard | append-metadata | gzip
endef

define Device/trimslice
  DEVICE_TITLE := CompuLab TrimSlice
  DEVICE_DTS := tegra20-trimslice
  DEVICE_PACKAGES := \
	kmod-r8169 \
	kmod-rtc-em3027 \
	kmod-rt2800-usb \
	kmod-usb-storage
  SUPPORTED_DEVICES := compulab,trimslice
endef

TARGET_DEVICES += trimslice

$(eval $(call BuildImage))
