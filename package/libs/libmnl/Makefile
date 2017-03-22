#
# Copyright (C) 2011-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libmnl
PKG_VERSION:=1.0.4.git-2016-08-08
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://git.netfilter.org/libmnl
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.xz
PKG_SOURCE_VERSION:=fbe0f33b45abd585eb9f52cb56d751a750667dc6
PKG_MIRROR_HASH:=8c1f13ec95ba13b8df6757242c88c6519c75520a88f8edbdf1dbed67caa0b409

PKG_MAINTAINER:=Jo-Philipp Wich <jo@mein.io>

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_LICENSE:=LGPL-2.1+

include $(INCLUDE_DIR)/package.mk

define Package/libmnl
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Minimalistic user-space library for Netlink
  URL:=http://www.netfilter.org/projects/libmnl/
endef

define Package/libmnl/description
 libmnl is a minimalistic user-space library oriented to Netlink developers.
 There are a lot of common tasks in parsing, validating, constructing of
 both the Netlink header and TLVs that are repetitive and easy to get wrong.
 This library aims to provide simple helpers that allows you to re-use code
 and to avoid re-inventing the wheel. The main features of this library are:
 .
 * Small: the shared library requires around 30KB for an x86-based computer.
 .
 * Simple: this library avoids complexity and elaborated abstractions that
   tend to hide Netlink details.
 .
 * Easy to use: the library simplifies the work for Netlink-wise developers.
   It provides functions to make socket handling, message building, validating,
   parsing and sequence tracking, easier.
 .
 * Easy to re-use: you can use the library to build your own abstraction layer
   on top of this library.
 .
 * Decoupling: the interdependency of the main bricks that compose the library
   is reduced, i.e. the library provides many helpers, but the programmer is not
   forced to use them.
endef

CONFIGURE_ARGS+= \
	--enable-shared \
	--enable-static \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libmnl $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libmnl.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libmnl.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libmnl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libmnl.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libmnl))
