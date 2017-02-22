ARCH:=x86_64
BOARDNAME:=x86_64 for UEFI based system
DEFAULT_PACKAGES += kmod-button-hotplug kmod-e1000e kmod-e1000 kmod-r8169 kmod-igb kmod-fs-vfat kmod-fs-msdos

define Target/Description
        Build images for 64 bit UEFI systems including virtualized guests.
endef
