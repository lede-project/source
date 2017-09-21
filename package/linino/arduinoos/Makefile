#
# Copyright (c) 2015 Arduino Srl - All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arduinoos
PKG_VERSION:=2.0.3
PKG_REVISION:=1
PKG_SRC_REVISION:=2ec5e2a
PKG_RELEASE:=$(PKG_REVISION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SRC_REVISION).tar.gz
PKG_SOURCE_URL:=http://download.linino.org/sources
PKG_MD5SUM:=806975cb0ec68addbb4d7b218fb80769
PKG_BUILD_DIR:=$(BUILD_DIR)/arduinoos-$(PKG_VERSION)-$(PKG_SRC_REVISION)

include $(INCLUDE_DIR)/package.mk

define Package/arduinoos
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:=The new Linino configuration panel based on Os.js
  DEPENDS:=+luci +luci-lib-json +uhttpd-mod-lua +lua-coxpcall +lua-wsapi-base +lua-wsapi-xavante +lua-base64 +lua-xavante +luafilesystem +iw +shellinabox
endef

define Package/arduinoos/description
	The new Linino configuration panel based on Os.js
endef

define Build/Compile
	# NOOP
endef

define Package/arduinoos/install
	$(INSTALL_DIR) $(1)/
	$(INSTALL_DIR) $(1)/usr/lib/lua
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller/linino

	$(CP) -R $(PKG_BUILD_DIR)/osjs $(1)/

	$(CP) files/index.lua $(1)/usr/lib/lua/luci/controller/linino
	$(CP) $(PKG_BUILD_DIR)/lua/osjs.lua $(1)/usr/lib/lua

	$(INSTALL_BIN) files/osjs-init.sh $(1)/etc/init.d/osjs-init
endef

define Package/arduinoos/postinst
#!/bin/sh
if [ -e /etc/config/uhttpd ]
then
	# make the webserver ponting to /osjs instead of /www
	sed -i -e 's/www/osjs\/dist/g' /etc/config/uhttpd /etc/config/uhttpd

	# Linking LUci
	mkdir -p /osjs/dist/luci
	ln -sf /www/index.html /osjs/dist/luci/index.html
	ln -sf /www/luci-static /osjs/dist/luci-static
	ln -sf /www/cgi-bin/luci /osjs/dist/cgi-bin/luci

	# Restarting uHttp server
	# /etc/init.d/uhttpd restart
fi
exit 0
endef

define Package/arduinoos/postrm
#!/bin/sh
if [ -e /etc/config/uhttpd ] && [ -d /osjs ]
then
	# restoring orinal entry to /www
    sed -i -e 's/osjs\/dist/www/g' /etc/config/uhttpd /etc/config/uhttpd

	# cleaning ArduinoOs files
	# rm -rf /usr/lib/lua/osjs
	rm -rf /osjs

	# Restarting uHttp server
	/etc/init.d/uhttpd restart
fi
exit 0
endef

$(eval $(call BuildPackage,arduinoos))
