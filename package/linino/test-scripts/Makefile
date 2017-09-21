#
# Copyright (c) 2013 Arduino LLC. All right reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=test-scripts
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/test-scripts
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Test scripts
endef

define Build/Compile
	# NOOP
endef

define Package/test-scripts/install
	$(INSTALL_DIR) $(1)/etc/init.d

	$(INSTALL_BIN) ./files/testdog $(1)/etc/init.d
endef

$(eval $(call BuildPackage,test-scripts))

