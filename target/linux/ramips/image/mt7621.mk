#
# MT7621 Profiles
#

KERNEL_DTB += -d21
DEVICE_VARS += TPLINK_BOARD_ID TPLINK_HEADER_VERSION TPLINK_HWID TPLINK_HWREV

define Build/elecom-gst-factory
  $(eval product=$(word 1,$(1)))
  $(eval version=$(word 2,$(1)))
  ( $(STAGING_DIR_HOST)/bin/mkhash md5 $@ | tr -d '\n' ) >> $@
  ( \
    echo -n "ELECOM $(product) v$(version)" | \
      dd bs=32 count=1 conv=sync; \
    dd if=$@; \
  ) > $@.new
  mv $@.new $@
  echo -n "MT7621_ELECOM_$(product)" >> $@
endef

define Build/elecom-wrc-factory
  $(eval product=$(word 1,$(1)))
  $(eval version=$(word 2,$(1)))
  $(STAGING_DIR_HOST)/bin/mkhash md5 $@ >> $@
  ( \
    echo -n "ELECOM $(product) v$(version)" | \
      dd bs=32 count=1 conv=sync; \
    dd if=$@; \
  ) > $@.new
  mv $@.new $@
endef

define Build/iodata-factory
  $(eval fw_size=$(word 1,$(1)))
  $(eval fw_type=$(word 2,$(1)))
  $(eval product=$(word 3,$(1)))
  $(eval factory_bin=$(word 4,$(1)))
  if [ -e $(KDIR)/tmp/$(KERNEL_INITRAMFS_IMAGE) -a "$$(stat -c%s $@)" -lt "$(fw_size)" ]; then \
    $(CP) $(KDIR)/tmp/$(KERNEL_INITRAMFS_IMAGE) $(factory_bin); \
    $(STAGING_DIR_HOST)/bin/mksenaofw \
      -r 0x30a -p $(product) -t $(fw_type) \
      -e $(factory_bin) -o $(factory_bin).new; \
    mv $(factory_bin).new $(factory_bin); \
    $(CP) $(factory_bin) $(BIN_DIR)/; \
	else \
		echo "WARNING: initramfs kernel image too big, cannot generate factory image" >&2; \
	fi
endef

define Build/ubnt-erx-factory-image
	if [ -e $(KDIR)/tmp/$(KERNEL_INITRAMFS_IMAGE) -a "$$(stat -c%s $@)" -lt "$(KERNEL_SIZE)" ]; then \
		echo '21001:6' > $(1).compat; \
		$(TAR) -cf $(1) --transform='s/^.*/compat/' $(1).compat; \
		\
		$(TAR) -rf $(1) --transform='s/^.*/vmlinux.tmp/' $(KDIR)/tmp/$(KERNEL_INITRAMFS_IMAGE); \
		mkhash md5 $(KDIR)/tmp/$(KERNEL_INITRAMFS_IMAGE) > $(1).md5; \
		$(TAR) -rf $(1) --transform='s/^.*/vmlinux.tmp.md5/' $(1).md5; \
		\
		echo "dummy" > $(1).rootfs; \
		$(TAR) -rf $(1) --transform='s/^.*/squashfs.tmp/' $(1).rootfs; \
		\
		mkhash md5 $(1).rootfs > $(1).md5; \
		$(TAR) -rf $(1) --transform='s/^.*/squashfs.tmp.md5/' $(1).md5; \
		\
		echo '$(BOARD) $(VERSION_CODE) $(VERSION_NUMBER)' > $(1).version; \
		$(TAR) -rf $(1) --transform='s/^.*/version.tmp/' $(1).version; \
		\
		$(CP) $(1) $(BIN_DIR)/; \
	else \
		echo "WARNING: initramfs kernel image too big, cannot generate factory image" >&2; \
	fi
endef

