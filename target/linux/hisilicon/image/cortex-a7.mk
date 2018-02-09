#
# Copyright (C) 2017 Marty E. Plummer
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

ifeq ($(SUBTARGET),cortexa7)

define Device/sdr-b74301n
  DEVICE_TITLE:=Samsungsv SDR-B74301N DVR
  DEVICE_PACKAGES:=kmod-rtc-hisi
  SUPPORTED_DEVICES:=sdr-b74301n
  DEVICE_DTS:=hi3521a-rs-dm290e
endef

TARGET_DEVICES += sdr-b74301n

endif
