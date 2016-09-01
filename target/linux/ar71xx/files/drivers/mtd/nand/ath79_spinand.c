/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2003-2013 Broadcom Corporation
 *
 * Copyright (c) 2009-2010 Micron Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/spi/spi.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mtd/mtd.h>
#include <linux/sizes.h>

/* cmd */
#define ATH79_SPINAND_CMD_READ			0x13
#define ATH79_SPINAND_CMD_READ_RDM			0x03
#define ATH79_SPINAND_CMD_PROG_PAGE_LOAD		0x02
#define ATH79_SPINAND_CMD_PROG_PAGE			0x84
#define ATH79_SPINAND_CMD_PROG_PAGE_EXC		0x10
#define ATH79_SPINAND_CMD_ERASE_BLK			0xd8
#define ATH79_SPINAND_CMD_WR_ENABLE			0x06
#define ATH79_SPINAND_CMD_WR_DISABLE			0x04
#define ATH79_SPINAND_CMD_READ_ID			0x9f
#define ATH79_SPINAND_CMD_RESET			0xff
#define ATH79_SPINAND_CMD_READ_REG			0x0f
#define ATH79_SPINAND_CMD_WRITE_REG			0x1f

/* feature/ status reg */
#define ATH79_SPINAND_REG_BLOCK_LOCK			0xa0
#define ATH79_SPINAND_REG_OTP				0xb0
#define ATH79_SPINAND_REG_STATUS			0xc0

/* status */
#define ATH79_SPINAND_STATUS_OIP_MASK			0x01
#define ATH79_SPINAND_STATUS_READY			0
#define ATH79_SPINAND_STATUS_BUSY			BIT(1)

#define ATH79_SPINAND_STATUS_E_FAIL_MASK		0x04
#define ATH79_SPINAND_STATUS_E_FAIL			BIT(2)

#define ATH79_SPINAND_STATUS_P_FAIL_MASK		0x08
#define ATH79_SPINAND_STATUS_P_FAIL			BIT(3)

/* ECC/OTP enable defines */
#define ATH79_SPINAND_REG_ECC_MASK			0x10
#define ATH79_SPINAND_REG_ECC_OFF			0
#define ATH79_SPINAND_REG_ECC_ON			BIT(4)

#define ATH79_SPINAND_REG_OTP_EN			BIT(6)
#define ATH79_SPINAND_REG_OTP_PRT			BIT(7)

/* block lock */
#define ATH79_SPINAND_BL_ALL_UNLOCKED			0

#define ATH79_SPINAND_PAGE_TO_BLOCK(p)		((p) >> 6)
#define ATH79_SPINAND_OFS_TO_PAGE(ofs)		(((ofs) >> 17) << 6)
#define ATH79_SPINAND_BUF_SIZE			(2048 * 64)

#define ATH79_SPINAND_BITS_PER_WORD(len)		((len) & 0x02 ? 16 : 32)

struct ath79_spinand_priv {
	u8 			mfr;
	u8			ecc_error;
	int			ecc_size;
	int			ecc_bytes;
	int			ecc_strength;
	struct nand_ecclayout   *ecc_layout;
	struct nand_bbt_descr	*badblock_pattern;
	u8			(*ecc_status)(u8 status);
	void			(*read_rdm_addr)(u32 offset, u8 *addr);
	int			(*program_load)(struct spi_device *spi,
						u32 offset, u32 len, u8 *wbuf);
	int			(*erase_block)(struct spi_device *spi, u32 page);
	int			(*page_read_to_cache)(struct spi_device *spi, u32 page);
	int			(*program_execute)(struct spi_device *spi, u32 page);
};

struct ath79_spinand_state {
	uint32_t	col;
	uint32_t	row;
	int		buf_ptr;
	u8		*buf;
};

struct ath79_spinand_info {
	struct spi_device		*spi;
	struct ath79_spinand_state	*state;
	void				*priv;
};

struct ath79_spinand_command {
	u8		cmd;
	u32		n_addr;		/* Number of address */
	u8		addr[3];	/* Reg Offset */
	u32		n_dummy;	/* Dummy use */
	u32		n_tx;		/* Number of tx bytes */
	u8		*tx_buf;	/* Tx buf */
	u32		n_rx;		/* Number of rx bytes */
	u8		*rx_buf;	/* Rx buf */
};

static u8 badblock_pattern[] = { 0xff, };

static struct nand_bbt_descr ath79_badblock_pattern_default = {
	.options = 0,
	.offs = 0,
	.len = 1,
	.pattern = badblock_pattern,
};

static struct nand_ecclayout ath79_spinand_oob_128_gd = {
	.eccbytes = 64,
	.eccpos = {
		64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {
		{.offset = 16, .length = 48},
	}
};

/* ECC parity code stored in the additional hidden spare area */
static struct nand_ecclayout ath79_spinand_oob_64_mx = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 4,  .length = 4},
		{.offset = 20, .length = 4},
		{.offset = 36, .length = 4},
		{.offset = 52, .length = 4},
	}
};

