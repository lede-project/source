
/*
 * TP-LINK EAP245 v1 board support
 * Based on TP-Link GPL code for Linux 3.3.8 (hack of AP152 reference board)
 *
 * Copyright (c) 2014 The Linux Foundation. All rights reserved.
 * Copyright (c) 2012 Gabor Juhos <juhosg@openwrt.org>
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
 * EAP245 v1 board support wifi AC1750 and gigabit LAN:
 *  - QCA9563-AL3A MIPS 74kc and 2.4GHz wifi
 *  - QCA9880-BR4A 5 GHz wifi ath10k
 *  - AR8033-AL1A 1 gigabit lan port
 *  - 25Q128CSIG SPI NOR
 */

#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <asm/mach-ath79/ar71xx_regs.h>
#include <linux/delay.h>
#include <linux/platform_data/phy-at803x.h>
#include <linux/platform_data/mdio-gpio.h>
#include <ath79.h>

#include "common.h"
#include "dev-m25p80.h"
#include "machtypes.h"
#include "pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"

/* GPIO
 * GPIO4 reset the boad when direction is changed, be careful
 * GPIO5 is a master switch for all leds */
#define EAP245_V1_GPIO_LED_RED			1
#define EAP245_V1_GPIO_LED_YEL			9
#define EAP245_V1_GPIO_LED_GRN			7
#define EAP245_V1_GPIO_LED_ALL			5
#define EAP245_V1_GPIO_BTN_RESET		2
#define EAP245_V1_KEYS_POLL_INTERVAL		20 /* msecs */
#define EAP245_V1_KEYS_DEBOUNCE_INTERVAL	(3 * EAP245_V1_KEYS_POLL_INTERVAL)

#define EAP245_V1_GPIO_SMI_MDC			8
#define EAP245_V1_GPIO_SMI_MDIO			10
#define EAP245_V1_LAN_PHYADDR			4

static struct gpio_led eap245_v1_leds_gpio[] __initdata = {
	{
		.name = "eap245-v1:red:system",
		.gpio = EAP245_V1_GPIO_LED_RED,
	}, {
		.name = "eap245-v1:yellow:system",
		.gpio = EAP245_V1_GPIO_LED_YEL,
	}, {
		.name = "eap245-v1:green:system",
		.gpio = EAP245_V1_GPIO_LED_GRN,
	},
};

static struct gpio_keys_button eap245_v1_gpio_keys[] __initdata = {
	{
		.desc = "Reset button",
		.type = EV_KEY,
		.code = KEY_RESTART,
		.debounce_interval = EAP245_V1_KEYS_DEBOUNCE_INTERVAL,
		.gpio = EAP245_V1_GPIO_BTN_RESET,
		.active_low = 1,
	},
};

static struct at803x_platform_data eap245_v1_ar8033_data = {
	.disable_smarteee = 0,
	.enable_rgmii_rx_delay = 1,
	.enable_rgmii_tx_delay = 0,
	.fixup_rgmii_tx_delay = 1,
};

static struct mdio_gpio_platform_data eap245_v1_mdio = {
	.mdc = EAP245_V1_GPIO_SMI_MDC,
	.mdio = EAP245_V1_GPIO_SMI_MDIO,
	.phy_mask = ~BIT(EAP245_V1_LAN_PHYADDR),
};

static struct platform_device eap245_v1_phy_device = {
	.name = "mdio-gpio",
	.id = 0,
	.dev = {
		.platform_data = &eap245_v1_mdio, &eap245_v1_ar8033_data
	},
};

