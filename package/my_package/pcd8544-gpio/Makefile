#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=pcd8544-gpio

PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_UNPACK=mkdir -p $(PKG_BUILD_DIR); $(CP) ./src/* $(PKG_BUILD_DIR)/


include $(INCLUDE_DIR)/package.mk


define KernelPackage/pcd8544-gpio
  SUBMENU:=Other modules
  TITLE:=driver for pcd8544
  FILES:=$(PKG_BUILD_DIR)/pcd8544-gpio.ko
  AUTOLOAD:=$(call AutoLoad,10,pcd8544-gpio)
endef

define KernelPackage/pcd8544-gpio/description
 ah...
endef

SOURCE_DIR:=$(PKG_BUILD_DIR)
export SOURCE_DIR

define Build/Compile
	$(MAKE) -C $(LINUX_DIR) \
                ARCH="$(LINUX_KARCH)" \
                CROSS_COMPILE="$(TARGET_CROSS)" \
                M="$(PKG_BUILD_DIR)" \
		modules
endef

define KernelPackage/pcd8544-gpio/install
endef

$(eval $(call KernelPackage,pcd8544-gpio))
