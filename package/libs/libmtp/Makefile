#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libmtp
PKG_VERSION:=1.1.15
PKG_RELEASE:=1

PKG_SOURCE_URL:=@SF/${PKG_NAME}
PKG_SOURCE:=${PKG_NAME}-${PKG_VERSION}.tar.gz
PKG_HASH:=d9118944e746c44f5312f0b30ea2f6dd
PKG_CAT:=zcat

PKG_INSTALL:=1

PKG_FIXUP:=patch-libtool

include $(INCLUDE_DIR)/package.mk

define Package/libmtp
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libusb-1.0
  TITLE:=MTP Library
  URL:=https://github.com/libmtp/libmtp
endef

define Package/libmtp/description
libmtp implements an MTP initiator, which means it initiate
MTP sessions with devices. The devices responding are known
as MTP responders. libmtp runs on something with a USB host
controller interface, using libusb to access the host
controller.
endef

CONFIGURE_ARGS += \
	--without-udev

define Build/Configure
  $(call Build/Configure/Default)
endef

define Package/libmtp/install
	$(INSTALL_DIR) $(1)/usr/local/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libmtp*.so* $(1)/usr/local/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/mtp-* $(1)/usr/bin/
endef

define Build/InstallDev
	$(CP) $(PKG_INSTALL_DIR)/* $(1)/
endef

$(eval $(call BuildPackage,libmtp))
