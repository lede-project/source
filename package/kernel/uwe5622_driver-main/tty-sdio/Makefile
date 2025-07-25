# ccflags-y += -DDEBUG
# ccflags-y += -DKERNEL_VERSION_414
ifeq ($(CONFIG_RK_WIFI_DEVICE_UWE5622),y)
ccflags-y += -I$(srctree)/drivers/net/wireless/uwe5622/sprdwcn/include
endif
#ccflags-y += -DOTT_UWE
ccflags-y += -Wno-implicit-fallthrough

ifneq ($(UNISOC_BSP_INCLUDE),)
ccflags-y += -I$(UNISOC_BSP_INCLUDE)
endif

ifeq ($(CONFIG_RK_WIFI_DEVICE_UWE5622),y)
sprdbt_tty-objs := \
	tty.o \
	rfkill.o \
	lpm.o \
	woble.o\
	alignment/sitm.o \
	dump.o

obj-$(CONFIG_TTY_OVERY_SDIO) += sprdbt_tty.o
endif
