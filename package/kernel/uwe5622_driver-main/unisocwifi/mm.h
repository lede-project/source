#ifndef __SPRDWL_MM_H__
#define __SPRDWL_MM_H__

#include <linux/skbuff.h>
#include <linux/dma-direction.h>

#define SPRDWL_PHYS_LEN 5
#define SPRDWL_PHYS_MASK (((uint64_t)1 << 40) - 1)
#define SPRDWL_MH_ADDRESS_BIT ((uint64_t)1 << 39)

#define SPRDWL_MAX_MH_BUF 500
#define SPRDWL_ADD_MH_BUF_THRESHOLD 300
#define SPRDWL_MAX_ADD_MH_BUF_ONCE 200
#define SPRDWL_ADDR_BUF_LEN (sizeof(struct sprdwl_addr_hdr) +\
			     sizeof(struct sprdwl_addr_trans_value) +\
			     (SPRDWL_MAX_ADD_MH_BUF_ONCE * SPRDWL_PHYS_LEN))

struct sprdwl_mm {
	int hif_offset;
	struct sk_buff_head buffer_list;
	/* hdr point to hdr of addr buf */
	void *hdr;
	/* addr_trans point to addr trans of addr buf */
	void *addr_trans;
};

int sprdwl_mm_init(struct sprdwl_mm *mm_entry, void *intf);
int sprdwl_mm_deinit(struct sprdwl_mm *mm_entry, void *intf);
void mm_mh_data_process(struct sprdwl_mm *mm_entry, void *data,
			int len, int buffer_type);
void mm_mh_data_event_process(struct sprdwl_mm *mm_entry, void *data,
			      int len, int buffer_type);
unsigned long mm_virt_to_phys(struct device *dev, void *buffer, size_t size,
			      enum dma_data_direction direction);
void *mm_phys_to_virt(struct device *dev, unsigned long pcie_addr, size_t size,
		      enum dma_data_direction direction, bool is_mh);
int sprdwl_tx_addr_buf_unmap(void *tx_msg,
			     int complete, int tx_count);
int mm_buffer_alloc(struct sprdwl_mm *mm_entry, int need_num);
#endif /* __SPRDWL_MM_H__ */
