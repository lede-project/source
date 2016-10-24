include $(TOPDIR)/rules.mk

PKG_NAME:=shadowsocks-libev
PKG_VERSION:=2.3.0-096e1eca5d
PKG_RELEASE:=1
PKG_REV:=096e1eca5d0d23718b7b0e0a182d178d8cabfdaa

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/shadowsocks/shadowsocks-libev.git
PKG_SOURCE_VERSION:=$(PKG_REV)


PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/shadowsocks-libev/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Lightweight Secured Socks5 Proxy
  URL:=https://github.com/madeye/shadowsocks-libev
endef

define Package/shadowsocks-libev
  $(call Package/shadowsocks-libev/Default)
  TITLE+= (OpenSSL)
  VARIANT:=openssl
  DEPENDS:=+libopenssl
endef

define Package/shadowsocks-libev-polarssl
  $(call Package/shadowsocks-libev/Default)
  TITLE+= (PolarSSL)
  VARIANT:=polarssl
  DEPENDS:=+libpolarssl
endef

define Package/shadowsocks-libev/description
Shadowsocks-libev is a lightweight secured scoks5 proxy for embedded devices and low end boxes.
endef

Package/shadowsocks-libev-polarssl/description=$(Package/shadowsocks-libev/description)

define Package/shadowsocks-libev/conffiles
/etc/shadowsocks.json
endef

ifeq ($(BUILD_VARIANT),polarssl)
	CONFIGURE_ARGS += --with-crypto-library=polarssl
endif

define Package/shadowsocks-libev/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config

	$(INSTALL_DATA) ./files/shadowsocks.config $(1)/etc/config/shadowsocks
	$(INSTALL_BIN) ./files/shadowsocks.init $(1)/etc/init.d/shadowsocks
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ss-local $(1)/usr/bin
endef

Package/shadowsocks-libev-polarssl/install=$(Package/shadowsocks-libev/install)

$(eval $(call BuildPackage,shadowsocks-libev))
$(eval $(call BuildPackage,shadowsocks-libev-polarssl))
