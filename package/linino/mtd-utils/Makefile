#
# Copyright (C) 2009-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mtd-utils
PKG_VERSION:=1.5.2
PKG_RELEASE:=3
PKG_MD5SUM:=596bc7b20a6d4fb86d63fc9b8af674d6

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://download.linino.org/sources/

# PKG_REV:=aea36417067dade75192bafa03af70b6eb2677b1
# PKG_SOURCE_URL:=git://git.infradead.org/mtd-utils.git
# PKG_SOURCE_PROTO:=git
# PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
# PKG_SOURCE_VERSION:=$(PKG_REV)

PKG_INSTALL:=1

PKG_BUILD_DEPENDS:=util-linux

include $(INCLUDE_DIR)/package.mk

FILES:= \
	doc_loadbios \
	docfdisk \
	flash_erase \
	flash_eraseall:+mtd-utils-flash-erase \
	flash_lock \
	flash_otp_dump \
	flash_otp_lock \
	flash_otp_write \
	flash_unlock \
	flashcp \
	ftl_check \
	ftl_format \
	jffs2dump \
	jffs2reader:+zlib \
	mkfs.jffs2:+zlib \
	mkfs.ubifs:+libuuid:+liblzo:+zlib \
	mtd_debug \
	mtdinfo \
	nanddump \
	nandtest \
	nandwrite \
	nftldump \
	nftl_format \
	recv_image \
	rfddump \
	rfdformat \
	serve_image \
	sumtool \
	ubiattach \
	ubicrc32 \
	ubidetach \
	ubiformat \
	ubimkvol \
	ubinfo \
	ubinize \
	ubirename \
	ubirmvol \
	ubirsvol \
	ubiupdatevol

define PartGen
define Package/mtd-utils-$(subst _,-,$(firstword $(subst :, ,$(1))))
  TITLE:=$(firstword $(subst :, ,$(1))) package from mtd-utils
  URL:=http://www.linux-mtd.infradead.org/
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libpthread mtd-utils $(wordlist 2,$(words $(subst :, ,$(1))),$(subst :, ,$(1)))
endef
endef

define Package/mtd-utils
  TITLE:=Utilities for flash info/debug
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://www.linux-mtd.infradead.org/
  MENU:=1
endef

define Package/mtd-utils/description
  Utilities for manipulating memory technology devices.
endef

define Package/mtd-utils/install
	true
endef

$(foreach file,$(FILES),$(eval $(call PartGen,$(file))))

MAKE_FLAGS += \
	DESTDIR="$(PKG_INSTALL_DIR)" \
	BUILDDIR="$(PKG_BUILD_DIR)" \
	LDLIBS+="$(LIBGCC_S)" \
	WITHOUT_XATTR=1 \
	WITHOUT_LZO=1 \
	all tests

TARGET_LDFLAGS += -lpthread

define PartInstall
define Package/mtd-utils-$(subst _,-,$(firstword $(subst :, ,$(1))))/install
	$(INSTALL_DIR) $$(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/$(firstword $(subst :, ,$(1))) \
		$$(1)/usr/sbin/
endef
endef


define Package/mtd-utils-tests
	TITLE:=Tests packages from mtd-utils
	URL:=http://www.linux-mtd.infradead.org/
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:=+libpthread mtd-utils $(foreach file,$(FILES),$(addprefix mtd-utils-,$(subst _,-,$(firstword $(subst :, ,$(file))))))
endef

define Package/mtd-utils-tests/install
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/integ $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/io_basic $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/io_paral $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/io_read $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/io_update $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mkvol_bad $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mkvol_basic $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mkvol_paral $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rsvol $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/volrefcnt $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/ubi-tests/runtests.sh $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/ubi-tests/stress-test.sh $(1)/usr/share/mtd-tests/ubi-tests/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/fs-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/help_all.sh $(1)/usr/share/mtd-tests/fs-tests/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/run_all.sh $(1)/usr/share/mtd-tests/fs-tests/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/fs-tests/integrity
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/integrity/integck $(1)/usr/share/mtd-tests/fs-tests/integrity/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/fs-tests/simple
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/simple/ftrunc $(1)/usr/share/mtd-tests/fs-tests/simple/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/simple/orph $(1)/usr/share/mtd-tests/fs-tests/simple/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/simple/perf $(1)/usr/share/mtd-tests/fs-tests/simple/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/simple/test_1 $(1)/usr/share/mtd-tests/fs-tests/simple/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/simple/test_2 $(1)/usr/share/mtd-tests/fs-tests/simple/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/fs-tests/stress
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/stress00.sh $(1)/usr/share/mtd-tests/fs-tests/stress/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/stress01.sh $(1)/usr/share/mtd-tests/fs-tests/stress/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/fs-tests/stress/atoms
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/fwrite00 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/gcd_hupper $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/pdfrun $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/rmdir00 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/rndrm00 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/rndrm99 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/rndwrite00 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/stress_1 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/stress_2 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/stress/atoms/stress_3 $(1)/usr/share/mtd-tests/fs-tests/stress/atoms/
	$(INSTALL_DIR) $(1)/usr/share/mtd-tests/fs-tests/utils
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/utils/free_space $(1)/usr/share/mtd-tests/fs-tests/utils/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/fs-tests/utils/fstest_monitor $(1)/usr/share/mtd-tests/fs-tests/utils/
endef


$(foreach file,$(FILES),$(eval $(call PartInstall,$(file))))

$(eval $(call BuildPackage,mtd-utils))
$(foreach file,$(FILES),$(eval $(call BuildPackage,mtd-utils-$(subst _,-,$(firstword $(subst :, ,$(file)))))))
$(eval $(call BuildPackage,mtd-utils-tests))
