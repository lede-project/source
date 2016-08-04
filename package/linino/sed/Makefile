
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sed
PKG_VERSION:=4.2.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@GNU/sed
# PKG_MD5SUM:=b7e35aa175778c84942b1fee4144988b

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/sed
  SECTION:=devel
  CATEGORY:=Development
  DEPENDS:=+USE_EGLIBC:librt
  TITLE:=sed
  URL:=http://www.gnu.org/software/sed/
endef

define Package/sed/description
	The GNU sed utility
endef

#CONFIGURE_VARS += \
#	ac_cv_func_mempcpy=n \
#	gl_cv_func_wcwidth_works=yes \
#    ac_cv_func_fnmatch_gnu=yes \

define Package/sed/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/sed $(1)/usr/bin/
endef

define Package/sed/preinst
#!/bin/sh
for x in sed; do
  [ -L "$${IPKG_INSTROOT}/usr/bin/$$x" ] && rm -f "$${IPKG_INSTROOT}/usr/bin/$$x"
done
exit 0
endef

define Package/sed/postrm
#!/bin/sh
for x in sed; do
  /bin/busybox $$x -h 2>&1 | grep -q BusyBox && ln -sf ../../bin/busybox /usr/bin/$$x
done
exit 0
endef

$(eval $(call BuildPackage,sed))
