#
# Copyright (C) 2012-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
SUBTARGET:=armada_37x_xp
BOARDNAME:=Marvell Armada 37x/XP
FEATURES:=fpu usb pci pcie gpio nand squashfs
CPU_TYPE:=cortex-a9
CPU_SUBTYPE:=vfpv3

define Target/Description
	Build firmware for Marvell Armada 37x and XP SoC.
endef
