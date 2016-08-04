#
# Copyright (C) 2014 Dog Hunter LLC
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=v8m-rb
PKG_VERSION:=3.14.5
PKG_RELEASE:=6

PKG_SOURCE_PROTO:=git
GIT_SOURCE:=https://github.com/paul99/v8m-rb.git

PKG_BUILD_PARALLEL:=3

include $(INCLUDE_DIR)/package.mk

define Package/v8m-rb
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Google v8 javascript engine
  URL:=https://github.com/paul99/v8m-rb/tree/dm-mipsbe-3.14
  DEPENDS:=+libstdcpp +libc +libpthread
endef

define Package/v8m-rb/description
 	Port of Google v8 javascript engine to MIPS architecture
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	(git clone $(GIT_SOURCE) -b dm-mipsbe-3.14 $(PKG_BUILD_DIR); \
	cd $(PKG_BUILD_DIR); \
	make dependencies);
endef

define Build/Compile
	CC="$(TOOLCHAIN_DIR)/bin/mips-openwrt-linux-gcc" \
	CXX="$(TOOLCHAIN_DIR)/bin/mips-openwrt-linux-g++" \
	AR="$(TOOLCHAIN_DIR)/bin/mips-openwrt-linux-ar" \
	RANLIB="$(TOOLCHAIN_DIR)/bin/mips-openwrt-linux-ranlib" \
	LINK="$(TOOLCHAIN_DIR)/bin/mips-openwrt-linux-g++" \
	LD="$(TOOLCHAIN_DIR)/bin/mips-openwrt-linux-g++" \
	PATH="/usr/bin/:$(PATH)" \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) DESTDIR="$(PKG_INSTALL_DIR)" mips.release werror=no library=shared snapshot=off
endef

define Package/v8m-rb/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/out/mips.release/lib.target/libv8.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,v8m-rb))
