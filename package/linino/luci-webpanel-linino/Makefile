#
# Copyright (c) 2014 Dog Hungter AG - All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-webpanel-linino
PKG_VERSION:=1.0.3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=http://github.com/linino/luci-webpanel-linino/archive
PKG_SOURCE_URL:=http://download.linino.org/pkg-src
PKG_MD5SUM:=751cbfb8575255fe20611d6b058ac079

include $(INCLUDE_DIR)/package.mk

define Package/luci-webpanel-linino
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:= simplified wifi configuration panel
  DEPENDS:=+luci +luci-lib-json +uhttpd-mod-lua
endef

define Package/luci-webpanel-linino/description
	simplified wifi configuration panel
endef

define Build/Compile
	# NOOP
endef

define Package/luci-webpanel-linino/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller/arduino
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/arduino
	$(INSTALL_DIR) $(1)/www/luci-static/resources/arduino

	find $(PKG_BUILD_DIR)/usr/ -name '*.lua' -exec luac -s -o {} {} \;
	$(CP) $(PKG_BUILD_DIR)/usr/* $(1)/usr/
	$(CP) $(PKG_BUILD_DIR)/www/* $(1)/www/
endef

$(eval $(call BuildPackage,luci-webpanel-linino))

