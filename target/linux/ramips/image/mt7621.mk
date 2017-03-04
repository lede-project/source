#
# MT7621 Profiles
#

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

define Device/11acnas
  DTS := 11ACNAS
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := WeVO 11AC NAS Router
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-mt76
endef
TARGET_DEVICES += 11acnas

define Device/ac1200pro
  DTS := AC1200pro
  IMAGE_SIZE := $(ralink_default_fw_size_32M)
  DEVICE_TITLE := Digineo AC1200 Pro
  DEVICE_PACKAGES := kmod-usb3 kmod-ata-core kmod-ata-ahci
endef
TARGET_DEVICES += ac1200pro

define Device/dir-860l-b1
  DTS := DIR-860L-B1
  BLOCKSIZE := 64k
  IMAGES += factory.bin
  KERNEL := kernel-bin | patch-dtb | relocate-kernel | lzma | uImage lzma
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  IMAGE/sysupgrade.bin := \
	append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | append-rootfs | \
	seama -m "dev=/dev/mtdblock/2" -m "type=firmware" | \
	pad-rootfs | append-metadata | check-size $$$$(IMAGE_SIZE)
  IMAGE/factory.bin := \
	append-kernel | pad-offset $$$$(BLOCKSIZE) 64 | \
	append-rootfs | pad-rootfs -x 64 | \
	seama -m "dev=/dev/mtdblock/2" -m "type=firmware" | \
	seama-seal -m "signature=wrgac13_dlink.2013gui_dir860lb" | \
	check-size $$$$(IMAGE_SIZE)
  DEVICE_TITLE := D-Link DIR-860L B1
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += dir-860l-b1

define Device/ew1200
  DTS := EW1200
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := AFOUNDRY EW1200
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-ata-core kmod-ata-ahci
endef
TARGET_DEVICES += ew1200

define Device/firewrt
  DTS := FIREWRT
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Firefly FireWRT
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += firewrt

define Device/hc5962
  DTS := HC5962
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  KERNEL_SIZE := 2097152
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := $(ralink_default_fw_size_32M)
  IMAGES += factory.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory.bin := append-kernel | pad-to $$(KERNEL_SIZE) | append-ubi | check-size $$$$(IMAGE_SIZE)
  DEVICE_TITLE := HiWiFi HC5962
  DEVICE_PACKAGES := kmod-usb3 kmod-mt76
endef
TARGET_DEVICES += hc5962

define Device/mt7621
  DTS := MT7621
  BLOCKSIZE := 64k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  DEVICE_TITLE := MediaTek MT7621 EVB
endef
TARGET_DEVICES += mt7621

define Device/newifi-d1
  DTS := Newifi-D1
  IMAGE_SIZE := $(ralink_default_fw_size_32M)
  DEVICE_TITLE := Newifi D1
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += newifi-d1

define Device/pbr-m1
  DTS := PBR-M1
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := PBR-M1
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620
endef
TARGET_DEVICES += pbr-m1

define Device/rb750gr3
  DTS := RB750Gr3
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := MikroTik RB750Gr3
  DEVICE_PACKAGES := kmod-usb3 uboot-envtools -kmod-mt76 -kmod-rt2x00-lib -kmod-mac80211 -kmod-cfg80211 -wpad-mini -iwinfo
endef
TARGET_DEVICES += rb750gr3

define Device/re6500
  DTS := RE6500
  DEVICE_TITLE := Linksys RE6500
endef
TARGET_DEVICES += re6500

define Device/sap-g3200u3
  DTS := SAP-G3200U3
  DEVICE_TITLE := STORYLiNK SAP-G3200U3
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += sap-g3200u3

define Device/sk-wb8
  DTS := SK-WB8
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := SamKnows Whitebox 8
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport uboot-envtools
endef
TARGET_DEVICES += sk-wb8

define Device/timecloud
  DTS := Timecloud
  DEVICE_TITLE := Thunder Timecloud
  DEVICE_PACKAGES := kmod-usb3
endef
TARGET_DEVICES += timecloud

define Device/ubnt-erx
  DTS := UBNT-ERX
  FILESYSTEMS := squashfs
  KERNEL_SIZE := 3145728
  KERNEL := $(KERNEL_DTB) | uImage lzma
  IMAGES := sysupgrade.tar
  KERNEL_INITRAMFS := $$(KERNEL) | ubnt-erx-factory-image $(KDIR)/tmp/$$(KERNEL_INITRAMFS_PREFIX)-factory.tar
  IMAGE/sysupgrade.tar := sysupgrade-tar | append-metadata
  DEVICE_TITLE := Ubiquiti EdgeRouter X
  DEVICE_PACKAGES := -kmod-mt76 -kmod-rt2x00-lib -kmod-mac80211 -kmod-cfg80211 -wpad-mini -iwinfo
endef
TARGET_DEVICES += ubnt-erx

define Device/vr500
  DTS := VR500
  IMAGE_SIZE := 66453504
  DEVICE_TITLE := Planex VR500
  DEVICE_PACKAGES := kmod-usb3
endef
TARGET_DEVICES += vr500

define Device/w2914nsv2
  DTS := W2914NSV2
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := WeVO W2914NS v2
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-mt76
endef
TARGET_DEVICES += w2914nsv2

define Device/wf-2881
  DTS := WF-2881
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  FILESYSTEMS := squashfs
  IMAGE_SIZE := 129280k
  KERNEL := $(KERNEL_DTB) | pad-offset $$(BLOCKSIZE) 64 | uImage lzma
  UBINIZE_OPTS := -E 5
  IMAGE/sysupgrade.bin := append-kernel | append-ubi | append-metadata | check-size $$$$(IMAGE_SIZE)
  DEVICE_TITLE := NETIS WF-2881
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += wf-2881

define Device/witi
  DTS := WITI
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := MQmaker WiTi
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620
endef
TARGET_DEVICES += witi

define Device/wndr3700v5
  DTS := WNDR3700V5
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Netgear WNDR3700v5
  DEVICE_PACKAGES := kmod-usb3
endef
TARGET_DEVICES += wndr3700v5

define Device/wsr-1166
  DTS := WSR-1166
  IMAGE/sysupgrade.bin := trx | pad-rootfs | append-metadata
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Buffalo WSR-1166
endef
TARGET_DEVICES += wsr-1166

define Device/wsr-600
  DTS := WSR-600
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := Buffalo WSR-600
endef
TARGET_DEVICES += wsr-600

define Device/zbt-wg2626
  DTS := ZBT-WG2626
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := ZBT WG2626
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620
endef
TARGET_DEVICES += zbt-wg2626

define Device/zbt-wg3526
  DTS := ZBT-WG3526
  IMAGE_SIZE := $(ralink_default_fw_size_16M)
  DEVICE_TITLE := ZBT WG3526
  DEVICE_PACKAGES := kmod-usb3 kmod-usb-ledtrig-usbport kmod-ata-core kmod-ata-ahci kmod-sdhci-mt7620
endef
TARGET_DEVICES += zbt-wg3526

# FIXME: is this still needed?
define Image/Prepare
#define Build/Compile
	rm -rf $(KDIR)/relocate
	$(CP) ../../generic/image/relocate $(KDIR)
	$(MAKE) -C $(KDIR)/relocate KERNEL_ADDR=$(KERNEL_LOADADDR) CROSS_COMPILE=$(TARGET_CROSS)
	$(CP) $(KDIR)/relocate/loader.bin $(KDIR)/loader.bin
endef
