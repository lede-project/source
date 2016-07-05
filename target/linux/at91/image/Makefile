#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

UBIFS_OPTS = -F -m 2048 -e 126KiB -c 2048 -U
UBI_OPTS = -m 2048 -p 128KiB -s 512

AT91SAMA5D3XPLAINED_UBIFS_OPTS = -m 2048 -e 124KiB -c 2048
AT91SAMA5D3XPLAINED_UBI_OPTS = -m 2048 -p 128KiB -s 2048

DEVICE_VARS += KERNEL_PREFIX DTS

KERNEL_LOADADDR := 0x20008000

define Device/Default
  PROFILES = Default $$(DEVICE_NAME)
  KERNEL_NAME := zImage2
  KERNEL_INITRAMFS_NAME := vmlinux-initramfs
  KERNEL := append-dtb 
  DEVICE_TITLE := $$(DEVICE_NAME)
  FILESYSTEMS := squashfs
  IMAGES := uImage
  IMAGE/uImage := append-kernel | uImage none 
endef

define Build/append-dtb
	cat $(KDIR)/zImage $(DTS_DIR)/$(DTS).dtb > $@
endef

define MkOftree
	cp -u $(KDIR)/zImage $(BIN_DIR)/$(IMG_PREFIX)-zImage
	cp $(DTS_DIR)/$(2).dtb $(BIN_DIR)/$(IMG_PREFIX)-$(1)-oftree.dtb
endef

ifeq ($(SUBTARGET),legacy)
# Atmel
define Device/AT91SAM9263EK
  DTS := at91sam9263ek
endef
TARGET_DEVICES += AT91SAM9263EK

define Device/AT91SAM9G15EK
  DTS := at91sam9g15ek
endef
TARGET_DEVICES += AT91SAM9G15EK

define Device/AT91SAM9G20EK
  DTS := at91sam9g20ek
endef
TARGET_DEVICES += AT91SAM9G20EK

define Device/AT91SAM9G20EK-2MMC
  DTS := at91sam9g20ek_2mmc
endef
TARGET_DEVICES += AT91SAM9G20EK-2MMC

define Device/AT91SAM9G25EK
  DTS := at91sam9g25ek
endef
TARGET_DEVICES += AT91SAM9G25EK

define Device/AT91SAM9G35EK
  DTS := at91sam9g35ek
endef
TARGET_DEVICES += AT91SAM9G35EK

define Device/AT91SAM9M10G45EK
  DTS := at91sam9m10g45ek
endef
TARGET_DEVICES += AT91SAM9M10G45EK

define Device/AT91SAM9X25EK
  DTS := at91sam9x25ek
endef
TARGET_DEVICES += AT91SAM9X25EK

define Device/AT91SAM9X35EK
  DTS := at91sam9x35ek
endef
TARGET_DEVICES += AT91SAM9X35EK

# CalAmp
define Device/LMU5000
  DTS := lmu5000
endef
TARGET_DEVICES += LMU5000

# Calao
define Device/TNYA9260
  DTS := tny_a9260
endef
TARGET_DEVICES += TNYA9260

define Device/TNYA9263
  DTS := tny_a9263
endef
TARGET_DEVICES += TNYA9263

define Device/TNYA9G20
  DTS := tny_a9g20
endef
TARGET_DEVICES += TNYA9G20

define Device/USBA9260
  DTS := usb_a9260
endef
TARGET_DEVICES += USBA9260

define Device/USBA9263
  DTS := usb_a9263
endef
TARGET_DEVICES += USBA9263

define Device/USBA9G20
  DTS := usb_a9g20
endef
TARGET_DEVICES += USBA9G20

# Ethernut
define Device/ETHERNUT5
  DTS := ethernut5
endef
TARGET_DEVICES += ETHERNUT5

# Exegin
define Device/Q5XR5
  DTS := at91-q5xr5
endef
TARGET_DEVICES += Q5XR5

endif

ifeq ($(SUBTARGET),sama5d3)

define Device/AT91SAMA5D3XPLAINED
  DTS := at91-sama5d3_xplained
endef
TARGET_DEVICES += AT91SAMA5D3XPLAINED

endif


define Image/BuildKernel
	mkimage -A arm -T kernel -C none -a 0x20008000 -e 0x20008000 -n linux-2.6 \
		-d $(KDIR)/Image $(BIN_DIR)/$(IMG_PREFIX)-uImage
	if [ $(CONFIG_FLEXIBITY_ROOT) ]; then \
		$(INSTALL_BIN) $(BIN_DIR)/$(IMG_PREFIX)-uImage $(TARGET_DIR)/uImage ; \
	fi
endef

define Image/Prepare
	cp $(LINUX_DIR)/arch/arm/boot/Image $(KDIR)/Image
	cp $(LINUX_DIR)/arch/arm/boot/zImage $(KDIR)/zImage
endef

define Image/Build
	$(call Image/Build/$(1))
	cp $(KDIR)/root.$(1) $(BIN_DIR)/$(IMG_PREFIX)-root.$(1)
endef

$(eval $(call BuildImage))
