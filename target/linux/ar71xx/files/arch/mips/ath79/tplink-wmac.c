#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/mtd/mtd.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-wmac.h"
#include "dev-eth.h"

#define WMAC_BUILTIN	1
#define WMAC_AP91	2

static u8 wmac_mac1[6];
static int wmac1_register = 0;
static size_t wmac1_art_offset = 0;

static u8 wmac_mac2[6];
static int wmac2_register = 0;
static size_t wmac2_art_offset = 0;

__init void tplink_register_builtin_wmac1(size_t offset, u8 *mac, int mac_inc)
{
	wmac1_register = WMAC_BUILTIN;
	ath79_init_mac(wmac_mac1, mac, mac_inc);
	wmac1_art_offset = offset;
}

__init void tplink_register_builtin_wmac2(size_t offset, u8 *mac, int mac_inc)
{
	wmac2_register = WMAC_BUILTIN;
	ath79_init_mac(wmac_mac2, mac, mac_inc);
	wmac2_art_offset = offset;
}

__init void tplink_register_ap91_wmac1(size_t offset, u8 *mac, int mac_inc)
{
	wmac1_register = WMAC_AP91;
	ath79_init_mac(wmac_mac1, mac, mac_inc);
	wmac1_art_offset = offset;
}

__init void tplink_register_ap91_wmac2(size_t offset, u8 *mac, int mac_inc)
{
	wmac2_register = WMAC_AP91;
	ath79_init_mac(wmac_mac2, mac, mac_inc);
	wmac2_art_offset = offset;
}

__init static int tplink_builtin_wmac_post_register(void)
{
	struct mtd_info * mtd;
	size_t nb = 0;
	u8 *art;
	int ret;

	if (!wmac1_register && !wmac2_register)
		return 0;

	mtd = get_mtd_device_nm("art");
	if (IS_ERR(mtd))
		return PTR_ERR(mtd);

	if (mtd->size != 0x10000)
		return -1;

	if (wmac1_register)
	{
		art = kzalloc(0x1000, GFP_KERNEL);
		if (!art)
			return -1;

		ret = mtd_read(mtd, wmac1_art_offset, 0x1000, &nb, art);
		if (nb != 0x1000)
		{
			kfree(art);
			return ret;
		}

		switch (wmac1_register)
		{
		case WMAC_BUILTIN:
			ath79_register_wmac(art, wmac_mac1);
			break;
		case WMAC_AP91:
			ap91_pci_init(art, wmac_mac1);
			break;
		}
	}

	if (wmac2_register)
	{
		art = kzalloc(0x1000, GFP_KERNEL);
		if (!art)
			return -1;

		ret = mtd_read(mtd, wmac2_art_offset, 0x1000, &nb, art);
		if (nb != 0x1000)
		{
			kfree(art);
			return ret;
		}

		switch (wmac2_register)
		{
		case WMAC_BUILTIN:
			ath79_register_wmac(art, wmac_mac2);
			break;
		case WMAC_AP91:
			ap91_pci_init(art, wmac_mac2);
			break;
		}
	}

	return 0;
}

late_initcall(tplink_builtin_wmac_post_register);
