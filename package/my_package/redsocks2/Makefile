#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=redsocks2
PKG_VERSION:=58ab445
PKG_RELEASE:=1
PKG_REV:=58ab445f21e439e11f8eec044e6d33cbc5de8754

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/semigodking/redsocks.git
PKG_SOURCE_VERSION:=$(PKG_REV)

include $(INCLUDE_DIR)/package.mk

define Package/redsocks2
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Web Servers/Proxies
  DEPENDS:=+libevent2 +libopenssl
  TITLE:=Redirect any TCP connection to a SOCKS or HTTPS proxy server
endef

define Package/redsocks2/description
  This is a modified version of original redsocks.
  The name is changed to be REDSOCKS2 since this release to distinguish with original redsocks. 
  This variant is useful for anti-GFW (Great Fire Wall).
endef

define Package/redsocks2/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/redsocks2 $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,redsocks2))
