#
# Copyright (C) 2016 Arduino Srl
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arm-toolchain
PKG_VERSION:=48q3
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://download.linino.org/sources/
PKG_SOURCE:=gcc-arm-none-eabi-4_8-2014q3-20161201-linux-mips-openwrt-musl.tar.bz2
PKG_MD5SUM:=1026ec346a74c7b94abfb82ab48933b3

include $(INCLUDE_DIR)/package.mk

define Package/arm-toolchain
	SECTION:=devel
	CATEGORY:=Development
	TITLE:=Tools for ARM
	DEPENDS:=+zlib +libc +libstdcpp
endef

define Package/arm-toolchain/description
	The ARM toolchain package contains a linker, an assembler, and other tools for handling object files
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

define Package/arm-toolchain/install
	$(INSTALL_DIR) $(1)/usr/arm-none-eabi

	$(CP) -av $(PKG_BUILD_DIR)/temp/* $(1)/usr/arm-none-eabi
endef

define Package/arm-toolchain/preinst
#!/bin/sh
# check if we are on a real system
if `grep -qs '/opt' /proc/mounts`
then
	sed -i '/export PATH/ s|$$|:\/opt\/usr\/arm-none-eabi\/bin|' /etc/profile
else
	sed -i '/export PATH/ s|$$|:\/usr\/arm-none-eabi\/bin|' /etc/profile
fi
endef

define Package/arm-toolchain/postrm
#!/bin/sh
# check if we are on a real system
if `grep -qs '/opt' /proc/mounts`
then
	rm -rf /opt/usr/avr
	sed -i 's/:\/opt\/usr\/arm-none-eabi\/bin//g' /etc/profile
else
	rm -rf /usr/avr
	sed -i 's/:\/usr\/arm-none-eabi\/bin//g' /etc/profile
fi
endef

$(eval $(call BuildPackage,arm-toolchain))
