#
# Copyright (C) 2016 LEDE Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gettext
PKG_VERSION:=0.19.8.1

PKG_SOURCE:=gettext-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@GNU/gettext
PKG_MD5SUM:=df3f5690eaa30fd228537b00cb7b7590

HOST_FIXUP := autoreconf
HOST_BUILD_PARALLEL := 1

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_ARGS += \
	--disable-java \
	--disable-native-java \
	--disable-openmp \
	--with-included-libxml \
	--without-libxml2-prefix \
	--with-included-libcroco

$(eval $(call HostBuild))
