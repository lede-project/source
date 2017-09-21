#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=neardal
PKG_VERSION:=0.14
PKG_RELEASE:=6

PKG_FIXUP:=libtool autoreconf
PKG_SOURCE_PROTO:=git

PKG_REVISION:=5b1c8b5c2c45c10f11cee12fbcb397f8953850d7
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/connectivity/neardal.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_REVISION).tar.gz

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Nicolas Thill <nico@openwrt.org>

PKG_BUILD_DEPENDS:=libedit

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/neardal
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Exchange datas with NFC daemon (Neard)
  DEPENDS:=+libc +libstdcpp +dbus +libedit +glib2 +neard +libncurses
  URL:=https://01.org/linux-nfc
endef

define Package/neardal/description
	Provides a simple C APIs to exchange datas with the NFC daemon (Neard) present on the system.
endef

define Package/neardal/conffiles
/etc/dbus-1/system.d/org.neardal.conf
endef

TARGET_LDFLAGS += -lncurses -ledit

CONFIGURE_ARGS += \
	--disable-dependency-tracking \
	--prefix=/usr \
	--enable-debug \
	--enable-trace \
	--disable-c99

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/neardal
	$(CP) $(PKG_INSTALL_DIR)/usr/include/neardal/*.h $(1)/usr/include/neardal/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libneardal.{a,so*} $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/*.pc $(1)/usr/lib/pkgconfig
endef

define Package/neardal/install
	$(INSTALL_DIR) $(1)/etc/dbus-1/system.d/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/etc/dbus-1/system.d/org.neardal.conf $(1)/etc/dbus-1/system.d/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libneardal.{a,so*} $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/ncl $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/demo/.libs/demo $(1)/usr/bin/ncl-demo
endef

$(eval $(call BuildPackage,neardal))
