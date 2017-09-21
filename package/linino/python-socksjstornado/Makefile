#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sockjs-tornado
PKG_VERSION:=1.0.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://pypi.python.org/packages/source/s/sockjs-tornado/
#PKG_BUILD_DIR:=$(BUILD_DIR)/SleekXMPP-$(PKG_NAME)-$(PKG_VERSION)
PKG_MD5SUM:=

# PKG_BUILD_DEPENDS:=+python-expat +python-openssl +libopenssl

# TARGET_LDFLAGS += -L${STAGING_DIR}/lib \
		-L${STAGING_DIR}/usr/lib/python2.7

include $(INCLUDE_DIR)/package.mk
$(call include_mk, python-package.mk)

define Package/python-sockjs_tornado
  SECTION:=language-python
  CATEGORY:=Languages
  SUBMENU:=Python
  TITLE:=python-sockjs_tornado
  URL:=http://sourceforge.net/projects/sockjs-tornado
  DEPENDS:=+python
endef

define Package/python-sockjs_tornado/description
Python module interface to the cURL library.
endef

define Build/Compile
	$(call Build/Compile/PyMod,,install --prefix=/usr --root=$(PKG_INSTALL_DIR))
endef

define Package/python-sockjs_tornado/install
	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)
	$(CP) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/* \
		$(1)$(PYTHON_PKG_DIR)
endef

$(eval $(call BuildPackage,python-sockjs_tornado))
