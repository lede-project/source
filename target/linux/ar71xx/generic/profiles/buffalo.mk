#
# Copyright (C) 2009-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/WZRHPG300NH
	NAME:=Buffalo WZR-HP-G300NH
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/WZRHPG300NH/Description
	Package set optimized for the Buffalo WZR-HP-G300NH and WZR-HP-G301NH
endef

$(eval $(call Profile,WZRHPG300NH))

define Profile/WZRHPG300NH2
	NAME:=Buffalo WZR-HP-G300NH2
	PACKAGES:=kmod-ath9k wpad-mini kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/WZRHPG300NH/Description
	Package set optimized for the Buffalo WZR-HP-G300NH2
endef

$(eval $(call Profile,WZRHPG300NH2))

define Profile/WZRHPAG300H
	NAME:=Buffalo WZR-HP-AG300H
	PACKAGES:=kmod-usb-ohci kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/WZRHPAG300H/Description
	Package set optimized for the Buffalo WZR-HP-AG300H
endef

$(eval $(call Profile,WZRHPAG300H))

define Profile/WZRHPG450H
	NAME:=Buffalo WZR-HP-G450H
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/WZRHPG450H/Description
	Package set optimized for the Buffalo WZR-HP-G450H
endef

$(eval $(call Profile,WZRHPG450H))

define Profile/WZR450HP2
	NAME:=Buffalo WZR-450HP2
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/WZR450HP2/Description
	Package set optimized for the Buffalo WZR-450HP2
endef

$(eval $(call Profile,WZR450HP2))

define Profile/WZR600DHP
	NAME:=Buffalo WZR-600DHP
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
endef

define Profile/WZR600DHP/Description
	Package set optimized for the Buffalo WZR-600DHP
endef

$(eval $(call Profile,WZR600DHP))

define Profile/WHRG301N
	NAME:=Buffalo WHR-G301N
	PACKAGES:=
endef

define Profile/WHRG301N/Description
	Package set optimized for the Buffalo WHR-G301N.
endef

$(eval $(call Profile,WHRG301N))


define Profile/WHRHPG300N
	NAME:=Buffalo WHR-HP-G300N
	PACKAGES:=
endef

define Profile/WHRHPG300N/Description
	Package set optimized for the Buffalo WHR-HP-G300N
endef

$(eval $(call Profile,WHRHPG300N))


define Profile/WHRHPGN
	NAME:=Buffalo WHR-HP-GN
	PACKAGES:=
endef

define Profile/WHRHPGN/Description
	Package set optimized for the Buffalo WHR-HP-GN.
endef

$(eval $(call Profile,WHRHPGN))

define Profile/WLAEAG300N
	NAME:=Buffalo WLAE-AG300N
	PACKAGES:=kmod-ledtrig-netdev
endef

define Profile/WLAEAG300N/Description
	Package set optimized for the Buffalo WLAE-AG300N
endef

$(eval $(call Profile,WLAEAG300N))
