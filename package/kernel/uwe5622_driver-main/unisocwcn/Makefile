PWD := $(shell pwd)
CURFOLDER ?= $(pwd)

LD_CFLAGS += -D__linux__
EXTRA_CFLAGS=-fno-pic

all_dependencies := driver
driver_dependencies :=
BIN2HEX_TOOL := $(PWD)/fw/bin2hex
firmware_file := $(PWD)/fw/wcnmodem.bin
firmware_target := $(PWD)/fw/wcnmodem.bin.hex

#### Global configs for each item ######
ifeq ($(CFG_HI_WIFI_DEVICE_UWE5621),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
#ccflags-y += -DCONFIG_UWE5621
BSP_CHIP_ID := uwe5621
WCN_HW_TYPE := sdio
endif

ifeq ($(CFG_HI_WIFI_DEVICE_UWE5623),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
#ccflags-y += -DCONFIG_UWE5623
BSP_CHIP_ID := uwe5621
WCN_HW_TYPE := sdio
endif

ifeq ($(CFG_HI_WIFI_DEVICE_UWE5623_USB),y)
#export CONFIG_WCN_SDIO = y
export CONFIG_WCN_USB = y
#ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
ccflags-y += -DCONFIG_UWE5623
BSP_CHIP_ID := uwe5623
WCN_HW_TYPE := usb
endif

ifeq ($(CFG_AML_WIFI_DEVICE_UWE5621),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
#ccflags-y += -DCONFIG_UWE5621
BSP_CHIP_ID := uwe5621
WCN_HW_TYPE := sdio
endif

ifeq ($(CFG_AML_WIFI_DEVICE_UWE5622),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
# export CONFIG_WCN_GNSS = y
ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
#ccflags-y += -DCONFIG_UWE5622
BSP_CHIP_ID := uwe5621
WCN_HW_TYPE := sdio
endif

ifeq ($(CONFIG_RK_WIFI_DEVICE_UWE5621),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
#ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
ccflags-y += -DCONFIG_UWE5621
BSP_CHIP_ID := uwe5621
WCN_HW_TYPE := sdio
endif

ifeq ($(CONFIG_RK_WIFI_DEVICE_UWE5622),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
# export CONFIG_WCN_GNSS = y
ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
#ccflags-y += -DCONFIG_UWE5622
BSP_CHIP_ID := uwe5622
WCN_HW_TYPE := sdio
endif

ifeq ($(CONFIG_AW_WIFI_DEVICE_UWE5622),y)
export CONFIG_WCN_SDIO = y
#export CONFIG_WCN_USB = y
# export CONFIG_WCN_GNSS = y
ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
# ccflags-y += -DCONFIG_UWE5622
BSP_CHIP_ID := uwe5622
WCN_HW_TYPE := sdio
endif

MODULE_NAME := $(BSP_CHIP_ID)_bsp_$(WCN_HW_TYPE)

ifneq ($(UNISOC_FW_PATH_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_FW_PATH=\"$(UNISOC_FW_PATH_CONFIG)\"
endif
ifneq ($(UNISOC_DBG_FILESIZE_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_DBG_FILESIZE=$(UNISOC_DBG_FILESIZE_CONFIG)
endif
ifneq ($(UNISOC_DBG_FILENUM_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_DBG_FILENUM=$(UNISOC_DBG_FILENUM_CONFIG)
endif
ifneq ($(UNISOC_DBG_PATH_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_DBG_PATH=\"$(UNISOC_DBG_PATH_CONFIG)\"
endif
ifneq ($(UNISOC_SDIO_IRQ_TYPE_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_SDIO_IRQ_TYPE=$(UNISOC_SDIO_IRQ_TYPE_CONFIG)
endif

#### add cflag ######
ifeq ($(CONFIG_WCN_SDIO),y)
ccflags-y += -DCONFIG_WCN_SDIO
ccflags-y += -DCONFIG_WCN_SLP
ccflags-y += -DCONFIG_WCN_SWD
export CONFIG_WCN_SWD = y
endif
ifeq ($(CONFIG_WCN_PCIE),y)
ccflags-y += -DCONFIG_WCN_PCIE
endif
ifeq ($(CONFIG_WCN_SIPC),y)
ccflags-y += -DCONFIG_WCN_SIPC
endif
ifeq ($(CONFIG_WCN_USB),y)
ccflags-y += -DCONFIG_WCN_USB
endif

ifeq ($(CONFIG_WCN_GNSS),y)
ccflags-y += -DCONFIG_WCN_GNSS
endif

ccflags-y += -DCONFIG_WCN_BOOT
ccflags-y += -DCONFIG_WCN_UTILS

#### include path ######
ccflags-y += -I$(src)/include/
ccflags-y += -I$(src)/platform/
ccflags-y += -I$(src)/platform/rf/

#### add cflag for Customer ######
### ---------- Hisilicon start ---------- ###
ifeq ($(CFG_HI_WIFI_DEVICE_UWE5621),y)
#### add cflag ######
ccflags-y += -DCONFIG_HISI_BOARD
#ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_CHIPEN
#ccflags-y += -DCONFIG_WCN_RESET
#ccflags-y += -DCONFIG_WCN_POWER_UP_DOWN
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
ccflags-y += -DCONFIG_BT_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WL_WAKE_HOST_EN
ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
#ccflags-y += -DCONFIG_MEM_PD
ifeq ($(CONFIG_WCN_SDIO),y)
ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DCONFIG_WCN_LOOPCHECK
ccflags-y += -DCONFIG_CPLOG_DEBUG
endif
ccflags-y += -DCONFIG_SDIO_TX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_RX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_INBAND_INT
#ccflags-y += -DCONFIG_SDIO_INBAND_POLLING
ccflags-y += -DCONFIG_SDIO_BLKSIZE_512
#ccflags-y += -DCONFIG_SDIO_PWRSEQ
endif

all_dependencies += install
endif

ifeq ($(CFG_HI_WIFI_DEVICE_UWE5623),y)
#### add cflag ######
ccflags-y += -DCONFIG_HISI_BOARD
#ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_CHIPEN
#ccflags-y += -DCONFIG_WCN_RESET
#ccflags-y += -DCONFIG_WCN_POWER_UP_DOWN
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
ccflags-y += -DCONFIG_BT_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WL_WAKE_HOST_EN
ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DCONFIG_WCN_LOOPCHECK
ccflags-y += -DCONFIG_CPLOG_DEBUG
endif
#ccflags-y += -DCONFIG_MEM_PD
ccflags-y += -DCONFIG_SDIO_TX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_RX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_INBAND_INT
#ccflags-y += -DCONFIG_SDIO_INBAND_POLLING
ccflags-y += -DCONFIG_SDIO_BLKSIZE_512
#ccflags-y += -DCONFIG_SDIO_PWRSEQ

all_dependencies += install
endif

ifeq ($(CFG_HI_WIFI_DEVICE_UWE5623_USB),y)
#### add cflag ######
ccflags-y += -DCONFIG_HISI_BOARD
#ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_CHIPEN
#ccflags-y += -DCONFIG_WCN_RESET
#ccflags-y += -DCONFIG_WCN_POWER_UP_DOWN
#ccflags-y += -DCONFIG_WCN_INT_AP
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
#ccflags-y += -DCONFIG_BT_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WL_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
ccflags-y += -DCONFIG_SYS_REBOOT_NOT_REPOWER_USBCHIP
ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DCONFIG_WCN_LOOPCHECK
ccflags-y += -DCONFIG_CPLOG_DEBUG
endif

all_dependencies += install
endif

### ---------- Hisilicon end ---------- ###

### ---------- Amlogic start ---------- ###
ifeq ($(CFG_AML_WIFI_DEVICE_UWE5621),y)
#### add cflag ######
ccflags-y += -DCONFIG_AML_BOARD
#ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_CHIPEN
#ccflags-y += -DCONFIG_WCN_RESET
ccflags-y += -DCONFIG_WCN_RESET_PIN_CONNECTED
ccflags-y += -DCONFIG_WCN_POWER_UP_DOWN
ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
ccflags-y += -DCONFIG_BT_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WL_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
#ccflags-y += -DCONFIG_WCN_LOOPCHECK
#ccflags-y += -DCONFIG_CPLOG_DEBUG
#ccflags-y += -DCONFIG_MEM_PD
#ccflags-y += -DCONFIG_SDIO_TX_ADMA_MODE
#ccflags-y += -DCONFIG_SDIO_RX_ADMA_MODE
# ccflags-y += -DCONFIG_SDIO_INBAND_INT
ccflags-y += -DCONFIG_CUSTOMIZE_SDIO_IRQ_TYPE=2
#ccflags-y += -DCONFIG_SDIO_INBAND_POLLING
ccflags-y += -DCONFIG_SDIO_BLKSIZE_512
#ccflags-y += -DCONFIG_SDIO_PWRSEQ

driver_dependencies += $(firmware_target)
endif

ifeq ($(CFG_AML_WIFI_DEVICE_UWE5622),y)
#### add cflag ######
ccflags-y += -DCONFIG_AML_BOARD
#ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_CHIPEN
#ccflags-y += -DCONFIG_WCN_RESET
#ccflags-y += -DCONFIG_WCN_RESET_PIN_CONNECTED
ccflags-y += -DCONFIG_WCN_POWER_UP_DOWN
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
#ccflags-y += -DCONFIG_BT_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WL_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
#ccflags-y += -DCONFIG_WCN_LOOPCHECK
#ifneq ($(TARGET_BUILD_VARIANT),user)
#ccflags-y += -DCONFIG_CPLOG_DEBUG
#endif
#ccflags-y += -DCONFIG_MEM_PD
#ccflags-y += -DCONFIG_SDIO_TX_ADMA_MODE
#ccflags-y += -DCONFIG_SDIO_RX_ADMA_MODE
#ccflags-y += -DCONFIG_SDIO_INBAND_INT
#ccflags-y += -DCONFIG_SDIO_INBAND_POLLING
ccflags-y += -DCONFIG_CUSTOMIZE_SDIO_IRQ_TYPE=3
ccflags-y += -DCONFIG_SDIO_BLKSIZE_512
#ccflags-y += -DCONFIG_SDIO_PWRSEQ

driver_dependencies += $(firmware_target)
endif
### ---------- Amlogic end ---------- ###

### ---------- Rockchip start ---------- ###
ifeq ($(CONFIG_RK_WIFI_DEVICE_UWE5621),y)
#### add cflag ######
ccflags-y += -DCONFIG_RK_BOARD
ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
#ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
ifneq ($(TARGET_BUILD_VARIANT),user)
#ccflags-y += -DCONFIG_WCN_LOOPCHECK
ccflags-y += -DCONFIG_CPLOG_DEBUG
endif
#ccflags-y += -DCONFIG_MEM_PD

all_dependencies += install
endif

ifeq ($(CONFIG_RK_WIFI_DEVICE_UWE5622),y)
#### add cflag ######
ccflags-y += -DCONFIG_RK_BOARD
ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
#ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DCONFIG_WCN_LOOPCHECK
#ccflags-y += -DCONFIG_CPLOG_DEBUG
endif
#ccflags-y += -DCONFIG_MEM_PD

all_dependencies += install
endif
### ---------- Rockchip end ---------- ###

### ---------- Allwinner start ---------- ###
ifeq ($(CONFIG_AW_WIFI_DEVICE_UWE5622),y)
#### add cflag ######
ccflags-y += -DCONFIG_AW_BOARD
#ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_WCN_CHIPEN
#ccflags-y += -DCONFIG_WCN_RESET
#ccflags-y += -DCONFIG_WCN_RESET_PIN_CONNECTED
ccflags-y += -DCONFIG_WCN_POWER_UP_DOWN
#ccflags-y += -DCONFIG_WCN_DOWNLOAD_FIRMWARE_FROM_HEX
ccflags-y += -DCONFIG_BT_WAKE_HOST_EN
ccflags-y += -DCONFIG_WL_WAKE_HOST_EN
#ccflags-y += -DCONFIG_WCN_RESUME_POWER_DOWN
#ccflags-y += -DCONFIG_WCN_RESUME_KEEPPWR_RESETSDIO
#ccflags-y += -DCONFIG_WCN_CHECK_MODULE_VENDOR
#ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DCONFIG_WCN_LOOPCHECK
#ccflags-y += -DCONFIG_CPLOG_DEBUG
#endif
#ccflags-y += -DCONFIG_MEM_PD
ccflags-y += -DCONFIG_SDIO_TX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_RX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_INBAND_INT
#ccflags-y += -DCONFIG_SDIO_INBAND_POLLING
#ccflags-y += -DCONFIG_SDIO_BLKSIZE_512
#ccflags-y += -DCONFIG_SDIO_PWRSEQ

all_dependencies += install
endif
### ---------- Allwinner end ---------- ###

ifdef CONFIG_WCN_BSP_DRIVER_BUILDIN
#
# Support sprdwcn_bsp build-in.
#
obj-y := $(MODULE_NAME).o
else
#
# Support sprdwcn_bsp ko module loading.
# This else branch is reserved for debuging.
#
obj-m := $(MODULE_NAME).o
endif

$(MODULE_NAME)-y += wcn_bus.o \
			platform/wcn_boot.o \
			platform/wcn_dump.o \
			platform/bufring.o \
			platform/wcn_txrx.o \
			platform/wcn_op.o \
			platform/wcn_parn_parser.o \
			platform/rdc_debug.o \
			platform/wcn_log.o \
			platform/wcn_misc.o \
			platform/wcn_procfs.o \
			platform/loopcheck.o

$(MODULE_NAME)-$(CONFIG_WCN_SWD) += platform/wcn_swd_dp.o

$(MODULE_NAME)-$(CONFIG_WCN_USB) += \
			usb/wcn_usb.o \
			usb/wcn_usb_rx_tx.o \
			usb/wcn_usb_store.o \
			usb/wcn_usb_io.o \
			usb/wcn_usb_test.o \
			usb/wcn_usb_download.o \
			platform/usb_boot.o

$(MODULE_NAME)-$(CONFIG_WCN_SDIO) += \
			sdio/sdiohal_main.o \
			sdio/sdiohal_common.o \
			sdio/sdiohal_tx.o \
			sdio/sdiohal_rx.o \
			sdio/sdio_v3.o \
			sdio/sdiohal_ctl.o \
			sleep/sdio_int.o \
			sleep/slp_mgr.o \
			sleep/slp_sdio.o \
			sleep/slp_test.o \
			platform/mem_pd_mgr.o

$(MODULE_NAME)-$(CONFIG_WCN_GNSS) += \
			platform/gnss/gnss_common_ctl.o \
			platform/gnss/gnss_pmnotify_ctl.o \
			platform/gnss/gnss_dbg.o \
			platform/gnss/gnss_dump.o \

KDIR ?= $(ANDROID_PRODUCT_OUT)/obj/KERNEL_OBJ
ARCH ?= arm
CROSS_COMPILE ?= arm-histbv310-linux-

EXTRA_CFLAGS += -I$(src)/include -D__linux__

all: $(all_dependencies)

$(firmware_target): $(firmware_file)
	@echo Convert $(firmware_file) to $(firmware_target)
	@$(BIN2HEX_TOOL) $(firmware_file)

driver: $(driver_dependencies)
	@echo Convert $(firmware_file) to $(firmware_target)
	@$(BIN2HEX_TOOL) $(firmware_file)
	@echo build driver
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

install:
	@echo install targets
	mkdir -p $(ANDROID_PRODUCT_OUT)/system/lib/modules
	cp -fv $(PWD)/$(MODULE_NAME).ko $(ANDROID_PRODUCT_OUT)/system/lib/modules/$(MODULE_NAME).ko
	make clean

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) Module.markers
	$(RM) modules.order
