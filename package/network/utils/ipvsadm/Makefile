#
# Copyright (C) 2016-2017 Mauro Mozzarelli
#
# This is free software, licensed under the GNU General Public License
# See /LICENSE for more information.
#
# AUTHOR: Mauro Mozzarelli <mauro@ezplanet.org>
#
include $(TOPDIR)/rules.mk

PKG_NAME:=ipvsadm
PKG_VERSION:=1.29
PKG_MAINTAINER:=Mauro Mozzarelli <mauro@ezplanet.org>
PKG_LICENSE:=GPL-1.0
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://www.kernel.org/pub/linux/utils/kernel/ipvsadm/
PKG_HASH:=c3de4a21d90a02c621f0c72ee36a7aa27374b6f29fd4178f33fbf71b4c66c149

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  TITLE:=IP Virtual Server Configuration Manager
  URL:=http://www.linuxvirtualserver.org
  DEPENDS:= +kmod-nf-ipvs +libnl-tiny +libpopt +wget
endef

define Package/$(PKG_NAME)/description
  IPVS (IP Virtual Server) implements transport-layer load balancing inside the Linux kernel, so called Layer-4 switching.
  ipvsadm is used to set up, maintain or inspect the virtual server table in the Linux kernel.
  The Linux Virtual Server can be used to build scalable network services based on a cluster of two or more nodes.
endef

	#-I$(STAGING_DIR)/usr/include/libnl-tiny -fPIC -DLIBIPVS_USE_NL -D_GNU_SOURCE
TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include/libnl-tiny

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)" \
	LIBS="-lnl-tiny -lpopt" \
	CROSS_COMPILE=wrt

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipvsadm $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipvsadm-save $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipvsadm-restore $(1)/sbin/
	$(CP) ./files/* $(1)/
	chmod +x $(1)/etc/init.d/ipvsadm
	chmod +x $(1)/usr/sbin/checkRealServers
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
