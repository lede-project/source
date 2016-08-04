#
# Copyright (c) 2013 Arduino LLC. All right reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=spacebrew
PKG_VERSION:=2.3.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=git://github.com/julioterra/yunSpacebrew.git
PKG_SOURCE_SUBDIR:=spacebrew-$(PKG_VERSION)
PKG_SOURCE_VERSION:=0299adaca762f75afd4551aa63ff54c703cbd7e6
PKG_SOURCE_PROTO:=git

include $(INCLUDE_DIR)/package.mk

define Package/spacebrew
  SECTION:=net
  CATEGORY:=Network
  TITLE:= A toolkit for creating interactive spaces
  DEPENDS:=+python-mini +python-json
endef

define Package/spacebrew/description
	Flexible routing and re-routing between anything that can speak over Websockets.
endef

define Build/Compile
	# NOOP
endef

define Package/spacebrew/install
	$(INSTALL_DIR) $(1)/usr/lib/python2.7/websocket
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/linino/websocket/* $(1)/usr/lib/python2.7/websocket/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/linino/websocket/websocket.py $(1)/usr/lib/python2.7/websocket/

	$(INSTALL_DIR) $(1)/usr/lib/python2.7/spacebrew
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/linino/spacebrew/* $(1)/usr/lib/python2.7/spacebrew/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/linino/spacebrew/spacebrew.py $(1)/usr/lib/python2.7/spacebrew/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/linino/spacebrew/getprocpid.py $(1)/usr/lib/python2.7/spacebrew/

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/linino/shell/run-spacebrew $(1)/usr/bin
endef

$(eval $(call BuildPackage,spacebrew))

