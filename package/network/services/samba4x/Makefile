#
# Copyright (C) 2007-2017 OpenWrt
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=samba4x
PKG_VERSION:=4.7.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/samba-team/samba.git
PKG_SOURCE_VERSION:=d1e69845e28c20a491c4cd60c712b46ddfcb9dc0
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_MIRROR_HASH:=97e6fa7c0bc951ab19265fb2065695a2bef05c1fee94d74319124a96a3879f07
PKG_LICENSE:=GPL-3.0

PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
# for $(LINUX_VERSION)
include $(INCLUDE_DIR)/kernel.mk
# for $(VERSION_DIST)
include $(INCLUDE_DIR)/version.mk

define Package/samba4x-server
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Samba $(PKG_VERSION) SMB/CIFS server
  URL:=http://www.samba.org/
  DEPENDS:=+zlib +libpopt +libgnutls +tdb
endef

define Package/samba4x-server/description
  The Samba software suite is a collection of programs that implements the
  SMB protocol for UNIX systems, allowing you to serve files and printers to
  Windows, NT, OS/2 and DOS clients. This protocol is sometimes also referred
  to as the LanManager or Netbios protocol.
endef

CONFIGURE_VARS += \
	CPP="$(TARGET_CROSS)cpp" \
	python_LDFLAGS="" \
	python_LIBDIR=""

CONFIGURE_CMD = ./buildtools/bin/waf

CONFIGURE_PREFIX = /usr

CONFIGURE_ARGS = \
		--target=$(GNU_TARGET_NAME) \
		--program-prefix="" \
		--prefix=$(CONFIGURE_PREFIX) \
		--exec-prefix=$(CONFIGURE_PREFIX) \
		--bindir=$(CONFIGURE_PREFIX)/bin \
		--sbindir=$(CONFIGURE_PREFIX)/sbin \
		--libexecdir=$(CONFIGURE_PREFIX)/lib \
		--sysconfdir=/etc \
		--datadir=$(CONFIGURE_PREFIX)/share \
		--localstatedir=/var \
		--mandir=$(CONFIGURE_PREFIX)/man \
		--infodir=$(CONFIGURE_PREFIX)/info

CONFIGURE_ARGS += \
		--hostcc="$(HOSTCC)" \
		--cross-compile \
		--cross-answers="$(PKG_BUILD_DIR)/cache.txt" \
		--disable-avahi \
		--disable-cephfs \
		--disable-cups \
		--disable-fault-handling \
		--disable-glusterfs \
		--disable-iprint \
		--disable-python \
		--disable-rpath \
		--disable-rpath-install \
		--disable-rpath-private-install \
		--enable-fhs \
		--without-acl-support \
		--without-ads \
		--without-ad-dc \
		--without-automount \
		--without-dnsupdate \
		--without-iconv \
		--without-ldap \
		--without-lttng \
		--without-ntvfs-fileserver \
		--without-pam \
		--without-quotas \
		--without-systemd \
		--without-utmp \
		--without-winbind \
		--nopyc --nopyo

define Build/Configure
	cp ./files/samba4.cache.txt $(PKG_BUILD_DIR)/cache.txt;
	echo -e "\nChecking uname sysname type: \"$(VERSION_DIST)\" \
	\nChecking uname release type: \"$(LINUX_VERSION)\" \
	\nChecking uname machine type: \"$(ARCH)\" \
	\nChecking uname version type: \"$(VERSION_DIST) Linux-$(LINUX_VERSION) $(shell date +%Y-%m-%d)\"\n" >> $(PKG_BUILD_DIR)/cache.txt;

	$(call Build/Configure/Default,configure)
endef

define Build/Compile
	(cd $(PKG_BUILD_DIR); \
		./buildtools/bin/waf build)
endef

define Package/samba4x-server/install
	$(INSTALL_DIR) $(1)/etc/config $(1)/etc/samba $(1)/etc/init.d
	$(INSTALL_DATA) ./files/samba.config $(1)/etc/config/samba
	$(INSTALL_DATA) ./files/smb.conf.template $(1)/etc/samba
	$(INSTALL_BIN) ./files/samba.init $(1)/etc/init.d/samba
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/samba/*.so* $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/{smbd,nmbd} $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/smbpasswd $(1)/usr/bin
endef

define Package/samba4x-server/conffiles
/etc/config/samba
/etc/samba/smb.conf.template
/etc/samba/smbpasswd
endef

$(eval $(call BuildPackage,$(PKG_NAME)-server))
