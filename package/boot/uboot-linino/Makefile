#
# Copyright (C) 2013 Linino.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=u-boot
PKG_VERSION:=linino
PKG_RELEASE:=7
PKG_MD5SUM:=

PKG_REVISION:=29cb27bd059ce23435bc47620098157bdc6c4f9a

PKG_SOURCE_PROTO:=git

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_REVISION).tar.gz
PKG_SOURCE_URL:=https://github.com/artynet/uboot.git
PKG_SOURCE_VERSION:=$(PKG_REVISION)

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_TARGETS:=bin

CAL_DATA:=art-linino.bin

include $(INCLUDE_DIR)/package.mk

define uboot/Default
  TITLE:=
  CONFIG:=
  IMAGE:=
endef

define uboot/linino-yun
  TITLE:=U-boot for Linino project
endef

define uboot/linino-yun-mini
  TITLE:=U-boot for Linino project
endef

define uboot/linino-one
  TITLE:=U-boot for Linino project
endef

define uboot/linino-freedog
  TITLE:=U-boot for Linino project
endef

define uboot/linino-lei
  TITLE:=U-boot for Linino project
endef

define uboot/linino-chiwawa
  TITLE:=U-boot for Linino project
endef

define uboot/linino-chiwawa-101
  TITLE:=U-boot for Linino project
endef

define uboot/linino-tian
  TITLE:=U-boot for Linino project
endef

UBOOTS:=linino-yun linino-one linino-freedog linino-lei linino-chiwawa linino-yun-mini linino-tian linino-chiwawa-101

define Package/uboot/template
define Package/uboot-ar71xx-$(1)
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=$(2)
  DEPENDS:=@TARGET_ar71xx
  URL:=http://www.linino.org
  DEFAULT:=y if (TARGET_ar71xx_generic_Linino)
  VARIANT:=$(1)
endef
endef

define Package/uboot-ar71xx-linino/config
	source $(SOURCE)/Config.in
endef

ifdef CONFIG_COMPRESSED_UBOOT
      export COMPRESSED_UBOOT=1
endif

ifdef CONFIG_HORNET_1_1_WAR
      export CONFIG_HORNET_1_1_WAR=1
endif

ifdef CONFIG_NEW_DDR_TAP_CAL
      export NEW_DDR_TAP_CAL=1
endif

ifdef CONFIG_LININO_DDR2
      export CONFIG_LININO_DDR2=1
endif

define BuildUbootPackage
	$(eval $(uboot/Default))
	$(eval $(uboot/$(1)))
	$(call Package/uboot/template,$(1),$(TITLE))
endef


ifdef BUILD_VARIANT
$(eval $(call uboot/$(BUILD_VARIANT)))
UBOOT_CONFIG:=$(if $(CONFIG),$(CONFIG),$(BUILD_VARIANT))
UBOOT_IMAGE:=$(if $(IMAGE),$(IMAGE),openwrt-$(BOARD)-$(BUILD_VARIANT)-u-boot.bin)
endif

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./files/* $(PKG_BUILD_DIR)
	$(CP) ./$(CAL_DATA) $(BIN_DIR)/linino-caldata.bin && touch $(BIN_DIR)/linino-caldata.bin
endef

define Build/Configure
	$(MAKE) -C $(PKG_BUILD_DIR) \
		mrproper
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(UBOOT_CONFIG)_config
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE=$(TARGET_CROSS)
endef

define Package/uboot/install/template
define Package/uboot-ar71xx-$(1)/install
	#$(INSTALL_DIR) $$(1)
	rm -rf $$(1)
	## Enable if COMPRESSED_BOOT is enabled
	#$(CP) $(PKG_BUILD_DIR)/tuboot.bin $(BIN_DIR)/$(2)
	$(CP) $(PKG_BUILD_DIR)/u-boot.bin $(BIN_DIR)/$(2)
endef
endef

$(foreach u,$(UBOOTS), \
	$(eval $(call Package/uboot/install/template,$(u),u-boot-$(u).bin)) \
)

$(foreach u,$(UBOOTS), \
	$(eval $(call BuildUbootPackage,$(u))) \
	$(eval $(call BuildPackage,uboot-ar71xx-$(u))) \
)
