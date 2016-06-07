#
# Copyright (C) 2013-2014 OpenWrt.org
# Copyright (C) 2015      GuoGuo<gch981213@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=agentx1
PKG_VERSION:=f398672
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://bitbucket.org/CrazyBoyFeng/agentx1
PKG_SOURCE_PROTO:=hg
PKG_SOURCE_VERSION:=f3986727185833f2cc11df6072e504c4f8df4153

include $(INCLUDE_DIR)/package.mk

define Package/agentx1
	CATEGORY:=Network
	TITLE:=Ruijie 802.1X authentication proxy
	URL:=https://bitbucket.org/CrazyBoyFeng/agentx1
	MAINTAINER:=CrazyBoyFeng
	DEPENDS:=+libpthread
endef

define Package/agentx1/description
	Ruijie 802.1X authentication proxy.
endef

define Build/Compile
	$(TARGET_CROSS)gcc -lpthread -o $(PKG_BUILD_DIR)/agentx1 $(PKG_BUILD_DIR)/Source/*.c -Wall -Os
endef


define Package/agentx1/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/agentx1 $(1)/usr/bin
endef

$(eval $(call BuildPackage,agentx1))

