#
# Copyright (C) 2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/APU2
  NAME:=PC Engines APU2
  PACKAGES:=beep flashrom libsensors lm-sensors usbutils wpad-mini \
	kmod-ath9k kmod-ath10k kmod-gpio-button-hotplug	kmod-gpio-nct5104d \
	kmod-hwmon-core kmod-hwmon-k10temp kmod-leds-apu2 kmod-leds-gpio kmod-pcspkr \
	kmod-sound-core kmod-sp5100_tco kmod-usb-core kmod-usb-ohci kmod-usb-serial \
	kmod-usb2 kmod-usb3 \
	-kmod-e1000e -kmod-e1000 -kmod-r8169
endef

define Profile/APU2/Description
	PC Engines APU2 Embedded Board
endef
$(eval $(call Profile,APU2))
