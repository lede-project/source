#
# Copyright (C) 2016 Jiang Yutang <yutang.jiang@nxp.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Build/append-ls-rcw
	rm -f $@
	dd if=$(KDIR)/rcw.bin >> $@
endef

define Build/append-ls-uboot
	if [ 32b = $(SUBTARGET) ]; then \
		$(CP) $(BIN_DIR)/../64b/$(1)-u-boot-dtb.bin $(KDIR)/ ; \
	fi;
	dd if=$(KDIR)/$(1)-u-boot-dtb.bin >> $@
endef

define Build/append-ls-fman
	dd if=$(KDIR)/fman_ucode.bin >> $@
endef

define Build/append-ls-dtb
	$(call Image/BuildDTB,$(DTS_DIR)/$(1).dts,$(DTS_DIR)/$(1).dtb)
	dd if=$(DTS_DIR)/$(1).dtb >> $@
endef

define Device/Default
  FILESYSTEMS := squashfs
  KERNEL := kernel-bin | gzip | uImage gzip
  DEVICE_DTS :=
  IMAGES = firmware.bin

ifeq ($(SUBTARGET),64b)
  KERNEL_LOADADDR = 0x80080000
  KERNEL_ENTRY_POINT = 0x80080000
endif
ifeq ($(SUBTARGET),32b)
  KERNEL_LOADADDR = 0x80008000
  KERNEL_ENTRY_POINT = 0x80008000
endif
endef

define Device/ls1043ardb
  DEVICE_TITLE := ls1043ardb
  DEVICE_PACKAGES += uboot-layerscape-ls1043ardb fman-ucode
ifeq ($(SUBTARGET),64b)
  DEVICE_DTS = freescale/fsl-ls1043a-rdb
endif
ifeq ($(SUBTARGET),32b)
  DEVICE_DTS = ../../../arm64/boot/dts/freescale/fsl-ls1043a-rdb
endif
  IMAGE/firmware.bin = append-ls-rcw | pad-to 1M | append-ls-uboot $(1) | pad-to 3M | \
  					append-ls-fman | pad-to 4M | append-ls-dtb $$(DEVICE_DTS) | pad-to 5M | \
  					append-kernel | pad-to 10M | append-rootfs | pad-to 64M | check-size 67108865
endef
TARGET_DEVICES += ls1043ardb

$(eval $(call BuildImage))
