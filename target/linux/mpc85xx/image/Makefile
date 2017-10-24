#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

DEVICE_VARS += TPLINK_HWID TPLINK_HWREV TPLINK_FLASHLAYOUT

define Device/Default
  PROFILES := Default
  KERNEL_DEPENDS = $$(wildcard $(DTS_DIR)/$$(DEVICE_DTS).dts)
  DEVICE_DTS :=
  KERNEL_ENTRY := 0x00000000
  KERNEL_LOADADDR := 0x00000000
  KERNEL := kernel-bin
endef

define Build/append-empty-initrd
	-$(STAGING_DIR_HOST)/bin/mkimage \
		-A $(LINUX_KARCH) -O linux -T ramdisk -C none \
		-n "empty initrd" \
		-s \
		$(KDIR_TMP)/$(IMG_PREFIX)-empty-initrd.img

	-dd if=$(KDIR_TMP)/$(IMG_PREFIX)-empty-initrd.img >> $@
endef

define Build/tplink-v1-image
	$(STAGING_DIR_HOST)/bin/mktplinkfw \
		-H $(TPLINK_HWID) -W $(TPLINK_HWREV) -F $(TPLINK_FLASHLAYOUT) \
		-N "$(VERSION_DIST)" -V $(REVISION) $(1) \
		-k $@ \
		-r $(KDIR)/root.squashfs \
		-o $@.new
	@mv $@.new $@
endef

ifeq ($(SUBTARGET),generic)

define Device/TLWDR4900
  DEVICE_TITLE := TP-Link TL-WDR4900
  DEVICE_PACKAGES := kmod-ath9k wpad-mini
  DEVICE_DTS := tl-wdr4900-v1
  BLOCKSIZE := 128k
  TPLINK_HWID := 0x49000001
  TPLINK_HWREV := 1
  TPLINK_FLASHLAYOUT := 16Mppc
  KERNEL_NAME := cuImage
  KERNEL := append-dtb | pad-to $$(BLOCKSIZE) | append-kernel
  IMAGES := sysupgrade.img factory.img
  IMAGE/factory.img := append-kernel | tplink-v1-image
  IMAGE/sysupgrade.img := append-kernel | tplink-v1-image -s
endef
TARGET_DEVICES += TLWDR4900

endif

ifeq ($(SUBTARGET),p1020)

define Device/hiveap-330
  DEVICE_TITLE := Aerohive HiveAP-330
  DEVICE_PACKAGES := kmod-ath9k wpad-mini kmod-tpm-i2c-atmel
  DEVICE_DTS := hiveap-330
  BLOCKSIZE := 128k
  KERNEL_NAME := zImage
  KERNEL_SIZE := 8m
  IMAGES := devicetree.dtb sysupgrade.img
  IMAGE/devicetree.dtb := append-dtb
  IMAGE/sysupgrade.img := append-dtb | pad-to 256k | check-size 256k | \
	append-empty-initrd | pad-to 256k | check-size 512k | \
	append-rootfs | pad-rootfs $$(BLOCKSIZE) | pad-to 41216k | check-size 41216k | \
	append-kernel
endef
TARGET_DEVICES += hiveap-330

endif

$(eval $(call BuildImage))