/* MDIO initialization code from TP-Link GPL code */
static void __init athrs_sgmii_res_cal(void __iomem * gmac_sgmi_base)
{
	u32 t, reversed_sgmii_value, i, vco, startValue = 0, endValue = 0;

	ath79_pll_wr(QCA956X_PLL_SGMII_SERDES_REG,
			QCA956X_PLL_SGMII_SERDES_EN_LOCK_DETECT |
			QCA956X_PLL_SGMII_SERDES_EN_PLL);

	t = __raw_readl(gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);
	vco = t & (QCA956X_SGMII_SERDES_VCO_FAST | QCA956X_SGMII_SERDES_VCO_SLOW);

	/* set resistor Calibration from 0000 -> 1111 */
	for (i = 0; i < 0x10; i++) {
		t = __raw_readl(gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);
		t &= ~(QCA956X_SGMII_SERDES_RES_CALIBRATION_MASK << QCA956X_SGMII_SERDES_RES_CALIBRATION_SHIFT);
		t |= i << QCA956X_SGMII_SERDES_RES_CALIBRATION_SHIFT;
		__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);

		udelay(50);

		t = __raw_readl(gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);
		if (vco != (t & (QCA956X_SGMII_SERDES_VCO_FAST | QCA956X_SGMII_SERDES_VCO_SLOW))) {
			if (startValue == 0) {
				startValue = endValue = i;
			} else {
				endValue = i;
			}
		}
		vco = t & (QCA956X_SGMII_SERDES_VCO_FAST | QCA956X_SGMII_SERDES_VCO_SLOW);
	}

	if (startValue == 0) {
		/* No boundary found, use middle value for resistor calibration value */
		reversed_sgmii_value = 0x7;
	} else {
		/* get resistor calibration from the middle of boundary */
		reversed_sgmii_value = (startValue + endValue) / 2;
	}

	t = __raw_readl(gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);
	t &= ~(QCA956X_SGMII_SERDES_RES_CALIBRATION_MASK << QCA956X_SGMII_SERDES_RES_CALIBRATION_SHIFT);
	t |= (reversed_sgmii_value & QCA956X_SGMII_SERDES_RES_CALIBRATION_MASK) << QCA956X_SGMII_SERDES_RES_CALIBRATION_SHIFT;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);

	ath79_pll_wr(QCA956X_PLL_SGMII_SERDES_REG, QCA956X_PLL_SGMII_SERDES_EN_LOCK_DETECT | QCA956X_PLL_SGMII_SERDES_EN_PLL);

	t = __raw_readl(gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);

	/* missing in original code, clear before setting */
	t &= ~(QCA956X_SGMII_SERDES_CDR_BW_MASK << QCA956X_SGMII_SERDES_CDR_BW_SHIFT |
		QCA956X_SGMII_SERDES_TX_DR_CTRL_MASK << QCA956X_SGMII_SERDES_TX_DR_CTRL_SHIFT |
		QCA956X_SGMII_SERDES_VCO_REG_MASK << QCA956X_SGMII_SERDES_VCO_REG_SHIFT);

	t |= (3 << QCA956X_SGMII_SERDES_CDR_BW_SHIFT) |
		(1 << QCA956X_SGMII_SERDES_TX_DR_CTRL_SHIFT) |
		QCA956X_SGMII_SERDES_PLL_BW |
		QCA956X_SGMII_SERDES_EN_SIGNAL_DETECT |
		QCA956X_SGMII_SERDES_FIBER_SDO |
		(3 << QCA956X_SGMII_SERDES_VCO_REG_SHIFT);
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_SERDES_REG);

	ath79_device_reset_clear(QCA956X_RESET_SGMII_ANALOG);
	udelay(25);
	ath79_device_reset_clear(QCA956X_RESET_SGMII);
	while (!(__raw_readl(gmac_sgmi_base + QCA956X_SGMII_SERDES_REG) &
		QCA956X_SGMII_SERDES_LOCK_DETECT_STATUS)) ;
}

#define EAP245_V1_SGMII_LINK_WAR_MAX_TRY	10
static void __init athrs_sgmii_set_up(void __iomem * gmac_sgmi_base, void __iomem * ge0_base)
{
	u32 status, count = 0, t;

	__raw_writel(0x3f, ge0_base + 0);
	udelay(10);
	__raw_writel(0x3c041, gmac_sgmi_base + 0);

	t = 2 << QCA956X_SGMII_CONFIG_MODE_CTRL_SHIFT;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_CONFIG_REG);

	t = QCA956X_MR_AN_CONTROL_AN_ENABLE | QCA956X_MR_AN_CONTROL_PHY_RESET;
	__raw_writel(t, gmac_sgmi_base + QCA956X_MR_AN_CONTROL_REG);

	/*
	 * SGMII reset sequence suggested by systems team.
	 */
	t = QCA956X_SGMII_RESET_RX_CLK_N_RESET;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_RESET_REG);

	t = QCA956X_SGMII_RESET_HW_RX_125M_N;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_RESET_REG);

	t |= QCA956X_SGMII_RESET_RX_125M_N;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_RESET_REG);

	t |= QCA956X_SGMII_RESET_TX_125M_N;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_RESET_REG);

	t |= QCA956X_SGMII_RESET_RX_CLK_N;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_RESET_REG);

	t |= QCA956X_SGMII_RESET_TX_CLK_N;
	__raw_writel(t, gmac_sgmi_base + QCA956X_SGMII_RESET_REG);

	t = __raw_readl(gmac_sgmi_base + QCA956X_MR_AN_CONTROL_REG) & ~QCA956X_MR_AN_CONTROL_PHY_RESET;
	__raw_writel(t, gmac_sgmi_base + QCA956X_MR_AN_CONTROL_REG);
	/*
	 * WAR::Across resets SGMII link status goes to weird
	 * state.
	 * if 0xb8070058 (SGMII_DEBUG register) reads other then 0x1f or 0x10
	 * for sure we are in bad  state.
	 * Issue a PHY reset in QCA956X_MR_AN_CONTROL_REG to keep going.
	 */
	status = (__raw_readl(gmac_sgmi_base + QCA956X_SGMII_DEBUG_REG) & 0xff);
	while (!(status == 0xf || status == 0x10)) {

		__raw_writel(t | QCA956X_MR_AN_CONTROL_PHY_RESET, gmac_sgmi_base + QCA956X_MR_AN_CONTROL_REG);
		udelay(100);
		__raw_writel(t, gmac_sgmi_base + QCA956X_MR_AN_CONTROL_REG);
		if (count++ == EAP245_V1_SGMII_LINK_WAR_MAX_TRY) {
			printk("Max resets limit reached exiting...\n");
			break;
		}
		status = (__raw_readl(gmac_sgmi_base + QCA956X_SGMII_DEBUG_REG) & 0xff);
	}
}

