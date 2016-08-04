# Dog Hunter AG 2014

include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=sqlite3
PKG_NAME:=node-$(PKG_NPM_NAME)
PKG_VERSION:=3.1.4
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NPM_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://registry.nodejitsu.com/$(PKG_NPM_NAME)/-/
# PKG_MD5SUM:=290e0f3a13df7d9aaeb6b60296314c61

PKG_BUILD_DEPENDS:=node
PKG_NODE_VERSION:=0.10.46

include $(INCLUDE_DIR)/package.mk

define Package/node-sqlite3
	DEPENDS:=+node +libsqlite3
	SUBMENU:=Node.js
	SECTION:=lang
	CATEGORY:=Languages
	TITLE:=Node.js package to Asynchronous, non-blocking SQLite3 bindings
	URL:=https://www.npmjs.org/package/sqlite3
endef

define Package/node-sqlite3/description
	Asynchronous, non-blocking SQLite3 bindings
endef

define Build/Prepare
	/bin/tar xzf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components 1
	$(Build/Patch)
endef

define Build/Compile
	$(MAKE_FLAGS) \
	GYPFLAGS="-Dv8_use_mips_abi_hardfloat=false -Dv8_can_use_fpu_instructions=false" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	npm_config_arch=mips \
	npm_config_nodedir=$(BUILD_DIR)/node-v$(PKG_NODE_VERSION)/ \
	PREFIX="$(PKG_INSTALL_DIR)/usr/" \
	$(STAGING_DIR_HOST)/bin/npm install --target_arch=mips -g $(PKG_BUILD_DIR)
endef

define Package/node-sqlite3/install
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

$(eval $(call BuildPackage,node-sqlite3))
