#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=rhash
PKG_VERSION:=1.3.4
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git

PKG_REVISION:=v1.3.4

PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/rhash/RHash.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_REVISION).tar.gz

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING

# PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/rhash
  TITLE:=Rhash toolkit
  SECTION:=utils
  DEFAULT:=m
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +libc +libstdcpp +librhash
  URL:=https://github.com/rhash/RHash.git
endef

define Package/librhash
  TITLE:=RHash library.
  SECTION:=libs
  DEFAULT:=m
  CATEGORY:=Libraries
  DEPENDS:=+libpthread +libc +libstdcpp
  URL:=https://github.com/rhash/RHash.git
endef

define Package/rhash/conffiles
/etc/rhashrc
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/librhash/*.h $(1)/usr/include

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/librhash/librhash{*.a,*.so*} $(1)/usr/lib/
endef

define Build/configure
	# NOOP
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC=$(REAL_GNU_TARGET_NAME)-gcc CXX=$(REAL_GNU_TARGET_NAME)-g++ \
		DESTDIR="$(PKG_INSTALL_DIR)" PREFIX="/usr" \
		all build-shared install
endef

define Package/rhash/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/rhashrc $(1)/etc

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

define Package/librhash/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/librhash/librhash{*.a,*.so*} $(1)/usr/lib/
endef

$(eval $(call BuildPackage,rhash))
$(eval $(call BuildPackage,librhash))
