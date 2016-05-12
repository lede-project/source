#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/UBNTRS
	NAME:=Ubiquiti RouterStation
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2
endef

define Profile/UBNTRS/Description
	Package set optimized for the Ubiquiti RouterStation.
endef

$(eval $(call Profile,UBNTRS))

define Profile/UBNTRSPRO
	NAME:=Ubiquiti RouterStation Pro
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2
endef

define Profile/UBNTRSPRO/Description
	Package set optimized for the Ubiquiti RouterStation Pro.
endef

$(eval $(call Profile,UBNTRSPRO))

define Profile/UBNTUNIFI
	NAME:=Ubiquiti UniFi AP
	PACKAGES:=
endef

define Profile/UBNTUNIFI/Description
	Package set optimized for the Ubiquiti UniFi AP.
endef

$(eval $(call Profile,UBNTUNIFI))

define Profile/UBNTUNIFIACLITE
	NAME:=Ubiquiti UniFi AP AC LITE/LR
	PACKAGES:=kmod-ath10k ath10k-firmware-qca988x
endef

define Profile/UBNTUNIFIACLITE/Description
	Package set optimized for the Ubiquiti UniFi AP AC LITE/LR.
endef

$(eval $(call Profile,UBNTUNIFIACLITE))

define Profile/UBNTUNIFIACPRO
	NAME:=Ubiquiti UniFi AP AC PRO
	PACKAGES:=kmod-ath10k ath10k-firmware-qca988x kmod-usb-core kmod-usb-ohci kmod-usb2
endef

define Profile/UBNTUNIFIACPRO/Description
	Package set optimized for the Ubiquiti UniFi AP AC PRO.
endef

$(eval $(call Profile,UBNTUNIFIACPRO))

define Profile/UBNTUNIFIOUTDOOR
	NAME:=Ubiquiti UniFiAP Outdoor
	PACKAGES:=
endef

define Profile/UBNTUNIFIOUTDOOR/Description
	Package set optimized for the Ubiquiti UniFiAP Outdoor.
endef

$(eval $(call Profile,UBNTUNIFIOUTDOOR))

define Profile/UAPPRO
	NAME:=Ubiquiti UniFi AP Pro
	PACKAGES:=
endef

define Profile/UAPPRO/Description
	Package set optimized for the Ubiquiti UniFi AP Pro.
endef

$(eval $(call Profile,UAPPRO))


define Profile/UBNT
	NAME:=Ubiquiti Products
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2
endef

define Profile/UBNT/Description
	Build images for all Ubiquiti products (including LS-SR71, RouterStation and RouterStation Pro)
endef

$(eval $(call Profile,UBNT))
