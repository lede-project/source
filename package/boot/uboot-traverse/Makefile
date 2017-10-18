#
# Copyright (C) 2017 Traverse Technologies
# Author: Mathew McBride <matt@traverse.com.au>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=uboot-traverse
PKG_VERSION:=v1.2
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://gitlab.com/traverse-nxp-qoriq/qoriq-uboot.git
PKG_SOURCE_VERSION:=52d56f2b6e12f324cd402789ff757347e56b3a2e

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=layerscape
  BUILD_SUBTARGET:=armv8_64b
  BUILD_DEVICES:=$(1)
  UBOOT_IMAGE:=u-boot-dtb.bin
  HIDDEN:=n
endef

define U-Boot/ls1043ardb-nand
  NAME:=NXP LS1043ARDB (Traverse NAND Configuration)
   UBOOT_CONFIG:=ls1043ardb_nand
   UBOOT_IMAGE:=u-boot-with-spl-pbl.bin
endef

define U-Boot/traverse-ls1043v
  NAME:=Traverse LS1043V Board (LS1043A)
  UBOOT_CONFIG:=ls1043v
  UBOOT_IMAGE:=u-boot-with-spl-pbl.bin
  BUILD_DEVICES:=traverse-five64
endef

define U-Boot/traverse-ls1043v-sdcard
  NAME:=Traverse LS1043V Board (LS1043A,SD card)
  UBOOT_CONFIG:=ls1043v_sdcard
  UBOOT_IMAGE:=u-boot-with-spl-pbl.bin
  BUILD_DEVICES:=traverse-five64
endef

UBOOT_TARGETS := \
	traverse-ls1043v \
	traverse-ls1043v-sdcard \

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-64b-uboot.bin
endef

# Override the default U-Boot compile so the kernel DTC is not called
# (there are some feature checks in the u-boot DTC not yet in the kernel DTC)
define Build/Compile/U-Boot
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE=$(TARGET_CROSS) \
		$(UBOOT_MAKE_FLAGS)
endef

define Package/u-boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
