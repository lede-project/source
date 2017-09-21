/*
 * Copyright (c) dog hunter AG - Zug - CH
 * General Public License version 2 (GPLv2)
 * Author Federico Vaga <federicov@linino.org>
 */

#ifndef MACH_LININO_H_
#define MACH_LININO_H_

#if defined(LININO_FREEDOG)
	#define GPIO_FUNC_SET (AR933X_GPIO_FUNC_JTAG_DISABLE | \
				AR933X_GPIO_FUNC_I2S_MCK_EN)
	#define GPIO_FUNC_CLEAR (AR933X_GPIO_FUNC_SPDIF_EN | \
				AR933X_GPIO_FUNC_I2SO_EN | \
				AR933X_GPIO_FUNC_UART_RTS_CTS_EN)
	#define GPIO_FUNC2_SET (AR933X_GPIO_FUNC2_JUMPSTART_DISABLE | \
				AR933X_GPIO_FUNC2_WPS_DISABLE | \
				AR933X_GPIO_FUNC2_DIS_MIC)
	#define GPIO_FUNC2_CLEAR (AR933X_GPIO_FUNC2_I2SD_ON_12)
#else
	#define GPIO_FUNC_SET (AR933X_GPIO_FUNC_JTAG_DISABLE | \
				AR933X_GPIO_FUNC_I2S_MCK_EN)
	#define GPIO_FUNC_CLEAR (0)
	#define GPIO_FUNC2_SET (AR933X_GPIO_FUNC2_JUMPSTART_DISABLE)
	#define GPIO_FUNC2_CLEAR (0)
#endif

#define DS_GPIO_LED0			0
#define DS_GPIO_LED1			1
#define DS_GPIO_LED2			13
#define DS_GPIO_LED3			14
#define DS_GPIO_LED4			15
#define DS_GPIO_LED5			16

#define DS_GPIO_RGB_LED_BLUE            DS_GPIO_LED0
#define DS_GPIO_RGB_LED_RED             DS_GPIO_LED1
#define DS_GPIO_RGB_LED_GREEN           DS_GPIO_LED3
#define DS_GPIO_LED_WLAN             	DS_GPIO_LED0
#define DS_GPIO_LED_USB            	DS_GPIO_LED1

#define DS_GPIO_OE			21
#define DS_GPIO_OE2			22
#define DS_GPIO_MCU_RESET		18
#define DS_GPIO_UART_ENA		23
#define DS_GPIO_CONF_BTN	20

/* Configure level shifter enable GPIO */
#if defined(LININO_FREEDOG)
	#define DS_GPIO_SWD_EN		12
	#define DS_GPIO_SWD_OE		11
	#define DS_GPIO_UART_POL 	GPIOF_OUT_INIT_HIGH
#else	/* YUN */
	#define DS_GPIO_UART_POL        GPIOF_OUT_INIT_LOW
#endif

#define DS_KEYS_POLL_INTERVAL		20	/* msecs */
#define DS_KEYS_DEBOUNCE_INTERVAL	(3 * DS_KEYS_POLL_INTERVAL)

#define DS_MAC0_OFFSET			0x0000
#define DS_MAC1_OFFSET			0x0006
#define DS_CALDATA_OFFSET		0x1000
#define DS_WMAC_MAC_OFFSET		0x1002

/* SPI GPIOs */

#if defined(LININO_FREEDOG)
	#define	LININO_GPIO_SPI_SCK	7
#else	/* YUN */
	#define	LININO_GPIO_SPI_SCK	11
#endif

#define	LININO_N_SPI_CHIP_SELECT	1
#define	LININO_GPIO_SPI_MOSI		27
#define	LININO_GPIO_SPI_MISO		8
#define LININO_GPIO_SPI_CS0		26

// HANDSHAKE signal
#if defined(LININO_FREEDOG)
	#define LININO_GPIO_SPI_INTERRUPT		23
#else
	#define LININO_GPIO_SPI_INTERRUPT		19
#endif

#endif /* MACH_LININO_H_ */
