#
# Copyright (C) 2016 Jiang Yutang <yutang.jiang@nxp.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Build/mk_firmware
	$(call Image/BuildDTB,$(DTS_DIR)/$(word 1,$(1)).dts,$(DTS_DIR)/$(word 1,$(1)).dtb)
	dd if=/dev/zero bs=$(word 2,$(1)) count=1 | sed 's/\x00/\xff/g' > $(DTS_DIR)/$(word 1,$(1)).dtb.par
	dd if=$(DTS_DIR)/$(word 1,$(1)).dtb of=$(DTS_DIR)/$(word 1,$(1)).dtb.par conv=notrunc

	dd if=/dev/zero bs=$(word 3,$(1)) count=1 | sed 's/\x00/\xff/g' > $(KDIR)/$(word 5,$(1))$(KERNEL_SUFFIX).par
	dd if=$(KDIR)/$(word 5,$(1))$(KERNEL_SUFFIX) of=$(KDIR)/$(word 5,$(1))$(KERNEL_SUFFIX).par conv=notrunc

	dd if=/dev/zero bs=$(word 4,$(1)) count=1 | sed 's/\x00/\xff/g' > $(KDIR)/root.squashfs.$(word 5,$(1)).par
	dd if=$(KDIR)/root.squashfs of=$(KDIR)/root.squashfs.$(word 5,$(1)).par conv=notrunc

	cat $(DTS_DIR)/$(word 1,$(1)).dtb.par $(KDIR)/$(word 5,$(1))$(KERNEL_SUFFIX).par $(KDIR)/root.squashfs.$(word 5,$(1)).par > $@
endef

define Device/Default
	FILESYSTEMS := squashfs
	KERNEL := kernel-bin | gzip | uImage gzip
	DEVICE_DTS :=
	PROFILES = Default $$(DEVICE_NAME)

	IMAGES = firmware.bin
	IMAGE/firmware.bin = mk_firmware $$(DEVICE_DTS) $$(DTB_PARTITION_SIZE) \
		$$(KERNEL_PARTITION_SIZE) $$(SQUASHFS_PARTITION_SIZE) $(1) $$$$(FIRMWARE_SIZE)
endef
DEVICE_VARS += DEVICE_DTS

ifeq ($(SUBTARGET),64b)
define Device/ls1043ardb-64bit
	KERNEL_LOADADDR = 0x80080000
	KERNEL_ENTRY_POINT = 0x80080000
	DEVICE_DTS = freescale/fsl-ls1043a-rdb

	DTB_PARTITION_SIZE := 1024k
	KERNEL_PARTITION_SIZE := 5120k
	SQUASHFS_PARTITION_SIZE := 55296k
	FIRMWARE_SIZE := 61440k

	DEVICE_TITLE := ls1043ardb 64bit
	DEVICE_PACKAGES += kmod-e1000e
endef
TARGET_DEVICES += ls1043ardb-64bit
endif

ifeq ($(SUBTARGET),32b)
define Device/ls1043ardb-32bit
	KERNEL_LOADADDR = 0x80008000
	KERNEL_ENTRY_POINT = 0x80008000
	DEVICE_DTS = ../../../arm64/boot/dts/freescale/fsl-ls1043a-rdb

	DTB_PARTITION_SIZE := 1024k
	KERNEL_PARTITION_SIZE := 5120k
	SQUASHFS_PARTITION_SIZE := 55296k
	FIRMWARE_SIZE := 61440k

	DEVICE_TITLE := ls1043ardb 32bit
	DEVICE_PACKAGES += kmod-e1000e
endef
TARGET_DEVICES += ls1043ardb-32bit
endif

$(eval $(call BuildImage))
