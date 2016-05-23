DEVICE_VARS += TPLINK_HWID TPLINK_HWREV TPLINK_FLASHLAYOUT TPLINK_HEADER_VERSION TPLINK_BOARD_NAME

# combine kernel and rootfs into one image
# mktplinkfw <type> <optional extra arguments to mktplinkfw binary>
# <type> is "sysupgrade" or "factory"
#
# -a align the rootfs start on an <align> bytes boundary
# -j add jffs2 end-of-filesystem markers
# -s strip padding from end of the image
# -X reserve <size> bytes in the firmware image (hexval prefixed with 0x)
define Build/mktplinkfw
	-$(STAGING_DIR_HOST)/bin/mktplinkfw \
		-H $(TPLINK_HWID) -W $(TPLINK_HWREV) -F $(TPLINK_FLASHLAYOUT) -N OpenWrt -V $(REVISION) \
		-m $(TPLINK_HEADER_VERSION) \
		-k $(word 1,$^) \
		-r $@ \
		-o $@.new \
		-j -X 0x40000 \
		-a $(call rootfs_align,$(FILESYSTEM)) \
		$(wordlist 2,$(words $(1)),$(1)) \
		$(if $(findstring sysupgrade,$(word 1,$(1))),-s) && mv $@.new $@ || rm -f $@
endef

# mktplinkfw-initramfs <optional extra arguments to mktplinkfw binary>
#
# -c combined image
define Build/mktplinkfw-initramfs
	$(STAGING_DIR_HOST)/bin/mktplinkfw \
		-H $(TPLINK_HWID) -W $(TPLINK_HWREV) -F $(TPLINK_FLASHLAYOUT) -N OpenWrt -V $(REVISION) $(1) \
		-m $(TPLINK_HEADER_VERSION) \
		-k $@ \
		-o $@.new \
		-s -S \
		-c
	@mv $@.new $@
endef

define Build/tplink-safeloader
       -$(STAGING_DIR_HOST)/bin/tplink-safeloader \
		-B $(TPLINK_BOARD_NAME) \
		-V $(REVISION) \
		-k $(word 1,$^) \
		-r $@ \
		-o $@.new \
		-j \
		$(wordlist 2,$(words $(1)),$(1)) \
		$(if $(findstring sysupgrade,$(word 1,$(1))),-S) && mv $@.new $@ || rm -f $@
endef

define Device/tplink
  TPLINK_HWREV := 0x1
  TPLINK_HEADER_VERSION := 1
  LOADER_TYPE := gz
  KERNEL := kernel-bin | patch-cmdline | lzma
  KERNEL_INITRAMFS := kernel-bin | patch-cmdline | lzma | mktplinkfw-initramfs
  IMAGES := sysupgrade.bin factory.bin
  IMAGE/sysupgrade.bin := append-rootfs | mktplinkfw sysupgrade
  IMAGE/factory.bin := append-rootfs | mktplinkfw factory
endef

define Device/tplink-nolzma
$(Device/tplink)
  LOADER_FLASH_OFFS := 0x22000
  COMPILE := loader-$(1).gz
  COMPILE/loader-$(1).gz := loader-okli-compile
  KERNEL := copy-file $(KDIR)/vmlinux.bin.lzma | uImage lzma -M 0x4f4b4c49 | loader-okli $(1)
  KERNEL_INITRAMFS := copy-file $(KDIR)/vmlinux-initramfs.bin.lzma | loader-kernel-cmdline | mktplinkfw-initramfs
endef

define Device/tplink-4m
$(Device/tplink-nolzma)
  TPLINK_FLASHLAYOUT := 4M
  IMAGE_SIZE := 3904k
endef

define Device/tplink-8m
$(Device/tplink-nolzma)
  TPLINK_FLASHLAYOUT := 8M
  IMAGE_SIZE := 7936k
endef

define Device/tplink-4mlzma
$(Device/tplink)
  TPLINK_FLASHLAYOUT := 4Mlzma
  IMAGE_SIZE := 3904k
endef

define Device/tplink-8mlzma
$(Device/tplink)
  TPLINK_FLASHLAYOUT := 8Mlzma
  IMAGE_SIZE := 7936k
endef

define Device/tplink-16mlzma
$(Device/tplink)
  TPLINK_FLASHLAYOUT := 16Mlzma
  IMAGE_SIZE := 15872k
endef

