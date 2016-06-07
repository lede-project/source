include $(TOPDIR)/rules.mk

PKG_NAME:=qos-guoguo
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/qos-guoguo
	SECTION:=net
	CATEGORY:=Network
	TITLE:=A set of QoS scripts based on qos-gargoyle for Gargoyle Firmware
	DEPENDS:=+tc +ip-full +kmod-sched +iptables-mod-filter +iptables-mod-ipopt +iptables-mod-imq
	MAINTAINER:=GuoGuo <gch981213@gmail.com>
endef

define Package/qos-guoguo/description
	A set of QoS scripts based on qos-gargoyle for Gargoyle Firmware
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Build/Configure
endef

define Build/Compile
	-$(MAKE) -C $(PKG_BUILD_DIR) clean
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) -I $(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L $(STAGING_DIR)/usr/lib" \
		BUILD_DIR="$(BUILD_DIR)" \
		all


endef

define Package/qos-guoguo/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DIR) $(1)/etc/config
	
	$(INSTALL_BIN) ./files/qos_guoguo.init $(1)/etc/init.d/qos_guoguo
	$(INSTALL_BIN) ./files/qos_guoguo.hotplug $(1)/etc/hotplug.d/iface/23-qos_guoguo
	$(INSTALL_DATA) ./files/qos_guoguo.conf $(1)/etc/config/qos_guoguo

	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qosmon $(1)/usr/sbin/qosmon

endef

define Package/qos-guoguo/prerm
	$${IPKG_INSTROOT}/etc/init.d/qos_guoguo stop 2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/qos_guoguo disable 2>/dev/null
	ls >/dev/null 2>&1
endef

$(eval $(call BuildPackage,qos-guoguo))
