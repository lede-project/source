#
# Copyright (C) 2017 Marty E. Plummer
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Device/Default
  PROFILES := Default
  DEVICE_DTS :=
  KERNEL_ENTRY := 0x80008000
  KERNEL_LOADADDR := 0x80008000
  KERNEL_NAME := zImage
  KERNEL_INITRAMFS := kernel-bin | append-dtb | uImage none
  KERNEL := kernel-bin | append-dtb | uImage none
  IMAGES := 
endef

define Build/dtb
	$(call Image/BuildDTB,$(DTS_DIR)/$(DEVICE_DTS).dts,$@.dtb,,)
endef

include cortex-a7.mk

$(eval $(call BuildImage))
