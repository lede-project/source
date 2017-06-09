define Device/COMPONENT
  KERNEL_INSTALL := 1
  KERNEL_SUFFIX := -uImage
  UBINIZE_OPTS :=
  IMAGES += root.ubi dtb
  IMAGE/dtb :=
  IMAGE/root.ubi := append-ubi
endef

define Device/OFTREE
  KERNEL := kernel-bin | lzma | uImage lzma
  KERNEL_SIZE := 4096k
  DTB_SIZE := 128k
  IMAGE/dtb := install-dtb
  IMAGE/factory.bin := append-dtb | pad-to $$(DTB_SIZE) \
	  | append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi
endef

define Device/at91sam9263ek
  DEVICE_TITLE := Atmel AT91SAM9263-EK
  $(Device/COMPONENT)
  $(Device/OFTREE)
endef
TARGET_DEVICES += at91sam9263ek

define Device/at91sam9g15ek
  DEVICE_TITLE := Atmel AT91SAM9G15-EK
  $(Device/COMPONENT)
endef
TARGET_DEVICES += at91sam9g15ek

define Device/at91sam9g20ek
  DEVICE_TITLE := Atmel AT91SAM9G20-EK
  $(Device/COMPONENT)
  $(Device/OFTREE)
endef
TARGET_DEVICES += at91sam9g20ek

define Device/at91sam9g20ek_2mmc
  DEVICE_TITLE := Atmel AT91SAM9G20-EK 2MMC
  $(Device/COMPONENT)
  $(Device/OFTREE)
endef
TARGET_DEVICES += at91sam9g20ek_2mmc

define Device/at91sam9g25ek
  DEVICE_TITLE := Atmel AT91SAM9G25-EK
  $(Device/COMPONENT)
endef
TARGET_DEVICES += at91sam9g25ek

define Device/at91sam9g35ek
  DEVICE_TITLE := Atmel AT91SAM9G35-EK
  $(Device/COMPONENT)
endef
TARGET_DEVICES += at91sam9g35ek

define Device/at91sam9m10g45ek
  DEVICE_TITLE := Atmel AT91SAM9M10G45-EK
  $(Device/COMPONENT)
endef
TARGET_DEVICES += at91sam9m10g45ek

define Device/at91sam9x25ek
  DEVICE_TITLE := Atmel AT91SAM9X25-EK
  $(Device/COMPONENT)
endef
TARGET_DEVICES += at91sam9x25ek

define Device/at91sam9x35ek
  DEVICE_TITLE := Atmel AT91SAM9X35-EK
  $(Device/COMPONENT)
endef
TARGET_DEVICES += at91sam9x35ek

define Device/lmu5000
  DEVICE_TITLE := CalAmp LMU5000
  DEVICE_PACKAGES := kmod-rtc-pcf2123 kmod-usb-acm kmod-usb-serial \
    kmod-usb-serial-option kmod-usb-serial-sierrawireless kmod-gpio-mcp23s08
  KERNEL_SIZE := 4096k
  IMAGES := factory.bin
endef
TARGET_DEVICES += lmu5000

define Device/tny_a9260
  DEVICE_TITLE := Calao TNYA9260
  IMAGES := factory.bin
  $(Device/OFTREE)
endef
TARGET_DEVICES += tny_a9260

define Device/tny_a9263
  DEVICE_TITLE := Calao TNYA9263
  IMAGES := factory.bin
  $(Device/OFTREE)
endef
TARGET_DEVICES += tny_a9263

define Device/tny_a9g20
  DEVICE_TITLE := Calao TNYA9G20
  IMAGES := factory.bin
  $(Device/OFTREE)
endef
TARGET_DEVICES += tny_a9g20

define Device/usb_a9260
  DEVICE_TITLE := Calao USBA9260
  IMAGES := factory.bin
  $(Device/OFTREE)
endef
TARGET_DEVICES += usb_a9260

define Device/usb_a9263
  DEVICE_TITLE := Calao USBA9263
  IMAGES := factory.bin
  $(Device/OFTREE)
endef
TARGET_DEVICES += usb_a9263

define Device/usb_a9g20
  DEVICE_TITLE := Calao USBA9G20
  IMAGES := factory.bin
  $(Device/OFTREE)
endef
TARGET_DEVICES += usb_a9g20

define Device/ethernut5
  DEVICE_TITLE := Ethernut 5
  $(Device/COMPONENT)
  UBINIZE_OPTS := -E 5
endef
TARGET_DEVICES += ethernut5

define Device/at91-q5xr5
  DEVICE_TITLE := Exegin Q5XR5
  KERNEL := kernel-bin | lzma | uImage lzma
  KERNEL_SIZE := 2048k
  IMAGES += dtb factory.bin
  IMAGE/dtb := install-dtb
endef
TARGET_DEVICES += at91-q5xr5
