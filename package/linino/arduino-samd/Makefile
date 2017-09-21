#
# Copyright (C) 2016 Arduino Srl
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arduino-samd
PKG_VERSION:=1.6.15
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://download.linino.org/pkg-src/
PKG_SOURCE:=arduino-samd-$(PKG_VERSION).tar.bz2
# PKG_MD5SUM:=d284f3079632fa3d95caefe534fdc1cf

include $(INCLUDE_DIR)/package.mk

define Package/arduino-samd
	SECTION:=devel
	CATEGORY:=Development
	TITLE:=SAMD platform for Arduino IDE v1.8.x
	DEPENDS:=+libc +libstdcpp +arduino-files-18
endef

define Package/arduino-samd/description
	Configuration Files for Arduino IDE
endef

# Distribution URL doesn't always have the correct version
# Using the OpenWrt/Linino mirror provides a stable version
define Download/cmsis
	URL:=http://download.linino.org/pkg-src/
	FILE:=CMSIS-4.5.0.tar.bz2
	# MD5SUM:=$(MD5SUM_NODE)
endef

# Distribution URL doesn't always have the correct version
# Using the OpenWrt/Linino mirror provides a stable version
define Download/cmsisatmel
	URL:=http://download.linino.org/pkg-src/
	FILE:=CMSIS-Atmel-1.1.0.tar.bz2
	# MD5SUM:=$(MD5SUM_NODE)
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(TAR) xvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components=1
	$(eval $(call Download,cmsis))
	$(eval $(call Download,cmsisatmel))
	$(TAR) xvf $(DL_DIR)/CMSIS-4.5.0.tar.bz2 -C $(PKG_BUILD_DIR)
	$(TAR) xvf $(DL_DIR)/CMSIS-Atmel-1.1.0.tar.bz2 -C $(PKG_BUILD_DIR)
endef

define Build/Configure
	# NOOP
endef

define Build/Compile
	# NOOP
endef

define Package/arduino-samd/install
	$(INSTALL_DIR) $(1)/opt/arduino-18/hardware/arduino
	$(INSTALL_DIR) $(1)/opt/arduino-18/hardware/tools

	$(CP) -r $(PKG_BUILD_DIR)/$(PKG_VERSION) $(1)/opt/arduino-18/hardware/arduino/samd
	$(CP) -r $(PKG_BUILD_DIR)/CMSIS $(1)/opt/arduino-18/hardware/tools
	$(CP) -r $(PKG_BUILD_DIR)/CMSIS-Atmel $(1)/opt/arduino-18/hardware/tools
endef

define Package/arduino-samd/preinst
#!/bin/sh
# check if we are on a real system
if [ ! -d /opt/arduino-18 ]
then
	mkdir -p /opt/arduino-18
fi
endef

define Package/arduino-samd/postrm
#!/bin/sh
# check if we are on a real system
if [ -d /opt/arduino-18/hardware/arduino/samd ]
then
	rm -rvf /opt/arduino-18/hardware/arduino/samd
fi
endef

$(eval $(call BuildPackage,arduino-samd))
