#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Xeon
  NAME:=Xeon
endef

define Profile/Xeon/Description
	Baseline Xeon Profile
endef
$(eval $(call Profile,Xeon))
