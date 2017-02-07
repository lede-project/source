/*
 *  MikroTik RouterBOARD hAP-x based boards support
 *
 *  Copyright (C) 2015-2016 David Hutchison <dhutchison@bluemesh.net>
 *  Copyright (C) 2016-2017 Sergey Sergeev <adron@yapic.net>
 *  Copyright (C) 2017 Samorukov Alexey <samm@os2.kiev.ua>
 *  Copyright (C) 2017 Thibaut VARENE <varenet at parisc-linux.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/export.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/routerboot.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/74x164.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/prom.h>
#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-spi.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include "routerboot.h"

/* common RB hAP series reset gpio */
#define RBHAPX_GPIO_BTN_RESET    16
#define RBHAPX_KEYS_POLL_INTERVAL 20 /* msecs */
#define RBHAPX_KEYS_DEBOUNCE_INTERVAL (3 * RBHAPX_KEYS_POLL_INTERVAL)


/* RB 941-2nD gpios */
#define RB941_GPIO_LED_ACT      14

/* RB 951Ui-2nD gpios */
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

/* RB mAP L-2nD gpios */
#define RBMAPL_GPIO_LED_POWER   17
#define RBMAPL_GPIO_LED_USER    14
#define RBMAPL_GPIO_LED_ETH     4
#define RBMAPL_GPIO_LED_WLAN    11

#define RB_ROUTERBOOT_OFFSET    0x0000
#define RB_BIOS_SIZE            0x1000
#define RB_SOFT_CFG_SIZE        0x1000
#define RB_KERNEL_OFFSET        0xa00000
#define RB_KERNEL_SIZE          MTDPART_SIZ_FULL

enum {
  ROUTERBOOT_PART,
  HARD_CONFIG_PART,
  BIOS_PART,
  ROUTERBOOT2_PART,
  SOFT_CONFIG_PART,
  ROOTFS_PART,
  KERNEL_PART,
  TOTAL_PARTS_COUNT
};

void __init rbhapx_wlan_init(int wmac_offset)
{
    char *art_buf;
    u8 wlan_mac[ETH_ALEN];

    art_buf = rb_get_wlan_data();
    if (art_buf == NULL)
            return;

    ath79_init_mac(wlan_mac, ath79_mac_base, wmac_offset);
    ath79_register_wmac(art_buf + 0x1000, wlan_mac);

    kfree(art_buf);
}

static struct mtd_partition rbhapx_spi_partitions[TOTAL_PARTS_COUNT];
/* calculate the partitions offsets and size based on initial
 * parsing. Kernel is fixed at RB_KERNEL_OFFSET so the rootfs
 * partition adjusts accordingly */
static void __init rbhapx_init_partitions(const struct rb_info *info)
{
  struct mtd_partition *p = rbhapx_spi_partitions;
  memset(p, 0x0, sizeof(rbhapx_spi_partitions));
  p[ROUTERBOOT_PART].name = "routerboot",
  p[ROUTERBOOT_PART].offset = RB_ROUTERBOOT_OFFSET;
  p[ROUTERBOOT_PART].size = info->hard_cfg_offs;
  p[ROUTERBOOT_PART].mask_flags = MTD_WRITEABLE;
  p[HARD_CONFIG_PART].name = "hard_config";
  p[HARD_CONFIG_PART].offset = info->hard_cfg_offs;
  p[HARD_CONFIG_PART].size = info->hard_cfg_size;
  p[HARD_CONFIG_PART].mask_flags = MTD_WRITEABLE;
  p[BIOS_PART].name = "bios";
  p[BIOS_PART].offset = info->hard_cfg_offs + info->hard_cfg_size;
  p[BIOS_PART].size = RB_BIOS_SIZE;
  p[BIOS_PART].mask_flags = MTD_WRITEABLE;
  p[ROUTERBOOT2_PART].name = "routerboot2";
  p[ROUTERBOOT2_PART].offset = p[BIOS_PART].offset + RB_BIOS_SIZE;
  p[ROUTERBOOT2_PART].size = info->soft_cfg_offs - p[ROUTERBOOT2_PART].offset;
  p[ROUTERBOOT2_PART].mask_flags = MTD_WRITEABLE;
  p[SOFT_CONFIG_PART].name = "soft_config";
  p[SOFT_CONFIG_PART].offset = info->soft_cfg_offs;
  p[SOFT_CONFIG_PART].size = RB_SOFT_CFG_SIZE;
  p[ROOTFS_PART].name = "rootfs";
  p[ROOTFS_PART].offset = p[SOFT_CONFIG_PART].offset + RB_SOFT_CFG_SIZE;
  p[ROOTFS_PART].size = RB_KERNEL_OFFSET - p[ROOTFS_PART].offset;
  p[KERNEL_PART].name = "kernel";
  p[KERNEL_PART].offset = RB_KERNEL_OFFSET;
  p[KERNEL_PART].size = RB_KERNEL_SIZE;
}

static struct flash_platform_data rbhapx_spi_flash_data = {
    .parts          = rbhapx_spi_partitions,
    .nr_parts       = ARRAY_SIZE(rbhapx_spi_partitions),
};

