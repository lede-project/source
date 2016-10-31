
include $(TOPDIR)/rules.mk

PKG_NAME:=libjsoncpp
PKG_VERSION:=1.7.7
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git

PKG_REVISION:=1.7.7
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/open-source-parsers/jsoncpp.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_REVISION).tar.gz

# PKG_BUILD_DIR:=$(BUILD_DIR)/jsoncpp-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS = \
	-DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
	-DJSONCPP_WITH_TESTS=OFF \
	-DJSONCPP_WITH_STRICT_ISO=OFF \
	-DBUILD_SHARED_LIBS=ON

define Package/libjsoncpp
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=implementation of a JSON reader and writer in C++
  DEPENDS:=+libstdcpp
  URL:=https://github.com/open-source-parsers/jsoncpp
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/json
	$(CP) $(PKG_INSTALL_DIR)/usr/include/json/*.h $(1)/usr/include/json
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib
endef


define Package/libjsoncpp/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib
endef

$(eval $(call BuildPackage,libjsoncpp))
