PKG_NAME ?= barebox

ifndef PKG_SOURCE_PROTO
PKG_SOURCE = $(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL = \
	http://sources.lede-project.org \
	http://www.barebox.org/download
endif

PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_TARGETS := bin
PKG_FLAGS:=nonshared

PKG_LICENSE:=GPL-2.0 GPL-2.0+
PKG_LICENSE_FILES:=Licenses/README

PKG_MAINTAINER:=Jianhui Zhao <jianhuizhao329@gmail.com>

PKG_BUILD_PARALLEL:=1

export GCC_HONOUR_COPTS=s

define Package/barebox/install/default
	$(CP) $(patsubst %,$(PKG_BUILD_DIR)/%,$(BAREBOX_IMAGE)) $(1)/
endef

Package/barebox/install = $(Package/barebox/install/default)

define Barebox/Init
  BUILD_TARGET:=
  BUILD_SUBTARGET:=
  BUILD_DEVICES:=
  NAME:=
  DEPENDS:=
  HIDDEN:=
  DEFAULT:=
  VARIANT:=$(1)
  BAREBOX_CONFIG:=$(1)
  BAREBOX_IMAGE:=barebox.bin
endef

TARGET_DEP = TARGET_$(BUILD_TARGET)$(if $(BUILD_SUBTARGET),_$(BUILD_SUBTARGET))

define Build/Barebox/Target
  $(eval $(call Barebox/Init,$(1)))
  $(eval $(call Barebox/Default,$(1)))
  $(eval $(call Barebox/$(1),$(1)))

 define Package/barebox-$(1)
    SECTION:=boot
    CATEGORY:=Boot Loaders
    TITLE:=Barebox for $(1)
    VARIANT:=$(VARIANT)
    DEPENDS:=@!IN_SDK $(DEPENDS)
    HIDDEN:=$(HIDDEN)
    ifneq ($(BUILD_TARGET),)
      DEPENDS += @$(TARGET_DEP)
      ifneq ($(BUILD_DEVICES),)
        DEFAULT := y if ($(TARGET_DEP)_Default \
		$(patsubst %,|| $(TARGET_DEP)_DEVICE_%,$(BUILD_DEVICES)) \
		$(patsubst %,|| $(patsubst TARGET_%,TARGET_DEVICE_%,$(TARGET_DEP))_DEVICE_%,$(BUILD_DEVICES)))
      endif
    endif
    $(if $(DEFAULT),DEFAULT:=$(DEFAULT))
    URL:=http://www.barebox.org
  endef

  define Package/barebox-$(1)/install
	$$(Package/barebox/install)
  endef
endef

define Build/Configure/Barebox
	+$(MAKE) ARCH=$(ARCH) $(PKG_JOBS) -C $(PKG_BUILD_DIR) $(BAREBOX_CONFIGURE_VARS) $(BAREBOX_CONFIG)_defconfig
endef

DTC=$(wildcard $(LINUX_DIR)/scripts/dtc/dtc)

define Build/Compile/Barebox
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) \
		$(if $(DTC),DTC="$(DTC)") \
		$(if $(findstring s,$(OPENWRT_VERBOSE)),V=1)
endef

define BuildPackage/Barebox/Defaults
  Build/Configure/Default = $$$$(Build/Configure/Barebox)
  Build/Compile/Default = $$$$(Build/Compile/Barebox)
endef

define BuildPackage/Barebox
  $(eval $(call BuildPackage/Barebox/Defaults))
  $(foreach type,$(if $(DUMP),$(BAREBOX_TARGETS),$(BUILD_VARIANT)), \
    $(eval $(call Build/Barebox/Target,$(type)))
  )
  $(eval $(call Build/DefaultTargets))
  $(foreach type,$(if $(DUMP),$(BAREBOX_TARGETS),$(BUILD_VARIANT)), \
    $(call BuildPackage,barebox-$(type))
  )
endef
