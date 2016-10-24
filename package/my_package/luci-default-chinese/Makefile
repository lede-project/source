#
# Copyright (C) 2010-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-default-chinese
PKG_VERSION:=1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/luci-default-chinese
  SECTION:=LuCI
  CATEGORY:=LuCI
  TITLE:=Set Simplified Chinese as default language.
  DEPENDS:=@LUCI_LANG_zh-cn
  MAINTAINER:=GuoGuo
endef

define Package/luci-default-chinese/description
Set Simplified Chinese as default language.
endef

define Build/Compile
endef

define Package/luci-default-chinese/install
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/ucidef.sh $(1)/etc/uci-defaults/99-luci-default-chinese
endef

$(eval $(call BuildPackage,luci-default-chinese))
