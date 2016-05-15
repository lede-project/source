#
# MT7620A Profiles
#

define Build/tplink-header
	$(STAGING_DIR_HOST)/bin/mktplinkfw2 -a 0x4 -V "ver. 2.0" -B $(1) \
		-o $@.new -k $@  && mv $@.new $@
endef

define Build/pad-ex2700
	cat ex2700-fakeroot.uImage >> $@; cat ex2700-fakeroot.uImage >> $@;
	dd if=$@ of=$@.new bs=64k conv=sync && truncate -s 128 $@.new && mv $@.new $@
endef

define Build/append-ex2700
	cat ex2700-fakeroot.uImage >> $@
endef

define Build/netgear-header
	$(STAGING_DIR_HOST)/bin/mkdniimg \
		$(1) -v OpenWrt -i $@ \
		-o $@.new && mv $@.new $@
endef

define Build/poray-header
	mkporayfw $(1) \
		-f $@ \
		-o $@.new; \
		mv $@.new $@
endef

define Build/umedia-header
	fix-u-media-header -T 0x46 -B $(1) -i $@ -o $@.new && mv $@.new $@
endef

define Build/elecom-header
	cp $@ $(KDIR)/v_0.0.0.bin
	( \
		$(STAGING_DIR_HOST)/bin/md5sum $(KDIR)/v_0.0.0.bin | \
			sed 's/ .*//' && \
		echo 458 \
	) | $(STAGING_DIR_HOST)/bin/md5sum | \
		sed 's/ .*//' > $(KDIR)/v_0.0.0.md5
	$(STAGING_DIR_HOST)/bin/tar -cf $@ -C $(KDIR) v_0.0.0.bin v_0.0.0.md5
endef

define Device/ArcherC20i
  DTS := ArcherC20i
  KERNEL := $(KERNEL_DTB)
  KERNEL_INITRAMFS := $(KERNEL_DTB) | tplink-header ArcherC20i -c
  IMAGE/sysupgrade.bin := append-kernel | tplink-header ArcherC20i -j -r $(KDIR)/root.squashfs
  DEVICE_TITLE := TP-Link ArcherC20i
endef
TARGET_DEVICES += ArcherC20i

ex2700_mtd_size=3866624
define Device/ex2700
  DTS := EX2700
  IMAGE_SIZE := $(ex2700_mtd_size)
  IMAGES += factory.bin
  KERNEL := $(KERNEL_DTB) | pad-ex2700 | uImage lzma | append-ex2700
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | netgear-header -B EX2700 -H 29764623+4+0+32+2x2+0
  DEVICE_TITLE := Netgear EX2700
endef
TARGET_DEVICES += ex2700

define Device/wt3020-4M
  DTS := WT3020-4M
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | poray-header -B WT3020 -F 4M
  DEVICE_TITLE := Nexx WT3020 (4MB)
endef
TARGET_DEVICES += wt3020-4M

define Device/wt3020-8M
  DTS := WT3020-8M
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | poray-header -B WT3020 -F 4M
  DEVICE_TITLE := Nexx WT3020 (8MB)
endef
TARGET_DEVICES += wt3020-8M

define Device/wrh-300cr
  DTS := WRH-300CR
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | elecom-header
  DEVICE_TITLE := Elecom WRH-300CR 
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += wrh-300cr

define Device/e1700
  DTS := E1700
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | umedia-header 0x013326
  DEVICE_TITLE := Linksys E1700
endef
TARGET_DEVICES += e1700

br100_mtd_size=8126464
define Device/ai-br100
  DTS := AI-BR100
  IMAGE_SIZE := $(br100_mtd_size)
  DEVICE_TITLE := Aigale Ai-BR100
  DEVICE_PACKAGES:= kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += ai-br100

whr_300hp2_mtd_size=7012352
define Device/whr-300hp2
  DTS := WHR-300HP2
  IMAGE_SIZE := $(whr_300hp2_mtd_size)
  DEVICE_TITLE := Buffalo WHR-300HP2
endef
TARGET_DEVICES += whr-300hp2

define Device/whr-600d
  DTS := WHR-600D
  IMAGE_SIZE := $(whr_300hp2_mtd_size)
  DEVICE_TITLE := Buffalo WHR-600D
endef
TARGET_DEVICES += whr-600d

whr_1166d_mtd_size=15400960
define Device/whr-1166d
  DTS := WHR-1166D
  IMAGE_SIZE := $(whr_1166d_mtd_size)
  DEVICE_TITLE := Buffalo WHR-1166D
endef
TARGET_DEVICES += whr-1166d

dlink810l_mtd_size=6881280
define Device/dir-810l
  DTS := DIR-810L
  IMAGE_SIZE := $(dlink810l_mtd_size)
  DEVICE_TITLE := D-Link DIR-810L
endef
TARGET_DEVICES += dir-810l

na930_mtd_size=20971520
define Device/na930
  DTS := NA930
  IMAGE_SIZE := $(na930_mtd_size)
  DEVICE_TITLE := Sercomm NA930
endef
TARGET_DEVICES += na930

microwrt_mtd_size=16515072
define Device/microwrt
  DTS := MicroWRT
  IMAGE_SIZE := $(microwrt_mtd_size)
  DEVICE_TITLE := Microduino MicroWRT
endef
TARGET_DEVICES += microwrt

