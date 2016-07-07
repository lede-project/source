#
# Copyright (C) 2007-2008 OpenWrt.org
# Copyright (C) 2016 LEDE Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ifneq ($(__profiles_inc),1)
__profiles_inc=1

ifneq ($(DUMP),)
  all: dumpinfo
endif

include $(TOPDIR)/include/target.mk
include $(TOPDIR)/include/defaults.mk

define ProfileDefault
  NAME:=
  PRIORITY:=
  PACKAGES:=
endef

ifndef Profile
define Profile
  $(eval $(call ProfileDefault))
  $(eval $(call Profile/$(1)))
  dumpinfo : $(call shexport,Profile/$(1)/Description)
  PKGDUMPINFO += \
	echo "Target-Profile: $(1)"; \
	echo "Target-Profile-Packages: $(PACKAGES) $(call extra_packages,$(DEFAULT_PACKAGES) $(PACKAGES))"; \
	echo "@@"; \
	echo;
  DUMPINFO += \
	echo "Target-Profile: $(1)"; \
	$(if $(PRIORITY), echo "Target-Profile-Priority: $(PRIORITY)"; ) \
	echo "Target-Profile-Name: $(NAME)"; \
	echo "Target-Profile-Device-Type: $(if $(TARGET_DEVICE_TYPE),$(TARGET_DEVICE_TYPE),router)"; \
	echo "Target-Profile-Description:"; \
	echo "$$$$$$$$$(call shvar,Profile/$(1)/Description)"; \
	echo "@@"; \
	echo;
endef
endif

ifneq ($(PLATFORM_DIR),$(PLATFORM_SUBDIR))
  define IncludeProfiles
    -include $(sort $(wildcard $(PLATFORM_DIR)/profiles/*.mk))
    -include $(sort $(wildcard $(PLATFORM_SUBDIR)/profiles/*.mk))
  endef
else
  define IncludeProfiles
    -include $(sort $(wildcard $(PLATFORM_DIR)/profiles/*.mk))
  endef
endif

PROFILE:=$(call qstrip,$(CONFIG_TARGET_PROFILE))

ifeq ($(TARGET_BUILD),1)
  ifneq ($(DUMP),)
    $(eval $(call IncludeProfiles))
  endif
endif

endif #__profiles_inc
