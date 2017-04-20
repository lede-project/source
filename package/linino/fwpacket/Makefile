#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lora-packet-forwarder
PKG_VERSION:=4.0.1
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/Lora-net/packet_forwarder.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=v4.0.1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_DEPENDS:=+lora-gateway

LORA_VERSION:=$(wildcard $(BUILD_DIR)/lora-gateway-5*)

# PKG_INSTALL:=1

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/lora-packet-forwarder/Default
	SECTION:=net
	CATEGORY:=Network
	URL:=http://www.semtech.com/wireless-rf/lora.html
	DEPENDS:=+libc +librt +lora-gateway
endef

define Package/lora-packet-forwarder-eu/conffiles
/etc/pkt_fwd/global_conf.json
/etc/pkt_fwd/local_conf.json
endef

define Package/lora-packet-forwarder-au/conffiles
/etc/pkt_fwd/global_conf.json
/etc/pkt_fwd/local_conf.json
endef

define Package/lora-packet-forwarder-us/conffiles
/etc/pkt_fwd/global_conf.json
/etc/pkt_fwd/local_conf.json
endef

# EU

define Package/lora-packet-forwarder-eu
$(call Package/lora-packet-forwarder/Default)
	SUBMENU:=LoRa
	TITLE:=Semtech lora-packet-forwarder package (EU)
endef

define Package/lora-packet-forwarder-eu/description
	A LoRa packet forwarder is a program running on the host of a LoRa gateway
	that forwards RF packets receive by the concentrator to a server through a
	IP/UDP link, and emits RF packets that are sent by the server (EU settings).
endef

# AU

define Package/lora-packet-forwarder-au
$(call Package/lora-packet-forwarder/Default)
	SUBMENU:=LoRa
	TITLE:=Semtech lora-packet-forwarder package (AU)
endef

define Package/lora-packet-forwarder-au/description
	A LoRa packet forwarder is a program running on the host of a LoRa gateway
	that forwards RF packets receive by the concentrator to a server through a
	IP/UDP link, and emits RF packets that are sent by the server (AU settings).
endef

# US

define Package/lora-packet-forwarder-us
$(call Package/lora-packet-forwarder/Default)
	SUBMENU:=LoRa
	TITLE:=Semtech lora-packet-forwarder package (US)
endef

define Package/lora-packet-forwarder-us/description
	A LoRa packet forwarder is a program running on the host of a LoRa gateway
	that forwards RF packets receive by the concentrator to a server through a
	IP/UDP link, and emits RF packets that are sent by the server (US settings).
endef

# build settings

TARGET_CFLAGS += $(FPIC) -O2 -Wall -Wextra -std=c99 -Iinc -I. -lm

define Build/Compile
	$(MAKE) CROSS_COMPILE=$(REAL_GNU_TARGET_NAME)- \
		LIBS="-lloragw -lrt -lpthread -lm" \
		CFLAGS+="-I$(PKG_BUILD_DIR)/lora_pkt_fwd/inc" \
		LGW_PATH=$(LORA_VERSION)/libloragw -C $(PKG_BUILD_DIR)/lora_pkt_fwd
	#
	$(MAKE) CROSS_COMPILE=$(REAL_GNU_TARGET_NAME)- \
		LIBS="-lloragw -lrt -lpthread -lm" \
		CFLAGS+="-I$(PKG_BUILD_DIR)/util_ack/inc" \
		LGW_PATH=$(LORA_VERSION)/libloragw -C $(PKG_BUILD_DIR)/util_ack
	#
	$(MAKE) CROSS_COMPILE=$(REAL_GNU_TARGET_NAME)- \
		LIBS="-lloragw -lrt -lpthread -lm" \
		CFLAGS+="-I$(PKG_BUILD_DIR)/util_sink/inc" \
		LGW_PATH=$(LORA_VERSION)/libloragw -C $(PKG_BUILD_DIR)/util_sink
	#
	$(MAKE) CROSS_COMPILE=$(REAL_GNU_TARGET_NAME)- \
		LIBS="-lloragw -lrt -lpthread -lm" \
		CFLAGS+="-I$(PKG_BUILD_DIR)/util_tx_test/inc" \
		LGW_PATH=$(LORA_VERSION)/libloragw -C $(PKG_BUILD_DIR)/util_tx_test
