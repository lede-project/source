#
# Copyright (C) 2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libassuan
PKG_VERSION:=2.2.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.gnupg.org/gcrypt/libassuan/
PKG_MD5SUM:=a104faed3e97b9c302c5d67cc22b1d60

PKG_FIXUP:=libtool
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libassuan
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=GnuPG IPC library
  URL:=http://www.gnupg.org/
  DEPENDS:=+libgpg-error
endef

define Package/libassuan/description
 The Libassuan package contains an IPC library used by some of the other
 GnuPG related packages. Libassuan's primary use is to allow a client to
 interact with a non-persistent server. Libassuan is not, however,
 limited to use with GnuPG servers and clients. It was designed to be
 flexible enough to meet the demands of many transaction based
 environments with non-persistent servers.
endef

CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static \
	--with-gpg-error-prefix="$(STAGING_DIR)/usr" \
	--without-pth-test \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/assuan.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libassuan.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/share/aclocal
	$(CP) $(PKG_INSTALL_DIR)/usr/share/aclocal/libassuan.m4 $(1)/usr/share/aclocal/
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/libassuan-config $(1)/usr/bin/
	$(INSTALL_DIR) $(2)/bin
	$(LN) -nsf ../../usr/bin/libassuan-config $(2)/bin/
endef

define Package/libassuan/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libassuan*.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libassuan))
