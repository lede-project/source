#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=uucp
PKG_VERSION:=1.07
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).orig.tar.gz
PKG_SOURCE_URL:=http://ftp.debian.org/debian/pool/main/u/uucp/
PKG_MD5SUM:=64c54d43787339a7cced48390eb3e1d0

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/uucp
  SECTION:=net
  CATEGORY:=Network
  TITLE:=copy files to or from another (usually *nix) system
  URL:=http://packages.debian.org/etch/uucp
endef

define Package/uucp/description
	UUCP is used to transfer mail, news and random files between systems which are not
	connected by more modern networks. The communication can be made via modems, direct
	(hard-wired) serial connections or via an IP connection.
endef

CONFIGURE_ARGS += \
	--enable-static \
	--enable-shared

define Package/uucp/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/{cu,tstuu,uu{chk,cico,conv,cp,log,name,pick,sched,stat,to,x,xqt}} $(1)/usr/bin/
endef

$(eval $(call BuildPackage,uucp))