static struct nand_ecclayout ath79_spinand_oob_64_win = {
	.eccbytes = 40,
	.eccpos = {
		8, 9, 10, 11, 12, 13, 14, 15,
		24, 25, 26, 27, 28, 29, 30, 31,
		40, 41, 42, 43, 44, 45, 46, 47,
		56, 57, 58, 59, 60, 61, 62, 63,
		72, 73, 74, 75, 76, 77, 78, 79},
	.oobfree = {
		{.offset = 4,  .length = 4},
		{.offset = 20, .length = 4},
		{.offset = 36, .length = 4},
		{.offset = 52, .length = 4},
	}
};

static inline struct ath79_spinand_state *mtd_to_state(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct ath79_spinand_info *info = (struct ath79_spinand_info *)chip->priv;

	return info->state;
}

static inline struct ath79_spinand_priv *spi_to_priv(struct spi_device *spi)
{
	struct mtd_info *mtd = dev_get_drvdata(&spi->dev);
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct ath79_spinand_info *info = (struct ath79_spinand_info *)chip->priv;

	return info->priv;
}

static inline u8 ath79_spinand_ecc_status(struct spi_device *spi, u8 status)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	return priv->ecc_status(status);
}

static inline u8 ath79_spinand_ecc_error(struct spi_device *spi)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	return priv->ecc_error;
}

static inline void ath79_spinand_read_rdm_addr(struct spi_device *spi, u32 offset, u8 *addr)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	priv->read_rdm_addr(offset, addr);
}

static inline int ath79_spinand_program_load(struct spi_device *spi, u32 offset,
					     u32 len, u8 *wbuf)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	return priv->program_load(spi, offset, len, wbuf);
}

static inline int ath79_spinand_erase_block_erase(struct spi_device *spi, u32 page)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	return priv->erase_block(spi, page);
}

static inline int ath79_spinand_read_page_to_cache(struct spi_device *spi, u32 page)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	return priv->page_read_to_cache(spi, page);
}

static inline int ath79_spinand_program_execute(struct spi_device *spi, u32 page)
{
	struct ath79_spinand_priv *priv = spi_to_priv(spi);

	return priv->program_execute(spi, page);
}

static int ath79_spinand_cmd(struct spi_device *spi, struct ath79_spinand_command *cmd)
{
	struct spi_message message;
	struct spi_transfer x[4];
	u8 dummy = 0xff;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));

	x[0].len = 1;
	x[0].tx_buf = &cmd->cmd;
	spi_message_add_tail(&x[0], &message);

	if (cmd->n_addr) {
		x[1].len = cmd->n_addr;
		x[1].tx_buf = cmd->addr;
		spi_message_add_tail(&x[1], &message);
	}

	if (cmd->n_dummy) {
		x[2].len = cmd->n_dummy;
		x[2].tx_buf = &dummy;
		spi_message_add_tail(&x[2], &message);
	}

	if (cmd->n_tx) {
		x[3].len = cmd->n_tx;
		x[3].tx_buf = cmd->tx_buf;
		if (!(cmd->n_tx & 1))
			x[3].bits_per_word = ATH79_SPINAND_BITS_PER_WORD(cmd->n_tx);
		spi_message_add_tail(&x[3], &message);
	} else if (cmd->n_rx) {
		x[3].len = cmd->n_rx;
		x[3].rx_buf = cmd->rx_buf;
		if (!(cmd->n_rx & 1))
			x[3].bits_per_word = ATH79_SPINAND_BITS_PER_WORD(cmd->n_rx);
		spi_message_add_tail(&x[3], &message);
	}

	return spi_sync(spi, &message);
}

static int ath79_spinand_read_id(struct spi_device *spi_nand, u8 *id)
{
	int retval;
	u8 nand_id[3];
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = ATH79_SPINAND_CMD_READ_ID;
	cmd.n_rx = 3;
	cmd.rx_buf = nand_id;

	retval = ath79_spinand_cmd(spi_nand, &cmd);
	if (retval < 0) {
		dev_err(&spi_nand->dev, "error %d reading id\n", retval);
		return retval;
	}

	if (nand_id[0] == NAND_MFR_GIGADEVICE || nand_id[0] == NAND_MFR_HEYANGTEK) {
		id[0] = nand_id[0];
		id[1] = nand_id[1];
	} else { /* Macronix, Micron */
		id[0] = nand_id[1];
		id[1] = nand_id[2];
	}

	return retval;
}

static int ath79_spinand_read_status(struct spi_device *spi_nand, uint8_t *status)
{
	struct ath79_spinand_command cmd = {0};
	int ret;

	cmd.cmd = ATH79_SPINAND_CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = ATH79_SPINAND_REG_STATUS;
	cmd.n_rx = 1;
	cmd.rx_buf = status;

	ret = ath79_spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "err: %d read status register\n", ret);

	return ret;
}

