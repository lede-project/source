obj-$(CONFIG_RK_WIFI_DEVICE_UWE5622) += unisocwcn/
obj-$(CONFIG_WLAN_UWE5622)    += unisocwifi/
obj-$(CONFIG_TTY_OVERY_SDIO)  += tty-sdio/

UNISOCWCN_DIR := $(shell cd $(src)/unisocwcn/ && /bin/pwd)
UNISOC_BSP_INCLUDE := $(UNISOCWCN_DIR)/include
export UNISOC_BSP_INCLUDE

UNISOC_FW_PATH_CONFIG := "/lib/firmware/uwe5622/"
export UNISOC_FW_PATH_CONFIG
