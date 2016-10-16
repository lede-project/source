#
# Copyright (C) 2016 Jiang Yutang <yutang.jiang@nxp.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

PKG_NAME:=rcw
PKG_VERSION:=fsl-sdk-v2.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://git.freescale.com/git/cgit.cgi/ppc/sdk/rcw.git
PKG_SOURCE_VERSION:=6ae9086528019ab55968da05c25bd319737c8e62

include $(INCLUDE_DIR)/package.mk

define Package/rcw
  SECTION:=firmware
  CATEGORY:=Firmware
  URL:=http://git.freescale.com/git/cgit.cgi/ppc/sdk/rcw.git
  TITLE:=freescale reset configuration word
  DEPENDS:=@TARGET_layerscape
endef

define Package/rcw/config
	if PACKAGE_rcw
		config RCW_LS1043ARDB
			string "rcw for ls1043ardb"
			default ls1043ardb/RR_FQPP_1455/rcw_1600.bin
			depends on TARGET_layerscape_64b_DEVICE_ls1043ardb || TARGET_layerscape_32b_DEVICE_ls1043ardb
	endif
endef

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR)
endef

RCW_DEVICES:=$(CONFIG_RCW_LS1043ARDB)
define Package/rcw/install
	$(foreach rcw,$(RCW_DEVICES), $(CP) $(PKG_BUILD_DIR)/$(rcw) $(KDIR)/rcw.bin ;)
endef

$(eval $(call BuildPackage,rcw))