#define ATH79_SPINAND_MAX_WAIT_JIFFIES  (40 * HZ)
static int __ath79_wait_till_ready(struct spi_device *spi_nand, u8 *status)
{
	unsigned long deadline;

	deadline = jiffies + ATH79_SPINAND_MAX_WAIT_JIFFIES;
	do {
		if (ath79_spinand_read_status(spi_nand, status))
			return -1;
		else if ((*status & ATH79_SPINAND_STATUS_OIP_MASK) == ATH79_SPINAND_STATUS_READY)
			break;

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	return 0;
}

static int ath79_wait_till_ready(struct spi_device *spi_nand)
{
	u8 stat = 0;

	if (__ath79_wait_till_ready(spi_nand, &stat))
		return -1;

	if ((stat & ATH79_SPINAND_STATUS_OIP_MASK) == ATH79_SPINAND_STATUS_READY)
		return 0;

	return -1;
}

static int ath79_spinand_get_otp(struct spi_device *spi_nand, u8 *otp)
{
	struct ath79_spinand_command cmd = {0};
	int retval;

	cmd.cmd = ATH79_SPINAND_CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = ATH79_SPINAND_REG_OTP;
	cmd.n_rx = 1;
	cmd.rx_buf = otp;

	retval = ath79_spinand_cmd(spi_nand, &cmd);
	if (retval < 0)
		dev_err(&spi_nand->dev, "error %d get otp\n", retval);

	return retval;
}

static int ath79_spinand_set_otp(struct spi_device *spi_nand, u8 *otp)
{
	int retval;
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = ATH79_SPINAND_CMD_WRITE_REG,
	cmd.n_addr = 1,
	cmd.addr[0] = ATH79_SPINAND_REG_OTP,
	cmd.n_tx = 1,
	cmd.tx_buf = otp,

	retval = ath79_spinand_cmd(spi_nand, &cmd);
	if (retval < 0)
		dev_err(&spi_nand->dev, "error %d set otp\n", retval);

	return retval;
}

static int ath79_spinand_enable_ecc(struct spi_device *spi_nand)
{
	u8 otp = 0;

	if (ath79_spinand_get_otp(spi_nand, &otp))
		return -1;

	if ((otp & ATH79_SPINAND_REG_ECC_MASK) == ATH79_SPINAND_REG_ECC_ON)
		return 0;

	otp |= ATH79_SPINAND_REG_ECC_ON;
	if (ath79_spinand_set_otp(spi_nand, &otp))
		return -1;

	return ath79_spinand_get_otp(spi_nand, &otp);
}

static int ath79_spinand_disable_ecc(struct spi_device *spi_nand)
{
	u8 otp = 0;

	if (ath79_spinand_get_otp(spi_nand, &otp))
		return -1;

	if ((otp & ATH79_SPINAND_REG_ECC_MASK) == ATH79_SPINAND_REG_ECC_OFF)
		return 0;

	otp &= ~ATH79_SPINAND_REG_ECC_MASK;
	if (ath79_spinand_set_otp(spi_nand, &otp))
		return -1;

	return ath79_spinand_get_otp(spi_nand, &otp);
}

static int ath79_spinand_write_enable(struct spi_device *spi_nand)
{
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = ATH79_SPINAND_CMD_WR_ENABLE;

	return ath79_spinand_cmd(spi_nand, &cmd);
}

static int ath79_spinand_read_from_cache(struct spi_device *spi_nand,
		u32 offset, u32 len, u8 *rbuf)
{
	struct ath79_spinand_command cmd = {0};

	ath79_spinand_read_rdm_addr(spi_nand, offset, cmd.addr);

	cmd.cmd = ATH79_SPINAND_CMD_READ_RDM;
	cmd.n_addr = 3;
	cmd.n_dummy = 0;
	cmd.n_rx = len;
	cmd.rx_buf = rbuf;

	return ath79_spinand_cmd(spi_nand, &cmd);
}

static int ath79_spinand_read_page(struct spi_device *spi_nand, u32 page_id,
		u32 offset, u32 len, u8 *rbuf)
{
	int ret;
	u8 status = 0;

	ret = ath79_spinand_read_page_to_cache(spi_nand, page_id);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "Read page to cache failed!\n");
		return ret;
	}

	if (__ath79_wait_till_ready(spi_nand, &status)) {
		dev_err(&spi_nand->dev, "WAIT timedout!\n");
		return -EBUSY;
	}

	if ((status & ATH79_SPINAND_STATUS_OIP_MASK) != ATH79_SPINAND_STATUS_READY)
		return -EBUSY;

	status = ath79_spinand_ecc_status(spi_nand, status);
	if (ath79_spinand_ecc_error(spi_nand) == status) {
		dev_err(&spi_nand->dev,
			"ecc error, page=%d\n", page_id);

		return -1;
	}

	ret = ath79_spinand_read_from_cache(spi_nand, offset, len, rbuf);
	if (ret < 0) {
		dev_err(&spi_nand->dev, "read from cache failed!!\n");
		return ret;
	}

	return ret;
}

