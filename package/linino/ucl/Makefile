# 
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ucl
PKG_VERSION:=1.03
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.oberhumer.com/opensource/ucl/download/
PKG_MD5SUM:=852bd691d8abc75b52053465846fba34

PKG_FIXUP:=libtool
PKG_REMOVE_FILES:=aclocal.m4 acconfig/ltmain.sh

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libucl
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Portable lossless data compression library
  URL:=http://www.oberhumer.com/opensource/ucl/
endef

define Package/libucl/description
UCL is a portable lossless data compression library written in ANSI C. UCL
implements a number of compression algorithms that achieve an excellent
compression ratio while allowing *very* fast decompression. Decompression
requires no additional memory.
endef

CONFIGURE_ARGS += \
	--enable-static \
	--enable-shared \
	--disable-asm

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/ucl
	$(CP) $(PKG_INSTALL_DIR)/usr/include/ucl/*.h $(1)/usr/include/ucl/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libucl.{a,so*} $(1)/usr/lib/
endef

define Package/libucl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libucl.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libucl))

