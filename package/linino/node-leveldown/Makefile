#
# Copyright (C) 2014 Arduino LLC
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=leveldown-mips
PKG_NAME:=node-leveldown
PKG_VERSION:=1.4.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NPM_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://registry.npmjs.org/$(PKG_NPM_NAME)/-/
# PKG_MD5SUM:=54cc3fa9dfc7a44b5153e45b8cae9fdb

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.11.0

include $(INCLUDE_DIR)/package.mk

define Package/node-leveldown
  DEPENDS:=+node
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=A Low-level Node.js LevelDB binding
  URL:=https://www.npmjs.org/package/leveldown
endef

define Package/node-leveldown/description
	A Node.js LevelDB binding, primary backend for LevelUP - MIPS implementation
endef

define Build/Prepare
	/bin/tar xzf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components 1
	$(Build/Patch)
endef

# EXTRA_LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \

define Build/Compile
	$(MAKE_FLAGS) \
	npm_config_arch=$(CONFIG_ARCH) \
	npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	PREFIX="$(PKG_INSTALL_DIR)/usr/" \
	$(STAGING_DIR_HOSTPKG)/bin/npm install --build-from-source --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/node-leveldown/install
	$(INSTALL_DIR) $(1)/usr/lib/
	
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules $(1)/usr/lib/
endef

define Package/$(PKG_NAME)/postrm
#!/bin/sh
# check if we are on a real system
if `grep -qs '/opt' /proc/mounts`
then
    rm -rvf /opt/usr/lib/node_modules/$(PKG_NPM_NAME)
else
    rm -rvf /usr/lib/node_modules/$(PKG_NPM_NAME)
fi
endef

$(eval $(call BuildPackage,node-leveldown))

