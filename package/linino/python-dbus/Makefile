#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dbus-python
PKG_VERSION:=1.2.4
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://dbus.freedesktop.org/releases/dbus-python/
# PKG_MD5SUM:=562c78876219bf458d79a041af18293d

PKG_FIXUP:=libtool
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk
$(call include_mk, python-package.mk)


TARGET_LDFLAGS += \
	-Wl,-rpath=$(STAGING_DIR)/usr/lib

define Package/dbus-python
  SUBMENU:=Python
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Python bindings for dbus
  URL:=http://dbus.freedesktop.org/
  DEPENDS:=+python-base +python-light +python-logging +python-xml +libdbus-glib
endef

define Package/dbus-python/description
  Python bindings for dbus.
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/dbus-1.0/dbus
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/dbus-1.0/dbus/* \
		$(1)/usr/include/dbus-1.0/dbus/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* \
		$(1)/usr/lib/pkgconfig
endef

define Package/dbus-python/install
	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/*.so \
		$(1)$(PYTHON_PKG_DIR)

	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)/dbus
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/dbus/*.py \
		$(1)$(PYTHON_PKG_DIR)/dbus

	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)/dbus/mainloop
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/dbus/mainloop/*.py \
		$(1)$(PYTHON_PKG_DIR)/dbus/mainloop

endef

$(eval $(call BuildPackage,dbus-python))
