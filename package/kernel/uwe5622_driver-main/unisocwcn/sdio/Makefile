obj-$(CONFIG_SDIOHAL) := sdiohal.o

sdiohal-c-objs += sdiohal_main.o
sdiohal-c-objs += sdiohal_common.o
sdiohal-c-objs += sdiohal_tx.o
sdiohal-c-objs += sdiohal_rx.o
sdiohal-c-objs += sdio_v3.o

ifdef CONFIG_DEBUG_FS
sdiohal-c-objs += sdiohal_ctl.o
endif

sdiohal-objs += $(sdiohal-c-objs)
