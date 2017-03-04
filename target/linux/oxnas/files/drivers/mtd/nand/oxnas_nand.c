/*
 * Oxford Semiconductor OXNAS NAND driver

 * Copyright (C) 2016 Neil Armstrong <narmstrong@baylibre.com>
 * Heavily based on plat_nand.c :
 * Author: Vitaly Wool <vitalywool@gmail.com>
 * Copyright (C) 2013 Ma Haijun <mahaijuns@gmail.com>
 * Copyright (C) 2012 John Crispin <blogic@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/of.h>

/* Nand commands */
#define OXNAS_NAND_CMD_ALE		BIT(18)
#define OXNAS_NAND_CMD_CLE		BIT(19)

#define OXNAS_NAND_MAX_CHIPS	1

struct oxnas_nand {
	struct nand_hw_control base;
	void __iomem *io_base;
	struct clk *clk;
	struct nand_chip *chips[OXNAS_NAND_MAX_CHIPS];
	unsigned long ctrl;
	struct mtd_partition *partitions;
	int nr_partitions;
};

static uint8_t oxnas_nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct oxnas_nand *oxnas = nand_get_controller_data(chip);

	return readb(oxnas->io_base);
}

static void oxnas_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct oxnas_nand *oxnas = nand_get_controller_data(chip);

	ioread8_rep(oxnas->io_base, buf, len);
}

static void oxnas_nand_write_buf(struct mtd_info *mtd,
				 const uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct oxnas_nand *oxnas = nand_get_controller_data(chip);

	iowrite8_rep(oxnas->io_base + oxnas->ctrl, buf, len);
}

/* Single CS command control */
static void oxnas_nand_cmd_ctrl(struct mtd_info *mtd, int cmd,
				unsigned int ctrl)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct oxnas_nand *oxnas = nand_get_controller_data(chip);

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_CLE)
			oxnas->ctrl = OXNAS_NAND_CMD_CLE;
		else if (ctrl & NAND_ALE)
			oxnas->ctrl = OXNAS_NAND_CMD_ALE;
		else
			oxnas->ctrl = 0;
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, oxnas->io_base + oxnas->ctrl);
}

/*
 * Probe for the NAND device.
 */
static int oxnas_nand_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *nand_np;
	struct oxnas_nand *oxnas;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	struct resource *res;
	int nchips = 0;
	int count = 0;
	int err = 0;

	/* Allocate memory for the device structure (and zero it) */
	oxnas = devm_kzalloc(&pdev->dev, sizeof(struct nand_chip),
			    GFP_KERNEL);
	if (!oxnas)
		return -ENOMEM;

	nand_hw_control_init(&oxnas->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	oxnas->io_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(oxnas->io_base))
		return PTR_ERR(oxnas->io_base);

	oxnas->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(oxnas->clk))
		oxnas->clk = NULL;

	/* Only a single chip node is supported */
	count = of_get_child_count(np);
	if (count > 1)
		return -EINVAL;

	clk_prepare_enable(oxnas->clk);
	device_reset_optional(&pdev->dev);

	for_each_child_of_node(np, nand_np) {
		chip = devm_kzalloc(&pdev->dev, sizeof(struct nand_chip),
				    GFP_KERNEL);
		if (!chip)
			return -ENOMEM;

		chip->controller = &oxnas->base;

		nand_set_flash_node(chip, nand_np);
		nand_set_controller_data(chip, oxnas);

		mtd = nand_to_mtd(chip);
		mtd->dev.parent = &pdev->dev;
		mtd->priv = chip;

		chip->cmd_ctrl = oxnas_nand_cmd_ctrl;
		chip->read_buf = oxnas_nand_read_buf;
		chip->read_byte = oxnas_nand_read_byte;
		chip->write_buf = oxnas_nand_write_buf;
		chip->chip_delay = 30;

		/* Scan to find existence of the device */
		err = nand_scan(mtd, 1);
		if (err)
			return err;

		err = mtd_device_register(mtd, NULL, 0);
		if (err) {
			nand_release(mtd);
			return err;
		}

		oxnas->chips[nchips] = chip;
		++nchips;
	}

	/* Exit if no chips found */
	if (!nchips)
		return -ENODEV;

	platform_set_drvdata(pdev, oxnas);

	return 0;
}

static int oxnas_nand_remove(struct platform_device *pdev)
{
	struct oxnas_nand *oxnas = platform_get_drvdata(pdev);

	if (oxnas->chips[0])
		nand_release(nand_to_mtd(oxnas->chips[0]));

	clk_disable_unprepare(oxnas->clk);

	return 0;
}

static const struct of_device_id oxnas_nand_match[] = {
	{ .compatible = "oxsemi,ox820-nand" },
	{},
};
MODULE_DEVICE_TABLE(of, oxnas_nand_match);

static struct platform_driver oxnas_nand_driver = {
	.probe	= oxnas_nand_probe,
	.remove	= oxnas_nand_remove,
	.driver	= {
		.name		= "oxnas_nand",
		.of_match_table = oxnas_nand_match,
	},
};

module_platform_driver(oxnas_nand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Neil Armstrong <narmstrong@baylibre.com>");
MODULE_DESCRIPTION("Oxnas NAND driver");
MODULE_ALIAS("platform:oxnas_nand");
