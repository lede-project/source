#
# Copyright (C) 2006-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=iptables
PKG_VERSION:=1.6.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:= http://www.lt.netfilter.org/projects/iptables/files/ \
	http://www.hu.netfilter.org/projects/iptables/files/ \
	http://www.netfilter.org/projects/iptables/files/
PKG_HASH:=4bb72a0a0b18b5a9e79e87631ddc4084528e5df236bc7624472dcaa8480f1c60

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0

PKG_INSTALL:=1
PKG_CONFIG_DEPENDS:= \
	CONFIG_IPTABLES_IPV4 \
	CONFIG_IPTABLES_IPV6 \
	CONFIG_IPTABLES_ARPTABLES \
	CONFIG_IPTABLES_EBTABLES

include $(INCLUDE_DIR)/package.mk

define Package/iptables/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Firewall
  URL:=http://netfilter.org/
endef

define Package/iptables
$(call Package/iptables/Default)
  TITLE:=IP firewall administration tool
  MENU:=1
  DEPENDS:= \
	+libxtables \
	+IPTABLES_IPV4:libip4tc \
	+kmod-ipt-core \
	+IPTABLES_IPV6:libip6tc \
	+IPTABLES_IPV6:kmod-ip6tables
  PROVIDES:= ip6tables
endef

define Package/iptables/config
source "$(SOURCE)/Config.in"
endef

define Package/iptables/description
IP firewall administration tool.

 Matches:
  - icmp
  - tcp
  - udp
  - comment
  - conntrack
  - limit
  - mac
  - mark
  - multiport
  - set
  - state
  - time

 Targets:
  - ACCEPT
  - CT
  - DNAT
  - DROP
  - REJECT
  - LOG
  - MARK
  - MASQUERADE
  - REDIRECT
  - SET
  - SNAT
  - TCPMSS

 Tables:
  - filter
  - mangle
  - nat
  - raw

endef

define Package/iptables-mod-xtables
  $(call Package/iptables/Default)
  TITLE:= Extra xtables extensions
  DEPENDS:= +libxtables
  PROVIDES:= \
	iptables-mod-checksum \
	iptables-mod-cluster \
	iptables-mod-conntrack-extra \
	iptables-mod-extra \
	iptables-mod-filter \
	iptables-mod-hashlimit \
	iptables-mod-iprange \
	iptables-mod-led \
	iptables-mod-nflog \
	iptables-mod-nfqueue \
	iptables-mod-tee \
	iptables-mod-tproxy \
	iptables-mod-u32
endef

define Package/iptables-mod-ipv4
  $(call Package/iptables/Default)
  TITLE:= IPv4 iptables extensions
  DEPENDS:= +libxtables +@IPTABLES_IPV4
  PROVIDES:= \
	iptables-mod-clusterip \
	iptables-mod-ipopt \
	iptables-mod-ipsec \
	iptables-mod-nat-extra \
	iptables-mod-ulog
endef

define Package/iptables-mod-ipv4/description
  TODO
endef

define Package/iptables-mod-ipv6
  $(call Package/iptables/Default)
  TITLE:= IPv6 iptables extensions
  DEPENDS:= +libxtables +@IPTABLES_IPV6
  PROVIDES:= \
	ip6tables-extra \
	ip6tables-mod-nat
endef

define Package/iptables-mod-ipv6/description
  TODO
endef

