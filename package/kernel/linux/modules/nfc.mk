define KernelPackage/nfc
  SUBMENU:=NFC
  DEFAULT:=m
  TITLE:=NFC Generic driver
  DEPENDS:=@TARGET_ar71xx
  KCONFIG:=CONFIG_NFC
  FILES:=$(LINUX_DIR)/net/nfc/nfc.ko
  AUTOLOAD:=$(call AutoLoad,95,nfc)
endef

define KernelPackage/nfc/description
 	This package contains the NFC Generic driver
endef

$(eval $(call KernelPackage,nfc))

define KernelPackage/nfc-digital
  SUBMENU:=NFC
  DEFAULT:=m
  TITLE:=NFC Digital driver
  DEPENDS:=@TARGET_ar71xx +kmod-nfc +kmod-lib-crc-ccitt +kmod-lib-crc-itu-t
  KCONFIG:=CONFIG_NFC_DIGITAL
  FILES:=$(LINUX_DIR)/net/nfc/nfc_digital.ko
  AUTOLOAD:=$(call AutoLoad,95,nfc-digital)
endef

define KernelPackage/nfc-digital/description
 	This package contains the NFC Digital driver
endef

$(eval $(call KernelPackage,nfc-digital))

define KernelPackage/nfc-digital-uart
  SUBMENU:=NFC
  DEFAULT:=m
  TITLE:=NFC Digital UART driver
  DEPENDS:=@TARGET_ar71xx +kmod-nfc
  KCONFIG:=CONFIG_NFC_DIGITAL_UART
  FILES:=$(LINUX_DIR)/net/nfc/nfc_digital_uart.ko
  AUTOLOAD:=$(call AutoLoad,95,nfc-digital-uart)
endef

define KernelPackage/nfc-digital-uart/description
 	This package contains the NFC Digital UART driver
endef

$(eval $(call KernelPackage,nfc-digital-uart))

define KernelPackage/nfcst
  SUBMENU:=NFC
  DEFAULT:=m
  TITLE:=ST NFC driver
  DEPENDS:=@TARGET_ar71xx +kmod-nfc-digital
  KCONFIG:=CONFIG_NFC_ST
  FILES:=$(LINUX_DIR)/drivers/nfc/nfcst/nfcst.ko
  AUTOLOAD:=$(call AutoLoad,95,nfcst)
endef

define KernelPackage/nfcst/description
 	This package contains the NFC ST driver
endef

$(eval $(call KernelPackage,nfcst))

define KernelPackage/nfcst-uart
  SUBMENU:=NFC
  DEFAULT:=m
  TITLE:=ST NFC UART driver
  DEPENDS:=@TARGET_ar71xx +kmod-nfc-digital-uart +kmod-nfcst
  KCONFIG:=CONFIG_NFC_ST_UART
  FILES:=$(LINUX_DIR)/drivers/nfc/nfcst/nfcst_uart.ko
  AUTOLOAD:=$(call AutoLoad,95,nfcst-uart)
endef

define KernelPackage/nfcst-uart/description
 	This package contains the NFC ST UART driver
endef

$(eval $(call KernelPackage,nfcst-uart))
