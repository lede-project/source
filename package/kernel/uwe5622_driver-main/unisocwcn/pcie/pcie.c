/*
 * Copyright (C) 2016-2018 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/msi.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <wcn_bus.h>

#include "edma_engine.h"
#include "ioctl.h"
#include "mchn.h"
#include "pcie.h"
#include "pcie_dbg.h"
#include "wcn_log.h"
#include "wcn_misc.h"
#include "wcn_op.h"
#include "wcn_procfs.h"

static struct wcn_pcie_info *g_pcie_dev;

struct wcn_pcie_info *get_wcn_device_info(void)
{
	return g_pcie_dev;
}

static int sprd_pcie_msi_irq(int irq, void *arg)
{
	struct wcn_pcie_info *priv = arg;

	/*
	 * priv->irq : the first msi irq
	 * irq: the current irq
	 */
	irq -= priv->irq;
	msi_irq_handle(irq);

	return IRQ_HANDLED;
}

static int sprd_pcie_legacy_irq(int irq, void *arg)
{
	legacy_irq_handle(irq);

	return IRQ_HANDLED;
}

int pcie_bar_write(struct wcn_pcie_info *priv, int bar, int offset,
		   char *buf, int len)
{
	char *mem = priv->bar[bar].vmem;

	mem += offset;
	PCIE_INFO("%s(%d, 0x%x, 0x%x)\n", __func__, bar, offset, *((int *)buf));
	memcpy(mem, buf, len);
	hexdump("read", mem, 16);

	return 0;
}
EXPORT_SYMBOL(pcie_bar_write);

int pcie_bar_read(struct wcn_pcie_info *priv, int bar, int offset,
		  char *buf, int len)
{
	char *mem = priv->bar[bar].vmem;

	mem += offset;
	memcpy(buf, mem, len);

	return 0;
}
EXPORT_SYMBOL(pcie_bar_read);

char *pcie_bar_vmem(struct wcn_pcie_info *priv, int bar)
{
	if (!priv) {
		PCIE_ERR("sprd pcie_dev NULL\n");
		return NULL;
	}

	return priv->bar[bar].vmem;
}

int dmalloc(struct wcn_pcie_info *priv, struct dma_buf *dm, int size)
{
	struct device *dev = &(priv->dev->dev);

	if (!dev) {
		PCIE_ERR("%s(NULL)\n", __func__);
		return ERROR;
	}

	if (dma_set_mask(dev, DMA_BIT_MASK(64))) {
		PCIE_INFO("dma_set_mask err\n");
		if (dma_set_coherent_mask(dev, DMA_BIT_MASK(64))) {
			PCIE_ERR("dma_set_coherent_mask err\n");
			return ERROR;
		}
	}

	dm->vir =
	    (unsigned long)dma_alloc_coherent(dev, size,
					      (dma_addr_t *)(&(dm->phy)),
					      GFP_DMA);
	if (dm->vir == 0) {
		PCIE_ERR("dma_alloc_coherent err\n");
		return ERROR;
	}
	dm->size = size;
	memset((unsigned char *)(dm->vir), 0x56, size);
	PCIE_INFO("dma_alloc_coherent(%d) 0x%lx 0x%lx\n",
		  size, dm->vir, dm->phy);

	return 0;
}
EXPORT_SYMBOL(dmalloc);

int dmfree(struct wcn_pcie_info *priv, struct dma_buf *dm)
{
	struct device *dev = &(priv->dev->dev);

	if (!dev) {
		PCIE_ERR("%s(NULL)\n", __func__);
		return ERROR;
	}
	PCIE_INFO("dma_free_coherent(%d,0x%lx,0x%lx)\n",
		dm->size, dm->vir, dm->phy);
	dma_free_coherent(dev, dm->size, (void *)(dm->vir), dm->phy);
	memset(dm, 0x00, sizeof(struct dma_buf));

	return ERROR;
}

