#
# Copyright (C) 2014 Dog Hunter 2014
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=johnny-five
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=0.9.59
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NPM_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://registry.npmjs.org/$(PKG_NPM_NAME)/-/
# PKG_MD5SUM:=7b51dc3280c1dd03a0d32abc1b9ccb02

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.11.0

include $(INCLUDE_DIR)/package.mk

define Package/node-johnny-five
  DEPENDS:=+node
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=The JavaScript Hardware Programming Framework
  URL:=https://www.npmjs.org/package/johnny-five
endef

define Package/node-johnny-five/description
	The JavaScript Hardware Programming Framework. Use with: Arduino (all models), Electric Imp, Beagle Bone, 
	Intel Galileo & Edison, Linino One, Pinoccio, Raspberry Pi, Spark Core, TI Launchpad and more! 
endef

define Build/Prepare
	/bin/tar xzf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components=1
	$(Build/Patch)
endef

define Build/Compile
	$(MAKE_FLAGS) \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	npm_config_arch=mips \
	npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
	PREFIX="$(PKG_INSTALL_DIR)/usr/" \
	$(STAGING_DIR_HOSTPKG)/bin/npm install --build-from-source --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/node-johnny-five/install
	$(INSTALL_DIR) $(1)/usr/lib
	
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules $(1)/usr/lib
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

$(eval $(call BuildPackage,node-johnny-five))
