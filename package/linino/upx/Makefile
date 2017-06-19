#
# Copyright (C) 2011-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=upx
PKG_VERSION:=3.91
PKG_RELEASE:=2

# PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-src.tar.gz
# PKG_SOURCE_URL:=@SF/upx
# PKG_MD5SUM:=1253da46eac54a217eb73e2d44818e53
# PKG_CAT:=bzcat

PKG_BUILD_DEPENDS:=upx/host

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/upx/upx.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)-src
PKG_SOURCE_VERSION:=v$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_SOURCE_VERSION)-src.tar.gz

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)-src
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)-src

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/upx
	TITLE:=UPX - the Ultimate Packer for eXecutables
	URL:=https://github.com/upx/upx.git
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:=+libc +libstdcpp +zlib +libucl
endef

define Package/upx/description
	UPX - the Ultimate Packer for eXecutables
endef

define Host/Compile
	rm -f $(HOST_BUILD_DIR)/src/.depend
	$(MAKE) UPX_LZMADIR="$(BUILD_DIR_HOST)/lzma-4.65" -C $(HOST_BUILD_DIR)/src \
		UPX_UCLDIR="$(STAGING_DIR_HOSTPKG)" \
		CXXFLAGS="$(CXXFLAGS) -DUCL_NO_ASM" \
		CXXFLAGS_WERROR="" LDFLAGS="-L$(STAGING_DIR_HOSTPKG)/lib -lucl -static" \
		CXX="$(HOSTCXX)"
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/src/upx.out $(STAGING_DIR_HOSTPKG)/bin/upx
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/upx
endef

define Build/Compile
	rm -f $(BUILD_DIR)/src/.depend
	$(MAKE) UPX_LZMADIR="$(BUILD_DIR_HOST)/lzma-4.65" -C $(PKG_BUILD_DIR)/src \
		CXXFLAGS="$(TARGET_CXXFLAGS) -DUCL_NO_ASM" \
		CXXFLAGS_WERROR="" LDFLAGS="$(TARGET_LDFLAGS) -lucl" \
		CC="$(REAL_GNU_TARGET_NAME)-gcc" \
		CXX="$(REAL_GNU_TARGET_NAME)-g++"
endef

define Package/upx/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/upx.out $(1)/usr/bin/upx
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,upx))
