#
# Copyright (C) 2017 Marty E. Plummer
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=hisilicon
BOARDNAME:=Hisilicon DVR boards
FEATURES:=fpu rtc squashfs
SUBTARGETS:=cortexa7
MAINTAINER:=Marty E. Plummer <hanetzer@startmail.com>

KERNEL_PATCHVER=4.9
KERNELNAME:=zImage dtbs

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
