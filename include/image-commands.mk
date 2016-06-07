# Build commands that can be called from Device/* templates

define Build/uImage
	mkimage -A $(LINUX_KARCH) \
		-O linux -T kernel \
		-C $(1) -a $(KERNEL_LOADADDR) -e $(if $(KERNEL_ENTRY),$(KERNEL_ENTRY),$(KERNEL_LOADADDR)) \
		-n '$(call toupper,$(LINUX_KARCH)) LEDE Linux-$(LINUX_VERSION)' -d $@ $@.new
	@mv $@.new $@
endef

define Build/netgear-chk
	$(STAGING_DIR_HOST)/bin/mkchkimg \
		-o $@.new \
		-k $@ \
		-b $(NETGEAR_BOARD_ID) \
		-r $(NETGEAR_REGION)
	mv $@.new $@
endef

define Build/netgear-dni
	$(STAGING_DIR_HOST)/bin/mkdniimg \
		-B $(NETGEAR_BOARD_ID) -v LEDE.$(REVISION) \
		$(if $(NETGEAR_HW_ID),-H $(NETGEAR_HW_ID)) \
		-r "$(1)" \
		-i $@ -o $@.new
	mv $@.new $@
endef

define Build/tplink-safeloader
       -$(STAGING_DIR_HOST)/bin/tplink-safeloader \
		-B $(TPLINK_BOARD_NAME) \
		-V $(REVISION) \
		-k $(word 1,$^) \
		-r $@ \
		-o $@.new \
		-j \
		$(wordlist 2,$(words $(1)),$(1)) \
		$(if $(findstring sysupgrade,$(word 1,$(1))),-S) && mv $@.new $@ || rm -f $@
endef

define Build/fit
	$(TOPDIR)/scripts/mkits.sh \
		-D $(DEVICE_NAME) -o $@.its -k $@ \
		$(if $(word 2,$(1)),-d $(word 2,$(1))) -C $(word 1,$(1)) \
		-a $(KERNEL_LOADADDR) -e $(if $(KERNEL_ENTRY),$(KERNEL_ENTRY),$(KERNEL_LOADADDR)) \
		-A $(ARCH) -v $(LINUX_VERSION)
	PATH=$(LINUX_DIR)/scripts/dtc:$(PATH) mkimage -f $@.its $@.new
	@mv $@.new $@
endef

define Build/lzma
	$(call Build/lzma-no-dict,-lc1 -lp2 -pb2 $(1))
endef

define Build/lzma-no-dict
	$(STAGING_DIR_HOST)/bin/lzma e $@ $(1) $@.new
	@mv $@.new $@
endef

define Build/gzip
	gzip -9n -c $@ $(1) > $@.new
	@mv $@.new $@
endef

define Build/jffs2
	rm -rf $(KDIR_TMP)/$(DEVICE_NAME)/jffs2 && \
		mkdir -p $(KDIR_TMP)/$(DEVICE_NAME)/jffs2/$$(dirname $(1)) && \
		cp $@ $(KDIR_TMP)/$(DEVICE_NAME)/jffs2/$(1) && \
		$(STAGING_DIR_HOST)/bin/mkfs.jffs2 --pad \
			$(if $(CONFIG_BIG_ENDIAN),--big-endian,--little-endian) \
			--squash-uids -v -e $(patsubst %k,%KiB,$(BLOCKSIZE)) \
			-o $@.new \
			-d $(KDIR_TMP)/$(DEVICE_NAME)/jffs2 \
			2>&1 1>/dev/null | awk '/^.+$$$$/' && \
		$(STAGING_DIR_HOST)/bin/padjffs2 $@.new -J $(patsubst %k,,$(BLOCKSIZE))
	-rm -rf $(KDIR_TMP)/$(DEVICE_NAME)/jffs2/
	@mv $@.new $@
endef

define Build/kernel-bin
	rm -f $@
	cp $< $@
endef

define Build/patch-cmdline
	$(STAGING_DIR_HOST)/bin/patch-cmdline $@ '$(CMDLINE)'
endef

define Build/append-kernel
	dd if=$(word 1,$^) $(if $(1),bs=$(1) conv=sync) >> $@
endef

define Build/append-rootfs
	dd if=$(word 2,$^) $(if $(1),bs=$(1) conv=sync) >> $@
endef

define Build/append-ubi
	sh $(TOPDIR)/scripts/ubinize-image.sh \
		$(if $(UBOOTENV_IN_UBI),--uboot-env) \
		$(if $(KERNEL_IN_UBI),--kernel $(word 1,$^)) \
		$(word 2,$^) \
		$@.tmp \
		-p $(BLOCKSIZE) -m $(PAGESIZE) -E 5 \
		$(if $(SUBPAGESIZE),-s $(SUBPAGESIZE))
	cat $@.tmp >> $@
	rm $@.tmp
endef

define Build/pad-to
	dd if=$@ of=$@.new bs=$(1) conv=sync
	mv $@.new $@
endef

define Build/pad-rootfs
	$(STAGING_DIR_HOST)/bin/padjffs2 $@ $(1) 4 8 16 64 128 256
endef

define Build/pad-offset
	let \
		size="$$(stat -c%s $@)" \
		pad="$(word 1, $(1))" \
		offset="$(word 2, $(1))" \
		pad="(pad - ((size + offset) % pad)) % pad" \
		newsize='size + pad'; \
		dd if=$@ of=$@.new bs=$$newsize count=1 conv=sync
	mv $@.new $@
endef

define Build/check-size
	@[ $$(($(subst k,* 1024,$(subst m, * 1024k,$(1))))) -ge "$$(stat -c%s $@)" ] || { \
		echo "WARNING: Image file $@ is too big" >&2; \
		rm -f $@; \
	}
endef

define Build/combined-image
	-sh $(TOPDIR)/scripts/combined-image.sh \
		"$(word 1,$^)" \
		"$@" \
		"$@.new"
	@mv $@.new $@
endef

define Build/sysupgrade-nand
	sh $(TOPDIR)/scripts/sysupgrade-nand.sh \
		--board $(if $(BOARD_NAME),$(BOARD_NAME),$(DEVICE_NAME)) \
		--kernel $(word 1,$^) \
		--rootfs $(word 2,$^) \
		$@
endef
