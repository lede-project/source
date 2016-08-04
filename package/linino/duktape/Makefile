#
# Copyright (C) Dog Hunter AG 2014
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=duktape
PKG_VERSION:=1.2.4
PKG_RELEASE:=1
PKG_MD5SUM:=a103ddfd626b928001c2d383d7b30cdb

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
# PKG_SOURCE:=$(PKG_NAME)-master.tar.xz
PKG_SOURCE_URL:=http://duktape.org/
# PKG_SOURCE_URL:=http://duktape.org/snapshots/

PKG_FILE:="$(STAGING_DIR)/pkginfo/libc.provides"
PKG_CAT=`cat $(PKG_FILE) | grep libm.so.6` 

include $(INCLUDE_DIR)/package.mk

define Package/duktape
	DEPENDS:=+libstdcpp +libc +libpthread
	SECTION:=lang
	CATEGORY:=Languages
	TITLE:=Duktape embeddable JavaScript interpreter
	URL:=http://www.duktape.org
endef

define Package/duktape/description
	Duktape is a light-weight embeddable JavaScript interpreter.  It is provided as
	a C source file that is to be compiled into other projects.  Because it is
	highly configurable at compile-time, it is not suitable for being built as a
	library.
endef

define Build/Prepare
	(if [ ! $(PKG_CAT) ];then echo "libm.so.6" >> $(PKG_FILE); fi)
	/bin/tar xvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components=1
	$(Build/Patch)
	mv $(PKG_BUILD_DIR)/Makefile.cmdline $(PKG_BUILD_DIR)/Makefile
endef

define Build/Compile
	CC=$(TARGET_CC) \
	CXX=$(TARGET_CXX) \
	$(MAKE) -C $(PKG_BUILD_DIR) DESTDIR="$(PKG_INSTALL_DIR)"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr
	$(INSTALL_DIR) $(1)/usr/src
	$(INSTALL_DIR) $(1)/usr/src/duktape
	$(INSTALL_DIR) $(1)/usr/src/duktape/src
	$(CP) $(PKG_BUILD_DIR)/src/*.c $(1)/usr/src/duktape/src
	$(CP) $(PKG_BUILD_DIR)/src/*.h $(1)/usr/src/duktape/src
endef

define Package/duktape/install
	$(INSTALL_DIR) $(1)/usr/bin/
	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/duk $(1)/usr/bin/
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,duktape))