static int ath79_spinand_program_data_to_cache(struct spi_device *spi_nand,
		u32 offset, u32 len, u8 *wbuf)
{
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = ATH79_SPINAND_CMD_PROG_PAGE_LOAD;
	cmd.n_addr = 2;
	cmd.addr[0] = (u8)(offset >> 8);
	cmd.addr[1] = (u8)(offset >> 0);
	cmd.n_tx = len;
	cmd.tx_buf = wbuf;

	return ath79_spinand_cmd(spi_nand, &cmd);
}

static int ath79_spinand_program_page(struct spi_device *spi_nand,
		u32 page_id, u32 offset, u32 len, u8 *buf, u32 cache_size)
{
	int retval;
	u8 status = 0;

	if (unlikely(offset)) {
		uint8_t *wbuf;
		unsigned int i, j;

		wbuf = devm_kzalloc(&spi_nand->dev, cache_size, GFP_KERNEL);
		if (!wbuf) {
			dev_err(&spi_nand->dev, "No memory\n");
			return -ENOMEM;
		}

		if (ath79_spinand_read_page(spi_nand, page_id, 0, cache_size, wbuf)) {
			devm_kfree(&spi_nand->dev, wbuf);
			return -1;
		}

		for (i = offset, j = 0; i < len; i++, j++)
			wbuf[i] &= buf[j];

		retval = ath79_spinand_program_load(spi_nand, offset, len, wbuf);

		devm_kfree(&spi_nand->dev, wbuf);
	} else {
		retval = ath79_spinand_program_load(spi_nand, offset, len, buf);
	}

	if (retval < 0)
		return retval;

	retval = ath79_spinand_program_execute(spi_nand, page_id);
	if (retval < 0) {
		dev_err(&spi_nand->dev, "program execute failed\n");
		return retval;
	}

	if (__ath79_wait_till_ready(spi_nand, &status)) {
		dev_err(&spi_nand->dev, "wait timedout!!!\n");
		return -EBUSY;
	}

	if ((status & ATH79_SPINAND_STATUS_OIP_MASK) != ATH79_SPINAND_STATUS_READY)
		return -EBUSY;

	if ((status & ATH79_SPINAND_STATUS_P_FAIL_MASK) == ATH79_SPINAND_STATUS_P_FAIL) {
		dev_err(&spi_nand->dev,
			"program error, page %d\n", page_id);
		return -1;
	}

	return 0;
}

static int ath79_spinand_erase_block(struct mtd_info *mtd,
		struct spi_device *spi_nand, u32 page)
{
	int retval;
	u8 status = 0;

	retval = ath79_spinand_write_enable(spi_nand);
	if (retval < 0) {
		dev_err(&spi_nand->dev, "write enable failed!\n");
		return retval;
	}

	if (ath79_wait_till_ready(spi_nand)) {
		dev_err(&spi_nand->dev, "wait timedout!\n");
		return -EBUSY;
	}

	retval = ath79_spinand_erase_block_erase(spi_nand, page);
	if (retval < 0) {
		dev_err(&spi_nand->dev, "erase block failed!\n");
		return retval;
	}

	if (__ath79_wait_till_ready(spi_nand, &status)) {
		dev_err(&spi_nand->dev, "wait timedout!\n");
		return -EBUSY;
	}

	if ((status & ATH79_SPINAND_STATUS_OIP_MASK) != ATH79_SPINAND_STATUS_READY)
		return -EBUSY;

	if ((status & ATH79_SPINAND_STATUS_E_FAIL_MASK) == ATH79_SPINAND_STATUS_E_FAIL) {
		dev_err(&spi_nand->dev,
			"erase error, block %d\n", ATH79_SPINAND_PAGE_TO_BLOCK(page));
		return -1;
	}

	return 0;
}

static int ath79_spinand_write_page_hwecc(struct mtd_info *mtd,
					  struct nand_chip *chip,
					  const uint8_t *buf,
					  int oob_required)
{
	chip->write_buf(mtd, buf, chip->ecc.size * chip->ecc.steps);

	return 0;
}

