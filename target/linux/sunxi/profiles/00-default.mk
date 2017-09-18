#
# Copyright (C) 2017 Yousong Zhou
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Default
  NAME:=Default Profile (all drivers)
  PACKAGES:= \
	brcmfmac-firmware-43362-sdio \
	kmod-ata-core \
	kmod-ata-sunxi \
	kmod-brcmfmac \
	kmod-rtl8192cu \
	kmod-rtl8xxxu \
	kmod-sun4i-emac \
	rtl8188eu-firmware \
	swconfig \
	wpad-mini
  PRIORITY := 1
endef

define Profile/Default/Description
  Default profile with package set compatible with most boards.
endef
$(eval $(call Profile,Default))
