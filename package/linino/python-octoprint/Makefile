#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=python-octoprint
PKG_VERSION:=1.2.13
PKG_RELEASE:=6

PKG_SOURCE:=$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/foosel/OctoPrint/archive/

PKG_BUILD_DIR:=$(BUILD_DIR)/OctoPrint-$(PKG_VERSION)

PKG_MD5SUM:=

include $(INCLUDE_DIR)/package.mk
$(call include_mk, python-package.mk)

define Package/python-octoprint
  SECTION:=language-python
  CATEGORY:=Languages
  SUBMENU:=Python
  TITLE:=python-octoprint
  URL:=http://sourceforge.net/projects/octoprint
  DEPENDS:=+python +python-setuptools +python-pyserial +python-psutil \
	+python-babel +python-backports +python-blinker \
	+python-yaml +python-argh +python-certifi +python-flask \
	+python-flask-assets +python-flask-babel +python-flask-login \
	+python-flask-principal +python-itsdangerous +python-jinja2 \
	+python-markupsafe +python-netaddr +python-netifaces \
	+python-pathtools +python-pkginfo +python-pyasn1 +python-pylru \
	+python-pytz +python-requests +python-rsa +python-sarge \
	+python-semantic_version +python-sockjs_tornado +python-speaklater \
	+python-tornado +python-tornado +python-watchdog +python-webassets \
	+python-werkzeug
endef

define Package/python-octoprint/description
	Python 3D printing module.
endef

define Build/Compile
	$(call Build/Compile/PyMod,,install --prefix=/usr --root=$(PKG_INSTALL_DIR))
endef

define Package/python-octoprint/install
	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)
	$(CP) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/* \
		$(1)$(PYTHON_PKG_DIR)

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/octoprint $(1)/usr/bin
endef

$(eval $(call BuildPackage,python-octoprint))
