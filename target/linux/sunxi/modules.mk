#
# Copyright (C) 2013-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

define KernelPackage/axp20x-adc
  SUBMENU:=$(OTHER_MENU)
  TITLE:=X-Powers AXP20X PMIC ADC support
  KCONFIG:=CONFIG_AXP20X_ADC
  FILES:=$(LINUX_DIR)/drivers/iio/adc/axp20x_adc.ko
  AUTOLOAD:=$(call AutoProbe,axp20x_adc)
  DEPENDS:=@TARGET_sunxi +kmod-iio-core
endef

define KernelPackage/axp20x-adc/description
  Kernel support for AXP20X PMIC ADC
endef

$(eval $(call KernelPackage,axp20x-adc))

define KernelPackage/axp20x-ac-power
  SUBMENU:=$(OTHER_MENU)
  TITLE:=X-Powers AXP20X PMIC AC power supply support
  KCONFIG:=CONFIG_CHARGER_AXP20X
  FILES:=$(LINUX_DIR)/drivers/power/supply/axp20x_ac_power.ko
  AUTOLOAD:=$(call AutoProbe,axp20x_ac_power)
  DEPENDS:=@TARGET_sunxi +kmod-axp20x-adc
endef

define KernelPackage/axp20x-ac-power/description
  Kernel support for AXP20X PMIC AC power supply
endef

$(eval $(call KernelPackage,axp20x-ac-power))

define KernelPackage/axp20x-battery
  SUBMENU:=$(OTHER_MENU)
  TITLE:=X-Powers AXP20X PMIC battery support
  KCONFIG:=CONFIG_BATTERY_AXP20X
  FILES:=$(LINUX_DIR)/drivers/power/supply/axp20x_battery.ko
  AUTOLOAD:=$(call AutoProbe,axp20x_battery)
  DEPENDS:=@TARGET_sunxi +kmod-axp20x-adc
endef

define KernelPackage/axp20x-battery/description
  Kernel support for AXP20X PMIC battery
endef

$(eval $(call KernelPackage,axp20x-battery))

define KernelPackage/axp20x-usb-power
  SUBMENU:=$(OTHER_MENU)
  TITLE:=X-Powers AXP20X PMIC USB power supply support
  KCONFIG:=CONFIG_AXP20X_POWER
  FILES:=$(LINUX_DIR)/drivers/power/supply/axp20x_usb_power.ko
  AUTOLOAD:=$(call AutoProbe,axp20x_usb_power)
  DEPENDS:=@TARGET_sunxi +kmod-axp20x-adc
endef

define KernelPackage/axp20x-usb-power/description
  Kernel support for AXP20X PMIC USB power supply
endef

$(eval $(call KernelPackage,axp20x-usb-power))

define KernelPackage/sunxi-ir
    SUBMENU:=$(OTHER_MENU)
    TITLE:=Sunxi SoC built-in IR support (A20)
    DEPENDS:=@TARGET_sunxi +kmod-input-core
    $(call AddDepends/rtc)
    KCONFIG:= \
	CONFIG_MEDIA_SUPPORT=y \
	CONFIG_MEDIA_RC_SUPPORT=y \
	CONFIG_RC_DEVICES=y \
	CONFIG_IR_SUNXI
    FILES:=$(LINUX_DIR)/drivers/media/rc/sunxi-cir.ko
    AUTOLOAD:=$(call AutoLoad,50,sunxi-cir)
endef

define KernelPackage/sunxi-ir/description
 Support for the AllWinner sunXi SoC's onboard IR (A20)
endef

$(eval $(call KernelPackage,sunxi-ir))

define KernelPackage/ata-sunxi
    TITLE:=AllWinner sunXi AHCI SATA support
    SUBMENU:=$(BLOCK_MENU)
    DEPENDS:=@TARGET_sunxi +kmod-ata-ahci-platform +kmod-scsi-core
    KCONFIG:=CONFIG_AHCI_SUNXI
    FILES:=$(LINUX_DIR)/drivers/ata/ahci_sunxi.ko
    AUTOLOAD:=$(call AutoLoad,41,ahci_sunxi,1)
endef

define KernelPackage/ata-sunxi/description
 SATA support for the AllWinner sunXi SoC's onboard AHCI SATA
endef

$(eval $(call KernelPackage,ata-sunxi))

define KernelPackage/sun4i-emac
  SUBMENU:=$(NETWORK_DEVICES_MENU)
  TITLE:=AllWinner EMAC Ethernet support
  DEPENDS:=@TARGET_sunxi +kmod-of-mdio +kmod-libphy
  KCONFIG:=CONFIG_SUN4I_EMAC
  FILES:=$(LINUX_DIR)/drivers/net/ethernet/allwinner/sun4i-emac.ko
  AUTOLOAD:=$(call AutoProbe,sun4i-emac)
endef

$(eval $(call KernelPackage,sun4i-emac))


define KernelPackage/sound-soc-sunxi
  TITLE:=AllWinner built-in SoC sound support
  KCONFIG:=CONFIG_SND_SUN4I_CODEC
  FILES:=$(LINUX_DIR)/sound/soc/sunxi/sun4i-codec.ko
  AUTOLOAD:=$(call AutoLoad,65,sun4i-codec)
  DEPENDS:=@TARGET_sunxi +kmod-sound-soc-core
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-sunxi/description
  Kernel support for AllWinner built-in SoC audio
endef

$(eval $(call KernelPackage,sound-soc-sunxi))
