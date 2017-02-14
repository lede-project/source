#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Atom64
  NAME:=Atom64
  PACKAGES:=
endef

define Profile/Atom64/Description
	Baseline Atom64 Profile
endef
$(eval $(call Profile,Atom64))