# The OEM webinterface expects an kernel with initramfs which has the uImage
# header field ih_name.
# We don't wan't to set the header name field for the kernel include in the
# sysupgrade image as well, as this image shouldn't be accepted by the OEM
# webinterface. It will soft-brick the board.
define Build/wr1201-factory-header
	mkimage -A $(LINUX_KARCH) \
		-O linux -T kernel \
		-C lzma -a $(KERNEL_LOADADDR) -e $(if $(KERNEL_ENTRY),$(KERNEL_ENTRY),$(KERNEL_LOADADDR)) \
		-n 'WR1201_8_128' -d $@ $@.new
	mv $@.new $@
endef

define Device/afoundry_ew1200
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := AFOUNDRY
  DEVICE_MODEL := EW1200
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-mt76x2 kmod-mt7603 kmod-usb3 \
	kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += ew1200
endef
TARGET_DEVICES += afoundry_ew1200

define Device/asiarf_ap7621-001
  MTK_SOC := mt7621
  IMAGE_SIZE := 16000k
  DEVICE_VENDOR := AsiaRF
  DEVICE_MODEL := AP7621-001
  DEVICE_PACKAGES := \
	kmod-sdhci-mt7620 kmod-mt76x2 kmod-usb3
endef
TARGET_DEVICES += asiarf_ap7621-001

define Device/asiarf_ap7621-nv1
  MTK_SOC := mt7621
  IMAGE_SIZE := 16000k
  DEVICE_VENDOR := AsiaRF
  DEVICE_MODEL := AP7621-NV1
  DEVICE_PACKAGES := \
	kmod-sdhci-mt7620 kmod-mt76x2 kmod-usb3
endef
TARGET_DEVICES += asiarf_ap7621-nv1

define Device/asus_rt-ac57u
  MTK_SOC := mt7621
  DEVICE_VENDOR := ASUS
  DEVICE_MODEL := RT-AC57U
  IMAGE_SIZE := 16064k
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += asus_rt-ac57u

define Device/asus_rt-ac65p
  MTK_SOC := mt7621
  DEVICE_VENDOR := ASUS
  DEVICE_MODEL := RT-AC65P
  IMAGE_SIZE := 51200k
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 4096k
  IMAGES += factory.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory.bin := append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := kmod-usb3 kmod-mt7615e wpad-basic uboot-envtools
endef
TARGET_DEVICES += asus_rt-ac65p

define Device/asus_rt-ac85p
  MTK_SOC := mt7621
  DEVICE_VENDOR := ASUS
  DEVICE_MODEL := RT-AC85P
  IMAGE_SIZE := 51200k
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 4096k
  IMAGES += factory.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory.bin := append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := kmod-usb3 kmod-mt7615e wpad-basic uboot-envtools
endef
TARGET_DEVICES += asus_rt-ac85p

define Device/buffalo_wsr-1166dhp
  MTK_SOC := mt7621
  IMAGE/sysupgrade.bin := trx | pad-rootfs | append-metadata
  IMAGE_SIZE := 15936k
  DEVICE_VENDOR := Buffalo
  DEVICE_MODEL := WSR-1166DHP
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 wpad-basic
  SUPPORTED_DEVICES += wsr-1166
endef
TARGET_DEVICES += buffalo_wsr-1166dhp

define Device/buffalo_wsr-600dhp
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Buffalo
  DEVICE_MODEL := WSR-600DHP
  DEVICE_PACKAGES := kmod-mt7603 kmod-rt2800-pci wpad-basic
  SUPPORTED_DEVICES += wsr-600
endef
TARGET_DEVICES += buffalo_wsr-600dhp

define Device/dlink_dir-860l-b1
  $(Device/seama)
  MTK_SOC := mt7621
  BLOCKSIZE := 64k
  SEAMA_SIGNATURE := wrgac13_dlink.2013gui_dir860lb
  KERNEL := kernel-bin | append-dtb | relocate-kernel | lzma | uImage lzma
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := D-Link
  DEVICE_MODEL := DIR-860L
  DEVICE_VARIANT := B1
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += dir-860l-b1
endef
TARGET_DEVICES += dlink_dir-860l-b1

define Device/d-team_newifi-d2
  MTK_SOC := mt7621
  IMAGE_SIZE := 32448k
  DEVICE_VENDOR := Newifi
  DEVICE_MODEL := D2
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += d-team_newifi-d2

