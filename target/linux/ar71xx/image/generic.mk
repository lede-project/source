define Device/bsb
  DEVICE_TITLE := Smart Electronics Black Swift board
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = BSB
  IMAGE_SIZE = 16000k
  CONSOLE = ttyATH0,115200
  MTDPARTS = spi0.0:128k(u-boot)ro,64k(u-boot-env)ro,16128k(firmware),64k(art)ro
endef
TARGET_DEVICES += bsb

define Device/carambola2
  DEVICE_TITLE := Carambola2 board from 8Devices
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = CARAMBOLA2
  IMAGE_SIZE = 16000k
  CONSOLE = ttyATH0,115200
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,16000k(firmware),64k(art)ro
endef
TARGET_DEVICES += carambola2

define Device/cf-e316n-v2
  DEVICE_TITLE := COMFAST CF-E316N v2
  BOARDNAME = CF-E316N-V2
  IMAGE_SIZE = 16192k
  CONSOLE = ttyS0,115200
  MTDPARTS = spi0.0:64k(u-boot)ro,64k(art)ro,16192k(firmware),64k(nvram)ro
endef
TARGET_DEVICES += cf-e316n-v2

define Device/weio
  DEVICE_TITLE := WeIO
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = WEIO
  IMAGE_SIZE = 16000k
  CONSOLE = ttyATH0,115200
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,16000k(firmware),64k(art)ro
endef
TARGET_DEVICES += weio

define Device/gl-ar150
  DEVICE_TITLE := GL AR150
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = GL-AR150
  IMAGE_SIZE = 16000k
  CONSOLE = ttyATH0,115200
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,16000k(firmware),64k(art)ro
endef
TARGET_DEVICES += gl-ar150

define Device/gl-ar300
  DEVICE_TITLE := GL AR300
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = GL-AR300
  IMAGE_SIZE = 16000k
  CONSOLE = ttyS0,115200
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,16000k(firmware),64k(art)ro
endef
TARGET_DEVICES += gl-ar300

define Device/gl-domino
  DEVICE_TITLE := GL Domino Pi
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = DOMINO
  IMAGE_SIZE = 16000k
  CONSOLE = ttyATH0,115200
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,16000k(firmware),64k(art)ro
endef
TARGET_DEVICES += gl-domino

define Device/wndr3700
  DEVICE_TITLE := NETGEAR WNDR3700
  DEVICE_PACKAGES := kmod-usb-core kmod-usb-ohci kmod-usb2 kmod-ledtrig-usbdev kmod-leds-wndr3700-usb
  BOARDNAME = WNDR3700
  NETGEAR_KERNEL_MAGIC = 0x33373030
  NETGEAR_BOARD_ID = WNDR3700
  IMAGE_SIZE = 7680k
  MTDPARTS = spi0.0:320k(u-boot)ro,128k(u-boot-env)ro,7680k(firmware),64k(art)ro
  IMAGES := sysupgrade.bin factory.img factory-NA.img
  KERNEL := kernel-bin | patch-cmdline | lzma -d20 | netgear-uImage lzma
  IMAGE/default = append-kernel $$$$(BLOCKSIZE) | netgear-squashfs | append-rootfs | pad-rootfs
  IMAGE/sysupgrade.bin = $$(IMAGE/default) | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.img = $$(IMAGE/default) | netgear-dni | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory-NA.img = $$(IMAGE/default) | netgear-dni NA | check-size $$$$(IMAGE_SIZE)
endef

define Device/wndr3700v2
$(Device/wndr3700)
  DEVICE_TITLE := NETGEAR WNDR3700 v2
  NETGEAR_BOARD_ID = WNDR3700v2
  NETGEAR_KERNEL_MAGIC = 0x33373031
  NETGEAR_HW_ID = 29763654+16+64
  IMAGE_SIZE = 15872k
  MTDPARTS = spi0.0:320k(u-boot)ro,128k(u-boot-env)ro,15872k(firmware),64k(art)ro
  IMAGES := sysupgrade.bin factory.img
endef

define Device/wndr3800
$(Device/wndr3700v2)
  DEVICE_TITLE := NETGEAR WNDR3800
  NETGEAR_BOARD_ID = WNDR3800
  NETGEAR_HW_ID = 29763654+16+128
endef

