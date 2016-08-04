
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=oled-utils
PKG_VERSION:=1.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://download.linino.org/pkg-src/
PKG_MD5SUM:=07e992166232ebf5e29493e82c78406

include $(INCLUDE_DIR)/package.mk

define Package/oled-utils
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+USE_EGLIBC:librt +libc +libstdcpp
  TITLE:=OLED utilities and examples
  URL:=http://download.linino.org/
endef

define Package/oled-utils/description
	OLED utilities and examples
endef

define Build/Compile
	# NOOP
endef

define Package/oled-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/root/image_gif
	$(INSTALL_DIR) $(1)/root/image
	
	$(CP) ./files/1.1bpp $(1)/root/image_gif
	$(CP) ./files/2.1bpp $(1)/root/image_gif
	$(CP) ./files/3.1bpp $(1)/root/image_gif
	
	$(CP) ./files/linino_log.1bpp $(1)/root/image
	$(CP) ./files/linino_log1.1bpp $(1)/root/image
	$(CP) ./files/linino_log2.1bpp $(1)/root/image
	
	$(INSTALL_BIN) ./files/oled $(1)/usr/bin
	$(INSTALL_BIN) ./files/gpio_enable $(1)/usr/bin
	$(INSTALL_BIN) ./files/gpio_disable $(1)/usr/bin
endef

define Package/oled-utils/postrm
#!/bin/sh
# check if we are on a real system
if [ -d /root/image ]
then 
	rm -r /root/image
fi

if [ -d /root/image_gif ]
then 
	rm -r /root/image_gif
fi
endef

$(eval $(call BuildPackage,oled-utils))
