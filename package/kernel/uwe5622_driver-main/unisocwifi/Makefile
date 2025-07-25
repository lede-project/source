PWD := $(shell pwd)
all_dependencies := driver


ifneq ($(UNISOC_BSP_INCLUDE),)
ccflags-y += -I$(UNISOC_BSP_INCLUDE)
endif

ifneq ($(UNISOC_WIFI_CUS_CONFIG),)
ccflags-y += -DCUSTOMIZE_WIFI_CFG_PATH=\"$(UNISOC_WIFI_CUS_CONFIG)\"
endif

ifneq ($(UNISOC_WIFI_MAC_FILE),)
ccflags-y += -DCUSTOMIZE_WIFI_MAC_FILE=\"$(UNISOC_WIFI_MAC_FILE)\"
endif

ifneq ($(UNISOC_MODULE_NAME),)
MODULE_NAME := $(UNISOC_MODULE_NAME)
else
MODULE_NAME := sprdwl_ng
endif

####add cflag######
ccflags-y += -DUWE5621_FTR
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
ccflags-y += -DACS_SUPPORT -DRX_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
#ccflags-y += -DSOFTAP_HOOK
ccflags-y += -DATCMD_ASSERT
ccflags-y += -DTCPACK_DELAY_SUPPORT
#ccflags-y += -DDFS_MASTER
#ccflags-y += -DRX_NAPI
ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DWL_CONFIG_DEBUG
endif
ccflags-y += -DSPLIT_STACK
ccflags-y += -DOTT_UWE
#ccflags-y += -DCP2_RESET_SUPPORT
ifeq ($(UNISOC_STA_SOFTAP_SCC_MODE),y)
ccflags-y += -DSTA_SOFTAP_SCC_MODE
endif

ccflags-$(CONFIG_UNISOC_WIFI_PS) += -DUNISOC_WIFI_PS
ccflags-y += -DPPPOE_LLC_SUPPORT
ccflags-y += -DSYNC_DISCONNECT
#ccflags-y += -DSPRDWL_TX_SELF
#ccflags-y += -DTCP_ACK_DROP_SUPPORT
#ccflags-y += -DWOW_SUPPORT -DCONFIG_PM

#####module name ###
obj-m += $(MODULE_NAME).o

#######add .o file#####
$(MODULE_NAME)-objs += main.o cfg80211.o txrx.o cmdevt.o npi.o msg.o work.o vendor.o \
				  tcp_ack.o mm.o reorder.o wl_core.o tx_msg.o rx_msg.o \
				  wl_intf.o qos.o dbg_ini_util.o reg_domain.o
$(MODULE_NAME)-objs += defrag.o
$(MODULE_NAME)-objs += ibss.o
$(MODULE_NAME)-objs += nan.o
$(MODULE_NAME)-objs += tracer.o
$(MODULE_NAME)-objs += rf_marlin3.o
$(MODULE_NAME)-objs += rtt.o
$(MODULE_NAME)-objs += api_version.o
$(MODULE_NAME)-objs += rnd_mac_addr.o
$(MODULE_NAME)-objs += debug.o
#$(MODULE_NAME)-objs += 11h.o

all: $(all_dependencies)
driver: $(driver_dependencies)
	@echo build driver
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) Module.markers
	$(RM) modules.order
