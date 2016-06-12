#
# Copyright (C) 2007-2008 OpenWrt.org
# Copyright (C) 2016 LEDE Project
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ifneq ($(__defaults_inc),1)
__defaults_inc=1

# default device type
DEVICE_TYPE:=$(if $(call qstrip,$(CONFIG_DEVICE_TYPE)),$(call qstrip,$(CONFIG_DEVICE_TYPE)),router)
DEVICE_TYPES:=nas router bootloader developerboard other minimal_sdk

# Default packages - the really basic set
BASE_DEFAULT_PACKAGES:=base-files libc libgcc busybox opkg

DEFAULT_PACKAGES.device_common:=dropbear mtd uci netifd fstools uclient-fetch logd
# For nas targets
DEFAULT_PACKAGES.nas:=block-mount fdisk lsblk mdadm $(DEFAULT_PACKAGES.device_common)
# For router targets
DEFAULT_PACKAGES.router:=dnsmasq iptables ip6tables ppp ppp-mod-pppoe firewall odhcpd odhcp6c $(DEFAULT_PACKAGES.device_common)
DEFAULT_PACKAGES.bootloader:=
DEFAULT_PACKAGES.minimal_sdk:=
DEFAULT_PACKAGES.other:=$(DEFAULT_PACKAGES.device_common)
DEFAULT_PACKAGES.developerboard:=$(DEFAULT_PACKAGES.device_common)

filter_packages = $(filter-out -% $(patsubst -%,%,$(filter -%,$(1))),$(1))
extra_packages = $(if $(filter wpad-mini wpad nas,$(1)),iwinfo)

ifneq ($(DUMP_DEFAULTS_BASE),)

define DeviceTypeDefaults
	echo 'Device-Type: $(1)'; \
	echo 'Device-Type-Conf: USE_DEVICE_TYPE_$(1)'; \
	echo 'Device-Type-Packages: $(DEFAULT_PACKAGES.$(1)) $(call extra_packages,$(DEFAULT_PACKAGES.$(1)))' ; \
	echo '@@' ;
endef

define BuildDefaults/DumpCurrent
  all:
	@echo 'Base-Default-Packages: $(BASE_DEFAULT_PACKAGES)'; \
	 echo '@@' ; \
	 $(foreach dt,$(DEVICE_TYPES),$(call DeviceTypeDefaults,$(dt)))
endef

$(eval $(call BuildDefaults/DumpCurrent))

else

include $(TOPDIR)/include/profiles.mk

# DEFAULT_PACKAGES may appended in target definitions
define BuildDefaults/DumpCurrent
  .PHONY: dumpinfo
  dumpinfo:
	@echo 'Target: $(TARGETID)'; \
	 echo 'Default-Packages: $(DEFAULT_PACKAGES) $(call extra_packages,$(DEFAULT_PACKAGES))'; \
	 $(PKGDUMPINFO)
	$(if $(CUR_SUBTARGET),$(SUBMAKE) -r --no-print-directory -C image -s DUMP=1 SUBTARGET=$(CUR_SUBTARGET))
	$(if $(SUBTARGET),,@$(foreach SUBTARGET,$(SUBTARGETS),$(SUBMAKE) -s DUMP=1 SUBTARGET=$(SUBTARGET); ))
endef

include $(TOPDIR)/include/target.mk

endif #DUMP_DEFAULTS_BASE

endif #__defaults_inc
