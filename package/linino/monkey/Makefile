#
# Copyright (C) 2015 Arduino Srl
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=monkey
PKG_VERSION:=1.6.6
PKG_RELEASE:=4

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://monkey-project.com/releases/1.6/
PKG_MD5SUM:=03966619b24c999803806b345fe1916a

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/monkey
  TITLE:=The Monkey Web Server
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Web Servers/Proxies
  DEFAULT:=m
  DEPENDS:=+libpthread +libc +libstdcpp
  URL:=https://github.com/monkey/monkey
endef

CMAKE_OPTIONS = \
        -DCMAKE_PREFIX=/usr \
        -DWITH_SYSTEM_MALLOC=ON \
        -DWITH_BACKTRACE=OFF \
		-DWITH_MUSL=1

define Build/InstallDev
	$(INSTALL_DIR) \
			$(1)/usr/include \
			$(1)/usr/lib
					
	$(CP) -R $(PKG_INSTALL_DIR)/usr/include/monkey $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so $(1)/usr/lib/
endef

define Package/monkey/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/{monkey,mk_passwd} $(1)/usr/sbin/
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so $(1)/usr/lib
	
	$(INSTALL_DIR) $(1)/www
	$(CP) -R $(PKG_INSTALL_DIR)/var/www/monkey $(1)/www
	
	$(INSTALL_DIR) $(1)/etc
	$(CP) -R $(PKG_INSTALL_DIR)/etc/monkey $(1)/etc
endef

define Package/monkey/postinst
#!/bin/sh
if [ -e $${IPKG_INSTROOT}/etc/monkey/monkey.conf ]
then
	sed -i -e s/www-data/root/g $${IPKG_INSTROOT}/etc/monkey/monkey.conf
fi

if [ ! -d $${IPKG_INSTROOT}/var/run ]
then
    mkdir -p $${IPKG_INSTROOT}/var/run
fi

if [ ! -d $${IPKG_INSTROOT}/var/log ]
then
    mkdir -p $${IPKG_INSTROOT}/var/log
fi
endef

define Package/monkey/postrm
#!/bin/sh
if [ -d $${IPKG_INSTROOT}/etc/monkey ]
then
    rm -rf $${IPKG_INSTROOT}/etc/monkey
fi

if [ -d $${IPKG_INSTROOT}/www/monkey ]
then
    rm -rf $${IPKG_INSTROOT}/www/monkey
fi

exit 0
endef

$(eval $(call BuildPackage,monkey))
