#
# Copyright (C) 2010-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libc-ver-compat
PKG_VERSION:=1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/libc-ver-compat
  CATEGORY:=Base system
  TITLE:=Some symlinks
  DEPENDS:= +libc +libpthread +librt
  MAINTAINER:=GuoGuo
endef

define Package/libc-ver-compat/description
Some symlinks to let binary programs work with new uClibc
endef

define Build/Compile
endef

define Package/libc-ver-compat/install
	$(INSTALL_DIR) $(1)/lib
	$(LN) libdl.so.1 $(1)/lib/libdl.so.0
	$(LN) librt.so.1 $(1)/lib/librt.so.0
	$(LN) libcrypt.so.1 $(1)/lib/libcrypt.so.0
	$(LN) libutil.so.1 $(1)/lib/libutil.so.0
	$(LN) libc.so.1 $(1)/lib/libc.so.0
	$(LN) libm.so.1 $(1)/lib/libm.so.0
	$(LN) libpthread.so.1 $(1)/lib/libpthread.so.0
endef

$(eval $(call BuildPackage,libc-ver-compat))
