#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bluezdev
PKG_VERSION:=5.45
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git

PKG_REVISION:=677a3456afa886e759582a96e1b9d47992b17c52
PKG_SOURCE_VERSION=$(PKG_REVISION)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=http://git.kernel.org/pub/scm/bluetooth/bluez.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_REVISION).tar.gz

PKG_FIXUP:=libtool autoreconf

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Nicolas Thill <nico@openwrt.org>

PKG_BUILD_PARALLEL:=3
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/bluezdev/Default
  TITLE:=Bluetooth Trunk
  URL:=http://www.bluez.org/
endef

define Package/bluezdev-examples
$(call Package/bluezdev/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= python example apps
  DEPENDS:=+python
endef

define Package/bluezdev-examples/description
  contains many examples apps for bluetooth, requiring python
endef

define Package/bluezdev-6lowpan-daemon
$(call Package/bluezdev/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= Nordic BLE 6lowpan daemon
  DEPENDS:=+bluezdev-libs +bluezdev-utils +kmod-ipv6 +kmod-6lowpan +kmod-bluetooth_6lowpan +luci-app-6lowpand
endef

define Package/bluezdev-6lowpan-daemon/description
 	Nordic BLE 6lowpan daemon (Trunk)
endef

define Package/bluezdev-libs
$(call Package/bluezdev/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE+= library
  DEPENDS:=+libpthread +kmod-bluetooth
endef

define Package/bluezdev-utils
$(call Package/bluezdev/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= utilities
  DEPENDS:=+bluezdev-libs +libpthread +librt +glib2 +libncurses +libreadline $(INTL_DEPENDS) $(ICONV_DEPENDS) +libdbus
endef

define Package/bluezdev-daemon
$(call Package/bluezdev/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= daemon
  DEPENDS:=+bluezdev-libs +bluezdev-utils +dbus +libical $(INTL_DEPENDS) $(ICONV_DEPENDS)
endef

define Package/bluezdev-daemon/conffiles
/etc/bluetooth/main.conf
/etc/bluetooth/network.conf
/etc/bluetooth/input.conf
/etc/bluetooth/proximity.conf
/etc/bluetooth/audio.conf
/etc/config/bluetooth
endef

define Package/bluezdev-6lowpan-daemon/conffiles
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

define Package/bluezdev-examples/install
	$(INSTALL_DIR) $(1)/usr/bin/bluezdev
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/test/* $(1)/usr/bin/bluezdev/
endef

define Package/bluezdev-libs/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libbluetooth.so.* $(1)/usr/lib/
endef

define Package/bluezdev-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/bccmd $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/bluemoon $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/btattach $(1)/usr/bin/
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
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/mpris-proxy $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rctest $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rfcomm $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/sdptool $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/attrib/gatttool $(1)/usr/bin/
endef

define Package/bluezdev-6lowpan-daemon/install
	$(INSTALL_DIR) $(1)/etc/bluetooth
	$(INSTALL_DATA) ./files/bluetooth_6lowpand.conf $(1)/etc/bluetooth/bluetooth_6lowpand.conf

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/bluetooth_6lowpand.init $(1)/etc/init.d/bluetooth_6lowpand

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/bluetooth_6lowpand $(1)/usr/bin/
endef

define Package/bluezdev-daemon/install
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

define Package/bluezdev-daemon/postinst
#!/bin/sh
if [ -e $${IPKG_INSTROOT}/etc/bluetooth/main.conf ]
then
    BOARD=$$(awk '/machine/ {print $$4}' /proc/cpuinfo)
    MACSHORT=`ifconfig wlan0 | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}' | tail -c 6 | sed s/://g`
    sed -i -e "s/bluez/$${BOARD}-$${MACSHORT}/g" $${IPKG_INSTROOT}/etc/bluetooth/main.conf
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

$(eval $(call BuildPackage,bluezdev-examples))
$(eval $(call BuildPackage,bluezdev-libs))
$(eval $(call BuildPackage,bluezdev-utils))
$(eval $(call BuildPackage,bluezdev-daemon))
$(eval $(call BuildPackage,bluezdev-6lowpan-daemon))