define Device/d-team_pbr-m1
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := PandoraBox
  DEVICE_MODEL := PBR-M1
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-mt7603 kmod-mt76x2 kmod-sdhci-mt7620 \
	kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += pbr-m1
endef
TARGET_DEVICES += d-team_pbr-m1

define Device/edimax_rg21s
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Edimax
  DEVICE_MODEL := Gemini AC2600 RG21S
  IMAGES += factory.bin
  IMAGE/factory.bin := \
    $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
    elx-header 02020038 8844A2D168B45A2D
  DEVICE_PACKAGES := \
        kmod-mt7615e wpad-basic
endef
TARGET_DEVICES += edimax_rg21s

define Device/elecom_wrc-1167ghbk2-s
  MTK_SOC := mt7621
  IMAGE_SIZE := 15488k
  DEVICE_VENDOR := ELECOM
  DEVICE_MODEL := WRC-1167GHBK2-S
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) |\
    elecom-wrc-factory WRC-1167GHBK2-S 0.00
  DEVICE_PACKAGES := kmod-mt7615e wpad-basic
endef
TARGET_DEVICES += elecom_wrc-1167ghbk2-s

define Device/elecom_wrc-1900gst
  MTK_SOC := mt7621
  IMAGE_SIZE := 11264k
  DEVICE_VENDOR := ELECOM
  DEVICE_MODEL := WRC-1900GST
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) |\
    elecom-gst-factory WRC-1900GST 0.00
endef
TARGET_DEVICES += elecom_wrc-1900gst

define Device/elecom_wrc-2533gst
  MTK_SOC := mt7621
  IMAGE_SIZE := 11264k
  DEVICE_VENDOR := ELECOM
  DEVICE_MODEL := WRC-2533GST
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) |\
    elecom-gst-factory WRC-2533GST 0.00
endef
TARGET_DEVICES += elecom_wrc-2533gst

define Device/firefly_firewrt
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Firefly
  DEVICE_MODEL := FireWRT
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += firewrt
endef
TARGET_DEVICES += firefly_firewrt

define Device/gehua_ghl-r-001
  MTK_SOC := mt7621
  IMAGE_SIZE := 32448k
  DEVICE_VENDOR := GeHua
  DEVICE_MODEL := GHL-R-001
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += gehua_ghl-r-001

define Device/gnubee_gb-pc1
  MTK_SOC := mt7621
  DEVICE_VENDOR := GnuBee
  DEVICE_MODEL := Personal Cloud One
  DEVICE_PACKAGES := kmod-ata-core kmod-ata-ahci kmod-usb3 kmod-sdhci-mt7620
  IMAGE_SIZE := 32448k
endef
TARGET_DEVICES += gnubee_gb-pc1

define Device/gnubee_gb-pc2
  MTK_SOC := mt7621
  DEVICE_VENDOR := GnuBee
  DEVICE_MODEL := Personal Cloud Two
  DEVICE_PACKAGES := kmod-ata-core kmod-ata-ahci kmod-usb3 kmod-sdhci-mt7620
  IMAGE_SIZE := 32448k
endef
TARGET_DEVICES += gnubee_gb-pc2

define Device/hiwifi_hc5962
  MTK_SOC := mt7621
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 2097152
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 32768k
  IMAGES += factory.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory.bin := append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi | check-size $$$$(IMAGE_SIZE)
  DEVICE_VENDOR := HiWiFi
  DEVICE_MODEL := HC5962
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 kmod-usb3 wpad-basic
  SUPPORTED_DEVICES += hc5962
endef
TARGET_DEVICES += hiwifi_hc5962

define Device/iodata_wn-ax1167gr
  MTK_SOC := mt7621
  IMAGE_SIZE := 15552k
  KERNEL_INITRAMFS := $$(KERNEL) | \
    iodata-factory 7864320 4 0x1055 $(KDIR)/tmp/$$(KERNEL_INITRAMFS_PREFIX)-factory.bin
  DEVICE_VENDOR := I-O DATA
  DEVICE_MODEL := WN-AX1167GR
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 wpad-basic
endef
TARGET_DEVICES += iodata_wn-ax1167gr

