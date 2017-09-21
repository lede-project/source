#
# Copyright (C) 2016 Arduino Srl
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arduino-builder
PKG_VERSION:=1.3.25
PKG_RELEASE:=3

CPUDEST:=$(subst powerpc,ppc,$(subst aarch64,arm64,$(subst x86_64,amd64,$(subst i386,386,$(ARCH)))))

ifeq ($(ARCH),mips)
  PKG_MD5SUM:=44e571707079f52e823e19aad92f32e0
endif
ifeq ($(ARCH),x86_64)
  PKG_MD5SUM:=9336138f77d2169e2d4d9ba7b42d0998
endif
ifeq ($(ARCH),i386)
  PKG_MD5SUM:=83ba350c73b96a724edd7f4dda7606a6
endif
ifeq ($(ARCH),arm)
  PKG_MD5SUM:=011c99bfb8d893e3abc7e4cf3cf400c2
endif

PKG_SOURCE_URL:=http://download.linino.org/pkg-src/
PKG_SOURCE:=arduino-builder-$(PKG_VERSION)-linux_$(CPUDEST).tar.bz2

include $(INCLUDE_DIR)/package.mk

define Package/arduino-builder
	SECTION:=devel
	CATEGORY:=Development
	TITLE:=Arduino Builder tool
	DEPENDS:=+libc +libstdcpp
endef

define Package/arduino-builder/description
	The Arduino builder engine written in GoLang
endef

define Build/Prepare
	$(TAR) xvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR)
endef

define Build/Configure
	# NOOP
endef

define Build/Compile
	# NOOP
endef

define Package/arduino-builder/install
	$(INSTALL_DIR) $(1)/usr/bin

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/arduino-builder $(1)/usr/bin
endef

$(eval $(call BuildPackage,arduino-builder))