static void __init ath_gmac_mii_setup(void __iomem * gmac_sgmi_base, void __iomem * mac_cfg_base)
{
	u32 t;

	ath79_pll_wr(QCA956X_PLL_SWITCH_CLOCK_CONTROL_REG,
		(2 << QCA956X_PLL_SWITCH_CLOCK_SPARE_USB_REFCLK_FREQ_SEL_SHIFT)|
		QCA956X_PLL_SWITCH_CLOCK_SPARE_EN_PLL_TOP |
		QCA956X_PLL_SWITCH_CLOCK_SPARE_MDIO_CLK_SEL1_1 |
		QCA956X_PLL_SWITCH_CLOCK_SPARE_OEN_CLK125M_PLL |
		QCA956X_PLL_SWITCH_CLOCK_SPARE_SWITCHCLK_SEL);

	t = (3 << QCA956X_ETH_CFG_RDV_DELAY_SHIFT) |
		(3 << QCA956X_ETH_CFG_RXD_DELAY_SHIFT) |
		QCA956X_ETH_CFG_RGMII_EN | QCA956X_ETH_CFG_GE0_SGMII;

	__raw_writel(t, gmac_sgmi_base + QCA956X_GMAC_REG_ETH_CFG);

	ath79_pll_wr(QCA956X_PLL_ETH_XMII_CONTROL_REG,
		QCA956X_PLL_ETH_XMII_TX_INVERT |
		(2 << QCA956X_PLL_ETH_XMII_RX_DELAY_SHIFT) |
		(1 << QCA956X_PLL_ETH_XMII_TX_DELAY_SHIFT) |
		QCA956X_PLL_ETH_XMII_GIGE);

	mdelay(1);

	t = QCA956X_MGMT_CFG_CLK_DIV_20 | BIT(31);
	__raw_writel(t, mac_cfg_base + QCA956X_MAC_MII_MGMT_CFG_REG);

	t = QCA956X_MGMT_CFG_CLK_DIV_20;
	__raw_writel(t, mac_cfg_base + QCA956X_MAC_MII_MGMT_CFG_REG);
}

static void __init ath_gmac_hw_start(void __iomem * mac_cfg_base)
{
	u32 t;

	t = QCA956X_MAC_CFG2_PAD_CRC_EN | QCA956X_MAC_CFG2_LEN_CHECK | QCA956X_MAC_CFG2_IF_10_100;
	__raw_writel(t, mac_cfg_base + QCA956X_MAC_CFG2_REG);

	__raw_writel(0x1f00, mac_cfg_base + QCA956X_MAC_FIFO_CFG0_REG);
	__raw_writel(0x10ffff, mac_cfg_base + QCA956X_MAC_FIFO_CFG1_REG);
	__raw_writel(0xAAA0555, mac_cfg_base + QCA956X_MAC_FIFO_CFG2_REG);
	__raw_writel(0x3ffff, mac_cfg_base + QCA956X_MAC_FIFO_CFG4_REG);
	__raw_writel(0x7eccf, mac_cfg_base + QCA956X_MAC_FIFO_CFG5_REG);
	__raw_writel(0x1f00140, mac_cfg_base + QCA956X_MAC_FIFO_CFG3_REG);
}

