#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#-DCMAKE_INSTALL_PREFIX=/home/arturo/Scaricati/TARBALLS/lapack-3.6.0/build/install_temp -DBUILD_SHARED_LIBS=ON ../

include $(TOPDIR)/rules.mk

PKG_NAME:=wyliodrin-server
PKG_VERSION:=3.16-git
PKG_RELEASE:=3

PKG_SOURCE_PROTO:=git

PKG_REVISION:=86b6819d4f4c9a3ce147d3de7103f3024d6469ee
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/Wyliodrin/wyliodrin-server.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_REVISION).tar.gz

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/wyliodrin-server
  TITLE:=Wiliodryn Server
  SECTION:=utils
  DEFAULT:=m
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +libcrypto +libc +libstdcpp +libhiredis +jansson +libevent2 +libstrophe +libcurl +libfuse +libopenssl
  URL:=https://github.com/Wyliodrin/wyliodrin-server.git
endef

define Package/wyliodrin-server/description
	WTalk is a light memory-safe C client that allows boards to communicate with clients.
endef

define Package/wyliodrin-server/conffiles
/etc/wyliodrin/settings_arduinogalileo.json
/etc/wyliodrin/settings_beagleboneblack.json
/etc/wyliodrin/settings_edison.json
/etc/wyliodrin/settings_minnowboardmax.json
/etc/wyliodrin/settings_raspberrypi.json
/etc/wyliodrin/settings_udooneo.json
endef

CMAKE_OPTIONS = \
        -DCMAKE_INSTALL_PREFIX=/usr

define Package/wyliodrin-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/{wyliodrind,wyliodrin_hypervisor}  $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/wyliodrin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/setup/settings_*.json  $(1)/etc/wyliodrin
endef

$(eval $(call BuildPackage,wyliodrin-server))
