#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=netaddr
PKG_VERSION:=0.7.17
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://pypi.python.org/packages/source/n/netaddr/
#PKG_BUILD_DIR:=$(BUILD_DIR)/SleekXMPP-$(PKG_NAME)-$(PKG_VERSION)
PKG_MD5SUM:=

# PKG_BUILD_DEPENDS:=+python-expat +python-openssl +libopenssl

# TARGET_LDFLAGS += -L${STAGING_DIR}/lib \
		-L${STAGING_DIR}/usr/lib/python2.7

include $(INCLUDE_DIR)/package.mk
$(call include_mk, python-package.mk)

define Package/python-netaddr
  SECTION:=language-python
  CATEGORY:=Languages
  SUBMENU:=Python
  TITLE:=python-netaddr
  URL:=http://sourceforge.net/projects/netaddr
  DEPENDS:=+python
endef

define Package/python-netaddr/description
Python module interface to the cURL library.
endef

define Build/Compile
	$(call Build/Compile/PyMod,,install --prefix=/usr --root=$(PKG_INSTALL_DIR))
endef

define Package/python-netaddr/install
	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)
	$(CP) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/* \
		$(1)$(PYTHON_PKG_DIR)
endef

$(eval $(call BuildPackage,python-netaddr))
