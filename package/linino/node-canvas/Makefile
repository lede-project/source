#
# Copyright (C) 2014 Arduino LLC
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=canvas
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=1.6.5
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NPM_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://registry.npmjs.org/$(PKG_NPM_NAME)/-/
# PKG_MD5SUM:=fa4034f98a2fbee4a48f0ff36b8c25dd

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=6.11.0

include $(INCLUDE_DIR)/package.mk

define Package/node-canvas
  DEPENDS:=+node +cairo +giflib +pixman +fontconfig +directfb +pango +libjpeg
  SUBMENU:=Node.js
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Canvas graphics API backed by Cairo
  URL:=https://www.npmjs.org/package/canvas
endef

define Package/node-canvas/description
	Node canvas is a Cairo backed Canvas implementation for NodeJS.
endef

define Build/Prepare
	/bin/tar xzf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components 1
	$(Build/Patch)
endef

EXTRA_CXXFLAGS += -I$(STAGING_DIR)/usr/include/cairo/ -I$(STAGING_DIR)/usr/include/glib-2.0
EXTRA_LDFLAGS += -lfreetype -lfontconfig -lcairo -lpng -lgif

define Build/Compile
	$(MAKE_FLAGS) \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/ -L$(STAGING_DIR)/usr/lib" \
	CFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS) $(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS) -I$(STAGING_DIR)/usr/include" \
	CXXFLAGS="$(TARGET_CXXFLAGS) $(EXTRA_CXXFLAGS) $(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS) -I$(STAGING_DIR)/usr/include" \
	npm_config_arch=mips \
	npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
	PREFIX="$(PKG_INSTALL_DIR)/usr/" \
	$(STAGING_DIR_HOSTPKG)/bin/npm install --build-from-source --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/node-canvas/install
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

$(eval $(call BuildPackage,node-canvas))