define Device/mt7620a
  DTS := MT7620a
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := MediaTek MT7620a EVB
endef
TARGET_DEVICES += mt7620a

define Device/mt7620a_mt7610e
  DTS := MT7620a_MT7610e
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := MediaTek MT7620a + MT7610e EVB
endef
TARGET_DEVICES += mt7620a_mt7610e

define Device/mt7620a_mt7530
  DTS := MT7620a_MT7530
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := MediaTek MT7620a + MT7530 EVB
endef
TARGET_DEVICES += mt7620a_mt7530

define Device/mt7620a_v22sg
  DTS := MT7620a_V22SG
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := MediaTek MT7620a V22SG
endef
TARGET_DEVICES += mt7620a_v22sg

define Device/rp-n53
  DTS := RP-N53
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Asus RP-N53
endef
TARGET_DEVICES += rp-n53

define Device/cf-wr800n
  DTS := CF-WR800N
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Comfast CF-WR800N
endef
TARGET_DEVICES += cf-wr800n

define Device/cs-qr10
  DTS := CS-QR10
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Planex CS-QR10
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-i2c-core kmod-i2c-ralink kmod-sound-core kmod-sound-mtk kmod-sdhci-mt7620
endef
TARGET_DEVICES += cs-qr10

define Device/db-wrt01
  DTS := DB-WRT01
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Planex DB-WRT01
endef
TARGET_DEVICES += db-wrt01

define Device/mzk-750dhp
  DTS := MZK-750DHP
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Planex MZK-750DHP
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += mzk-750dhp

define Device/mzk-ex300np
  DTS := MZK-EX300NP
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Planex MZK-EX300NP
endef
TARGET_DEVICES += mzk-ex300np

define Device/mzk-ex750np
  DTS := MZK-EX750NP
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Planex MZK-EX750NP
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += mzk-ex750np

define Device/hc5661
  DTS := HC5661
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := HiWiFi HC5661
  DEVICE_PACKAGES := kmod-usb2 kmod-sdhci kmod-sdhci-mt7620 kmod-ledtrig-usbdev
endef
TARGET_DEVICES += hc5661

define Device/hc5761
  DTS := HC5761
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := HiWiFi HC5761 
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-sdhci kmod-sdhci-mt7620 kmod-ledtrig-usbdev
endef
TARGET_DEVICES += hc5761

define Device/hc5861
  DTS := HC5861
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := HiWiFi HC5861
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-sdhci kmod-sdhci-mt7620 kmod-ledtrig-usbdev
endef
TARGET_DEVICES += hc5861

define Device/oy-0001
  DTS := OY-0001
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Oh Yeah OY-0001
endef
TARGET_DEVICES += oy-0001

define Device/psg1208
  DTS := PSG1208
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Phicomm PSG1208
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += psg1208

define Device/y1
  DTS := Y1
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Lenovo Y1
endef
TARGET_DEVICES += y1

define Device/y1s
  DTS := Y1S
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Lenovo Y1S
endef
TARGET_DEVICES += y1s

define Device/mlw221
  DTS := MLW221
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Kingston MLW221
endef
TARGET_DEVICES += mlw221

define Device/mlwg2
  DTS := MLWG2
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Kingston MLWG2
endef
TARGET_DEVICES += mlwg2

define Device/wmr-300
  DTS := WMR-300
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Buffalo WMR-300
endef
TARGET_DEVICES += wmr-300

define Device/rt-n14u
  DTS := RT-N14U
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Asus RT-N14u
endef
TARGET_DEVICES += rt-n14u

define Device/wrtnode
  DTS := WRTNODE
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := WRTNode
endef
TARGET_DEVICES += wrtnode

define Device/miwifi-mini
  DTS := MIWIFI-MINI
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Xiaomi MiWiFi Mini
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += miwifi-mini

define Device/gl-mt300a
  DTS := GL-MT300A
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := GL-Inet GL-MT300A
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76
endef
TARGET_DEVICES += gl-mt300a

define Device/gl-mt300n
  DTS := GL-MT300N
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := GL-Inet GL-MT300N
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76
endef
TARGET_DEVICES += gl-mt300n

define Device/gl-mt750
  DTS := GL-MT750
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := GL-Inet GL-MT750
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76
endef
TARGET_DEVICES += gl-mt750

define Device/zte-q7
  DTS := ZTE-Q7
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := ZTE Q7
endef
TARGET_DEVICES += zte-q7

define Device/youku-yk1
  DTS := YOUKU-YK1
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := YOUKU YK1
endef
TARGET_DEVICES += youku-yk1

define Device/zbt-wa05
  DTS := ZBT-WA05
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Zbtlink ZBT-WA05
endef
TARGET_DEVICES += zbt-wa05

define Device/zbt-we826
  DTS := ZBT-WE826
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Zbtlink ZBT-WE826
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76 kmod-sdhci-mt7620 
endef
TARGET_DEVICES += zbt-we826

define Device/zbt-wr8305rt
  DTS := ZBT-WR8305RT
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Zbtlink ZBT-WR8305RT
endef
TARGET_DEVICES += zbt-wr8305rt

define Device/tiny-ac
  DTS := TINY-AC
  IMAGE_SIZE := $(ralink_default_fw_size_8M)
  DEVICE_TITLE := Dovado Tiny AC
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += tiny-ac
