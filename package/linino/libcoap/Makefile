#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libcoap
PKG_RELEASE:=2

PKG_SOURCE_URL:=@SF/libcoap
PKG_VERSION:=4.1.1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_MD5SUM:=2ab6daf1f187f02d25b77c39c2ecc56b

PKG_MAINTAINER:=Anton Glukhov <anton.a.glukhov@gmail.com>
PKG_LICENSE:=GPL-2.0+ BSD-2-Clause
PKG_LICENSE_FILE:=COPYING LICENSE.GPL LICENSE.BSD

PKG_FIXUP:=autoreconf
# PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libcoap
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=CoAP (RFC 7252) library
  URL:=http://libcoap.net/
endef

define Package/libcoap/description
  Constrained Application Protocol (RFC 7252) library
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--disable-examples \
	--disable-documentation \
	--with-shared

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/coap.h $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libcoap{.a,.so} $(1)/usr/lib/
endef

define Package/libcoap/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/examples/{coap-client,coap-server} $(1)/usr/bin	

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libcoap{.a,.so} $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libcoap))
