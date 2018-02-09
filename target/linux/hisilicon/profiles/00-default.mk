#
# Copyright (C) 2017 Marty E. Plummer
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Default
  NAME:=Default Profile (all drivers)
  PACKAGES:= \
	kmod-rtc-hisi
  PRIORITY := 1
endef

define Profile/Default/Description
  Default profile with package set compatible with most boards.
endef
$(eval $(call Profile,Default))
