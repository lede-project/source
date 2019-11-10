include ./common-tp-link.mk

define Device/tplink_tl-mr10u
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9331
  DEVICE_MODEL := TL-MR10U
  DEVICE_PACKAGES := kmod-usb-chipidea2
  TPLINK_HWID := 0x00100101
  SUPPORTED_DEVICES += tl-mr10u
endef
TARGET_DEVICES += tplink_tl-mr10u

define Device/tplink_tl-mr3020-v1
  $(Device/tplink-4mlzma)
  IMAGE_SIZE := 3840k
  ATH_SOC := ar9331
  DEVICE_MODEL := TL-MR3020
  DEVICE_VARIANT := v1
  DEVICE_PACKAGES := kmod-usb-chipidea2 kmod-usb-ledtrig-usbport
  TPLINK_HWID := 0x30200001
  SUPPORTED_DEVICES += tl-mr3020
endef
TARGET_DEVICES += tplink_tl-mr3020-v1

define Device/tplink_tl-mr3040-v2
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9331
  DEVICE_MODEL := TL-MR3040
  DEVICE_VARIANT := v2
  DEVICE_PACKAGES := kmod-usb-chipidea2 kmod-usb-ledtrig-usbport
  TPLINK_HWID := 0x30400002
  SUPPORTED_DEVICES += tl-mr3040-v2
endef
TARGET_DEVICES += tplink_tl-mr3040-v2

define Device/tplink_tl-mr3220-v1
  $(Device/tplink-4m)
  ATH_SOC := ar7241
  DEVICE_MODEL := TL-MR3220
  DEVICE_VARIANT := v1
  TPLINK_HWID := 0x32200001
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ledtrig-usbport
  SUPPORTED_DEVICES += tl-mr3220-v1
endef
TARGET_DEVICES += tplink_tl-mr3220-v1

define Device/tplink_tl-mr3420-v1
  $(Device/tplink-4m)
  ATH_SOC := ar7241
  DEVICE_MODEL := TL-MR3420
  DEVICE_VARIANT := v1
  TPLINK_HWID := 0x34200001
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ledtrig-usbport
  SUPPORTED_DEVICES += tl-mr3420-v1
endef
TARGET_DEVICES += tplink_tl-mr3420-v1

define Device/tplink_tl-mr3420-v2
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9341
  DEVICE_MODEL := TL-MR3420
  DEVICE_VARIANT := v2
  TPLINK_HWID := 0x34200002
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-ledtrig-usbport
  SUPPORTED_DEVICES += tl-mr3420-v2
endef
TARGET_DEVICES += tplink_tl-mr3420-v2

define Device/tplink_tl-wa901nd-v2
  $(Device/tplink-4m)
  ATH_SOC := ar9132
  DEVICE_MODEL := TL-WA901ND
  DEVICE_VARIANT := v2
  TPLINK_HWID := 0x09010002
endef
TARGET_DEVICES += tplink_tl-wa901nd-v2

define Device/tplink_tl-wr703n
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9331
  DEVICE_MODEL := TL-WR703N
  DEVICE_PACKAGES := kmod-usb-chipidea2
  TPLINK_HWID := 0x07030101
  SUPPORTED_DEVICES += tl-wr703n
endef
TARGET_DEVICES += tplink_tl-wr703n

define Device/tplink_tl-wr740n-v1
  $(Device/tplink-4m)
  ATH_SOC := ar7240
  DEVICE_MODEL := TL-WR740N
  DEVICE_VARIANT := v1/v2
  TPLINK_HWID := 0x07400001
endef
TARGET_DEVICES += tplink_tl-wr740n-v1

define Device/tplink_tl-wr740n-v3
  $(Device/tplink-4m)
  ATH_SOC := ar7240
  DEVICE_MODEL := TL-WR740N
  DEVICE_VARIANT := v3
  TPLINK_HWID := 0x07400003
endef
TARGET_DEVICES += tplink_tl-wr740n-v3

define Device/tplink_tl-wr740n-v4
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9331
  DEVICE_MODEL := TL-WR740N
  DEVICE_VARIANT := v4
  TPLINK_HWID := 0x07400004
  SUPPORTED_DEVICES += tl-wr740n-v4
endef
TARGET_DEVICES += tplink_tl-wr740n-v4

define Device/tplink_tl-wr741-v1
  $(Device/tplink-4m)
  ATH_SOC := ar7240
  DEVICE_MODEL := TL-WR741N/ND
  DEVICE_VARIANT := v1/v2
  TPLINK_HWID := 0x07410001
endef
TARGET_DEVICES += tplink_tl-wr741-v1

define Device/tplink_tl-wr741nd-v4
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9331
  DEVICE_MODEL := TL-WR741N/ND
  DEVICE_VARIANT := v4
  TPLINK_HWID := 0x07410004
  SUPPORTED_DEVICES += tl-wr741n-v4
endef
TARGET_DEVICES += tplink_tl-wr741nd-v4

define Device/tplink_tl-wr743nd-v1
  $(Device/tplink-4m)
  ATH_SOC := ar7240
  DEVICE_MODEL := TL-WR743ND
  DEVICE_VARIANT := v1
  TPLINK_HWID := 0x07430001
endef
TARGET_DEVICES += tplink_tl-wr743nd-v1

define Device/tplink_tl-wr841-v5
  $(Device/tplink-4m)
  ATH_SOC := ar7240
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v5/v6
  TPLINK_HWID := 0x08410005
