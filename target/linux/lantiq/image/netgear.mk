#
# Copyright (C) 2017 LEDE & Edward O'Callaghan
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

DEVICE_VARS += NETGEAR_BOARD_ID NETGEAR_REGION NETGEAR_HW_ID

define Device/lantiqNetgear
  KERNEL := kernel-bin | append-dtb | lzma | uImage lzma | pad-offset 4 0
  IMAGES := sysupgrade.bin factory.img
  IMAGE/default := append-kernel | append-rootfs | pad-rootfs | append-metadata
  IMAGE/sysupgrade.bin := $$(IMAGE/default) | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.img := $$(IMAGE/default) | netgear-dni | check-size $$$$(IMAGE_SIZE)
endef

define Device/DM200
  $(Device/lantiqNetgear)
  DEVICE_TITLE := Netgear DM200
  DEVICE_PACKAGES := kmod-spi-lantiq-ssc

  NETGEAR_BOARD_ID := DM200
  NETGEAR_HW_ID := 29765233+8+0+64+0+0
  IMAGE_SIZE := 8192k
endef
TARGET_DEVICES += DM200
