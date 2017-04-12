#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=node7
PKG_VERSION:=v7.9.0
PKG_RELEASE:=1

PKG_SOURCE:=node-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://nodejs.org/dist/$(PKG_VERSION)
# PKG_MD5SUM:=9f15b916f8677ec1615c46bdd6d2208ed1b24fad26384f9ac249f5b09d31c32b

PKG_BUILD_DIR:=$(BUILD_DIR)/node-$(PKG_VERSION)
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/node-$(PKG_VERSION)

HOST_BUILD_DEPENDS:=python/host
PKG_BUILD_DEPENDS:=python/host node7/host
PKG_INSTALL:=1
PKG_USE_MIPS16:=0

HOST_BUILD_PARALLEL:=1
PKG_BUILD_PARALLEL:=1

PKG_MAINTAINER:=John Crispin <blogic@openwrt.org>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/node7
  SECTION:=lang
  CATEGORY:=Languages
  SUBMENU:=Node.js-v7
  TITLE:=Node.js is a platform built on Chrome's JavaScript runtime
  URL:=http://nodejs.org/
  DEPENDS:=+libstdcpp +libopenssl +zlib +USE_UCLIBC:libpthread +USE_UCLIBC:librt +NODEJS_ICU:icu
endef

define Package/node7/description
  Node.js® is a JavaScript runtime built on Chrome's V8 JavaScript engine. Node.js uses
  an event-driven, non-blocking I/O model that makes it lightweight and efficient. Node.js'
   package ecosystem, npm, is the largest ecosystem of open source libraries in the world.
endef

define Package/node7/config
	menu "Module Selection"

	config NODEJS_ICU
		bool "enable i18n features"
		default y

	endmenu
endef

NODEJS_CPU:=$(subst powerpc,ppc,$(subst aarch64,arm64,$(subst x86_64,x64,$(subst i386,ia32,$(ARCH)))))

MAKE_VARS+= \
	DESTCPU=$(NODEJS_CPU)

CONFIGURE_ARGS:= \
	--dest-cpu=$(NODEJS_CPU) \
	--dest-os=linux \
	--without-snapshot \
	--shared-zlib \
	--shared-openssl \
	--prefix=/usr

ifeq ($(CONFIG_NODEJS_ICU),y)
CONFIGURE_ARGS+= \
	--with-intl=system-icu
else
CONFIGURE_ARGS+= \
	--with-intl=none
endif

ifneq ($(findstring mips,$(NODEJS_CPU)),)
ifeq ($(CONFIG_SOFT_FLOAT),y)
CONFIGURE_ARGS+= \
	--with-mips-float-abi=soft
endif
endif

ifneq ($(findstring arm,$(NODEJS_CPU)),)
ifeq ($(CONFIG_SOFT_FLOAT),y)
CONFIGURE_ARGS+= \
	--with-arm-float-abi=softfp
else
ifneq ($(findstring vfp,$(CPU_SUBTYPE)),)
CONFIGURE_ARGS+= \
	--with-arm-fpu=vfp
endif
ifneq ($(findstring vfpv3,$(CPU_SUBTYPE)),)
CONFIGURE_ARGS+= \
	--with-arm-fpu=vfpv3
endif
ifneq ($(findstring vfpv3-d16,$(CPU_SUBTYPE)),)
CONFIGURE_ARGS+= \
	--with-arm-fpu=vfpv3-d16
endif
ifneq ($(findstring neon,$(CPU_SUBTYPE)),)
CONFIGURE_ARGS+= \
	--with-arm-fpu=neon
endif
endif
endif

HOST_CONFIGURE_VARS:=
HOST_CONFIGURE_ARGS:= \
	--dest-os=linux \
	--without-snapshot \
	--shared-zlib \
	--prefix=$(HOST_BUILD_PREFIX)

HOST_CONFIGURE_CMD:=python ./configure

MKTOOL=$(HOST_BUILD_DIR)/out/Release/mkpeephole
FILE_V8_GYP:=$(PKG_BUILD_DIR)/deps/v8/src/v8.gyp

define Build/Prepare
	$(Build/Prepare/Default)
	$(SED) "s#<(mkpeephole_exec)#$(MKTOOL)#g" $(FILE_V8_GYP)
endef

define Host/install
	# NOOP
endef

define Package/node7/install
	mkdir -p $(1)/usr/bin $(1)/usr/lib/node_modules/npm/{bin,lib,node_modules}
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/{node,npm} $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/npm/{package.json,LICENSE,cli.js} $(1)/usr/lib/node_modules/npm
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/npm/bin/npm-cli.js $(1)/usr/lib/node_modules/npm/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/npm/lib/* $(1)/usr/lib/node_modules/npm/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/npm/node_modules/* $(1)/usr/lib/node_modules/npm/node_modules/
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,node7))
