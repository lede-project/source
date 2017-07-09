#
# Copyright (C) 2012-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

BOARD:=mvebu
BOARDNAME:=Marvell Armada
FEATURES:=fpu usb pci pcie gpio nand squashfs
SUBTARGETS:=armada_37x_xp armada_38x
MAINTAINER:=Imre Kaloz <kaloz@openwrt.org>

KERNEL_PATCHVER:=4.9

define Target/Description
	Build firmware images for Marvell ARM SoC
endef

include $(INCLUDE_DIR)/target.mk

KERNELNAME:=zImage dtbs

DEFAULT_PACKAGES += uboot-envtools kmod-gpio-button-hotplug

$(eval $(call BuildTarget))
