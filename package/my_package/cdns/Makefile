#
# Copyright (C) 2014 OpenWrt-dist
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk


PKG_NAME:=cdns
PKG_VERSION:=20170609
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/semigodking/cdns
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=6ddb888e75934cff6fe5d038a7a8241f04437d5f
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MAINTAINER:=semigodking  <semigodking@gmail.com>
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)/$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk

define Package/cdns/Default
	SECTION:=net
	CATEGORY:=Utilities
	SUBMENU:=DNS
	TITLE:=CureDNS is an experimental tool to cure your poisoned DNS.
	URL:=https://github.com/semigodking/cdns
	DEPENDS:=+libevent2
endef
Package/cdns = $(call Package/cdns/Default)

define Package/cdns-pandorabox
	$(call Package/cdns/Default)
	TITLE:=PandoraBox SDK please compile this first
	DEPENDS+= +argp-standalone
endef

define Package/cdns/description
cdns is an experimental tool to cure your poisoned DNS. The benifits are:
1,No VPN required
2,You can use many foreign DNS servers as upstream server.
3,You can get really good experience if you use fast foreign DNS server as up stream server.
endef

define Package/cdns-pandorabox/description
when you use PandoraBox SDK please compile this package first,then compile the cdns in case to compile the dependence of argp. And you should install the 'cdns' not 'cdns-pandorabox'
endef


define Package/cdns/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cdns $(1)/usr/bin/cdns
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) ./files/etc/config.conf.example $(1)/etc/cdns.json
	
endef

Package/cdns-pandorabox/install = $(call Package/cdns/install,$(1))


$(eval $(call BuildPackage,cdns))
$(eval $(call BuildPackage,cdns-pandorabox))