define Device/cpe210-220
  MTDPARTS := spi0.0:128k(u-boot)ro,64k(pation-table)ro,64k(product-info)ro,1536k(kernel),6144k(rootfs),192k(config)ro,64k(ART)ro,7680k@0x40000(firmware)
  IMAGE_SIZE := 7680k
  BOARDNAME := CPE210
  TPLINK_BOARD_NAME := CPE510
  DEVICE_PROFILE := CPE510
  LOADER_TYPE := elf
  KERNEL := kernel-bin | patch-cmdline | lzma | loader-kernel
  IMAGES := sysupgrade.bin factory.bin
  IMAGE/sysupgrade.bin := append-rootfs | tplink-safeloader sysupgrade
  IMAGE/factory.bin := append-rootfs | tplink-safeloader factory
endef

define Device/cpe510-520
$(Device/cpe210-220)
  BOARDNAME := CPE510
endef
TARGET_DEVICES += cpe210-220 cpe510-520

define Device/tl-wdr4300-v1
$(Device/tplink-8mlzma)
  BOARDNAME = TL-WDR4300
  DEVICE_PROFILE = TLWDR4300
  TPLINK_HWID := 0x43000001
endef

define Device/tl-wdr3500-v1
$(Device/tl-wdr4300-v1)
  BOARDNAME = TL-WDR3500
  TPLINK_HWID := 0x35000001
endef

define Device/tl-wdr3600-v1
$(Device/tl-wdr4300-v1)
  TPLINK_HWID := 0x36000001
endef

define Device/tl-wdr4300-v1-il
$(Device/tl-wdr4300-v1)
  TPLINK_HWID := 0x43008001
endef

define Device/tl-wdr4310-v1
$(Device/tl-wdr4300-v1)
  TPLINK_HWID := 0x43100001
endef

define Device/mw4530r-v1
$(Device/tl-wdr4300-v1)
  TPLINK_HWID := 0x45300001
endef
TARGET_DEVICES += tl-wdr3500-v1 tl-wdr3600-v1 tl-wdr4300-v1 tl-wdr4300-v1-il tl-wdr4310-v1 mw4530r-v1

define Device/tl-wdr6500-v2
$(Device/tplink-8mlzma)
  KERNEL := kernel-bin | patch-cmdline | lzma | uImage lzma
  KERNEL_INITRAMFS := kernel-bin | patch-cmdline | lzma | uImage lzma | mktplinkfw-initramfs
  BOARDNAME = TL-WDR6500-v2
  DEVICE_PROFILE = TLWDR6500V2
  TPLINK_HWID := 0x65000002
  TPLINK_HEADER_VERSION := 2
endef
TARGET_DEVICES += tl-wdr6500-v2

define Device/tl-wdr3320-v2
$(Device/tplink-4mlzma)
  BOARDNAME = TL-WDR3320-v2
  DEVICE_PROFILE = TLWDR3320V2
  TPLINK_HWID := 0x33200002
  TPLINK_HEADER_VERSION := 2
endef
TARGET_DEVICES += tl-wdr3320-v2

define Device/archer-c5-v1
    $(Device/tplink-16mlzma)
    BOARDNAME := ARCHER-C5
    DEVICE_PROFILE := ARCHERC7
    TPLINK_HWID := 0xc5000001
endef

define Device/archer-c7-v1
    $(Device/tplink-8mlzma)
    BOARDNAME := ARCHER-C7
    DEVICE_PROFILE := ARCHERC7
    TPLINK_HWID := 0x75000001
endef

define Device/archer-c7-v2
    $(Device/tplink-16mlzma)
    BOARDNAME := ARCHER-C7-V2
    DEVICE_PROFILE := ARCHERC7
    TPLINK_HWID := 0xc7000002
    IMAGE/factory.bin := append-rootfs | mktplinkfw factory -C US
endef

define Device/tl-wdr7500-v3
    $(Device/tplink-8mlzma)
    BOARDNAME := ARCHER-C7
    DEVICE_PROFILE := ARCHERC7
    TPLINK_HWID := 0x75000003
endef
TARGET_DEVICES += archer-c5-v1 archer-c7-v1 archer-c7-v2 tl-wdr7500-v3

define Device/tl-mr10u-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR10U
    DEVICE_PROFILE := TLMR10U
    TPLINK_HWID := 0x00100101
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr11u-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR11U
    DEVICE_PROFILE := TLMR11U
    TPLINK_HWID := 0x00110101
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr11u-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR11U
    DEVICE_PROFILE := TLMR11U
    TPLINK_HWID := 0x00110102
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr12u-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR13U
    DEVICE_PROFILE := TLMR12U
    TPLINK_HWID := 0x00120101
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr13u-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR13U
    DEVICE_PROFILE := TLMR13U
    TPLINK_HWID := 0x00130101
    CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += tl-mr10u-v1 tl-mr11u-v1 tl-mr11u-v2 tl-mr12u-v1 tl-mr13u-v1