static int ath79_spinand_read_page_hwecc(struct mtd_info *mtd,
					 struct nand_chip *chip,
					 uint8_t *buf,
					 int oob_required,
					 int page)
{
	u8 status;
	uint8_t *p = buf;
	struct ath79_spinand_info *info =
			(struct ath79_spinand_info *)chip->priv;
	struct spi_device *spi_nand = info->spi;

	chip->read_buf(mtd, p, chip->ecc.size * chip->ecc.steps);

	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	if (__ath79_wait_till_ready(spi_nand, &status)) {
		dev_err(&spi_nand->dev, "wait timedout!\n");
		return -EBUSY;
	}

	if ((status & ATH79_SPINAND_STATUS_OIP_MASK) != ATH79_SPINAND_STATUS_READY)
		return -EBUSY;

	status = ath79_spinand_ecc_status(spi_nand, status);
	if (ath79_spinand_ecc_error(spi_nand) == status) {
		pr_info("%s: Internal ECC error\n", __func__);
		mtd->ecc_stats.failed++;
	} else if (status) {
		pr_debug("%s: Internal ECC error corrected\n", __func__);
		mtd->ecc_stats.corrected++;
	}

	return 0;
}

static void ath79_spinand_select_chip(struct mtd_info *mtd, int dev)
{
}

/*
 * Mark bad block: the 1st page of the bad block set to zero, include
 * main area and spare area.
 * NOTE, irreversible and erase is not recommanded
 */
static int ath79_spinand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	u8 status;
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct ath79_spinand_info *info = (struct ath79_spinand_info *)chip->priv;
	struct spi_device *spi_nand = info->spi;
	struct ath79_spinand_state *state = info->state;
	int page_len = mtd->writesize + mtd->oobsize;
	int retval, page_id = ATH79_SPINAND_OFS_TO_PAGE(ofs);;

	memset(state->buf, 0, page_len);

	retval = ath79_spinand_program_load(spi_nand, 0, page_len, state->buf);
	if (retval < 0)
		return retval;

	retval = ath79_spinand_program_execute(spi_nand, page_id);
	if (retval < 0) {
		dev_err(&spi_nand->dev, "program execute failed\n");
		return retval;
	}

	if (__ath79_wait_till_ready(spi_nand, &status)) {
		dev_err(&spi_nand->dev, "wait timedout!!!\n");
		return -EBUSY;
	}

	if ((status & ATH79_SPINAND_STATUS_OIP_MASK) != ATH79_SPINAND_STATUS_READY)
		return -EBUSY;

	if ((status & ATH79_SPINAND_STATUS_P_FAIL_MASK) == ATH79_SPINAND_STATUS_P_FAIL) {
		dev_err(&spi_nand->dev,
			"program error, page %d\n", page_id);
		return -1;
	}

	return 0;
}

static uint8_t ath79_spinand_read_byte(struct mtd_info *mtd)
{
	struct ath79_spinand_state *state = mtd_to_state(mtd);

	return state->buf[state->buf_ptr++];
}

static int ath79_spinand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct ath79_spinand_info *info = (struct ath79_spinand_info *)chip->priv;
	unsigned long timeo = jiffies;
	u8 status;

	if (chip->state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 20) / 1000;

	while (time_before(jiffies, timeo)) {
		if (ath79_spinand_read_status(info->spi, &status))
			return NAND_STATUS_FAIL;

		if ((status & ATH79_SPINAND_STATUS_OIP_MASK) == ATH79_SPINAND_STATUS_READY) {
			status = ath79_spinand_ecc_status(info->spi, status);
			return (ath79_spinand_ecc_error(info->spi) == status)  ?
				NAND_STATUS_FAIL : NAND_STATUS_READY;
		}

		cond_resched();
	}
	return NAND_STATUS_FAIL;
}

static void ath79_spinand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct ath79_spinand_state *state = mtd_to_state(mtd);

	BUG_ON(ATH79_SPINAND_BUF_SIZE - state->buf_ptr < len);

	memcpy(state->buf + state->buf_ptr, buf, len);
	state->buf_ptr += len;
}

static void ath79_spinand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct ath79_spinand_state *state = mtd_to_state(mtd);

	memcpy(buf, state->buf + state->buf_ptr, len);
	state->buf_ptr += len;
}

static void ath79_spinand_reset(struct spi_device *spi_nand)
{
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = ATH79_SPINAND_CMD_RESET;

	if (ath79_spinand_cmd(spi_nand, &cmd) < 0)
		pr_info("ath79_spinand reset failed!\n");

	/* elapse 1ms before issuing any other command */
	udelay(1000);

	if (ath79_wait_till_ready(spi_nand))
		dev_err(&spi_nand->dev, "wait timedout!\n");
}

