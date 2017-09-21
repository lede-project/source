#
# Copyright (C) 2008-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=colordiff
PKG_VERSION:=1.0.18
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://www.colordiff.org/
PKG_MD5SUM:=07658f09a44f30a3b5c1cea9c132baed
PKG_MAINTAINER:=Roger D <rogerdammit@gmail.com>
PKG_LICENSE:=GPL-3.0

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/colordiff
  SECTION:=devel
  CATEGORY:=Development
  DEPENDS:=+USE_GLIBC:librt +diffutils +perl +perlbase-essential +perlbase-getopt
  TITLE:=colordiff
  URL:=http://www.colordiff.org
endef

define Package/colordiff/description
	The Perl script colordiff is a wrapper for 'diff' and produces the same
	output but with pretty 'syntax' highlighting. Colour schemes can be customized.
endef

define Package/colordiff/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/colordiffrc $(1)/etc

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/bin/{cdiff,colordiff} $(1)/usr/bin/
endef

$(eval $(call BuildPackage,colordiff))
