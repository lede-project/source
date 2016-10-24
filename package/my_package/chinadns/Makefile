#
# Copyright (C) 2015 OpenWrt-dist
# Copyright (C) 2015 Jian Chang <aa65535@live.com>
#
# This is free software, licensed under the GNU General Public License v3.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ChinaDNS
PKG_VERSION:=1.3.2
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/aa65535/openwrt-chinadns/releases/download/v$(PKG_VERSION)
PKG_MD5SUM:=c529ac231aed4e5874251639f77e92de

PKG_LICENSE:=GPLv3
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=clowwindy <clowwindy42@gmail.com>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)/$(PKG_NAME)-$(PKG_VERSION)

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/ChinaDNS
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ChinaDNS
	URL:=https://github.com/clowwindy/ChinaDNS
	DEPENDS:=+ipip-chnroute
endef

define Package/ChinaDNS/description
A DNS forwarder that ignores incorrect(you knew it) responses.
endef

define Package/ChinaDNS/conffiles
/etc/config/chinadns
/etc/chinadns_chnroute.txt
endef

define Package/ChinaDNS/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/chinadns.init $(1)/etc/init.d/chinadns
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/chinadns.config $(1)/etc/config/chinadns
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/chinadns $(1)/usr/bin
endef

$(eval $(call BuildPackage,ChinaDNS))
