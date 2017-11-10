/*
  Copyright (c) 2014, Matthias Schiffer <mschiffer@universe-factory.net>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/*
   tplink-safeloader

   Image generation tool for the TP-LINK SafeLoader as seen on
   TP-LINK Pharos devices (CPE210/220/510/520)
*/


#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "md5.h"


#define ALIGN(x,a) ({ typeof(a) __a = (a); (((x) + __a - 1) & ~(__a - 1)); })


#define MAX_PARTITIONS	32

/** An image partition table entry */
struct image_partition_entry {
	const char *name;
	size_t size;
	uint8_t *data;
};

/** A flash partition table entry */
struct flash_partition_entry {
	const char *name;
	uint32_t base;
	uint32_t size;
};

/** Firmware layout description */
struct device_info {
	const char *id;
	const char *vendor;
	const char *support_list;
	char support_trail;
	const char *soft_ver;
	const struct flash_partition_entry partitions[MAX_PARTITIONS+1];
	const char *first_sysupgrade_partition;
	const char *last_sysupgrade_partition;
};

/** The content of the soft-version structure */
struct __attribute__((__packed__)) soft_version {
	uint32_t magic;
	uint32_t zero;
	uint8_t pad1;
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t version_patch;
	uint8_t year_hi;
	uint8_t year_lo;
	uint8_t month;
	uint8_t day;
	uint32_t rev;
	uint8_t pad2;
};


static const uint8_t jffs2_eof_mark[4] = {0xde, 0xad, 0xc0, 0xde};


/**
   Salt for the MD5 hash

   Fortunately, TP-LINK seems to use the same salt for most devices which use
   the new image format.
*/
static const uint8_t md5_salt[16] = {
	0x7a, 0x2b, 0x15, 0xed,
	0x9b, 0x98, 0x59, 0x6d,
	0xe5, 0x04, 0xab, 0x44,
	0xac, 0x2a, 0x9f, 0x4e,
};