define Device/wndr3800ch
$(Device/wndr3800)
  DEVICE_TITLE := NETGEAR WNDR3800 (Ch)
  NETGEAR_BOARD_ID = WNDR3800CH
endef

define Device/wndrmac
$(Device/wndr3700v2)
  DEVICE_TITLE := NETGEAR WNDRMAC
  NETGEAR_BOARD_ID = WNDRMAC
endef

define Device/wndrmacv2
$(Device/wndr3800)
  DEVICE_TITLE := NETGEAR WNDRMAC v2
  NETGEAR_BOARD_ID = WNDRMACv2
endef

TARGET_DEVICES += wndr3700 wndr3700v2 wndr3800 wndr3800ch wndrmac wndrmacv2

define Device/cap324
  DEVICE_TITLE := PowerCloud CAP324 Cloud AP
  DEVICE_PACKAGES := uboot-envtools
  BOARDNAME := CAP324
  DEVICE_PROFILE := CAP324
  IMAGE_SIZE = 15296k
  MTDPARTS = spi0.0:256k(u-boot),64k(u-boot-env)ro,15296k(firmware),640k(certs),64k(nvram),64k(art)
endef

TARGET_DEVICES += cap324

define Device/cap324-nocloud
  DEVICE_TITLE := PowerCloud CAP324 Cloud AP
  DEVICE_PACKAGES := uboot-envtools
  BOARDNAME := CAP324
  DEVICE_PROFILE := CAP324
  IMAGE_SIZE = 16000k
  MTDPARTS = spi0.0:256k(u-boot),64k(u-boot-env)ro,16000k(firmware),64k(art)
endef

TARGET_DEVICES += cap324-nocloud

define Device/cr3000
  DEVICE_TITLE := PowerCloud CR3000 Cloud Router
  DEVICE_PACKAGES := uboot-envtools
  BOARDNAME := CR3000
  DEVICE_PROFILE := CR3000
  IMAGE_SIZE = 7104k
  MTDPARTS = spi0.0:256k(u-boot),64k(u-boot-env)ro,7104k(firmware),640k(certs),64k(nvram),64k(art)
endef

TARGET_DEVICES += cr3000

define Device/cr3000-nocloud
  DEVICE_TITLE := PowerCloud CR3000 (No-Cloud)
  DEVICE_PACKAGES := uboot-envtools
  BOARDNAME := CR3000
  DEVICE_PROFILE := CR3000
  IMAGE_SIZE = 7808k
  MTDPARTS = spi0.0:256k(u-boot),64k(u-boot-env)ro,7808k(firmware),64k(art)
endef

TARGET_DEVICES += cr3000-nocloud

define Device/cr5000
  DEVICE_TITLE := PowerCloud CR5000 Cloud Router
  DEVICE_PACKAGES := uboot-envtools kmod-usb2 kmod-usb-ohci kmod-ledtrig-usbdev kmod-usb-core
  BOARDNAME := CR5000
  DEVICE_PROFILE := CR5000
  IMAGE_SIZE = 7104k
  MTDPARTS = spi0.0:256k(u-boot),64k(u-boot-env)ro,7104k(firmware),640k(certs),64k(nvram),64k(art)
endef

TARGET_DEVICES += cr5000

define Device/cr5000-nocloud
  DEVICE_TITLE := PowerCloud CR5000 (No-Cloud)
  DEVICE_PACKAGES := uboot-envtools kmod-usb2 kmod-usb-ohci kmod-ledtrig-usbdev kmod-usb-core
  BOARDNAME := CR5000
  DEVICE_PROFILE := CR5000
  IMAGE_SIZE = 7808k
  MTDPARTS = spi0.0:256k(u-boot),64k(u-boot-env)ro,7808k(firmware),64k(art)
endef

TARGET_DEVICES += cr5000-nocloud

define Device/antminer-s1
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := Antminer-S1
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-crypto-manager kmod-i2c-gpio-custom kmod-usb-hid
  BOARDNAME := ANTMINER-S1
  DEVICE_PROFILE := ANTMINERS1
  TPLINK_HWID := 0x04440101
  CONSOLE := ttyATH0,115200
endef

define Device/antminer-s3
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := Antminer-S3
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-crypto-manager kmod-i2c-gpio-custom kmod-usb-hid
  BOARDNAME := ANTMINER-S3
  DEVICE_PROFILE := ANTMINERS3
  TPLINK_HWID := 0x04440301
  CONSOLE := ttyATH0,115200
