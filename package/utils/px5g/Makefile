#
# Copyright (C) 2010-2015 Jo-Philipp Wich <jo@mein.io>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=px5g
PKG_RELEASE:=4
PKG_LICENSE:=LGPL-2.1
PKG_BUILD_DIR:=$(BUILD_DIR)/px5g-$(BUILD_VARIANT)

PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/px5g-polarssl
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=X.509 certificate generator (using PolarSSL)
  MAINTAINER:=Jo-Philipp Wich <jo@mein.io>
  DEPENDS:=+libpolarssl
  PROVIDES:=px5g
  VARIANT:=polarssl
endef

define Package/px5g-mbedtls
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=X.509 certificate generator (using (new) mbedtls)
  MAINTAINER:=Jo-Philipp Wich <jo@mein.io>
  DEPENDS:=+libmbedtls
  PROVIDES:=px5g
  VARIANT:=mbedtls
endef

define Package/px5g-polarssl/description
 Px5g is a tiny standalone X.509 certificate generator.
 It suitable to create key files and certificates in DER
 and PEM format for use with stunnel, uhttpd and others.
endef

Package/px5g-mbedtls/description=$(Package/px5g-polarssl/description)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

ifeq ($(BUILD_VARIANT),mbedtls)
TARGET_CFLAGS += -DMBEDTLS
endif

ifeq ($(BUILD_VARIANT),mbedtls)
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/px5g px5g.c -lmbedtls -lmbedx509 -lmbedcrypto
endef
else
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/px5g px5g.c -lpolarssl
endef
endif

define Package/px5g-polarssl/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/px5g $(1)/usr/sbin/px5g
endef

Package/px5g-mbedtls/install=$(Package/px5g-polarssl/install)

$(eval $(call BuildPackage,px5g-polarssl))
$(eval $(call BuildPackage,px5g-mbedtls))
