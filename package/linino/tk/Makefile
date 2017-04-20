#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tk
PKG_VERSION:=8.6.6
PKG_RELEASE:=5

PKG_SOURCE:=$(PKG_NAME)$(PKG_VERSION)-src.tar.gz
PKG_SOURCE_URL:=@SF/tcl
PKG_MD5SUM:=dd7dbb3a6523c42d05f6ab6e86096e99

PKG_LICENSE:=TCL
PKG_LICENSE_FILES:=license.terms
PKG_MAINTAINER:=Joe Mistachkin <joe@mistachkin.com>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/tk
  SUBMENU:=Tk
  SECTION:=lang
  CATEGORY:=Languages
  DEPENDS:=+libpthread +zlib +libX11 +libXft +tcl
  TITLE:=The Tk language
  URL:=http://www.tk.tk/
endef

define Package/tk/description
  Tk, or Tool Command Language, is a an elegant, versatile, feature-rich,
  simple-to-learn yet very powerful industrial-strength open-source
  programming language and development platform. It is renowned for its
  stability and utility, and its emphasis on providing a cross-platform
  programming API makes it an ideal choice for an enormous variety of
  programming jobs.
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR)/unix; \
	autoconf configure.in > configure; \
	sed -i.bak "s/relid'/relid/" configure; \
	$(TARGET_CONFIGURE_OPTS) \
	./configure \
		--build=$(GNU_HOST_NAME) \
		--host=$(REAL_GNU_TARGET_NAME) \
		--target=$(REAL_GNU_TARGET_NAME) \
		--prefix=/usr \
		--enable-threads \
		--disable-shared \
	)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/unix \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/*.h $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.a $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/tkConfig.sh $(1)/usr/lib/
endef

define Package/tk/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.a $(1)/usr/lib

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin
endef

#define Package/tk/prerm
##!/bin/sh
#for x in itk4.0.5 sqlite3.13.0 tk8 tk8.6 tdbc1.0.4 tdbcmysql1.0.4 tdbcodbc1.0.4 tdbcpostgres1.0.4 thread2.8.0; do
#  find $${IPKG_INSTROOT}/usr/lib -name $$x -type d -exec rm -rf {} \; 2>/dev/null
#done
#
#for q in libtk8.6.a libtkstub8.6.a tkConfig.sh tkConfig.sh; do
#  find $${IPKG_INSTROOT}/usr/lib -name $$q -type f -exec rm -rf {} \; 2>/dev/null
#done
#exit 0
#endef

$(eval $(call BuildPackage,tk))
