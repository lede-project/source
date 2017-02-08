# Copyright (c) dog hunter AG - Zug - CH
# General Public License version 2 (GPLv2)
# Author Mimmo La Fauci <mimmo@linino.org>

# ###########################
#							#
# Starting Multi Profiles	#
#							#
#############################

# Linino ALL

define Profile/LININO
	NAME:=Linino All Profiles
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef

define Profile/LININO/Description
	Select this in order to build an image for every Linino's board.
endef

$(eval $(call Profile,LININO))

# Linino YunOne

define Profile/LININO_YUNONE
	NAME:=Linino YunOne
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef

define Profile/LININO_YUNONE/Description
	Select this in order to build an image for Arduino Yun and Linino One.
endef

$(eval $(call Profile,LININO_YUNONE))

# Linino YunOneLei

define Profile/LININO_YUNONELEI
	NAME:=Linino YunOneLei
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef

define Profile/LININO_YUNONELEI/Description
	Select this in order to build an image for Arduino Yun, Arduino Lei and Linino One.
endef

$(eval $(call Profile,LININO_YUNONELEI))

# Linino AVR

define Profile/LININO_AVR
	NAME:=Linino AVR
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage \
		kmod-fs-vfat kmod-fs-msdos kmod-fs-ntfs kmod-fs-ext4 linino-scripts linino-conf \
		kmod-nls-cp437 kmod-nls-cp850 kmod-nls-cp852 kmod-nls-iso8859-1 kmod-nls-utf8
endef

define Profile/LININO_AVR/Description
	Select this in order to build an image for Linino AVR-based boards (Yun, Yun-mini, One, Chiwawa)
endef

$(eval $(call Profile,LININO_AVR))

# ###########################
#							#
# Starting Single Profiles	#
#							#
#############################

# Linino Yun

define Profile/LININO_YUN
	NAME:=Linino Yun
	PACKAGES:=kmod-usb-core kmod-usb2 luci-app-arduino-webpanel linino-scripts linino-conf
endef

define Profile/LININO_YUN/Description
	Package set optimized for the Arduino Yun based on Atheros AR9331.
endef

$(eval $(call Profile,LININO_YUN))

# Linino One

define Profile/LININO_ONE
	NAME:=Linino One
	PACKAGES:=kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef

define Profile/LININO_ONE/Description
	Package set optimized for the Linino One based on Atheros AR9331.
endef

$(eval $(call Profile,LININO_ONE))

# Linino Freedog

define Profile/LININO_FREEDOG
	NAME:=Linino Freedog
	PACKAGES:=kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef

define Profile/LININO_FREEDOG/Description
	Package set optimized for the Linino Freedog based on Atheros AR9331.
endef

$(eval $(call Profile,LININO_FREEDOG))

# Linino Lei

define Profile/LININO_LEI
	NAME:=Linino Lei
	PACKAGES:=kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef

define Profile/LININO_LEI/Description
	Package set optimized for the Linino Lei based on Atheros AR9342.
endef

$(eval $(call Profile,LININO_LEI))

# Linino Tian

define Profile/LININO_TIAN
	NAME:=Linino Tian
	PACKAGES:=kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef

define Profile/LININO_TIAN/Description
	Package set optimized for the Linino Tian based on Atheros AR9342.
endef

$(eval $(call Profile,LININO_TIAN))

# Linino Chiwawa

define Profile/LININO_CHIWAWA
	NAME:=Linino Chiwawa
	PACKAGES:=kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef

define Profile/LININO_CHIWAWA/Description
	Package set optimized for the Linino Chiwawa based on
	Atheros AR9331.
endef

$(eval $(call Profile,LININO_CHIWAWA))


# Linino Yun Mini

define Profile/LININO_YUN_MINI
	NAME:=Linino Yun mini
	PACKAGES:=kmod-usb-core kmod-usb2 luci-webpanel-linino linino-scripts linino-conf
endef

define Profile/LININO_YUN_MINI/Description
	Package set optimized for the Linino Yun mini based on Atheros AR9331.
endef

$(eval $(call Profile,LININO_YUN_MINI))
