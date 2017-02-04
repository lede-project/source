/*
 *  MikroTik RouterBOARD hAP-x based boards support
 *
 *  Copyright (C) 2016-2017 Sergey Sergeev <adron@yapic.net>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/export.h>
#include <linux/pci.h>
#include <linux/ath9k_platform.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/ar8216_platform.h>
#include <linux/rle.h>
#include <linux/routerboot.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/74x164.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/irq.h>

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-spi.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include "routerboot.h"

#define RB941_GPIO_LED_ACT      14
#define RB941_GPIO_BTN_RESET    16

#define RB952_GPIO_LED_ACT      4
#define RB952_GPIO_PORT5_POE    14
#define RB952_HC595_GPIO_BASE   40
#define RB952_GPIO_LED_LAN1     40
#define RB952_GPIO_LED_LAN2     41
#define RB952_GPIO_LED_LAN3     42
#define RB952_GPIO_LED_LAN4     43
#define RB952_GPIO_LED_LAN5     44
#define RB952_GPIO_USB_POWER    45
#define RB952_GPIO_LED_WLAN     46

#define RB952_SSR_STROBE        11
#define RB952_SSR_BIT_0         0
#define RB952_SSR_BIT_1         1
#define RB952_SSR_BIT_2         2
#define RB952_SSR_BIT_3         3
#define RB952_SSR_BIT_4         4
#define RB952_SSR_BIT_5         5
#define RB952_SSR_BIT_6         6
#define RB952_SSR_BIT_7         7

#define RB941_KEYS_POLL_INTERVAL 20 /* msecs */
#define RB941_KEYS_DEBOUNCE_INTERVAL (3 * RB941_KEYS_POLL_INTERVAL)

#define RB_ROUTERBOOT_OFFSET    0x0000
#define RB_ROUTERBOOT_SIZE      0xe000
#define RB_HARD_CFG_OFFSET      0xe000
#define RB_HARD_CFG_SIZE        0x1000
#define RB_BIOS_OFFSET          0xf000
#define RB_BIOS_SIZE            0x1000
#define RB_ROUTERBOOT2_OFFSET   0x10000
#define RB_ROUTERBOOT2_SIZE     0xf000
#define RB_SOFT_CFG_OFFSET      0x1f000
#define RB_SOFT_CFG_SIZE        0x1000
#define RB_ROOTFS_OFFSET        0x20000
#define RB_ROOTFS_SIZE          0x9e0000
#define RB_KERNEL_OFFSET        0xa00000
#define RB_KERNEL_SIZE          MTDPART_SIZ_FULL

void __init rb941_wlan_init(void)
{
    char *art_buf;
    u8 wlan_mac[ETH_ALEN];

    art_buf = rb_get_wlan_data();
    if (art_buf == NULL)
            return;

    ath79_init_mac(wlan_mac, ath79_mac_base, 11);
    ath79_register_wmac(art_buf + 0x1000, wlan_mac);

    kfree(art_buf);
}

static struct mtd_partition rb941_spi_partitions[] = {
    {
            .name = "routerboot",
            .offset = RB_ROUTERBOOT_OFFSET,
            .size = RB_ROUTERBOOT_SIZE,
    }, {
            .name = "hard_config",
            .offset = RB_HARD_CFG_OFFSET,
            .size = RB_HARD_CFG_SIZE,
    }, {
            .name = "bios",
            .offset = RB_BIOS_OFFSET,
            .size = RB_BIOS_SIZE,
    }, {
            .name = "routerboot2",
            .offset = RB_ROUTERBOOT2_OFFSET,
            .size = RB_ROUTERBOOT2_SIZE,
    }, {
            .name = "soft_config",
            .offset = RB_SOFT_CFG_OFFSET,
            .size = RB_SOFT_CFG_SIZE,
    }, {
            .name = "rootfs",
            .offset = RB_ROOTFS_OFFSET,
            .size = RB_ROOTFS_SIZE,
    }, {
            .name = "kernel",
            .offset = RB_KERNEL_OFFSET,
            .size = RB_KERNEL_SIZE,
    }
};