static void ath79_spinand_cmdfunc(struct mtd_info *mtd, unsigned int command,
				  int column, int page)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct ath79_spinand_info *info = (struct ath79_spinand_info *)chip->priv;
	struct ath79_spinand_state *state = info->state;

	switch (command) {
	case NAND_CMD_READ1:
	case NAND_CMD_READ0:
		state->buf_ptr = 0;
		ath79_spinand_read_page(info->spi, page, 0x0,
					mtd->writesize + mtd->oobsize,
					state->buf);
		break;
	case NAND_CMD_READOOB:
		state->buf_ptr = 0;
		ath79_spinand_read_page(info->spi, page, mtd->writesize,
					mtd->oobsize, state->buf);
		break;
	case NAND_CMD_RNDOUT:
		state->buf_ptr = column;
		break;
	case NAND_CMD_READID:
		state->buf_ptr = 0;
		ath79_spinand_read_id(info->spi, (u8 *)state->buf);
		break;
	case NAND_CMD_PARAM:
		state->buf_ptr = 0;
		break;
	/* ERASE1 stores the block and page address */
	case NAND_CMD_ERASE1:
		ath79_spinand_erase_block(mtd, info->spi, page);
		break;
	/* ERASE2 uses the block and page address from ERASE1 */
	case NAND_CMD_ERASE2:
		break;
	/* SEQIN sets up the addr buffer and all registers except the length */
	case NAND_CMD_SEQIN:
		state->col = column;
		state->row = page;
		state->buf_ptr = 0;
		break;
	/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
	case NAND_CMD_PAGEPROG:
		ath79_spinand_program_page(info->spi, state->row, state->col,
					   state->buf_ptr, state->buf,
					   mtd->writesize + mtd->oobsize);
		break;
	case NAND_CMD_STATUS:
		ath79_spinand_get_otp(info->spi, state->buf);
		if (!(state->buf[0] & ATH79_SPINAND_REG_OTP_PRT))
			state->buf[0] = ATH79_SPINAND_REG_OTP_PRT;
		state->buf_ptr = 0;
		break;
	/* RESET command */
	case NAND_CMD_RESET:
		if (ath79_wait_till_ready(info->spi))
			dev_err(&info->spi->dev, "WAIT timedout!!!\n");

		/* a minimum of 250us must elapse before issuing RESET cmd*/
		udelay(250);
		ath79_spinand_reset(info->spi);
		break;
	default:
		dev_err(&mtd->dev, "Unknown CMD: 0x%x\n", command);
	}
}

static int ath79_spinand_lock_block(struct spi_device *spi_nand, u8 lock)
{
	struct ath79_spinand_command cmd = {0};
	int ret;

	cmd.cmd = ATH79_SPINAND_CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = ATH79_SPINAND_REG_BLOCK_LOCK;
	cmd.n_tx = 1;
	cmd.tx_buf = &lock;

	ret = ath79_spinand_cmd(spi_nand, &cmd);
	if (ret < 0)
		dev_err(&spi_nand->dev, "error %d lock block\n", ret);

	return ret;
}

/*
 * 	ECCSR[2:0]	ECC Status
 *	-------------------------------------------
 *	000		no bit errors were detected
 *	001		bit errors(<3) corrected
 *	010		bit errors(=4) corrected
 *	011		bit errors(=5) corrected
 *	100		bit errors(=6) corrected
 *	101		bit errors(=7) corrected
 *	110		bit errors(=8) corrected
 *	111		uncorrectable
 */
static inline u8 ath79_spinand_eccsr_gd(u8 status)
{
	return status >> 4 & 0x7;
}

/*
 * 	ECCSR[1:0]	ECC Status
 *	-------------------------------------------
 *	00		no bit errors were detected
 *	01		bit errors(1~4) corrected
 *	10		uncorrectable
 *	11		reserved
 */
static inline u8 ath79_spinand_eccsr_common(u8 status)
{
	return status >> 4 & 0x3;
}

static inline void ath79_spinand_read_rdm_addr_gd(u32 offset, u8 *addr)
{
	addr[0] = 0xff; /* dummy byte */
	addr[1] = (u8)(offset >> 8);
	addr[2] = (u8)(offset >> 0);
}

static inline void ath79_spinand_read_rdm_addr_common(u32 offset, u8 *addr)
{
	addr[0] = (u8)(offset >> 8);
	addr[1] = (u8)(offset >> 0);
	addr[2] = 0xff; /* dummy byte */
}

static inline int ath79_spinand_program_load_gd(struct spi_device *spi, u32 offset,
						u32 len, u8 *wbuf)
{
	int retval;

	retval = ath79_spinand_program_data_to_cache(spi, offset, len, wbuf);
	if (retval < 0) {
		dev_err(&spi->dev, "program data to cache failed\n");
		return retval;
	}

	retval = ath79_spinand_write_enable(spi);
	if (retval < 0) {
		dev_err(&spi->dev, "write enable failed!!\n");
		return retval;
	}

	if (ath79_wait_till_ready(spi)) {
		dev_err(&spi->dev, "wait timedout!!!\n");
		return -EBUSY;
	}

	return 0;
}

static inline int ath79_spinand_program_load_common(struct spi_device *spi, u32 offset,
						u32 len, u8 *wbuf)
{
	int retval;

	retval = ath79_spinand_write_enable(spi);
	if (retval < 0) {
		dev_err(&spi->dev, "write enable failed!!\n");
		return retval;
	}

	retval = ath79_spinand_program_data_to_cache(spi, offset, len, wbuf);
	if (retval < 0) {
		dev_err(&spi->dev, "program data to cache failed\n");
		return retval;
	}

	if (ath79_wait_till_ready(spi)) {
		dev_err(&spi->dev, "wait timedout!!!\n");
		return -EBUSY;
	}

	return 0;
}

