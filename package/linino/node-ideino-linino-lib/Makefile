#
# Copyright (C) 2014 Dog Hunter AG 
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=ideino-linino-lib
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=0.1.2
PKG_RELEASE:=3

PKG_SOURCE_PROTO:=git

PKG_SOURCE_URL:=https://github.com/ideino/ideino-linino-lib.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=7f65c03d2dafe1d861fda6096c2ab7d031b7b31d
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
# PKG_SOURCE_URL:=https://github.com/ideino/ideino-linino-lib/archive/

# PKG_INSTALL:=1
PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.11.0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+node +libstdcpp
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Linino library for Dog Hunter boards
  URL:=https://github.com/ideino/ideino-linino-lib
endef

define Package/$(PKG_NAME)/description
	Linino library for Dog Hunter boards
endef

# define Build/Prepare
#	/bin/tar zxvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components=1
#	$(Build/Patch)
# endef

define Build/Compile
	$(MAKE_FLAGS) \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	npm_config_arch=mips \
	npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
	PREFIX="$(PKG_INSTALL_DIR)/usr/" \
	$(STAGING_DIR_HOSTPKG)/bin/npm install --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules $(1)/usr/lib
endef

define Package/$(PKG_NAME)/postrm
#!/bin/sh
if `grep -qs '/opt' /proc/mounts`
then
	rm -rvf /opt/usr/lib/node_modules/$(PKG_NPM_NAME)
else
	rm -rvf /usr/lib/node_modules/$(PKG_NPM_NAME)
fi
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
