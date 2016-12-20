#
# RT288X Profiles
#

define Build/gemtek-header
	mkheader_gemtek $@ $@.new $(1) && mv $@.new $@
endef

define Device/ar670w
  DTS := AR670W
  BLOCKSIZE := 64k
  DEVICE_TITLE := Airlink AR670W
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  KERNEL := $(KERNEL_DTB)
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	wrg-header wrgn16a_airlink_ar670w
endef
TARGET_DEVICES += ar670w

define Device/ar725w
  DTS := AR725W
  DEVICE_TITLE := Gemtek AR725W
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | check-size $$$$(IMAGE_SIZE) | \
	gemtek-header ar725w
endef
TARGET_DEVICES += ar725w

define Device/f5d8235v1
  DTS := F5D8235_V1
  IMAGE_SIZE := 7744k
  DEVICE_TITLE := Belkin F5D8235 V1
  DEVICE_PACKAGES := kmod-switch-rtl8366s kmod-usb-core kmod-usb-ohci \
    kmod-usb-ohci-pci kmod-usb2 kmod-usb2-pci kmod-usb-ledtrig-usbport
endef
TARGET_DEVICES += f5d8235v1

define Device/rt-n15
  DTS := RT-N15
  BLOCKSIZE := 64k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  DEVICE_TITLE := Asus RT-N15
  DEVICE_PACKAGES := kmod-switch-rtl8366s
endef
TARGET_DEVICES += rt-n15

define Device/v11st-fe
  DTS := V11STFE
  BLOCKSIZE := 64k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  DEVICE_TITLE := Ralink V11ST-FE
endef
TARGET_DEVICES += v11st-fe

define Device/wli-tx4-ag300n
  DTS := WLI-TX4-AG300N
  BLOCKSIZE := 64k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  DEVICE_TITLE := Buffalo WLI-TX4-AG300N
endef
TARGET_DEVICES += wli-tx4-ag300n

define Device/wzr-agl300nh
  DTS := WZR-AGL300NH
  BLOCKSIZE := 64k
  IMAGE_SIZE := $(ralink_default_fw_size_4M)
  DEVICE_TITLE := Buffalo WZR-AGL300NH
  DEVICE_PACKAGES := kmod-switch-rtl8366s
endef
TARGET_DEVICES += wzr-agl300nh
