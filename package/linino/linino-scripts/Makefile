#
# Copyright (c) 2013 Arduino LLC. All right reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=linino-scripts
PKG_VERSION:=1.2
PKG_RELEASE:=9

include $(INCLUDE_DIR)/package.mk

define Package/linino-scripts
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Custom Linino scripts
  DEPENDS:=+kmod-loop
endef

define Build/Compile
	# NOOP
endef

define Package/linino-scripts/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d

	$(INSTALL_BIN) ./files/usr/bin/blink-start $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/blink-stop $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/boot-complete-notify $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/update-file-available $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/run-sysupgrade $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/wifi-live-or-reset $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/wifi-reset-and-reboot $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/wifi-reload $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/reset-to-factory-anyway $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/run-avrdude $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/run-swddude $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/merge-sketch-with-bootloader.lua $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/wifi-reset-button-pressed $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/wifi-reset-button-released $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/kill-bridge $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/run-bridge $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/reset-mcu $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/special-storage-available $(1)/usr/bin/
	$(INSTALL_BIN) ./files/usr/bin/pretty-wifi-info.lua $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/upgrade-all $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/extract-built-date $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/extract-built-date-from-sysupgrade-image $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/prod $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/cali $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/mcu-reset $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/tst $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/tsto $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/overlay-only $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/update-all $(1)/usr/bin

	$(INSTALL_BIN) ./files/etc/init.d/rename-wifi-if-access-point $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/delete_uhttpd_cert $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/handle_wifi_reset $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/rngd-turn-off $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/alljoynfixboot $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/avahifixboot $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/board-panel $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/defconfig $(1)/etc/init.d
endef

$(eval $(call BuildPackage,linino-scripts))

