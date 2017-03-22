#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnftnl
PKG_VERSION:=1.0.7.git-2017-03-15
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://git.netfilter.org/libnftnl
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.xz
PKG_SOURCE_VERSION:=4b89c0cb0883f638ff1abbc2ff47c43cdc26aac5
PKG_MIRROR_HASH:=929ca8af5eeacca1c2526d6bc7dcb8852b9c96d0a85fd12ad10800112e287f5e

PKG_MAINTAINER:=Steven Barth <steven@midlink.org>
PKG_LICENSE:=GPL-2.0+

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libnftnl
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libmnl
  TITLE:=Low-level netlink library for the nf_tables subsystem
  URL:=http://www.netfilter.org/projects/libnftnl
endef

define Package/libnftnl/description
 libnftnl is a userspace library providing a low-level netlink
 programming interface (API) to the in-kernel nf_tables subsystem.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--enable-static \
	--enable-shared \
	--without-json-parsing \
	--without-xml-parsing \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libnftnl
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/libnftnl/*.h \
		$(1)/usr/include/libnftnl/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libnftnl.{so*,a,la} \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libnftnl.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libnftnl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libnftnl.so.* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,libnftnl))