define Device/iodata_wn-gx300gr
  MTK_SOC := mt7621
  IMAGE_SIZE := 7616k
  DEVICE_VENDOR := I-O DATA
  DEVICE_MODEL := WN-GX300GR
  DEVICE_PACKAGES := kmod-mt7603 wpad-basic
endef
TARGET_DEVICES += iodata_wn-gx300gr

define Device/iodata_wnpr2600g
  MTK_SOC := mt7621
  DEVICE_VENDOR := I-O DATA
  DEVICE_MODEL := WNPR2600G
  IMAGE_SIZE := 13952k
  IMAGES += factory.bin
  IMAGE/factory.bin := \
    $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
    elx-header 0104003a 8844A2D168B45A2D
  DEVICE_PACKAGES := kmod-mt7615e wpad-basic
endef
TARGET_DEVICES += iodata_wnpr2600g

define Device/lenovo_newifi-d1
  MTK_SOC := mt7621
  IMAGE_SIZE := 32448k
  DEVICE_VENDOR := Newifi
  DEVICE_MODEL := D1
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-sdhci-mt7620 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += newifi-d1
endef
TARGET_DEVICES += lenovo_newifi-d1

define Device/linksys_re6500
  MTK_SOC := mt7621
  IMAGE_SIZE := 7872k
  DEVICE_VENDOR := Linksys
  DEVICE_MODEL := RE6500
  DEVICE_PACKAGES := kmod-mt76x2 wpad-basic
  SUPPORTED_DEVICES += re6500
endef
TARGET_DEVICES += linksys_re6500

define Device/mediatek_ap-mt7621a-v60
  MTK_SOC := mt7621
  IMAGE_SIZE := 7872k
  DEVICE_VENDOR := Mediatek
  DEVICE_MODEL := AP-MT7621A-V60 EVB
  DEVICE_PACKAGES := kmod-usb3 kmod-sdhci-mt7620 kmod-sound-mt7620
endef
TARGET_DEVICES += mediatek_ap-mt7621a-v60

define Device/mediatek_mt7621-eval-board
  MTK_SOC := mt7621
  BLOCKSIZE := 64k
  IMAGE_SIZE := 15104k
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := MT7621 EVB
  SUPPORTED_DEVICES += mt7621
endef
TARGET_DEVICES += mediatek_mt7621-eval-board

define Device/MikroTik
  MTK_SOC := mt7621
  DEVICE_VENDOR := MikroTik
  BLOCKSIZE := 64k
  IMAGE_SIZE := 16128k
  DEVICE_PACKAGES := kmod-usb3
  LOADER_TYPE := elf
  PLATFORM := mt7621
  KERNEL := $(KERNEL_DTB) | loader-kernel
  IMAGE/sysupgrade.bin := append-kernel | kernel2minor -s 1024 | pad-to $$$$(BLOCKSIZE) | \
	append-rootfs | pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
endef

define Device/mikrotik_rb750gr3
  $(Device/MikroTik)
  DEVICE_MODEL := RouterBOARD RB750G
  DEVICE_VARIANT := r3
  DEVICE_PACKAGES += kmod-gpio-beeper
endef
TARGET_DEVICES += mikrotik_rb750gr3

define Device/mikrotik_rbm11g
  $(Device/MikroTik)
  DEVICE_MODEL := RouterBOARD M11G
endef
TARGET_DEVICES += mikrotik_rbm11g

define Device/mikrotik_rbm33g
  $(Device/MikroTik)
  DEVICE_MODEL := RouterBOARD M33G
endef
TARGET_DEVICES += mikrotik_rbm33g

define Device/mqmaker_witi
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := MQmaker
  DEVICE_MODEL := WiTi
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-mt76x2 kmod-sdhci-mt7620 kmod-usb3 \
	kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += witi mqmaker,witi-256m mqmaker,witi-512m
endef
TARGET_DEVICES += mqmaker_witi

