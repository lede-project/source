#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#-DCMAKE_INSTALL_PREFIX=/home/arturo/Scaricati/TARBALLS/lapack-3.6.0/build/install_temp -DBUILD_SHARED_LIBS=ON ../

include $(TOPDIR)/rules.mk

PKG_NAME:=liblapack
PKG_VERSION:=3.6.1
PKG_RELEASE:=1

PKG_SOURCE:=lapack-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://www.netlib.org/lapack/

PKG_BUILD_DIR:=$(BUILD_DIR)/lapack-$(PKG_VERSION)

PKG_BUILD_DEPENDS:=python/host @INSTALL_GFORTRAN

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/liblapack
  TITLE:=Free open source implementation of the Object Exchange (OBEX) protocol.
  SECTION:=libs
  DEFAULT:=m
  CATEGORY:=Libraries
  DEPENDS:=+libpthread +libc +libstdcpp +libgfortran
  URL:=https://gitorious.org/liblapack
endef

define Package/liblapack/description
	LAPACK is written in Fortran 90 and provides routines for solving systems
	of simultaneous linear equations, least-squares solutions of linear systems
	of equations, eigenvalue problems, and singular value problems.
endef

CMAKE_OPTIONS = \
        -DCMAKE_INSTALL_PREFIX=/usr \
		-DBUILD_SHARED_LIBS=ON \
		-DCMAKE_Fortran_COMPILER=$(REAL_GNU_TARGET_NAME)-gfortran

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/include \
		$(1)/usr/lib \
		$(1)/usr/lib/pkgconfig/

	$(CP) $(PKG_BUILD_DIR)/CBLAS/include/*.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/LAPACKE/include/*.h $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/*.pc $(1)/usr/lib/pkgconfig/
endef

define Package/liblapack/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,liblapack))
