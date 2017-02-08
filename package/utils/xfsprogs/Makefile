#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=xfsprogs
PKG_RELEASE:=1

PKG_VERSION:=4.9.0
PKG_SOURCE_URL:=https://www.kernel.org/pub/linux/utils/fs/xfs/xfsprogs/
PKG_HASH:=cb97fa49b4eb0fb9622c064510904aeac556c51befb647c08e73643b47a49eb3
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz

PKG_INSTALL:=1

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/xfsprogs/default
  SECTION:=utils
  CATEGORY:=Utilities
  SUBMENU:=Filesystem
  DEPENDS:=+libuuid +libpthread +librt
  URL:=http://oss.sgi.com/projects/xfs
endef

define Package/xfs-mkfs
$(call Package/xfsprogs/default)
  TITLE:=Utility for creating XFS filesystems
endef

define Package/xfs-fsck
$(call Package/xfsprogs/default)
  TITLE:=Utilities for checking and repairing XFS filesystems
endef

define Package/xfs-growfs
$(call Package/xfsprogs/default)
  TITLE:=Utility for increasing the size of XFS filesystems
endef

define Package/xfs-io
$(call Package/xfsprogs/default)
  TITLE:=Utility for debugging the I/O path of an XFS filesystem
endef

define Build/Configure
	$(MAKE) -C $(PKG_BUILD_DIR) configure
endef

CONFIGURE_ARGS += \
	--disable-blkid \
	--disable-editline \
	--disable-gettext \
	--disable-lib64 \
	--disable-readline \
	--disable-termcap \
	--enable-librt

MAKE_FLAGS += \
	DEBUG=-DNDEBUG Q= \
	PCFLAGS="-Wall" \
	PKG_PLATFORM=linux \
	ENABLE_GETTEXT=no \
	prefix=$(PKG_INSTALL_DIR)/usr \
	exec_prefix=$(PKG_INSTALL_DIR)/usr \
	PKG_SBIN_DIR=$(PKG_INSTALL_DIR)/usr/sbin \
	PKG_ROOT_SBIN_DIR=$(PKG_INSTALL_DIR)/sbin \
	PKG_MAN_DIR=$(PKG_INSTALL_DIR)/usr/man \
	PKG_LOCALE_DIR=$(PKG_INSTALL_DIR)/usr/share/locale \
	PKG_ROOT_LIB_DIR=$(PKG_INSTALL_DIR)/lib \
	PKG_DOC_DIR=$(PKG_INSTALL_DIR)/usr/share/doc/xfsprogs

define Package/xfs-mkfs/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/sbin/mkfs.xfs $(1)/usr/sbin
endef

define Package/xfs-fsck/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/sbin/xfs_repair $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/xfs_db $(1)/usr/sbin
endef

define Package/xfs-growfs/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/xfs_growfs $(1)/usr/sbin
endef

define Package/xfs-io/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/xfs_io $(1)/sbin
endef

$(eval $(call BuildPackage,xfs-mkfs))
$(eval $(call BuildPackage,xfs-fsck))
$(eval $(call BuildPackage,xfs-growfs))
$(eval $(call BuildPackage,xfs-io))