static inline int __ath79_spinand_execute_cmd_common(struct spi_device *spi,
						     u32 page, u8 cmd_id)
{
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = cmd_id;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)(page >> 16);
	cmd.addr[1] = (u8)(page >> 8);
	cmd.addr[2] = (u8)(page >> 0);

	return ath79_spinand_cmd(spi, &cmd);
}

static inline int __ath79_spinand_execute_cmd_win(struct spi_device *spi,
						  u32 page, u8 cmd_id)
{
	struct ath79_spinand_command cmd = {0};

	cmd.cmd = cmd_id;
	cmd.n_addr = 3;
	cmd.addr[0] = 0;
	cmd.addr[1] = (u8)(page >> 8);
	cmd.addr[2] = (u8)(page >> 0);

	return ath79_spinand_cmd(spi, &cmd);
}

static inline int ath79_spinand_erase_block_erase_common(struct spi_device *spi_nand, u32 page)
{
	return __ath79_spinand_execute_cmd_common(spi_nand, page, ATH79_SPINAND_CMD_ERASE_BLK);
}

static inline int ath79_spinand_erase_block_erase_win(struct spi_device *spi_nand, u32 page)
{
	return __ath79_spinand_execute_cmd_win(spi_nand, page, ATH79_SPINAND_CMD_ERASE_BLK);
}

static inline int ath79_spinand_page_read_to_cache_common(struct spi_device *spi_nand, u32 page)
{
	return __ath79_spinand_execute_cmd_common(spi_nand, page, ATH79_SPINAND_CMD_READ);
}

static inline int ath79_spinand_page_read_to_cache_win(struct spi_device *spi_nand, u32 page)
{
	return __ath79_spinand_execute_cmd_win(spi_nand, page, ATH79_SPINAND_CMD_READ);
}

static inline int ath79_spinand_program_execute_common(struct spi_device *spi_nand, u32 page)
{
	return __ath79_spinand_execute_cmd_common(spi_nand, page, ATH79_SPINAND_CMD_PROG_PAGE_EXC);
}

static inline int ath79_spinand_program_execute_win(struct spi_device *spi_nand, u32 page)
{
	return __ath79_spinand_execute_cmd_win(spi_nand, page, ATH79_SPINAND_CMD_PROG_PAGE_EXC);
}

static struct ath79_spinand_priv ath79_spinand_ids[] = {
	{ /* Giga Device */
		NAND_MFR_GIGADEVICE,			/* manufacturer */
		0x07,					/* ecc error code */
		SZ_512,					/* ecc size */
		16,					/* ecc bytes */
		1,					/* ecc strength */
		&ath79_spinand_oob_128_gd,		/* ecc layout */
		&ath79_badblock_pattern_default, 	/* bad block pattern */
		ath79_spinand_eccsr_gd,			/* get ecc status */
		ath79_spinand_read_rdm_addr_gd,		/* wrap address for 03h command */
		ath79_spinand_program_load_gd,		/* program load data to cache */
		ath79_spinand_erase_block_erase_common,	/* erase block */
		ath79_spinand_page_read_to_cache_common,/* page read to cache */
		ath79_spinand_program_execute_common,	/* program execute */
	},
	{ /* Macronix */
		NAND_MFR_MACRONIX,			/* manufacturer*/
		0x02,					/* ecc error code */
		SZ_512,					/* ecc size */
		7,					/* ecc bytes */
		1,					/* ecc strength */
		&ath79_spinand_oob_64_mx,		/* ecc layout */
		&ath79_badblock_pattern_default,	/* bad block pattern */
		ath79_spinand_eccsr_common,		/* get ecc status */
		ath79_spinand_read_rdm_addr_common,	/* wrap address for 03h command */
		ath79_spinand_program_load_common,	/* program load data to cache */
		ath79_spinand_erase_block_erase_common,	/* erase block */
		ath79_spinand_page_read_to_cache_common,/* page read to cache */
		ath79_spinand_program_execute_common,	/* program execute */
	},
	{ /* Winbond */
		NAND_MFR_WINBOND,			/* manufacturer*/
		0x02,					/* ecc error code */
		SZ_512,					/* ecc size */
		10,					/* ecc bytes */
		1,					/* ecc strength */
		&ath79_spinand_oob_64_win,		/* ecc layout */
		&ath79_badblock_pattern_default,	/* bad block pattern */
		ath79_spinand_eccsr_common,		/* get ecc status */
		ath79_spinand_read_rdm_addr_common,	/* wrap address for 03h command */
		ath79_spinand_program_load_common,	/* program load data to cache */
		ath79_spinand_erase_block_erase_win,	/* erase block */
		ath79_spinand_page_read_to_cache_win,	/* page read to cache */
		ath79_spinand_program_execute_win,	/* program execute */
	},
	{ /* HeYang Tek (the same as Giga Device) */
		NAND_MFR_HEYANGTEK,			/* manufacturer */
		0x07,					/* ecc error code */
		SZ_512,					/* ecc size */
		16,					/* ecc bytes */
		1,					/* ecc strength */
		&ath79_spinand_oob_128_gd,		/* ecc layout */
		&ath79_badblock_pattern_default, 	/* bad block pattern */
		ath79_spinand_eccsr_gd,			/* get ecc status */
		ath79_spinand_read_rdm_addr_gd,		/* wrap address for 03h command */
		ath79_spinand_program_load_gd,		/* program load data to cache */
		ath79_spinand_erase_block_erase_common,	/* erase block */
		ath79_spinand_page_read_to_cache_common,/* page read to cache */
		ath79_spinand_program_execute_common,	/* program execute */
	},
};

