include $(TOPDIR)/rules.mk

PKG_NAME:=dns2socks
PKG_VERSION:=1.8
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

PKG_UNPACK=mkdir -p $(PKG_BUILD_DIR); $(CP) ./src/* $(PKG_BUILD_DIR)/

define Package/dns2socks
  CATEGORY:=Network
  TITLE:=dns2socks
  DEPENDS:=+libpthread
endef

define Package/dns2socks/description
DNS2SOCKS is a command line utility running to forward DNS requests to a DNS server via a SOCKS tunnel.
endef

define Build/Compile
	$(TARGET_CROSS)gcc -pthread -Wall -Os -o $(PKG_BUILD_DIR)/dns2socks $(PKG_BUILD_DIR)/DNS2SOCKS.c
endef

define Package/dns2socks/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dns2socks $(1)/usr/bin
endef


$(eval $(call BuildPackage,dns2socks))
