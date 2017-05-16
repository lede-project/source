#
# Copyright (C) 2007-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnfnetlink
PKG_VERSION:=1.0.1.git-2015-05-28
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://git.netfilter.org/libnfnetlink
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.xz
PKG_SOURCE_VERSION:=62ca426c2cfbca9be47c778e09990e9412b68bac
PKG_MIRROR_HASH:=18bcf3ddc3f4ec31498d4506a90fcc7f3717d519a66ad450b3ac2d8f01cc4cf8

PKG_MAINTAINER:=Jo-Philipp Wich <jo@mein.io>
PKG_LICENSE:=GPL-2.0+

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libnfnetlink
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A low-level library for netfilter related kernel/userspace communication
  URL:=http://netfilter.org/projects/libnfnetlink/
endef

define Package/libnfnetlink/description
 libnfnetlink is is the low-level library for netfilter related kernel/userspace communication.
 It provides a generic messaging infrastructure for in-kernel netfilter subsystems
 (such as nfnetlink_log, nfnetlink_queue, nfnetlink_conntrack) and their respective users
 and/or management tools in userspace.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--enable-static \
	--enable-shared

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libnfnetlink
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/libnfnetlink/{libnfnetlink,linux_nfnetlink,linux_nfnetlink_compat}.h \
		$(1)/usr/include/libnfnetlink/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libnfnetlink.{a,so*} \
		$(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libnfnetlink.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libnfnetlink/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libnfnetlink.so.* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,libnfnetlink))
