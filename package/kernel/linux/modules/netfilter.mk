
#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

NF_MENU:=Netfilter Extensions
NF_KMOD:=1
include $(INCLUDE_DIR)/netfilter.mk


define KernelPackage/nf-ipt
  SUBMENU:=$(NF_MENU)
  TITLE:=Iptables core
  KCONFIG:= \
  	CONFIG_NETFILTER=y \
	CONFIG_NETFILTER_ADVANCED=y \
	$(KCONFIG_NF_IPT)
  FILES:=$(foreach mod,$(NF_IPT-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_IPT-m)))
endef

$(eval $(call KernelPackage,nf-ipt))


define KernelPackage/nf-ipt6
  SUBMENU:=$(NF_MENU)
  TITLE:=Ip6tables core
  KCONFIG:=$(KCONFIG_NF_IPT6)
  FILES:=$(foreach mod,$(NF_IPT6-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_IPT6-m)))
  DEPENDS:=+kmod-nf-ipt +kmod-nf-conntrack6
endef

$(eval $(call KernelPackage,nf-ipt6))



define KernelPackage/ipt-core
  SUBMENU:=$(NF_MENU)
  TITLE:=Iptables core
  KCONFIG:=$(KCONFIG_IPT_CORE)
  FILES:=$(foreach mod,$(IPT_CORE-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_CORE-m)))
  DEPENDS:=+kmod-nf-ipt
endef

define KernelPackage/ipt-core/description
 Netfilter core kernel modules
 Includes:
 - comment
 - limit
 - LOG
 - mac
 - multiport
 - REJECT
 - TCPMSS
endef

$(eval $(call KernelPackage,ipt-core))


define KernelPackage/nf-conntrack
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter connection tracking
  KCONFIG:= \
        CONFIG_NETFILTER=y \
        CONFIG_NETFILTER_ADVANCED=y \
        CONFIG_NF_CONNTRACK_ZONES=y \
	$(KCONFIG_NF_CONNTRACK)
  FILES:=$(foreach mod,$(NF_CONNTRACK-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_CONNTRACK-m)))
endef

$(eval $(call KernelPackage,nf-conntrack))


define KernelPackage/nf-conntrack6
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter IPv6 connection tracking
  KCONFIG:=$(KCONFIG_NF_CONNTRACK6)
  DEPENDS:=@IPV6 +kmod-nf-conntrack
  FILES:=$(foreach mod,$(NF_CONNTRACK6-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_CONNTRACK6-m)))
endef

$(eval $(call KernelPackage,nf-conntrack6))


define KernelPackage/nf-nat
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter NAT
  KCONFIG:=$(KCONFIG_NF_NAT)
  DEPENDS:=+kmod-nf-conntrack +kmod-nf-ipt
  FILES:=$(foreach mod,$(NF_NAT-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_NAT-m)))
endef

$(eval $(call KernelPackage,nf-nat))


define KernelPackage/nf-nat6
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter IPV6-NAT
  KCONFIG:=$(KCONFIG_NF_NAT6)
  DEPENDS:=+kmod-nf-conntrack6 +kmod-nf-ipt6 +kmod-nf-nat
  FILES:=$(foreach mod,$(NF_NAT6-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_NAT6-m)))
endef

$(eval $(call KernelPackage,nf-nat6))


define AddDepends/ipt
  SUBMENU:=$(NF_MENU)
  DEPENDS+= +kmod-ipt-core $(1)
endef


define KernelPackage/ipt-conntrack
  TITLE:=Basic connection tracking modules
  KCONFIG:=$(KCONFIG_IPT_CONNTRACK)
  FILES:=$(foreach mod,$(IPT_CONNTRACK-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_CONNTRACK-m)))
  $(call AddDepends/ipt,+kmod-nf-conntrack)
endef

define KernelPackage/ipt-conntrack/description
 Netfilter (IPv4) kernel modules for connection tracking
 Includes:
 - conntrack
 - defrag
 - iptables_raw
 - NOTRACK
 - state
endef

$(eval $(call KernelPackage,ipt-conntrack))


