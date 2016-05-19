define Profile/BTHOMEHUBV2B
  NAME:=BT Home Hub 2B
  PACKAGES:=kmod-ltq-hcd-danube kmod-ledtrig-usbdev \
	kmod-ltq-adsl-danube-mei kmod-ltq-adsl-danube \
	kmod-ltq-adsl-danube-fw-a kmod-ltq-atm-danube \
	kmod-ltq-deu-danube \
	ltq-adsl-app ppp-mod-pppoa \
	kmod-ath9k wpad-mini \
	swconfig
endef

$(eval $(call Profile,BTHOMEHUBV2B))

define Profile/BTHOMEHUBV3A
  NAME:=BT Home Hub 3A
  PACKAGES:=kmod-usb-dwc2 kmod-ledtrig-usbdev \
	kmod-ltq-adsl-ar9-mei kmod-ltq-adsl-ar9 \
	kmod-ltq-adsl-ar9-fw-a kmod-ltq-atm-ar9 \
	kmod-ltq-deu-ar9 \
	ltq-adsl-app ppp-mod-pppoa \
	kmod-ath9k wpad-mini \
	swconfig uboot-envtools
endef

$(eval $(call Profile,BTHOMEHUBV3A))