define Device/mtc_wr1201
  MTK_SOC := mt7621
  IMAGE_SIZE := 16000k
  DEVICE_VENDOR := MTC
  DEVICE_MODEL := Wireless Router WR1201
  KERNEL_INITRAMFS := $(KERNEL_DTB) | wr1201-factory-header
  DEVICE_PACKAGES := \
	kmod-sdhci-mt7620 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += mtc_wr1201

define Device/netgear_ex6150
  MTK_SOC := mt7621
  DEVICE_VENDOR := NETGEAR
  DEVICE_MODEL := EX6150
  DEVICE_PACKAGES := kmod-mt76x2 wpad-basic
  NETGEAR_BOARD_ID := U12H318T00_NETGEAR
  IMAGE_SIZE := 14848k
  IMAGES += factory.chk
  IMAGE/factory.chk := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | netgear-chk
endef
TARGET_DEVICES += netgear_ex6150

define Device/netgear_sercomm_nand
  MTK_SOC := mt7621
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 4096k
  UBINIZE_OPTS := -E 5
  IMAGES += factory.img kernel.bin rootfs.bin
  IMAGE/factory.img := pad-extra 2048k | append-kernel | pad-to 6144k | append-ubi | \
	pad-to $$$$(BLOCKSIZE) | sercom-footer | pad-to 128 | \
	zip $$$$(SERCOMM_HWNAME).bin | sercom-seal
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/kernel.bin := append-kernel
  IMAGE/rootfs.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  DEVICE_VENDOR := NETGEAR
  DEVICE_PACKAGES := kmod-mt7603 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
DEVICE_VARS += SERCOMM_HWNAME SERCOMM_HWID SERCOMM_HWVER SERCOMM_SWVER

define Device/netgear_r6220
  $(Device/netgear_sercomm_nand)
  DEVICE_MODEL := R6220
  SERCOMM_HWNAME := R6220
  SERCOMM_HWID := AYA
  SERCOMM_HWVER := A001
  SERCOMM_SWVER := 0x0086
  IMAGE_SIZE := 28672k
  DEVICE_PACKAGES += kmod-mt76x2
  SUPPORTED_DEVICES += r6220
endef
TARGET_DEVICES += netgear_r6220


define Device/netgear_r6260
  $(Device/netgear_sercomm_nand)
  DEVICE_MODEL := R6260
  SERCOMM_HWNAME := R6260
  SERCOMM_HWID := CHJ
  SERCOMM_HWVER := A001
  SERCOMM_SWVER := 0x0052
  IMAGE_SIZE := 40960k
  DEVICE_PACKAGES += kmod-mt7615e
endef
TARGET_DEVICES += netgear_r6260

define Device/netgear_r6350
  $(Device/netgear_sercomm_nand)
  DEVICE_MODEL := R6350
  SERCOMM_HWNAME := R6350
  SERCOMM_HWID := CHJ
  SERCOMM_HWVER := A001
  SERCOMM_SWVER := 0x0052
  IMAGE_SIZE := 40960k
  DEVICE_PACKAGES += kmod-mt7615e
endef
TARGET_DEVICES += netgear_r6350

define Device/netgear_r6850
  $(Device/netgear_sercomm_nand)
  DEVICE_MODEL := R6850
  SERCOMM_HWNAME := R6850
  SERCOMM_HWID := CHJ
  SERCOMM_HWVER := A001
  SERCOMM_SWVER := 0x0052
  IMAGE_SIZE := 40960k
  DEVICE_PACKAGES += kmod-mt7615e
endef
TARGET_DEVICES += netgear_r6850

define Device/netgear_wndr3700-v5
  MTK_SOC := mt7621
  BLOCKSIZE := 64k
  IMAGE_SIZE := 15232k
  SERCOMM_HWID := AYB
  SERCOMM_HWVER := A001
  SERCOMM_SWVER := 0x1054
  IMAGES += factory.img
  IMAGE/default := append-kernel | pad-to $$$$(BLOCKSIZE) | append-rootfs | pad-rootfs
  IMAGE/sysupgrade.bin := $$(IMAGE/default) | append-metadata | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.img := pad-extra 320k | $$(IMAGE/default) | pad-to $$$$(BLOCKSIZE) | \
	sercom-footer | pad-to 128 | zip WNDR3700v5.bin | sercom-seal
  DEVICE_VENDOR := NETGEAR
  DEVICE_MODEL := WNDR3700
  DEVICE_VARIANT := v5
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += wndr3700v5
endef
TARGET_DEVICES += netgear_wndr3700-v5

