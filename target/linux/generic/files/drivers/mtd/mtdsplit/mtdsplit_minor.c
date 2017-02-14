/*
 *  Copyright (C) 2017 Thibaut VARENE <varenet@parisc-linux.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/byteorder/generic.h>
#include <linux/string.h>

#include "mtdsplit.h"

#define YAFFS_OBJECT_TYPE_FILE	0x1
#define YAFFS_OBJECTID_ROOT	0x1
#define YAFFS_SUM_UNUSED	0xFFFF
#define YAFFS_NAME		"kernel"

#define MINOR_NR_PARTS		2

struct minor_header {
	__be32 yaffs_type;
	__be32 yaffs_obj_id;
	__be16 yaffs_sum_unused;
	char yaffs_name[sizeof(YAFFS_NAME)];
};

static int mtdsplit_parse_minor(struct mtd_info *master,
				const struct mtd_partition **pparts,
				struct mtd_part_parser_data *data)
{
	struct minor_header hdr;
	size_t hdr_len, retlen;
	size_t rootfs_offset;
	struct mtd_partition *parts;
	int err;

	hdr_len = sizeof(hdr);
	err = mtd_read(master, 0, hdr_len, &retlen, (void *) &hdr);
	if (err)
		return err;

	if (retlen != hdr_len)
		return -EIO;

	/* match header */
	if (be32_to_cpu(hdr.yaffs_type) != YAFFS_OBJECT_TYPE_FILE)
		return -EINVAL;

	if (be32_to_cpu(hdr.yaffs_obj_id) != YAFFS_OBJECTID_ROOT)
		return -EINVAL;

	if (be16_to_cpu(hdr.yaffs_sum_unused) != YAFFS_SUM_UNUSED)
		return -EINVAL;

	if (memcmp(hdr.yaffs_name, YAFFS_NAME, sizeof(YAFFS_NAME)))
		return -EINVAL;

	err = mtd_find_rootfs_from(master, master->erasesize, master->size,
				   &rootfs_offset, NULL);
	if (err)
		return err;

	parts = kzalloc(MINOR_NR_PARTS * sizeof(*parts), GFP_KERNEL);
	if (!parts)
		return -ENOMEM;

	parts[0].name = KERNEL_PART_NAME;
	parts[0].offset = 0;
	parts[0].size = rootfs_offset;

	parts[1].name = ROOTFS_PART_NAME;
	parts[1].offset = rootfs_offset;
	parts[1].size = master->size - rootfs_offset;

	*pparts = parts;
	return MINOR_NR_PARTS;
}

static struct mtd_part_parser mtdsplit_minor_parser = {
	.owner = THIS_MODULE,
	.name = "minor-fw",
	.parse_fn = mtdsplit_parse_minor,
	.type = MTD_PARSER_TYPE_FIRMWARE,
};

static int __init mtdsplit_minor_init(void)
{
	register_mtd_parser(&mtdsplit_minor_parser);

	return 0;
}

subsys_initcall(mtdsplit_minor_init);
