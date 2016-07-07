#
# Copyright (C) 2010-2015 Jo-Philipp Wich <jo@mein.io>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=collections
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2

include $(INCLUDE_DIR)/package.mk

define Package/collections/Default
  SECTION:=base
  CATEGORY:=Base system
  SUBMENU:=Collections
  MAINTAINER:=Daniel Dickinson <lede@daniel.thecshore.com>
endef

define Package/collections-common-device
  TITLE:=Common packages for most devices
  DEPENDS:=+dropbear +mtd +uci +opkg +netifd +fstools +uclient-fetch +logd
endef

define Package/collections-common-device/description
  Metapackage for selecting packages that should be included on most devices.
endef

define Package/collections-router
  TITLE:=Packages useful on most routers
  DEPENDS:=+dnsmasq +iptables +IPV6:ip6tables +ppp +ppp-mod-pppoe +firewall +IPV6:odhcpd +IPV6:odhcp6c +collections-common-device
endef

define Package/collections-router/description
  Metapackage for selecting packages that should be included on most routers.
endef

define Package/collections-nas
  TITLE:=Packages useful on most NAS devices
  DEPENDS:=+block-mount +fdisk +lsblk +mdadm +collections-common-device
endef

define Package/collections-nas/description
  Metapackage for selecting packages that should be included on most NAS devices.
endef

define Build/Prepare
	true
endef

define Build/Configure
	true
endef

define Build/Compile
	true
endef

define Package/collections-common-device/install
	true
endef

Package/collections-router/install=$(Package/collections-common-device/install)
Package/collections-nas/install=$(Package/collections-common-device/install)

$(eval $(call BuildPackage,collections-common-device))
$(eval $(call BuildPackage,collections-router))
$(eval $(call BuildPackage,collections-nas))
