#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cmake-38
PKG_VERSION:=3.8.0
PKG_RELEASE:=1

PKG_SOURCE_SUBDIR:=cmake-$(PKG_VERSION)
PKG_SOURCE_URL:=https://cmake.org/files/v3.8/
PKG_SOURCE:=cmake-$(PKG_VERSION).tar.gz

PKG_MD5SUM:=f28cba717ba38ad82a488daed8f45b5b

PKG_BUILD_DIR:=$(BUILD_DIR)/cmake-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/cmake-38
  TITLE:=The CMake suite of tools
  SECTION:=devel
  DEFAULT:=m
  CATEGORY:=Development
  DEPENDS:=+libpthread +libc +libstdcpp +librt +libncurses +libopenssl +libcurl +libexpat +libarchive +bzip2 +liblzma +libjsoncpp +libuv +librhash
  URL:=https://cmake.org
endef

define Package/cmake-38/description
	CMake is an open-source, cross-platform family of tools designed to build, test and package software.
endef

CMAKE_OPTIONS = \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_Fortran_COMPILER=$(TOOLCHAIN_DIR)/bin/$(REAL_GNU_TARGET_NAME)-gfortran \
	-DKWSYS_LFS_WORKS=TRUE \
	-DKWSYS_CHAR_IS_SIGNED=TRUE \
	-DCMAKE_USE_SYSTEM_LIBRARIES=1 \
	-DCTEST_USE_XMLRPC=OFF \
	-DBUILD_CursesDialog=OFF

define Package/cmake-38/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/*  $(1)/usr/bin

	$(INSTALL_DIR) $(1)/usr/share/aclocal
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/aclocal/cmake.m4 $(1)/usr/share/aclocals

	$(INSTALL_DIR) $(1)/usr/share
	$(CP) -R $(PKG_INSTALL_DIR)/usr/share/cmake-3.8 $(1)/usr/share
endef

$(eval $(call BuildPackage,cmake-38))
