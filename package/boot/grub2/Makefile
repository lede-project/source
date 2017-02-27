#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=grub
PKG_VERSION:=2.02~rc2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://alpha.gnu.org/gnu/grub \
	http://gnualpha.uib.no/grub/ \
	http://mirrors.fe.up.pt/pub/gnu-alpha/grub/ \
	http://www.nic.funet.fi/pub/gnu/alpha/gnu/grub/
PKG_HASH:=053bfcbe366733e4f5a1baf4eb15e1efd977225bdd323b78087ce5fa172fc246

PKG_FIXUP:=autoreconf
HOST_CONFIGURE_PARALLEL:=1
HOST_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=grub2/host

PKG_SSP:=0

PKG_FLAGS:=nonshared

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/grub2
  CATEGORY:=Boot Loaders
  SECTION:=boot
  TITLE:=GRand Unified Bootloader
  URL:=http://www.gnu.org/software/grub/
  DEPENDS:=@TARGET_x86||TARGET_x86_64
endef

define Package/grub2-editenv
  CATEGORY:=Utilities
  SECTION:=utils
  SUBMENU:=Boot Loaders
  TITLE:=Grub2 Environment editor
  URL:=http://www.gnu.org/software/grub/
  DEPENDS:=@TARGET_x86||TARGET_x86_64
endef

define Package/grub2-editenv/description
	Edit grub2 environment files.
endef

ifneq ($(CONFIG_GRUB_EFI),)
GRUB_FONT:=unifont
GRUB_FONT_VERSION:=9.0.06
GRUB_FONT_SOURCE:=$(GRUB_FONT)-$(GRUB_FONT_VERSION).bdf.gz
GRUB_FONT_SOURCE_URL:=@GNU/unifont
GRUB_FONT_SOURCE_HASH:=4246c4773ed70f78a7e27ff1118fd257a280d1102200265ad5d58bb2011195ef

define Download/grub_font
  FILE:=$(GRUB_FONT_SOURCE)
  URL:=$(GRUB_FONT_SOURCE_URL)
  HASH:=$(GRUB_FONT_SOURCE_HASH)
endef
endif

HOST_BUILD_PREFIX := $(STAGING_DIR_HOST)

HOST_CONFIGURE_CMD := $(BASH) ../configure

BIOS_CONFIGURE_VARS += \
	grub_build_mkfont_excuse="don't want fonts"

BIOS_CONFIGURE_ARGS += \
	--target=$(REAL_GNU_TARGET_NAME) \
	--disable-werror \
	--disable-nls \
	--disable-device-mapper \
	--disable-libzfs \
	--disable-grub-mkfont

HOST_BIOS_CONFIGURE_VARS += \
	grub_build_mkfont_excuse="don't want fonts"

HOST_BIOS_CONFIGURE_ARGS += \
	--disable-grub-mkfont \
	--target=$(REAL_GNU_TARGET_NAME) \
	--prefix="$(STAGING_DIR_HOST)" \
	--sbindir="$(STAGING_DIR_HOST)/bin" \
	--disable-werror \
	--disable-libzfs \
	--disable-nls

ifneq ($(CONFIG_GRUB_EFI),)
EFI_CONFIGURE_ARGS += \
	--with-platform=efi \
	--target=$(REAL_GNU_TARGET_NAME) \
	--disable-werror \
	--disable-nls \
	--disable-device-mapper \
	--disable-libzfs \
	--enable-grub-mkfont

HOST_EFI_CONFIGURE_ARGS += \
	--enable-grub-mkfont \
	--with-platform=efi \
	--target=$(REAL_GNU_TARGET_NAME) \
	--prefix="$(STAGING_DIR_HOST)" \
	--sbindir="$(STAGING_DIR_HOST)/bin" \
	--disable-werror \
	--disable-libzfs \
	--disable-nls
endif

HOST_MAKE_FLAGS += \
	TARGET_RANLIB=$(TARGET_RANLIB) \
	LIBLZMA=$(STAGING_DIR_HOST)/lib/liblzma.a

