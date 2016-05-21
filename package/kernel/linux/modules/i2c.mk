#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

I2C_MENU:=I2C support

ModuleConfVar=$(word 1,$(subst :,$(space),$(1)))
ModuleFullPath=$(LINUX_DIR)/$(word 2,$(subst :,$(space),$(1))).ko
ModuleKconfig=$(foreach mod,$(1),$(call ModuleConfVar,$(mod)))
ModuleFiles=$(foreach mod,$(1),$(call ModuleFullPath,$(mod)))
ModuleAuto=$(call AutoLoad,$(1),$(foreach mod,$(2),$(basename $(notdir $(call ModuleFullPath,$(mod))))),$(3))

define i2c_defaults
  SUBMENU:=$(I2C_MENU)
  KCONFIG:=$(call ModuleKconfig,$(1))
  FILES:=$(call ModuleFiles,$(1))
  AUTOLOAD:=$(call ModuleAuto,$(2),$(1),$(3))
endef

I2C_CORE_MODULES:= \
  CONFIG_I2C:drivers/i2c/i2c-core \
  CONFIG_I2C_CHARDEV:drivers/i2c/i2c-dev

ifeq ($(CONFIG_OF),y)
  I2C_CORE_MODULES+=CONFIG_OF_I2C:drivers/of/of_i2c@lt3.12
endif

define KernelPackage/i2c-core
  $(call i2c_defaults,$(I2C_CORE_MODULES),51)
  TITLE:=I2C support
endef

define KernelPackage/i2c-core/description
 Kernel modules for I2C support
endef

$(eval $(call KernelPackage,i2c-core))


I2C_ALGOBIT_MODULES:= \
  CONFIG_I2C_ALGOBIT:drivers/i2c/algos/i2c-algo-bit

define KernelPackage/i2c-algo-bit
  $(call i2c_defaults,$(I2C_ALGOBIT_MODULES),55)
  TITLE:=I2C bit-banging interfaces
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-algo-bit/description
 Kernel modules for I2C bit-banging interfaces
endef

$(eval $(call KernelPackage,i2c-algo-bit))


I2C_ALGOPCA_MODULES:= \
  CONFIG_I2C_ALGOPCA:drivers/i2c/algos/i2c-algo-pca

define KernelPackage/i2c-algo-pca
  $(call i2c_defaults,$(I2C_ALGOPCA_MODULES),55)
  TITLE:=I2C PCA 9564 interfaces
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-algo-pca/description
 Kernel modules for I2C PCA 9564 interfaces
endef

$(eval $(call KernelPackage,i2c-algo-pca))


I2C_ALGOPCF_MODULES:= \
  CONFIG_I2C_ALGOPCF:drivers/i2c/algos/i2c-algo-pcf

define KernelPackage/i2c-algo-pcf
  $(call i2c_defaults,$(I2C_ALGOPCF_MODULES),55)
  TITLE:=I2C PCF 8584 interfaces
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-algo-pcf/description
 Kernel modules for I2C PCF 8584 interfaces
endef

$(eval $(call KernelPackage,i2c-algo-pcf))


I2C_GPIO_MODULES:= \
  CONFIG_I2C_GPIO:drivers/i2c/busses/i2c-gpio

define KernelPackage/i2c-gpio
  $(call i2c_defaults,$(I2C_GPIO_MODULES),59)
  TITLE:=GPIO-based bitbanging I2C
  DEPENDS:=@GPIO_SUPPORT +kmod-i2c-algo-bit
endef

define KernelPackage/i2c-gpio/description
 Kernel modules for a very simple bitbanging I2C driver utilizing the
 arch-neutral GPIO API to control the SCL and SDA lines.
endef

$(eval $(call KernelPackage,i2c-gpio))

I2C_MPC_MODULES:=\
  CONFIG_I2C_MPC:drivers/i2c/busses/i2c-mpc

define KernelPackage/i2c-mpc
  $(call i2c_defaults,$(I2C_MPC_MODULES),59)
  TITLE:=MPC I2C accessors
  DEPENDS:=@TARGET_mpc52xx||TARGET_mpc83xx||TARGET_mpc85xx +kmod-i2c-core
endef

define KernelPackage/i2c-mpc/description
 Kernel module for Freescale MPC52xx MPC83xx MPC85xx I2C accessors
endef

$(eval $(call KernelPackage,i2c-mpc))

I2C_IBM_IIC_MODULES:=\
  CONFIG_I2C_IBM_IIC:drivers/i2c/busses/i2c-ibm_iic

define KernelPackage/i2c-ibm-iic
  $(call i2c_defaults,$(OF_I2C_MODULES),59)
  TITLE:=IBM PPC 4xx on-chip I2C interface support
  DEPENDS:=@TARGET_ppc40x||TARGET_ppc4xx +kmod-i2c-core
endef

define KernelPackage/i2c-ibm-iic/description
 Kernel module for IIC peripheral found on embedded IBM PPC4xx based systems