define KernelPackage/ipt-conntrack-extra
  TITLE:=Extra connection tracking modules
  KCONFIG:=$(KCONFIG_IPT_CONNTRACK_EXTRA)
  FILES:=$(foreach mod,$(IPT_CONNTRACK_EXTRA-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_CONNTRACK_EXTRA-m)))
  $(call AddDepends/ipt,+kmod-ipt-conntrack)
endef

define KernelPackage/ipt-conntrack-extra/description
 Netfilter (IPv4) extra kernel modules for connection tracking
 Includes:
 - connbytes
 - connmark/CONNMARK
 - conntrack
 - helper
 - recent
endef

$(eval $(call KernelPackage,ipt-conntrack-extra))


define KernelPackage/ipt-filter
  TITLE:=Modules for packet content inspection
  KCONFIG:=$(KCONFIG_IPT_FILTER)
  FILES:=$(foreach mod,$(IPT_FILTER-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_FILTER-m)))
  $(call AddDepends/ipt,+kmod-lib-textsearch +kmod-ipt-conntrack)
endef

define KernelPackage/ipt-filter/description
 Netfilter (IPv4) kernel modules for packet content inspection
 Includes:
 - string
endef

$(eval $(call KernelPackage,ipt-filter))


define KernelPackage/ipt-ipopt
  TITLE:=Modules for matching/changing IP packet options
  KCONFIG:=$(KCONFIG_IPT_IPOPT)
  FILES:=$(foreach mod,$(IPT_IPOPT-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_IPOPT-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-ipopt/description
 Netfilter (IPv4) modules for matching/changing IP packet options
 Includes:
 - CLASSIFY
 - dscp/DSCP
 - ecn/ECN
 - hl/HL
 - length
 - mark/MARK
 - statistic
 - tcpmss
 - time
 - ttl/TTL
 - unclean
endef

$(eval $(call KernelPackage,ipt-ipopt))


define KernelPackage/ipt-ipsec
  TITLE:=Modules for matching IPSec packets
  KCONFIG:=$(KCONFIG_IPT_IPSEC)
  FILES:=$(foreach mod,$(IPT_IPSEC-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_IPSEC-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-ipsec/description
 Netfilter (IPv4) modules for matching IPSec packets
 Includes:
 - ah
 - esp
 - policy
endef

$(eval $(call KernelPackage,ipt-ipsec))

IPSET_MODULES:= \
	ipset/ip_set \
	ipset/ip_set_bitmap_ip \
	ipset/ip_set_bitmap_ipmac \
	ipset/ip_set_bitmap_port \
	ipset/ip_set_hash_ip \
	ipset/ip_set_hash_ipmark \
	ipset/ip_set_hash_ipport \
	ipset/ip_set_hash_ipportip \
	ipset/ip_set_hash_ipportnet \
	ipset/ip_set_hash_mac \
	ipset/ip_set_hash_netportnet \
	ipset/ip_set_hash_net \
	ipset/ip_set_hash_netnet \
	ipset/ip_set_hash_netport \
	ipset/ip_set_hash_netiface \
	ipset/ip_set_list_set \
	xt_set

define KernelPackage/ipt-ipset
  SUBMENU:=Netfilter Extensions
  TITLE:=IPset netfilter modules
  DEPENDS+= +kmod-ipt-core +kmod-nfnetlink
  KCONFIG:= \
	CONFIG_IP_SET \
	CONFIG_IP_SET_MAX=256 \
	CONFIG_NETFILTER_XT_SET \
	CONFIG_IP_SET_BITMAP_IP \
	CONFIG_IP_SET_BITMAP_IPMAC \
	CONFIG_IP_SET_BITMAP_PORT \
	CONFIG_IP_SET_HASH_IP \
	CONFIG_IP_SET_HASH_IPMARK \
	CONFIG_IP_SET_HASH_IPPORT \
	CONFIG_IP_SET_HASH_IPPORTIP \
	CONFIG_IP_SET_HASH_IPPORTNET \
	CONFIG_IP_SET_HASH_MAC \
	CONFIG_IP_SET_HASH_NET \
	CONFIG_IP_SET_HASH_NETNET \
	CONFIG_IP_SET_HASH_NETIFACE \
	CONFIG_IP_SET_HASH_NETPORT \
	CONFIG_IP_SET_HASH_NETPORTNET \
	CONFIG_IP_SET_LIST_SET \
	CONFIG_NET_EMATCH_IPSET=n
  FILES:=$(foreach mod,$(IPSET_MODULES),$(LINUX_DIR)/net/netfilter/$(mod).ko)
  AUTOLOAD:=$(call AutoLoad,49,$(notdir $(IPSET_MODULES)))
endef
$(eval $(call KernelPackage,ipt-ipset))


define KernelPackage/ipt-nat
  TITLE:=Basic NAT targets
  KCONFIG:=$(KCONFIG_IPT_NAT)
  FILES:=$(foreach mod,$(IPT_NAT-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_NAT-m)))
  $(call AddDepends/ipt,+kmod-nf-nat)
endef

define KernelPackage/ipt-nat/description
 Netfilter (IPv4) kernel modules for basic NAT targets
 Includes:
 - MASQUERADE
endef

$(eval $(call KernelPackage,ipt-nat))


define KernelPackage/ipt-nat6
  TITLE:=IPv6 NAT targets
  KCONFIG:=$(KCONFIG_IPT_NAT6)
  FILES:=$(foreach mod,$(IPT_NAT6-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoLoad,43,$(notdir $(IPT_NAT6-m)))
  $(call AddDepends/ipt,+kmod-nf-nat6)
  $(call AddDepends/ipt,+kmod-ipt-conntrack)
  $(call AddDepends/ipt,+kmod-ipt-nat)
  $(call AddDepends/ipt,+kmod-ip6tables)
endef

define KernelPackage/ipt-nat6/description
 Netfilter (IPv6) kernel modules for NAT targets
endef

$(eval $(call KernelPackage,ipt-nat6))


define KernelPackage/ipt-nat-extra
  TITLE:=Extra NAT targets
  KCONFIG:=$(KCONFIG_IPT_NAT_EXTRA)
  FILES:=$(foreach mod,$(IPT_NAT_EXTRA-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_NAT_EXTRA-m)))
  $(call AddDepends/ipt,+kmod-ipt-nat)
endef

define KernelPackage/ipt-nat-extra/description
 Netfilter (IPv4) kernel modules for extra NAT targets
 Includes:
 - NETMAP
 - REDIRECT
endef

$(eval $(call KernelPackage,ipt-nat-extra))


define KernelPackage/nf-nathelper
  SUBMENU:=$(NF_MENU)
  TITLE:=Basic Conntrack and NAT helpers
  KCONFIG:=$(KCONFIG_NF_NATHELPER)
  FILES:=$(foreach mod,$(NF_NATHELPER-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_NATHELPER-m)))
  DEPENDS:=+kmod-nf-nat
endef

define KernelPackage/nf-nathelper/description
 Default Netfilter (IPv4) Conntrack and NAT helpers
 Includes:
 - ftp
 - irc
 - tftp
endef

$(eval $(call KernelPackage,nf-nathelper))


define KernelPackage/nf-nathelper-extra
  SUBMENU:=$(NF_MENU)
  TITLE:=Extra Conntrack and NAT helpers
  KCONFIG:=$(KCONFIG_NF_NATHELPER_EXTRA)
  FILES:=$(foreach mod,$(NF_NATHELPER_EXTRA-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NF_NATHELPER_EXTRA-m)))
  DEPENDS:=+kmod-nf-nat +kmod-lib-textsearch
endef

define KernelPackage/nf-nathelper-extra/description
 Extra Netfilter (IPv4) Conntrack and NAT helpers
 Includes:
 - amanda
 - h323
 - mms
 - pptp
 - proto_gre
 - sip
 - snmp_basic
 - broadcast
endef

$(eval $(call KernelPackage,nf-nathelper-extra))


define KernelPackage/ipt-ulog
  TITLE:=Module for user-space packet logging
  KCONFIG:=$(KCONFIG_IPT_ULOG)
  FILES:=$(foreach mod,$(IPT_ULOG-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_ULOG-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-ulog/description
 Netfilter (IPv4) module for user-space packet logging
 Includes:
 - ULOG
endef

$(eval $(call KernelPackage,ipt-ulog))


define KernelPackage/ipt-nflog
  TITLE:=Module for user-space packet logging
  KCONFIG:=$(KCONFIG_IPT_NFLOG)
  FILES:=$(foreach mod,$(IPT_NFLOG-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_NFLOG-m)))
  $(call AddDepends/ipt,+kmod-nfnetlink-log)
endef

define KernelPackage/ipt-nflog/description
 Netfilter module for user-space packet logging
 Includes:
 - NFLOG
endef

$(eval $(call KernelPackage,ipt-nflog))


define KernelPackage/ipt-nfqueue
  TITLE:=Module for user-space packet queuing
  KCONFIG:=$(KCONFIG_IPT_NFQUEUE)
  FILES:=$(foreach mod,$(IPT_NFQUEUE-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_NFQUEUE-m)))
  $(call AddDepends/ipt,+kmod-nfnetlink-queue)
endef

define KernelPackage/ipt-nfqueue/description
 Netfilter module for user-space packet queuing
 Includes:
 - NFQUEUE
endef

$(eval $(call KernelPackage,ipt-nfqueue))


define KernelPackage/ipt-debug
  TITLE:=Module for debugging/development
  KCONFIG:=$(KCONFIG_IPT_DEBUG)
  DEFAULT:=n
  FILES:=$(foreach mod,$(IPT_DEBUG-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_DEBUG-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-debug/description
 Netfilter modules for debugging/development of the firewall
 Includes:
 - TRACE
endef

$(eval $(call KernelPackage,ipt-debug))


define KernelPackage/ipt-led
  TITLE:=Module to trigger a LED with a Netfilter rule
  KCONFIG:=$(KCONFIG_IPT_LED)
  FILES:=$(foreach mod,$(IPT_LED-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_LED-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-led/description
 Netfilter target to trigger a LED when a network packet is matched.
endef

$(eval $(call KernelPackage,ipt-led))

define KernelPackage/ipt-tproxy
  TITLE:=Transparent proxying support
  DEPENDS+=+kmod-ipt-conntrack +IPV6:kmod-ip6tables
  KCONFIG:= \
  	CONFIG_NETFILTER_TPROXY \
  	CONFIG_NETFILTER_XT_MATCH_SOCKET \
  	CONFIG_NETFILTER_XT_TARGET_TPROXY
  FILES:= \
  	$(foreach mod,$(IPT_TPROXY-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir nf_tproxy_core $(IPT_TPROXY-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-tproxy/description
  Kernel modules for Transparent Proxying
endef

$(eval $(call KernelPackage,ipt-tproxy))

define KernelPackage/ipt-tee
  TITLE:=TEE support
  DEPENDS:=+kmod-ipt-conntrack @!LINUX_4_4
  KCONFIG:= \
  	CONFIG_NETFILTER_XT_TARGET_TEE
  FILES:= \
  	$(LINUX_DIR)/net/netfilter/xt_TEE.ko \
  	$(foreach mod,$(IPT_TEE-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir nf_tee $(IPT_TEE-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-tee/description
  Kernel modules for TEE
endef

$(eval $(call KernelPackage,ipt-tee))


define KernelPackage/ipt-u32
  TITLE:=U32 support
  KCONFIG:= \
  	CONFIG_NETFILTER_XT_MATCH_U32
  FILES:= \
  	$(LINUX_DIR)/net/netfilter/xt_u32.ko \
  	$(foreach mod,$(IPT_U32-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir nf_tee $(IPT_U32-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-u32/description
  Kernel modules for U32
endef

$(eval $(call KernelPackage,ipt-u32))


define KernelPackage/ipt-iprange
  TITLE:=Module for matching ip ranges
  KCONFIG:=$(KCONFIG_IPT_IPRANGE)
  FILES:=$(foreach mod,$(IPT_IPRANGE-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_IPRANGE-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-iprange/description
 Netfilter (IPv4) module for matching ip ranges
 Includes:
 - iprange
endef

$(eval $(call KernelPackage,ipt-iprange))

define KernelPackage/ipt-cluster
  TITLE:=Module for matching cluster
  KCONFIG:=$(KCONFIG_IPT_CLUSTER)
  FILES:=$(foreach mod,$(IPT_CLUSTER-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_CLUSTER-m)))
  $(call AddDepends/ipt)
endef

define KernelPackage/ipt-cluster/description
 Netfilter (IPv4/IPv6) module for matching cluster
 This option allows you to build work-load-sharing clusters of
 network servers/stateful firewalls without having a dedicated
 load-balancing router/server/switch. Basically, this match returns
 true when the packet must be handled by this cluster node. Thus,
 all nodes see all packets and this match decides which node handles
 what packets. The work-load sharing algorithm is based on source
 address hashing.

 This module is usable for ipv4 and ipv6.

 To use it also enable iptables-mod-cluster

 see `iptables -m cluster --help` for more information.
endef

$(eval $(call KernelPackage,ipt-cluster))

define KernelPackage/ipt-clusterip
  TITLE:=Module for CLUSTERIP
  KCONFIG:=$(KCONFIG_IPT_CLUSTERIP)
  FILES:=$(foreach mod,$(IPT_CLUSTERIP-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_CLUSTERIP-m)))
  $(call AddDepends/ipt,+kmod-nf-conntrack)
endef

define KernelPackage/ipt-clusterip/description
 Netfilter (IPv4-only) module for CLUSTERIP
 The CLUSTERIP target allows you to build load-balancing clusters of
 network servers without having a dedicated load-balancing
 router/server/switch.

 To use it also enable iptables-mod-clusterip

 see `iptables -j CLUSTERIP --help` for more information.
endef

$(eval $(call KernelPackage,ipt-clusterip))


define KernelPackage/ipt-extra
  TITLE:=Extra modules
  KCONFIG:=$(KCONFIG_IPT_EXTRA)
  FILES:=$(foreach mod,$(IPT_EXTRA-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(IPT_EXTRA-m)))
  $(call AddDepends/ipt,+kmod-br-netfilter)
endef

define KernelPackage/ipt-extra/description
 Other Netfilter (IPv4) kernel modules
 Includes:
 - addrtype
 - owner
 - physdev (if bridge support was enabled in kernel)
 - pkttype
 - quota
endef

$(eval $(call KernelPackage,ipt-extra))


define KernelPackage/ip6tables
  SUBMENU:=$(NF_MENU)
  TITLE:=IPv6 modules
  DEPENDS:=+kmod-nf-ipt6 +kmod-ipt-core +kmod-ipt-conntrack
  KCONFIG:=$(KCONFIG_IPT_IPV6)
  FILES:=$(foreach mod,$(IPT_IPV6-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoLoad,42,$(notdir $(IPT_IPV6-m)))
endef

define KernelPackage/ip6tables/description
 Netfilter IPv6 firewalling support
endef

$(eval $(call KernelPackage,ip6tables))

define KernelPackage/ip6tables-extra
  SUBMENU:=$(NF_MENU)
  TITLE:=Extra IPv6 modules
  DEPENDS:=+kmod-ip6tables
  KCONFIG:=$(KCONFIG_IPT_IPV6_EXTRA)
  FILES:=$(foreach mod,$(IPT_IPV6_EXTRA-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoLoad,43,$(notdir $(IPT_IPV6_EXTRA-m)))
endef

define KernelPackage/ip6tables-extra/description
 Netfilter IPv6 extra header matching modules
endef

$(eval $(call KernelPackage,ip6tables-extra))

ARP_MODULES = arp_tables arpt_mangle arptable_filter
define KernelPackage/arptables
  SUBMENU:=$(NF_MENU)
  TITLE:=ARP firewalling modules
  DEPENDS:=+kmod-ipt-core
  FILES:=$(LINUX_DIR)/net/ipv4/netfilter/arp*.ko
  KCONFIG:=CONFIG_IP_NF_ARPTABLES \
    CONFIG_IP_NF_ARPFILTER \
    CONFIG_IP_NF_ARP_MANGLE
  AUTOLOAD:=$(call AutoProbe,$(ARP_MODULES))
endef

define KernelPackage/arptables/description
 Kernel modules for ARP firewalling
endef

$(eval $(call KernelPackage,arptables))


define KernelPackage/br-netfilter
  SUBMENU:=$(NF_MENU)
  TITLE:=Bridge netfilter support modules
  HIDDEN:=1
  DEPENDS:=+kmod-ipt-core +kmod-bridge
  FILES:=$(LINUX_DIR)/net/bridge/br_netfilter.ko
  KCONFIG:=CONFIG_BRIDGE_NETFILTER
  AUTOLOAD:=$(call AutoProbe,br_netfilter)
endef

$(eval $(call KernelPackage,br-netfilter))


define KernelPackage/ebtables
  SUBMENU:=$(NF_MENU)
  TITLE:=Bridge firewalling modules
  DEPENDS:=+kmod-ipt-core +kmod-bridge +kmod-br-netfilter
  FILES:=$(foreach mod,$(EBTABLES-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_EBTABLES)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(EBTABLES-m)))
endef

define KernelPackage/ebtables/description
  ebtables is a general, extensible frame/packet identification
  framework. It provides you to do Ethernet
  filtering/NAT/brouting on the Ethernet bridge.
endef

$(eval $(call KernelPackage,ebtables))


define AddDepends/ebtables
  SUBMENU:=$(NF_MENU)
  DEPENDS+=kmod-ebtables $(1)
endef


define KernelPackage/ebtables-ipv4
  TITLE:=ebtables: IPv4 support
  FILES:=$(foreach mod,$(EBTABLES_IP4-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_EBTABLES_IP4)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(EBTABLES_IP4-m)))
  $(call AddDepends/ebtables)
endef

define KernelPackage/ebtables-ipv4/description
 This option adds the IPv4 support to ebtables, which allows basic
 IPv4 header field filtering, ARP filtering as well as SNAT, DNAT targets.
endef

$(eval $(call KernelPackage,ebtables-ipv4))


define KernelPackage/ebtables-ipv6
  TITLE:=ebtables: IPv6 support
  FILES:=$(foreach mod,$(EBTABLES_IP6-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_EBTABLES_IP6)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(EBTABLES_IP6-m)))
  $(call AddDepends/ebtables)
endef

define KernelPackage/ebtables-ipv6/description
 This option adds the IPv6 support to ebtables, which allows basic
 IPv6 header field filtering and target support.
endef

$(eval $(call KernelPackage,ebtables-ipv6))


define KernelPackage/ebtables-watchers
  TITLE:=ebtables: watchers support
  FILES:=$(foreach mod,$(EBTABLES_WATCHERS-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_EBTABLES_WATCHERS)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(EBTABLES_WATCHERS-m)))
  $(call AddDepends/ebtables)
endef

define KernelPackage/ebtables-watchers/description
 This option adds the log watchers, that you can use in any rule
 in any ebtables table.
endef

$(eval $(call KernelPackage,ebtables-watchers))


define KernelPackage/nfnetlink
  SUBMENU:=$(NF_MENU)
  TITLE:=Netlink-based userspace interface
  FILES:=$(foreach mod,$(NFNETLINK-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_NFNETLINK)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NFNETLINK-m)))
endef

define KernelPackage/nfnetlink/description
 Kernel modules support for a netlink-based userspace interface
endef

$(eval $(call KernelPackage,nfnetlink))


define AddDepends/nfnetlink
  SUBMENU:=$(NF_MENU)
  DEPENDS+=+kmod-nfnetlink $(1)
endef


define KernelPackage/nfnetlink-log
  TITLE:=Netfilter LOG over NFNETLINK interface
  FILES:=$(foreach mod,$(NFNETLINK_LOG-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_NFNETLINK_LOG)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NFNETLINK_LOG-m)))
  $(call AddDepends/nfnetlink)
endef

define KernelPackage/nfnetlink-log/description
 Kernel modules support for logging packets via NFNETLINK
 Includes:
 - NFLOG
endef

$(eval $(call KernelPackage,nfnetlink-log))


define KernelPackage/nfnetlink-queue
  TITLE:=Netfilter QUEUE over NFNETLINK interface
  FILES:=$(foreach mod,$(NFNETLINK_QUEUE-m),$(LINUX_DIR)/net/$(mod).ko)
  KCONFIG:=$(KCONFIG_NFNETLINK_QUEUE)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NFNETLINK_QUEUE-m)))
  $(call AddDepends/nfnetlink)
endef

define KernelPackage/nfnetlink-queue/description
 Kernel modules support for queueing packets via NFNETLINK
 Includes:
 - NFQUEUE
endef

$(eval $(call KernelPackage,nfnetlink-queue))


define KernelPackage/nf-conntrack-netlink
  TITLE:=Connection tracking netlink interface
  FILES:=$(LINUX_DIR)/net/netfilter/nf_conntrack_netlink.ko
  KCONFIG:=CONFIG_NF_CT_NETLINK CONFIG_NF_CONNTRACK_EVENTS=y
  AUTOLOAD:=$(call AutoProbe,nf_conntrack_netlink)
  $(call AddDepends/nfnetlink,+kmod-ipt-conntrack)
endef

define KernelPackage/nf-conntrack-netlink/description
 Kernel modules support for a netlink-based connection tracking
 userspace interface
endef

$(eval $(call KernelPackage,nf-conntrack-netlink))

define KernelPackage/ipt-hashlimit
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter hashlimit match
  DEPENDS:=+kmod-ipt-core
  KCONFIG:=$(KCONFIG_IPT_HASHLIMIT)
  FILES:=$(LINUX_DIR)/net/netfilter/xt_hashlimit.ko
  AUTOLOAD:=$(call AutoProbe,xt_hashlimit)
  $(call KernelPackage/ipt)
endef

define KernelPackage/ipt-hashlimit/description
 Kernel modules support for the hashlimit bucket match module
endef

$(eval $(call KernelPackage,ipt-hashlimit))


define KernelPackage/nft-core
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter nf_tables support
  DEPENDS:=+kmod-nfnetlink +kmod-nf-conntrack6
  FILES:=$(foreach mod,$(NFT_CORE-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NFT_CORE-m)))
  KCONFIG:= \
	CONFIG_NETFILTER=y \
	CONFIG_NETFILTER_ADVANCED=y \
	CONFIG_NFT_COMPAT=n \
	CONFIG_NFT_QUEUE=n \
	CONFIG_NF_TABLES_ARP=n \
	CONFIG_NF_TABLES_BRIDGE=n \
	$(KCONFIG_NFT_CORE)
endef

define KernelPackage/nft-core/description
 Kernel module support for nftables
endef

$(eval $(call KernelPackage,nft-core))


define KernelPackage/nft-nat
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter nf_tables NAT support
  DEPENDS:=+kmod-nft-core +kmod-nf-nat
  FILES:=$(foreach mod,$(NFT_NAT-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NFT_NAT-m)))
  KCONFIG:=$(KCONFIG_NFT_NAT)
endef

$(eval $(call KernelPackage,nft-nat))


define KernelPackage/nft-nat6
  SUBMENU:=$(NF_MENU)
  TITLE:=Netfilter nf_tables IPv6-NAT support
  DEPENDS:=+kmod-nft-core +kmod-nf-nat6
  FILES:=$(foreach mod,$(NFT_NAT6-m),$(LINUX_DIR)/net/$(mod).ko)
  AUTOLOAD:=$(call AutoProbe,$(notdir $(NFT_NAT6-m)))
  KCONFIG:=$(KCONFIG_NFT_NAT6)
endef

$(eval $(call KernelPackage,nft-nat6))