endef
TARGET_DEVICES += tplink_tl-wr841-v5

define Device/tplink_tl-wr841-v7
  $(Device/tplink-4m)
  ATH_SOC := ar7241
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v7
  TPLINK_HWID := 0x08410007
  SUPPORTED_DEVICES += tl-wr841-v7
endef
TARGET_DEVICES += tplink_tl-wr841-v7

define Device/tplink_tl-wr841-v8
  $(Device/tplink-4mlzma)
  ATH_SOC := ar9341
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v8
  TPLINK_HWID := 0x08410008
  SUPPORTED_DEVICES += tl-wr841n-v8
endef
TARGET_DEVICES += tplink_tl-wr841-v8

define Device/tplink_tl-wr841-v9
  $(Device/tplink-4mlzma)
  ATH_SOC := qca9533
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v9
  TPLINK_HWID := 0x08410009
  SUPPORTED_DEVICES += tl-wr841n-v9
endef
TARGET_DEVICES += tplink_tl-wr841-v9

define Device/tplink_tl-wr841-v10
  $(Device/tplink-4mlzma)
  ATH_SOC := qca9533
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v10
  TPLINK_HWID := 0x08410010
  SUPPORTED_DEVICES += tl-wr841n-v9
endef
TARGET_DEVICES += tplink_tl-wr841-v10

define Device/tplink_tl-wr841-v11
  $(Device/tplink-4mlzma)
  ATH_SOC := qca9533
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v11
  TPLINK_HWID := 0x08410011
  SUPPORTED_DEVICES += tl-wr841n-v11
  IMAGES += factory-us.bin factory-eu.bin
  IMAGE/factory-us.bin := append-rootfs | mktplinkfw factory -C US
  IMAGE/factory-eu.bin := append-rootfs | mktplinkfw factory -C EU
endef
TARGET_DEVICES += tplink_tl-wr841-v11

define Device/tplink_tl-wr841-v12
  $(Device/tplink-4mlzma)
  ATH_SOC := qca9533
  DEVICE_MODEL := TL-WR841N/ND
  DEVICE_VARIANT := v12
  TPLINK_HWID := 0x08410012
  SUPPORTED_DEVICES += tl-wr841n-v11
  IMAGES += factory-us.bin factory-eu.bin
  IMAGE/factory-us.bin := append-rootfs | mktplinkfw factory -C US
  IMAGE/factory-eu.bin := append-rootfs | mktplinkfw factory -C EU
endef
TARGET_DEVICES += tplink_tl-wr841-v12

define Device/tplink_tl-wr940n-v3
  $(Device/tplink-4mlzma)
  ATH_SOC := tp9343
  DEVICE_MODEL := TL-WR940N
  DEVICE_VARIANT := v3
  TPLINK_HWID := 0x09410006
  SUPPORTED_DEVICES += tl-wr941nd-v6
endef
TARGET_DEVICES += tplink_tl-wr940n-v3

define Device/tplink_tl-wr940n-v4
  $(Device/tplink-4mlzma)
  ATH_SOC := tp9343
  DEVICE_MODEL := TL-WR940N
  DEVICE_VARIANT := v4
  TPLINK_HWID := 0x09400004
  SUPPORTED_DEVICES += tl-wr940n-v4
  IMAGES += factory-us.bin factory-eu.bin factory-br.bin
  IMAGE/factory-us.bin := append-rootfs | mktplinkfw factory -C US
  IMAGE/factory-eu.bin := append-rootfs | mktplinkfw factory -C EU
  IMAGE/factory-br.bin := append-rootfs | mktplinkfw factory -C BR
endef
TARGET_DEVICES += tplink_tl-wr940n-v4

define Device/tplink_tl-wr941-v2
  $(Device/tplink-4m)
  ATH_SOC := ar9132
  DEVICE_MODEL := TL-WR941ND
  DEVICE_VARIANT := v2/v3
  DEVICE_ALT0_VENDOR := TP-Link
  DEVICE_ALT0_MODEL := TL-WR941N
  DEVICE_ALT0_VARIANT := v2/v3
  TPLINK_HWID := 0x09410002
  TPLINK_HWREV := 2
endef
TARGET_DEVICES += tplink_tl-wr941-v2

define Device/tplink_tl-wr941-v4
  $(Device/tplink-4m)
  ATH_SOC := ar7240
  DEVICE_MODEL := TL-WR941ND
  DEVICE_VARIANT := v4
  DEVICE_ALT0_VENDOR := TP-Link
  DEVICE_ALT0_MODEL := TL-WR941N
  DEVICE_ALT0_VARIANT := v4
  TPLINK_HWID := 0x09410004
endef
TARGET_DEVICES += tplink_tl-wr941-v4

define Device/tplink_tl-wr941nd-v6
  $(Device/tplink-4mlzma)
  ATH_SOC := tp9343
  DEVICE_MODEL := TL-WR941ND
  DEVICE_VARIANT := v6
  TPLINK_HWID := 0x09410006
  SUPPORTED_DEVICES += tl-wr941nd-v6
endef
TARGET_DEVICES += tplink_tl-wr941nd-v6

define Device/tplink_tl-wr941n-v7-cn
  $(Device/tplink-4mlzma)
  ATH_SOC := qca9558
  DEVICE_MODEL := TL-WR941N
  DEVICE_VARIANT := v7 (CN)
  TPLINK_HWID := 0x09410007
endef
TARGET_DEVICES += tplink_tl-wr941n-v7-cn
