#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=curaengine
PKG_VERSION:=2.4.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/Ultimaker/CuraEngine.git
PKG_SOURCE_SUBDIR:=CuraEngine-$(PKG_VERSION)
PKG_SOURCE_VERSION:=2.4.0
PKG_SOURCE:=CuraEngine-$(PKG_SOURCE_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/CuraEngine-$(PKG_VERSION)

PKG_INSTALL:=1

PKG_BUILD_DEPENDS:=python3/host libprotobuf libArcus

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/curaengine
  TITLE:=CuraEngine is a powerful, fast and robust engine for processing 3D models into 3D printing instruction
  SECTION:=utils
  DEFAULT:=m
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +libc +libstdcpp +libprotobuf +libArcus
  URL:=https://gitorious.org/curaengine
  MDEPENDS:=@!USE_UCLIBC
endef

define package/curaengine/Description
	CuraEngine is a powerful, fast and robust engine for processing 3D models into 3D printing 
	instruction for Ultimaker and other GCode based 3D printers. It is part of the larger open 
	source project called "Cura".
endef

define Package/curaengine/install
	$(INSTALL_DIR) $(1)/usr/bin

	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/CuraEngine $(1)/usr/bin
endef

$(eval $(call BuildPackage,curaengine))
