#
# Copyright (C) 2009 OpenWrt.org
#

SUBTARGET:=mt7620
BOARDNAME:=MT7620 based boards
FEATURES+=usb
CPU_TYPE:=24kec

DEFAULT_PACKAGES += kmod-rt2800-pci kmod-rt2800-soc kmod-mt76

define Target/Description
	Build firmware images for Ralink MT7620 based boards.
endef

