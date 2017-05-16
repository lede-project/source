#
# Copyright (C) 2009-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnetfilter_conntrack
PKG_VERSION:=1.0.6.git-2017-03-03
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://git.netfilter.org/libnetfilter_conntrack
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.xz
PKG_SOURCE_VERSION:=79dac5ac16ffe102b120c00600fb97653fe49c4b
PKG_MIRROR_HASH:=e5994bb86988d46dae591f1e631b4d498e029e4ff26b28b1fd6327baac40901d

PKG_MAINTAINER:=Jo-Philipp Wich <jo@mein.io>

PKG_FIXUP:=autoreconf
PKG_LICENSE:=GPL-2.0+

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libnetfilter-conntrack
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libnfnetlink +kmod-nf-conntrack-netlink +libmnl
  TITLE:=API to the in-kernel connection tracking state table
  URL:=http://www.netfilter.org/projects/libnetfilter_conntrack/
endef

define Package/libnetfilter-conntrack/description
 libnetfilter_conntrack is a userspace library providing a programming
 interface (API) to the in-kernel connection tracking state table. The
 library libnetfilter_conntrack has been previously known as
 libnfnetlink_conntrack and libctnetlink. This library is currently
 used by conntrack-tools among many other applications.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--enable-static \
	--enable-shared \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libnetfilter_conntrack
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/libnetfilter_conntrack/*.h \
		$(1)/usr/include/libnetfilter_conntrack/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libnetfilter_conntrack.{so*,a,la} \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libnetfilter_conntrack.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libnetfilter-conntrack/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libnetfilter_conntrack.so.* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,libnetfilter-conntrack))
