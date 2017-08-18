#
# Copyright (C) 2012-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
SUBTARGET:=armada_38x
BOARDNAME:=Marvell Armada 38x
FEATURES:=fpu usb pci pcie gpio nand squashfs
CPU_TYPE:=cortex-a9
CPU_SUBTYPE:=neon

define Target/Description
	Build firmware for Marvell Armada 38x.
endef