define Host/Prepare
	$(Host/Prepare/Default)
	gzip -d -c $(DL_DIR)/$(GRUB_FONT_SOURCE) > $(HOST_BUILD_DIR)/$(GRUB_FONT).bdf
endef

define Host/Configure/bios
	$(INSTALL_DIR) $(HOST_BUILD_DIR)/grub-bios
	$(if $(HOST_CONFIGURE_PARALLEL),+)(cd $(HOST_BUILD_DIR)/grub-bios; \
		if [ -x ../configure ]; then \
			$(CP) $(SCRIPT_DIR)/config.{guess,sub} $(HOST_BUILD_DIR)/grub-bios/ && \
			$(HOST_BIOS_CONFIGURE_VARS) \
			$(2) \
			$(HOST_CONFIGURE_CMD) \
			$(HOST_BIOS_CONFIGURE_ARGS) \
			$(1); \
		fi \
	)
endef

define Host/Configure/efi
	$(INSTALL_DIR) $(HOST_BUILD_DIR)/grub-efi
	$(if $(HOST_CONFIGURE_PARALLEL),+)(cd $(HOST_BUILD_DIR)/grub-efi; \
		if [ -x ../configure ]; then \
			$(CP) $(SCRIPT_DIR)/config.{guess,sub} $(HOST_BUILD_DIR)/grub-efi/ && \
			$(HOST_EFI_CONFIGURE_VARS) \
			$(2) \
			$(HOST_CONFIGURE_CMD) \
			$(HOST_EFI_CONFIGURE_ARGS) \
			$(1); \
		fi \
	)
endef

define Host/Compile/bios
	+$(HOST_MAKE_VARS) \
	$(MAKE) $(HOST_JOBS) -C $(HOST_BUILD_DIR)/grub-bios \
		$(HOST_MAKE_FLAGS) \
		$(1)
endef

define Host/Compile/efi
	+$(HOST_MAKE_VARS) \
	$(MAKE) $(HOST_JOBS) -C $(HOST_BUILD_DIR)/grub-efi \
		$(HOST_MAKE_FLAGS) \
		$(1)
endef

define Host/Install/bios
	$(call Host/Compile/bios,install,$(HOST_BUILD_PREFIX))
endef

define Host/Install/efi
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/share/fonts/$(GRUB_FONT)/
	$(INSTALL_DATA) $(HOST_BUILD_DIR)/$(GRUB_FONT).bdf $(STAGING_DIR_HOST)/share/fonts/$(GRUB_FONT)/
	$(call Host/Compile/efi,install,$(HOST_BUILD_PREFIX))
endef

ifneq ($(CONFIG_GRUB_EFI),)
define Host/Configure
	$(SED) 's,(RANLIB),(TARGET_RANLIB),' $(HOST_BUILD_DIR)/grub-core/Makefile.in
	$(call Host/Configure/bios)
	$(call Host/Configure/efi)
endef
define Host/Compile
	$(call Host/Compile/bios)
	$(call Host/Compile/efi)
endef
define Host/Install
	$(call Host/Install/bios)
	$(call Host/Install/efi)
endef
else
define Host/Configure
	$(SED) 's,(RANLIB),(TARGET_RANLIB),' $(HOST_BUILD_DIR)/grub-core/Makefile.in
	$(call Host/Configure/bios)
endef
define Host/Compile
	$(call Host/Compile/bios)
endef
define Host/Install
	$(call Host/Install/bios)
endef
endif

define Package/grub2/Configure
	true
endef

define Package/grub2/Compile
	true
endef

define Package/grub2/Install
	true
endef

define Package/grub2-editenv/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/grub-pc/grub-editenv $(1)/usr/sbin/
endef

$(eval $(call Download/grub_font))
$(eval $(call HostBuild))
$(eval $(call BuildPackage,grub2))
$(eval $(call BuildPackage,grub2-editenv))
