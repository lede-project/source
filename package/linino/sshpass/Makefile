
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licenmake under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sshpass
PKG_VERSION:=1.05
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/sshpass
PKG_MD5SUM:=c52d65fdee0712af6f77eb2b60974ac7

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

CONFIGURE_ARGS += \
	--prefix=/usr

define Package/sshpass
  SECTION:=devel
  CATEGORY:=Utilities
  DEPENDS:=+USE_EGLIBC:librt +libelf +dropbear
  TITLE:=sshpass
  URL:=http://sourceforge.net/projects/sshpass
endef

define Package/sshpass/description
	Non-interactive ssh password auth
endef

define Package/sshpass/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/sshpass $(1)/usr/bin/
endef

$(eval $(call BuildPackage,sshpass))
