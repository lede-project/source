#
# Copyright (C) 2016 LEDE
# Copyright (C) 2016 Adrian Panella
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=simple-cksum
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/simple-cksum
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Utility to calculate simple checksum
  MAINTAINER:=Adrian Panella <ianchi74@outlook.com>
endef

define Package/simple-cksum/description
Utility to calculate simple checksum
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Package/simple-cksum/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/scksum $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,simple-cksum))
