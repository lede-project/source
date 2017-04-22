#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bluez
PKG_VERSION:=5.44
PKG_RELEASE:=7

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=@KERNEL/linux/bluetooth/
PKG_MD5SUM:=94273617129ced06612fcb9f5273d14c

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Nicolas Thill <nico@openwrt.org>

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/bluez/Default
  TITLE:=Bluetooth
  URL:=http://www.bluez.org/
endef

define Package/bluez-examples
$(call Package/bluez/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= python example apps
  DEPENDS:=+python
endef

define Package/bluez-examples/description
  contains many examples apps for bluetooth, requiring python
endef

define Package/bluez-6lowpan-daemon
$(call Package/bluez/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= Nordic BLE 6lowpan daemon
  DEPENDS:=+bluez-libs +bluez-utils +kmod-ipv6 +kmod-6lowpan +kmod-bluetooth_6lowpan +luci-app-6lowpand
endef

define Package/bluez-6lowpan-daemon/description
 	Nordic BLE 6lowpan daemon
endef

define Package/bluez-libs
$(call Package/bluez/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE+= library
  DEPENDS:=+libpthread +kmod-bluetooth
endef

define Package/bluez-utils
$(call Package/bluez/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= utilities
  DEPENDS:=+bluez-libs +libpthread +librt +glib2 +libncurses +libreadline $(INTL_DEPENDS) $(ICONV_DEPENDS)
endef

define Package/bluez-daemon
$(call Package/bluez/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= daemon
  DEPENDS:=+bluez-libs +bluez-utils +dbus +libical $(INTL_DEPENDS) $(ICONV_DEPENDS)
endef

define Package/bluez-daemon/conffiles
/etc/bluetooth/main.conf
/etc/bluetooth/network.conf
/etc/bluetooth/input.conf
/etc/bluetooth/proximity.conf
/etc/bluetooth/audio.conf
/etc/config/bluetooth
endef

define Package/bluez-6lowpan-daemon/conffiles
/etc/bluetooth/bluetooth_6lowpand.conf
endef

CONFIGURE_ARGS += \
	--enable-static \
	--enable-shared \
	--enable-client \
	--enable-datafiles \
	--enable-experimental \
	--enable-library \
	--enable-monitor \
	--enable-obex \
	--enable-threads \
	--enable-tools \
	--disable-android \
	--disable-cups \
	--disable-manpages \
	--disable-sixaxis \
	--disable-systemd \
	--disable-test \
	--disable-udev \
	--enable-deprecated

TARGET_CPPFLAGS += \
	-D_GNU_SOURCE

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/bluetooth $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libbluetooth.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/bluez.pc $(1)/usr/lib/pkgconfig/
endef

define Package/bluez-examples/install
	$(INSTALL_DIR) $(1)/usr/bin/bluez
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/test/* $(1)/usr/bin/bluez/
endef

define Package/bluez-libs/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libbluetooth.so.* $(1)/usr/lib/
endef

define Package/bluez-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/bccmd $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/bluemoon $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/btmon $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/tools/btmgmt $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ciptool $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hciattach $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hciconfig $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hcidump $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hcitool $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/hex2hcd $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/l2ping $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/l2test $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rctest $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rfcomm $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/sdptool $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/attrib/gatttool $(1)/usr/bin/
endef

define Package/bluez-6lowpan-daemon/install
	$(INSTALL_DIR) $(1)/etc/bluetooth
	$(INSTALL_DATA) ./files/bluetooth_6lowpand.conf $(1)/etc/bluetooth/bluetooth_6lowpand.conf

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/bluetooth_6lowpand.init $(1)/etc/init.d/bluetooth_6lowpand

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/bluetooth_6lowpand $(1)/usr/bin/
endef

define Package/bluez-daemon/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/bluetooth/bluetoothd $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/bluetoothctl $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/bluetooth/obexd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/bluetooth.config $(1)/etc/config/bluetooth
	$(INSTALL_DIR) $(1)/etc/dbus-1/system.d/
	$(INSTALL_DATA) ./files/bluetooth.dbus $(1)/etc/dbus-1/system.d/bluetooth.conf
	$(INSTALL_DIR) $(1)/etc/bluetooth
	$(INSTALL_DATA) ./files/main.conf $(1)/etc/bluetooth/main.conf
	$(INSTALL_DATA) ./files/audio.conf $(1)/etc/bluetooth/audio.conf
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/profiles/network/network.conf $(1)/etc/bluetooth/network.conf
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/profiles/input/input.conf $(1)/etc/bluetooth/input.conf
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/profiles/proximity/proximity.conf $(1)/etc/bluetooth/proximity.conf
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/bluetoothd.init $(1)/etc/init.d/bluetoothd
endef

define Package/bluez-daemon/postinst
#!/bin/sh
if [ -e $${IPKG_INSTROOT}/etc/bluetooth/main.conf ]
then
    BOARD=$$(awk '/machine/ {print $$4}' /proc/cpuinfo)
    MACSHORT=`ifconfig wlan0 | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}' | tail -c 6 | sed s/://g`
    sed -i -e "s/BlueZ/$${BOARD}-$${MACSHORT}/g" $${IPKG_INSTROOT}/etc/bluetooth/main.conf
    case $${BOARD} in
        'Tian')
            sed -i -e s/#Class/Class/g $${IPKG_INSTROOT}/etc/bluetooth/main.conf
            sed -i -e "/Class/ s|$$|0x20041C|" $${IPKG_INSTROOT}/etc/bluetooth/main.conf
            ;;
        *)
            ;;
    esac
fi
exit 0
endef

$(eval $(call BuildPackage,bluez-examples))
$(eval $(call BuildPackage,bluez-libs))
$(eval $(call BuildPackage,bluez-utils))
$(eval $(call BuildPackage,bluez-daemon))
$(eval $(call BuildPackage,bluez-6lowpan-daemon))