static struct gpio_led rb941_leds[] __initdata = {
    {
        .name = "rb:green:act",
        .gpio = RB941_GPIO_LED_ACT,
        .active_low = 1,
    },
};

static struct gpio_keys_button rbhapx_gpio_keys[] __initdata = {
    {
        .desc = "Reset button",
        .type = EV_KEY,
        .code = KEY_RESTART,
        .debounce_interval = RBHAPX_KEYS_DEBOUNCE_INTERVAL,
        .gpio = RBHAPX_GPIO_BTN_RESET,
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

static struct gpio_led rbmapl_leds[] __initdata = {
    {
        .name = "rb:green:power",
        .gpio = RBMAPL_GPIO_LED_POWER,
        .active_low = 0,
        .default_state = LEDS_GPIO_DEFSTATE_ON,
    },
    {
        .name = "rb:green:user",
        .gpio = RBMAPL_GPIO_LED_USER,
        .active_low = 0,
    },
    {
        .name = "rb:green:eth",
        .gpio = RBMAPL_GPIO_LED_ETH,
        .active_low = 0,
    },
    {
        .name = "rb:green:wlan",
        .gpio = RBMAPL_GPIO_LED_WLAN,
        .active_low = 0,
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
        .platform_data = &rbhapx_spi_flash_data,
    }, {
        .bus_num = 0,
        .chip_select = 1,
        .max_speed_hz = 10000000,
        .modalias = "74x164",
        .platform_data = &rb952_ssr_data,
    }
};

static int __init rbhapx_setup_common(void)
{
  const struct rb_info *info;
  char buf[64];
  info = rb_init_info((void *)(KSEG1ADDR(AR71XX_SPI_BASE)), 0x20000);
  if (!info)
    return -ENODEV;
  scnprintf(buf, sizeof(buf), "MikroTik %s",
    (info->board_name) ? info->board_name : "");
  mips_set_machine_name(buf);
  /* fix partitions based on flash parsing */
  rbhapx_init_partitions(info);
  return 0;
}

static void __init rb941_setup(void)
{
    rbhapx_setup_common();

    ath79_register_m25p80(&rbhapx_spi_flash_data);
    ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_ONLY_MODE);
    ath79_register_mdio(0, 0x0);

    /* LAN */
    ath79_init_mac(ath79_eth1_data.mac_addr, ath79_mac_base, 0);
    ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
    ath79_register_eth(1);

    rbhapx_wlan_init(4);

    ath79_register_leds_gpio(-1, ARRAY_SIZE(rb941_leds), rb941_leds);
    ath79_register_gpio_keys_polled(-1, RBHAPX_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rbhapx_gpio_keys),
                                    rbhapx_gpio_keys);
}

static void __init rb952_setup(void)
{
    rbhapx_setup_common();
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
    rbhapx_wlan_init(5);

    ath79_register_leds_gpio(-1, ARRAY_SIZE(rb952_leds), rb952_leds);
    ath79_register_gpio_keys_polled(-1, RBHAPX_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rbhapx_gpio_keys),
                                    rbhapx_gpio_keys);
}

static void __init rb750r2_setup(void)
{
    rbhapx_setup_common();

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
    ath79_register_gpio_keys_polled(-1, RBHAPX_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rbhapx_gpio_keys),
                                    rbhapx_gpio_keys);
}

/* The mAP L-2nD (mAP lite) has a single ethernet port, connected to PHY0.
 * Trying to use GMAC0 in direct mode was unsucessful, so we're
 * using SW_ONLY_MODE, which connects PHY0 to MAC1 on the internal
 * switch, which is connected to GMAC1 on the SoC. GMAC0 is unused. */
static void __init rbmapl_setup(void){
    rbhapx_setup_common();

    ath79_register_m25p80(&rbhapx_spi_flash_data);

    /* set the SoC to SW_ONLY_MODE, which connects all PHYs
     * to the internal switch */
    ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_ONLY_MODE);
    ath79_register_mdio(0, 0x0);

    /* GMAC0 is left unused */
    /* GMAC1 is connected to MAC0 on the internal switch */
    ath79_init_mac(ath79_eth1_data.mac_addr, ath79_mac_base, 0);
    ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
    ath79_register_eth(1);

    rbhapx_wlan_init(1); /* WLAN MAC is HW MAC + 1 */

    ath79_register_leds_gpio(-1, ARRAY_SIZE(rbmapl_leds), rbmapl_leds);
    ath79_register_gpio_keys_polled(-1, RBHAPX_KEYS_POLL_INTERVAL,
                                    ARRAY_SIZE(rbhapx_gpio_keys),
                                    rbhapx_gpio_keys);

    /* Clear internal multiplexing */
    ath79_gpio_output_select(RBMAPL_GPIO_LED_ETH, 0);
    ath79_gpio_output_select(RBMAPL_GPIO_LED_POWER, 0);
}

MIPS_MACHINE_NONAME(ATH79_MACH_RB_941, "H951L", rb941_setup);
MIPS_MACHINE_NONAME(ATH79_MACH_RB_952, "952-hb", rb952_setup);
MIPS_MACHINE_NONAME(ATH79_MACH_RB_750R2, "750-hb", rb750r2_setup);
MIPS_MACHINE_NONAME(ATH79_MACH_RB_MAPL, "map-hb", rbmapl_setup);
