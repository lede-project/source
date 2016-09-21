#
# Copyright (C) 2016 Jiang Yutang <yutang.jiang@nxp.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/Default
	NAME:=ls1 32bit Default Profile
endef

define Profile/Default/Description
	ls1 32bit Default Profile Description
endef

$(eval $(call Profile,Default))
