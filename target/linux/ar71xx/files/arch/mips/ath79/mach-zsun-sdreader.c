/*
 *  ZSUN WiFi SD Card Reader support
 *
 *  Copyright (C) 2015 by Andrzej Surowiec <emeryth@hackerspace.pl>
 *  Based on mach-carambola2.c copyright (C) 2013 Darius Augulis <darius@8devices.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>
#include "common.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define ZSUN_SDREADER_GPIO_LED_SYSTEM		0

#define ZSUN_SDREADER_GPIO_SW_SD		22
#define ZSUN_SDREADER_GPIO_USB_SWITCH		21
#define ZSUN_SDREADER_GPIO_SD_RESET		18

#define ZSUN_SDREADER_MAC0_OFFSET		0x0000
#define ZSUN_SDREADER_MAC1_OFFSET		0x0006
#define ZSUN_SDREADER_CALDATA_OFFSET		0x1000
#define ZSUN_SDREADER_WMAC_MAC_OFFSET		0x1002

#define ZSUN_SDREADER_KEYS_POLL_INTERVAL	20	/* msecs */
#define ZSUN_SDREADER_KEYS_DEBOUNCE_INTERVAL	(3 * ZSUN_SDREADER_KEYS_POLL_INTERVAL)

static struct gpio_led zsun_sdreader_leds_gpio[] __initdata = {
	{
		.name		= "zsun-sdreader:green:system",
		.gpio		= ZSUN_SDREADER_GPIO_LED_SYSTEM,
		.active_low	= 0,
	}
};

static struct gpio_keys_button zsun_sdreader_gpio_keys[] __initdata = {
	{
		.desc           = "SD Card insert switch",
		.type           = EV_KEY,
		.code           = BTN_1,
		.debounce_interval = ZSUN_SDREADER_KEYS_DEBOUNCE_INTERVAL,
		.gpio           = ZSUN_SDREADER_GPIO_SW_SD,
		.active_low	= 1,
	}
};


static void __init zsun_sdreader_common_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_register_m25p80(NULL);
	ath79_register_wmac(art + ZSUN_SDREADER_CALDATA_OFFSET,
			    art + ZSUN_SDREADER_WMAC_MAC_OFFSET);

	ath79_setup_ar933x_phy4_switch(true, true);

	//ath79_init_mac(ath79_eth0_data.mac_addr, art + ZSUN_SDREADER_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, art + ZSUN_SDREADER_MAC1_OFFSET, 0);


	ath79_register_mdio(0, 0x0);

	//ath79_register_eth(0);
	ath79_register_eth(1);
}

static void __init zsun_sdreader_setup(void)
{
	zsun_sdreader_common_setup();

	ath79_gpio_function_disable(AR724X_GPIO_FUNC_ETH_SWITCH_LED0_EN |
				AR724X_GPIO_FUNC_ETH_SWITCH_LED1_EN |
				AR724X_GPIO_FUNC_ETH_SWITCH_LED2_EN |
				AR724X_GPIO_FUNC_ETH_SWITCH_LED3_EN |
				AR724X_GPIO_FUNC_ETH_SWITCH_LED4_EN);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(zsun_sdreader_leds_gpio),
				 zsun_sdreader_leds_gpio);
	ath79_register_gpio_keys_polled(-1, ZSUN_SDREADER_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(zsun_sdreader_gpio_keys),
					zsun_sdreader_gpio_keys);
	gpio_request_one(ZSUN_SDREADER_GPIO_USB_SWITCH, GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED, "USB switch");
	gpio_request_one(ZSUN_SDREADER_GPIO_SD_RESET, GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_CHANGEABLE, "SD reader reset");

	ath79_register_usb();
}

MIPS_MACHINE(ATH79_MACH_ZSUN_SDREADER, "ZSUN-SDREADER", "ZSUN WiFi SD Card Reader",
		zsun_sdreader_setup);
