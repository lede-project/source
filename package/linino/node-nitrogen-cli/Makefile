#
# Copyright (C) 2014 Dog Hunter 2014
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=nitrogen-cli
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=0.2.17
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NPM_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://registry.npmjs.org/$(PKG_NPM_NAME)/-/
# PKG_MD5SUM:=adf99f32190acccbb890d9d16e03a527

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.10.2

include $(INCLUDE_DIR)/package.mk

define Package/node-nitrogen-cli
  DEPENDS:=+node +node-nitrogen
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Provides a command line interface to the Nitrogen service
  URL:=https://www.npmjs.org/package/nitrogen-cli
endef

define Package/node-nitrogen-cli/description
	Nitrogen is a platform for building connected devices. Nitrogen provides the authentication,
	authorization, and real time message passing framework so that you can focus on your device and application
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
	$(STAGING_DIR_HOSTPKG)/bin/npm install --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/node-nitrogen-cli/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules $(1)/usr/lib
	
	$(INSTALL_DIR) $(1)/usr/bin
	ln -sf ../lib/node_modules/nitrogen-cli/n2 $(1)/usr/bin/n2
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

$(eval $(call BuildPackage,node-nitrogen-cli))
