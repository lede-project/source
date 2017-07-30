#
# MT7620A Profiles
#

DEVICE_VARS += TPLINK_BOARD_ID

define Build/elecom-header
	cp $@ $(KDIR)/v_0.0.0.bin
	( \
		mkhash md5 $(KDIR)/v_0.0.0.bin && \
		echo 458 \
	) | mkhash md5 > $(KDIR)/v_0.0.0.md5
	$(STAGING_DIR_HOST)/bin/tar -cf $@ -C $(KDIR) v_0.0.0.bin v_0.0.0.md5
endef

define Build/zyimage
	$(STAGING_DIR_HOST)/bin/zyimage $(1) $@
endef

define Device/ai-br100
  DTS := AI-BR100
  IMAGE_SIZE := 7936k
  DEVICE_TITLE := Aigale Ai-BR100
  DEVICE_PACKAGES:= kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += ai-br100

define Device/Archer
  KERNEL := $(KERNEL_DTB)
  KERNEL_INITRAMFS := $(KERNEL_DTB) | tplink-v2-header
  IMAGE/factory.bin := tplink-v2-image
  IMAGE/sysupgrade.bin := tplink-v2-image -s | append-metadata
endef

define Device/ArcherC20i
  $(Device/Archer)
  DTS := ArcherC20i
  SUPPORTED_DEVICES := c20i
  TPLINK_BOARD_ID := ArcherC20i
  IMAGES += factory.bin
  DEVICE_TITLE := TP-Link ArcherC20i
endef
TARGET_DEVICES += ArcherC20i

define Device/ArcherC50
  $(Device/Archer)
  DTS := ArcherC50
  SUPPORTED_DEVICES := c50
  TPLINK_BOARD_ID := ArcherC50
  IMAGES += factory.bin
  DEVICE_TITLE := TP-Link ArcherC50
endef
TARGET_DEVICES += ArcherC50

define Device/ArcherMR200
  $(Device/Archer)
  DTS := ArcherMR200
  SUPPORTED_DEVICES := mr200
  TPLINK_BOARD_ID := ArcherMR200
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-net kmod-usb-net-rndis kmod-usb-serial kmod-usb-serial-option adb-enablemodem
  DEVICE_TITLE := TP-Link ArcherMR200
endef
TARGET_DEVICES += ArcherMR200

define Device/cf-wr800n
  DTS := CF-WR800N
  DEVICE_TITLE := Comfast CF-WR800N
endef
TARGET_DEVICES += cf-wr800n

define Device/cs-qr10
  DTS := CS-QR10
  DEVICE_TITLE := Planex CS-QR10
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-i2c-core kmod-i2c-ralink kmod-sound-core kmod-sound-mtk kmod-sdhci-mt7620
endef
TARGET_DEVICES += cs-qr10

define Device/d240
  DTS := D240
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Sanlinking Technologies D240
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76-core kmod-mt76x2 kmod-sdhci-mt7620
endef
TARGET_DEVICES += d240

define Device/db-wrt01
  DTS := DB-WRT01
  DEVICE_TITLE := Planex DB-WRT01
endef
TARGET_DEVICES += db-wrt01

define Device/dch-m225
  DTS := DCH-M225
  BLOCKSIZE := 4k
  IMAGES += factory.bin
  IMAGE_SIZE := 6848k
  IMAGE/sysupgrade.bin := \
	append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | append-rootfs | \
	seama -m "dev=/dev/mtdblock/2" -m "type=firmware" | \
	pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := \
	append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | \
	append-rootfs | pad-rootfs -x 64 | \
	seama -m "dev=/dev/mtdblock/2" -m "type=firmware" | \
	seama-seal -m "signature=wapn22_dlink.2013gui_dap1320b" | \
	check-size $$$$(IMAGE_SIZE)
  DEVICE_TITLE := D-Link DCH-M225
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += dch-m225

define Device/dir-810l
  DTS := DIR-810L
  IMAGE_SIZE := 6720k
  DEVICE_TITLE := D-Link DIR-810L
endef
TARGET_DEVICES += dir-810l

define Device/e1700
  DTS := E1700
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	umedia-header 0x013326
  DEVICE_TITLE := Linksys E1700
endef
TARGET_DEVICES += e1700

define Device/ex2700
  NETGEAR_HW_ID := 29764623+4+0+32+2x2+0
  NETGEAR_BOARD_ID := EX2700
  DTS := EX2700
  BLOCKSIZE := 4k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  IMAGES += factory.bin
  KERNEL := $(KERNEL_DTB) | uImage lzma | pad-offset 64k 64 | append-uImage-fakeroot-hdr
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	netgear-dni
  DEVICE_PACKAGES := -kmod-mt76
  DEVICE_TITLE := Netgear EX2700
endef
TARGET_DEVICES += ex2700