define Device/tl-mr3020-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR3020
    DEVICE_PROFILE := TLMR3020
    TPLINK_HWID := 0x30200001
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr3040-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR3040
    DEVICE_PROFILE := TLMR3040
    TPLINK_HWID := 0x30400001
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr3040-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR3040-v2
    DEVICE_PROFILE := TLMR3040
    TPLINK_HWID := 0x30400002
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr3220-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-MR3220
    DEVICE_PROFILE := TLMR3220
    TPLINK_HWID := 0x32200001
endef

define Device/tl-mr3220-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR3220-v2
    DEVICE_PROFILE := TLMR3220
    TPLINK_HWID := 0x32200002
    CONSOLE := ttyATH0,115200
endef

define Device/tl-mr3420-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-MR3420
    DEVICE_PROFILE := TLMR3420
    TPLINK_HWID := 0x34200001
endef

define Device/tl-mr3420-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-MR3420-v2
    DEVICE_PROFILE := TLMR3420
    TPLINK_HWID := 0x34200002
endef
TARGET_DEVICES += tl-mr3020-v1 tl-mr3040-v1 tl-mr3040-v2 tl-mr3220-v1 tl-mr3220-v2 tl-mr3420-v1 tl-mr3420-v2

define Device/tl-wr703n-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR703N
    DEVICE_PROFILE := TLWR703
    TPLINK_HWID := 0x07030101
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr710n-v1
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR710N
    DEVICE_PROFILE := TLWR710
    TPLINK_HWID := 0x07100001
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr710n-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR710N
    DEVICE_PROFILE := TLWR710
    TPLINK_HWID := 0x07100002
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr710n-v2.1
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR710N
    DEVICE_PROFILE := TLWR710
    TPLINK_HWID := 0x07100002
    TPLINK_HWREV := 0x00000002
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr720n-v3
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR720N-v3
    DEVICE_PROFILE := TLWR720
    TPLINK_HWID := 0x07200103
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr720n-v4
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR720N-v3
    DEVICE_PROFILE := TLWR720
    TPLINK_HWID := 0x07200104
    CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += tl-wr703n-v1 tl-wr710n-v1 tl-wr710n-v2 tl-wr710n-v2.1 tl-wr720n-v3 tl-wr720n-v4

define Device/tl-wr740n-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR740
    TPLINK_HWID := 0x07400001
endef

define Device/tl-wr740n-v3
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR740
    TPLINK_HWID := 0x07400003
endef

define Device/tl-wr740n-v4
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR741ND-v4
    DEVICE_PROFILE := TLWR740
    TPLINK_HWID := 0x07400004
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr740n-v5
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR741ND-v4
    DEVICE_PROFILE := TLWR740
    TPLINK_HWID := 0x07400005
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr740n-v6
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR841N-v9
    DEVICE_PROFILE := TLWR740
    TPLINK_HWID := 0x07400006
endef

define Device/tl-wr741nd-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR741
    TPLINK_HWID := 0x07410001
endef

define Device/tl-wr741nd-v2
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR741
    TPLINK_HWID := 0x07410001
endef

define Device/tl-wr741nd-v4
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR741ND-v4
    DEVICE_PROFILE := TLWR741
    TPLINK_HWID := 0x07410004
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr741nd-v5
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR741ND-v4
    DEVICE_PROFILE := TLWR741
    TPLINK_HWID := 0x07400005
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wr810n
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR810N
    DEVICE_PROFILE := TLWR810
    TPLINK_HWID := 0x08100001
endef
TARGET_DEVICES += tl-wr810n

define Device/tl-wr743nd-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR743
    TPLINK_HWID := 0x07430001
endef

define Device/tl-wr743nd-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR741ND-v4
    DEVICE_PROFILE := TLWR743
    TPLINK_HWID := 0x07430002
    CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += tl-wr740n-v1 tl-wr740n-v3 tl-wr740n-v4 tl-wr740n-v5 tl-wr740n-v6 tl-wr741nd-v1 tl-wr741nd-v2 tl-wr741nd-v4 tl-wr741nd-v5 tl-wr743nd-v1 tl-wr743nd-v2

