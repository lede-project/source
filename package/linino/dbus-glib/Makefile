#
# Copyright (C) 2007-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dbus-glib
PKG_VERSION:=0.106
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://dbus.freedesktop.org/releases/dbus-glib/
# PKG_MD5SUM:=86ea60ba2118a1b9deafe8257f6a6a1a

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk


define Package/libdbus-glib
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=D-Bus Glib bindings
  URL:=http://dbus.freedesktop.org/doc/dbus-glib/
  DEPENDS:=+libdbus +glib2 $(ICONV_DEPENDS) $(INTL_DEPENDS)
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static \
	--disable-tests \
	--disable-xml-docs \
	--disable-doxygen-docs \

CONFIGURE_VARS+= \
	ac_cv_func_posix_getpwnam_r="yes" \
	ac_cv_have_abstract_sockets="yes" \
	CPPFLAGS="$$$$CPPFLAGS -I$(ICONV_PREFIX)/include" \
	CPPFLAGS="$$$$CPPFLAGS -I$(INTL_PREFIX)/include" \
	LDFLAGS="$$$$LDFLAGS -L$(ICONV_PREFIX)/lib" \
	LDFLAGS="$$$$LDFLAGS -L$(INTL_PREFIX)/lib" \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/dbus-1.0/dbus
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/dbus-1.0/dbus/*.h \
		$(1)/usr/include/dbus-1.0/dbus/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libdbus-glib-1.{la,a,so*} \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/dbus-glib-1.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libdbus-glib/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdbus-glib-1.so.* $(1)/usr/lib/
endef


$(eval $(call BuildPackage,libdbus-glib))
