# Makefile for OpenWrt
#
# Copyright (C) 2007-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

TOPDIR:=${CURDIR}
LC_ALL:=C
LANG:=C
export TOPDIR LC_ALL LANG IB
export OPENWRT_VERBOSE=s

IB:=1

all: help

include $(TOPDIR)/include/host.mk

ifneq ($(OPENWRT_BUILD),1)
  override OPENWRT_BUILD=1
  export OPENWRT_BUILD
endif

include rules.mk
include $(INCLUDE_DIR)/debug.mk
include $(INCLUDE_DIR)/depends.mk

include $(INCLUDE_DIR)/version.mk

ifneq ($(BUILD_IMAGE),)
  -include .config
endif

export REVISION

SCAN_COOKIE?=$(shell echo $$$$)
export SCAN_COOKIE

define Helptext
Available Commands:
	help:	This help text
	info:	Show a list of available target profiles
	clean:	Remove images and temporary build files
	image:	Build an image (see below for more information).
	menuconfig: Configure image generation

Building images:
	By default 'make image' will create an image with the default
	target profile and package set. You can use menuconfig as well as
	the following variable to to change that:

	make image PACKAGES="<pkg1> [<pkg2> [<pkg3> ...]]" # include extra packages
	make image FILES="<path>" # include extra files from <path>
	make image BIN_DIR="<path>" # alternative output directory for the images
	make image EXTRA_IMAGE_NAME="<string>" # Add this to the output image filename (sanitized)
endef
$(eval $(call shexport,Helptext))

help: FORCE
	echo "$$$(call shvar,Helptext)"


# override variables from rules.mk
PACKAGE_DIR:=$(TOPDIR)/packages
LISTS_DIR:=$(subst $(space),/,$(patsubst %,..,$(subst /,$(space),$(TARGET_DIR))))$(DL_DIR)
OPKG:= \
  IPKG_NO_SCRIPT=1 \
  IPKG_TMP="$(TMP_DIR)/ipkgtmp" \
  IPKG_INSTROOT="$(TARGET_DIR)" \
  IPKG_CONF_DIR="$(TMP_DIR)" \
  IPKG_OFFLINE_ROOT="$(TARGET_DIR)" \
  $(STAGING_DIR_HOST)/bin/opkg \
	-f $(TOPDIR)/repositories.conf \
	--force-depends \
	--force-overwrite \
	--force-postinstall \
	--cache $(DL_DIR) \
	--lists-dir $(LISTS_DIR) \
	--offline-root $(TARGET_DIR) \
	--add-dest root:/ \
	--add-arch all:100 \
	--add-arch $(ARCH_PACKAGES):200

include $(INCLUDE_DIR)/target.mk

ifneq ($(USE_PROFILES),)
  -include .profiles.mk
endif

.profiles.mk: $(TOPDIR)/tmp/.targetinfo
	$(SCRIPT_DIR)/target-metadata.pl profile_mk $< '$(BOARD)$(if $(SUBTARGET),/$(SUBTARGET))' > $@
$(TOPDIR)/tmp/.config-targets.in: $(TOPDIR)/tmp/.targetinfo
	$(SCRIPT_DIR)/target-metadata.pl profile_config $< '$(BOARD)$(if $(SUBTARGET),/$(SUBTARGET))' > $@

$(TOPDIR)/tmp/.config-targets.in: $(TOPDIR)/tmp/.targetinfo
	$(SCRIPT_DIR)/target-metadata.pl profile_config $< '$(BOARD)$(if $(SUBTARGET),/$(SUBTARGET))' > $@

$(TOPDIR)/tmp/.targetinfo:
	mkdir -p tmp/info
	$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f include/scan.mk SCAN_TARGET="targetinfo" SCAN_DIR="target/linux" SCAN_NAME="target" SCAN_DEPS="image/Makefile profiles/*.mk $(TOPDIR)/include/kernel*.mk $(TOPDIR)/include/target.mk" SCAN_DEPTH=2 SCAN_EXTRA="" SCAN_MAKEOPTS="TARGET_BUILD=1"

staging_dir/host/.prereq-build: include/prereq-build.mk
	mkdir -p tmp
	rm -f tmp/.host.mk
	@$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f $(TOPDIR)/include/prereq-build.mk prereq 2>/dev/null || { \
		echo "Prerequisite check failed. Use FORCE=1 to override."; \
		false; \
	}
  ifneq ($(realpath $(TOPDIR)/include/prepare.mk),)
	@$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f $(TOPDIR)/include/prepare.mk prepare 2>/dev/null || { \
		echo "Preparation failed."; \
		false; \
	}
  endif
	touch $@

_call_info: FORCE
	echo 'Current Target: "$(BOARD)$(if $(SUBTARGET), ($(BOARDNAME)))"'
	echo 'Default Packages: $(DEFAULT_PACKAGES)'
	echo 'Profile Packages: $($(PROFILE)_PACKAGES)'

BUILD_PACKAGES:=$(USER_PACKAGES) $(sort $(DEFAULT_PACKAGES) $($(PROFILE)_PACKAGES) kernel)
# "-pkgname" in the package list means remove "pkgname" from the package list
BUILD_PACKAGES:=$(filter-out $(filter -%,$(BUILD_PACKAGES)) $(patsubst -%,%,$(filter -%,$(BUILD_PACKAGES))),$(BUILD_PACKAGES))
PACKAGES:=

