#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=avr-gdb
PKG_VERSION:=3.5.0-atmel
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME).tar.bz2
PKG_SOURCE_URL:=http://distribute.atmel.no/tools/opensource/Atmel-AVR-GNU-Toolchain/3.5.0/
#PKG_MD5SUM:=39e654460c9cdd80200a29ac020cfe11

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1
PKG_LICENSE:=GPL-3.0+

include $(INCLUDE_DIR)/package.mk

define Package/avr-gdb/Default
  SECTION:=devel
  CATEGORY:=Development
  DEPENDS:=+!USE_MUSL:libthread-db +PACKAGE_zlib:zlib
  URL:=http://www.gnu.org/software/avr-gdb/
endef

define Package/avr-gdb
$(call Package/avr-gdb/Default)
  TITLE:=GNU AVR Debugger
  DEPENDS+=+libreadline +libncurses +zlib
endef

define Package/avr-gdb/description
GDB, the GNU Project debugger, allows you to see what is going on `inside'
another program while it executes -- or what another program was doing at the
moment it crashed.
endef

define Package/avr-gdbserver
$(call Package/avr-gdb/Default)
  TITLE:=Remote server for GNU Debugger
endef

define Package/avr-gdbserver/description
GDBSERVER is a program that allows you to run GDB on a different machine than the
one which is running the program being debugged.
endef

# XXX: add --disable-werror to prevent build failure with arm
CONFIGURE_ARGS+= \
	--with-system-readline \
	--without-expat \
	--without-lzma \
	--disable-werror \
	--target=avr \
	--disable-nls \
	--disable-binutils \
	--disable-docs

CONFIGURE_VARS+= \
	ac_cv_search_tgetent="$(TARGET_LDFLAGS) -lncurses -lreadline"

define Build/Prepare
	/bin/tar xvf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip-components=1
	$(call Build/Patch)
endef

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		CPPFLAGS="$(TARGET_CPPFLAGS)" \
		MAKEINFO=true all
endef

define Build/Install
	$(MAKE) -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		CPPFLAGS="$(TARGET_CPPFLAGS)" \
		install-gdb
endef

define Package/avr-gdb/install
	$(INSTALL_DIR) $(1)/usr/avr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/gdb $(1)/usr/avr/bin/avr-gdb
endef

$(eval $(call BuildPackage,avr-gdb))
