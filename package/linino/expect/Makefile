
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licenmake under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=expect
PKG_VERSION:=5.45
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/expect
PKG_MD5SUM:=44e1a4f4c877e9ddc5a542dfa7ecc92b
PKG_USE_MIPS16:=0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)$(PKG_VERSION)

PKG_BUILD_DEPENDS:=tcl tcl/host

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

CONFIGURE_ARGS += \
	--prefix=/usr \
	--with-tcl=$(STAGING_DIR)/usr/lib

define Package/expect
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libc +libstdcpp +tcl
  TITLE:=expect
  URL:=http://sourceforge.net/projects/expect
endef

define Package/expect/description
	Non-interactive ssh password auth
endef

define Build/Prepare
	$(Build/Prepare/Default)
	(cd $(PKG_BUILD_DIR) && autoconf configure.in > configure) 
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		TCLLIBPATH=$(STAGING_DIR)/usr/lib \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		SCRIPTS="" all
endef

define Package/expect/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/expect $(1)/usr/bin/
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/expect$(PKG_VERSION)/*.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,expect))
