#
# Copyright (C) 2014 Arduino LLC
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=modbus
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=0.0.12
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NPM_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://registry.npmjs.org/$(PKG_NPM_NAME)/-/
# PKG_MD5SUM:=f68e9c828c1b55ba00445d3e09872325

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.10.2

include $(INCLUDE_DIR)/package.mk

define Package/node-modbus
    DEPENDS:=+node +libmodbus +python +dbus-python
    SUBMENU:=Node.js
    SECTION:=lang
    CATEGORY:=Languages
    TITLE:=This is binding to native library libmodbus (http://libmodbus.org/)
    URL:=https://www.npmjs.org/package/modbus
endef

define Package/node-modbus/description
    Node.Js binding to native library libmodbus (http://libmodbus.org/)
endef

define Build/Prepare
	/bin/tar xzf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components 1
	$(Build/Patch)
endef

define Build/Compile
	$(MAKE_FLAGS) \
    GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
    npm_config_arch=mips \
    npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
    PREFIX="$(PKG_INSTALL_DIR)/usr/" \
    $(STAGING_DIR_HOSTPKG)/bin/npm install --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/node-modbus/install
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

$(eval $(call BuildPackage,node-modbus))
