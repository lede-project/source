#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2017.09
PKG_RELEASE:=1

PKG_HASH:=b2d15f2cf5f72e706025cde73d67247c6da8cd35f7e10891eefe7d9095089744

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

UBOOT_MAKE_FLAGS:=

define U-Boot/Default
  BUILD_TARGET:=mvebu
  HIDDEN:=1
endef

define U-Boot/clearfog
  NAME:=SolidRun ClearFog A1
  BUILD_DEVICES:=armada-388-clearfog-base armada-388-clearfog-pro
  UBOOT_IMAGE:=u-boot-spl.kwb
endef

UBOOT_TARGETS:= \
	clearfog

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_VARIANT)-u-boot-spl.kwb
endef

$(eval $(call BuildPackage/U-Boot))
