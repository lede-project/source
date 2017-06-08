/*
 *  TP-LINK TL-MR22U board support
 *
 *  Copyright (C) 2011 dongyuqi <729650915@qq.com>
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2016-2017 Tomislav Požega <pozega.tomislav@gmail.com>

 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/ar8216_platform.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define TL_MR22U_GPIO_USB_POWER		11
#define TL_MR22U_GPIO_BTN_RESET		12
#define TL_MR22U_GPIO_LED_SYSTEM	13
#define TL_MR22U_GPIO_BTN_SW1		14
#define TL_MR22U_GPIO_BTN_SW2		16
#define TL_MR22U_KEYS_POLL_INTERVAL	20	/* msecs */
#define TL_MR22U_KEYS_DEBOUNCE_INTERVAL	(3 * TL_MR22U_KEYS_POLL_INTERVAL)

static const char *tl_mr22u_part_probes[] = {
	"tp-link",
	NULL,
};

static struct flash_platform_data tl_mr22u_flash_data = {
	.part_probes	= tl_mr22u_part_probes,
};

static struct gpio_led tl_mr22u_leds_gpio[] __initdata = {
	{
		.name		= "tp-link:blue:system",
		.gpio		= TL_MR22U_GPIO_LED_SYSTEM,
		.active_low	= 1,
	},
};

static struct gpio_keys_button tl_mr22u_gpio_keys[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = TL_MR22U_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= TL_MR22U_GPIO_BTN_RESET,
		.active_low	= 1,
	},
	{
		.desc		= "sw1",
		.type		= EV_KEY,
		.code		= BTN_0,
		.debounce_interval = TL_MR22U_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= TL_MR22U_GPIO_BTN_SW1,
		.active_low	= 0,
	},
	{
		.desc		= "sw2",
		.type		= EV_KEY,
		.code		= BTN_1,
		.debounce_interval = TL_MR22U_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= TL_MR22U_GPIO_BTN_SW2,
		.active_low	= 0,
	},
};

static void __init tl_mr22u_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
	u8 *ee = (u8 *) KSEG1ADDR(0x1fff1000);

	/* disable PHY_SWAP and PHY_ADDR_SWAP bits */
	ath79_setup_ar933x_phy4_switch(false, false);

	ath79_register_m25p80(&tl_mr22u_flash_data);
	ath79_register_leds_gpio(-1, ARRAY_SIZE(tl_mr22u_leds_gpio),
				 tl_mr22u_leds_gpio);
	ath79_register_gpio_keys_polled(-1, TL_MR22U_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(tl_mr22u_gpio_keys),
					tl_mr22u_gpio_keys);

	gpio_request_one(TL_MR22U_GPIO_USB_POWER,
			 GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED,
			 "USB power");
	ath79_register_usb();

	ath79_register_mdio(0, 0x0);
	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 0);
	ath79_eth0_data.duplex = DUPLEX_FULL;
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.speed = SPEED_100;
	ath79_eth0_data.phy_mask = BIT(4);
	ath79_register_eth(0);
	ath79_register_wmac(ee, mac);
}

MIPS_MACHINE(ATH79_MACH_TL_MR22U, "TL-MR22U", "TP-LINK TL-MR22U v1",
	     tl_mr22u_setup);