_call_image: staging_dir/host/.prereq-build
	echo 'Building images for $(BOARD)$(if $($(PROFILE)_NAME), - $($(PROFILE)_NAME))'
	echo 'Packages: $(BUILD_PACKAGES)'
	echo
	rm -rf $(TARGET_DIR)
	mkdir -p $(TARGET_DIR) $(BIN_DIR) $(TMP_DIR) $(DL_DIR)
	if [ ! -f "$(PACKAGE_DIR)/Packages" ] || [ ! -f "$(PACKAGE_DIR)/Packages.gz" ] || [ "`find $(PACKAGE_DIR) -cnewer $(PACKAGE_DIR)/Packages.gz`" ]; then \
		echo "Package list missing or not up-to-date, generating it.";\
		$(MAKE) package_index; \
	else \
		mkdir -p $(TARGET_DIR)/tmp; \
		$(OPKG) update || true; \
	fi
	$(MAKE) package_install
ifneq ($(USER_FILES),)
	$(MAKE) copy_files
endif
	$(MAKE) package_postinst
	$(MAKE) build_image

package_index: FORCE
	@echo
	@echo Building package index...
	@mkdir -p $(TMP_DIR) $(TARGET_DIR)/tmp
	(cd $(PACKAGE_DIR); $(SCRIPT_DIR)/ipkg-make-index.sh . > Packages && \
		gzip -9nc Packages > Packages.gz \
	) >/dev/null 2>/dev/null
	$(OPKG) update || true

package_install: FORCE
	@echo
	@echo Installing packages...
	$(OPKG) install $(firstword $(wildcard $(PACKAGE_DIR)/libc_*.ipk $(PACKAGE_DIR)/base/libc_*.ipk))
	$(OPKG) install $(firstword $(wildcard $(PACKAGE_DIR)/kernel_*.ipk $(PACKAGE_DIR)/base/kernel_*.ipk))
	$(OPKG) install $(BUILD_PACKAGES)
	rm -f $(TARGET_DIR)/usr/lib/opkg/lists/*

copy_files: FORCE
	@echo
	@echo Copying extra files
	@$(call file_copy,$(USER_FILES)/*,$(TARGET_DIR)/)

package_postinst: FORCE
	@echo
	@echo Cleaning up
	@rm -f $(TARGET_DIR)/tmp/opkg.lock
	@echo
	@echo Activating init scripts
	@mkdir -p $(TARGET_DIR)/etc/rc.d
	@( \
		cd $(TARGET_DIR); \
		for script in ./usr/lib/opkg/info/*.postinst; do \
			IPKG_INSTROOT=$(TARGET_DIR) $$(which bash) $$script; \
		done || true \
	)
	rm -f $(TARGET_DIR)/usr/lib/opkg/info/*.postinst
	$(if $(CONFIG_CLEAN_IPKG),rm -rf $(TARGET_DIR)/usr/lib/opkg)

build_image: FORCE
	@echo
	@echo Building images...
	$(NO_TRACE_MAKE) -C target/linux/$(BOARD)/image install TARGET_BUILD=1 IB=1 EXTRA_IMAGE_NAME="$(EXTRA_IMAGE_NAME)" \

clean:
	rm -rf $(TMP_DIR) $(DL_DIR) $(TARGET_DIR) $(BIN_DIR)


info:
	(unset FILES PACKAGES MAKEFLAGS; $(MAKE) USE_PROFILES=1 -s _call_info)

image_target:
	(unset FILES PACKAGES MAKEFLAGS; \
	$(MAKE) _call_image \
		$(if $(FILES),USER_FILES="$(FILES)") \
		$(if $(PACKAGES),USER_PACKAGES="$(PACKAGES)") \
		$(if $(BIN_DIR),BIN_DIR="$(BIN_DIR)"))

image: .config
	$(MAKE) image_target PACKAGES="$(PACKAGES)" FILES="$(FILES)" BIN_DIR="$(BIN_DIR)" EXTRA_IMAGE_NAME="$(EXTRA_IMAGE_NAME)" USE_PROFILES=1 BUILD_IMAGE=1

.config: ./scripts/config/conf
	@+if [ \! -e .config ] || ! grep CONFIG_HAVE_DOT_CONFIG .config >/dev/null; then \
		[ -e $(HOME)/.cshorewrt/defconfig ] && cp $(HOME)/.cshorewrt/defconfig .config; \
		$(_SINGLE)$(NO_TRACE_MAKE) menuconfig $(PREP_MK); \
	fi

defconfig: scripts/config/conf $(TOPDIR)/tmp/.config-targets.in FORCE
	touch .config
	@if [ -e $(HOME)/.cshorewrt/defconfig ]; then cp $(HOME)/.cshorewrt/defconfig .config; fi
	$< --defconfig=.config Config.in

confdefault-y=allyes
confdefault-m=allmod
confdefault-n=allno
confdefault:=$(confdefault-$(CONFDEFAULT))

oldconfig: scripts/config/conf $(TOPDIR)/tmp/.config-targets.in FORCE
	$< --$(if $(confdefault),$(confdefault),old)config Config.in

menuconfig: scripts/config/mconf $(TOPDIR)/tmp/.config-targets.in FORCE
	if [ \! -e .config -a -e $(HOME)/.cshorewrt/defconfig ]; then \
		cp $(HOME)/.cshorewrt/defconfig .config; \
	fi
	$< Config.in

.SILENT: help info image