define Package/libiptc
  $(call Package/iptables/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libip4tc +libip6tc +libxtables
  TITLE:=IPv4/IPv6 firewall - shared libiptc library (compatibility stub)
endef

define Package/libip4tc
  $(call Package/iptables/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=IPv4 firewall - shared libiptc library
  DEPENDS:=+libxtables
endef

define Package/libip6tc
  $(call Package/iptables/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=IPv6 firewall - shared libiptc library
  DEPENDS:=+libxtables
endef

define Package/libxtables
  $(call Package/iptables/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=IPv4/IPv6 firewall - shared xtables library
  DEPENDS:= +libnfnetlink +libnetfilter-conntrack
endef

IPTABLES_XTLIBDIR:=/usr/lib/iptables

define Iptables/InstallExtensions
	$(INSTALL_DIR) $(1)/$(IPTABLES_XTLIBDIR)
	for m in $(2) ; do \
		$(CP) $(PKG_INSTALL_DIR)/$(IPTABLES_XTLIBDIR)/lib$$$${m}.so $(1)/$(IPTABLES_XTLIBDIR)/ ; \
	done
endef

TARGET_CFLAGS += \
	-ffunction-sections -fdata-sections

TARGET_LDFLAGS += \
	-Wl,--gc-sections

CONFIGURE_ARGS += \
	--enable-shared \
	--disable-static \
	--enable-devel \
	--disable-nftables \
	--prefix=/usr \
	--with-xtlibdir=$(IPTABLES_XTLIBDIR) \
	$(if $(CONFIG_IPTABLES_IPV4),--enable-ipv4,--disable-ipv4) \
	$(if $(CONFIG_IPTABLES_IPV6),--enable-ipv6,--disable-ipv6)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include

	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libxtables.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libip*tc.so* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/xtables.pc $(1)/usr/lib/pkgconfig/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libip*tc.pc $(1)/usr/lib/pkgconfig/
	

endef

define Package/iptables/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/xtables-multi $(1)/usr/sbin/
	$(call Iptables/InstallExtensions,$(1), \
		xt_CT xt_MARK xt_SET xt_TCPMSS xt_comment xt_conntrack \
		xt_limit xt_mac xt_mark xt_multiport xt_set xt_standard \
		xt_state xt_tcp xt_time xt_udp)

  ifdef CONFIG_IPTABLES_IPV4
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/iptables{,-restore,-save} $(1)/usr/sbin/
	$(call Iptables/InstallExtensions,$(1), \
		ipt_LOG ipt_REJECT ipt_icmp \
		ipt_DNAT ipt_MASQUERADE ipt_REDIRECT ipt_SNAT)
  endif

  ifdef CONFIG_IPTABLES_IPV6
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/ip6tables{,-restore,-save} $(1)/usr/sbin/
	$(call Iptables/InstallExtensions,$(1), \
		ip6t_LOG ip6t_REJECT ip6t_icmp6)
  endif

  ifdef CONFIG_IPTABLES_ARPTABLES
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/arptables-compat $(1)/usr/sbin/
	$(call Iptables/InstallExtensions,$(1), \
		arpt_mangle)
  endif

  ifdef CONFIG_IPTABLES_EBTABLE
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/ebtables-compat $(1)/usr/sbin/
	$(call Iptables/InstallExtensions,$(1), \
		ebt_802_3 ebt_ip ebt_limit ebt_mark ebt_mark_m ebt_log ebt_nflog)
  endif
endef

define Package/iptables-mod-xtables/install
	$(call Iptables/InstallExtensions,$(1), \
		xt_AUDIT xt_CHECKSUM xt_CLASSIFY xt_CONNMARK xt_CONNSECMARK \
		xt_DSCP xt_HMARK xt_IDLETIMER xt_LED xt_NFLOG xt_NFQUEUE \
		xt_NOTRACK xt_RATEEST xt_SECMARK xt_SYNPROXY xt_TCPOPTSTRIP \
		xt_TEE xt_TOS xt_TPROXY xt_TRACE xt_addrtype xt_bpf xt_cgroup \
		xt_cluster xt_connbytes xt_connlabel xt_connlimit xt_connmark \
		xt_cpu xt_dccp xt_devgroup xt_dscp xt_ecn xt_esp xt_hashlimit \
		xt_helper xt_ipcomp xt_iprange xt_ipvs xt_length xt_mangle \
		xt_nfacct xt_osf xt_owner xt_physdev xt_pkttype xt_policy \
		xt_quota xt_rateest xt_recent xt_rpfilter xt_sctp xt_socket \
		xt_statistic xt_string xt_tcpmss xt_tos xt_u32)
endef

define Package/iptables-mod-ipv4/install
	$(call Iptables/InstallExtensions,$(1), \
		ipt_CLUSTERIP ipt_ECN ipt_NETMAP ipt_TTL ipt_ULOG \
		ipt_ah ipt_realm ipt_ttl)
endef

define Package/iptables-mod-ipv6/install
	$(call Iptables/InstallExtensions,$(1), \
		ip6t_DNAT ip6t_MASQUERADE ip6t_REDIRECT ip6t_SNAT DNPT ip6t_HL \
		ip6t_ip6t_NETMAP ip6t_SNPT ip6t_ah ip6t_dst ip6t_eui64 frag ip6t_hbh \
		ip6t_ip6t_hl ip6t_ipv6header ip6t_mh ip6t_rt)
endef

define Package/libiptc/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libiptc.so* $(1)/usr/lib/
endef

define Package/libip4tc/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libip4tc.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/extensions/libiptext4.so $(1)/usr/lib/
endef

define Package/libip6tc/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libip6tc.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/extensions/libiptext6.so $(1)/usr/lib/
endef

define Package/libxtables/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libxtables.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/extensions/libiptext.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,iptables))
$(eval $(call BuildPackage,iptables-mod-xtables))
$(eval $(call BuildPackage,iptables-mod-ipv4))
$(eval $(call BuildPackage,iptables-mod-ipv6))
$(eval $(call BuildPackage,libiptc))
$(eval $(call BuildPackage,libip4tc))
$(eval $(call BuildPackage,libip6tc))
$(eval $(call BuildPackage,libxtables))
