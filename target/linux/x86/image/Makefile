# 
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

export PATH=$(TARGET_PATH):/sbin

GRUB2_MODULES = biosdisk boot chain configfile fat ext2 linux ls part_msdos halt reboot serial vga
GRUB2_MODULES_ISO = biosdisk boot chain configfile fat iso9660 linux ls part_msdos halt reboot serial vga
GRUB_TERMINALS =
GRUB_SERIAL_CONFIG =
GRUB_TERMINAL_CONFIG =
GRUB_CONSOLE_CMDLINE =
GRUB_ROOT = hd0,msdos1

USE_ATKBD = generic 64

ifneq ($(strip $(foreach subtarget,$(USE_ATKBD),$(CONFIG_TARGET_x86_$(subtarget)))),)
  GRUB2_MODULES += at_keyboard
  GRUB2_MODULES_ISO += at_keyboard
endif

ifneq ($(CONFIG_GRUB_EFI),)
  GRUB2_MODULES_EFI = boot chain configfile fat ext2 squash4 iso9660 linux ls part_msdos part_gpt halt reboot serial efi_gop efi_uga video fixvideo video_fb gfxterm terminal font
  GRUB_FONT = unifont
  GRUB_EFI_CONFIG = insmod font; if loadfont /boot/grub/$(GRUB_FONT).pf2; then insmod efi_gop; insmod efi_uga; set gfxmode=auto; insmod gfxterm; set gfxpayload=keep; fi
  GRUB_TERMINAL_INPUT_EFI := console serial
  GRUB_TERMINAL_OUTPUT_EFI := console serial gfxterm
  GRUB_TERMINAL_CONFIG_EFI := terminal_input $(GRUB_TERMINAL_INPUT_EFI); terminal_output $(GRUB_TERMINAL_OUTPUT_EFI)

ifneq ($(CONFIG_TARGET_x86_64),)
  GRUB_EFI_PLATFORM = x86_64-efi
  GRUB_EFI_BOOTFILE = BOOTX64.EFI
else
  GRUB_EFI_PLATFORM = i386-efi
  GRUB_EFI_BOOTFILE = BOOTIA32.EFI
endif

endif

ifneq ($(CONFIG_GRUB_CONSOLE),)
  GRUB_CONSOLE_CMDLINE += console=tty0
  GRUB_TERMINALS += console
endif

GRUB_SERIAL:=$(call qstrip,$(CONFIG_GRUB_SERIAL))

ifneq ($(GRUB_SERIAL),)
  GRUB_CONSOLE_CMDLINE += console=$(GRUB_SERIAL),$(CONFIG_GRUB_BAUDRATE)n8$(if $(CONFIG_GRUB_FLOWCONTROL),r,)
  GRUB_SERIAL_CONFIG := serial --unit=0 --speed=$(CONFIG_GRUB_BAUDRATE) --word=8 --parity=no --stop=1 --rtscts=$(if $(CONFIG_GRUB_FLOWCONTROL),on,off)
  GRUB_TERMINALS += serial
endif

ifneq ($(GRUB_TERMINALS),)
  GRUB_TERMINAL_CONFIG := terminal_input $(GRUB_TERMINALS); terminal_output $(GRUB_TERMINALS)
endif

SIGNATURE:=$(shell perl -e 'printf("%08x", rand(0xFFFFFFFF))')
ROOTPART:=$(call qstrip,$(CONFIG_TARGET_ROOTFS_PARTNAME))
ROOTPART:=$(if $(ROOTPART),$(ROOTPART),PARTUUID=$(SIGNATURE)-02)

GRUB_TIMEOUT:=$(call qstrip,$(CONFIG_GRUB_TIMEOUT))

ifneq ($(CONFIG_TARGET_x86_xen_domu),)
  GRUB_ROOT = xen/xvda,msdos1
endif

