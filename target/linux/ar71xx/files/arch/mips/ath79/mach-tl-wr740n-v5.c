/*
 *  TP-LINK TL-WR741N v4 China/TL-WR740N v5/TL-WR340G+ v7 board support
 *  TP-LINK TL-WR742N v5/TL-WR745N v2 board support
 *
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2015 Weijie Gao <hackpascal@gmail.com>
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
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define TL_WR740NV5_GPIO_BTN_RESET	11

#define TL_WR740NV5_GPIO_LED_WLAN	0
#define TL_WR740NV5_GPIO_LED_QSS	26
#define TL_WR740NV5_GPIO_LED_WAN	17
#define TL_WR740NV5_GPIO_LED_LAN1	13
#define TL_WR740NV5_GPIO_LED_LAN2	14
#define TL_WR740NV5_GPIO_LED_LAN3	15
#define TL_WR740NV5_GPIO_LED_LAN4	16
#define TL_WR740NV5_GPIO_LED_SYSTEM	27

#define TL_WR740NV5_KEYS_POLL_INTERVAL	20	/* msecs */
#define TL_WR740NV5_KEYS_DEBOUNCE_INTERVAL (3 * TL_WR740NV5_KEYS_POLL_INTERVAL)

static const char *tl_wr740nv5_part_probes[] = {
	"tp-link",
	NULL,
};

static struct flash_platform_data tl_wr740nv5_flash_data = {
	.part_probes	= tl_wr740nv5_part_probes,
};

static struct gpio_led tl_wr740nv5_leds_gpio[] __initdata = {
	{
		.name		= "tp-link:green:lan1",
		.gpio		= TL_WR740NV5_GPIO_LED_LAN1,
		.active_low	= 0,
	}, {
		.name		= "tp-link:green:lan2",
		.gpio		= TL_WR740NV5_GPIO_LED_LAN2,
		.active_low	= 0,
	}, {
		.name		= "tp-link:green:lan3",
		.gpio		= TL_WR740NV5_GPIO_LED_LAN3,
		.active_low	= 0,
	}, {
		.name		= "tp-link:green:lan4",
		.gpio		= TL_WR740NV5_GPIO_LED_LAN4,
		.active_low	= 0,
	}, {
		.name		= "tp-link:green:wan",
		.gpio		= TL_WR740NV5_GPIO_LED_WAN,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:system",
		.gpio		= TL_WR740NV5_GPIO_LED_SYSTEM,
		.active_low	= 1,
	}, {
		.name		= "tp-link:green:qss",
		.gpio		= TL_WR740NV5_GPIO_LED_QSS,
		.active_low	= 0,
	}, {
		.name		= "tp-link:green:wlan",
		.gpio		= TL_WR740NV5_GPIO_LED_WLAN,
		.active_low	= 0,
	}
};

static struct gpio_keys_button tl_wr740nv5_gpio_keys[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = TL_WR740NV5_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= TL_WR740NV5_GPIO_BTN_RESET,
		.active_low	= 0,
	}
};

static void __init tl_ap121_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
	u8 *ee = (u8 *) KSEG1ADDR(0x1fff1000);

	ath79_setup_ar933x_phy4_switch(false, false);

	ath79_gpio_function_disable(AR933X_GPIO_FUNC_ETH_SWITCH_LED0_EN |
				    AR933X_GPIO_FUNC_ETH_SWITCH_LED1_EN |
				    AR933X_GPIO_FUNC_ETH_SWITCH_LED2_EN |
				    AR933X_GPIO_FUNC_ETH_SWITCH_LED3_EN |
				    AR933X_GPIO_FUNC_ETH_SWITCH_LED4_EN);

	ath79_register_gpio_keys_polled(1, TL_WR740NV5_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(tl_wr740nv5_gpio_keys),
					tl_wr740nv5_gpio_keys);

	ath79_register_m25p80(&tl_wr740nv5_flash_data);
	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 1);
	ath79_init_mac(ath79_eth1_data.mac_addr, mac, -1);

	ath79_register_mdio(0, 0x0);
	ath79_register_eth(1);
	ath79_register_eth(0);

	ath79_register_wmac(ee, mac);

	ath79_register_usb();
}

static void __init tl_wr740nv5_setup(void)
{
	tl_ap121_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(tl_wr740nv5_leds_gpio),
				 tl_wr740nv5_leds_gpio);
}

MIPS_MACHINE(ATH79_MACH_TL_WR740N_V5, "TL-WR740N-v5",
	     "TP-LINK TL-WR740N v5", tl_wr740nv5_setup);

static void __init tl_wr742nv5_setup(void)
{
	tl_ap121_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(tl_wr740nv5_leds_gpio) - 2,
				 tl_wr740nv5_leds_gpio);
}

MIPS_MACHINE(ATH79_MACH_TL_WR742N_V5, "TL-WR742N-v5",
	     "TP-LINK TL-WR742N v5", tl_wr742nv5_setup);