unsigned char *ibreg_base(struct wcn_pcie_info *priv, char region)
{
	unsigned char *p = pcie_bar_vmem(priv, 4);

	if (!p)
		return NULL;
	if (region > 8)
		return NULL;
	PCIE_INFO("%s(%d):0x%x\n", __func__, region, (0x10100 | (region << 9)));
	/*
	 * 0x10000: iATU relative offset to BAR4.
	 * BAR4 included map iatu reg information.
	 * i= region
	 * Base = 0x10000
	 * outbound = Base + i * 0x200
	 * inbound = Base + i * 0x200 + 0x100
	 */
	p = p + (0x10100 | (region << 9));
	PCIE_INFO("base =0x%p\n", p);

	return p;
}

unsigned char *obreg_base(struct wcn_pcie_info *priv, char region)
{
	unsigned char *p = pcie_bar_vmem(priv, 4);

	if (!p)
		return NULL;
	if (region > 8)
		return NULL;
	PCIE_INFO("%s(%d):0x%x\n", __func__, region, (0x10000 | (region << 9)));
	p = p + (0x10000 | (region << 9));

	return p;
}

static int sprd_ep_addr_map(struct wcn_pcie_info *priv)
{
	struct inbound_reg *ibreg0;
	struct outbound_reg *obreg0;
	struct outbound_reg *obreg1;

	if (!pcie_bar_vmem(priv, 4)) {
		PCIE_INFO("get bar4 base err\n");
		return -1;
	}

	ibreg0 = (struct inbound_reg *) (pcie_bar_vmem(priv, 4) +
							IBREG0_OFFSET_ADDR);
	obreg0 = (struct outbound_reg *) (pcie_bar_vmem(priv, 4) +
							OBREG0_OFFSET_ADDR);
	obreg1 = (struct outbound_reg *) (pcie_bar_vmem(priv, 4) +
							OBREG1_OFFSET_ADDR);

	ibreg0->lower_target_addr = 0x40000000;
	ibreg0->upper_target_addr = 0x00000000;
	ibreg0->type    = 0x00000000;
	ibreg0->limit   = 0x00FFFFFF;
	ibreg0->en      = REGION_EN | BAR_MATCH_MODE;

	obreg0->type    = 0x00000000;
	obreg0->en      = REGION_EN & ADDR_MATCH_MODE;
	obreg0->lower_base_addr  = 0x00000000;
	obreg0->upper_base_addr  = 0x00000080;
	obreg0->limit   = 0xffffffff;
	obreg0->lower_target_addr = 0x00000000;
	obreg0->upper_target_addr = 0x00000000;

	obreg1->type    = 0x00000000;
	obreg1->en      = REGION_EN & ADDR_MATCH_MODE;
	obreg1->lower_base_addr  = 0x00000000;
	obreg1->upper_base_addr  = 0x00000081;
	obreg1->limit   = 0xffffffff;
	obreg1->lower_target_addr = 0x00000000;
	obreg1->upper_target_addr = 0x00000001;

	return 0;
}

int pcie_config_read(struct wcn_pcie_info *priv, int offset, char *buf, int len)
{
	int i, ret;

	for (i = 0; i < len; i++) {
		ret = pci_read_config_byte(priv->dev, i, &(buf[i]));
		if (ret) {
			PCIE_ERR("pci_read_config_dword %d err\n", ret);
			return ERROR;
		}
	}
	return 0;
}

int pcie_config_write(struct wcn_pcie_info *priv, int offset,
		      char *buf, int len)
{
	int i, ret;

	for (i = 0; i < len; i++) {
		ret = pci_write_config_byte(priv->dev, i, buf[i]);
		if (ret) {
			PCIE_ERR("%s %d err\n", __func__, ret);
			return ERROR;
		}

	}
	return 0;
}

