
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=findutils
PKG_VERSION:=4.6.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@GNU/findutils/
PKG_MD5SUM:=9936aa8009438ce185bea2694a997fc1

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/findutils
  SECTION:=devel
  CATEGORY:=Development
  DEPENDS:=+USE_EGLIBC:librt
  TITLE:=findutils
  URL:=http://www.gnu.org/software/findutils/
endef

define Package/findutils/description
  The Findutils package contains programs that show the differences between
  files or directories.
endef

CONFIGURE_VARS += \
	ac_cv_func_mempcpy=n \
	gl_cv_func_wcwidth_works=yes \
    ac_cv_func_fnmatch_gnu=yes \

define Package/findutils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/{find,xargs,locate,updatedb} $(1)/usr/bin/
endef

define Package/findutils/preinst
#!/bin/sh
for x in find xargs locate updatedb; do
  [ -L "$${IPKG_INSTROOT}/usr/bin/$$x" ] && rm -f "$${IPKG_INSTROOT}/usr/bin/$$x"
done
exit 0
endef

define Package/findutils/postrm
#!/bin/sh
for x in find xargs locate updatedb; do
  /bin/busybox $$x -h 2>&1 | grep -q BusyBox && ln -sf ../../bin/busybox /usr/bin/$$x
done
exit 0
endef

$(eval $(call BuildPackage,findutils))
