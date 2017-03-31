#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

VIRTUAL_MENU:=Virtualization Support

define KernelPackage/virtio-balloon
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=VirtIO balloon driver
  DEPENDS:=@TARGET_x86_kvm_guest
  KCONFIG:=CONFIG_VIRTIO_BALLOON
  FILES:=$(LINUX_DIR)/drivers/virtio/virtio_balloon.ko
  AUTOLOAD:=$(call AutoLoad,06,virtio-balloon)
endef

define KernelPackage/virtio-balloon/description
 Kernel module for VirtIO memory ballooning support
endef

$(eval $(call KernelPackage,virtio-balloon))


define KernelPackage/virtio-net
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=VirtIO network driver
  DEPENDS:=@TARGET_x86_kvm_guest
  KCONFIG:=CONFIG_VIRTIO_NET
  FILES:=$(LINUX_DIR)/drivers/net/virtio_net.ko
  AUTOLOAD:=$(call AutoLoad,50,virtio_net)
endef

define KernelPackage/virtio-net/description
 Kernel module for the VirtIO paravirtualized network device
endef

$(eval $(call KernelPackage,virtio-net))


define KernelPackage/virtio-random
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=VirtIO Random Number Generator support
  DEPENDS:=@TARGET_x86_kvm_guest
  KCONFIG:=CONFIG_HW_RANDOM_VIRTIO
  FILES:=$(LINUX_DIR)/drivers/char/hw_random/virtio-rng.ko
  AUTOLOAD:=$(call AutoLoad,09,virtio-rng)
endef

define KernelPackage/virtio-random/description
 Kernel module for the VirtIO Random Number Generator
endef

$(eval $(call KernelPackage,virtio-random))


define KernelPackage/xen-privcmd
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=Xen private commands
  DEPENDS:=@TARGET_x86_xen_domu
  KCONFIG:=CONFIG_XEN_PRIVCMD
  FILES:=$(LINUX_DIR)/drivers/xen/xen-privcmd.ko
  AUTOLOAD:=$(call AutoLoad,04,xen-privcmd)
endef

define KernelPackage/xen-privcmd/description
 Kernel module for Xen private commands
endef

$(eval $(call KernelPackage,xen-privcmd))


define KernelPackage/xen-fs
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=Xen filesystem
  DEPENDS:=@TARGET_x86_xen_domu +kmod-xen-privcmd
  KCONFIG:= \
  	CONFIG_XENFS \
  	CONFIG_XEN_COMPAT_XENFS=y
  FILES:=$(LINUX_DIR)/drivers/xen/xenfs/xenfs.ko
  AUTOLOAD:=$(call AutoLoad,05,xenfs)
endef

define KernelPackage/xen-fs/description
 Kernel module for the Xen filesystem
endef

$(eval $(call KernelPackage,xen-fs))


define KernelPackage/xen-evtchn
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=Xen event channels
  DEPENDS:=@TARGET_x86_xen_domu
  KCONFIG:=CONFIG_XEN_DEV_EVTCHN
  FILES:=$(LINUX_DIR)/drivers/xen/xen-evtchn.ko
  AUTOLOAD:=$(call AutoLoad,06,xen-evtchn)
endef

define KernelPackage/xen-evtchn/description
 Kernel module for the /dev/xen/evtchn device
endef

$(eval $(call KernelPackage,xen-evtchn))

define KernelPackage/xen-fbdev
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=Xen virtual frame buffer
  DEPENDS:=@TARGET_x86_xen_domu +kmod-fb
  KCONFIG:= \
  	CONFIG_XEN_FBDEV_FRONTEND \
  	CONFIG_FB_DEFERRED_IO=y \
  	CONFIG_FB_SYS_COPYAREA \
  	CONFIG_FB_SYS_FILLRECT \
  	CONFIG_FB_SYS_FOPS \
  	CONFIG_FB_SYS_IMAGEBLIT \
  	CONFIG_FIRMWARE_EDID=n
  FILES:= \
  	$(LINUX_DIR)/drivers/video/fbdev/xen-fbfront.ko \
  	$(LINUX_DIR)/drivers/video/fbdev/core/syscopyarea.ko \
  	$(LINUX_DIR)/drivers/video/fbdev/core/sysfillrect.ko \
  	$(LINUX_DIR)/drivers/video/fbdev/core/fb_sys_fops.ko \
  	$(LINUX_DIR)/drivers/video/fbdev/core/sysimgblt.ko
  AUTOLOAD:=$(call AutoLoad,07, \
  	fb \
  	syscopyarea \
  	sysfillrect \
  	fb_sys_fops \
  	sysimgblt \
  	xen-fbfront \
  )
endef

define KernelPackage/xen-fbdev/description
 Kernel module for the Xen virtual frame buffer
endef

$(eval $(call KernelPackage,xen-fbdev))


