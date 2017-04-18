#
# Copyright (C) 2007-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=protobuf
PKG_VERSION:=3.2.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-cpp-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/google/protobuf/releases/download/v$(PKG_VERSION)
PKG_MD5SUM:=46ab34b63ac77c02d4351961f6f85ded

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=protobuf/host
PKG_USE_MIPS16:=0 # MIPS16 prevents protobuf's usage of the 'sync' asm-opcode

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/protobuf
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A structured data encoding library
  URL:=http://code.google.com/p/protobuf/
  DEPENDS:=+zlib +libpthread +libstdcpp +libprotobuf
  MAINTAINER:=Jacob Siverskog <jacob@teenageengineering.com>
endef

define Package/protobuf/description
 Protocol Buffers are a way of encoding structured data in an efficient
 yet extensible format. Google uses Protocol Buffers for almost all
 of its internal RPC protocols and file formats.
endef

define Package/libprotobuf
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A structured data encoding library
  URL:=http://code.google.com/p/protobuf/
  DEPENDS:=+zlib +libpthread +libstdcpp
  MAINTAINER:=Jacob Siverskog <jacob@teenageengineering.com>
endef

define Package/libprotobuf/description
 Protocol Buffers are a way of encoding structured data in an efficient
 yet extensible format. Google uses Protocol Buffers for almost all
 of its internal RPC protocols and file formats.
endef

CONFIGURE_ARGS += --with-protoc=$(STAGING_DIR_HOSTPKG)/bin/protoc

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/lib \
		$(1)/usr/include

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/* \
		$(1)/usr/include/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/* \
		$(1)/usr/lib/
endef

define Package/libprotobuf/install
	$(INSTALL_DIR) \
		$(1)/usr/lib

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libprotoc.so*  \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libprotobuf-lite.so* \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libprotobuf.so* \
		$(1)/usr/lib/
endef


define Package/protobuf/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/protoc $(1)/usr/bin
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,libprotobuf))
$(eval $(call BuildPackage,protobuf))
