/*
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <wcn_bus.h>

#include "bus_common.h"
#include "edma_engine.h"
#include "mchn.h"
#include "pcie.h"

static int pcie_preinit(void)
{
	return 0;
}

static void pcie_preexit(void)
{
}

static int pcie_buf_list_alloc(int chn, struct mbuf_t **head,
			       struct mbuf_t **tail, int *num)
{
	return mbuf_link_alloc(chn, head, tail, num);
}

static int pcie_buf_list_free(int chn, struct mbuf_t *head,
			      struct mbuf_t *tail, int num)
{
	return mbuf_link_free(chn, head, tail, num);
}

static int pcie_list_push(int chn, struct mbuf_t *head,
			  struct mbuf_t *tail, int num)
{
	return mchn_push_link(chn, head, tail, num);
}

static int pcie_chn_init(struct mchn_ops_t *ops)
{
	return mchn_init(ops);
}

static int pcie_chn_deinit(struct mchn_ops_t *ops)
{
	return mchn_deinit(ops);
}

static struct sprdwcn_bus_ops pcie_bus_ops = {
	.preinit = pcie_preinit,
	.deinit = pcie_preexit,
	.chn_init = pcie_chn_init,
	.chn_deinit = pcie_chn_deinit,
	.list_alloc = pcie_buf_list_alloc,
	.list_free = pcie_buf_list_free,
	.push_list = pcie_list_push,
};

void module_bus_init(void)
{
	module_ops_register(&pcie_bus_ops);
}
EXPORT_SYMBOL(module_bus_init);

void module_bus_deinit(void)
{
	module_ops_unregister();
}
EXPORT_SYMBOL(module_bus_deinit);
