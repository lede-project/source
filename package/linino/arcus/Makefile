#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libArcus
PKG_VERSION:=16.06.22
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/Ultimaker/libArcus.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=c33e01da61d412bf9d3f025bc5e1701b8cb1d6d5
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_INSTALL:=1

PKG_BUILD_DEPENDS:=python3/host +libprotobuf
PKG_USE_MIPS16:=0 # MIPS16 prevents protobuf's usage of the 'sync' asm-opcode

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libArcus
  TITLE:=This library contains a C++ and Python3 class for creating a socket in a thread and using this socket.
  SECTION:=libs
  DEFAULT:=m
  CATEGORY:=Libraries
  DEPENDS:=+libpthread +libc +libstdcpp +libprotobuf
  URL:=https://gitorious.org/libArcus
endef

CMAKE_OPTIONS = \
        -DCMAKE_PREFIX=/usr \
		-DBUILD_PYTHON=OFF

define package/libArcus/Description
    This library contains a C++ and Python3 class for creating a socket in a thread and using
    this socket to send and receive messages based on the Protocol Buffers library.
    It is designed to facilitate the communication between Cura and its backend and similar code.
endef

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/include/Arcus \
		$(1)/usr/lib \

	$(CP) $(PKG_INSTALL_DIR)/usr/include/Arcus/*.h $(1)/usr/include/Arcus
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib* $(1)/usr/lib/
	$(CP) -R $(PKG_INSTALL_DIR)/usr/lib/cmake $(1)/usr/lib/
endef

define Package/libArcus/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib* $(1)/usr/lib/
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,libArcus))