static struct flash_platform_data rb941_spi_flash_data = {
    .parts          = rb941_spi_partitions,
    .nr_parts       = ARRAY_SIZE(rb941_spi_partitions),
};

static struct gpio_led rb941_leds[] __initdata = {
    {
        .name = "rb:green:act",
        .gpio = RB941_GPIO_LED_ACT,
        .active_low = 1,
    },
};

static struct gpio_keys_button rb941_gpio_keys[] __initdata = {
    {
        .desc = "Reset button",
        .type = EV_KEY,
        .code = KEY_RESTART,
        .debounce_interval = RB941_KEYS_DEBOUNCE_INTERVAL,
        .gpio = RB941_GPIO_BTN_RESET,
        .active_low = 1,
    },
};

static struct gpio_led rb952_leds[] __initdata = {
    {
        .name = "rb:green:act",
        .gpio = RB952_GPIO_LED_ACT,
        .active_low = 1,
    }, {
        .name = "rb:red:poe5",
        .gpio = RB952_GPIO_PORT5_POE,
        .active_low = 0,
    }, {
        .name = "rb:blue:wlan",
        .gpio = RB952_GPIO_LED_WLAN,
        .active_low = 1,
    }, {
        .name = "rb:green:port1",
        .gpio = RB952_GPIO_LED_LAN1,
        .active_low = 1,
    }, {
        .name = "rb:green:port2",
        .gpio = RB952_GPIO_LED_LAN2,
        .active_low = 1,
    }, {
        .name = "rb:green:port3",
        .gpio = RB952_GPIO_LED_LAN3,
        .active_low = 1,
    }, {
        .name = "rb:green:port4",
        .gpio = RB952_GPIO_LED_LAN4,
        .active_low = 1,
    }, {
        .name = "rb:green:port5",
        .gpio = RB952_GPIO_LED_LAN5,
        .active_low = 1,
    },
};

static struct gpio_led rb750r2_leds[] __initdata = {
    {
        .name = "rb:green:act",
        .gpio = RB952_GPIO_LED_ACT,
        .active_low = 1,
    }, {
        .name = "rb:green:port1",
        .gpio = RB952_GPIO_LED_LAN1,
        .active_low = 1,
    }, {
        .name = "rb:green:port2",
        .gpio = RB952_GPIO_LED_LAN2,
        .active_low = 1,
    }, {
        .name = "rb:green:port3",
        .gpio = RB952_GPIO_LED_LAN3,
        .active_low = 1,
    }, {
        .name = "rb:green:port4",
        .gpio = RB952_GPIO_LED_LAN4,
        .active_low = 1,
    }, {
        .name = "rb:green:port5",
        .gpio = RB952_GPIO_LED_LAN5,
        .active_low = 1,
    },
};

static int rb952_spi_cs_gpios[2] = {
    -ENOENT,
    RB952_SSR_STROBE,
};

static struct ath79_spi_platform_data rb952_spi_data __initdata = {
    .bus_num = 0,
    .num_chipselect = 2,
    .cs_gpios = rb952_spi_cs_gpios,
};

static u8 rb952_ssr_initdata[] __initdata = {
    BIT(RB952_SSR_BIT_7) |
    BIT(RB952_SSR_BIT_6) |
    BIT(RB952_SSR_BIT_5) |
    BIT(RB952_SSR_BIT_4) |
    BIT(RB952_SSR_BIT_3) |
    BIT(RB952_SSR_BIT_2) |
    BIT(RB952_SSR_BIT_1)
};

static struct gen_74x164_chip_platform_data rb952_ssr_data = {
    .base = RB952_HC595_GPIO_BASE,
    .num_registers = ARRAY_SIZE(rb952_ssr_initdata),
    .init_data = rb952_ssr_initdata,
};

static struct spi_board_info rb952_spi_info[] = {
    {
        .bus_num = 0,
        .chip_select = 0,
        .max_speed_hz = 25000000,
        .modalias = "m25p80",
        .platform_data = &rb941_spi_flash_data,
    }, {
        .bus_num = 0,
        .chip_select = 1,
        .max_speed_hz = 10000000,
        .modalias = "74x164",
        .platform_data = &rb952_ssr_data,
    }
};

