#
# Copyright (C) 2016 GuoGuo <gch981213@gmail.com>
#
# This is free software, licensed under the GNU General Public License v3.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ipip-chnroute
PKG_VERSION:=20160704
PKG_RELEASE:=1
PKG_REV:=ca571051bd4d611631d017b48842f890cb44b0dc

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/17mon/china_ip_list.git
PKG_SOURCE_VERSION:=$(PKG_REV)



include $(INCLUDE_DIR)/package.mk

define Package/ipip-chnroute
	SECTION:=net
	CATEGORY:=Network
	TITLE:=China IP List by ipip.net
	URL:=https://github.com/17mon/china_ip_list
endef

define Package/ipip-chnroute/description

endef

define Build/Compile
endef

define Package/ipip-chnroute/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/china_ip_list.txt $(1)/etc/chnroute.txt

endef

$(eval $(call BuildPackage,ipip-chnroute))
