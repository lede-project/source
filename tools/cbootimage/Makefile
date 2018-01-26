#
# Copyright (c) 2017 Tomasz Maciej Nowak <tomek_n@o2.pl>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME := cbootimage

PKG_SOURCE_DATE := 2017-10-03
PKG_SOURCE_PROTO := git
PKG_SOURCE_URL := https://github.com/NVIDIA/cbootimage.git
PKG_SOURCE_VERSION := 3b3c3cccf0640326229935be5ff702ac948fd51b

HOST_BUILD_PARALLEL := 1

include $(INCLUDE_DIR)/host-build.mk

define Host/Configure
	(cd $(HOST_BUILD_DIR); autoreconf --install --symlink)
	$(call Host/Configure/Default)
endef

$(eval $(call HostBuild))