endef

define Device/antrouter-r1
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := Antrouter-R1
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME := ANTROUTER-R1
  DEVICE_PROFILE := ANTROUTERR1
  TPLINK_HWID := 0x44440101
  CONSOLE := ttyATH0,115200
endef

define Device/el-m150
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := EasyLink EL-M150
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME := EL-M150
  DEVICE_PROFILE := ELM150
  TPLINK_HWID := 0x01500101
  CONSOLE := ttyATH0,115200
endef

define Device/el-mini
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := EasyLink EL-MINI
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME := EL-MINI
  DEVICE_PROFILE := ELMINI
  TPLINK_HWID := 0x01530001
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += antminer-s1 antminer-s3 antrouter-r1 el-m150 el-mini

define Device/gl-inet-6408A-v1
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := GL.iNet 6408
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME := GL-INET
  DEVICE_PROFILE := GLINET
  TPLINK_HWID := 0x08000001
  CONSOLE := ttyATH0,115200
endef

define Device/gl-inet-6416A-v1
  $(Device/tplink-16mlzma)
  DEVICE_TITLE := GL.iNet 6416
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME := GL-INET
  DEVICE_PROFILE := GLINET
  TPLINK_HWID := 0x08000001
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += gl-inet-6408A-v1 gl-inet-6416A-v1

define Device/rnx-n360rt
  $(Device/tplink-4m)
  DEVICE_TITLE := Rosewill RNX-N360RT
  BOARDNAME := TL-WR941ND
  DEVICE_PROFILE := RNXN360RT
  TPLINK_HWID := 0x09410002
  TPLINK_HWREV := 0x00420001
endef
TARGET_DEVICES += rnx-n360rt

define Device/mc-mac1200r
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := MERCURY MAC1200R
  DEVICE_PACKAGES := kmod-ath10k
  BOARDNAME := MC-MAC1200R
  DEVICE_PROFILE := MAC1200R
  TPLINK_HWID := 0x12000001
endef
TARGET_DEVICES += mc-mac1200r

define Device/minibox-v1
  $(Device/tplink-16mlzma)
  DEVICE_TITLE := Gainstrong MiniBox V1.0
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2  kmod-ledtrig-usbdev
  BOARDNAME := MINIBOX-V1
  DEVICE_PROFILE := MINIBOXV1
  TPLINK_HWID := 0x3C000201
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += minibox-v1

define Device/omy-g1
  $(Device/tplink-16mlzma)
  DEVICE_TITLE := OMYlink OMY-G1
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME := OMY-G1
  DEVICE_PROFILE := OMYG1
  TPLINK_HWID := 0x06660101
endef

define Device/omy-x1
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := OMYlink OMY-X1
  BOARDNAME := OMY-X1
  DEVICE_PROFILE := OMYX1
  TPLINK_HWID := 0x06660201
endef
TARGET_DEVICES += omy-g1 omy-x1

define Device/onion-omega
  $(Device/tplink-16mlzma)
  DEVICE_TITLE := Onion Omega
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-storage kmod-i2c-core kmod-i2c-gpio-custom kmod-spi-bitbang kmod-spi-dev kmod-spi-gpio kmod-spi-gpio-custom kmod-usb-serial
  BOARDNAME := ONION-OMEGA
  DEVICE_PROFILE := OMEGA
  TPLINK_HWID := 0x04700001
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += onion-omega

define Device/smart-300
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := NC-LINK SMART-300
  BOARDNAME := SMART-300
  DEVICE_PROFILE := SMART-300
  TPLINK_HWID := 0x93410001
endef
TARGET_DEVICES += smart-300

define Device/som9331
  $(Device/tplink-8mlzma)
  DEVICE_TITLE := OpenEmbed SOM9331
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-storage kmod-i2c-core kmod-i2c-gpio-custom kmod-spi-bitbang kmod-spi-dev kmod-spi-gpio kmod-spi-gpio-custom kmod-usb-serial
  BOARDNAME := SOM9331
  DEVICE_PROFILE := SOM9331
  TPLINK_HWID := 0x04800054
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += som9331

