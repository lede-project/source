#
# Copyright (C) 2016 Arduino Srl
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=arduino-avr
PKG_VERSION:=1.6.19
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://download.linino.org/pkg-src/
PKG_SOURCE:=arduino-avr-$(PKG_VERSION).tar.bz2
# PKG_MD5SUM:=d284f3079632fa3d95caefe534fdc1cf

include $(INCLUDE_DIR)/package.mk

define Package/arduino-avr
	SECTION:=devel
	CATEGORY:=Development
	TITLE:=AVR platform for Arduino IDE v1.8.x
	DEPENDS:=+libc +libstdcpp +arduino-files-18
endef

define Package/arduino-avr/description
	AVR platform files for Arduino IDE
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(TAR) xvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components=1
endef

define Build/Configure
	# NOOP
endef

define Build/Compile
	# NOOP
endef

define Package/arduino-avr/install
	$(INSTALL_DIR) $(1)/opt/arduino-18/hardware/arduino

	$(CP) -r $(PKG_BUILD_DIR)/$(PKG_VERSION) $(1)/opt/arduino-18/hardware/arduino/avr
endef

define Package/arduino-avr/preinst
#!/bin/sh
# check if we are on a real system
if [ ! -d /opt/arduino-18 ]
then
	mkdir -p /opt/arduino-18
fi
endef

define Package/arduino-avr/postrm
#!/bin/sh
# check if we are on a real system
if [ -d /opt/arduino-18/hardware/arduino/avr ]
then
	rm -rvf /opt/arduino-18/hardware/arduino/avr
fi
endef

$(eval $(call BuildPackage,arduino-avr))
