/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * Filename : version.h
 * Abstract : This file is a general definition for driver version
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SPRDWL_VERSION_H
#define SPRDWL_VERSION_H

#define SPRDWL_DRIVER_VERSION "Marlin3"
#define SPRDWL_UPDATE "000e"
#define SPRDWL_RESERVE ""
#define MAIN_DRV_VERSION (1)
#define MAX_API			(256)
#define DEFAULT_COMPAT (255)
#ifdef COMPAT_SAMPILE_CODE
#define VERSION_1 (1)
#define VERSION_2 (2)
#define VERSION_3 (3)
#define VERSION_4 (4)
#endif

struct sprdwl_ver {
	char kernel_ver[8];
	char drv_ver[8];
	char update[8];
	char reserve[8];
};

struct api_version_t {
	unsigned char cmd_id;
	unsigned char drv_version;
	unsigned char fw_version;
};

/*struct used for priv to store all info*/
struct sync_api_verion_t {
	unsigned int compat;
	unsigned int main_drv;
	unsigned int main_fw;
	struct api_version_t *api_array;
};

#endif