define Device/ex3700-ex3800
  NETGEAR_BOARD_ID := U12H319T00_NETGEAR
  DTS := EX3700
  BLOCKSIZE := 4k
  IMAGE_SIZE := 7744k
  IMAGES += factory.chk
  IMAGE/factory.chk := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | netgear-chk
  DEVICE_PACKAGES := -kmod-mt76 kmod-mt76x2
  DEVICE_TITLE := Netgear EX3700/EX3800
  SUPPORTED_DEVICES := ex3700
endef
TARGET_DEVICES += ex3700-ex3800

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

define Device/hc5661
  DTS := HC5661
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := HiWiFi HC5661
  DEVICE_PACKAGES := kmod-usb2 kmod-sdhci-mt7620 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += hc5661

define Device/hc5761
  DTS := HC5761
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := HiWiFi HC5761
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-sdhci-mt7620 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += hc5761

define Device/hc5861
  DTS := HC5861
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := HiWiFi HC5861
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-sdhci-mt7620 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += hc5861

define Device/kng_rc
  DTS := kng_rc
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := ZyXEL Keenetic Viva
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-usb-ledtrig-usbport kmod-switch-rtl8366-smi kmod-switch-rtl8367b
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | pad-to 64k | check-size $$$$(IMAGE_SIZE) | \
	zyimage -d 8997 -v "ZyXEL Keenetic Viva"
endef
TARGET_DEVICES += kng_rc

define Device/kn_rc
  DTS := kn_rc
  DEVICE_TITLE := ZyXEL Keenetic Omni
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-usb-ledtrig-usbport
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | pad-to 64k | check-size $$$$(IMAGE_SIZE) | \
	zyimage -d 4882 -v "ZyXEL Keenetic Omni"
endef
TARGET_DEVICES += kn_rc

define Device/kn_rf
  DTS := kn_rf
  DEVICE_TITLE := ZyXEL Keenetic Omni II
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-usb-ledtrig-usbport
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(IMAGE/sysupgrade.bin) | pad-to 64k | check-size $$$$(IMAGE_SIZE) | \
	zyimage -d 2102034 -v "ZyXEL Keenetic Omni II"
endef
TARGET_DEVICES += kn_rf

define Device/microwrt
  DTS := MicroWRT
  IMAGE_SIZE := 16128k
  DEVICE_TITLE := Microduino MicroWRT
endef
TARGET_DEVICES += microwrt

define Device/miwifi-mini
  DTS := MIWIFI-MINI
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Xiaomi MiWiFi Mini
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += miwifi-mini

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

define Device/mt7620a
  DTS := MT7620a
  DEVICE_TITLE := MediaTek MT7620a EVB
endef
TARGET_DEVICES += mt7620a

define Device/mt7620a_mt7530
  DTS := MT7620a_MT7530
  DEVICE_TITLE := MediaTek MT7620a + MT7530 EVB
endef
TARGET_DEVICES += mt7620a_mt7530

define Device/mt7620a_mt7610e
  DTS := MT7620a_MT7610e
  DEVICE_TITLE := MediaTek MT7620a + MT7610e EVB
endef
TARGET_DEVICES += mt7620a_mt7610e

define Device/mt7620a_v22sg
  DTS := MT7620a_V22SG
  DEVICE_TITLE := MediaTek MT7620a V22SG
endef
TARGET_DEVICES += mt7620a_v22sg

define Device/mzk-750dhp
  DTS := MZK-750DHP
  DEVICE_TITLE := Planex MZK-750DHP
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += mzk-750dhp

define Device/mzk-ex300np
  DTS := MZK-EX300NP
  DEVICE_TITLE := Planex MZK-EX300NP
endef
TARGET_DEVICES += mzk-ex300np

define Device/mzk-ex750np
  DTS := MZK-EX750NP
  DEVICE_TITLE := Planex MZK-EX750NP
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += mzk-ex750np

define Device/na930
  DTS := NA930
  IMAGE_SIZE := 20m
  DEVICE_TITLE := Sercomm NA930
endef
TARGET_DEVICES += na930

define Device/oy-0001
  DTS := OY-0001
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Oh Yeah OY-0001
endef
TARGET_DEVICES += oy-0001

define Device/psg1208
  DTS := PSG1208
  DEVICE_TITLE := Phicomm PSG1208
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += psg1208

define Device/psg1218a
  DTS := PSG1218A
  DEVICE_TITLE := Phicomm PSG1218 rev.Ax
  DEVICE_PACKAGES := kmod-mt76x2
  SUPPORTED_DEVICES += psg1218
endef
TARGET_DEVICES += psg1218a

define Device/psg1218b
  DTS := PSG1218B
  DEVICE_TITLE := Phicomm PSG1218 rev.Bx
  DEVICE_PACKAGES := kmod-mt76x2
  SUPPORTED_DEVICES += psg1218
endef
TARGET_DEVICES += psg1218b

define Device/rp-n53
  DTS := RP-N53
  DEVICE_TITLE := Asus RP-N53
endef
TARGET_DEVICES += rp-n53