define Device/tl-wr841-v1.5
    $(Device/tplink-4m)
    BOARDNAME := TL-WR841N-v1.5
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410002
    TPLINK_HWREV := 2
endef

define Device/tl-wr841-v3
    $(Device/tplink-4m)
    BOARDNAME := TL-WR941ND
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410003
    TPLINK_HWREV := 3
endef

define Device/tl-wr841-v5
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410005
endef

define Device/tl-wr841-v7
    $(Device/tplink-4m)
    BOARDNAME := TL-WR841N-v7
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410007
endef

define Device/tl-wr841-v8
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR841N-v8
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410008
endef

define Device/tl-wr841-v9
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR841N-v9
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410009
endef

define Device/tl-wr841-v10
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR841N-v9
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08410010
endef

define Device/tl-wr841-v11
   $(Device/tplink-4mlzma)
   BOARDNAME := TL-WR841N-v9
   DEVICE_PROFILE := TLWR841
   TPLINK_HWID := 0x08410011
endef

define Device/tl-wr842n-v1
    $(Device/tplink-8m)
    BOARDNAME := TL-MR3420
    DEVICE_PROFILE := TLWR842
    TPLINK_HWID := 0x08420001
endef

define Device/tl-wr842n-v2
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR842N-v2
    DEVICE_PROFILE := TLWR842
    TPLINK_HWID := 0x8420002
endef

define Device/tl-wr842n-v3
    $(Device/tplink-16mlzma)
    BOARDNAME := TL-WR842N-v3
    DEVICE_PROFILE := TLWR842
    TPLINK_HWID := 0x08420003
endef

define Device/tl-wr843nd-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR841N-v8
    DEVICE_PROFILE := TLWR843
    TPLINK_HWID := 0x08430001
endef

define Device/tl-wr847n-v8
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR841N-v8
    DEVICE_PROFILE := TLWR841
    TPLINK_HWID := 0x08470008
endef
TARGET_DEVICES += tl-wr841-v1.5 tl-wr841-v3 tl-wr841-v5 tl-wr841-v7 tl-wr841-v8 tl-wr841-v9 tl-wr841-v10 tl-wr841-v11 tl-wr842n-v1 tl-wr842n-v2 tl-wr842n-v3 tl-wr843nd-v1 tl-wr847n-v8

define Device/tl-wr941nd-v2
    $(Device/tplink-4m)
    BOARDNAME := TL-WR941ND
    DEVICE_PROFILE := TLWR941
    TPLINK_HWID := 0x09410002
    TPLINK_HWREV := 2
endef

define Device/tl-wr941nd-v3
    $(Device/tplink-4m)
    BOARDNAME := TL-WR941ND
    DEVICE_PROFILE := TLWR941
    TPLINK_HWID := 0x09410002
    TPLINK_HWREV := 2
endef

define Device/tl-wr941nd-v4
    $(Device/tplink-4m)
    BOARDNAME := TL-WR741ND
    DEVICE_PROFILE := TLWR941
    TPLINK_HWID := 0x09410004
endef

define Device/tl-wr941nd-v5
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR941ND-v5
    DEVICE_PROFILE := TLWR941
    TPLINK_HWID := 0x09410005
endef

define Device/tl-wr941nd-v6
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR941ND-v6
    DEVICE_PROFILE := TLWR941
    TPLINK_HWID := 0x09410006
endef

# Chinese version (unlike European) is similar to the TL-WDR3500
define Device/tl-wr941nd-v6-cn
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WDR3500
    DEVICE_PROFILE := TLWR941
    TPLINK_HWID := 0x09410006
endef
TARGET_DEVICES += tl-wr941nd-v2 tl-wr941nd-v3 tl-wr941nd-v4 tl-wr941nd-v5 tl-wr941nd-v6 tl-wr941nd-v6-cn

define Device/tl-wr1041n-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WR1041N-v2
    DEVICE_PROFILE := TLWR1041
    TPLINK_HWID := 0x10410002
endef
TARGET_DEVICES += tl-wr1041n-v2

define Device/tl-wr1043nd-v1
    $(Device/tplink-8m)
    BOARDNAME := TL-WR1043ND
    DEVICE_PROFILE := TLWR1043
    TPLINK_HWID := 0x10430001
endef

define Device/tl-wr1043nd-v2
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR1043ND-v2
    DEVICE_PROFILE := TLWR1043
    TPLINK_HWID := 0x10430002
endef

