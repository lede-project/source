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
  KERNEL_SIZE := 6144k
  DTB_SIZE := 512k
  IMAGE/dtb := install-dtb
  IMAGE/factory.bin := append-dtb | pad-to $$(DTB_SIZE) \
	  | append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi
endef

define Device/at91-sama5d3_xplained
  DEVICE_TITLE := Atmel AT91SAMA5D3XPLAINED
  $(Device/COMPONENT)
  $(Device/OFTREE)
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  SUBPAGESIZE := 2048
  MKUBIFS_OPTS := -m $$(PAGESIZE) -e 124KiB -c 2048
endef
TARGET_DEVICES += at91-sama5d3_xplained