ifneq ($(CONFIG_GRUB_IMAGES),)

  BOOTOPTS:=$(call qstrip,$(CONFIG_GRUB_BOOTOPTS))

  define Image/cmdline/ext4
    root=$(ROOTPART) rootfstype=ext4 rootwait
  endef

  define Image/cmdline/squashfs
    root=$(ROOTPART) rootfstype=squashfs rootwait
  endef

  define Image/Build/grub2
	# left here because the image builder doesnt need these
	$(INSTALL_DIR) $(KDIR)/root.grub/boot/grub $(KDIR)/grub2
	$(CP) $(KDIR)/bzImage $(KDIR)/root.grub/boot/vmlinuz
	grub-mkimage \
		-p /boot/grub \
		-d $(STAGING_DIR_HOST)/lib/grub/i386-pc \
		-o $(KDIR)/grub2/core.img \
		-O i386-pc \
		-c ./grub-early.cfg \
		$(GRUB2_MODULES)
	$(CP) $(STAGING_DIR_HOST)/lib/grub/i386-pc/*.img $(KDIR)/grub2/
	echo '(hd0) $(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).img' > $(KDIR)/grub2/device.map
	sed \
		-e 's#@SERIAL_CONFIG@#$(strip $(GRUB_SERIAL_CONFIG))#g' \
		-e 's#@TERMINAL_CONFIG@#$(strip $(GRUB_TERMINAL_CONFIG))#g' \
		-e 's#@CMDLINE@#$(strip $(call Image/cmdline/$(1)) $(BOOTOPTS) $(GRUB_CONSOLE_CMDLINE))#g' \
		-e 's#@TIMEOUT@#$(GRUB_TIMEOUT)#g' \
		-e 's#@ROOT@#$(GRUB_ROOT)#g' \
		./grub.cfg > $(KDIR)/root.grub/boot/grub/grub.cfg
	PADDING="$(CONFIG_TARGET_IMAGES_PAD)" SIGNATURE="$(SIGNATURE)" PATH="$(TARGET_PATH)" ./gen_image_generic.sh \
		$(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).img \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) $(KDIR)/root.grub \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) $(KDIR)/root.$(1) \
		256
	grub-bios-setup \
		--device-map="$(KDIR)/grub2/device.map" \
		-d "$(KDIR)/grub2" \
		-r "hd0,msdos1" \
		"$(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).img"
  endef

ifneq ($(CONFIG_GRUB_EFI),)
  define Image/Build/grub2-efi
	$(INSTALL_DIR) $(KDIR)/root.grub2-efi/boot/grub $(KDIR)/root.grub2-efi/EFI/BOOT
	$(CP) $(KDIR)/bzImage $(KDIR)/root.grub2-efi/boot/vmlinuz
	grub-mkimage \
		-p /boot/grub \
		-d $(STAGING_DIR_HOST)/lib/grub/$(GRUB_EFI_PLATFORM) \
		-o $(KDIR)/root.grub2-efi/EFI/BOOT/$(GRUB_EFI_BOOTFILE) \
		-O $(GRUB_EFI_PLATFORM) \
		-c ./grub-early.cfg \
		$(GRUB2_MODULES_EFI)
	grub-mkfont \
		-o $(KDIR)/root.grub2-efi/boot/grub/$(GRUB_FONT).pf2 \
		$(STAGING_DIR_HOST)/share/fonts/$(GRUB_FONT)/$(GRUB_FONT).bdf
	sed \
		-e 's#@EFI_CONFIG@#$(strip $(GRUB_EFI_CONFIG))#g' \
		-e 's#@SERIAL_CONFIG@#$(strip $(GRUB_SERIAL_CONFIG))#g' \
		-e 's#@TERMINAL_CONFIG@#$(strip $(GRUB_TERMINAL_CONFIG_EFI))#g' \
		-e 's#@CMDLINE@#$(strip $(call Image/cmdline/$(1)) $(BOOTOPTS) $(GRUB_CONSOLE_CMDLINE))#g' \
		-e 's#@TIMEOUT@#$(GRUB_TIMEOUT)#g' \
		-e 's#@ROOT@#$(GRUB_ROOT)#g' \
		./grub-efi.cfg > $(KDIR)/root.grub2-efi/boot/grub/grub.cfg
	PADDING="$(CONFIG_TARGET_IMAGES_PAD)" SIGNATURE="$(SIGNATURE)" PATH="$(TARGET_PATH)" ./gen_image_efi.sh \
		$(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).img \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) $(KDIR)/root.grub2-efi \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) $(KDIR)/root.$(1) \
		256
	# Also setup bios mbr to prevent invalid image on sysupgrade.
	echo '(hd0) $(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).img' > $(KDIR)/grub2/uefi-device.map
	grub-bios-setup \
		--device-map="$(KDIR)/grub2/uefi-device.map" \
		-d "$(KDIR)/grub2" \
		-r "hd0,msdos1" \
		"$(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).img"
  endef
endif

endif

define Image/Build/squashfs
	dd if=/dev/zero bs=128k count=1 >> $(KDIR)/root.squashfs
endef

define Image/Build/iso
	$(INSTALL_DIR) $(KDIR)/root.grub/boot/grub $(KDIR)/grub2
	$(CP) $(KDIR)/bzImage $(KDIR)/root.grub/boot/vmlinuz
	grub-mkimage \
		-p /boot/grub \
		-d $(STAGING_DIR_HOST)/lib/grub/i386-pc \
		-o $(KDIR)/grub2/eltorito.img \
		-O i386-pc \
		-c ./grub-early.cfg \
		$(GRUB2_MODULES_ISO)
	cat \
		$(STAGING_DIR_HOST)/lib/grub/i386-pc/cdboot.img \
		$(KDIR)/grub2/eltorito.img \
		> $(KDIR)/root.grub/boot/grub/eltorito.img
	sed \
		-e 's#@SERIAL_CONFIG@#$(strip $(GRUB_SERIAL_CONFIG))#g' \
		-e 's#@TERMINAL_CONFIG@#$(strip $(GRUB_TERMINAL_CONFIG))#g' \
		-e 's#@CMDLINE@#root=/dev/sr0 rootfstype=iso9660 rootwait $(strip $(call Image/cmdline/$(1)) $(BOOTOPTS) $(GRUB_CONSOLE_CMDLINE))#g' \
		-e 's#@TIMEOUT@#$(GRUB_TIMEOUT)#g' \
		./grub-iso.cfg > $(KDIR)/root.grub/boot/grub/grub.cfg
	mkisofs -R -b boot/grub/eltorito.img -no-emul-boot -boot-info-table \
		-o $(KDIR)/root.iso $(KDIR)/root.grub $(TARGET_DIR)
endef

ifneq ($(CONFIG_VDI_IMAGES),)
  define Image/Build/vdi/Default
	rm $(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).vdi || true
	qemu-img convert -f raw -O vdi \
		$(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).img \
		$(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).vdi
	# XXX: VBoxManage insists on setting perms to 0600
	chmod 0644 $(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).vdi
  endef
ifneq ($(CONFIG_GRUB_EFI),)
  define Image/Build/vdi
	$(call Image/Build/vdi/Default,$(1))
	rm $(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).vdi || true
	qemu-img convert -f raw -O vdi \
		$(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).img \
		$(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).vdi
	# XXX: VBoxManage insists on setting perms to 0600
	chmod 0644 $(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).vdi
  endef
else
  define Image/Build/vdi
	$(call Image/Build/vdi/Default,$(1))
  endef
endif

endif

ifneq ($(CONFIG_VMDK_IMAGES),)
  define Image/Build/vmdk/Default
	rm $(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).vmdk || true
	qemu-img convert -f raw -O vmdk \
		$(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).img \
		$(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).vmdk
  endef

ifneq ($(CONFIG_GRUB_EFI),)
  define Image/Build/vmdk
	$(call Image/Build/vmdk/Default,$(1))
	rm $(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).vmdk || true
	qemu-img convert -f raw -O vmdk \
		$(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).img \
		$(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).vmdk
  endef
else
  define Image/Build/vmdk
	$(call Image/Build/vmdk/Default,$(1))
  endef
endif

endif

define Image/Build/gzip/Default
	gzip -f9n $(BIN_DIR)/$(IMG_PREFIX)-combined-$(1).img
	gzip -f9n $(BIN_DIR)/$(IMG_PREFIX)-rootfs-$(1).img
endef

ifneq ($(CONFIG_GRUB_EFI),)
  define Image/Build/gzip
	gzip -f9n $(BIN_DIR)/$(IMG_PREFIX)-uefi-combined-$(1).img
	$(call Image/Build/gzip/Default,$(1))
  endef
else
  define Image/Build/gzip
	$(call Image/Build/gzip/Default,$(1))
  endef	
endif

ifneq ($(CONFIG_TARGET_IMAGES_GZIP),)
  define Image/Build/gzip/ext4
	$(call Image/Build/gzip,ext4)
  endef
  ifneq ($(CONFIG_TARGET_IMAGES_PAD),)
    define Image/Build/gzip/squashfs
	$(call Image/Build/gzip,squashfs)
    endef
  endif
endif

define Image/BuildKernel
	$(CP) $(KDIR)/bzImage $(BIN_DIR)/$(IMG_PREFIX)-vmlinuz
endef

define Image/Prepare/Default
	$(call Image/Prepare/grub2)
endef

ifneq ($(CONFIG_GRUB_EFI),)
  define Image/Prepare
	$(call Image/Prepare/grub2-efi)
	$(call Image/Prepare/Default)
  endef
else
  define Image/Prepare
	$(call Image/Prepare/Default)
  endef
endif

define Image/Build/Initramfs
	$(CP) $(KDIR)/bzImage-initramfs $(BIN_DIR)/$(IMG_PREFIX)-ramfs.bzImage
endef

define Image/Build
	$(call Image/Build/$(1))
  ifneq ($(1),iso)
    ifneq ($(CONFIG_GRUB_EFI),)
      ifeq ($(1),ext4)
	$(call Image/Build/grub2-efi,$(1))
      endif
    endif
	$(call Image/Build/grub2,$(1))
	$(call Image/Build/vdi,$(1))
	$(call Image/Build/vmdk,$(1))
	$(CP) $(KDIR)/root.$(1) $(BIN_DIR)/$(IMG_PREFIX)-rootfs-$(1).img
  else
	$(CP) $(KDIR)/root.iso $(BIN_DIR)/$(IMG_PREFIX).iso
  endif
	$(CP) $(KDIR)/bzImage $(BIN_DIR)/$(IMG_PREFIX)-vmlinuz
	$(call Image/Build/gzip/$(1))
ifeq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),y)
	$(call Image/Build/Initramfs)
endif
endef

$(eval $(call BuildImage))
