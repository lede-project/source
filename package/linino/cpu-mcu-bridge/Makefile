#
# Copyright (c) 2013 Arduino LLC. All right reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cpu-mcu-bridge
PKG_VERSION:=1.5.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://downloads.arduino.cc/
PKG_MD5SUM:=11338f65f3ed041abf3f0089bfc6ae50

include $(INCLUDE_DIR)/package.mk

define Package/cpu-mcu-bridge
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=CPU-MCU Bridge
  DEPENDS:=+python-mini
endef

define Package/cpu-mcu-bridge/description
 Bridge lets linux and arduino communicate
endef

define Build/Compile
	# NOOP
endef

define Package/cpu-mcu-bridge/install
	$(INSTALL_DIR) $(1)/usr/lib/python2.7/bridge
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/*.py $(1)/usr/lib/python2.7/bridge

	$(INSTALL_DIR) $(1)/usr/lib/php/bridge
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/php/*.php $(1)/usr/lib/php/bridge
endef

$(eval $(call BuildPackage,cpu-mcu-bridge))
