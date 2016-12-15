#
# Copyright (C) 2016 LEDE Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libffi
PKG_VERSION:=3.2.1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=ftp://sourceware.org/pub/libffi/
PKG_MD5SUM:=83b89587607e3eb65c70d361f13bab43

HOST_FIXUP := autoreconf
HOST_BUILD_PARALLEL := 1

include $(INCLUDE_DIR)/host-build.mk

$(eval $(call HostBuild))
