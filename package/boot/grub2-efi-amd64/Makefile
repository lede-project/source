#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=grub
PKG_VERSION:=2.02~beta3
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://alpha.gnu.org/gnu/grub \
	http://gnualpha.uib.no/grub/ \
	http://mirrors.fe.up.pt/pub/gnu-alpha/grub/ \
	http://www.nic.funet.fi/pub/gnu/alpha/gnu/grub/
PKG_HASH:=30ec3d555e52a702c3eef449872ef874eff28b320f40b55ffc47f70db8e5ada1

PKG_FIXUP:=autoreconf
HOST_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=grub2-efi-amd64/host

PKG_SSP:=0

PKG_FLAGS:=nonshared

GRUB_FONT:=unifont
GRUB_FONT_VERSION:=9.0.06
GRUB_FONT_SOURCE:=$(GRUB_FONT)-$(GRUB_FONT_VERSION).bdf.gz
GRUB_FONT_SOURCE_URL:=http://unifoundry.com/pub/$(PKG_NAME)-$(PKG_VERSION)/font-builds/
GRUB_FONT_SOURCE_HASH:=4246c4773ed70f78a7e27ff1118fd257a280d1102200265ad5d58bb2011195ef

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/grub2-efi-amd64
  CATEGORY:=Boot Loaders
  SECTION:=boot
  TITLE:=GRand Unified Bootloader
  URL:=http://www.gnu.org/software/grub/
  DEPENDS:=@TARGET_x86_64_efi
endef

define Download/grub_font
  FILE:=$(GRUB_FONT_SOURCE)
  URL:=$(GRUB_FONT_SOURCE_URL)
  HASH:=$(GRUB_FONT_SOURCE_HASH)
endef
$(eval $(call Download,grub_font))

define Host/Prepare
	$(Host/Prepare/Default)
	gzip -d -c $(DL_DIR)/$(GRUB_FONT_SOURCE) > $(HOST_BUILD_DIR)/$(GRUB_FONT).bdf
endef	

define Host/Install
	$(Host/Install/Default)
	$(INSTALL_DIR) $(1)/share/fonts/$(GRUB_FONT)/
	$(CP) $(HOST_BUILD_DIR)/$(GRUB_FONT).bdf $(1)/share/fonts/$(GRUB_FONT)/
endef

define Package/grub2-efi-amd64-editenv
  CATEGORY:=Utilities
  SECTION:=utils
  SUBMENU:=Boot Loaders
  TITLE:=Grub2 Environment editor
  URL:=http://www.gnu.org/software/grub/
  DEPENDS:=@TARGET_x86_64_efi
endef

define Package/grub2-efi-amd64-editenv/description
	Edit grub2 environment files.
endef

HOST_BUILD_PREFIX := $(STAGING_DIR_HOST)

CONFIGURE_ARGS += \
	--with-platform=efi \
	--target=$(REAL_GNU_TARGET_NAME) \
	--disable-werror \
	--disable-nls \
	--disable-device-mapper \
	--disable-libzfs \

HOST_CONFIGURE_ARGS += \
	--with-platform=efi \
	--target=$(REAL_GNU_TARGET_NAME) \
	--enable-grub-mkfont \
	--sbindir="$(STAGING_DIR_HOST)/bin" \
	--disable-werror \
	--disable-libzfs \
	--disable-nls

HOST_MAKE_FLAGS += \
	TARGET_RANLIB=$(TARGET_RANLIB) \
	LIBLZMA=$(STAGING_DIR_HOST)/lib/liblzma.a

define Host/Configure
	$(SED) 's,(RANLIB),(TARGET_RANLIB),' $(HOST_BUILD_DIR)/grub-core/Makefile.in
	$(Host/Configure/Default)
endef

define Package/grub2-efi-amd64-editenv/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/grub-editenv $(1)/usr/sbin/
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,grub2-efi-amd64))
$(eval $(call BuildPackage,grub2-efi-amd64-editenv))
