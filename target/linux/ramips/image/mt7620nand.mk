#
# MT7620A Profiles
#

DEVICE_VARS +=

define Device/miwifi-r3
  DTS := MIWIFI-R3
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 1440k
  KERNEL := $(KERNEL_DTB) | uImage lzma
  IMAGE_SIZE := 32768k
  UBINIZE_OPTS := -E 5
  IMAGES := sysupgrade.tar kernel1.bin rootfs0.bin
  IMAGE/kernel1.bin := append-kernel | check-size $$$$(KERNEL_SIZE)
  IMAGE/rootfs0.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.tar := sysupgrade-tar | append-metadata
  DEVICE_TITLE := Xiaomi Mi Router R3
  SUPPORTED_DEVICES += miwifi-r3
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci uboot-envtools
endef
TARGET_DEVICES += miwifi-r3
