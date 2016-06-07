#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=adkill

PKG_VERSION:=e2b788431f
PKG_RELEASE:=1
PKG_REV:=e2b788431ff65f5217cefd1842e69261c8829a54

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/981213/adkill.git
PKG_SOURCE_VERSION:=$(PKG_REV)

#PKG_UNPACK=mkdir -p $(PKG_BUILD_DIR); $(CP) ./src/* $(PKG_BUILD_DIR)/

include $(INCLUDE_DIR)/package.mk


define KernelPackage/adkill
  SUBMENU:=Other modules
  TITLE:=An AD-Killer for some Chinese video sites
  FILES:=$(PKG_BUILD_DIR)/adv-kill.ko
endef

define KernelPackage/adkill/description
  An AD-Killer for some Chinese video sites
endef

define Build/Compile
	$(MAKE) -C $(LINUX_DIR) \
                ARCH="$(LINUX_KARCH)" \
                CROSS_COMPILE="$(TARGET_CROSS)" \
                M="$(PKG_BUILD_DIR)" \
		modules
endef

define KernelPackage/adkill/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/conffile $(1)/etc/config/adkill
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/init $(1)/etc/init.d/adkill
endef

$(eval $(call KernelPackage,adkill))
