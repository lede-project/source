#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=corkscrew
PKG_VERSION:=2.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.agroman.net/corkscrew/
PKG_MD5SUM:=35df77e7f0e59c0ec4f80313be52c10a

include $(INCLUDE_DIR)/package.mk

define Package/corkscrew
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=SSH
  TITLE:=Tunneling SSH through HTTP proxies.
  URL:=http://www.agroman.net/corkscrew/
endef

define Build/Configure
  $(call Build/Configure/Default,)
endef

define Package/corkscrew/install
        $(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/corkscrew $(1)/usr/bin/
endef

$(eval $(call BuildPackage,corkscrew))
