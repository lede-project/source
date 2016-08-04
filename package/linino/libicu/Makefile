# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#


include $(TOPDIR)/rules.mk

PKG_NAME:=libicu
PKG_VERSION:=55.1
PKG_RELEASE:=1

PKG_MD5SUM:=e2d523df79d6cb7855c2fbe284f4db29

PKG_SOURCE:=icu4c-55_1-src.tgz
#PKG_SOURCE_URL:=http://download.icu-project.org/files/icu4c/$(PKG_VERSION)
PKG_SOURCE_URL:=@SF/icu

PKG_BUILD_DIR:=$(BUILD_DIR)/icu
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/icu
PKG_SOURCE_SUBDIR:=icu/source
PKG_FIXUP:=libtool,no-autoreconf
PKG_INSTALL=1
PKG_BUILD_DEPENDS:=libicu/host
PKG_BUILD_PARALLEL:=1


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/libicu
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=ICU library for software internationalization
  URL:=http://icu-project.org
  DEPENDS:=+glib2 +libstdcpp +libc
endef

define Package/libicu/description
	ICU is the premier library for software internationalization. ICU 55.1 is a major reference release of ICU, with
	new features, new APIs, and many bug fixes in data and code.
endef

# Specify what needs to be done to prepare for building the package.
# In our case, we need to copy the source files to the build directory.
# This is NOT the default.  The default uses the PKG_SOURCE_URL and the
# PKG_SOURCE which is not defined here to download the source from the web.
# In order to just build a simple program that we have just written, it is
# much easier to do it this way.

define Build/Prepare
        $(call Build/Prepare/Default)        
	$(CP) -a $(PKG_BUILD_DIR)/source/* $(PKG_BUILD_DIR)/
endef
		
define Build/Configure
	$(call Build/Configure/Default, \
	--with-cross-build=$(HOST_BUILD_DIR) \
	--enable-shared=yes --enable-tests=no \
	--enable-samples=no)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/bin/* \
		$(1)/usr/bin/

	$(INSTALL_DIR) $(1)/usr/include
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/* \
		$(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libicu/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib/
endef

define Host/Prepare
	$(call Host/Prepare/Default)
	mkdir -p $(HOST_BUILD_DIR)/
	$(CP) -a $(HOST_BUILD_DIR)/source/* $(HOST_BUILD_DIR)/
endef
		
define Host/Configure
	$(call Host/Configure/Default)
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,libicu))