define KernelPackage/xen-netdev
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=Xen network device frontend
  DEPENDS:=@TARGET_x86_xen_domu
  KCONFIG:=CONFIG_XEN_NETDEV_FRONTEND
  FILES:=$(LINUX_DIR)/drivers/net/xen-netfront.ko
  AUTOLOAD:=$(call AutoLoad,09,xen-netfront)
endef

define KernelPackage/xen-netdev/description
 Kernel module for the Xen network device frontend
endef

$(eval $(call KernelPackage,xen-netdev))


define KernelPackage/xen-pcidev
  SUBMENU:=$(VIRTUAL_MENU)
  TITLE:=Xen PCI device frontend
  DEPENDS:=@TARGET_x86_xen_domu
  KCONFIG:=CONFIG_XEN_PCIDEV_FRONTEND
  FILES:=$(LINUX_DIR)/drivers/pci/xen-pcifront.ko
  AUTOLOAD:=$(call AutoLoad,10,xen-pcifront)
endef

define KernelPackage/xen-pcidev/description
 Kernel module for the Xen network device frontend
endef

$(eval $(call KernelPackage,xen-pcidev))

#
# Hyper-V Drives depends on x86 or x86_64.
#
define KernelPackage/hyperv-balloon
  SUBMENU:=$(VIRTUAL_MENU)
  DEPENDS:=@(TARGET_x86||TARGET_x86_64)
  TITLE:=Microsoft Hyper-V Balloon Driver
  KCONFIG:= \
    CONFIG_HYPERV_BALLOON \
    CONFIG_HYPERVISOR_GUEST=y \
    CONFIG_PARAVIRT=n \
    CONFIG_HYPERV=y
  FILES:=$(LINUX_DIR)/drivers/hv/hv_balloon.ko \
    $(LINUX_DIR)/drivers/hv/hv_vmbus.ko
  AUTOLOAD:=$(call AutoLoad,06,hv_balloon)
endef

define KernelPackage/hyperv-balloon/description
  Microsofot Hyper-V balloon driver.
endef

$(eval $(call KernelPackage,hyperv-balloon))

define KernelPackage/hyperv-net-vsc
  SUBMENU:=$(VIRTUAL_MENU)
  DEPENDS:=@(TARGET_x86||TARGET_x86_64)
  TITLE:=Microsoft Hyper-V Network Driver
  KCONFIG:= \
    CONFIG_HYPERV_NET \
    CONFIG_HYPERVISOR_GUEST=y \
    CONFIG_PARAVIRT=n \
    CONFIG_HYPERV=y
  FILES:=$(LINUX_DIR)/drivers/net/hyperv/hv_netvsc.ko \
    $(LINUX_DIR)/drivers/hv/hv_vmbus.ko
  AUTOLOAD:=$(call AutoLoad,35,hv_netvsc)
endef

define KernelPackage/hyperv-net-vsc/description
  Microsoft Hyper-V Network Driver
endef

$(eval $(call KernelPackage,hyperv-net-vsc))

define KernelPackage/hyperv-util
  SUBMENU:=$(VIRTUAL_MENU)
  DEPENDS:=@(TARGET_x86||TARGET_x86_64)
  TITLE:=Microsoft Hyper-V Utility Driver
  KCONFIG:= \
    CONFIG_HYPERV_UTILS \
    CONFIG_HYPERVISOR_GUEST=y \
    CONFIG_PARAVIRT=n \
    CONFIG_HYPERV=y
  FILES:=$(LINUX_DIR)/drivers/hv/hv_util.ko \
    $(LINUX_DIR)/drivers/hv/hv_vmbus.ko
  AUTOLOAD:=$(call AutoLoad,10,hv_util)
endef

define KernelPackage/hyperv-util/description
  Microsoft Hyper-V Utility Driver
endef

$(eval $(call KernelPackage,hyperv-util))

#
# Hyper-V Storage Drive needs to be in kernel rather than module to load the root fs.
#
define KernelPackage/hyperv-storage
  SUBMENU:=$(VIRTUAL_MENU)
  DEPENDS:=@(TARGET_x86||TARGET_x86_64) +kmod-scsi-core
  TITLE:=Microsoft Hyper-V Storage Driver
  KCONFIG:= \
    CONFIG_HYPERV_STORAGE=y \
    CONFIG_HYPERVISOR_GUEST=y \
    CONFIG_PARAVIRT=n \
    CONFIG_HYPERV=y
  FILES:=$(LINUX_DIR)/drivers/scsi/hv_storvsc.ko \
    $(LINUX_DIR)/drivers/hv/hv_vmbus.ko
  AUTOLOAD:=$(call AutoLoad,40,hv_storvsc)
endef

define KernelPackage/hyperv-storage/description
  Microsoft Hyper-V Storage Driver
endef

$(eval $(call KernelPackage,hyperv-storage))
