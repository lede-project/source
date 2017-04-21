#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cairo
PKG_VERSION:=1.14.8
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://www.cairographics.org/releases/
# PKG_MD5SUM:=d3e1a1035ae563812d4dd44a74fb0dd0
PKG_FIXUP:=libtool

include $(INCLUDE_DIR)/package.mk

PKG_INSTALL:=1

define Package/cairo
  SECTION:=xorg-libraries
  CATEGORY:=Xorg
  SUBMENU:=libraries
  TITLE:=Multi-platform 2D graphics library
  DEPENDS:=+fontconfig +libpng +libexpat +pixman +liblzo \
  +FEATURE_drawing-backend_DirectFB:directfb \
  +FEATURE_drawing-backend_libX11:libX11
  URL:=http://cairographics.org/
endef

define Package/cairo/description
Cairo is a 2D graphics library. The cairo API provides operations similar
to the drawing operators of PostScript and PDF. Operations in cairo
include stroking and filling cubic Bézier splines, transforming and
compositing translucent images, and antialiased text rendering. All
drawing operations can be transformed by any affine transformation
(scale, rotation, shear, etc.).
endef

CONFIGURE_VARS+=FONTCONFIG_LIBS="-lfreetype -lfontconfig -lz -lexpat"

CONFIGURE_ARGS+= \
	--enable-freetype=yes \
	--disable-win32 \
	--enable-xlib-xrender=no \
	$(if $(CONFIG_FEATURE_drawing-backend_DirectFB), --enable-directfb, --disable-directfb) \
	$(if $(CONFIG_FEATURE_drawing-backend_libX11), --enable-xlib --with-x, --disable-xlib --without-x)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.{so*,a,la} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* $(1)/usr/lib/pkgconfig/
	$(INSTALL_DIR) $(1)/usr/include/cairo/
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/include/cairo/* $(1)/usr/include/cairo/
endef

define Package/cairo/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,cairo))
