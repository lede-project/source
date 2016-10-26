#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Generic
  NAME:=Generic (default)
  PACKAGES:= \
	kmod-mmc kmod-mvsdio kmod-usb2 kmod-usb-storage \
	kmod-i2c-core kmod-i2c-mv64xxx \
	kmod-ata-core kmod-ata-marvell-sata \
	kmod-thermal-kirkwood \
	kmod-mwl8k swconfig wpad-mini
endef

define Profile/Generic/Description
 Package set compatible with most Marvell Kirkwood based boards.
endef

$(eval $(call Profile,Generic))
