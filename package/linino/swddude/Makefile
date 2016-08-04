#
# Copyright (c) dog hunter AG - Zug - CH
# General Public License version 2 (GPLv2)
# Author Federico Vaga <federicov@linino.org>
#

include $(TOPDIR)/rules.mk

PKG_NAME:=swddude
PKG_VERSION:=1.1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=swddude-$(PKG_VERSION)
GIT_SOURCE:=https://github.com/linino/swddude.git

include $(INCLUDE_DIR)/package.mk

define Package/swddude
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=SWD Downloader/UploaDEr
  URL:=https://github.com/linino/swddude
  DEPENDS:=+libftdi +libpthread +libusb-compat +libstdcpp
endef

define Package/swddude/description
 SWDDUDE is a full featured program for programming ARM CPU on SWD bus.
endef

define Build/Prepare
	(cd $(PKG_BUILD_DIR);\
	 if [ ! -d swddure-src ]; then \
	   git clone --recursive $(GIT_SOURCE) swddude-src; \
	 fi; \
	); \
        $(call Build/Prepare/Default)
endef

CFLAGS:=$(TARGET_CFLAGS) -lstdc++
LDFLAGS:=$(TARGET_LDFLAGS) -lstdc++

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/swddude-src/source \
		ARCH=mips \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		CFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		$(PKG_NAME) release
endef


define Package/swddude/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/swddude-src/source/release/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,swddude))
