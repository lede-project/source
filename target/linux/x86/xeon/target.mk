ARCH:=x86_64
BOARDNAME:=xeon
DEFAULT_PACKAGES += kmod-button-hotplug kmod-e1000e kmod-e1000 kmod-igb \
	kmod-ixgbe
ARCH_PACKAGES:=xeon
CPU_TYPE:=westmere
MAINTAINER:=Philip Prindevile <philipp@redfish-solutions.com>

define Target/Description
        Build images for 64 bit Xeon-based servers and network appliances.
endef