define Device/tellstick-znet-lite
  $(Device/tplink-16mlzma)
  DEVICE_TITLE := TellStick ZNet Lite
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-acm kmod-usb-serial kmod-usb-serial-pl2303
  BOARDNAME := TELLSTICK-ZNET-LITE
  DEVICE_PROFILE := TELLSTICKZNETLITE
  TPLINK_HWID := 0x00726001
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += tellstick-znet-lite

define Device/oolite
  $(Device/tplink-16mlzma)
  DEVICE_TITLE := Gainstrong OOLITE
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev
  BOARDNAME := GS-OOLITE
  DEVICE_PROFILE := OOLITE
  TPLINK_HWID := 0x3C000101
  CONSOLE := ttyATH0,115200
endef
TARGET_DEVICES += oolite


define Device/NBG6616
  DEVICE_TITLE := ZyXEL NBG6616
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev kmod-usb-storage kmod-rtc-pcf8563 kmod-ath10k
  BOARDNAME = NBG6616
  KERNEL_SIZE = 2048k
  IMAGE_SIZE = 15323k
  MTDPARTS = spi0.0:192k(u-boot)ro,64k(env)ro,64k(RFdata)ro,384k(zyxel_rfsd),384k(romd),64k(header),2048k(kernel),13184k(rootfs),15232k@0x120000(firmware)
  CMDLINE += mem=128M
  IMAGES := sysupgrade.bin
  KERNEL := kernel-bin | patch-cmdline | lzma | uImage lzma | jffs2 boot/vmlinux.lzma.uImage
  IMAGE/sysupgrade.bin = append-kernel $$$$(KERNEL_SIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
  # We cannot currently build a factory image. It is the sysupgrade image
  # prefixed with a header (which is actually written into the MTD device).
  # The header is 2kiB and is filled with 0xff. The format seems to be:
  #   2 bytes:  0x0000
  #   2 bytes:  checksum of the data partition (big endian)
  #   4 bytes:  length of the contained image file (big endian)
  #  32 bytes:  Firmware Version string (NUL terminated, 0xff padded)
  #   2 bytes:  0x0000
  #   2 bytes:  checksum over the header partition (big endian)
  #  32 bytes:  Model (e.g. "NBG6616", NUL termiated, 0xff padded)
  #      rest: 0xff padding
  #
  # The checksums are calculated by adding up all bytes and if a 16bit
  # overflow occurs, one is added and the sum is masked to 16 bit:
  #   csum = csum + databyte; if (csum > 0xffff) { csum += 1; csum &= 0xffff };
  # Should the file have an odd number of bytes then the byte len-0x800 is
  # used additionally.
  # The checksum for the header is calcualted over the first 2048 bytes with
  # the firmware checksum as the placeholder during calculation.
  #
  # The header is padded with 0xff to the erase block size of the device.
endef

TARGET_DEVICES += NBG6616

define Device/c-55
  DEVICE_TITLE := AirTight Networks C-55
  DEVICE_PACKAGES := kmod-ath9k
  BOARDNAME = C-55
  KERNEL_SIZE = 2048k
  IMAGE_SIZE = 15872k
  MTDPARTS = spi0.0:256k(u-boot)ro,128k(u-boot-env)ro,2048k(kernel),13824k(rootfs),13824k(opt)ro,2624k(failsafe)ro,64k(art)ro,15872k@0x60000(firmware)
  IMAGE/sysupgrade.bin = append-kernel $$$$(KERNEL_SIZE) | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
endef

TARGET_DEVICES += c-55


define Build/uImageHiWiFi
	# Field ih_name needs to start with "tw150v1"
	mkimage -A $(LINUX_KARCH) \
		-O linux -T kernel \
		-C $(1) -a $(KERNEL_LOADADDR) -e $(if $(KERNEL_ENTRY),$(KERNEL_ENTRY),$(KERNEL_LOADADDR)) \
		-n 'tw150v1 $(call toupper,$(LINUX_KARCH)) LEDE Linux-$(LINUX_VERSION)' -d $@ $@.new
	@mv $@.new $@
endef

define Device/hiwifi-hc6361
  DEVICE_TITLE := HiWiFi HC6361
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-storage \
	kmod-fs-ext4 kmod-nls-iso8859-1 e2fsprogs
  BOARDNAME := HiWiFi-HC6361
  DEVICE_PROFILE := HIWIFI_HC6361
  IMAGE_SIZE := 16128k
  KERNEL := kernel-bin | patch-cmdline | lzma | uImageHiWiFi lzma
  CONSOLE := ttyATH0,115200
  MTDPARTS := spi0.0:64k(u-boot)ro,64k(bdinfo)ro,16128k(firmware),64k(backup)ro,64k(art)ro
endef
TARGET_DEVICES += hiwifi-hc6361


# The pre-filled 64 bytes consist of
# - 28 bytes seama_header
# - 36 bytes of META data (4-bytes aligned)
#
# And as the 4 bytes jffs2 marker will be erased on first boot, they need to
# be excluded from the calculation of checksum
define Build/seama-factory
	( dd if=/dev/zero bs=64 count=1; cat $(word 1,$^) ) >$@.loader.tmp
	( dd if=$@.loader.tmp bs=64k conv=sync; dd if=$(word 2,$^) ) >$@.tmp.0
	tail -c +65 $@.tmp.0 >$@.tmp.1
	head -c -4 $@.tmp.1 >$@.tmp.2
	$(STAGING_DIR_HOST)/bin/seama \
		-i $@.tmp.2 \
		-m "dev=/dev/mtdblock/1" -m "type=firmware"
	$(STAGING_DIR_HOST)/bin/seama \
		-s $@ \
		-m "signature=$(1)" \
		-i $@.tmp.2.seama
	tail -c 4 $@.tmp.1 >>$@
	rm -f $@.loader.tmp $@.tmp.*
endef

define Build/seama-sysupgrade
	$(STAGING_DIR_HOST)/bin/seama \
		-i $(word 1,$^) \
		-m "dev=/dev/mtdblock/1" -m "type=firmware"
	( dd if=$(word 1,$^).seama bs=64k conv=sync; dd if=$(word 2,$^) ) >$@
endef

define Build/seama-initramfs
	$(STAGING_DIR_HOST)/bin/seama \
		-i $@ \
		-m "dev=/dev/mtdblock/1" -m "type=firmware"
	mv $@.seama $@
endef

define Device/seama
  CONSOLE := ttyS0,115200
  KERNEL := kernel-bin | loader-kernel-cmdline | lzma
  KERNEL_INITRAMFS := kernel-bin | patch-cmdline | lzma | seama-initramfs
  KERNEL_INITRAMFS_SUFFIX = $$(KERNEL_SUFFIX).seama
  IMAGES := sysupgrade.bin factory.bin
  IMAGE/sysupgrade.bin := seama-sysupgrade $$$$(SEAMA_SIGNATURE) | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := seama-factory $$$$(SEAMA_SIGNATURE) | check-size $$$$(IMAGE_SIZE)
  SEAMA_SIGNATURE :=
  DEVICE_VARS := SEAMA_SIGNATURE
endef

define Device/mynet-n600
$(Device/seama)
  DEVICE_TITLE := Western Digital My Net N600
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = MYNET-N600
  IMAGE_SIZE = 15808k
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,64k(devdata)ro,64k(devconf)ro,15872k(firmware),64k(radiocfg)ro
  SEAMA_SIGNATURE := wrgnd16_wd_db600
endef

define Device/mynet-n750
$(Device/seama)
  DEVICE_TITLE := Western Digital My Net N750
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2
  BOARDNAME = MYNET-N750
  IMAGE_SIZE = 15808k
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env)ro,64k(devdata)ro,64k(devconf)ro,15872k(firmware),64k(radiocfg)ro
  SEAMA_SIGNATURE := wrgnd13_wd_av
endef

define Device/qihoo-c301
$(Device/seama)
  DEVICE_TITLE := Qihoo C301
  DEVICE_PACKAGES :=  kmod-usb-core kmod-usb2 kmod-ledtrig-usbdev kmod-ath10k
  BOARDNAME = QIHOO-C301
  IMAGE_SIZE = 15744k
  MTDPARTS = mtdparts=spi0.0:256k(u-boot)ro,64k(u-boot-env),64k(devdata),64k(devconf),15744k(firmware),64k(warm_start),64k(action_image_config),64k(radiocfg)ro;spi0.1:15360k(upgrade2),1024k(privatedata)
  SEAMA_SIGNATURE := wrgac26_qihoo360_360rg
endef

TARGET_DEVICES += mynet-n600 mynet-n750 qihoo-c301
