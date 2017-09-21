#
# Copyright (C) Arduino Srl 2015
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=nfcst-utils
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

# PKG_FILE:="$(STAGING_DIR)/pkginfo/libc.provides"
# PKG_CAT=`cat $(PKG_FILE) | grep libc.so.6`

include $(INCLUDE_DIR)/package.mk

define Package/nfcst-utils
	DEPENDS:=+libc +libstdcpp
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Utility and Daemon for ST NFC driver
	URL:=http://www.st.com
endef

define Package/nfcst-utils/description
	Utility and Daemon for ST NFC driver
endef

define Build/Prepare
	# (if [ ! $(PKG_CAT) ];then echo "libc.so.6" >> $(PKG_FILE); fi)
	cp src/*.c $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(PKG_BUILD_DIR)/stnfcd.c -o $(PKG_BUILD_DIR)/stnfcd
	$(TARGET_CC) $(PKG_BUILD_DIR)/stnfcwake_test.c -o $(PKG_BUILD_DIR)/stnfcwake_test
endef

define Package/nfcst-utils/install
	$(INSTALL_DIR) $(1)/usr/bin/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/stnfcd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/stnfcwake_test $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/stnfcd-openwrt $(1)/etc/init.d
endef

$(eval $(call BuildPackage,nfcst-utils))