define Device/netis_wf-2881
  MTK_SOC := mt7621
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  FILESYSTEMS := squashfs
  IMAGE_SIZE := 129280k
  KERNEL := $(KERNEL_DTB) | pad-offset $$(BLOCKSIZE) 64 | uImage lzma
  UBINIZE_OPTS := -E 5
  IMAGE/sysupgrade.bin := append-kernel | append-ubi | append-metadata | check-size $$$$(IMAGE_SIZE)
  DEVICE_VENDOR := NETIS
  DEVICE_MODEL := WF-2881
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += wf-2881
endef
TARGET_DEVICES += netis_wf-2881

define Device/phicomm_k2p
  MTK_SOC := mt7621
  IMAGE_SIZE := 15744k
  DEVICE_VENDOR := Phicomm
  DEVICE_MODEL := K2P
  DEVICE_ALT0_VENDOR := Phicomm
  DEVICE_ALT0_MODEL := KE 2P
  SUPPORTED_DEVICES += k2p
  DEVICE_PACKAGES := kmod-mt7615e wpad-basic
endef
TARGET_DEVICES += phicomm_k2p

define Device/planex_vr500
  MTK_SOC := mt7621
  IMAGE_SIZE := 65216k
  DEVICE_VENDOR := Planex
  DEVICE_MODEL := VR500
  DEVICE_PACKAGES := kmod-usb3
  SUPPORTED_DEVICES += vr500
endef
TARGET_DEVICES += planex_vr500

define Device/samknows_whitebox-v8
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := SamKnows
  DEVICE_MODEL := Whitebox 8
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport \
	uboot-envtools wpad-basic
  SUPPORTED_DEVICES += sk-wb8
endef
TARGET_DEVICES += samknows_whitebox-v8

define Device/storylink_sap-g3200u3
  MTK_SOC := mt7621
  IMAGE_SIZE := 7872k
  DEVICE_VENDOR := STORYLiNK
  DEVICE_MODEL := SAP-G3200U3
  DEVICE_PACKAGES := kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += sap-g3200u3
endef
TARGET_DEVICES += storylink_sap-g3200u3

define Device/telco-electronics_x1
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Telco Electronics
  DEVICE_MODEL := X1
  DEVICE_PACKAGES := kmod-usb3 kmod-mt76 wpad-basic
endef
TARGET_DEVICES += telco-electronics_x1

define Device/thunder_timecloud
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Thunder
  DEVICE_MODEL := Timecloud
  DEVICE_PACKAGES := kmod-usb3
  SUPPORTED_DEVICES += timecloud
endef
TARGET_DEVICES += thunder_timecloud

define Device/totolink_a7000r
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  UIMAGE_NAME := C8340R1C-9999
  DEVICE_VENDOR := TOTOLINK
  DEVICE_MODEL := A7000R
  DEVICE_PACKAGES := kmod-mt7615e wpad-basic
endef
TARGET_DEVICES += totolink_a7000r

define Device/adslr_g7
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := ADSLR
  DEVICE_MODEL := G7
  DEVICE_PACKAGES := kmod-mt7615e wpad-basic
endef
TARGET_DEVICES += adslr_g7

define Device/tplink-safeloader
  MTK_SOC := mt7621
  DEVICE_VENDOR := TP-Link
  TPLINK_BOARD_ID :=
  TPLINK_HWID := 0x0
  TPLINK_HWREV := 0
  TPLINK_HEADER_VERSION := 1
  KERNEL := $(KERNEL_DTB) | tplink-v1-header -e -O
  IMAGES += factory.bin
  IMAGE/sysupgrade.bin := append-rootfs | tplink-safeloader sysupgrade | \
	append-metadata | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := append-rootfs | tplink-safeloader factory
