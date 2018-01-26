#
# Copyright (C) 2017 Tomasz Maciej Nowak <tomek_n@o2.pl>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_VERSION := 2017.11
PKG_RELEASE := 1

PKG_HASH := b2d15f2cf5f72e706025cde73d67247c6da8cd35f7e10891eefe7d9095089744

PKG_MAINTAINER := Tomasz Maciej Nowak <tomek_n@o2.pl>

include $(INCLUDE_DIR)/image.mk
include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET := tegra
  DEPENDS := @TARGET_tegra
  DEFAULT := y
  HIDDEN := y
endef

define U-Boot/trimslice
  NAME := CompuLab TrimSlice
  UBOOT_IMAGE := trimslice-spi.img
  SOC := t20
  BCT_DIR := tegra20/compulab/trimslice
  BCT_CONFIG := trimslice-spi
  BCT_IMAGE := trimslice-spi
endef

UBOOT_TARGETS := trimslice

define Build/bct-image
	ln -sf $(PKG_BUILD_DIR)/u-boot-dtb-tegra.bin u-boot.bin

	cbootimage -$(SOC) \
		-gbct \
		$(STAGING_DIR_HOST)/share/cbootimage-configs/$(BCT_DIR)/$(BCT_CONFIG).bct.cfg \
		$(BCT_CONFIG).bct

	cbootimage -$(SOC) \
		$(STAGING_DIR_HOST)/share/cbootimage-configs/$(BCT_DIR)/$(BCT_IMAGE).img.cfg \
		$(PKG_BUILD_DIR)/$(BCT_IMAGE).img

	rm -f u-boot.bin *.bct
endef

define Build/Compile
	$(call Build/Compile/U-Boot)
	$(call Build/bct-image)
endef

$(eval $(call BuildPackage/U-Boot))