int sprd_pcie_bar_map(struct wcn_pcie_info *priv, int bar, unsigned int addr)
{
	struct inbound_reg *ibreg = (struct inbound_reg *) ibreg_base(priv,
								      bar);

	if (!ibreg) {
		PCIE_ERR("ibreg(%d) NULL\n", bar);
		return -1;
	}
	ibreg->lower_target_addr = addr;
	ibreg->upper_target_addr = 0x00000000;
	ibreg->type = 0x00000000;
	ibreg->limit = 0x00FFFFFF;
	ibreg->en = REGION_EN | BAR_MATCH_MODE;
	PCIE_ERR("%s(%d, 0x%x)\n", __func__, bar, addr);

	return 0;
}
EXPORT_SYMBOL(sprd_pcie_bar_map);

static int sprd_pcie_probe(struct pci_dev *pdev,
			   const struct pci_device_id *pci_id)
{

	struct wcn_pcie_info *priv;

	int ret = -ENODEV, i, flag;

	PCIE_INFO("%s Enter\n", __func__);

	priv = kzalloc(sizeof(struct wcn_pcie_info), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	g_pcie_dev = priv;
	priv->dev = pdev;
	pci_set_drvdata(pdev, priv);

	/* enable device */
	if (pci_enable_device(pdev)) {
		PCIE_ERR("cannot enable device:%s\n", pci_name(pdev));
		goto err_out;
	}

	/* enable bus master capability on device */
	pci_set_master(pdev);

	priv->irq = pdev->irq;
	PCIE_INFO("dev->irq %d\n", pdev->irq);

	priv->legacy_en = 0;
	priv->msi_en = 1;
	priv->msix_en = 0;

	if (priv->legacy_en == 1)
		priv->irq = pdev->irq;

	if (priv->msi_en == 1) {
		priv->irq_num = pci_msi_vec_count(pdev);
		PCIE_INFO("pci_msi_vec_count ret %d\n", priv->irq_num);

		ret = pci_enable_msi_range(pdev, 1, priv->irq_num);
#if 0
		ret = pci_alloc_irq_vectors(pdev, 1, priv->irq_num,
					    PCI_IRQ_MSI);
#endif
		if (ret > 0) {
			PCIE_INFO("pci_enable_msi_range %d ok\n", ret);
			priv->msi_en = 1;
		} else {
			PCIE_INFO("pci_enable_msi_range err=%d\n", ret);
			priv->msi_en = 0;
		}
		priv->irq = pdev->irq;
	}

	if (priv->msix_en == 1) {
		for (i = 0; i < 65; i++) {
			priv->msix[i].entry = i;
			priv->msix[i].vector = 0;
		}
		priv->irq_num = pci_enable_msix_range(pdev, priv->msix, 1, 64);
		if (priv->irq_num > 0) {
			PCIE_INFO("pci_enable_msix_range %d ok\n",
				  priv->irq_num);
			priv->msix_en = 1;
		} else {
			PCIE_INFO("pci_enable_msix_range %d err\n",
				  priv->irq_num);
			priv->msix_en = 0;
		}
		priv->irq = pdev->irq;
	}
	PCIE_INFO("dev->irq %d\n", pdev->irq);
	PCIE_INFO("legacy %d msi_en %d, msix_en %d\n", priv->legacy_en,
		   priv->msi_en, priv->msix_en);
	for (i = 0; i < 8; i++) {
		flag = pci_resource_flags(pdev, i);
		if (!(flag & IORESOURCE_MEM))
			continue;

		priv->bar[i].mmio_start = pci_resource_start(pdev, i);
		priv->bar[i].mmio_end = pci_resource_end(pdev, i);
		priv->bar[i].mmio_flags = pci_resource_flags(pdev, i);
		priv->bar[i].mmio_len = pci_resource_len(pdev, i);
		priv->bar[i].mem =
		    ioremap(priv->bar[i].mmio_start, priv->bar[i].mmio_len);
		priv->bar[i].vmem = priv->bar[i].mem;
		if (priv->bar[i].vmem == NULL) {
			PCIE_ERR("%s:cannot remap mmio, aborting\n",
			       pci_name(pdev));
			ret = -EIO;
			goto err_out;
		}
		PCIE_INFO("BAR(%d) (0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)\n", i,
			  (unsigned long)priv->bar[i].mmio_start,
			  (unsigned long)priv->bar[i].mmio_end,
			  priv->bar[i].mmio_flags,
			  (unsigned long)priv->bar[i].mmio_len,
			  (unsigned long)priv->bar[i].vmem);
	}
	priv->bar_num = 8;
	ret = pci_request_regions(pdev, DRVER_NAME);
	if (ret) {
		priv->in_use = 1;
		goto err_out;
	}

	if (priv->legacy_en == 1) {
		ret = request_irq(priv->irq,
				 (irq_handler_t) (&sprd_pcie_legacy_irq),
				 IRQF_NO_SUSPEND | IRQF_NO_THREAD | IRQF_PERCPU,
				 DRVER_NAME, (void *)priv);
		if (ret)
			PCIE_ERR("%s request_irq(%d), error %d\n", __func__,
				priv->irq, ret);
		else
			PCIE_INFO("%s request_irq(%d) ok\n", __func__,
				  priv->irq);
	}
	if (priv->msi_en == 1) {
		for (i = 0; i < priv->irq_num; i++) {
			ret =
			    request_irq(priv->irq + i,
					(irq_handler_t) (&sprd_pcie_msi_irq),
					IRQF_SHARED, DRVER_NAME, (void *)priv);
			if (ret) {
				PCIE_ERR("%s request_irq(%d), error %d\n",
				       __func__, priv->irq + i, ret);
				break;
			}
			PCIE_INFO("%s request_irq(%d) ok\n", __func__,
				priv->irq + i);
		}
		if (i == priv->irq_num)
			priv->irq_en = 1;
	}
	if (priv->msix_en == 1) {
		for (i = 0; i < priv->irq_num; i++) {
			ret =
			    request_irq(priv->msix[i].vector,
					(irq_handler_t) (&sprd_pcie_msi_irq),
					IRQF_SHARED, DRVER_NAME, (void *)priv);
			if (ret) {
				PCIE_ERR("%s request_irq(%d), error %d\n",
				       __func__, priv->msix[i].vector, ret);
				break;
			}

			PCIE_INFO("%s request_irq(%d) ok\n", __func__,
				priv->msix[i].vector);
		}
	}
	device_wakeup_enable(&(pdev->dev));
	ret = sprd_ep_addr_map(priv);
	if (ret < 0)
		return ret;
	edma_init(priv);
	dbg_attach_bus(priv);
	proc_fs_init();
	log_dev_init();
	mdbg_atcmd_owner_init();
	wcn_op_init();
	PCIE_INFO("%s ok\n", __func__);

	return 0;

err_out:
	kfree(priv);

	return ret;
}

static int sprd_ep_suspend(struct device *dev)
{
	int ret;
	struct mchn_ops_t *ops;
	int chn;
	struct pci_dev *pdev = to_pci_dev(dev);
	struct wcn_pcie_info *priv = pci_get_drvdata(pdev);

	for (chn = 0; chn < 16; chn++) {
		ops = mchn_ops(chn);
		if ((ops != NULL) && (ops->power_notify != NULL)) {
			ret = ops->power_notify(chn, 0);
			if (ret != 0) {
				PCIE_INFO("[%s] chn:%d suspend fail\n",
						 __func__, chn);
				return ret;
			}
		}
	}

	PCIE_INFO("%s[+]\n", __func__);

	if (!pdev)
		return 0;

	pci_save_state(to_pci_dev(dev));
	priv->saved_state = pci_store_saved_state(to_pci_dev(dev));

	ret = pci_enable_wake(pdev, PCI_D3hot, 1);
	PCIE_INFO("pci_enable_wake(PCI_D3hot) ret %d\n", ret);
	ret = pci_set_power_state(pdev, PCI_D3hot);
	PCIE_INFO("pci_set_power_state(PCI_D3hot) ret %d\n", ret);
	PCIE_INFO("%s[-]\n", __func__);

	return 0;
}

static int sprd_ep_resume(struct device *dev)
{
	int ret;
	struct mchn_ops_t *ops;
	int chn;
	struct pci_dev *pdev = to_pci_dev(dev);
	struct wcn_pcie_info *priv = pci_get_drvdata(pdev);

	PCIE_INFO("%s[+]\n", __func__);
	if (!pdev)
		return 0;

	pci_load_and_free_saved_state(to_pci_dev(dev), &priv->saved_state);
	pci_restore_state(to_pci_dev(dev));
	pci_write_config_dword(to_pci_dev(dev), 0x60, 0);

	ret = pci_set_power_state(pdev, PCI_D0);
	PCIE_INFO("pci_set_power_state(PCI_D0) ret %d\n", ret);
	ret = pci_enable_wake(pdev, PCI_D0, 0);
	PCIE_INFO("pci_enable_wake(PCI_D0) ret %d\n", ret);

	usleep_range(50000, 51000);
	ret = sprd_ep_addr_map(priv);
	if (ret)
		return ret;
	usleep_range(10000, 11000);

	for (chn = 0; chn < 16; chn++) {
		ops = mchn_ops(chn);
		if ((ops != NULL) && (ops->power_notify != NULL)) {
			ret = ops->power_notify(chn, 1);
			if (ret != 0) {
				PCIE_INFO("[%s] chn:%d resume fail\n",
						 __func__, chn);
				return ret;
			}
		}
	}
	return 0;
}

const struct dev_pm_ops sprd_ep_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sprd_ep_suspend, sprd_ep_resume)
};