endef

define Device/tplink_re350-v1
  $(Device/tplink-safeloader)
  DEVICE_MODEL := RE350
  DEVICE_VARIANT := v1
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 wpad-basic
  TPLINK_BOARD_ID := RE350-V1
  IMAGE_SIZE := 6016k
  SUPPORTED_DEVICES += re350-v1
endef
TARGET_DEVICES += tplink_re350-v1

define Device/tplink_re650-v1
  $(Device/tplink-safeloader)
  DEVICE_MODEL := RE650
  DEVICE_VARIANT := v1
  DEVICE_PACKAGES := kmod-mt7615e wpad-basic
  TPLINK_BOARD_ID := RE650-V1
  IMAGE_SIZE := 14208k
endef
TARGET_DEVICES += tplink_re650-v1

define Device/ubiquiti_edgerouterx
  MTK_SOC := mt7621
  IMAGE_SIZE := 256768k
  FILESYSTEMS := squashfs
  KERNEL_SIZE := 3145728
  KERNEL_INITRAMFS := $$(KERNEL) | ubnt-erx-factory-image $(KDIR)/tmp/$$(KERNEL_INITRAMFS_PREFIX)-factory.tar
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  DEVICE_VENDOR := Ubiquiti
  DEVICE_MODEL := EdgeRouter X
  SUPPORTED_DEVICES += ubnt-erx
endef
TARGET_DEVICES += ubiquiti_edgerouterx

define Device/ubiquiti_edgerouterx-sfp
  $(Device/ubiquiti_edgerouterx)
  DEVICE_VENDOR := Ubiquiti
  DEVICE_MODEL := EdgeRouter X-SFP
  DEVICE_PACKAGES += kmod-i2c-algo-pca kmod-gpio-pca953x kmod-i2c-gpio-custom
  SUPPORTED_DEVICES += ubnt-erx-sfp
endef
TARGET_DEVICES += ubiquiti_edgerouterx-sfp

define Device/unielec_u7621-06-16m
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := UniElec
  DEVICE_MODEL := U7621-06
  DEVICE_VARIANT := 16M
  DEVICE_PACKAGES := kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620 kmod-usb3
  SUPPORTED_DEVICES += u7621-06-256M-16M unielec,u7621-06-256m-16m
endef
TARGET_DEVICES += unielec_u7621-06-16m

define Device/unielec_u7621-06-64m
  MTK_SOC := mt7621
  IMAGE_SIZE := 65216k
  DEVICE_VENDOR := UniElec
  DEVICE_MODEL := U7621-06
  DEVICE_VARIANT := 64M
  DEVICE_PACKAGES := kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620 kmod-usb3
  SUPPORTED_DEVICES += unielec,u7621-06-512m-64m
endef
TARGET_DEVICES += unielec_u7621-06-64m

define Device/wevo_11acnas
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := WeVO
  DEVICE_MODEL := 11AC NAS Router
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += 11acnas
endef
TARGET_DEVICES += wevo_11acnas

define Device/wevo_w2914ns-v2
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := WeVO
  DEVICE_MODEL := W2914NS
  DEVICE_VARIANT := v2
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += w2914nsv2
endef
TARGET_DEVICES += wevo_w2914ns-v2

define Device/xiaomi_mir3g
  MTK_SOC := mt7621
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 4096k
  IMAGE_SIZE := 124416k
  UBINIZE_OPTS := -E 5
  IMAGES += kernel1.bin rootfs0.bin
  IMAGE/kernel1.bin := append-kernel
  IMAGE/rootfs0.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  DEVICE_VENDOR := Xiaomi
  DEVICE_MODEL := Mi Router 3G
  SUPPORTED_DEVICES += R3G
  SUPPORTED_DEVICES += mir3g
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic \
	uboot-envtools
endef
TARGET_DEVICES += xiaomi_mir3g

