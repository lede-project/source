#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ssocks
PKG_VERSION:=0.0.14
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/ssocks
PKG_MD5SUM:=db2452680c3d953631299e331daf49ef

PKG_MAINTAINER:=GuoGuo <gch981213@gmail.com>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/ssocks
  SUBMENU:=Web Servers/Proxies
  SECTION:=net
  CATEGORY:=Network
  TITLE:=A Socks5 Proxy server
  URL:=http://ssocks.sourceforge.net
endef

define Package/ssocks/description
Just a Socks5 Proxy server......
endef

define Package/ssocks/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/{ssocksd,nsocks,ssocks,rssocks,rcsocks} $(1)/usr/bin/
endef


$(eval $(call BuildPackage,ssocks))