endef

$(eval $(call KernelPackage,i2c-ibm-iic))

I2C_MV64XXX_MODULES:=\
  CONFIG_I2C_MV64XXX:drivers/i2c/busses/i2c-mv64xxx

define KernelPackage/i2c-mv64xxx
  $(call i2c_defaults,$(I2C_MV64XXX_MODULES),59)
  TITLE:=Orion Platform I2C interface support
  DEPENDS:=@TARGET_kirkwood||TARGET_orion +kmod-i2c-core
endef

define KernelPackage/i2c-mv64xxx/description
 Kernel module for I2C interface on the Kirkwood, Orion and Armada XP/370
 family processors
endef

$(eval $(call KernelPackage,i2c-mv64xxx))


I2C_TINY_USB_MODULES:= \
  CONFIG_I2C_TINY_USB:drivers/i2c/busses/i2c-tiny-usb

define KernelPackage/i2c-tiny-usb
  $(call i2c_defaults,$(I2C_TINY_USB_MODULES),59)
  TITLE:=I2C Tiny USB adaptor
  DEPENDS:=@USB_SUPPORT kmod-i2c-core +kmod-usb-core
endef

define KernelPackage/i2c-tiny-usb/description
 Kernel module for the I2C Tiny USB adaptor developed
 by Till Harbaum (http://www.harbaum.org/till/i2c_tiny_usb)
endef

$(eval $(call KernelPackage,i2c-tiny-usb))


I2C_PIIX4_MODULES:= \
  CONFIG_I2C_PIIX4:drivers/i2c/busses/i2c-piix4

define KernelPackage/i2c-piix4
  $(call i2c_defaults,$(I2C_PIIX4_MODULES),59)
  TITLE:=Intel PIIX4 and compatible I2C interfaces
  DEPENDS:=@PCI_SUPPORT @(x86||x86_64) kmod-i2c-core
endef

define KernelPackage/i2c-piix4/description
 Support for the Intel PIIX4 family of mainboard I2C interfaces,
 specifically Intel PIIX4, Intel 440MX, ATI IXP200, ATI IXP300,
 ATI IXP400, ATI SB600, ATI SB700/SP5100, ATI SB800, AMD Hudson-2,
 AMD ML, AMD CZ, Serverworks OSB4, Serverworks CSB5,
 Serverworks CSB6, Serverworks HT-1000, Serverworks HT-1100 and
 SMSC Victory66.
endef

$(eval $(call KernelPackage,i2c-piix4))


I2C_MUX_MODULES:= \
  CONFIG_I2C_MUX:drivers/i2c/i2c-mux

define KernelPackage/i2c-mux
  $(call i2c_defaults,$(I2C_MUX_MODULES),51)
  TITLE:=I2C bus multiplexing support
  DEPENDS:=kmod-i2c-core
endef

define KernelPackage/i2c-mux/description
 Kernel modules for I2C bus multiplexing support
endef

$(eval $(call KernelPackage,i2c-mux))

I2C_MUX_GPIO_MODULES:= \
  CONFIG_I2C_MUX_GPIO:drivers/i2c/muxes/i2c-mux-gpio

define KernelPackage/i2c-mux-gpio
  $(call i2c_defaults,$(I2C_MUX_GPIO_MODULES),51)
  TITLE:=GPIO-based I2C mux/switches
  DEPENDS:=kmod-i2c-mux
endef

define KernelPackage/i2c-mux-gpio/description
 Kernel modules for GENERIC_GPIO I2C bus mux/switching devices
endef

$(eval $(call KernelPackage,i2c-mux-gpio))

I2C_MUX_PCA954x_MODULES:= \
  CONFIG_I2C_MUX_PCA954x:drivers/i2c/muxes/i2c-mux-pca954x

define KernelPackage/i2c-mux-pca954x
  $(call i2c_defaults,$(I2C_MUX_PCA954x_MODULES),51)
  TITLE:=Philips PCA954x I2C mux/switches
  DEPENDS:=kmod-i2c-mux
endef

define KernelPackage/i2c-mux-pca954x/description
 Kernel modules for PCA954x I2C bus mux/switching devices
endef

$(eval $(call KernelPackage,i2c-mux-pca954x))


I2C_MUX_PCA9541_MODULES:= \
  CONFIG_I2C_MUX_PCA9541:drivers/i2c/muxes/i2c-mux-pca9541

define KernelPackage/i2c-mux-pca9541
  $(call i2c_defaults,$(I2C_MUX_PCA9541_MODULES),51)
  TITLE:=Philips PCA9541 I2C mux/switches
  DEPENDS:=kmod-i2c-mux
endef

define KernelPackage/i2c-mux-pca9541/description
 Kernel modules for PCA9541 I2C bus mux/switching devices
endef

$(eval $(call KernelPackage,i2c-mux-pca9541))
