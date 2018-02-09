#
# Copyright (C) 2017 Marty E. Plummer
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define KernelPackage/rtc-hisi
	SUBMENU:=$(OTHER_MENU)
	TITLE:=Hisilicon RTC Driver
	DEPENDS:=@TARGET_hisilicon
	$(call AddDepends/rtc)
	KCONFIG:= \
		CONFIG_RTC_DRV_HISI \
		CONFIG_RTC_CLASS=y
	FILES:=$(LINUX_DIR)/drivers/rtc/rtc-hisi.ko
	AUTOLOAD:=$(call AutoLoad,50,rtc-hisi)
endef

define KernelPackage/rtc-hisi/description
 Support for the Hisilicon SoC's onboard RTC
endef

$(eval $(call KernelPackage,rtc-hisi))