/** Firmware layout table */
static struct device_info boards[] = {
	/** Firmware layout for the CPE210/220 */
	{
		.id	= "CPE210",
		.vendor	= "CPE510(TP-LINK|UN|N300-5):1.0\r\n",
		.support_list =
			"SupportList:\r\n"
			"CPE210(TP-LINK|UN|N300-2):1.0\r\n"
			"CPE210(TP-LINK|UN|N300-2):1.1\r\n"
			"CPE210(TP-LINK|US|N300-2):1.1\r\n"
			"CPE210(TP-LINK|EU|N300-2):1.1\r\n"
			"CPE220(TP-LINK|UN|N300-2):1.1\r\n"
			"CPE220(TP-LINK|US|N300-2):1.1\r\n"
			"CPE220(TP-LINK|EU|N300-2):1.1\r\n",
		.support_trail = '\xff',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"partition-table", 0x20000, 0x02000},
			{"default-mac", 0x30000, 0x00020},
			{"product-info", 0x31100, 0x00100},
			{"signature", 0x32000, 0x00400},
			{"os-image", 0x40000, 0x170000},
			{"soft-version", 0x1b0000, 0x00100},
			{"support-list", 0x1b1000, 0x00400},
			{"file-system", 0x1c0000, 0x600000},
			{"user-config", 0x7c0000, 0x10000},
			{"default-config", 0x7d0000, 0x10000},
			{"log", 0x7e0000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the CPE510/520 */
	{
		.id	= "CPE510",
		.vendor	= "CPE510(TP-LINK|UN|N300-5):1.0\r\n",
		.support_list =
			"SupportList:\r\n"
			"CPE510(TP-LINK|UN|N300-5):1.0\r\n"
			"CPE510(TP-LINK|UN|N300-5):1.1\r\n"
			"CPE510(TP-LINK|UN|N300-5):1.1\r\n"
			"CPE510(TP-LINK|US|N300-5):1.1\r\n"
			"CPE510(TP-LINK|EU|N300-5):1.1\r\n"
			"CPE520(TP-LINK|UN|N300-5):1.1\r\n"
			"CPE520(TP-LINK|US|N300-5):1.1\r\n"
			"CPE520(TP-LINK|EU|N300-5):1.1\r\n",
		.support_trail = '\xff',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"partition-table", 0x20000, 0x02000},
			{"default-mac", 0x30000, 0x00020},
			{"product-info", 0x31100, 0x00100},
			{"signature", 0x32000, 0x00400},
			{"os-image", 0x40000, 0x170000},
			{"soft-version", 0x1b0000, 0x00100},
			{"support-list", 0x1b1000, 0x00400},
			{"file-system", 0x1c0000, 0x600000},
			{"user-config", 0x7c0000, 0x10000},
			{"default-config", 0x7d0000, 0x10000},
			{"log", 0x7e0000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	{
		.id	= "WBS210",
		.vendor	= "CPE510(TP-LINK|UN|N300-5):1.0\r\n",
		.support_list =
			"SupportList:\r\n"
			"WBS210(TP-LINK|UN|N300-2):1.20\r\n"
			"WBS210(TP-LINK|US|N300-2):1.20\r\n"
			"WBS210(TP-LINK|EU|N300-2):1.20\r\n",
		.support_trail = '\xff',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"partition-table", 0x20000, 0x02000},
			{"default-mac", 0x30000, 0x00020},
			{"product-info", 0x31100, 0x00100},
			{"signature", 0x32000, 0x00400},
			{"os-image", 0x40000, 0x170000},
			{"soft-version", 0x1b0000, 0x00100},
			{"support-list", 0x1b1000, 0x00400},
			{"file-system", 0x1c0000, 0x600000},
			{"user-config", 0x7c0000, 0x10000},
			{"default-config", 0x7d0000, 0x10000},
			{"log", 0x7e0000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	{
		.id	= "WBS510",
		.vendor	= "CPE510(TP-LINK|UN|N300-5):1.0\r\n",
		.support_list =
			"SupportList:\r\n"
			"WBS510(TP-LINK|UN|N300-5):1.20\r\n"
			"WBS510(TP-LINK|US|N300-5):1.20\r\n"
			"WBS510(TP-LINK|EU|N300-5):1.20\r\n",
		.support_trail = '\xff',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"partition-table", 0x20000, 0x02000},
			{"default-mac", 0x30000, 0x00020},
			{"product-info", 0x31100, 0x00100},
			{"signature", 0x32000, 0x00400},
			{"os-image", 0x40000, 0x170000},
			{"soft-version", 0x1b0000, 0x00100},
			{"support-list", 0x1b1000, 0x00400},
			{"file-system", 0x1c0000, 0x600000},
			{"user-config", 0x7c0000, 0x10000},
			{"default-config", 0x7d0000, 0x10000},
			{"log", 0x7e0000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the C2600 */
	{
		.id = "C2600",
		.vendor = "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:Archer C2600,product_ver:1.0.0,special_id:00000000}\r\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		.partitions = {
			{"SBL1", 0x00000, 0x20000},
			{"MIBIB", 0x20000, 0x20000},
			{"SBL2", 0x40000, 0x20000},
			{"SBL3", 0x60000, 0x30000},
			{"DDRCONFIG", 0x90000, 0x10000},
			{"SSD", 0xa0000, 0x10000},
			{"TZ", 0xb0000, 0x30000},
			{"RPM", 0xe0000, 0x20000},
			{"fs-uboot", 0x100000, 0x70000},
			{"uboot-env", 0x170000, 0x40000},
			{"radio", 0x1b0000, 0x40000},
			{"os-image", 0x1f0000, 0x200000},
			{"file-system", 0x3f0000, 0x1b00000},
			{"default-mac", 0x1ef0000, 0x00200},
			{"pin", 0x1ef0200, 0x00200},
			{"product-info", 0x1ef0400, 0x0fc00},
			{"partition-table", 0x1f00000, 0x10000},
			{"soft-version", 0x1f10000, 0x10000},
			{"support-list", 0x1f20000, 0x10000},
			{"profile", 0x1f30000, 0x10000},
			{"default-config", 0x1f40000, 0x10000},
			{"user-config", 0x1f50000, 0x40000},
			{"qos-db", 0x1f90000, 0x40000},
			{"usb-config", 0x1fd0000, 0x10000},
			{"log", 0x1fe0000, 0x20000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the C25v1 */
	{
		.id = "ARCHER-C25-V1",
		.support_list =
			"SupportList:\n"
			"{product_name:ArcherC25,product_ver:1.0.0,special_id:00000000}\n"
			"{product_name:ArcherC25,product_ver:1.0.0,special_id:55530000}\n"
			"{product_name:ArcherC25,product_ver:1.0.0,special_id:45550000}\n",
		.support_trail = '\x00',
		.soft_ver = "soft_ver:1.0.0\n",

		/**
		    We use a bigger os-image partition than the stock images (and thus
		    smaller file-system), as our kernel doesn't fit in the stock firmware's
		    1MB os-image.
		*/
		.partitions = {
			{"factory-boot", 0x00000, 0x20000},
			{"fs-uboot", 0x20000, 0x10000},
			{"os-image", 0x30000, 0x180000},	/* Stock: base 0x30000 size 0x100000 */
			{"file-system", 0x1b0000, 0x620000},	/* Stock: base 0x130000 size 0x6a0000 */
			{"user-config", 0x7d0000, 0x04000},
			{"default-mac", 0x7e0000, 0x00100},
			{"device-id", 0x7e0100, 0x00100},
			{"extra-para", 0x7e0200, 0x00100},
			{"pin", 0x7e0300, 0x00100},
			{"support-list", 0x7e0400, 0x00400},
			{"soft-version", 0x7e0800, 0x00400},
			{"product-info", 0x7e0c00, 0x01400},
			{"partition-table", 0x7e2000, 0x01000},
			{"profile", 0x7e3000, 0x01000},
			{"default-config", 0x7e4000, 0x04000},
			{"merge-config", 0x7ec000, 0x02000},
			{"qos-db", 0x7ee000, 0x02000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the C58v1 */
	{
		.id	= "ARCHER-C58-V1",
		.vendor	= "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:Archer C58,product_ver:1.0.0,special_id:00000000}\r\n"
			"{product_name:Archer C58,product_ver:1.0.0,special_id:45550000}\r\n"
			"{product_name:Archer C58,product_ver:1.0.0,special_id:55530000}\r\n",
		.support_trail = '\x00',
		.soft_ver = "soft_ver:1.0.0\n",

		.partitions = {
			{"fs-uboot", 0x00000, 0x10000},
			{"default-mac", 0x10000, 0x00200},
			{"pin", 0x10200, 0x00200},
			{"product-info", 0x10400, 0x00100},
			{"partition-table", 0x10500, 0x00800},
			{"soft-version", 0x11300, 0x00200},
			{"support-list", 0x11500, 0x00100},
			{"device-id", 0x11600, 0x00100},
			{"profile", 0x11700, 0x03900},
			{"default-config", 0x15000, 0x04000},
			{"user-config", 0x19000, 0x04000},
			{"os-image", 0x20000, 0x150000},
			{"file-system", 0x170000, 0x678000},
			{"certyficate", 0x7e8000, 0x08000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the C59v1 */
	{
		.id	= "ARCHER-C59-V1",
		.vendor	= "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:Archer C59,product_ver:1.0.0,special_id:00000000}\r\n"
			"{product_name:Archer C59,product_ver:1.0.0,special_id:45550000}\r\n"
			"{product_name:Archer C59,product_ver:1.0.0,special_id:52550000}\r\n"
			"{product_name:Archer C59,product_ver:1.0.0,special_id:55530000}\r\n",
		.support_trail = '\x00',
		.soft_ver = "soft_ver:1.0.0\n",

		.partitions = {
			{"fs-uboot", 0x00000, 0x10000},
			{"default-mac", 0x10000, 0x00200},
			{"pin", 0x10200, 0x00200},
			{"device-id", 0x10400, 0x00100},
			{"product-info", 0x10500, 0x0fb00},
			{"os-image", 0x20000, 0x180000},
			{"file-system", 0x1a0000, 0xcb0000},
			{"partition-table", 0xe50000, 0x10000},
			{"soft-version", 0xe60000, 0x10000},
			{"support-list", 0xe70000, 0x10000},
			{"profile", 0xe80000, 0x10000},
			{"default-config", 0xe90000, 0x10000},
			{"user-config", 0xea0000, 0x40000},
			{"usb-config", 0xee0000, 0x10000},
			{"certificate", 0xef0000, 0x10000},
			{"qos-db", 0xf00000, 0x40000},
			{"log", 0xfe0000, 0x10000},
			{"radio", 0xff0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the C60v1 */
	{
		.id	= "ARCHER-C60-V1",
		.vendor	= "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:Archer C60,product_ver:1.0.0,special_id:00000000}\r\n"
			"{product_name:Archer C60,product_ver:1.0.0,special_id:45550000}\r\n"
			"{product_name:Archer C60,product_ver:1.0.0,special_id:55530000}\r\n",
		.support_trail = '\x00',
		.soft_ver = "soft_ver:1.0.0\n",

		.partitions = {
			{"fs-uboot", 0x00000, 0x10000},
			{"default-mac", 0x10000, 0x00200},
			{"pin", 0x10200, 0x00200},
			{"product-info", 0x10400, 0x00100},
			{"partition-table", 0x10500, 0x00800},
			{"soft-version", 0x11300, 0x00200},
			{"support-list", 0x11500, 0x00100},
			{"device-id", 0x11600, 0x00100},
			{"profile", 0x11700, 0x03900},
			{"default-config", 0x15000, 0x04000},
			{"user-config", 0x19000, 0x04000},
			{"os-image", 0x20000, 0x150000},
			{"file-system", 0x170000, 0x678000},
			{"certyficate", 0x7e8000, 0x08000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the C5 */
	{
		.id = "ARCHER-C5-V2",
		.vendor = "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:ArcherC5,product_ver:2.0.0,special_id:00000000}\r\n"
			"{product_name:ArcherC5,product_ver:2.0.0,special_id:55530000}\r\n"
			"{product_name:ArcherC5,product_ver:2.0.0,special_id:4A500000}\r\n", /* JP version */
		.support_trail = '\x00',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x40000},
			{"os-image", 0x40000, 0x200000},
			{"file-system", 0x240000, 0xc00000},
			{"default-mac", 0xe40000, 0x00200},
			{"pin", 0xe40200, 0x00200},
			{"product-info", 0xe40400, 0x00200},
			{"partition-table", 0xe50000, 0x10000},
			{"soft-version", 0xe60000, 0x00200},
			{"support-list", 0xe61000, 0x0f000},
			{"profile", 0xe70000, 0x10000},
			{"default-config", 0xe80000, 0x10000},
			{"user-config", 0xe90000, 0x50000},
			{"log", 0xee0000, 0x100000},
			{"radio_bk", 0xfe0000, 0x10000},
			{"radio", 0xff0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the C7 */
	{
		.id = "ARCHER-C7-V4",
		.support_list =
			"SupportList:\n"
			"{product_name:Archer C7,product_ver:4.0.0,special_id:45550000}\n"
			"{product_name:Archer C7,product_ver:4.0.0,special_id:55530000}\n"
			"{product_name:Archer C7,product_ver:4.0.0,special_id:43410000}\n",
		.support_trail = '\x00',
		.soft_ver = "soft_ver:1.0.0\n",

		/**
		    We use a bigger os-image partition than the stock images (and thus
		    smaller file-system), as our kernel doesn't fit in the stock firmware's
		    1MB os-image.
		*/
		.partitions = {
			{"factory-boot", 0x00000, 0x20000},
			{"fs-uboot", 0x20000, 0x20000},
			{"os-image", 0x40000, 0x180000},	/* Stock: base 0x40000 size 0x120000 */
			{"file-system", 0x1c0000, 0xd40000},	/* Stock: base 0x160000 size 0xda0000 */
			{"default-mac", 0xf00000, 0x00200},
			{"pin", 0xf00200, 0x00200},
			{"device-id", 0xf00400, 0x00100},
			{"product-info", 0xf00500, 0x0fb00},
			{"soft-version", 0xf10000, 0x00100},
			{"extra-para", 0xf11000, 0x01000},
			{"support-list", 0xf12000, 0x0a000},
			{"profile", 0xf1c000, 0x04000},
			{"default-config", 0xf20000, 0x10000},
			{"user-config", 0xf30000, 0x40000},
			{"qos-db", 0xf70000, 0x40000},
			{"certificate", 0xfb0000, 0x10000},
			{"partition-table", 0xfc0000, 0x10000},
			{"log", 0xfd0000, 0x20000},
			{"radio", 0xff0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the C9 */
	{
		.id = "ARCHERC9",
		.vendor = "",
		.support_list =
			"SupportList:\n"
			"{product_name:ArcherC9,"
			"product_ver:1.0.0,"
			"special_id:00000000}\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x40000},
			{"os-image", 0x40000, 0x200000},
			{"file-system", 0x240000, 0xc00000},
			{"default-mac", 0xe40000, 0x00200},
			{"pin", 0xe40200, 0x00200},
			{"product-info", 0xe40400, 0x00200},
			{"partition-table", 0xe50000, 0x10000},
			{"soft-version", 0xe60000, 0x00200},
			{"support-list", 0xe61000, 0x0f000},
			{"profile", 0xe70000, 0x10000},
			{"default-config", 0xe80000, 0x10000},
			{"user-config", 0xe90000, 0x50000},
			{"log", 0xee0000, 0x100000},
			{"radio_bk", 0xfe0000, 0x10000},
			{"radio", 0xff0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the EAP120 */
	{
		.id     = "EAP120",
		.vendor = "EAP120(TP-LINK|UN|N300-2):1.0\r\n",
		.support_list =
			"SupportList:\r\n"
			"EAP120(TP-LINK|UN|N300-2):1.0\r\n",
		.support_trail = '\xff',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"partition-table", 0x20000, 0x02000},
			{"default-mac", 0x30000, 0x00020},
			{"support-list", 0x31000, 0x00100},
			{"product-info", 0x31100, 0x00100},
			{"soft-version", 0x32000, 0x00100},
			{"os-image", 0x40000, 0x180000},
			{"file-system", 0x1c0000, 0x600000},
			{"user-config", 0x7c0000, 0x10000},
			{"backup-config", 0x7d0000, 0x10000},
			{"log", 0x7e0000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the TL-WA850RE v2 */
	{
		.id     = "TLWA850REV2",
		.vendor = "",
		.support_list =
			"SupportList:\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:55530000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:00000000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:55534100}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:45550000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:4B520000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:42520000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:4A500000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:43410000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:41550000}\n"
			"{product_name:TL-WA850RE,product_ver:2.0.0,special_id:52550000}\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		/**
		   576KB were moved from file-system to os-image
		   in comparison to the stock image
		*/
		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x150000},
			{"file-system", 0x170000, 0x240000},
			{"partition-table", 0x3b0000, 0x02000},
			{"default-mac", 0x3c0000, 0x00020},
			{"pin", 0x3c0100, 0x00020},
			{"product-info", 0x3c1000, 0x01000},
			{"soft-version", 0x3c2000, 0x00100},
			{"support-list", 0x3c3000, 0x01000},
			{"profile", 0x3c4000, 0x08000},
			{"user-config", 0x3d0000, 0x10000},
			{"default-config", 0x3e0000, 0x10000},
			{"radio", 0x3f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the TL-WA855RE v1 */
	{
		.id     = "TLWA855REV1",
		.vendor = "",
		.support_list =
			"SupportList:\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:00000000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:55530000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:45550000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:4B520000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:42520000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:4A500000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:43410000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:41550000}\n"
			"{product_name:TL-WA855RE,product_ver:1.0.0,special_id:52550000}\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x150000},
			{"file-system", 0x170000, 0x240000},
			{"partition-table", 0x3b0000, 0x02000},
			{"default-mac", 0x3c0000, 0x00020},
			{"pin", 0x3c0100, 0x00020},
			{"product-info", 0x3c1000, 0x01000},
			{"soft-version", 0x3c2000, 0x00100},
			{"support-list", 0x3c3000, 0x01000},
			{"profile", 0x3c4000, 0x08000},
			{"user-config", 0x3d0000, 0x10000},
			{"default-config", 0x3e0000, 0x10000},
			{"radio", 0x3f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the TL-WR1043 v4 */
	{
		.id     = "TLWR1043NDV4",
		.vendor = "",
		.support_list =
			"SupportList:\n"
			"{product_name:TL-WR1043ND,product_ver:4.0.0,special_id:45550000}\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		/**
		    We use a bigger os-image partition than the stock images (and thus
		    smaller file-system), as our kernel doesn't fit in the stock firmware's
		    1MB os-image.
		*/
		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x180000},
			{"file-system", 0x1a0000, 0xdb0000},
			{"default-mac", 0xf50000, 0x00200},
			{"pin", 0xf50200, 0x00200},
			{"product-info", 0xf50400, 0x0fc00},
			{"soft-version", 0xf60000, 0x0b000},
			{"support-list", 0xf6b000, 0x04000},
			{"profile", 0xf70000, 0x04000},
			{"default-config", 0xf74000, 0x0b000},
			{"user-config", 0xf80000, 0x40000},
			{"partition-table", 0xfc0000, 0x10000},
			{"log", 0xfd0000, 0x20000},
			{"radio", 0xff0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the TL-WR902AC v1 */
	{
		.id     = "TL-WR902AC-V1",
		.vendor = "",
		.support_list =
			"SupportList:\n"
			"{product_name:TL-WR902AC,product_ver:1.0.0,special_id:45550000}\n"
			"{product_name:TL-WR902AC,product_ver:1.0.0,special_id:55530000}\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		/**
		   384KB were moved from file-system to os-image
		   in comparison to the stock image
		*/
		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x160000},
			{"file-system", 0x180000, 0x5d0000},
			{"default-mac", 0x750000, 0x00200},
			{"pin", 0x750200, 0x00200},
			{"product-info", 0x750400, 0x0fc00},
			{"soft-version", 0x760000, 0x0b000},
			{"support-list", 0x76b000, 0x04000},
			{"profile", 0x770000, 0x04000},
			{"default-config", 0x774000, 0x0b000},
			{"user-config", 0x780000, 0x40000},
			{"partition-table", 0x7c0000, 0x10000},
			{"log", 0x7d0000, 0x20000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the TL-WR942N V1 */
	{
		.id     = "TLWR942NV1",
		.vendor = "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:TL-WR942N,product_ver:1.0.0,special_id:00000000}\r\n"
			"{product_name:TL-WR942N,product_ver:1.0.0,special_id:52550000}\r\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x150000},
			{"file-system", 0x170000, 0xcd0000},
			{"default-mac", 0xe40000, 0x00200},
			{"pin", 0xe40200, 0x00200},
			{"product-info", 0xe40400, 0x0fc00},
			{"partition-table", 0xe50000, 0x10000},
			{"soft-version", 0xe60000, 0x10000},
			{"support-list", 0xe70000, 0x10000},
			{"profile", 0xe80000, 0x10000},
			{"default-config", 0xe90000, 0x10000},
			{"user-config", 0xea0000, 0x40000},
			{"qos-db", 0xee0000, 0x40000},
			{"certificate", 0xf20000, 0x10000},
			{"usb-config", 0xfb0000, 0x10000},
			{"log", 0xfc0000, 0x20000},
			{"radio-bk", 0xfe0000, 0x10000},
			{"radio", 0xff0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system",
	},

	/** Firmware layout for the RE350 v1 */
	{
		.id = "RE350-V1",
		.vendor = "",
		.support_list =
			"SupportList:\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:45550000}\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:00000000}\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:41550000}\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:55530000}\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:43410000}\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:4b520000}\n"
			"{product_name:RE350,product_ver:1.0.0,special_id:4a500000}\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		/**
			The original os-image partition is too small,
			so we enlarge it to 1.6M
		*/
		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x1a0000},
			{"file-system", 0x1c0000, 0x440000},
			{"partition-table", 0x600000, 0x02000},
			{"default-mac", 0x610000, 0x00020},
			{"pin", 0x610100, 0x00020},
			{"product-info", 0x611100, 0x01000},
			{"soft-version", 0x620000, 0x01000},
			{"support-list", 0x621000, 0x01000},
			{"profile", 0x622000, 0x08000},
			{"user-config", 0x630000, 0x10000},
			{"default-config", 0x640000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	/** Firmware layout for the RE450 */
	{
		.id = "RE450",
		.vendor = "",
		.support_list =
			"SupportList:\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:00000000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:55530000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:45550000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:4A500000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:43410000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:41550000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:4B520000}\r\n"
			"{product_name:RE450,product_ver:1.0.0,special_id:55534100}\r\n",
		.support_trail = '\x00',
		.soft_ver = NULL,

		/**
		   The flash partition table for RE450;
		   it is almost the same as the one used by the stock images,
		   576KB were moved from file-system to os-image.
		*/
		.partitions = {
			{"fs-uboot", 0x00000, 0x20000},
			{"os-image", 0x20000, 0x150000},
			{"file-system", 0x170000, 0x4a0000},
			{"partition-table", 0x600000, 0x02000},
			{"default-mac", 0x610000, 0x00020},
			{"pin", 0x610100, 0x00020},
			{"product-info", 0x611100, 0x01000},
			{"soft-version", 0x620000, 0x01000},
			{"support-list", 0x621000, 0x01000},
			{"profile", 0x622000, 0x08000},
			{"user-config", 0x630000, 0x10000},
			{"default-config", 0x640000, 0x10000},
			{"radio", 0x7f0000, 0x10000},
			{NULL, 0, 0}
		},

		.first_sysupgrade_partition = "os-image",
		.last_sysupgrade_partition = "file-system"
	},

	{}
};

#define error(_ret, _errno, _str, ...)				\
	do {							\
		fprintf(stderr, _str ": %s\n", ## __VA_ARGS__,	\
			strerror(_errno));			\
		if (_ret)					\
			exit(_ret);				\
	} while (0)


/** Stores a uint32 as big endian */
static inline void put32(uint8_t *buf, uint32_t val) {
	buf[0] = val >> 24;
	buf[1] = val >> 16;
	buf[2] = val >> 8;
	buf[3] = val;
}

/** Allocates a new image partition */
static struct image_partition_entry alloc_image_partition(const char *name, size_t len) {
	struct image_partition_entry entry = {name, len, malloc(len)};
	if (!entry.data)
		error(1, errno, "malloc");

	return entry;
}

/** Frees an image partition */
static void free_image_partition(struct image_partition_entry entry) {
	free(entry.data);
}

static time_t source_date_epoch = -1;
static void set_source_date_epoch() {
	char *env = getenv("SOURCE_DATE_EPOCH");
	char *endptr = env;
	errno = 0;
        if (env && *env) {
		source_date_epoch = strtoull(env, &endptr, 10);
		if (errno || (endptr && *endptr != '\0')) {
			fprintf(stderr, "Invalid SOURCE_DATE_EPOCH");
			exit(1);
		}
        }
}

/** Generates the partition-table partition */
static struct image_partition_entry make_partition_table(const struct flash_partition_entry *p) {
	struct image_partition_entry entry = alloc_image_partition("partition-table", 0x800);

	char *s = (char *)entry.data, *end = (char *)(s+entry.size);

	*(s++) = 0x00;
	*(s++) = 0x04;
	*(s++) = 0x00;
	*(s++) = 0x00;

	size_t i;
	for (i = 0; p[i].name; i++) {
		size_t len = end-s;
		size_t w = snprintf(s, len, "partition %s base 0x%05x size 0x%05x\n", p[i].name, p[i].base, p[i].size);

		if (w > len-1)
			error(1, 0, "flash partition table overflow?");

		s += w;
	}

	s++;

	memset(s, 0xff, end-s);

	return entry;
}


/** Generates a binary-coded decimal representation of an integer in the range [0, 99] */
static inline uint8_t bcd(uint8_t v) {
	return 0x10 * (v/10) + v%10;
}


/** Generates the soft-version partition */
static struct image_partition_entry make_soft_version(uint32_t rev) {
	struct image_partition_entry entry = alloc_image_partition("soft-version", sizeof(struct soft_version));
	struct soft_version *s = (struct soft_version *)entry.data;

	time_t t;

	if (source_date_epoch != -1)
		t = source_date_epoch;
	else if (time(&t) == (time_t)(-1))
		error(1, errno, "time");

	struct tm *tm = localtime(&t);

	s->magic = htonl(0x0000000c);
	s->zero = 0;
	s->pad1 = 0xff;

	s->version_major = 0;
	s->version_minor = 0;
	s->version_patch = 0;

	s->year_hi = bcd((1900+tm->tm_year)/100);
	s->year_lo = bcd(tm->tm_year%100);
	s->month = bcd(tm->tm_mon+1);
	s->day = bcd(tm->tm_mday);
	s->rev = htonl(rev);

	s->pad2 = 0xff;

	return entry;
}

static struct image_partition_entry make_soft_version_from_string(const char *soft_ver) {
	/** String length _including_ the terminating zero byte */
	uint32_t ver_len = strlen(soft_ver) + 1;
	/** Partition contains 64 bit header, the version string, and one additional null byte */
	size_t partition_len = 2*sizeof(uint32_t) + ver_len + 1;
	struct image_partition_entry entry = alloc_image_partition("soft-version", partition_len);

	uint32_t *len = (uint32_t *)entry.data;
	len[0] = htonl(ver_len);
	len[1] = 0;
	memcpy(&len[2], soft_ver, ver_len);

	entry.data[partition_len - 1] = 0;

	return entry;
}

/** Generates the support-list partition */
static struct image_partition_entry make_support_list(const struct device_info *info) {
	size_t len = strlen(info->support_list);
	struct image_partition_entry entry = alloc_image_partition("support-list", len + 9);

	put32(entry.data, len);
	memset(entry.data+4, 0, 4);
	memcpy(entry.data+8, info->support_list, len);
	entry.data[len+8] = info->support_trail;

	return entry;
}

/** Creates a new image partition with an arbitrary name from a file */
static struct image_partition_entry read_file(const char *part_name, const char *filename, bool add_jffs2_eof) {
	struct stat statbuf;

	if (stat(filename, &statbuf) < 0)
		error(1, errno, "unable to stat file `%s'", filename);

	size_t len = statbuf.st_size;

	if (add_jffs2_eof)
		len = ALIGN(len, 0x10000) + sizeof(jffs2_eof_mark);

	struct image_partition_entry entry = alloc_image_partition(part_name, len);

	FILE *file = fopen(filename, "rb");
	if (!file)
		error(1, errno, "unable to open file `%s'", filename);

	if (fread(entry.data, statbuf.st_size, 1, file) != 1)
		error(1, errno, "unable to read file `%s'", filename);

	if (add_jffs2_eof) {
		uint8_t *eof = entry.data + statbuf.st_size, *end = entry.data+entry.size;

		memset(eof, 0xff, end - eof - sizeof(jffs2_eof_mark));
		memcpy(end - sizeof(jffs2_eof_mark), jffs2_eof_mark, sizeof(jffs2_eof_mark));
	}

	fclose(file);

	return entry;
}

/** Creates a new image partition from arbitrary data */
static struct image_partition_entry put_data(const char *part_name, const char *datain, size_t len) {

	struct image_partition_entry entry = alloc_image_partition(part_name, len);

	memcpy(entry.data, datain, len);

	return entry;
}

/**
   Copies a list of image partitions into an image buffer and generates the image partition table while doing so

   Example image partition table:

     fwup-ptn partition-table base 0x00800 size 0x00800
     fwup-ptn os-image base 0x01000 size 0x113b45
     fwup-ptn file-system base 0x114b45 size 0x1d0004
     fwup-ptn support-list base 0x2e4b49 size 0x000d1

   Each line of the partition table is terminated with the bytes 09 0d 0a ("\t\r\n"),
   the end of the partition table is marked with a zero byte.

   The firmware image must contain at least the partition-table and support-list partitions
   to be accepted. There aren't any alignment constraints for the image partitions.

   The partition-table partition contains the actual flash layout; partitions
   from the image partition table are mapped to the corresponding flash partitions during
   the firmware upgrade. The support-list partition contains a list of devices supported by
   the firmware image.

   The base offsets in the firmware partition table are relative to the end
   of the vendor information block, so the partition-table partition will
   actually start at offset 0x1814 of the image.

   I think partition-table must be the first partition in the firmware image.
*/
static void put_partitions(uint8_t *buffer, const struct flash_partition_entry *flash_parts, const struct image_partition_entry *parts) {
	size_t i, j;
	char *image_pt = (char *)buffer, *end = image_pt + 0x800;

	size_t base = 0x800;
	for (i = 0; parts[i].name; i++) {
		for (j = 0; flash_parts[j].name; j++) {
			if (!strcmp(flash_parts[j].name, parts[i].name)) {
				if (parts[i].size > flash_parts[j].size)
					error(1, 0, "%s partition too big (more than %u bytes)", flash_parts[j].name, (unsigned)flash_parts[j].size);
				break;
			}
		}

		assert(flash_parts[j].name);

		memcpy(buffer + base, parts[i].data, parts[i].size);

		size_t len = end-image_pt;
		size_t w = snprintf(image_pt, len, "fwup-ptn %s base 0x%05x size 0x%05x\t\r\n", parts[i].name, (unsigned)base, (unsigned)parts[i].size);

		if (w > len-1)
			error(1, 0, "image partition table overflow?");

		image_pt += w;

		base += parts[i].size;
	}
}

/** Generates and writes the image MD5 checksum */
static void put_md5(uint8_t *md5, uint8_t *buffer, unsigned int len) {
	MD5_CTX ctx;

	MD5_Init(&ctx);
	MD5_Update(&ctx, md5_salt, (unsigned int)sizeof(md5_salt));
	MD5_Update(&ctx, buffer, len);
	MD5_Final(md5, &ctx);
}


/**
   Generates the firmware image in factory format

   Image format:

     Bytes (hex)  Usage
     -----------  -----
     0000-0003    Image size (4 bytes, big endian)
     0004-0013    MD5 hash (hash of a 16 byte salt and the image data starting with byte 0x14)
     0014-0017    Vendor information length (without padding) (4 bytes, big endian)
     0018-1013    Vendor information (4092 bytes, padded with 0xff; there seem to be older
                  (VxWorks-based) TP-LINK devices which use a smaller vendor information block)
     1014-1813    Image partition table (2048 bytes, padded with 0xff)
     1814-xxxx    Firmware partitions
*/
static void * generate_factory_image(const struct device_info *info, const struct image_partition_entry *parts, size_t *len) {
	*len = 0x1814;

	size_t i;
	for (i = 0; parts[i].name; i++)
		*len += parts[i].size;

	uint8_t *image = malloc(*len);
	if (!image)
		error(1, errno, "malloc");

	memset(image, 0xff, *len);
	put32(image, *len);

	if (info->vendor) {
		size_t vendor_len = strlen(info->vendor);
		put32(image+0x14, vendor_len);
		memcpy(image+0x18, info->vendor, vendor_len);
	}

	put_partitions(image + 0x1014, info->partitions, parts);
	put_md5(image+0x04, image+0x14, *len-0x14);

	return image;
}

/**
   Generates the firmware image in sysupgrade format

   This makes some assumptions about the provided flash and image partition tables and
   should be generalized when TP-LINK starts building its safeloader into hardware with
   different flash layouts.
*/
static void * generate_sysupgrade_image(const struct device_info *info, const struct image_partition_entry *image_parts, size_t *len) {
	size_t i, j;
	size_t flash_first_partition_index = 0;
	size_t flash_last_partition_index = 0;
	const struct flash_partition_entry *flash_first_partition = NULL;
	const struct flash_partition_entry *flash_last_partition = NULL;
	const struct image_partition_entry *image_last_partition = NULL;

	/** Find first and last partitions */
	for (i = 0; info->partitions[i].name; i++) {
		if (!strcmp(info->partitions[i].name, info->first_sysupgrade_partition)) {
			flash_first_partition = &info->partitions[i];
			flash_first_partition_index = i;
		} else if (!strcmp(info->partitions[i].name, info->last_sysupgrade_partition)) {
			flash_last_partition = &info->partitions[i];
			flash_last_partition_index = i;
		}
	}

	assert(flash_first_partition && flash_last_partition);
	assert(flash_first_partition_index < flash_last_partition_index);

	/** Find last partition from image to calculate needed size */
	for (i = 0; image_parts[i].name; i++) {
		if (!strcmp(image_parts[i].name, info->last_sysupgrade_partition)) {
			image_last_partition = &image_parts[i];
			break;
		}
	}

	assert(image_last_partition);

	*len = flash_last_partition->base - flash_first_partition->base + image_last_partition->size;

	uint8_t *image = malloc(*len);
	if (!image)
		error(1, errno, "malloc");

	memset(image, 0xff, *len);

	for (i = flash_first_partition_index; i <= flash_last_partition_index; i++) {
		for (j = 0; image_parts[j].name; j++) {
			if (!strcmp(info->partitions[i].name, image_parts[j].name)) {
				if (image_parts[j].size > info->partitions[i].size)
					error(1, 0, "%s partition too big (more than %u bytes)", info->partitions[i].name, (unsigned)info->partitions[i].size);
				memcpy(image + info->partitions[i].base - flash_first_partition->base, image_parts[j].data, image_parts[j].size);
				break;
			}

			assert(image_parts[j].name);
		}
	}

	return image;
}

/** Generates an image according to a given layout and writes it to a file */
static void build_image(const char *output,
		const char *kernel_image,
		const char *rootfs_image,
		uint32_t rev,
		bool add_jffs2_eof,
		bool sysupgrade,
		const struct device_info *info) {

	struct image_partition_entry parts[7] = {};

	parts[0] = make_partition_table(info->partitions);
	if (info->soft_ver)
		parts[1] = make_soft_version_from_string(info->soft_ver);
	else
		parts[1] = make_soft_version(rev);

	parts[2] = make_support_list(info);
	parts[3] = read_file("os-image", kernel_image, false);
	parts[4] = read_file("file-system", rootfs_image, add_jffs2_eof);

	if (strcasecmp(info->id, "ARCHER-C25-V1") == 0) {
		const char mdat[11] = {0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
		parts[5] = put_data("extra-para", mdat, 11);
	} else if (strcasecmp(info->id, "ARCHER-C7-V4") == 0) {
		const char mdat[11] = {0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0xca, 0x00, 0x01, 0x00, 0x00};
		parts[5] = put_data("extra-para", mdat, 11);
	}

	size_t len;
	void *image;
	if (sysupgrade)
		image = generate_sysupgrade_image(info, parts, &len);
	else
		image = generate_factory_image(info, parts, &len);

	FILE *file = fopen(output, "wb");
	if (!file)
		error(1, errno, "unable to open output file");

	if (fwrite(image, len, 1, file) != 1)
		error(1, 0, "unable to write output file");

	fclose(file);

	free(image);

	size_t i;
	for (i = 0; parts[i].name; i++)
		free_image_partition(parts[i]);
}

/** Usage output */
static void usage(const char *argv0) {
	fprintf(stderr,
		"Usage: %s [OPTIONS...]\n"
		"\n"
		"Options:\n"
		"  -B <board>      create image for the board specified with <board>\n"
		"  -k <file>       read kernel image from the file <file>\n"
		"  -r <file>       read rootfs image from the file <file>\n"
		"  -o <file>       write output to the file <file>\n"
		"  -V <rev>        sets the revision number to <rev>\n"
		"  -j              add jffs2 end-of-filesystem markers\n"
		"  -S              create sysupgrade instead of factory image\n"
		"  -h              show this help\n",
		argv0
	);
};


static const struct device_info *find_board(const char *id)
{
	struct device_info *board = NULL;

	for (board = boards; board->id != NULL; board++)
		if (strcasecmp(id, board->id) == 0)
			return board;

	return NULL;
}

int main(int argc, char *argv[]) {
	const char *board = NULL, *kernel_image = NULL, *rootfs_image = NULL, *output = NULL;
	bool add_jffs2_eof = false, sysupgrade = false;
	unsigned rev = 0;
	const struct device_info *info;
	set_source_date_epoch();

	while (true) {
		int c;

		c = getopt(argc, argv, "B:k:r:o:V:jSh");
		if (c == -1)
			break;

		switch (c) {
		case 'B':
			board = optarg;
			break;

		case 'k':
			kernel_image = optarg;
			break;

		case 'r':
			rootfs_image = optarg;
			break;

		case 'o':
			output = optarg;
			break;

		case 'V':
			sscanf(optarg, "r%u", &rev);
			break;

		case 'j':
			add_jffs2_eof = true;
			break;

		case 'S':
			sysupgrade = true;
			break;

		case 'h':
			usage(argv[0]);
			return 0;

		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (!board)
		error(1, 0, "no board has been specified");
	if (!kernel_image)
		error(1, 0, "no kernel image has been specified");
	if (!rootfs_image)
		error(1, 0, "no rootfs image has been specified");
	if (!output)
		error(1, 0, "no output filename has been specified");

	info = find_board(board);

	if (info == NULL)
		error(1, 0, "unsupported board %s", board);

	build_image(output, kernel_image, rootfs_image, rev, add_jffs2_eof, sysupgrade, info);

	return 0;
}
