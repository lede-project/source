#
# Copyright (C) 2012 Openwrt-for-Embedded http://code.google.com/p/openwrt-for-embedded/
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=obexftp
PKG_VERSION:=0.23
PKG_RELEASE:=1
#PKG_MD5SUM:=31a50dbbd09c63cbb5399772aaff7936

PKG_SOURCE:=obexftp-$(PKG_VERSION).tar.bz2
#PKG_BUILD_DIR=$(BUILD_DIR)/obexftp-$(PKG_VERSION)
PKG_SOURCE_URL:=@SF/openobex

PKG_FIXUP:=libtool
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=ObexFTP is a library bundling everything needed for OBEX
  DEFAULT:=m
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+openobex
  URL:=http://dev.zuckschwerdt.org/openobex
  MAINTAINER:=Richard Nee <richard.nee.cn@gmail.com>
endef

define Package/$(PKG_NAME)/description
  The overall goal of this project is to make an open source 
  implementation of the Object Exchange (OBEX) protocol. 
  OBEX is a session protocol and can best be described as a binary HTTP protocol.
endef

TARGET_LDFLAGS += -Wl,-rpath-link,$(STAGING_DIR)/usr/lib
TARGET_CFLAGS+=-I$(STAGING_DIR)/usr/include
TARGET_CXXFLAGS+=-I$(STAGING_DIR)/usr/include

define Build/Configure
	$(call Build/Configure/Default, \
		--disable-swig \
		--disable-python \
		--disable-perl \
		--disable-ruby \
		--disable-tcl \
	)
endef


define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/include \
		$(1)/usr/lib \
		$(1)/usr/lib/pkgconfig

	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.{a,so*,la} $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/obexftp.pc $(1)/usr/lib/pkgconfig/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) \
		$(1)/usr/bin \
		$(1)/usr/lib

	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
