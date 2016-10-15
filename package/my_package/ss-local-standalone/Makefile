include $(TOPDIR)/rules.mk

PKG_NAME:=ss-local-standalone
PKG_VERSION:=v2.5.3
PKG_RELEASE:=1
PKG_REV:=v2.5.3

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

CONFIGURE_ARGS += --disable-ssp --disable-documentation

define Package/ss-local-standalone/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Lightweight Secured Socks5 Proxy
  URL:=https://github.com/shadowsocks/shadowsocks-libev
  DEPENDS:=+zlib +libpcre
endef

define Package/ss-local-standalone
  $(call Package/ss-local-standalone/Default)
  TITLE+= (OpenSSL)
  VARIANT:=openssl
  DEPENDS+=+libopenssl
endef

define Package/ss-local-standalone-polarssl
  $(call Package/ss-local-standalone/Default)
  TITLE+= (PolarSSL)
  VARIANT:=polarssl
  DEPENDS+=+libpolarssl
endef

define Package/ss-local-standalone/description
Shadowsocks-libev is a lightweight secured scoks5 proxy for embedded devices and low end boxes.
endef

Package/ss-local-standalone-polarssl/description=$(Package/ss-local-standalone/description)

ifeq ($(BUILD_VARIANT),polarssl)
	CONFIGURE_ARGS += --with-crypto-library=polarssl
endif

define Package/ss-local-standalone/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config

	$(INSTALL_DATA) ./files/shadowsocks.config $(1)/etc/config/shadowsocks
	$(INSTALL_BIN) ./files/shadowsocks.init $(1)/etc/init.d/shadowsocks
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/ss-local $(1)/usr/bin
endef

Package/ss-local-standalone-polarssl/install=$(Package/ss-local-standalone/install)

$(eval $(call BuildPackage,ss-local-standalone))
$(eval $(call BuildPackage,ss-local-standalone-polarssl))
