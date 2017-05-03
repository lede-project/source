#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

KERNEL_LOADADDR := 0x20008000

ifeq ($(SUBTARGET),legacy)
include ./legacy.mk
endif
ifeq ($(SUBTARGET),sama5d3)
include ./sama5d3.mk
endif

define Device/Default
  PROFILES := Default
  FILESYSTEMS := squashfs ubifs
  DEVICE_DTS := $(1)
  KERNEL_NAME := zImage
  KERNEL := kernel-bin | append-dtb | lzma | uImage lzma
  KERNEL_SIZE :=
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  SUBPAGESIZE := 512
  MKUBIFS_OPTS := -m $$(PAGESIZE) -e 126KiB -c 2048
  UBINIZE_OPTS := -E 5
  IMAGES :=
  IMAGE/factory.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-ubi
endef

$(eval $(call BuildImage))
