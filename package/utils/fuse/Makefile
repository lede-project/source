#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=fuse
PKG_VERSION:=3.1.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/libfuse/libfuse/releases/download/$(PKG_NAME)-$(PKG_VERSION)
PKG_HASH:=3b1610e2d7ff644164526ba8a95d41ca33c01bfef19e63f7138112f1ea364499

PKG_LICENSE:=LGPLv2.1 GPLv2
PKG_LICENSE_FILES:=COPYING.LIB COPYING
PKG_CPE_ID:=cpe:/a:fuse_project:fuse

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/lib$(PKG_NAME)-$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/fuse/Default
  TITLE:=FUSE
  URL:=http://fuse.sourceforge.net/
endef

define Package/fuse/Default/description
 FUSE (Filesystem in UserSpacE)
endef

define Package/fuse-utils
$(call Package/fuse/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libfuse
  TITLE+= (utilities)
  SUBMENU:=Filesystem
endef

define Package/fuse-utils/description
$(call Package/fuse/Default/description)
 This package contains the FUSE utilities.
 - fusermount3
endef

define Package/libfuse
$(call Package/fuse/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE+= (library)
  DEPENDS:=+kmod-fuse +libpthread
  SUBMENU:=Filesystem
endef

define Package/libfuse/description
$(call Package/fuse/Default/description)
 This package contains the FUSE shared libraries, needed by other programs.
 - libfuse
endef

# generic args
CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static \
	--enable-lib \
	--enable-util \
	--disable-rpath \
	--disable-example \
	--disable-mtab

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP)	$(PKG_INSTALL_DIR)/usr/include/fuse3 $(1)/usr/include/
	$(INSTALL_DIR)  $(1)/usr/lib
	$(CP)	$(PKG_INSTALL_DIR)/usr/lib/libfuse3.{a,la,so*} $(1)/usr/lib/
	$(INSTALL_DIR)  $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/fuse3.pc $(1)/usr/lib/pkgconfig/
	$(SED) 's,-I$$$${includedir}/fuse3,,g' $(1)/usr/lib/pkgconfig/fuse3.pc
	$(SED) 's,-L$$$${libdir},,g' $(1)/usr/lib/pkgconfig/fuse3.pc
endef

define Package/fuse-utils/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/etc/init.d/fuse3 $(1)/etc/init.d/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/mount.fuse3 $(1)/sbin/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util/fusermount3 $(1)/usr/bin/
endef

define Package/libfuse/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libfuse3.{a,la,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/include
	$(CP)	$(PKG_INSTALL_DIR)/usr/include/fuse3 $(1)/usr/include/
endef

$(eval $(call BuildPackage,fuse-utils))
$(eval $(call BuildPackage,libfuse))
