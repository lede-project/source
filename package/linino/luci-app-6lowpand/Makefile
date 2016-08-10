#
# Copyright (C) 2008-2014 The LuCI Team <luci@lists.subsignal.org>
#
# This is free software, licensed under the Apache License, Version 2.0 .
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-6lowpand

# Version == major.minor.patch
# increase on new functionality (minor) or patches (patch)
PKG_VERSION:=1.0.0
PKG_URL:=https://bitbucket.org/glennrub/luci_bluetooth_6lowpand

# Release == build
# increase on changes of translation files
PKG_RELEASE:=1

PKG_MAINTAINER:=Glenn Ruben Bakke <Glenn.Ruben.Bakke@nordicsemi.no>

LUCI_TITLE:=LuCI Support for Bluetooh 6lowpand
LUCI_DEPENDS:=+bluez-utils +bluez-6lowpan-daemon

define Package/$(PKG_NAME)/config
# shown in make menuconfig <Help>
help
	$(LUCI_TITLE)
	.
	Version: $(PKG_VERSION)-$(PKG_RELEASE)
	$(PKG_MAINTAINER)
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
