#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/ZBT-WE826
	NAME:=ZBT-WE826
	PACKAGES:=\
		kmod-mt76 kmod-scsi-core kmod-scsi-generic kmod-mmc kmod-sdhci \
		block-mount kmod-usb-core kmod-usb2 kmod-usb-ohci kmod-usb-serial \
		kmod-usb-serial-option kmod-usb-serial-wwan comgt wwan
endef

define Profile/ZBT-WE826/Description
	Support for ZBT WE-826-(B,T) routers
endef
$(eval $(call Profile,ZBT-WE826))
