/*
 * Wallys DR344 board support
 *
 * Copyright (c) 2011 Qualcomm Atheros
 * Copyright (c) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (c) 2015 Philippe Duchein <wireless-dev@duchein.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <linux/ar8216_platform.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-gpio-buttons.h"
#include "dev-eth.h"
#include "dev-usb.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define DR344_GPIO_LED_SIG1	15
#define DR344_GPIO_LED_SIG2	11
#define DR344_GPIO_LED_SIG3	12
#define DR344_GPIO_LED_SIG4	13
#define DR344_GPIO_EXTERNAL_LNA0       18
#define DR344_GPIO_EXTERNAL_LNA1       19
#define DR344_GPIO_LED_STATUS	14

#define DR344_GPIO_BTN_RESET	12

#define DR344_KEYS_POLL_INTERVAL	20	/* msecs */
#define DR344_KEYS_DEBOUNCE_INTERVAL	(3 * DR344_KEYS_POLL_INTERVAL)

#define DR344_MAC0_OFFSET		0
#define DR344_MAC1_OFFSET		8
#define DR344_WMAC_CALDATA_OFFSET	0x1000
#define DR344_PCIE_CALDATA_OFFSET	0x5000

static struct gpio_led dr344_leds_gpio[] __initdata = {
	{
		.name		= "dr344:green:status",
		.gpio		= DR344_GPIO_LED_STATUS,
		.active_low	= 1,
	},
	{
		.name		= "dr344:red:sig1",
		.gpio		= DR344_GPIO_LED_SIG1,
		.active_low	= 1,
	},
	{
		.name		= "dr344:yellow:sig2",
		.gpio		= DR344_GPIO_LED_SIG2,
		.active_low	= 1,
	},
	{
		.name		= "dr344:green:sig3",
		.gpio		= DR344_GPIO_LED_SIG3,
		.active_low	= 1,
	},
	{
		.name		= "dr344:green:sig4",
		.gpio		= DR344_GPIO_LED_SIG4,
		.active_low	= 1,
	}
};

static struct gpio_keys_button dr344_gpio_keys[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = DR344_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= DR344_GPIO_BTN_RESET,
		.active_low	= 1,
	},
};

static struct ar8327_pad_cfg dr344_ar8327_pad0_cfg = {
	.mode = AR8327_PAD_MAC_RGMII,
	.txclk_delay_en = true,
	.rxclk_delay_en = true,
	.txclk_delay_sel = AR8327_CLK_DELAY_SEL1,
	.rxclk_delay_sel = AR8327_CLK_DELAY_SEL2,
};

static struct ar8327_led_cfg dr344_ar8327_led_cfg = {
	.led_ctrl0 = 0x00000000,
	.led_ctrl1 = 0xc737c737,
	.led_ctrl2 = 0x00000000,
	.led_ctrl3 = 0x00c30c00,
	.open_drain = true,
};

static struct ar8327_platform_data dr344_ar8327_data = {
	.pad0_cfg = &dr344_ar8327_pad0_cfg,
	.port0_cfg = {
		.force_link = 1,
		.speed = AR8327_PORT_SPEED_1000,
		.duplex = 1,
		.txpause = 1,
		.rxpause = 1,
	},
	.led_cfg = &dr344_ar8327_led_cfg,
};

static struct mdio_board_info dr344_mdio0_info[] = {
	{
		.bus_id = "ag71xx-mdio.0",
		.phy_addr = 0,
		.platform_data = &dr344_ar8327_data,
	},
};

static void __init dr344_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	u8 *mac = (u8 *) KSEG1ADDR(0x1f03f810);

	ath79_register_m25p80(NULL);
	ath79_register_leds_gpio(-1, ARRAY_SIZE(dr344_leds_gpio),
				 dr344_leds_gpio);
	ath79_register_gpio_keys_polled(-1, DR344_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(dr344_gpio_keys),
					dr344_gpio_keys);

	ath79_register_usb();

	ath79_wmac_set_ext_lna_gpio(0, DR344_GPIO_EXTERNAL_LNA0);

	ath79_wmac_set_ext_lna_gpio(1, DR344_GPIO_EXTERNAL_LNA1);

	ath79_register_wmac(art + DR344_WMAC_CALDATA_OFFSET, NULL);

	ath79_register_pci();

	mdiobus_register_board_info(dr344_mdio0_info,
					ARRAY_SIZE(dr344_mdio0_info));

	ath79_register_mdio(1, 0x0);
	ath79_register_mdio(0, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, mac + DR344_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, mac + DR344_MAC1_OFFSET, 0);

	ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_RGMII_GMAC0 |
				   AR934X_ETH_CFG_SW_ONLY_MODE);

	/* GMAC0 is connected to an AR8327 switch */
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = BIT(0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_eth0_pll_data.pll_1000 = 0x0e000000;
	ath79_eth0_pll_data.pll_100 = 0x0101;
	ath79_eth0_pll_data.pll_10 = 0x1313;

	/* GMAC1 is connected to the internal switch */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_eth1_data.speed = SPEED_1000;
	ath79_eth1_data.duplex = DUPLEX_FULL;

	ath79_register_eth(0);
	ath79_register_eth(1);
}

MIPS_MACHINE(ATH79_MACH_DR344, "DR344", "Wallys DR344", dr344_setup);
