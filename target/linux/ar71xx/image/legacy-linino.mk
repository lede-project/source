# Copyright (c) Arduino Srl
# General Public License version 2 (GPLv2)
# Author Arturo Rinaldi <arturo@arduino.org>

# ###########################
#							#
# Starting Multi Profiles	#
#							#
#############################

# Linino ALL

define LegacyDevice/LININO
	DEVICE_TITLE := Linino All Profiles
	DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef
LEGACY_DEVICES += LININO

# Linino YunOne

define LegacyDevice/LININO_YUNONE
	DEVICE_TITLE := Linino YunOne
	DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef
LEGACY_DEVICES += LININO_YUNONE

# Linino YunOneLei

define LegacyDevice/LININO_YUNONELEI
	DEVICE_TITLE := Linino YunOneLei
	DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef
LEGACY_DEVICES += LININO_YUNONELEI

# Linino AVR

define LegacyDevice/LININO_AVR
	DEVICE_TITLE := Linino AVR
	DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef
LEGACY_DEVICES += LININO_AVR

# ###########################
#							#
# Starting Single Profiles	#
#							#
#############################

# Linino Yun

define LegacyDevice/LININO_YUN
	DEVICE_TITLE := Linino Yun
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-app-arduino-webpanel linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_YUN

# Linino One

define LegacyDevice/LININO_ONE
	DEVICE_TITLE := Linino One
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_ONE

# Linino Freedog

define LegacyDevice/LININO_FREEDOG
	DEVICE_TITLE := Linino Freedog
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_FREEDOG

# Linino Lei

define LegacyDevice/LININO_LEI
	DEVICE_TITLE := Linino Lei
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_LEI

# Linino Tian

define LegacyDevice/LININO_TIAN
	DEVICE_TITLE := Linino Tian
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_TIAN

# Linino Chiwawa

define LegacyDevice/LININO_CHIWAWA
	DEVICE_TITLE := Linino Chiwawa
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_CHIWAWA

# Linino Yun Mini

define LegacyDevice/LININO_YUN_MINI
	DEVICE_TITLE := Linino Yun mini
	DEVICE_PACKAGES := kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef
LEGACY_DEVICES += LININO_YUN_MINI
