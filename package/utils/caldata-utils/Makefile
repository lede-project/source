#
# Copyright (C) 2006-2010 OpenWrt.org
# Copyright (C) 2016 Adrian Panella
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=caldata-utils
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS:=USE_UCLIBC:argp-standalone USE_MUSL:argp-standalone

include $(INCLUDE_DIR)/package.mk

ifdef CONFIG_USE_UCLIBC
	LIBARGP="-largp"
endif

ifdef CONFIG_USE_MUSL
	LIBARGP="-largp"
endif


define Package/caldata-utils
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Utility to manipulate calibration data for ath10k
  MAINTAINER:=Adrian Panella <ianchi74@outlook.com>
endef

define Package/caldata-utils/description
 This package contains an utility to manipulate calibration data for ath10k drivers.
 It enables to extract from MTD partition or file and to patch MAC address fixing the checksum.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS) $(LIBARGP)"
endef

define Package/caldata-utils/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/caldata $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,caldata-utils))