static void sprd_pcie_remove(struct pci_dev *pdev)
{
	int i;
	struct wcn_pcie_info *priv;

	priv = (struct wcn_pcie_info *) pci_get_drvdata(pdev);
	edma_deinit();
	ioctlcmd_deinit(priv);
	mpool_free();
	if (priv->legacy_en == 1)
		free_irq(priv->irq, (void *)priv);

	if (priv->msi_en == 1) {
		for (i = 0; i < priv->irq_num; i++)
			free_irq(priv->irq + i, (void *)priv);

		pci_disable_msi(pdev);
	}
	if (priv->msix_en == 1) {
		PCIE_INFO("disable MSI-X");
		for (i = 0; i < priv->irq_num; i++)
			free_irq(priv->msix[i].vector, (void *)priv);

		pci_disable_msix(pdev);
	}
	for (i = 0; i < priv->bar_num; i++) {
		if (priv->bar[i].mem)
			iounmap(priv->bar[i].mem);
	}
	pci_release_regions(pdev);
	kfree(priv);
	pci_set_drvdata(pdev, NULL);
	pci_disable_device(pdev);
}

static struct pci_device_id sprd_pcie_tbl[] = {
	{0x1db3, 0x2355, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, sprd_pcie_tbl);
static struct pci_driver sprd_pcie_driver = {
	.name = DRVER_NAME,
	.id_table = sprd_pcie_tbl,
	.probe = sprd_pcie_probe,
	.remove = sprd_pcie_remove,
	.driver = {
		.pm = &sprd_ep_pm_ops,
	},
};

static int __init sprd_pcie_init(void)
{
	int ret = 0;

	PCIE_INFO("%s init\n", __func__);

	ret = pci_register_driver(&sprd_pcie_driver);
	PCIE_INFO("pci_register_driver ret %d\n", ret);

	return ret;
}

static void __exit sprd_pcie_exit(void)
{
	PCIE_INFO("%s\n", __func__);
	pci_unregister_driver(&sprd_pcie_driver);
}

module_init(sprd_pcie_init);
module_exit(sprd_pcie_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("marlin3 pcie/edma drv");
