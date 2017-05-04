#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tree
PKG_RELEASE:=1
PKG_VERSION:=1.7.0
PKG_SOURCE_URL:=ftp://mama.indstate.edu/linux/tree/
PKG_HASH:=6957c20e82561ac4231638996e74f4cfa4e6faabc5a2f511f0b4e3940e8f7b12

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tgz

include $(INCLUDE_DIR)/package.mk

define Package/tree
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=A command list contents of directories in a tree-like format
  DEPENDS:=+libc +libgcc
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/tree/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tree $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,tree))
