#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=simple-mtpfs
PKG_VERSION:=0.2
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/simple-mtpfs-$(PKG_VERSION)
PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/phatina/simple-mtpfs.git
PKG_SOURCE_VERSION:=80784a0c8c31b812052c5f23781516bb25a4bf93

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_DEPENDS:=libmtp

include $(INCLUDE_DIR)/package.mk

define Package/simple-mtpfs
  SECTION:=base
  CATEGORY:=Base system
  DEPENDS:=+libusb-1.0 +libfuse +libmtp +libstdcpp
  TITLE:=simple-mtpfs
  URL:=https://github.com/phatina/simple-mtpfs
endef

define Package/simple-mtpfs/description
SIMPLE-MTPFS (Simple Media Transfer Protocol FileSystem) is a file system for Linux
(and other operating systems with a FUSE implementation, such as Mac OS X or FreeBSD)
capable of operating on files on MTP devices attached via USB to local machine.
On the local computer where the SIMPLE-MTPFS is mounted, the implementation makes use of
the FUSE (Filesystem in Userspace) kernel module.
The practical effect of this is that the end user can seamlessly interact with MTP device files.
endef

define Build/Configure
	$(call Build/Configure/Default)
endef

define Package/simple-mtpfs/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) ${PKG_INSTALL_DIR}/usr/bin/simple-mtpfs $(1)/usr/bin
endef

$(eval $(call BuildPackage,simple-mtpfs))