static void *ath79_spinand_priv_data_get(struct spi_device *spi_nand)
{
	u8 id[3];
	int i;

	ath79_spinand_read_id(spi_nand, id);

	for (i = 0; i < ARRAY_SIZE(ath79_spinand_ids); i++)
		if (ath79_spinand_ids[i].mfr == id[0])
			return &ath79_spinand_ids[i];

	return NULL;
}

static int ath79_spinand_probe(struct spi_device *spi_nand)
{
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct ath79_spinand_info *info;
	struct ath79_spinand_state *state;
	struct ath79_spinand_priv *priv_data;

	priv_data = ath79_spinand_priv_data_get(spi_nand);
	if (!priv_data)
		return -ENXIO;

	info  = devm_kzalloc(&spi_nand->dev, sizeof(struct ath79_spinand_info),
			GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->spi = spi_nand;

	ath79_spinand_lock_block(spi_nand, ATH79_SPINAND_BL_ALL_UNLOCKED);
	ath79_spinand_disable_ecc(spi_nand);

	state = devm_kzalloc(&spi_nand->dev, sizeof(struct ath79_spinand_state),
			    GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	info->state	= state;
	info->priv	= priv_data;
	state->buf_ptr	= 0;
	state->buf	= devm_kzalloc(&spi_nand->dev, ATH79_SPINAND_BUF_SIZE, GFP_KERNEL);
	if (!state->buf)
		return -ENOMEM;

	chip = devm_kzalloc(&spi_nand->dev, sizeof(struct nand_chip),
			    GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->ecc.mode		= NAND_ECC_HW;
	chip->ecc.size		= priv_data->ecc_size;
	chip->ecc.bytes		= priv_data->ecc_bytes;
	chip->ecc.strength	= priv_data->ecc_strength;
	chip->ecc.layout	= priv_data->ecc_layout;
	chip->badblock_pattern	= priv_data->badblock_pattern;
	chip->ecc.read_page	= ath79_spinand_read_page_hwecc;
	chip->ecc.write_page	= ath79_spinand_write_page_hwecc;
	chip->priv		= info;
	chip->read_buf		= ath79_spinand_read_buf;
	chip->write_buf		= ath79_spinand_write_buf;
	chip->read_byte		= ath79_spinand_read_byte;
	chip->cmdfunc		= ath79_spinand_cmdfunc;
	chip->waitfunc		= ath79_spinand_wait;
	chip->options		= NAND_CACHEPRG | NAND_NO_SUBPAGE_WRITE;
	chip->select_chip	= ath79_spinand_select_chip;
	chip->block_markbad	= ath79_spinand_block_markbad;

	mtd = devm_kzalloc(&spi_nand->dev, sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd)
		return -ENOMEM;

	dev_set_drvdata(&spi_nand->dev, mtd);

	mtd->priv		= chip;
	mtd->name		= dev_name(&spi_nand->dev);
	mtd->owner		= THIS_MODULE;

	if (nand_scan(mtd, 1))
		return -ENXIO;

	ath79_spinand_enable_ecc(spi_nand);

	return mtd_device_parse_register(mtd, NULL, NULL, NULL, 0);
}

static int ath79_spinand_remove(struct spi_device *spi)
{
	mtd_device_unregister(dev_get_drvdata(&spi->dev));

	return 0;
}

static struct spi_driver ath79_spinand_driver = {
	.driver = {
		.name		= "ath79-spinand",
		.bus		= &spi_bus_type,
		.owner		= THIS_MODULE,
	},
	.probe		= ath79_spinand_probe,
	.remove		= ath79_spinand_remove,
};

module_spi_driver(ath79_spinand_driver);

MODULE_DESCRIPTION("SPI NAND driver for  Giga Device/Macronix");
MODULE_LICENSE("GPL v2");
