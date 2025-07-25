#ifndef __TX_MSG_H__
#define __TX_MSG_H__
#include <linux/types.h>
#include <linux/workqueue.h>
#include "wl_core.h"
#include "msg.h"
#include "cfg80211.h"
#include "txrx.h"
#include "wl_intf.h"
#include "qos.h"

/*descriptor len + sdio header len + offset*/
#define SKB_DATA_OFFSET 15
#define SPRDWL_SDIO_MASK_LIST_CMD	0x1
#define SPRDWL_SDIO_MASK_LIST_SPECIAL	0x2
#define SPRDWL_SDIO_MASK_LIST_DATA	0x4

/*The number of bytes in an ethernet (MAC) address.*/
#define	ETHER_ADDR_LEN 6

/*The number of bytes in the type field.*/
#define	ETHER_TYPE_LEN 2

/* The length of the combined header.*/
#define	ETHER_HDR_LEN		(ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN)

#define DHCP_SERVER_PORT    0x0043
#define DHCP_CLIENT_PORT    0x0044
#define DHCP_SERVER_PORT_IPV6 0x0223
#define DHCP_CLIENT_PORT_IPV6 0x0222
#define ETH_P_PREAUTH       0x88C7

#define VOWIFI_SIP_DSCP		0x1a
#define VOWIFI_IKE_DSCP		0x30
#define VOWIFI_VIDEO_DSCP	0x28
#define VOWIFI_AUDIO_DSCP	0x36

#define VOWIFI_IKE_SIP_PORT	4500
#define VOWIFI_IKE_ONLY_PORT	500

#define TX_MAX_POLLING 10

#define MAX_COLOR_BIT 4
struct sprdwl_flow_control {
	enum sprdwl_mode mode;
	u8 color_bit;
	atomic_t flow;
};

struct sprdwl_tx_msg {
	struct sprdwl_intf *intf;
	unsigned long cmd_timeout;
	unsigned long data_timeout;
	spinlock_t lock;
	unsigned long net_stop_cnt;
	unsigned long net_start_cnt;
	unsigned long drop_cmd_cnt;
	unsigned long drop_data_cnt;
	/* sta */
	unsigned long drop_data1_cnt;
	/* p2p */
	unsigned long drop_data2_cnt;
	unsigned long ring_cp;
	unsigned long ring_ap;
	atomic_t flow0;
	atomic_t flow1;
	atomic_t flow2;

	struct task_struct *tx_thread;
	enum sprdwl_mode mode;

	/*4 flow control color, 00/01/10/11*/
	struct sprdwl_flow_control flow_ctrl[MAX_COLOR_BIT];
	unsigned char color_num[MAX_COLOR_BIT];
	unsigned char seq_num;
	/*temp for cmd debug, remove in future*/
	unsigned int cmd_send;
	unsigned int cmd_poped;
	int mbuf_short;
	ktime_t kt;
#define HANG_RECOVERY_ACKED 2
	int hang_recovery_status;
	int thermal_status;

	struct sprdwl_msg_list tx_list_cmd;
	struct sprdwl_msg_list tx_list_qos_pool;
	struct sprdwl_xmit_msg_list xmit_msg_list;
	struct tx_t *tx_list[SPRDWL_MODE_MAX];
	unsigned long tx_data_num;
	ktime_t txtimebegin;
	ktime_t txtimeend;

	struct completion tx_completed;
};

struct sprdwl_msg_buf *sprdwl_get_msg_buf(void *pdev,
					enum sprdwl_head_type type,
					enum sprdwl_mode mode,
					u8 ctx_id);
void sprdwl_tx_free_msg_buf(void *pdev, struct sprdwl_msg_buf *msg);
int sprdwl_tx_msg_func(void *pdev, struct sprdwl_msg_buf *msg);
int sprdwl_sdio_process_credit(void *pdev, void *data);
int sprdwl_tx_init(struct sprdwl_intf *dev);
void sprdwl_tx_deinit(struct sprdwl_intf *dev);
int sprdwl_tx_filter_packet(struct sk_buff *skb, struct net_device *ndev);
unsigned char sprdwl_find_index_using_addr(struct sprdwl_intf *dev);
void sprdwl_dequeue_data_buf(struct sprdwl_msg_buf *msg_buf);
void sprdwl_dequeue_data_list(struct mbuf_t *head, int num);
void sprdwl_free_cmd_buf(struct sprdwl_msg_buf *msg_buf,
			    struct sprdwl_msg_list *list);
void sprdwl_wake_net_ifneed(struct sprdwl_intf *dev,
			    struct sprdwl_msg_list *list,
			    enum sprdwl_mode mode);
u8 sprdwl_fc_set_clor_bit(struct sprdwl_tx_msg *tx_msg, int num);
void sprdwl_wakeup_tx(struct sprdwl_tx_msg *tx_msg);
void handle_tx_status_after_close(struct sprdwl_vif *vif);
void sprdwl_flush_tx_qoslist(struct sprdwl_tx_msg *tx_msg, int mode, int ac_index, int lut_index);
void sprdwl_flush_mode_txlist(struct sprdwl_tx_msg *tx_msg, enum sprdwl_mode mode);
void sprdwl_flush_tosendlist(struct sprdwl_tx_msg *tx_msg);
void sprdwl_fc_add_share_credit(struct sprdwl_vif *vif);

bool is_vowifi_pkt(struct sk_buff *skb, bool *b_cmd_path);
void tx_up(struct sprdwl_tx_msg *tx_msg);
#endif

