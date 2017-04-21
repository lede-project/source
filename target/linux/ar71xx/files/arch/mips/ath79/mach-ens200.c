/*
 * EnGenius ENS200 v1.0.0 board support
 *
 * Copyright (C) 2017 Marty Plummer <netz.kernel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>

#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include "nvram.h"

#define ENS200_GPIO_LED_WLAN1		1 /* on the wireless */
#define ENS200_GPIO_LED_WLAN2		14
#define ENS200_GPIO_LED_WLAN3		15
#define ENS200_GPIO_LED_WLAN4		13

#define ENS200_GPIO_BTN_RESET		1

#define ENS200_KEYS_POLL_INTERVAL	20	/* msecs */
#define ENS200_KEYS_DEBOUNCE_INTERVAL	(3 * ENS200_KEYS_POLL_INTERVAL)

#define ENS200_MAC0_OFFSET		0
#define ENS200_MAC1_OFFSET		6
#define ENS200_CAL_OFFSET		0x1000
#define ENS200_WMAC_OFFSET		0x108c

#define ENS200_CAL_LOCATION		0x1fa00000

static struct gpio_led ens200_leds_gpio[] __initdata = {
	{
		.name		= "engenius:red:wlan2",
		.gpio		= ENS200_GPIO_LED_WLAN2,
		.active_low	= 1,
	}, {
		.name		= "engenius:orange:wlan3",
		.gpio		= ENS200_GPIO_LED_WLAN3,
		.active_low	= 1,
	}, {
		.name		= "engenius:green:wlan4",
		.gpio		= ENS200_GPIO_LED_WLAN4,
		.active_low	= 1,
	}
};

static struct gpio_keys_button ens200_gpio_keys[] __initdata = {
	{
		.desc			= "reset",
		.type			= EV_KEY,
		.code			= KEY_RESTART,
		.debounce_interval	= ENS200_KEYS_DEBOUNCE_INTERVAL,
		.gpio			= ENS200_GPIO_BTN_RESET,
		.active_low		= 1,
	}
};

static struct gpio_led ens200_wmac_leds_gpio[] = {
	{
		.name		= "engenius:green:wlan1",
		.gpio		= ENS200_GPIO_LED_WLAN1,
		.active_low	= 1,
	}
};

static void __init ens200_get_wmac(u8 *wmac_gen_addr, int mac0_art_offset,
				int mac1_art_offset, int wmac_art_offset)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	u8 *eth0_mac_addr = (u8 *) (art + mac0_art_offset);
	u8 *eth1_mac_addr = (u8 *) (art + mac1_art_offset);
	u8 *wlan_mac_addr = (u8 *) (art + wmac_art_offset);

	if ((wlan_mac_addr[0] & wlan_mac_addr[1] & wlan_mac_addr[2] &
	     wlan_mac_addr[3] & wlan_mac_addr[4] & wlan_mac_addr[5]) == 0xff) {
		memcpy(wmac_gen_addr, eth0_mac_addr, 5);
		wmac_gen_addr[5] = max(eth0_mac_addr[5], eth1_mac_addr[5]) + 1;

		if (!wmac_gen_addr[5])
			wmac_gen_addr[5] = 1;
	} else
		memcpy(wmac_gen_addr, wlan_mac_addr, 6);
}

static void __init ens200_net_setup(u8 *wmac_addr)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_register_mdio(0, 0);

	ath79_init_mac(ath79_eth0_data.mac_addr, art+ENS200_MAC0_OFFSET, 0);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.speed = SPEED_100;
	ath79_eth0_data.duplex = DUPLEX_FULL;

	ath79_init_mac(ath79_eth1_data.mac_addr, art+ENS200_MAC1_OFFSET, 0);
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_RMII;
	ath79_eth1_data.phy_mask = 0x10;

	ath79_register_eth(0);
	ath79_register_eth(1);

	ap91_pci_init(art + ENS200_CAL_OFFSET, wmac_addr);
}

static void __init ens200_setup(void)
{
	u8 wlan_mac_addr[ETH_ALEN];

	ath79_gpio_function_enable(AR724X_GPIO_FUNC_JTAG_DISABLE);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(ens200_leds_gpio),
				 ens200_leds_gpio);
	ath79_register_gpio_keys_polled(-1, ENS200_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(ens200_gpio_keys),
					ens200_gpio_keys);

	ap9x_pci_setup_wmac_leds(0, ens200_wmac_leds_gpio,
				 ARRAY_SIZE(ens200_wmac_leds_gpio));

	ath79_register_m25p80(NULL);

	ens200_get_wmac(wlan_mac_addr, ENS200_MAC0_OFFSET,
		     ENS200_MAC1_OFFSET, ENS200_WMAC_OFFSET);

	ens200_net_setup(wlan_mac_addr);
}

MIPS_MACHINE(ATH79_MACH_ENS200, "ENS200", "EnGenius ENS200",
	     ens200_setup);
