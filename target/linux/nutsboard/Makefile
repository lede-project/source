#
# Copyright (C) 2017 NutsBoard.org
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=nutsboard
BOARDNAME:=NutsBoard
SUBTARGETS:=almond
FEATURES:=gpio
MAINTAINER:=Wig Cheng <onlywig@gmail.com>

KERNEL_PATCHVER:=4.4


define Target/Description
	Build firmware images for NutsBoard based boards.
endef

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
