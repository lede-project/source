#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MYNETN600
	NAME:=WD My Net N600
	PACKAGES:=kmod-usb-core kmod-usb2
endef

define Profile/MYNETN600/Description
  Package set optimized for the WD My Net N600 device.
endef
$(eval $(call Profile,MYNETN600))

define Profile/MYNETN750
	NAME:=WD My Net N750
	PACKAGES:=kmod-usb-core kmod-usb2
endef
define Profile/MYNETN750/Description
  Package set optimized for the WD My Net N750 device.
endef

$(eval $(call Profile,MYNETN750))

define Profile/MYNETREXT
	NAME:=WD My Net Wi-Fi Range Extender
	PACKAGES:=rssileds
endef

define Profile/MYNETREXT/Description
  Package set optimized for the WD My Net Wi-Fi Range Extender device.
endef
$(eval $(call Profile,MYNETREXT))