define Device/rt-n14u
  DTS := RT-N14U
  DEVICE_TITLE := Asus RT-N14u
endef
TARGET_DEVICES += rt-n14u

define Device/rt-ac51u
  DTS := RT-AC51U
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Asus RT-AC51U
  DEVICE_PACKAGES := kmod-usb-core kmod-usb2 kmod-usb-ehci kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += rt-ac51u

define Device/tiny-ac
  DTS := TINY-AC
  DEVICE_TITLE := Dovado Tiny AC
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += tiny-ac

define Device/whr-1166d
  DTS := WHR-1166D
  IMAGE_SIZE := 15040k
  DEVICE_TITLE := Buffalo WHR-1166D
endef
TARGET_DEVICES += whr-1166d

define Device/whr-300hp2
  DTS := WHR-300HP2
  IMAGE_SIZE := 6848k
  DEVICE_TITLE := Buffalo WHR-300HP2
endef
TARGET_DEVICES += whr-300hp2

define Device/whr-600d
  DTS := WHR-600D
  IMAGE_SIZE := 6848k
  DEVICE_TITLE := Buffalo WHR-600D
endef
TARGET_DEVICES += whr-600d

define Device/wmr-300
  DTS := WMR-300
  DEVICE_TITLE := Buffalo WMR-300
endef
TARGET_DEVICES += wmr-300

define Device/wn3000rpv3
  NETGEAR_HW_ID := 29764836+8+0+32+2x2+0
  NETGEAR_BOARD_ID := WN3000RPv3
  DTS := WN3000RPV3
  BLOCKSIZE := 4k
  IMAGES += factory.bin
  KERNEL := $(KERNEL_DTB) | uImage lzma | pad-offset 64k 64 | append-uImage-fakeroot-hdr
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	netgear-dni
  DEVICE_TITLE := Netgear WN3000RPv3
endef
TARGET_DEVICES += wn3000rpv3

define Device/wrh-300cr
  DTS := WRH-300CR
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	elecom-header
  DEVICE_TITLE := Elecom WRH-300CR
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += wrh-300cr

define Device/wrtnode
  DTS := WRTNODE
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := WRTNode
endef
TARGET_DEVICES += wrtnode

define Device/wt3020-4M
  DTS := WT3020-4M
  BLOCKSIZE := 4k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  IMAGES += factory.bin
  SUPPORTED_DEVICES += wt3020
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	poray-header -B WT3020 -F 4M
  DEVICE_TITLE := Nexx WT3020 (4MB)
endef
TARGET_DEVICES += wt3020-4M

define Device/wt3020-8M
  DTS := WT3020-8M
  IMAGES += factory.bin
  SUPPORTED_DEVICES += wt3020
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	poray-header -B WT3020 -F 8M
  DEVICE_TITLE := Nexx WT3020 (8MB)
endef
TARGET_DEVICES += wt3020-8M

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

define Device/youku-yk1
  DTS := YOUKU-YK1
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := YOUKU YK1
endef
TARGET_DEVICES += youku-yk1

define Device/zbt-ape522ii
  DTS := ZBT-APE522II
  DEVICE_TITLE := Zbtlink ZBT-APE522II
  DEVICE_PACKAGES := kmod-mt76
endef
TARGET_DEVICES += zbt-ape522ii

define Device/zbt-cpe102
  DTS := ZBT-CPE102
  DEVICE_TITLE := Zbtlink ZBT-CPE102
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci
endef
TARGET_DEVICES += zbt-cpe102

define Device/zbt-wa05
  DTS := ZBT-WA05
  DEVICE_TITLE := Zbtlink ZBT-WA05
endef
TARGET_DEVICES += zbt-wa05

define Device/zbt-we2026
  DTS := ZBT-WE2026
  DEVICE_TITLE := Zbtlink ZBT-WE2026
endef
TARGET_DEVICES += zbt-we2026

define Device/zbt-we826-16M
  DTS := ZBT-WE826-16M
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  SUPPORTED_DEVICES += zbt-we826
  DEVICE_TITLE := Zbtlink ZBT-WE826 (16M)
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76 kmod-sdhci-mt7620 
endef
TARGET_DEVICES += zbt-we826-16M

define Device/zbt-we826-32M
  DTS := ZBT-WE826-32M
  IMAGE_SIZE := $(ralink_default_fw_size_32M)
  DEVICE_TITLE := Zbtlink ZBT-WE826 (32M)
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ohci kmod-mt76 kmod-sdhci-mt7620
endef
TARGET_DEVICES += zbt-we826-32M

define Device/zbt-wr8305rt
  DTS := ZBT-WR8305RT
  DEVICE_TITLE := Zbtlink ZBT-WR8305RT
endef
TARGET_DEVICES += zbt-wr8305rt

define Device/zte-q7
  DTS := ZTE-Q7
  DEVICE_TITLE := ZTE Q7
endef
TARGET_DEVICES += zte-q7