define Device/tl-wr1043nd-v3
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR1043ND-v2
    DEVICE_PROFILE := TLWR1043
    TPLINK_HWID := 0x10430003
endef
TARGET_DEVICES += tl-wr1043nd-v1 tl-wr1043nd-v2 tl-wr1043nd-v3

define Device/tl-wr2543-v1
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WR2543N
    DEVICE_PROFILE := TLWR2543
    TPLINK_HWID := 0x25430001
    IMAGE/sysupgrade.bin := append-rootfs | mktplinkfw sysupgrade -v 3.13.99
    IMAGE/factory.bin := append-rootfs | mktplinkfw factory -v 3.13.99
endef
TARGET_DEVICES += tl-wr2543-v1

define Device/tl-wdr4900-v2
    $(Device/tplink-8mlzma)
    BOARDNAME := TL-WDR4900-v2
    DEVICE_PROFILE := TLWDR4900V2
    TPLINK_HWID := 0x49000002
endef
TARGET_DEVICES += tl-wdr4900-v2

define Device/tl-wa701nd-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WA901ND
    DEVICE_PROFILE := TLWA701
    TPLINK_HWID := 0x07010001
endef

define Device/tl-wa701nd-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA701ND-v2
    DEVICE_PROFILE := TLWA701
    TPLINK_HWID := 0x07010002
    CONSOLE := ttyATH0,115200
endef

define Device/tl-wa730re-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WA901ND
    DEVICE_PROFILE := TLWA730RE
    TPLINK_HWID := 0x07300001
endef

define Device/tl-wa750re-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA750RE
    DEVICE_PROFILE := TLWA750
    TPLINK_HWID := 0x07500001
endef

define Device/tl-wa7510n
    $(Device/tplink-4m)
    BOARDNAME := TL-WA7510N
    DEVICE_PROFILE := TLWA7510
    TPLINK_HWID := 0x75100001
endef
TARGET_DEVICES += tl-wa701nd-v1 tl-wa701nd-v2 tl-wa730re-v1 tl-wa750re-v1 tl-wa7510n

define Device/tl-wa801nd-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WA901ND
    DEVICE_PROFILE := TLWA801
    TPLINK_HWID := 0x08010001
endef

define Device/tl-wa801nd-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA801ND-v2
    DEVICE_PROFILE := TLWA801
    TPLINK_HWID := 0x08010002
endef

define Device/tl-wa801nd-v3
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA801ND-v3
    DEVICE_PROFILE := TLWA801
    TPLINK_HWID := 0x08010003
endef

define Device/tl-wa830re-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WA901ND
    DEVICE_PROFILE := TLWA830
    TPLINK_HWID := 0x08300010
endef

define Device/tl-wa830re-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA830RE-v2
    DEVICE_PROFILE := TLWA830
    TPLINK_HWID := 0x08300002
endef

define Device/tl-wa850re-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA850RE
    DEVICE_PROFILE := TLWA850
    TPLINK_HWID := 0x08500001
endef

define Device/tl-wa860re-v1
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA860RE
    DEVICE_PROFILE := TLWA860
    TPLINK_HWID := 0x08600001
endef
TARGET_DEVICES += tl-wa801nd-v1 tl-wa801nd-v2 tl-wa801nd-v3 tl-wa830re-v1 tl-wa830re-v2 tl-wa850re-v1 tl-wa860re-v1

define Device/tl-wa901nd-v1
    $(Device/tplink-4m)
    BOARDNAME := TL-WA901ND
    DEVICE_PROFILE := TLWA901
    TPLINK_HWID := 0x09010001
endef

define Device/tl-wa901nd-v2
    $(Device/tplink-4m)
    BOARDNAME := TL-WA901ND-v2
    DEVICE_PROFILE := TLWA901
    TPLINK_HWID := 0x09010002
endef

define Device/tl-wa901nd-v3
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA901ND-v3
    DEVICE_PROFILE := TLWA901
    TPLINK_HWID := 0x09010003
endef

define Device/tl-wa901nd-v4
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA901ND-v4
    DEVICE_PROFILE := TLWA901
    TPLINK_HWID := 0x09010004
endef

TARGET_DEVICES += tl-wa901nd-v1 tl-wa901nd-v2 tl-wa901nd-v3 tl-wa901nd-v4

define Device/tl-wa7210n-v2
    $(Device/tplink-4mlzma)
    BOARDNAME := TL-WA7210N-v2
    DEVICE_PROFILE := TLWA7210
    TPLINK_HWID := 0x72100002
    CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += tl-wa7210n-v2

