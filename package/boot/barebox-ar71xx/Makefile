#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_VERSION:=2017.11.0
PKG_RELEASE:=1

PKG_HASH:=7dff97dd94807fd7c0012fd9ddb8bc799a220e2b65e46dcc56724105a56feecc

include $(INCLUDE_DIR)/barebox.mk
include $(INCLUDE_DIR)/package.mk

define Barebox/Default
  BUILD_TARGET:=ar71xx
  BUILD_SUBTARGET:=generic
endef

define Barebox/tplink-mr3020
  BUILD_DEVICES:=tl-mr3020-v1
endef

define Barebox/tplink-wdr4300
  BUILD_DEVICES:=tl-wdr4300-v1
endef

BAREBOX_TARGETS:=tplink-mr3020 tplink-wdr4300

$(eval $(call BuildPackage/Barebox))
