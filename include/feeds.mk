#
# Copyright (C) 2014 OpenWrt.org
# Copyright (C) 2016 LEDE Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

-include $(TMP_DIR)/.packagesubdirs

FEEDS_INSTALLED:=$(notdir $(wildcard $(TOPDIR)/package/feeds/*))
FEEDS_AVAILABLE:=$(sort $(FEEDS_INSTALLED) $(shell $(SCRIPT_DIR)/feeds list -n))
FEEDS_ENABLED:=$(foreach feed,$(FEEDS_INSTALLED),$(if $(CONFIG_FEED_$(feed)),$(feed)))
FEEDS_DISABLED:=$(filter-out $(FEEDS_ENABLED),$(FEEDS_AVAILABLE))

PACKAGE_SUBDIRS=$(PACKAGE_DIR)
ifneq ($(CONFIG_PER_FEED_REPO),)
  PACKAGE_SUBDIRS += $(OUTPUT_DIR)/packages/$(ARCH_PACKAGES)/base
  ifneq ($(CONFIG_PER_FEED_REPO_ADD_DISABLED),)
    PACKAGE_SUBDIRS += $(foreach FEED,$(FEEDS_AVAILABLE),$(OUTPUT_DIR)/packages/$(ARCH_PACKAGES)/$(FEED))
  else
    PACKAGE_SUBDIRS += $(foreach FEED,$(FEEDS_ENABLED),$(OUTPUT_DIR)/packages/$(ARCH_PACKAGES)/$(FEED))
  endif
endif

PACKAGE_DIR_ALL := $(PACKAGE_DIR)_all

PKG_CONFIG_DEPENDS += \
	CONFIG_PER_FEED_REPO \
	CONFIG_PER_FEED_REPO_ADD_DISABLED \
	CONFIG_PER_FEED_REPO_ADD_COMMENTED \
	$(foreach feed,$(FEEDS_INSTALLED),CONFIG_FEED_$(feed))

# 1: package name
define FeedPackageDir
$(strip $(if $(CONFIG_PER_FEED_REPO), \
  $(if $(Package/$(1)/subdir), \
    $(abspath $(OUTPUT_DIR)/packages/$(ARCH_PACKAGES)/$(Package/$(1)/subdir)), \
    $(PACKAGE_DIR)), \
  $(PACKAGE_DIR)))
endef

# 1: destination file
define FeedSourcesAppend
( \
  echo "src/gz %n_core %U/targets/%S/packages"; \
  $(strip $(if $(CONFIG_PER_FEED_REPO), \
	$(foreach feed,base $(FEEDS_ENABLED),echo "src/gz %n_$(feed) %U/packages/%A/$(feed)";) \
	$(if $(CONFIG_PER_FEED_REPO_ADD_DISABLED), \
		$(foreach feed,$(FEEDS_DISABLED),echo "$(if $(CONFIG_PER_FEED_REPO_ADD_COMMENTED),# )src/gz %n_$(feed) %U/packages/%A/$(feed)";)))) \
) >> $(1)
endef
