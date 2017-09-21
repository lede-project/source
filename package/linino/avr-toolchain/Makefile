#
# Copyright (C) 2014 Dog Hunter AG
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=avr-toolchain
PKG_VERSION:=3.5.4
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://download.linino.org/sources/
PKG_SOURCE:=avr-354-mips-musl.tar.gz
PKG_MD5SUM:=42f1680186fcf9283bbd6958b410a700

include $(INCLUDE_DIR)/package.mk

define Package/avr-toolchain
	SECTION:=devel
	CATEGORY:=Development
	TITLE:=Tools for AVR
	DEPENDS:=+zlib +libc +libstdcpp
endef

define Package/avr-toolchain/description
	The AVR toolchain package contains a linker, an assembler, and other tools for handling object files
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/temp
	/bin/tar xvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR)/temp --strip-components=1
endef

define Build/Configure
	# NOOP
endef

define Build/Compile
	# NOOP
endef

define Package/avr-toolchain/install
	$(INSTALL_DIR) $(1)/usr/avr
	
	$(CP) -av $(PKG_BUILD_DIR)/temp/* $(1)/usr/avr
endef

define Package/avr-toolchain/preinst
#!/bin/sh
# check if we are on a real system
if `grep -qs '/opt' /proc/mounts`
then	
	sed -i '/export PATH/ s|$$|:\/opt\/usr\/avr\/bin|' /etc/profile
else
	sed -i '/export PATH/ s|$$|:\/usr\/avr\/bin|' /etc/profile
fi
endef

define Package/avr-toolchain/postrm
#!/bin/sh
# check if we are on a real system
if `grep -qs '/opt' /proc/mounts`
then
	rm -rf /opt/usr/avr
	sed -i 's/:\/opt\/usr\/avr\/bin//g' /etc/profile
else
	rm -rf /usr/avr
	sed -i 's/:\/usr\/avr\/bin//g' /etc/profile
fi
endef

$(eval $(call BuildPackage,avr-toolchain))
