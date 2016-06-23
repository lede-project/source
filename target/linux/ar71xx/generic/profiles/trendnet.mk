#
# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/TEW632BRP
	NAME:=TRENDNet TEW-632BRP
	PACKAGES:=
endef

define Profile/TEW632BRP/Description
	Package set optimized for the TRENDNet TEW-632BRP.
endef

$(eval $(call Profile,TEW632BRP))

define Profile/TEW652BRP
	NAME:=TRENDNet TEW-652BRP
	PACKAGES:=
endef

define Profile/TEW652BRP/Description
	Package set optimized for the TRENDNet TEW-652BRP.
endef

$(eval $(call Profile,TEW652BRP))

define Profile/TEW673GRU
	NAME:=TRENDNet TEW-673GRU
	PACKAGES:=kmod-usb-core kmod-usb-ohci kmod-usb2
endef

define Profile/TEW673GRU/Description
	Package set optimized for the TRENDNet TEW-673GRU.
endef

$(eval $(call Profile,TEW673GRU))

define Profile/TEW712BR
	NAME:=TRENDNet TEW-712BR
	PACKAGES:=
endef

define Profile/TEW712BR/Description
	Package set optimized for the TRENDNet TEW-712BR.
endef

$(eval $(call Profile,TEW712BR))


define Profile/TEW732BR
	NAME:=TRENDNet TEW-732BR
	PACKAGES:=
endef

define Profile/TEW732BR/Description
	Package set optimized for the TRENDNet TEW-732BR.
endef

$(eval $(call Profile,TEW732BR))


define Profile/TEW823DRU
	NAME:=TRENDNet TEW-823DRU
	PACKAGES:=kmod-usb-core kmod-usb2 kmod-ath10k
endef

define Profile/TEW823DRU/Description
	Package set optimized for the TRENDNet TEW-823DRU.
endef

$(eval $(call Profile,TEW823DRU))
