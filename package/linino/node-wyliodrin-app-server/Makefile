#
# Copyright (C) 2014 Arduino Srl
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=wyliodrin-app-server
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=1.0.7-git
PKG_RELEASE:=3

PKG_SOURCE_PROTO:=git

PKG_REVISION:=ac92329ced533d27f076b4c49dd9df32ff520ed0
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/Wyliodrin/wyliodrin-app-server.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.11.0

include $(INCLUDE_DIR)/package.mk

define Package/node-wyliodrin-app-server
  DEPENDS:=+node +node-authenticate-pam
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Wyliodrin Server node app
  URL:=https://github.com/Wyliodrin/wyliodrin-app-server
endef

define Package/node-wyliodrin-app-server/description
	Wyliodrin Server node app
endef

define Build/Compile
	$(MAKE_FLAGS) \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	npm_config_arch=mips \
	npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
	PREFIX="$(PKG_INSTALL_DIR)/usr/" \
	$(STAGING_DIR_HOSTPKG)/bin/npm install --target_arch=mips --build-from-source -g $(PKG_BUILD_DIR)
endef

define Package/node-wyliodrin-app-server/install
	$(INSTALL_DIR) $(1)/usr/bin

	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) -R $(PKG_INSTALL_DIR)/usr/lib/node_modules $(1)/usr/lib/
	ln -sf ../lib/node_modules/wyliodrin-app-server/startup.js $(1)/usr/bin/startwylio
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

$(eval $(call BuildPackage,node-wyliodrin-app-server))
