#

include $(TOPDIR)/rules.mk

PKG_NAME:=aria-ng
PKG_VERSION:=0.2.0
PKG_RELEASE:=2

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk

define Package/aria-ng
	SECTION:=net
	CATEGORY:=Network
	TITLE:=A web frontend making aria2 easier to use
	URL:=https://github.com/mayswind/AriaNg
	PKGARCH:=all
endef

define Package/aria-ng/description
AriaNg is a web frontend making aria2 easier to use. 
AriaNg is written in pure html & javascript. 
AriaNg uses responsive layout, and supports any desktop or mobile devices.
endef

define Build/Compile
endef

define Package/aria-ng/install
	$(INSTALL_DIR) $(1)/www/aria-ng
	$(CP) ./files/* $(1)/www/aria-ng
endef

$(eval $(call BuildPackage,aria-ng))
