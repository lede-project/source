/*
 *  TP-LINK TL-WR886N v5
 *
 *  Copyright (C) Weijie Gao <hackpascal@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/platform_device.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define TL_WR886NV5_GPIO_LED_WAN	2
#define TL_WR886NV5_GPIO_LED_LAN1	4
#define TL_WR886NV5_GPIO_LED_LAN2	5
#define TL_WR886NV5_GPIO_LED_LAN3	6
#define TL_WR886NV5_GPIO_LED_LAN4	7
#define TL_WR886NV5_GPIO_LED_SYS	18

#define TL_WR886NV5_GPIO_BTN_RESET	1

#define TL_WR886NV5_KEYS_POLL_INTERVAL	20	/* msecs */
#define TL_WR886NV5_KEYS_DEBOUNCE_INTERVAL (3 * TL_WR886NV5_KEYS_POLL_INTERVAL)

static const char *tl_wr886n_v5_part_probes[] = {
	"tp-link",
	NULL,
};

static struct flash_platform_data tl_wr886n_v5_flash_data = {
	.part_probes	= tl_wr886n_v5_part_probes,
};

static struct gpio_led tl_wr886n_v5_leds_gpio[] __initdata = {
	{
		.name		= "tp-link:green:lan1",
		.gpio		= TL_WR886NV5_GPIO_LED_LAN1,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:lan2",
		.gpio		= TL_WR886NV5_GPIO_LED_LAN2,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:lan3",
		.gpio		= TL_WR886NV5_GPIO_LED_LAN3,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:lan4",
		.gpio		= TL_WR886NV5_GPIO_LED_LAN4,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:wan",
		.gpio		= TL_WR886NV5_GPIO_LED_WAN,
		.active_low	= 1,
	}, {
		.name		= "tp-link:white:status",
		.gpio		= TL_WR886NV5_GPIO_LED_SYS,
		.active_low	= 1,
	},
};

static struct gpio_keys_button tl_wr886n_v5_gpio_keys[] __initdata = {
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = TL_WR886NV5_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= TL_WR886NV5_GPIO_BTN_RESET,
		.active_low	= 1,
	}
};


static void __init tl_wr886n_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
	u8 *ee = (u8 *) KSEG1ADDR(0x1fff1000);

	ath79_register_m25p80(&tl_wr886n_v5_flash_data);

	ath79_register_mdio(0, 0x0);

	/* LAN */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_eth1_data.duplex = DUPLEX_FULL;
	ath79_eth1_data.speed = SPEED_1000;
	ath79_init_mac(ath79_eth1_data.mac_addr, mac, 0);
	ath79_register_eth(1);

	/* WAN */
	ath79_switch_data.phy4_mii_en = 1;
	ath79_switch_data.phy_poll_mask = BIT(4);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.phy_mask = BIT(4);
	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 1);
	ath79_register_eth(0);

	ath79_register_wmac(ee, mac);
}

static void __init tl_wr886n_v5_setup(void)
{
	tl_wr886n_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(tl_wr886n_v5_leds_gpio),
				 tl_wr886n_v5_leds_gpio);

	ath79_register_gpio_keys_polled(1, TL_WR886NV5_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(tl_wr886n_v5_gpio_keys),
					tl_wr886n_v5_gpio_keys);
}

MIPS_MACHINE(ATH79_MACH_TL_WR886N_V5, "TL-WR886N-v5", "TP-LINK TL-WR886N/ND v5",
	     tl_wr886n_v5_setup);
