#
# Copyright (C) 2016 Jiang Yutang <yutang.jiang@nxp.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

BOARD:=layerscape
BOARDNAME:=NXP Layerscape
DEVICE_TYPE:=developerboard
KERNEL_PATCHVER:=4.4
KERNELNAME:=Image dtbs
FEATURES:=squashfs nand usb pcie gpio
SUBTARGETS:=64b 32b
MAINTAINER:=Jiang Yutang <yutang.jiang@nxp.com>

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for $(BOARDNAME) SoC devices.
endef

DEFAULT_PACKAGES += kmod-usb3 kmod-usb-dwc3 kmod-usb-storage \
		kmod-i2c-core kmod-i2c-imx \
		kmod-mmc kmod-sdhci kmod-sdhci-of-esdhc

$(eval $(call BuildTarget))
