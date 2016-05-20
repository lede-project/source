/*
 *  TP-LINK CPE210/220/510/520 board support
 *
 *  Copyright (C) 2014 Matthias Schiffer <mschiffer@universe-factory.net>
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


#define CPE510_GPIO_LED_LAN0	11
#define CPE510_GPIO_LED_LAN1	12
#define CPE510_GPIO_LED_L1	13
#define CPE510_GPIO_LED_L2	14
#define CPE510_GPIO_LED_L3	15
#define CPE510_GPIO_LED_L4	16

#define CPE510_GPIO_EXTERNAL_LNA0	18
#define CPE510_GPIO_EXTERNAL_LNA1	19

#define CPE510_GPIO_BTN_RESET	4

#define CPE510_KEYS_POLL_INTERVAL	20 /* msecs */
#define CPE510_KEYS_DEBOUNCE_INTERVAL	(3 * CPE510_KEYS_POLL_INTERVAL)


static struct gpio_led cpe510_leds_gpio[] __initdata = {
	{
		.name		= "tp-link:green:lan0",
		.gpio		= CPE510_GPIO_LED_LAN0,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:lan1",
		.gpio		= CPE510_GPIO_LED_LAN1,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:link1",
		.gpio		= CPE510_GPIO_LED_L1,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:link2",
		.gpio		= CPE510_GPIO_LED_L2,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:link3",
		.gpio		= CPE510_GPIO_LED_L3,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:link4",
		.gpio		= CPE510_GPIO_LED_L4,
		.active_low	= 1,
	},
};

static struct gpio_keys_button cpe510_gpio_keys[] __initdata = {
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = CPE510_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= CPE510_GPIO_BTN_RESET,
		.active_low	= 1,
	}
};


static void __init cpe510_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f830008);
	u8 *ee = (u8 *) KSEG1ADDR(0x1fff1000);

	/* Disable JTAG, enabling GPIOs 0-3 */
	/* Configure OBS4 line, for GPIO 4*/
	ath79_gpio_function_setup(AR934X_GPIO_FUNC_JTAG_DISABLE,
				  AR934X_GPIO_FUNC_CLK_OBS4_EN);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(cpe510_leds_gpio),
				 cpe510_leds_gpio);

	ath79_register_gpio_keys_polled(1, CPE510_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(cpe510_gpio_keys),
					cpe510_gpio_keys);

	ath79_wmac_set_ext_lna_gpio(0, CPE510_GPIO_EXTERNAL_LNA0);
	ath79_wmac_set_ext_lna_gpio(1, CPE510_GPIO_EXTERNAL_LNA1);

	ath79_register_m25p80(NULL);

	ath79_register_mdio(1, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, mac, 0);
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_register_eth(1);

	ath79_register_wmac(ee, mac);
}

MIPS_MACHINE(ATH79_MACH_CPE510, "CPE510", "TP-LINK CPE210/220/510/520",
	     cpe510_setup);
