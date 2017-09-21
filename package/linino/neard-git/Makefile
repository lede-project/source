#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=neard-git
PKG_VERSION:=0.15
PKG_RELEASE:=6

PKG_FIXUP:=autoreconf
PKG_SOURCE_PROTO:=git

PKG_REVISION:=421fd97857dee92e258aeea233d3cc6210a21d49
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://git.kernel.org/pub/scm/network/nfc/neard.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Nicolas Thill <nico@openwrt.org>

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/neard-git
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= NFC daemon utilities
  DEPENDS:=+libc +libstdcpp +dbus +libnl +glib2
  URL:=https://01.org/linux-nfc
endef

define Package/neard-git/description
	Near Field Communications is a short-range (a few centimeters or inches) radio technology that enables
	communication between devices for the purposes of tag-reading, peer-to-peer sharing or providing payment information.
endef

define Package/neard-git/conffiles
/etc/dbus-1/system.d/org.neard.conf
endef

define Package/python-neard-git
  TITLE:=neard stack (Python Bindings)
  SECTION:=lang
  DEFAULT:=m
  CATEGORY:=Languages
  SUBMENU:=Python
  DEPENDS:=+libpthread +libc +libstdcpp +python-base +python-light +dbus-python
  URL:=https://01.org/linux-nfc
endef

define Package/python-neard-git/description
	Python test utils for NFC neard stack
endef

CONFIGURE_ARGS += \
	--enable-maintainer-mode \
	--enable-debug \
	--disable-systemd \
	--prefix=/usr \
	--sysconfdir=/etc

TARGET_CFLAGS += -Wno-implicit-function-declaration

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) -R $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/neard.pc $(1)/usr/lib/pkgconfig
endef

define Package/neard-git/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/nfc/neard $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/dbus-1/system.d
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/dbus-1/system.d/org.neard.conf $(1)/etc/dbus-1/system.d
endef

define Package/python-neard-git/install
	$(INSTALL_DIR) $(1)/usr/lib/python2.7/site-packages
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/test/*.py $(1)/usr/lib/python2.7/site-packages

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/test/{test-*,phdc-simple-manager,monitor-near,*-agent,bt-handover} \
		$(1)/usr/bin
endef

$(eval $(call BuildPackage,neard-git))
$(eval $(call BuildPackage,python-neard-git))
