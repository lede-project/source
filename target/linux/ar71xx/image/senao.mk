define Build/senao-factory-image
	$(eval board=$(word 1,$(1)))
	$(eval rootfs=$(word 2,$(1)))

	mkdir -p $@.senao

	touch $@.senao/FWINFO-OpenWrt-$(REVISION)-$(board)
	$(CP) $(IMAGE_KERNEL) $@.senao/openwrt-senao-$(board)-uImage-lzma.bin
	$(CP) $(rootfs) $@.senao/openwrt-senao-$(board)-root.squashfs

	$(TAR) -c \
		--numeric-owner --owner=0 --group=0 --sort=name \
		$(if $(SOURCE_DATE_EPOCH),--mtime="@$(SOURCE_DATE_EPOCH)") \
		-C $@.senao . | gzip -9nc > $@

	rm -rf $@.senao
endef

define Device/ens200
  DEVICE_TITLE := EnGenius ENS200
  BOARDNAME := ENS200
  DEVICE_PACKAGES := rssileds
  IMAGE_SIZE := 5952k
  IMAGES += factory.bin
  KERNEL_INITRAMFS := kernel-bin | patch-cmdline | lzma | uImage lzma
  MTDPARTS := spi0.0:256k(u-boot)ro,64k(u-boot-env),320k(custom)ro,1024k(kernel),4928k(rootfs),1536k(failsafe)ro,64k(art)ro,5952k@0xa0000(firmware)
  IMAGE/factory.bin/squashfs := append-rootfs | pad-rootfs | senao-factory-image ens200 $$$$@
  IMAGE/sysupgrade.bin := append-kernel | pad-to 64k | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += ens200

define Device/ens202ext
  DEVICE_TITLE := EnGenius ENS202EXT
  BOARDNAME := ENS202EXT
  DEVICE_PACKAGES := rssileds
  KERNEL_SIZE := 1536k
  IMAGE_SIZE := 13632k
  IMAGES += factory.bin
  MTDPARTS := spi0.0:256k(u-boot)ro,64k(u-boot-env),320k(custom)ro,1536k(kernel),12096k(rootfs),2048k(failsafe)ro,64k(art)ro,13632k@0xa0000(firmware)
  IMAGE/factory.bin/squashfs := append-rootfs | pad-rootfs | senao-factory-image ens202ext $$$$@
  IMAGE/sysupgrade.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += ens202ext