static void __init ath_gmac_enet_initialize(void)
{
	u32 t;
	void __iomem *ge0_base;
	void __iomem *mac_cfg_base;
	void __iomem *gmac_sgmi_base;

	ge0_base = ioremap_nocache(AR71XX_GE0_BASE, AR71XX_GE0_SIZE);
	mac_cfg_base = ioremap_nocache(QCA956X_MAC_CFG_BASE, QCA956X_MAC_CFG_SIZE);
	gmac_sgmi_base = ioremap_nocache(QCA956X_GMAC_SGMII_BASE, QCA956X_GMAC_SGMII_SIZE);

	athrs_sgmii_res_cal(gmac_sgmi_base);

	ath79_device_reset_set(QCA956X_RESET_SGMII_ANALOG);
	mdelay(100);
	ath79_device_reset_clear(QCA956X_RESET_SGMII_ANALOG);
	mdelay(100);

	t = QCA956X_RESET_SGMII |
		QCA956X_RESET_SGMII_ANALOG |
		QCA956X_RESET_EXTERNAL |
		QCA956X_RESET_SWITCH_ANALOG |
		QCA956X_RESET_SWITCH;
	ath79_device_reset_set(t);
	mdelay(100);

	t = QCA956X_RESET_SGMII | QCA956X_RESET_SGMII_ANALOG | QCA956X_RESET_EXTERNAL;
	ath79_device_reset_clear(t);
	mdelay(100);

	t = __raw_readl(mac_cfg_base + QCA956X_MAC_CFG1_REG);
	t |= QCA956X_MAC_CFG1_SOFT_RST | QCA956X_MAC_CFG1_RX_RST | QCA956X_MAC_CFG1_TX_RST;
	__raw_writel(t, mac_cfg_base + QCA956X_MAC_CFG1_REG);

	t = QCA956X_RESET_GE0_MAC | QCA956X_RESET_GE1_MAC | QCA956X_RESET_GE0_MDIO | QCA956X_RESET_GE1_MDIO;
	ath79_device_reset_set(t);
	mdelay(100);
	ath79_device_reset_clear(t);
	mdelay(100);
	mdelay(10);

	ath_gmac_mii_setup(gmac_sgmi_base, mac_cfg_base);

	athrs_sgmii_set_up(gmac_sgmi_base, ge0_base);

	ath_gmac_hw_start(mac_cfg_base);

	iounmap(ge0_base);
	iounmap(mac_cfg_base);
	iounmap(gmac_sgmi_base);
}

static void __init eap245_v1_setup(void)
{
	u8 *mac = (u8 *) KSEG1ADDR(0x1f030008);
	u8 *ee = (u8 *) KSEG1ADDR(0x1fff1000);
	u8 wmac_addr[ETH_ALEN];

	ath79_register_leds_gpio(-1, ARRAY_SIZE(eap245_v1_leds_gpio),
		eap245_v1_leds_gpio);

	ath79_register_gpio_keys_polled(-1, EAP245_V1_KEYS_POLL_INTERVAL,
		ARRAY_SIZE(eap245_v1_gpio_keys), eap245_v1_gpio_keys);

	ath79_register_m25p80(NULL);

	/* This board need initialization code for LAN */
	ath_gmac_enet_initialize();
	platform_device_register(&eap245_v1_phy_device);
	ath79_setup_qca956x_eth_cfg(QCA956X_ETH_CFG_SW_PHY_SWAP | QCA956X_ETH_CFG_SW_PHY_ADDR_SWAP);

	printk(KERN_DEBUG "Read mac address %pK: %pM\n", mac, mac);
	/* Set 2.4 GHz to mac + 1 */
	ath79_init_mac(wmac_addr, mac, 1);
	ath79_register_wmac(ee, wmac_addr);

	/* Initializer pci bus for ath10k (5GHz) */
	ath79_register_pci();

	/* Set lan port to eepromm mac address */
	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_SGMII;
	ath79_eth0_data.speed = SPEED_1000;
	ath79_eth0_data.duplex = DUPLEX_FULL;
	ath79_eth0_data.phy_mask = BIT(EAP245_V1_LAN_PHYADDR);
	ath79_register_eth(0);
}

MIPS_MACHINE(ATH79_MACH_EAP245_V1, "EAP245-V1", "TP-LINK EAP245 v1", eap245_v1_setup);
