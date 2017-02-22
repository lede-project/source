define Device/mikrotik
  PROFILES := Default
  DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-usb-ledtrig-usbport nand-utils
  BOARD_NAME := routerboard
  KERNEL_INITRAMFS :=
  KERNEL_NAME := loader-generic.elf
  KERNEL := kernel-bin | kernel2minor -s 2048 -e -c
  FILESYSTEMS := squashfs
  IMAGES := sysupgrade.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar
endef

define Device/nand-64m
  DEVICE_TITLE := MikroTik RouterBoard with 64 MB NAND flash
$(Device/mikrotik)
  KERNEL := kernel-bin | kernel2minor -s 512 -e -c
endef

define Device/nand-large
  DEVICE_TITLE := MikroTik RouterBoard with >= 128 MB NAND flash
$(Device/mikrotik)
  KERNEL := kernel-bin | kernel2minor -s 2048 -e -c
endef

TARGET_DEVICES += nand-64m nand-large

define Device/rb-nor-flash-16M
  DEVICE_TITLE := MikroTik RouterBoard with 16 MB SPI NOR flash
  DEVICE_PACKAGES := rbcfg
  BLOCKSIZE := 64k
  IMAGE_SIZE := 16000k
  LOADER_TYPE := elf
  KERNEL_INSTALL := 1
  KERNEL := kernel-bin | lzma | loader-kernel | kernel2minor -s 1024 -e
  KERNEL_INITRAMFS := kernel-bin | lzma | loader-kernel
  IMAGES := sysupgrade.bin
  IMAGE/sysupgrade.bin = append-rootfs | pad-rootfs | combined-image | check-size $$$$(IMAGE_SIZE)
endef

TARGET_DEVICES += rb-nor-flash-16M