define Device/xiaomi_mir3g-v2
  MTK_SOC := mt7621
  IMAGE_SIZE := 14848k
  DEVICE_VENDOR := Xiaomi
  DEVICE_MODEL := Mi Router 3G
  DEVICE_VARIANT := v2
  DEVICE_ALT0_VENDOR := Xiaomi
  DEVICE_ALT0_MODEL := Mi Router 4A
  DEVICE_ALT0_VARIANT := Gigabit Edition
  DEVICE_PACKAGES := kmod-mt7603 kmod-mt76x2 wpad-basic
endef
TARGET_DEVICES += xiaomi_mir3g-v2

define Device/xiaomi_mir3p
  MTK_SOC := mt7621
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE:= 4096k
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 255488k
  DEVICE_VENDOR := Xiaomi
  DEVICE_MODEL := Mi Router 3 Pro
  IMAGES += factory.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory.bin := append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi | check-size $$$$(IMAGE_SIZE)
  DEVICE_PACKAGES := \
	kmod-mt7615e kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic \
	uboot-envtools
endef
TARGET_DEVICES += xiaomi_mir3p

define Device/xiaoyu_xy-c5
  MTK_SOC := mt7621
  IMAGE_SIZE := 32448k
  DEVICE_VENDOR := XiaoYu
  DEVICE_MODEL := XY-C5
  DEVICE_PACKAGES := kmod-ata-core kmod-ata-ahci kmod-usb3
endef
TARGET_DEVICES += xiaoyu_xy-c5

define Device/xzwifi_creativebox-v1
  MTK_SOC := mt7621
  IMAGE_SIZE := 32448k
  DEVICE_VENDOR := CreativeBox
  DEVICE_MODEL := v1
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-mt7603 kmod-mt76x2 kmod-sdhci-mt7620 \
	kmod-usb3
endef
TARGET_DEVICES += xzwifi_creativebox-v1

define Device/youhua_wr1200js
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := YouHua
  DEVICE_MODEL := WR1200JS
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += youhua_wr1200js

define Device/youku_yk-l2
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Youku
  DEVICE_MODEL := YK-L2
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += youku_yk-l2

define Device/zbtlink_zbt-we1326
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Zbtlink
  DEVICE_MODEL := ZBT-WE1326
  DEVICE_PACKAGES := \
	kmod-mt7603 kmod-mt76x2 kmod-usb3 kmod-sdhci-mt7620 wpad-basic
  SUPPORTED_DEVICES += zbt-we1326
endef
TARGET_DEVICES += zbtlink_zbt-we1326

define Device/zbtlink_zbt-we3526
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Zbtlink
  DEVICE_MODEL := ZBT-WE3526
  DEVICE_PACKAGES := \
	kmod-sdhci-mt7620 kmod-mt7603 kmod-mt76x2 \
	kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
endef
TARGET_DEVICES += zbtlink_zbt-we3526

define Device/zbtlink_zbt-wg2626
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Zbtlink
  DEVICE_MODEL := ZBT-WG2626
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620 kmod-mt76x2 kmod-usb3 \
	kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += zbt-wg2626
endef
TARGET_DEVICES += zbtlink_zbt-wg2626

define Device/zbtlink_zbt-wg3526-16m
  MTK_SOC := mt7621
  IMAGE_SIZE := 16064k
  DEVICE_VENDOR := Zbtlink
  DEVICE_MODEL := ZBT-WG3526
  DEVICE_VARIANT := 16M
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620 kmod-mt7603 kmod-mt76x2 \
	kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += zbt-wg3526 zbt-wg3526-16M
endef
TARGET_DEVICES += zbtlink_zbt-wg3526-16m

define Device/zbtlink_zbt-wg3526-32m
  MTK_SOC := mt7621
  IMAGE_SIZE := 32448k
  DEVICE_VENDOR := Zbtlink
  DEVICE_MODEL := ZBT-WG3526
  DEVICE_VARIANT := 32M
  DEVICE_PACKAGES := \
	kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620 kmod-mt7603 kmod-mt76x2 \
	kmod-usb3 kmod-usb-ledtrig-usbport wpad-basic
  SUPPORTED_DEVICES += ac1200pro zbt-wg3526-32M
endef
TARGET_DEVICES += zbtlink_zbt-wg3526-32m
