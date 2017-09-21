#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lua-base64
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_MAINTAINER:=Dirk Chang <dirk@kooiot.com>
PKG_LICENSE:=MIT

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/toastdriven/lua-base64.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=875217fe43b35ec8b6375897c08c51735db6741a
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/lua-base64
  SUBMENU:=Lua
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Lua Base64
  URL:=http://www.keplerproject.org/base64
  DEPENDS:=+lua
endef

define Package/lua-base64/description
	Another base64 implementation for LUA
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/lua-base64/install
	$(INSTALL_DIR) $(1)/usr/lib/lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/base64.lua $(1)/usr/lib/lua
endef

$(eval $(call BuildPackage,lua-base64))