static void __init rb941_setup(void)
{
    const struct rb_info *info;
    //try to get rb_info data
    info = rb_init_info((void *)(KSEG1ADDR(AR71XX_SPI_BASE)), 0x20000);
    if (!info){
        pr_err("%s: Can't get rb_info data from flash!\n", __func__);
        //return -EINVAL; //Not critical ... continue!
    }
    ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_ONLY_MODE);
    ath79_register_m25p80(&rb941_spi_flash_data);
    ath79_register_mdio(0, 0x0);

    /* LAN */
    ath79_init_mac(ath79_eth1_data.mac_addr, ath79_mac_base, 0);
    ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
    ath79_register_eth(1);

    rb941_wlan_init();

    ath79_register_leds_gpio(-1, ARRAY_SIZE(rb941_leds), rb941_leds);
    ath79_register_gpio_keys_polled(-1, RB941_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rb941_gpio_keys),
                                    rb941_gpio_keys);
}

static void __init rb952_setup(void)
{
    const struct rb_info *info;
    //try to get rb_info data
    info = rb_init_info((void *)(KSEG1ADDR(AR71XX_SPI_BASE)), 0x20000);
    if (!info){
        pr_err("%s: Can't get rb_info data from flash!\n", __func__);
        //return -EINVAL; //Not critical ... continue!
    }
    ath79_register_spi(&rb952_spi_data, rb952_spi_info,
                       ARRAY_SIZE(rb952_spi_info));

    ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_ONLY_MODE);
    ath79_register_mdio(0, 0x0);

    /* WAN */
    ath79_switch_data.phy4_mii_en = 1;
    ath79_switch_data.phy_poll_mask = BIT(4);
    ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
    ath79_eth0_data.phy_mask = BIT(4);
    ath79_init_mac(ath79_eth0_data.mac_addr, ath79_mac_base, 0);
    ath79_register_eth(0);

    /* LAN */
    ath79_init_mac(ath79_eth1_data.mac_addr, ath79_mac_base, 1);
    ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
    ath79_register_eth(1);

    gpio_request_one(RB952_GPIO_USB_POWER, GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
                     "USB power");

    ath79_register_usb();
    rb941_wlan_init();

    ath79_register_leds_gpio(-1, ARRAY_SIZE(rb952_leds), rb952_leds);
    ath79_register_gpio_keys_polled(-1, RB941_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rb941_gpio_keys),
                                    rb941_gpio_keys);
}

static void __init rb750r2_setup(void)
{
    ath79_register_spi(&rb952_spi_data, rb952_spi_info,
                       ARRAY_SIZE(rb952_spi_info));

    ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_ONLY_MODE);
    ath79_register_mdio(0, 0x0);

    /* WAN */
    ath79_switch_data.phy4_mii_en = 1;
    ath79_switch_data.phy_poll_mask = BIT(4);
    ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
    ath79_eth0_data.phy_mask = BIT(4);
    ath79_init_mac(ath79_eth0_data.mac_addr, ath79_mac_base, 0);
    ath79_register_eth(0);

    /* LAN */
    ath79_init_mac(ath79_eth1_data.mac_addr, ath79_mac_base, 1);
    ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
    ath79_register_eth(1);

    ath79_register_leds_gpio(-1, ARRAY_SIZE(rb750r2_leds), rb750r2_leds);
    ath79_register_gpio_keys_polled(-1, RB941_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rb941_gpio_keys),
                                    rb941_gpio_keys);
}

MIPS_MACHINE(ATH79_MACH_RB_941, "H951L", "MikroTik RouterBOARD 941-2nD", rb941_setup);
MIPS_MACHINE(ATH79_MACH_RB_952, "952-hb", "MikroTik RouterBOARD 951Ui-2nD", rb952_setup);
MIPS_MACHINE(ATH79_MACH_RB_750R2, "750-hb", "MikroTik RouterBOARD 750-r2", rb750r2_setup);
