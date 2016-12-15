#
# Copyright (C) 2010-2012 OpenWrt.org
# Copyright (C) 2016 LEDE
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=qemu
PKG_VERSION:=2.7.0
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://wiki.qemu-project.org/download/
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_MD5SUM:=08d4d06d1cb598efecd796137f4844ab

HOST_FIXUP:=libtool

include $(INCLUDE_DIR)/host-build.mk

HOST_CFLAGS += -I$(STAGING_DIR_HOST)/include/e2fsprogs

HOST_CONFIGURE_VARS:= \
	CFLAGS="$(HOST_CFLAGS)" \
	LDFLAGS="$(HOST_LDFLAGS)"

HOST_CONFIGURE_ARGS:= \
	--extra-cflags="$(HOST_CFLAGS)" \
	--enable-uuid \
	--without-pixman \
	--disable-tools \
	--disable-system

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) qemu-img
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/qemu-img $(STAGING_DIR_HOST)/bin
endef

$(eval $(call HostBuild))
