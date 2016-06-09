#
# Copyright (C) 2014 Qualcomm-Atheros Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sparse
PKG_VERSION:=0.5.0

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://git.kernel.org/pub/scm/devel/sparse/sparse.git
PKG_SOURCE_VERSION:=40791b94c56b1a6da2a0ddeb1f9d5c9d64de8f93
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.xz
#PKG_SOURCE_URL:=@KERNEL/software/devel/sparse/dist/
#PKG_MD5SUM:=68bc834c57836251fbee55a7707bab39

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

define Host/Install
       $(INSTALL_BIN) $(HOST_BUILD_DIR)/sparse $(STAGING_DIR_HOST)/bin
endef

$(eval $(call HostBuild))
