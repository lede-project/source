#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libwyliodrin
PKG_VERSION:=2.5
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git

PKG_REVISION:=263f7f5f77b0acb1a74c5758bf4713b50ac52c33
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/Wyliodrin/libwyliodrin.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_BUILD_DEPENDS:=+swig/host +node

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libwyliodrin
  TITLE:=Wiliodryn Library
  SECTION:=libs
  DEFAULT:=m
  CATEGORY:=Libraries
  DEPENDS:=+libpthread +libcrypto +libc +libstdcpp +librt +libhiredis +jansson +libevent2
  URL:=https://github.com/Wyliodrin/libwyliodrin.git
endef

define Package/libwyliodrin/description
	Wyliodrin library http://www.wyliodrin.com
endef

define Package/python-libwyliodrin
  TITLE:=Wiliodryn Library (Python Bindings)
  SECTION:=lang
  DEFAULT:=m
  CATEGORY:=Languages
  SUBMENU:=Python
  DEPENDS:=+libpthread +libcrypto +libc +libstdcpp +libwyliodrin +librt +python-base +python-light
  URL:=https://github.com/Wyliodrin/libwyliodrin.git
endef

define Package/python-libwyliodrin/description
	Wyliodrin library (http://www.wyliodrin.com) python bindings
endef

define Package/node-libwyliodrin
  TITLE:=Wiliodryn Library (Node.js bindings)
  SECTION:=lang
  SUBMENU:=Node.js
  DEFAULT:=m
  CATEGORY:=Languages
  DEPENDS:=+libpthread +libcrypto +libc +libstdcpp +libwyliodrin +librt +node
  URL:=https://github.com/Wyliodrin/libwyliodrin.git
endef

define Package/node-libwyliodrin/description
	Wyliodrin library (http://www.wyliodrin.com) Node.js bindings
endef

CMAKE_OPTIONS = \
        -DCMAKE_INSTALL_PREFIX=/usr \
		-DSERVER=ON

define Package/libwyliodrin/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/wylio  $(1)/usr/bin

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libwyliodrin.so.* $(1)/usr/lib/

	# $(INSTALL_DIR) $(1)/etc/wyliodrin
	# $(INSTALL_DATA) $(PKG_BUILD_DIR)/setup/settings_*.json  $(1)/etc/wyliodrin
endef

define Package/python-libwyliodrin/install
	$(INSTALL_DIR) $(1)/usr/lib/python2.7/site-packages
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/python2.7/dist-packages/{*.py,*so}  $(1)/usr/lib/python2.7/site-packages
endef

define Package/node-libwyliodrin/install
	$(INSTALL_DIR) $(1)/usr/lib/node_modules/wyliodrin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/wyliodrin/*  $(1)/usr/lib/node_modules/wyliodrin
endef

$(eval $(call BuildPackage,libwyliodrin))
$(eval $(call BuildPackage,python-libwyliodrin))
$(eval $(call BuildPackage,node-libwyliodrin))
