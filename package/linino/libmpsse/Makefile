#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libmpsse
PKG_VERSION:=1.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://libmpsse.googlecode.com/files/
PKG_MD5SUM:=628eb9a7d7250070f5841ed44a248f84

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/libmpsse/Default
  TITLE:=Interfacing with SPI/I2C devices (FT-2232 family)
  URL:=https://code.google.com/p/libmpsse/wiki/README
endef

define Package/libmpsse
$(call Package/libmpsse/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libftdi1
endef

define Package/libmpsse/description
  Libmpsse is a library for interfacing with SPI/I2C devices
  via FTDI's FT-2232 family of USB chips.
  Based around the libftdi library,
  it is written in C and includes a Python wrapper courtesy of swig.
endef

TARGET_CFLAGS += $(FPIC) -I$(STAGING_DIR)/usr/include/libftdi1
CONFIGURE_ARGS += --disable-python

define Build/Prepare
	$(call Build/Prepare/Default)
	cd $(PKG_BUILD_DIR); cp -r src/* ./;
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/mpsse.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libmpsse.{a,so} $(1)/usr/lib/
endef

define Package/libmpsse/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libmpsse.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libmpsse))
