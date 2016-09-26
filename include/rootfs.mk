include $(INCLUDE_DIR)/feeds.mk

ifdef CONFIG_USE_MKLIBS
  define mklibs
	rm -rf $(TMP_DIR)/mklibs-progs $(TMP_DIR)/mklibs-out
	# first find all programs and add them to the mklibs list
	find $(STAGING_DIR_ROOT) -type f -perm /100 -exec \
		file -r -N -F '' {} + | \
		awk ' /executable.*dynamically/ { print $$1 }' > $(TMP_DIR)/mklibs-progs
	# find all loadable objects that are not regular libraries and add them to the list as well
	find $(STAGING_DIR_ROOT) -type f -name \*.so\* -exec \
		file -r -N -F '' {} + | \
		awk ' /shared object/ { print $$1 }' > $(TMP_DIR)/mklibs-libs
	mkdir -p $(TMP_DIR)/mklibs-out
	$(STAGING_DIR_HOST)/bin/mklibs -D \
		-d $(TMP_DIR)/mklibs-out \
		--sysroot $(STAGING_DIR_ROOT) \
		`cat $(TMP_DIR)/mklibs-libs | sed 's:/*[^/]\+/*$$::' | uniq | sed 's:^$(STAGING_DIR_ROOT):-L :'` \
		--ldlib $(patsubst $(STAGING_DIR_ROOT)/%,/%,$(firstword $(wildcard \
			$(foreach name,ld-uClibc.so.* ld-linux.so.* ld-*.so ld-musl-*.so.*, \
			  $(STAGING_DIR_ROOT)/lib/$(name) \
			)))) \
		--target $(REAL_GNU_TARGET_NAME) \
		`cat $(TMP_DIR)/mklibs-progs $(TMP_DIR)/mklibs-libs` 2>&1
	$(RSTRIP) $(TMP_DIR)/mklibs-out
	for lib in `ls $(TMP_DIR)/mklibs-out/*.so.* 2>/dev/null`; do \
		LIB="$${lib##*/}"; \
		DEST="`ls "$(1)/lib/$$LIB" "$(1)/usr/lib/$$LIB" 2>/dev/null`"; \
		[ -n "$$DEST" ] || continue; \
		echo "Copying stripped library $$lib to $$DEST"; \
		cp "$$lib" "$$DEST" || exit 1; \
	done
  endef
endif

# where to build (and put) .ipk packages
opkg = \
  IPKG_NO_SCRIPT=1 \
  IPKG_INSTROOT=$(1) \
  TMPDIR=$(1)/tmp \
  $(STAGING_DIR_HOST)/bin/opkg \
	--offline-root $(1) \
	--force-postinstall \
	--add-dest root:/ \
	--add-arch all:100 \
	--add-arch $(if $(ARCH_PACKAGES),$(ARCH_PACKAGES),$(BOARD)):200

opkg_package_files = $(wildcard \
	$(foreach dir,$(PACKAGE_SUBDIRS), \
	  $(foreach pkg,$(1), $(dir)/$(pkg)_*.ipk)))

TARGET_DIR_ORIG := $(TARGET_ROOTFS_DIR)/root.orig-$(BOARD)

define prepare_rootfs
	@if [ -d $(TOPDIR)/files ]; then \
		$(call file_copy,$(TOPDIR)/files/.,$(1)); \
	fi
	@mkdir -p $(1)/etc/rc.d
	@( \
		cd $(1); \
		for script in ./usr/lib/opkg/info/*.postinst; do \
			IPKG_INSTROOT=$(1) $$(which bash) $$script; \
			ret=$$?; \
			if [ $$ret -ne 0 ]; then \
				echo "postinst script $$script has failed with exit code $$ret" >&2; \
				exit 1; \
			fi; \
		done; \
		for script in ./etc/init.d/*; do \
			grep '#!/bin/sh /etc/rc.common' $$script >/dev/null || continue; \
			IPKG_INSTROOT=$(1) $$(which bash) ./etc/rc.common $$script enable; \
		done || true \
	)
	$(if $(SOURCE_DATE_EPOCH),sed -i "s/Installed-Time: .*/Installed-Time: $(SOURCE_DATE_EPOCH)/" $(1)/usr/lib/opkg/status)
	@-find $(1) -name CVS   | $(XARGS) rm -rf
	@-find $(1) -name .svn  | $(XARGS) rm -rf
	@-find $(1) -name .git  | $(XARGS) rm -rf
	@-find $(1) -name '.#*' | $(XARGS) rm -f
	rm -f $(1)/usr/lib/opkg/lists/*
	rm -f $(1)/usr/lib/opkg/info/*.postinst*
	rm -f $(1)/usr/lib/opkg/info/*.prerm*
	$(if $(CONFIG_CLEAN_IPKG),rm -rf $(1)/usr/lib/opkg)
	$(call mklibs,$(1))
endef
