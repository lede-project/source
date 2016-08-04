#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=openobex
PKG_VERSION:=1.7.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-Source.tar.gz
PKG_SOURCE_URL:=@SF/openobex
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)-Source

#PKG_SOURCE_URL:=git://gitorious.org/openobex/mainline.git
#PKG_SOURCE_VERSION:=2e1c0fc5a8f5e41886141a6b402d00db735ccf49

# PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/openobex
  TITLE:=Open Source implementation of the OBject EXchange protocol
  SECTION:=utils
  DEFAULT:=m
  CATEGORY:=Utilities
  DEPENDS:=+libusb-1.0 +bluez-libs +eudev +libxml2
  TITLE:=Free open source implementation of the Object Exchange (OBEX) protocol.
  URL:=https://gitorious.org/openobex
endef

CMAKE_OPTIONS = \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -DBUILD_DOCUMENTATION=OFF

define Build/Prepare
	$(Build/Prepare/Default)
	$(RM) -f $(PKG_BUILD_DIR)/CMakeLists.txt
	$(CP) ./files/CMakeLists.txt $(PKG_BUILD_DIR)/CMakeLists.txt
endef

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/include \
		$(1)/usr/lib \
		$(1)/usr/lib/pkgconfig/

	$(CP) $(PKG_INSTALL_DIR)/usr/include/openobex $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libopenobex.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/openobex.pc $(1)/usr/lib/pkgconfig/
endef

define Package/openobex/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/* $(1)/sbin/
	$(INSTALL_DIR) $(1)/lib/udev/rules.d
	$(CP) $(PKG_BUILD_DIR)/udev/openobex.rules $(1)/lib/udev/rules.d/60-openobex.rules
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libopenobex.so* $(1)/usr/lib/
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,openobex))
