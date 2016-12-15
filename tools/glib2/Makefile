#
# Copyright (C) 2016 LEDE Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=glib2
PKG_VERSION:=2.49.7

PKG_SOURCE:=glib-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@GNOME/glib/2.49
PKG_MD5SUM:=397ead3fcf325cb921d54e2c9e7dfd7a

HOST_FIXUP := autoreconf
HOST_BUILD_PARALLEL := 1
HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/glib-$(PKG_VERSION)

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_ARGS += \
	--disable-selinux \
	--with-libiconv=maybe \
	--with-pcre=internal

$(eval $(call HostBuild))