endef

# pkg install

define Package/lora-packet-forwarder-au/install
	$(INSTALL_DIR) $(1)/etc/pkt_fwd
	$(INSTALL_DATA) ./files/AU-global_conf.json $(1)/etc/pkt_fwd/global_conf.json
	$(INSTALL_DATA) ./files/local_conf.json $(1)/etc/pkt_fwd

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lora $(1)/etc/init.d

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lora_pkt_fwd/lora_pkt_fwd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_ack/util_ack $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_sink/util_sink $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_tx_test/util_tx_test $(1)/usr/bin
endef

define Package/lora-packet-forwarder-eu/install
	$(INSTALL_DIR) $(1)/etc/pkt_fwd
	$(INSTALL_DATA) ./files/EU-global_conf.json $(1)/etc/pkt_fwd/global_conf.json
	$(INSTALL_DATA) ./files/local_conf.json $(1)/etc/pkt_fwd

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lora $(1)/etc/init.d

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lora_pkt_fwd/lora_pkt_fwd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_ack/util_ack $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_sink/util_sink $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_tx_test/util_tx_test $(1)/usr/bin
endef

define Package/lora-packet-forwarder-us/install
	$(INSTALL_DIR) $(1)/etc/pkt_fwd
	$(INSTALL_DATA) ./files/US-global_conf.json $(1)/etc/pkt_fwd/global_conf.json
	$(INSTALL_DATA) ./files/local_conf.json $(1)/etc/pkt_fwd

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lora $(1)/etc/init.d

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lora_pkt_fwd/lora_pkt_fwd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_ack/util_ack $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_sink/util_sink $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_tx_test/util_tx_test $(1)/usr/bin
endef

# AU routines

define Package/lora-packet-forwarder-au/preinst
#!/bin/sh
# check if we are on real system
if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json" ]; then
	echo -e "\nBacking up GLOBAL settings files...\n"
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp
fi

if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json" ]; then
	echo -e "\nBacking up LOCAL settings files...\n"
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp
fi

exit 0
endef

define Package/lora-packet-forwarder-au/postinst
#!/bin/sh
# check if we are on real system
if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp" ]; then
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.new
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json
fi

if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp" ]; then
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.new
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json
fi

exit 0
endef

define Package/lora-packet-forwarder-au/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Removing rc.d symlink for lora service"
        /etc/init.d/lora disable
fi
exit 0
endef

# EU routines

define Package/lora-packet-forwarder-eu/preinst
#!/bin/sh
# check if we are on real system
if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json" ]; then
	echo -e "\nBacking up GLOBAL settings files...\n"
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp
fi

if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json" ]; then
	echo -e "\nBacking up LOCAL settings files...\n"
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp
fi

exit 0
endef

define Package/lora-packet-forwarder-eu/postinst
#!/bin/sh
# check if we are on real system
if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp" ]; then
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.new
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json
fi

if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp" ]; then
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.new
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json
fi

exit 0
endef

define Package/lora-packet-forwarder-eu/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Removing rc.d symlink for lora service"
        /etc/init.d/lora disable
fi
exit 0
endef

# US routines

define Package/lora-packet-forwarder-us/preinst
#!/bin/sh
# check if we are on real system
if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json" ]; then
	echo -e "\nBacking up GLOBAL settings files...\n"
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp
fi

if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json" ]; then
	echo -e "\nBacking up LOCAL settings files...\n"
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp
fi

exit 0
endef

define Package/lora-packet-forwarder-us/postinst
#!/bin/sh
# check if we are on real system
if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp" ]; then
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.new
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json.tmp $${IPKG_INSTROOT}/etc/pkt_fwd/global_conf.json
fi

if [ -f "$${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp" ]; then
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.new
	mv $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json.tmp $${IPKG_INSTROOT}/etc/pkt_fwd/local_conf.json
fi

exit 0
endef

define Package/lora-packet-forwarder-us/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Removing rc.d symlink for lora service"
        /etc/init.d/lora disable
fi
exit 0
endef

$(eval $(call BuildPackage,lora-packet-forwarder-au))
$(eval $(call BuildPackage,lora-packet-forwarder-eu))
$(eval $(call BuildPackage,lora-packet-forwarder-us))
