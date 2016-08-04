#
# Copyright (C) 2015 DogHunter AG
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=node
PKG_VERSION:=v0.10.46
PKG_RELEASE:=1

PKG_SOURCE:=node-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://nodejs.org/dist/${PKG_VERSION}
# PKG_MD5SUM:=3f45f994f1f02736576bf7bff1870897

PKG_BUILD_PARALLEL:=3
PKG_BUILD_DEPENDS:=v8m-rb +python/host
PKG_V8_VERSION:=3.14.5

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/node
  DEPENDS:=+libpthread +librt +uclibcxx +v8m-rb +libopenssl +libstdcpp +zlib
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Node.js is a platform built on Chrome's JavaScript runtime
  URL:=http://nodejs.org/
endef

define Package/node/description
	Node.js is a platform built on Chrome's JavaScript runtime for easily building fast,
	scalable network applications. Node.js uses an event-driven, non-blocking I/O model
	that makes it lightweight and efficient, perfect for data-intensive real-time applications
	that run across distributed devices.
endef

# NODEJS:=node-$(PKG_VERSION)-linux-x86.tar.gz

CPUHOST:=$(subst x86_64-linux-gnu,x64,$(subst i686-linux-gnu,x86,$(GNU_HOST_NAME)))

NODEJS:=node-$(PKG_VERSION)-linux-$(CPUHOST).tar.gz

ifeq ($(CPUHOST),x64)
    MD5SUM_NODE:=cd36e700215520e02eda9f2ae4d3fba5
endif
ifeq ($(CPUHOST),x86)
    MD5SUM_NODE:=b4b9145436740fe6009449167ae780d4
endif

# Distribution URL doesn't always have the correct version
# Using the OpenWrt/Linino mirror provides a stable version
define Download/nodebin
  URL:=https://nodejs.org/dist/$(PKG_VERSION)/
  FILE:=$(NODEJS)
  MD5SUM:=$(MD5SUM_NODE)
endef

# Fetch latest nodejs that works
# Yes, I know this is ugly

CPU:=$(subst x86_64,x64,$(subst i386,ia32,$(ARCH)))

MAKE_VARS += \
	DESTCPU=$(CPU) \
	snapshot=off \
	werror=no

CONFIGURE_ARGS= \
	--prefix=/usr \
	--without-snapshot \
	--shared-v8 \
	--shared-v8-includes="$(STAGING_DIR)/include/" \
	--shared-v8-libpath="$(STAGING_DIR)/usr/lib" \
	--dest-cpu=$(CPU) \
	--shared-zlib \
	--shared-openssl \
	--dest-os=linux \
	--with-mips-float-abi=soft

define Build/Prepare
    $(call Build/Prepare/Default)
	$(eval $(call Download,nodebin))
	$(TAR) -xvf $(DL_DIR)/$(NODEJS) -C $(STAGING_DIR_HOST) --strip-components=1
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
endef

define Package/node/install
	mkdir -p $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/out/Release/node $(1)/usr/bin/
    #
	mkdir -p $(1)/usr/lib/node_modules
	$(CP) $(PKG_BUILD_DIR)/deps/npm $(1)/usr/lib/node_modules
	ln -sf ../lib/node_modules/npm/bin/npm-cli.js $(1)/usr/bin/npm
endef

$(eval $(call BuildPackage,node))
