#
# Copyright (C) 2012 Openwrt-for-Embedded http://code.google.com/p/openwrt-for-embedded/
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=obexfs
PKG_VERSION:=0.12
PKG_RELEASE:=1
PKG_MD5SUM:=0f505672b025cdb505e215ee707a2e2f

PKG_SOURCE:=obexfs-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/openobex

PKG_FIXUP:=libtool
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk


define Package/$(PKG_NAME)
  TITLE:=(ObexFS)FUSE based filesystem using ObexFTP
  DEFAULT:=m
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+kmod-fuse +libfuse +libusb-compat +fuse-utils +openobex +obexftp
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


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
