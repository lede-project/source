#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/fw8756
  NAME:=fw8756
  PACKAGES:=kmod-w83627-wdt kmod-hwmon-nct6775 lcdproc
endef

define Profile/fw8756/Description
	Baseline Lanner fw-8756 Profile
endef
$(eval $(call Profile,fw8756))
