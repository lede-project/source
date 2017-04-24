#
# Copyright (C) 2015 OpenWrt.org
#
# This i[M#Æs free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/CHILIBOARD
	NAME:=Grinn Chiliboard
	DEFAULT_PACKAGES += \
			kmod-core kmod-usb2
endef

define Profile/CHILIBOARD/Description
	Package set for the CHILIBOARD and similar devices.
endef

$(eval $(call Profile,CHILIBOARD))
