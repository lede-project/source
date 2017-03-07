/*
 *  Mifi board support
 *
 *  Copyright (C) 2011 dongyuqi <729650915@qq.com>
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2013 alzhao <alzhao@gmail.com>
 *  Copyright (C) 2014 Michel Stempin <michel.stempin@wanadoo.fr>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
*/

#include <linux/gpio.h>

#include <asm/mach-ath79/ath79.h>

#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define GL_MIFI_GPIO_LED_WAN		27
#define GL_MIFI_GPIO_LED_LAN		16
#define GL_MIFI_GPIO_LED_WLAN		1
#define GL_MIFI_GPIO_LED_NET		0
#define GL_MIFI_GPIO_LED_3GCONTROL	7

#define GL_MIFI_GPIO_BTN_RESET    11

#define GL_MIFI_KEYS_POLL_INTERVAL	20	/* msecs */
#define GL_MIFI_KEYS_DEBOUNCE_INTERVAL	(3 * GL_MIFI_KEYS_POLL_INTERVAL)

#define GL_MIFI_MAC0_OFFSET	    0x0000
#define GL_MIFI_MAC1_OFFSET	    0x0000
#define GL_MIFI_CALDATA_OFFSET	0x1000
#define GL_MIFI_WMAC_MAC_OFFSET	0x0000

static struct gpio_led gl_mifi_leds_gpio[] __initdata = {
	{
		.name = "gl-mifi:greeen:wan",
		.gpio = GL_MIFI_GPIO_LED_WAN,
		.active_low = 0,
	},
	{
		.name = "gl-mifi:green:lan",
		.gpio = GL_MIFI_GPIO_LED_LAN,
		.active_low = 0,
	},
	{
		.name = "gl-mifi:green:wlan",
		.gpio = GL_MIFI_GPIO_LED_WLAN,
		.active_low = 0,
	},
	{
		.name = "gl-mifi:green:net",
		.gpio = GL_MIFI_GPIO_LED_NET,
		.active_low = 0,
	},
	{
		.name = "gl-mifi:green:3gcontrol",
		.gpio = GL_MIFI_GPIO_LED_3GCONTROL,
		.active_low = 0,
	}
};

static struct gpio_keys_button gl_mifi_gpio_keys[] __initdata = {
	{
		.desc = "reset",
		.type = EV_KEY,
		.code = KEY_RESTART,
		.debounce_interval = GL_MIFI_KEYS_DEBOUNCE_INTERVAL,
		.gpio = GL_MIFI_GPIO_BTN_RESET,
		.active_low = 0,
	},
};

static void __init gl_mifi_setup(void)
{

	/* ART base address */
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	/* disable PHY_SWAP and PHY_ADDR_SWAP bits */
	ath79_setup_ar933x_phy4_switch(false, false);

	/* register flash. */
	ath79_register_m25p80(NULL);

	/* register gpio LEDs and keys */
	ath79_register_leds_gpio(-1, ARRAY_SIZE(gl_mifi_leds_gpio),
				 gl_mifi_leds_gpio);
	ath79_register_gpio_keys_polled(-1, GL_MIFI_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(gl_mifi_gpio_keys),
					gl_mifi_gpio_keys);

	/* enable usb */
	ath79_register_usb();

	/* register eth0 as WAN, eth1 as LAN */
	ath79_init_mac(ath79_eth0_data.mac_addr, art+GL_MIFI_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, art+GL_MIFI_MAC1_OFFSET, 0);
	ath79_register_mdio(0, 0x0);
	ath79_register_eth(0);
	ath79_register_eth(1);

	/* register wireless mac with cal data */
	ath79_register_wmac(art + GL_MIFI_CALDATA_OFFSET, art + GL_MIFI_WMAC_MAC_OFFSET);
}

MIPS_MACHINE(ATH79_MACH_GL_MIFI, "GL-MIFI", "GL-MIFI",gl_mifi_setup);
