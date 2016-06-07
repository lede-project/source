#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rtl8188eu
PKG_RELEASE:=1
PKG_VERSION:=fcc9034
PKG_REV:=fcc9034b0d2170b4942133a43a9521aab2e0beac

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/lwfinger/rtl8188eu.git
PKG_SOURCE_VERSION:=$(PKG_REV)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/rtl8188eu
  SUBMENU:=Wireless Drivers
  TITLE:=Realtek RTL8188EU Driver
  DEPENDS:=+kmod-usb-core
  FILES:=$(PKG_BUILD_DIR)/8188eu.ko
  AUTOLOAD:=$(call AutoLoad,30,8188eu,1)
  KCONFIG:=
endef

define KernelPackage/rtl8188eu/description
  Kernel module for RTL8188EU Driver
endef

#MAKE_OPTS:= \
#	ARCH="$(LINUX_KARCH)" \
#	CROSS_COMPILE="$(TARGET_CROSS)" \
#	SUBDIRS="$(PKG_BUILD_DIR)" \
#	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
#	$(EXTRA_KCONFIG)



define Build/Compile
	$(MAKE) -f $(PKG_BUILD_DIR)/Makefile ARCH="$(LINUX_KARCH)" CROSS_COMPILE="$(TARGET_CROSS)" KSRC=$(LINUX_DIR) PKG_BUILD_DIR=$(PKG_BUILD_DIR) all
endef

define KernelPackage/rtl8188eu/install
	$(INSTALL_DIR) $(1)/lib/firmware/rtlwifi
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/rtl8188eufw.bin $(1)/lib/firmware/rtlwifi
endef


$(eval $(call KernelPackage,rtl8188eu))
