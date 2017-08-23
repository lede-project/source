#
# Copyright (C) 2014-2017 Jian Chang <aa65535@live.com>
#
# This is free software, licensed under the GNU General Public License v3.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=shadowsocks
PKG_VERSION:=3.0.8
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/shadowsocks/shadowsocks-libev.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)
PKG_SOURCE_VERSION:=feeaff85e156e09f1636ed3a47ac9375c81ceb87
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.xz

PKG_LICENSE:=GPLv3
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Jian Chang <aa65535@live.com>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)/$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/shadowsocks/Default
	SECTION:=net
	CATEGORY:=Network
	TITLE:=Lightweight Secured Socks5 Proxy
	URL:=https://github.com/shadowsocks/shadowsocks-libev
	DEPENDS:=+libev +libudns +libpcre +libpthread +libsodium +libmbedtls
endef

Package/shadowsocks = $(Package/shadowsocks/Default)
Package/shadowsocks-server = $(Package/shadowsocks/Default)

define Package/shadowsocks/description
Shadowsocks is a lightweight secured socks5 proxy for embedded devices and low end boxes.
endef

Package/shadowsocks-server/description = $(Package/shadowsocks/description)

CONFIGURE_ARGS += --disable-ssp --disable-documentation --disable-assert

define Package/shadowsocks/preinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
	if [ -f "/etc/init.d/shadowsocks-libev" ]; then
		echo stopping shadowsocks-libev...
		/etc/init.d/shadowsocks-libev stop
		sslibevid=$$(pidof ss-redir)
		if [ -n "$$sslibevid" ]; then
			echo stopping shadowsocks-libev...
			/etc/init.d/shadowsocks-libev stop
		fi
		echo shadowsocks-libev disabled...
		/etc/init.d/shadowsocks-libev disable
	fi
fi
exit 0
endef

define Package/shadowsocks/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
	if [ -f /etc/uci-defaults/96-shadowsocks ]; then
		( . /etc/uci-defaults/96-shadowsocks ) && rm -f /etc/uci-defaults/96-shadowsocks
	fi
	rm -rf /tmp/luci-indexcache /tmp/luci-modulecache
fi
exit 0
endef

define Package/shadowsocks/conffiles
/etc/config/shadowsocks
endef

define Package/shadowsocks/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ss-{local,redir,tunnel} $(1)/usr/bin
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/* $(1)/etc/init.d/
	
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/* $(1)/etc/config/	
	
	$(INSTALL_DIR) $(1)/usr/share/shadowsocks
	$(INSTALL_BIN) ./files/usr/share/shadowsocks/firewall.include $(1)/usr/share/shadowsocks/
	$(INSTALL_BIN) ./files/usr/share/shadowsocks/ssruleupdate $(1)/usr/share/shadowsocks/
	$(INSTALL_BIN) ./files/usr/share/shadowsocks/onlineconfig $(1)/usr/share/shadowsocks/
	
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/etc/uci-defaults/96-shadowsocks $(1)/etc/uci-defaults/96-shadowsocks
	
endef

define Package/shadowsocks-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ss-server $(1)/usr/bin
endef

$(eval $(call BuildPackage,shadowsocks))
$(eval $(call BuildPackage,shadowsocks-server))
