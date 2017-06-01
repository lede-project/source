BOARDNAME:=i386 for UEFI based system
DEFAULT_PACKAGES += kmod-button-hotplug kmod-e1000e kmod-e1000 kmod-r8169 kmod-igb kmod-fs-vfat kmod-fs-msdos

define Target/Description
	Build firmware images for modern x86 based boards with CPUs
	supporting at least the Intel Pentium 4 instruction set with
	MMX, SSE and SSE2. This subtarget is built for UEFI based system
endef

