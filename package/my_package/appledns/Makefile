include $(TOPDIR)/rules.mk

PKG_NAME:=appledns
PKG_VERSION=20170527
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=net
	CATEGORY:=Network
	PKGARCH:=all
	TITLE:=Apple's CDN data in China.
	DEPENDS:=+libc
endef

define Package/$(PKG_NAME)/description
   Apple's CDN data in China.
endef

define Package/$(PKG_NAME)/postinst
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(CP) ./files/* $(1)/

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
