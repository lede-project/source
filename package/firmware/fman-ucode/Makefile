#
# Copyright (C) 2016 Jiang Yutang <yutang.jiang@nxp.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

PKG_NAME:=fman-ucode
PKG_VERSION:=fsl-sdk-v2.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://git.freescale.com/git/cgit.cgi/ppc/sdk/fm-ucode.git
PKG_SOURCE_VERSION:=b19c645821941493fbef32e616b5a16404259976

include $(INCLUDE_DIR)/package.mk

define Package/fman-ucode
  SECTION:=firmware
  CATEGORY:=Firmware
  URL:=http://git.freescale.com/git/cgit.cgi/ppc/sdk/rcw.git
  TITLE:=freescale fman ucode
  DEPENDS:=@TARGET_layerscape
endef

define Package/fman-ucode/config
	if PACKAGE_fman-ucode
		config FMAN-UCODE_LS1043
			string "fman ucode for ls1043 dpa"
			default fsl_fman_ucode_ls1043_r1.0_108_4_5.bin
			depends on TARGET_layerscape_64b_DEVICE_ls1043ardb || TARGET_layerscape_32b_DEVICE_ls1043ardb
	endif
endef

define Build/Compile
endef

FU_DEVICES:=$(CONFIG_FMAN-UCODE_LS1043)
define Package/fman-ucode/install
	$(foreach fu,$(FU_DEVICES), $(CP) $(PKG_BUILD_DIR)/$(fu) $(KDIR)/fman_ucode.bin ;)
endef

$(eval $(call BuildPackage,fman-ucode))
