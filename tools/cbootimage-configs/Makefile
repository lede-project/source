#
# Copyright (c) 2017 Tomasz Maciej Nowak <tomek_n@o2.pl>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME := cbootimage-configs

PKG_SOURCE_DATE := 2017-04-13
PKG_SOURCE_PROTO := git
PKG_SOURCE_URL := https://github.com/NVIDIA/cbootimage-configs.git
PKG_SOURCE_VERSION := 7c3b458b93ed6947cd083623f543e93f9103cc0f

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
endef

define Host/Install
	mkdir -p $(STAGING_DIR_HOST)/share/$(PKG_NAME)
	$(CP) $(HOST_BUILD_DIR)/* \
		$(STAGING_DIR_HOST)/share/$(PKG_NAME)
endef

$(eval $(call HostBuild))
