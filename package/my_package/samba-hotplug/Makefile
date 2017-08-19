#
# Copyright (C) 2010-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=samba-hotplug
PKG_VERSION:=1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/samba-hotplug
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Samba autoconfig hotplug script.
  MAINTAINER:=GuoGuo
endef

define Package/samba-hotplug/description
A hotplug script to config Samba share automatically.
endef

define Build/Compile
endef

define Package/samba-hotplug/install
	$(INSTALL_DIR) $(1)/etc/hotplug.d/block
	$(INSTALL_BIN) ./files/hotplug.sh $(1)/etc/hotplug.d/block/20-samba-hotplug
endef

$(eval $(call BuildPackage,samba-hotplug))
