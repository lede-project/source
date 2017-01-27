include $(TOPDIR)/rules.mk

PKG_NAME:=mt76-firmware
PKG_RELEASE=1

PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=

PKG_SOURCE_URL:=https://github.com/openwrt/mt76
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2017-01-15
PKG_SOURCE_VERSION:=85d12fea1abe8654f694dc6594781f85168aa2c2
PKG_MIRROR_HASH:=d07c019a3f836860409c58c303195df832f5dc31168201c3fa7d23f4f5927a79

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/mt76-firmware-default
  SECTION:=firmware
  CATEGORY:=Firmware
  URL:=$(PKG_SOURCE_URL)
  DEPENDS:=
endef

define Package/mt76-firmware
$(Package/mt76-firmware-default)
  TITLE:=MediaTek MT76x2/MT7603 firmware
  DEPENDS:=+mt76-firmware-mt7603 \
	  +mt76-firmware-mt7628 \
	  +mt76-firmware-mt7662
endef

define Package/mt76-firmware-mt7603
$(Package/mt76-firmware-default)
  TITLE:=MediaTek MT7603 firmware
endef

define Package/mt76-firmware-mt7603/install
	$(INSTALL_DIR) $(1)/lib-firmware
	cp \
		$(PKG_BUILD_DIR)/firmware/mt7603_e1.bin \
		$(PKG_BUILD_DIR)/firmware/mt7603_e2.bin \
		$(1)/lib/firmware
endef

define Package/mt76-firmware-mt7628
$(Package/mt76-firmware-default)
  TITLE:=MediaTek MT7628 firmware
endef

define Package/mt76-firmware-mt7628/install
	$(INSTALL_DIR) $(1)/lib-firmware
	cp \
		$(PKG_BUILD_DIR)/firmware/mt7628_e1.bin \
		$(PKG_BUILD_DIR)/firmware/mt7628_e2.bin \
		$(1)/lib/firmware
endef

define Package/mt76-firmware-mt7662
$(Package/mt76-firmware-default)
  TITLE:=MediaTek MT7662 firmware
endef

define Package/mt76-firmware-mt7662/install
	$(INSTALL_DIR) $(1)/lib-firmware
	cp \
		$(PKG_BUILD_DIR)/firmware/mt7662_rom_patch.bin \
		$(PKG_BUILD_DIR)/firmware/mt7662.bin \
		$(1)/lib/firmware
endef

$(eval $(call BuildPackage,mt76-firmware-mt7603))
$(eval $(call BuildPackage,mt76-firmware-mt7628))
$(eval $(call BuildPackage,mt76-firmware-mt7662))
$(eval $(call BuildPackage,mt76-firmware))
