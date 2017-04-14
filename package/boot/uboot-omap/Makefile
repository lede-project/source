#
# Copyright (C) 2012-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2017.03
PKG_RELEASE:=1

PKG_HASH:=f54baf3f9325bf444c7905f3a5b6f83680edb1e6e1a4d5f8a5ad80abe885113f

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=omap
  BUILD_DEVICES:=Default
  UBOOT_IMAGE:=u-boot.img MLO
endef

define U-Boot/omap4_panda
  NAME:=Pandaboard
endef

define U-Boot/am335x_evm
  NAME:=AM335x EVM
endef

define U-Boot/omap3_overo
  NAME:=Gumstix Overo
endef

define U-Boot/omap3_beagle
  NAME:=BeagleBoard
endef

define U-Boot/chiliboard
    NAME:=Chiliboard
endef

UBOOT_TARGETS:=omap4_panda am335x_evm omap3_overo omap3_beagle chiliboard

$(eval $(call BuildPackage/U-Boot))
