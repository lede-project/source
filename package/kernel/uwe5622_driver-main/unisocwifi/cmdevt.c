/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors	:
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
 * Jingxiang Li <Jingxiang.li@spreadtrum.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "sprdwl.h"
#include "cmdevt.h"
#include "cfg80211.h"
#include "msg.h"
#include "txrx.h"
#include "intf_ops.h"
#include "vendor.h"
#include "work.h"
#ifdef NAN_SUPPORT
#include "nan.h"
#endif /* NAN_SUPPORT */
#include "tx_msg.h"
#include "rx_msg.h"
#include "wl_intf.h"
#ifdef DFS_MASTER
#include "11h.h"
#endif
#include "rf_marlin3.h"
#include <linux/kthread.h>
#ifdef WMMAC_WFA_CERTIFICATION
#include "qos.h"
#endif
#include <linux/completion.h>

struct sprdwl_cmd {
	u8 cmd_id;
	int init_ok;
	u32 mstime;
	void *data;
	atomic_t refcnt;
	/* spin lock for command */
	spinlock_t lock;
	/* mutex for command */
	struct mutex cmd_lock;
	/* wake_lock for command */
	struct wakeup_source *wake_lock;
	/*complettion for command*/
	struct completion completed;
};

struct sprdwl_cmd g_sprdwl_cmd;

const uint16_t CRC_table[] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00,
	0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401,
	0x5000, 0x9C01, 0x8801, 0x4400,
};

#define C2S(x) \
{ \
	case x: \
		str = #x;\
		break; \
}

static int bss_count;
static const char *cmd2str(u8 cmd)
{
	const char *str = NULL;

	switch (cmd) {
	C2S(WIFI_CMD_ERR)
	C2S(WIFI_CMD_GET_INFO)
	C2S(WIFI_CMD_SET_REGDOM)
	C2S(WIFI_CMD_OPEN)
	C2S(WIFI_CMD_CLOSE)
	C2S(WIFI_CMD_POWER_SAVE)
	C2S(WIFI_CMD_SET_PARAM)
	C2S(WIFI_CMD_REQ_LTE_CONCUR)
	C2S(WIFI_CMD_SYNC_VERSION)
	C2S(WIFI_CMD_CONNECT)

	C2S(WIFI_CMD_SCAN)
	C2S(WIFI_CMD_SCHED_SCAN)
	C2S(WIFI_CMD_DISCONNECT)
	C2S(WIFI_CMD_KEY)
	C2S(WIFI_CMD_SET_PMKSA)
	C2S(WIFI_CMD_GET_STATION)
	C2S(WIFI_CMD_SET_CHANNEL)

	C2S(WIFI_CMD_START_AP)
	C2S(WIFI_CMD_DEL_STATION)
	C2S(WIFI_CMD_SET_BLACKLIST)
	C2S(WIFI_CMD_SET_WHITELIST)
	C2S(WIFI_CMD_MULTICAST_FILTER)

	C2S(WIFI_CMD_TX_MGMT)
	C2S(WIFI_CMD_REGISTER_FRAME)
	C2S(WIFI_CMD_REMAIN_CHAN)
	C2S(WIFI_CMD_CANCEL_REMAIN_CHAN)

	C2S(WIFI_CMD_SET_IE)
	C2S(WIFI_CMD_NOTIFY_IP_ACQUIRED)

	C2S(WIFI_CMD_SET_CQM)
	C2S(WIFI_CMD_SET_ROAM_OFFLOAD)
	C2S(WIFI_CMD_SET_MEASUREMENT)
	C2S(WIFI_CMD_SET_QOS_MAP)
	C2S(WIFI_CMD_TDLS)
	C2S(WIFI_CMD_11V)
	C2S(WIFI_CMD_NPI_MSG)
	C2S(WIFI_CMD_NPI_GET)

	C2S(WIFI_CMD_ASSERT)
	C2S(WIFI_CMD_FLUSH_SDIO)
	C2S(WIFI_CMD_ADD_TX_TS)
	C2S(WIFI_CMD_DEL_TX_TS)
	C2S(WIFI_CMD_LLSTAT)

	C2S(WIFI_CMD_GSCAN)
	C2S(WIFI_CMD_RSSI_MONITOR)

	C2S(WIFI_CMD_IBSS_JOIN)
	C2S(WIFI_CMD_SET_IBSS_ATTR)
	C2S(WIFI_CMD_NAN)
	C2S(WIFI_CMD_RND_MAC)
	C2S(WIFI_CMD_BA)
	C2S(WIFI_CMD_SET_MAX_CLIENTS_ALLOWED)
	C2S(WIFI_CMD_TX_DATA)
	C2S(WIFI_CMD_ADDBA_REQ)
	C2S(WIFI_CMD_DELBA_REQ)
	C2S(WIFI_CMD_SET_PROTECT_MODE)
	C2S(WIFI_CMD_GET_PROTECT_MODE)
	C2S(WIFI_CMD_DOWNLOAD_INI)
	C2S(WIFI_CMD_PACKET_OFFLOAD)
#ifdef DFS_MASTER
	C2S(WIFI_CMD_RADAR_DETECT)
	C2S(WIFI_CMD_RESET_BEACON)
#endif
	C2S(WIFI_CMD_VOWIFI_DATA_PROTECT)
	C2S(WIFI_CMD_SET_TLV)
	C2S(WIFI_CMD_SET_WOWLAN)
	default :
		return "WIFI_CMD_UNKNOWN";
	}

	return str;
}

#undef C2S

#define AR2S(x) \
{ \
	case x: \
		str = #x; \
		break; \
}

static const char *assert_reason_to_str(u8 reason)
{
	const char *str = NULL;

	switch (reason) {
	AR2S(SCAN_ERROR)
	AR2S(RSP_CNT_ERROR)
	AR2S(HANDLE_FLAG_ERROR)
	AR2S(CMD_RSP_TIMEOUT_ERROR)
	AR2S(LOAD_INI_DATA_FAILED)
	AR2S(DOWNLOAD_INI_DATA_FAILED)
	default :
		return "UNKNOWN ASSERT REASON";
	}
	return str;
}

#undef AR2S

uint16_t CRC16(uint8_t *buf, uint16_t len)
{
	uint16_t CRC = 0xFFFF;
	uint16_t i;
	uint8_t ch_char;

	for (i = 0; i < len; i++) {
		ch_char = *buf++;
		CRC = CRC_table[(ch_char ^ CRC) & 15] ^ (CRC >> 4);
		CRC = CRC_table[((ch_char >> 4) ^ CRC) & 15] ^ (CRC >> 4);
	}
	return CRC;
}

static const char *err2str(s8 error)
{
	char *str = NULL;

	switch (error) {
	case SPRDWL_CMD_STATUS_ARG_ERROR:
		str = "SPRDWL_CMD_STATUS_ARG_ERROR";
		break;
	case SPRDWL_CMD_STATUS_GET_RESULT_ERROR:
		str = "SPRDWL_CMD_STATUS_GET_RESULT_ERROR";
		break;
	case SPRDWL_CMD_STATUS_EXEC_ERROR:
		str = "SPRDWL_CMD_STATUS_EXEC_ERROR";
		break;
	case SPRDWL_CMD_STATUS_MALLOC_ERROR:
		str = "SPRDWL_CMD_STATUS_MALLOC_ERROR";
		break;
	case SPRDWL_CMD_STATUS_WIFIMODE_ERROR:
		str = "SPRDWL_CMD_STATUS_WIFIMODE_ERROR";
		break;
	case SPRDWL_CMD_STATUS_ERROR:
		str = "SPRDWL_CMD_STATUS_ERROR";
		break;
	case SPRDWL_CMD_STATUS_CONNOT_EXEC_ERROR:
		str = "SPRDWL_CMD_STATUS_CONNOT_EXEC_ERROR";
		break;
	case SPRDWL_CMD_STATUS_NOT_SUPPORT_ERROR:
		str = "SPRDWL_CMD_STATUS_NOT_SUPPORT_ERROR";
		break;
	case SPRDWL_CMD_STATUS_CRC_ERROR:
		str = "SPRDWL_CMD_STATUS_CRC_ERROR";
		break;
	case SPRDWL_CMD_STATUS_INI_INDEX_ERROR:
		str = "SPRDWL_CMD_STATUS_INI_INDEX_ERROR";
		break;
	case SPRDWL_CMD_STATUS_LENGTH_ERROR:
		str = "SPRDWL_CMD_STATUS_LENGTH_ERROR";
		break;
	case SPRDWL_CMD_STATUS_OTHER_ERROR:
		str = "SPRDWL_CMD_STATUS_OTHER_ERROR";
		break;
	case SPRDWL_CMD_STATUS_OK:
		str = "CMD STATUS OK";
		break;
	default:
		str = "SPRDWL_CMD_STATUS_UNKNOWN_ERROR";
		break;
	}
	return str;
}

#define SPRDWL_CMD_EXIT_VAL 0x8000
int sprdwl_cmd_init(void)
{
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	/* memset(cmd, 0, sizeof(*cmd)); */
	cmd->data = NULL;
#ifdef CONFIG_WIFI_RK_PM_PRIVATE_API
	cmd->wake_lock = wakeup_source_register(sprdwl_dev,
						"Wi-Fi_cmd_wakelock");
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	cmd->wake_lock = wakeup_source_register("Wi-Fi_cmd_wakelock");
#else
	cmd->wake_lock = wakeup_source_register(sprdwl_dev, "Wi-Fi_cmd_wakelock");
#endif
#endif
	if (!cmd->wake_lock) {
		wl_err("%s wakeup source register error.\n", __func__);
		return -EINVAL;
	}

#ifdef CP2_RESET_SUPPORT
	if (atomic_read(&cmd->refcnt) >= SPRDWL_CMD_EXIT_VAL)
		atomic_set(&cmd->refcnt, 0);
#endif

	spin_lock_init(&cmd->lock);
	mutex_init(&cmd->cmd_lock);
	init_completion(&cmd->completed);
	cmd->init_ok = 1;
	return 0;
}

void sprdwl_cmd_wake_upall(void)
{
	complete(&g_sprdwl_cmd.completed);
}

static void sprdwl_cmd_set(struct sprdwl_cmd_hdr *hdr)
{
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	u32 msec;
	ktime_t kt;

	kt = ktime_get();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
	msec = (u32)(div_u64(kt, NSEC_PER_MSEC));
#else
	msec = (u32)(div_u64(kt.tv64, NSEC_PER_MSEC));
#endif
	hdr->mstime = cpu_to_le32(msec);
	spin_lock_bh(&cmd->lock);
	kfree(cmd->data);
	cmd->data = NULL;
	cmd->mstime = msec;
	cmd->cmd_id = hdr->cmd_id;
	spin_unlock_bh(&cmd->lock);
}

static void sprdwl_cmd_clean(struct sprdwl_cmd *cmd)
{
	spin_lock_bh(&cmd->lock);
	kfree(cmd->data);
	cmd->data = NULL;
	cmd->mstime = 0;
	cmd->cmd_id = 0;
	spin_unlock_bh(&cmd->lock);
}

void sprdwl_cmd_deinit(void)
{
	unsigned long timeout;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;

	atomic_add(SPRDWL_CMD_EXIT_VAL, &cmd->refcnt);
	complete(&cmd->completed);
	timeout = jiffies + msecs_to_jiffies(1000);
	while (atomic_read(&cmd->refcnt) > SPRDWL_CMD_EXIT_VAL) {
		if (time_after(jiffies, timeout)) {
			wl_err("%s cmd lock timeout\n", __func__);
			break;
		}
		usleep_range(2000, 2500);
	}
	sprdwl_cmd_clean(cmd);
	mutex_destroy(&cmd->cmd_lock);
	if (cmd->wake_lock)
		wakeup_source_unregister(cmd->wake_lock);
#ifdef CP2_RESET_SUPPORT
	cmd->init_ok = 0;
#endif
}

extern struct sprdwl_intf_ops g_intf_ops;
static int sprdwl_cmd_lock(struct sprdwl_cmd *cmd)
{
	struct sprdwl_intf *intf = (struct sprdwl_intf *)g_intf_ops.intf;

#ifdef CP2_RESET_SUPPORT
	if (!unlikely(cmd->init_ok))
		return -1;
#endif

	if (atomic_inc_return(&cmd->refcnt) >= SPRDWL_CMD_EXIT_VAL) {
		atomic_dec(&cmd->refcnt);
		wl_err("%s failed, cmd->refcnt=%d\n",
			   __func__,
			   atomic_read(&cmd->refcnt));
		return -1;
	}
	mutex_lock(&cmd->cmd_lock);
	if (intf->priv->is_suspending == 0)
		__pm_stay_awake(cmd->wake_lock);

#ifdef UNISOC_WIFI_PS
	if (SPRDWL_PS_SUSPENDED == intf->suspend_mode) {
		reinit_completion(&intf->suspend_completed);
		wait_for_completion(&intf->suspend_completed);
		wl_info("wait for completion\n");
	}
#endif

	wl_debug("cmd->refcnt=%x\n", atomic_read(&cmd->refcnt));

	return 0;
}

static void sprdwl_cmd_unlock(struct sprdwl_cmd *cmd)
{
	struct sprdwl_intf *intf = (struct sprdwl_intf *)g_intf_ops.intf;

#ifdef CP2_RESET_SUPPORT
	if (!unlikely(cmd->init_ok))
		return;
#endif

	mutex_unlock(&cmd->cmd_lock);
	atomic_dec(&cmd->refcnt);
	if (intf->priv->is_suspending == 0)
		__pm_relax(cmd->wake_lock);
	if (intf->priv->is_suspending == 1)
		intf->priv->is_suspending = 0;
}

struct sprdwl_msg_buf *__sprdwl_cmd_getbuf(struct sprdwl_priv *priv,
					   u16 len, u8 ctx_id,
					   enum sprdwl_head_rsp rsp,
					   u8 cmd_id, gfp_t flags)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_hdr *hdr;
	u16 plen = sizeof(*hdr) + len;
	enum sprdwl_mode mode = SPRDWL_MODE_NONE;/*default to open new device*/
	void *data = NULL;
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);

	if (intf->cp_asserted == 1)
		return NULL;

#ifdef CP2_RESET_SUPPORT
	if ((g_sprdwl_priv->sync.scan_not_allowed == true) &&
	   (g_sprdwl_priv->sync.cmd_not_allowed == false)) {
		if ((cmd_id != WIFI_CMD_SYNC_VERSION) &&
		   (cmd_id != WIFI_CMD_DOWNLOAD_INI) &&
		   (cmd_id != WIFI_CMD_GET_INFO) &&
		   (cmd_id != WIFI_CMD_OPEN) &&
		   (cmd_id != WIFI_CMD_SET_REGDOM)) {
			   return NULL;
		}
	}
#endif

	if (cmd_id >= WIFI_CMD_OPEN) {
		vif = ctx_id_to_vif(priv, ctx_id);
		if (!vif)
			wl_err("%s cant't get vif, ctx_id: %d\n",
				   __func__, ctx_id);
		else
			mode =  vif->mode;
		sprdwl_put_vif(vif);
	}

	msg = sprdwl_intf_get_msg_buf(priv, SPRDWL_TYPE_CMD, mode, ctx_id);
	if (!msg) {
		wl_err("%s, %d, getmsgbuf fail, mode=%d\n",
			   __func__, __LINE__, mode);
		return NULL;
	}

	data = kzalloc((plen + priv->hw_offset), flags);
	if (data) {
		hdr = (struct sprdwl_cmd_hdr *)(data + priv->hw_offset);
		hdr->common.type = SPRDWL_TYPE_CMD;
		hdr->common.reserv = 0;
		hdr->common.rsp = rsp;
		hdr->common.ctx_id = ctx_id;
		hdr->plen = cpu_to_le16(plen);
		hdr->cmd_id = cmd_id;
		sprdwl_fill_msg(msg, NULL, data, plen);
		msg->data = hdr + 1;
	} else {
		wl_err("%s failed to allocate skb\n", __func__);
		sprdwl_intf_free_msg_buf(priv, msg);
		return NULL;
	}

	return msg;
}

/* if erro, data is released in this function
 * if OK, data is released in hif interface
 */
static int sprdwl_cmd_send_to_ic(struct sprdwl_priv *priv,
				 struct sprdwl_msg_buf *msg)
{
	struct sprdwl_cmd_hdr *hdr;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	hdr = (struct sprdwl_cmd_hdr *)(msg->tran_data + priv->hw_offset);
	/*TODO:consider common this if condition since
	 * SPRDWL_HEAD_NORSP not used any more
	 */
	if (hdr->common.rsp)
		sprdwl_cmd_set(hdr);

	wl_warn("[%u]ctx_id %d send[%s], num: %d\n",
		le32_to_cpu(hdr->mstime),
		hdr->common.ctx_id,
		cmd2str(hdr->cmd_id),
		tx_msg->cmd_send + 1);

	return sprdwl_send_cmd(priv, msg);
}

static int sprdwl_timeout_recv_rsp(struct sprdwl_priv *priv,
				   unsigned int timeout)
{
	int ret;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	ret = wait_for_completion_timeout(&cmd->completed, msecs_to_jiffies(timeout));
	if (!ret) {
		wl_err("[%s]timeout\n", cmd2str(cmd->cmd_id));
		return -1;
	} else if (sprdwl_intf_is_exit(priv) ||
		   atomic_read(&cmd->refcnt) >= SPRDWL_CMD_EXIT_VAL) {
		wl_err("cmd->refcnt=%x\n", atomic_read(&cmd->refcnt));
		return -1;
	} else if (tx_msg->hang_recovery_status == HANG_RECOVERY_ACKED
		&& cmd->cmd_id != WIFI_CMD_HANG_RECEIVED) {
		wl_warn("hang recovery happen\n");
		return -1;
	}

	spin_lock_bh(&cmd->lock);
	ret = cmd->data ? 0 : -1;
	spin_unlock_bh(&cmd->lock);

	return ret;
}

static int sprdwl_atcmd_assert(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 cmd_id, u8 reason)
{
#define ASSERT_INFO_BUF_SIZE	100

	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	char	buf[ASSERT_INFO_BUF_SIZE] = {0};
	u8 idx = 0;

	wl_err("%s ctx_id:%d, cmd_id:%d, reason:%d, cp_asserted:%d\n",
		   __func__, vif_ctx_id, cmd_id, reason, intf->cp_asserted);

	if (intf->cp_asserted == 0) {
		intf->cp_asserted = 1;

		if ((strlen(cmd2str(cmd_id)) + strlen(assert_reason_to_str(reason)) +
		   strlen("[CMD] ") + strlen(", [REASON] ")) < ASSERT_INFO_BUF_SIZE)
			idx += sprintf(buf+idx, "[CMD] %s, [REASON] %s", cmd2str(cmd_id), assert_reason_to_str(reason));
		else
			idx += sprintf(buf+idx, "[CMD ID] %d, [REASON ID] %d", cmd_id, reason);

		buf[idx] = '\0';

		mdbg_assert_interface(buf);
		sprdwl_net_flowcontrl(priv, SPRDWL_MODE_NONE, false);
		intf->exit = 1;

		return 1;
	} else {
		return -1;
	}

#undef ASSERT_INFO_BUF_SIZE
}

/* msg is released in this function or the realy driver
 * rbuf: the msg after sprdwl_cmd_hdr
 * rlen: input the length of rbuf
 *       output the length of the msg,if *rlen == 0, rbuf get nothing
 */
int sprdwl_cmd_send_recv(struct sprdwl_priv *priv,
			 struct sprdwl_msg_buf *msg,
			 unsigned int timeout, u8 *rbuf, u16 *rlen)
{
	u8 cmd_id;
	u16 plen;
	int ret = 0;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	struct sprdwl_cmd_hdr *hdr;
	u8 ctx_id;
	struct sprdwl_vif *vif;
	struct sprdwl_intf *intf;
	struct sprdwl_tx_msg *tx_msg;
	intf = (struct sprdwl_intf *)(priv->hw_priv);
	tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	ret = sprdwl_api_available_check(priv, msg);
	if (ret || sprdwl_cmd_lock(cmd)) {
		sprdwl_intf_free_msg_buf(priv, msg);
		kfree(msg->tran_data);
		if (rlen)
			*rlen = 0;
		if (ret)
			wl_err("API check fail, return!!\n");
		goto out;
	}

	hdr = (struct sprdwl_cmd_hdr *)(msg->tran_data + priv->hw_offset);
	cmd_id = hdr->cmd_id;
	ctx_id = hdr->common.ctx_id;

	reinit_completion(&cmd->completed);
	ret = sprdwl_cmd_send_to_ic(priv, msg);
	if (ret) {
		sprdwl_cmd_unlock(cmd);
		wl_err("%s ctx_id = %d, cmd: %s[%d] send failed, ret = %d\n",
				__func__, ctx_id, cmd2str(cmd_id), cmd_id, ret);
		return -1;
	}

	ret = sprdwl_timeout_recv_rsp(priv, timeout);

#ifdef CP2_RESET_SUPPORT
	if (true == priv->sync.cmd_not_allowed) {
		if (unlikely(cmd->init_ok))
			sprdwl_cmd_unlock(cmd);

		return 0;
	}
#endif

	if (ret != -1) {
		if (rbuf && rlen && *rlen) {
			hdr = (struct sprdwl_cmd_hdr *)cmd->data;
			plen = le16_to_cpu(hdr->plen) - sizeof(*hdr);
			*rlen = min(*rlen, plen);
			memcpy(rbuf, hdr->paydata, *rlen);
			wl_warn("ctx_id:%d cmd_id:%d [%s]rsp received, num=%d\n",
				hdr->common.ctx_id, cmd_id, cmd2str(cmd_id),
				tx_msg->cmd_send);
			if (cmd_id == WIFI_CMD_OPEN)
				rbuf[0] = hdr->common.ctx_id;
		} else {
			hdr = (struct sprdwl_cmd_hdr *)cmd->data;
			wl_info("ctx_id:%d cmd_id:%d [%s]rsp received, num=%d\n",
				hdr->common.ctx_id, cmd_id, cmd2str(cmd_id),
				tx_msg->cmd_send);
		}
	} else {
		wl_err("ctx_id:%d cmd: %s[%d] rsp timeout (mstime = %d), num=%d\n",
			   ctx_id, cmd2str(cmd_id), cmd_id, le32_to_cpu(cmd->mstime),
			   tx_msg->cmd_send);
		if (cmd_id == WIFI_CMD_CLOSE) {
			sprdwl_atcmd_assert(priv, ctx_id, cmd_id, CMD_RSP_TIMEOUT_ERROR);
			sprdwl_cmd_unlock(cmd);
			return ret;
		}
		vif = ctx_id_to_vif(priv, ctx_id);
		if (vif != NULL) {
			intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
			tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
			if (intf->cp_asserted == 0 &&
				tx_msg->hang_recovery_status == HANG_RECOVERY_END)
				sprdwl_send_assert_cmd(vif, cmd_id, CMD_RSP_TIMEOUT_ERROR);
			sprdwl_put_vif(vif);
		}
	}
	sprdwl_cmd_unlock(cmd);
out:
	return ret;
}

/* msg is released in this function or the realy driver
 * rbuf: the msg after sprdwl_cmd_hdr
 * rlen: input the length of rbuf
 *       output the length of the msg,if *rlen == 0, rbuf get nothing
 */
int sprdwl_cmd_send_recv_no_wait(struct sprdwl_priv *priv,
			 struct sprdwl_msg_buf *msg)
{
	u8 cmd_id;
	int ret = 0;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	struct sprdwl_cmd_hdr *hdr;
	u8 ctx_id;

	if (sprdwl_cmd_lock(cmd)) {
		wl_err("%s, %d, error!\n", __func__, __LINE__);
		sprdwl_intf_free_msg_buf(priv, msg);
		kfree(msg->tran_data);
		goto out;
	}

	hdr = (struct sprdwl_cmd_hdr *)(msg->tran_data + priv->hw_offset);
	cmd_id = hdr->cmd_id;
	ctx_id = hdr->common.ctx_id;

	ret = sprdwl_cmd_send_to_ic(priv, msg);
	if (ret) {
		sprdwl_cmd_unlock(cmd);
		return -1;
	}
	sprdwl_cmd_unlock(cmd);
out:
	return ret;
}

/*Commands to sync API version with firmware*/
int sprdwl_sync_version(struct sprdwl_priv *priv)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_api_t *drv_api = NULL;
	struct sprdwl_cmd_api_t *fw_api = NULL;
	u16 r_len = sizeof(*fw_api);
	u8 r_buf[sizeof(*fw_api)];
	int ret = 0;

	msg = sprdwl_cmd_getbuf(priv, sizeof(struct sprdwl_cmd_api_t),
			SPRDWL_MODE_NONE, SPRDWL_HEAD_RSP,
			WIFI_CMD_SYNC_VERSION);
	if (!msg)
		return -ENOMEM;
	drv_api = (struct sprdwl_cmd_api_t *)msg->data;
	/*fill drv api version got from local*/
	sprdwl_fill_drv_api_version(priv, drv_api);

	ret = sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, &r_len);
	if (!ret && r_len) {
		fw_api = (struct sprdwl_cmd_api_t *)r_buf;
		/*fill fw api version to priv got from firmware*/
		sprdwl_fill_fw_api_version(priv, fw_api);
	}
	return ret;
}

/* Commands */
static int sprdwl_down_ini_cmd(struct sprdwl_priv *priv,
						uint8_t *data, uint32_t len,
						uint8_t sec_num)
{
	int ret = 0;
	struct sprdwl_msg_buf *msg;
	uint8_t *p = NULL;
	uint16_t CRC = 0;

	/*reserved 4 byte of section num for align */
	msg = sprdwl_cmd_getbuf(priv, len + 4 + sizeof(CRC),
			SPRDWL_MODE_NONE, SPRDWL_HEAD_RSP,
			WIFI_CMD_DOWNLOAD_INI);
	if (!msg)
		return -ENOMEM;

	/*calc CRC value*/
	CRC = CRC16(data, len);
	wl_info("CRC value:%d\n", CRC);

	p = msg->data;
	*p = sec_num;

	/*copy data after section num*/
	memcpy((p + 4), data, len);
	/*put CRC value at the tail of INI data*/
	memcpy((p + 4 + len), &CRC, sizeof(CRC));

	ret = sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
	return ret;
}

void sprdwl_download_ini(struct sprdwl_priv *priv)
{
#define SEC1	1
#define SEC2	2
#define SEC3	3

	int ret;
	struct wifi_conf_t *wifi_data;
	struct wifi_conf_sec1_t *sec1;
	struct wifi_conf_sec2_t *sec2;

	wl_debug("%s enter:", __func__);
	/*if ini file has been download already, return*/
	if (sprdwl_get_ini_status(priv)) {
		wl_err("RF ini download already, skip!\n");
		return;
	}

	wifi_data = kzalloc(sizeof(struct wifi_conf_t), GFP_KERNEL);
	/*init INI data struct */
	/*got ini data from file*/
	ret = get_wifi_config_param(wifi_data);
	if (ret) {
		wl_err("load ini data failed, return\n");
		kfree(wifi_data);
		wlan_set_assert(priv, SPRDWL_MODE_NONE,
				WIFI_CMD_DOWNLOAD_INI,
				LOAD_INI_DATA_FAILED);
		return;
	}

	wl_info("total config len:%ld,sec1 len:%ld, sec2 len:%ld\n",
		(long unsigned int)sizeof(wifi_data), (long unsigned int)sizeof(*sec1),
		(long unsigned int)sizeof(*sec2));
	/*devide wifi_conf into sec1 and sec2 since it's too large*/
	sec1 = (struct wifi_conf_sec1_t *)wifi_data;
	sec2 = (struct wifi_conf_sec2_t *)(&wifi_data->tx_scale);

	wl_info("download the first section of config file\n");
	ret = sprdwl_down_ini_cmd(priv, (uint8_t *)sec1, sizeof(*sec1), SEC1);
	if (ret) {
		wl_err("download the first section of ini fail,return\n");
		kfree(wifi_data);
		wlan_set_assert(priv, SPRDWL_MODE_NONE,
				WIFI_CMD_DOWNLOAD_INI,
				DOWNLOAD_INI_DATA_FAILED);
		return;
	}

	wl_info("download the second section of config file\n");
	ret = sprdwl_down_ini_cmd(priv, (uint8_t *)sec2, sizeof(*sec2), SEC2);
	if (ret) {
		wl_err("download the second section of ini fail,return\n");
		kfree(wifi_data);
		wlan_set_assert(priv, SPRDWL_MODE_NONE,
				WIFI_CMD_DOWNLOAD_INI,
				DOWNLOAD_INI_DATA_FAILED);
		return;
	}

	if (wifi_data->rf_config.rf_data_len) {
		wl_info("download the third section of config file\n");
		wl_info("rf_data_len = %d\n", wifi_data->rf_config.rf_data_len);
		ret = sprdwl_down_ini_cmd(priv, wifi_data->rf_config.rf_data,
				wifi_data->rf_config.rf_data_len, SEC3);
		if (ret) {
			wl_err("download the third section of ini fail,return\n");
			kfree(wifi_data);
			wlan_set_assert(priv, SPRDWL_MODE_NONE,
					WIFI_CMD_DOWNLOAD_INI,
					DOWNLOAD_INI_DATA_FAILED);
			return;
		}
	}

	kfree(wifi_data);
}

int sprdwl_get_fw_info(struct sprdwl_priv *priv)
{
	int ret;
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_fw_info *p;
	struct sprdwl_tlv_data *tlv;
	u16 r_len = sizeof(*p) + GET_INFO_TLV_RBUF_SIZE;
	u16 r_len_ori = r_len;
	u8 r_buf[sizeof(*p) + GET_INFO_TLV_RBUF_SIZE];
	u8 compat_ver = 0;
	unsigned int len_count = 0;
	bool b_tlv_data_chk = true;
	u16 tlv_len = sizeof(struct ap_version_tlv_elmt);
#ifdef WL_CONFIG_DEBUG
	u8 ap_version = NOTIFY_AP_VERSION_USER_DEBUG;
#else
	u8 ap_version = NOTIFY_AP_VERSION_USER;
#endif

#ifdef OTT_UWE
#define OTT_UWE_OFFSET_ENABLE 1
	tlv_len += 1;
#endif

	memset(r_buf, 0, r_len);
	msg = sprdwl_cmd_getbuf(priv, tlv_len, SPRDWL_MODE_NONE,
				SPRDWL_HEAD_RSP, WIFI_CMD_GET_INFO);
	if (!msg)
		return -ENOMEM;

	compat_ver = need_compat_operation(priv, WIFI_CMD_GET_INFO);
	if (compat_ver) {
		switch (compat_ver) {
		case VERSION_1:
			/*add data struct modification in here!*/
			priv->sync_api.compat = VERSION_1;
			break;
		case VERSION_2:
			/*add data struct modification in here!*/
			priv->sync_api.compat = VERSION_2;
			break;
		case VERSION_3:
			/*add data struct modification in here!*/
			priv->sync_api.compat = VERSION_3;
			break;
		default:
			break;
		}
	}

	sprdwl_set_tlv_elmt((u8 *)msg->data, NOTIFY_AP_VERSION,
				sizeof(ap_version), &ap_version);

#ifdef OTT_UWE
	tlv = (struct sprdwl_tlv_data *)msg->data;
	tlv->type = OTT_UWE_OFFSET_ENABLE;
	tlv->len = 1;
	*((char *)tlv->data) = 1;
#endif
	ret = sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, &r_len);
	if (!ret && r_len) {
#if defined COMPAT_SAMPILE_CODE
		switch (compat_ver) {
		case VERSION_1:
			/*add data struct modification in here!*/
			break;
		case VERSION_2:
			/*add data struct modification in here!*/
			break;
		case VERSION_3:
			/*add data struct modification in here!*/
			break;
		default:
			break;
		}
#endif
		/* Version 1 Section */
		p = (struct sprdwl_cmd_fw_info *)r_buf;
		priv->chip_model = p->chip_model;
		priv->chip_ver = p->chip_version;
		priv->fw_ver = p->fw_version;
		priv->fw_capa = p->fw_capa;
		priv->fw_std = p->fw_std;
		priv->max_ap_assoc_sta = p->max_ap_assoc_sta;
		priv->max_acl_mac_addrs = p->max_acl_mac_addrs;
		priv->max_mc_mac_addrs = p->max_mc_mac_addrs;
		priv->wnm_ft_support = p->wnm_ft_support;
		len_count += SEC1_LEN;
		/*check sec2 data length got from fw*/
		if ((r_len - len_count) >= sizeof(struct wiphy_sec2_t)) {
			priv->wiphy_sec2_flag = 1;
			wl_info("save wiphy section2 info to sprdwl_priv\n");
			memcpy(&priv->wiphy_sec2, &p->wiphy_sec2,
					sizeof(struct wiphy_sec2_t));
		} else {
			goto out;
		}
		len_count += sizeof(struct wiphy_sec2_t);

		if ((r_len - len_count) >= ETH_ALEN) {
			ether_addr_copy(priv->mac_addr, p->mac_addr);
		} else {
			memset(priv->mac_addr, 0x00, ETH_ALEN);
			goto out;
		}
		len_count += ETH_ALEN;

		if ((r_len - len_count) >= 1)
			priv->credit_capa = p->credit_capa;
		else
			priv->credit_capa = TX_WITH_CREDIT;

		/* Version 2 Section */
		if (compat_ver == VERSION_1) {
			/* Set default value for non-version-1 variable */
			priv->ott_supt = OTT_NO_SUPT;
		} else {
			len_count = sizeof(struct sprdwl_cmd_fw_info);
			tlv = (struct sprdwl_tlv_data *)((u8 *)r_buf + len_count);
			while ((len_count + sizeof(struct sprdwl_tlv_data) + tlv->len) <= r_len) {
				b_tlv_data_chk = false;
				switch (tlv->type) {
				case GET_INFO_TLV_TP_OTT:
					if (tlv->len == 1) {
						priv->ott_supt = *((unsigned char *)(tlv->data));
						b_tlv_data_chk = true;
					}
					break;
				default:
					break;
				}

				wl_info("%s, TLV type=%d, len=%d, data_chk=%d\n",
					__func__, tlv->type, tlv->len, b_tlv_data_chk);

				if (b_tlv_data_chk == false) {
					wl_err("%s TLV check failed: type=%d, len=%d\n",
						   __func__, tlv->type, tlv->len);
					goto out;
				}

				len_count += (sizeof(struct sprdwl_tlv_data) + tlv->len);
				tlv = (struct sprdwl_tlv_data *)((u8 *)r_buf + len_count);
			}

			if (r_len_ori <= r_len) {
				wl_warn("%s check tlv rbuf size: r_len_ori=%d, r_len=%d\n",
					__func__, r_len_ori, r_len);
			}

			if (len_count != r_len) {
				wl_err("%s length mismatch: len_count=%d, r_len=%d\n",
					   __func__, len_count, r_len);
				goto out;
			}
		}

out:
		wl_err("%s, drv_version=%d, fw_version=%d, compat_ver=%d\n", __func__,
			(&priv->sync_api)->api_array[WIFI_CMD_GET_INFO].drv_version,
			(&priv->sync_api)->api_array[WIFI_CMD_GET_INFO].fw_version,
			compat_ver);
		wl_err("chip_model:0x%x, chip_ver:0x%x\n",
			priv->chip_model, priv->chip_ver);
		wl_err("fw_ver:%d, fw_std:0x%x, fw_capa:0x%x\n",
			priv->fw_ver, priv->fw_std, priv->fw_capa);
		if (is_valid_ether_addr(priv->mac_addr))
			wl_err("mac_addr:%02x:%02x:%02x:%02x:%02x:%02x\n",
				priv->mac_addr[0], priv->mac_addr[1], priv->mac_addr[2],
				priv->mac_addr[3], priv->mac_addr[4], priv->mac_addr[5]);
		wl_err("credit_capa:%s\n",
			(priv->credit_capa == TX_WITH_CREDIT) ?
			"TX_WITH_CREDIT" : "TX_NO_CREDIT");
		wl_err("ott support:%d\n", priv->ott_supt);
	}

	return ret;
}

int sprdwl_set_regdom(struct sprdwl_priv *priv, u8 *regdom, u32 len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_ieee80211_regdomain *p;
#ifdef COMPAT_SAMPILE_CODE
	u8 compat_ver = 0;
#endif

	msg = sprdwl_cmd_getbuf(priv, len, SPRDWL_MODE_NONE, SPRDWL_HEAD_RSP,
				WIFI_CMD_SET_REGDOM);
	if (!msg)
		return -ENOMEM;
#ifdef COMPAT_SAMPILE_CODE
	compat_ver = need_compat_operation(priv, WIFI_CMD_SET_REGDOM);
	if (compat_ver) {
		switch (compat_ver) {
		case VERSION_1:
		/*add data struct modification in here!*/
			priv->sync_api.compat = VERSION_1;
			break;
		case VERSION_2:
		/*add data struct modification in here!*/
			priv->sync_api.compat = VERSION_2;
			break;
		case VERSION_3:
			/*add data struct modification in here!*/
			priv->sync_api.compat = VERSION_3;
			break;
		default:
			break;
		}
	}
#endif
	p = (struct sprdwl_ieee80211_regdomain *)msg->data;
	memcpy(p, regdom, len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_open_fw(struct sprdwl_priv *priv, u8 *vif_ctx_id,
		   u8 mode, u8 *mac_addr)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_open *p;
	u16 rlen = 1;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), *vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_OPEN);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_open *)msg->data;
	p->mode = mode;
	if (mac_addr)
		memcpy(&p->mac[0], mac_addr, sizeof(p->mac));
	else
		wl_err("%s, %d, mac_addr error!\n", __func__, __LINE__);

	p->reserved = 0;
	if (0 != wfa_cap) {
		p->reserved = wfa_cap;
		wfa_cap = 0;
	}

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT,
					vif_ctx_id, &rlen);
}

int sprdwl_close_fw(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 mode)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_close *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_CLOSE);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_close *)msg->data;
	p->mode = mode;

	sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
	/* FIXME - in case of close failure */
	return 0;
}

int sprdwl_power_save(struct sprdwl_priv *priv, u8 vif_ctx_id,
			   u8 sub_type, u8 status)
{
	int ret;
	s32 ret_code;
	u16 len = 0;
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_power_save *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_POWER_SAVE);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_power_save *)msg->data;
	p->sub_type = sub_type;
	p->value = status;
	ret = sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, (u8 *)&ret_code, &len);

	return len == 4 ? ret_code : ret;
}

int sprdwl_add_key(struct sprdwl_priv *priv, u8 vif_ctx_id, const u8 *key_data,
		   u8 key_len, u8 pairwise, u8 key_index, const u8 *key_seq,
		   u8 cypher_type, const u8 *mac_addr)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_add_key *p;
	u8 *sub_cmd;
	int datalen = sizeof(*p) + sizeof(*sub_cmd) + key_len;

	msg = sprdwl_cmd_getbuf(priv, datalen, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_KEY);
	if (!msg)
		return -ENOMEM;

	sub_cmd = (u8 *)msg->data;
	*sub_cmd = SPRDWL_SUBCMD_ADD;
	p = (struct sprdwl_cmd_add_key *)(++sub_cmd);

	p->key_index = key_index;
	p->pairwise = pairwise;
	p->cypher_type = cypher_type;
	p->key_len = key_len;
	if (key_seq) {
		if (SPRDWL_CIPHER_WAPI == cypher_type)
			memcpy(p->keyseq, key_seq, WAPI_PN_SIZE);
		else
			memcpy(p->keyseq, key_seq, 8);
	}
	if (mac_addr)
		ether_addr_copy(p->mac, mac_addr);
	if (key_data)
		memcpy(p->value, key_data, key_len);

	if (mac_addr)
		reset_pn(priv, mac_addr);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_del_key(struct sprdwl_priv *priv, u8 vif_ctx_id, u16 key_index,
			bool pairwise, const u8 *mac_addr)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_del_key *p;
	u8 *sub_cmd;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + sizeof(*sub_cmd), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_KEY);
	if (!msg)
		return -ENOMEM;

	sub_cmd = (u8 *)msg->data;
	*sub_cmd = SPRDWL_SUBCMD_DEL;
	p = (struct sprdwl_cmd_del_key *)(++sub_cmd);

	p->key_index = key_index;
	p->pairwise = pairwise;
	if (mac_addr)
		ether_addr_copy(p->mac, mac_addr);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_def_key(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 key_index)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_def_key *p;
	u8 *sub_cmd;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + sizeof(*sub_cmd), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_KEY);
	if (!msg)
		return -ENOMEM;

	sub_cmd = (u8 *)msg->data;
	*sub_cmd = SPRDWL_SUBCMD_SET;
	p = (struct sprdwl_cmd_set_def_key *)(++sub_cmd);

	p->key_index = key_index;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_rekey_data(struct sprdwl_priv *priv, u8 vif_ctx_id,
		struct cfg80211_gtk_rekey_data *data)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_rekey *p;
	u8 *sub_cmd;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + sizeof(*sub_cmd), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_KEY);
	if (!msg)
		return -ENOMEM;
	sub_cmd = (u8 *)msg->data;
	*sub_cmd = SPRDWL_SUBCMD_REKEY;
	 p = (struct sprdwl_cmd_set_rekey *)(++sub_cmd);
	 memcpy(p->kek, data->kek, NL80211_KEK_LEN);
	 memcpy(p->kck, data->kck, NL80211_KCK_LEN);
	 memcpy(p->replay_ctr, data->replay_ctr, NL80211_REPLAY_CTR_LEN);
	 return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_ie(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 type,
		  const u8 *ie, u16 len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_ie *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_IE);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_set_ie *)msg->data;
	p->type = type;
	p->len = len;
	memcpy(p->data, ie, len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

#ifdef DFS_MASTER
int sprdwl_reset_beacon(struct sprdwl_priv *priv,
			 u8 vif_ctx_id, const u8 *beacon, u16 len)
{
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, len, vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_RESET_BEACON);
	if (!msg)
		return -ENOMEM;

	memcpy(msg->data, beacon, len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}
#endif

int sprdwl_start_ap(struct sprdwl_priv *priv,
			 u8 vif_ctx_id, u8 *beacon, u16 len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_start_ap *p;
	u16 datalen = sizeof(*p) + len;

	msg = sprdwl_cmd_getbuf(priv, datalen, vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_START_AP);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_start_ap *)msg->data;
	p->len = cpu_to_le16(len);
	memcpy(p->value, beacon, len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_del_station(struct sprdwl_priv *priv, u8 vif_ctx_id,
			   const u8 *mac_addr, u16 reason_code)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_del_station *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_DEL_STATION);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_del_station *)msg->data;
	if (mac_addr)
		memcpy(&p->mac[0], mac_addr, sizeof(p->mac));
	p->reason_code = cpu_to_le16(reason_code);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_get_station(struct sprdwl_priv *priv, u8 vif_ctx_id,
			struct sprdwl_cmd_get_station *sta)
{
	struct sprdwl_msg_buf *msg;
	u8 *r_buf = (u8 *)sta;
	u16 r_len = sizeof(struct sprdwl_cmd_get_station);
	int ret;

	msg = sprdwl_cmd_getbuf(priv, 0, vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_GET_STATION);
	if (!msg)
		return -ENOMEM;
	ret = sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, &r_len);

	return ret;
}

int sprdwl_set_channel(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 channel)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_channel *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_CHANNEL);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_set_channel *)msg->data;
	p->channel = channel;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_scan(struct sprdwl_priv *priv, u8 vif_ctx_id,
		u32 channels, int ssid_len, const u8 *ssid_list,
		u16 chn_count_5g, const u16 *chns_5g)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_scan *p;
	struct sprdwl_cmd_rsp_state_code state;
	u16 rlen;
	u32 data_len, chns_len_5g;

	struct sprdwl_5g_chn {
		u16 n_5g_chn;
		u16 chns[0];
	} *ext_5g;

	chns_len_5g = chn_count_5g * sizeof(*chns_5g);
	data_len = sizeof(*p) + ssid_len + chns_len_5g +
		sizeof(ext_5g->n_5g_chn);
	msg = sprdwl_cmd_getbuf(priv, data_len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_scan *)msg->data;
	p->channels = channels;
	if (ssid_len > 0) {
		memcpy(p->ssid, ssid_list, ssid_len);
		p->ssid_len = cpu_to_le16(ssid_len);
	}

	ext_5g = (struct sprdwl_5g_chn *)(p->ssid + ssid_len);
	if (chn_count_5g > 0) {
		ext_5g->n_5g_chn = chn_count_5g;
		memcpy(ext_5g->chns, chns_5g, chns_len_5g);
	} else {
		ext_5g->n_5g_chn = 0;
	}

	wl_hex_dump(L_DBG, "scan hex:", DUMP_PREFIX_OFFSET,
				 16, 1, p, data_len, true);

	rlen = sizeof(state);

	return	sprdwl_cmd_send_recv(priv, msg, CMD_SCAN_WAIT_TIMEOUT,
				 (u8 *)&state, &rlen);
}

int sprdwl_sched_scan_start(struct sprdwl_priv *priv, u8 vif_ctx_id,
				 struct sprdwl_sched_scan_buf *buf)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_sched_scan_hd *sscan_head = NULL;
	struct sprdwl_cmd_sched_scan_ie_hd *ie_head = NULL;
	struct sprdwl_cmd_sched_scan_ifrc *sscan_ifrc = NULL;
	u16 datalen;
	u8 *p = NULL;
	int len = 0, i, hd_len;

	datalen = sizeof(*sscan_head) + sizeof(*ie_head) + sizeof(*sscan_ifrc)
		+ buf->n_ssids * IEEE80211_MAX_SSID_LEN
		+ buf->n_match_ssids * IEEE80211_MAX_SSID_LEN + buf->ie_len;
	hd_len = sizeof(*ie_head);
	datalen = datalen + (buf->n_ssids ? hd_len : 0)
		+ (buf->n_match_ssids ? hd_len : 0)
		+ (buf->ie_len ? hd_len : 0);

	msg = sprdwl_cmd_getbuf(priv, datalen, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SCHED_SCAN);
	if (!msg)
		return -ENOMEM;

	p = msg->data;

	sscan_head = (struct sprdwl_cmd_sched_scan_hd *)(p + len);
	sscan_head->started = 1;
	sscan_head->buf_flags = SPRDWL_SCHED_SCAN_BUF_END;
	len += sizeof(*sscan_head);

	ie_head = (struct sprdwl_cmd_sched_scan_ie_hd *)(p + len);
	ie_head->ie_flag = SPRDWL_SEND_FLAG_IFRC;
	ie_head->ie_len = sizeof(*sscan_ifrc);
	len += sizeof(*ie_head);

	sscan_ifrc = (struct sprdwl_cmd_sched_scan_ifrc *)(p + len);

	sscan_ifrc->interval = buf->interval;
	sscan_ifrc->flags = buf->flags;
	sscan_ifrc->rssi_thold = buf->rssi_thold;
	memcpy(sscan_ifrc->chan, buf->channel, TOTAL_2G_5G_CHANNEL_NUM + 1);

	len += ie_head->ie_len;

	if (buf->n_ssids > 0) {
		ie_head = (struct sprdwl_cmd_sched_scan_ie_hd *)(p + len);
		ie_head->ie_flag = SPRDWL_SEND_FLAG_SSID;
		ie_head->ie_len = buf->n_ssids * IEEE80211_MAX_SSID_LEN;
		len += sizeof(*ie_head);
		for (i = 0; i < buf->n_ssids; i++) {
			memcpy((p + len + i * IEEE80211_MAX_SSID_LEN),
				   buf->ssid[i], IEEE80211_MAX_SSID_LEN);
		}
		len += ie_head->ie_len;
	}

	if (buf->n_match_ssids > 0) {
		ie_head = (struct sprdwl_cmd_sched_scan_ie_hd *)(p + len);
		ie_head->ie_flag = SPRDWL_SEND_FLAG_MSSID;
		ie_head->ie_len = buf->n_match_ssids * IEEE80211_MAX_SSID_LEN;
		len += sizeof(*ie_head);
		for (i = 0; i < buf->n_match_ssids; i++) {
			memcpy((p + len + i * IEEE80211_MAX_SSID_LEN),
				   buf->mssid[i], IEEE80211_MAX_SSID_LEN);
		}
		len += ie_head->ie_len;
	}

	if (buf->ie_len > 0) {
		ie_head = (struct sprdwl_cmd_sched_scan_ie_hd *)(p + len);
		ie_head->ie_flag = SPRDWL_SEND_FLAG_IE;
		ie_head->ie_len = buf->ie_len;
		len += sizeof(*ie_head);

		wl_info("%s: ie len is %zu, ie:%s\n",
			__func__, buf->ie_len, buf->ie);
		memcpy((p + len), buf->ie, buf->ie_len);
		len += ie_head->ie_len;
	}

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_sched_scan_stop(struct sprdwl_priv *priv, u8 vif_ctx_id)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_sched_scan_hd *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SCHED_SCAN);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_sched_scan_hd *)msg->data;
	p->started = 0;
	p->buf_flags = SPRDWL_SCHED_SCAN_BUF_END;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_connect(struct sprdwl_priv *priv, u8 vif_ctx_id,
			struct sprdwl_cmd_connect *p)
{
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_CONNECT);
	if (!msg)
		return -ENOMEM;

	memcpy(msg->data, p, sizeof(*p));

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_disconnect(struct sprdwl_priv *priv, u8 vif_ctx_id, u16 reason_code)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_disconnect *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_DISCONNECT);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_disconnect *)msg->data;
	p->reason_code = cpu_to_le16(reason_code);

	return sprdwl_cmd_send_recv(priv, msg, CMD_DISCONNECT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_param(struct sprdwl_priv *priv, u32 rts, u32 frag)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_param *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), SPRDWL_MODE_NONE,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_PARAM);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_set_param *)msg->data;
	p->rts = cpu_to_le32(rts);
	p->frag = cpu_to_le32(frag);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_pmksa(struct sprdwl_priv *priv, u8 vif_ctx_id,
		  const u8 *bssid, const u8 *pmkid, u8 type)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_pmkid *p;
	u8 *sub_cmd;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + sizeof(*sub_cmd), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_PMKSA);
	if (!msg)
		return -ENOMEM;

	sub_cmd = (u8 *)msg->data;
	*sub_cmd = type;
	p = (struct sprdwl_cmd_pmkid *)(++sub_cmd);

	if (bssid)
		memcpy(p->bssid, bssid, sizeof(p->bssid));
	if (pmkid)
		memcpy(p->pmkid, pmkid, sizeof(p->pmkid));

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_qos_map(struct sprdwl_priv *priv, u8 vif_ctx_id, void *qos_map)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_qos_map *p;
	int index;

	if (!qos_map)
		return 0;
	msg =
	    sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, 1,
			      WIFI_CMD_SET_QOS_MAP);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_qos_map *)msg->data;
	memset((u8 *)p, 0, sizeof(*p));
	memcpy((u8 *)p, qos_map, sizeof(*p));
	memcpy(&g_11u_qos_map.qos_exceptions[0], &p->dscp_exception[0],
		sizeof(struct sprdwl_cmd_dscp_exception) * QOS_MAP_MAX_DSCP_EXCEPTION);

	for (index = 0; index < 8; index++) {
		g_11u_qos_map.qos_ranges[index].low = p->up[index].low;
		g_11u_qos_map.qos_ranges[index].high = p->up[index].high;
		g_11u_qos_map.qos_ranges[index].up = index;
	}

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_gscan_subcmd(struct sprdwl_priv *priv, u8 vif_ctx_id,
			void *data, u16 subcmd, u16 len, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len,
				vif_ctx_id, SPRDWL_HEAD_RSP, WIFI_CMD_GSCAN);

	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = subcmd;

	if (data != NULL) {
		p->data_len = len;
		memcpy(p->data, data, len);
	} else{
		p->data_len = 0;
	}
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);

}

int sprdwl_set_gscan_config(struct sprdwl_priv *priv, u8 vif_ctx_id,
				void *data, u16 len, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len,
				vif_ctx_id, 1, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_GSCAN_SUBCMD_SET_CONFIG;
	p->data_len = len;
	memcpy(p->data, data, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_set_gscan_scan_config(struct sprdwl_priv *priv, u8 vif_ctx_id,
				 void *data, u16 len, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len,
				vif_ctx_id, 1, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_GSCAN_SUBCMD_SET_SCAN_CONFIG;
	p->data_len = len;
	memcpy(p->data, data, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_enable_gscan(struct sprdwl_priv *priv, u8 vif_ctx_id, void *data,
			u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + sizeof(int),
				vif_ctx_id, 1, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_GSCAN_SUBCMD_ENABLE_GSCAN;
	p->data_len = sizeof(int);
	memcpy(p->data, data, p->data_len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_get_gscan_capabilities(struct sprdwl_priv *priv, u8 vif_ctx_id,
				  u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_GSCAN_SUBCMD_GET_CAPABILITIES;
	p->data_len = 0;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_get_gscan_channel_list(struct sprdwl_priv *priv, u8 vif_ctx_id,
				  void *data, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	int *band;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p)+sizeof(*band), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;


	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_GSCAN_SUBCMD_GET_CHANNEL_LIST;
	p->data_len = sizeof(*band);

	band = (int *)(msg->data + sizeof(struct sprd_cmd_gscan_header));
	*band = *((int *)data);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_set_gscan_bssid_hotlist(struct sprdwl_priv *priv, u8 vif_ctx_id,
			    void *data, u16 len, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len,
				vif_ctx_id, 1, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_GSCAN_SUBCMD_SET_HOTLIST;
	p->data_len = len;
	memcpy(p->data, data, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_set_gscan_bssid_blacklist(struct sprdwl_priv *priv, u8 vif_ctx_id,
				void *data, u16 len, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;
	struct sprd_cmd_gscan_header *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len,
				vif_ctx_id, 1, WIFI_CMD_GSCAN);
	if (!msg)
		return -ENOMEM;

	p = (struct sprd_cmd_gscan_header *)msg->data;
	p->subcmd = SPRDWL_WIFI_SUBCMD_SET_BSSID_BLACKLIST;
	p->data_len = len;
	memcpy(p->data, data, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}
int sprdwl_add_tx_ts(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 tsid,
		     const u8 *peer, u8 user_prio, u16 admitted_time)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_tx_ts *p;
#ifdef WMMAC_WFA_CERTIFICATION
	edca_ac_t ac;
#endif

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, 1,
				WIFI_CMD_ADD_TX_TS);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_tx_ts *)msg->data;
	memset((u8 *)p, 0, sizeof(*p));

	p->tsid = tsid;
	ether_addr_copy(p->peer, peer);
	p->user_prio = user_prio;
	p->admitted_time = cpu_to_le16(admitted_time);

#ifdef WMMAC_WFA_CERTIFICATION
	ac = map_priority_to_edca_ac(p->user_prio);
	update_wmmac_ts_info(p->tsid, p->user_prio, ac, true, p->admitted_time);
	update_admitted_time(priv, p->tsid, p->admitted_time, true);
#endif
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_del_tx_ts(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 tsid,
			 const u8 *peer)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_tx_ts *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, 1,
				WIFI_CMD_DEL_TX_TS);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_tx_ts *)msg->data;
	memset((u8 *)p, 0, sizeof(*p));

	p->tsid = tsid;
	ether_addr_copy(p->peer, peer);

#ifdef WMMAC_WFA_CERTIFICATION
	update_admitted_time(priv, p->tsid, p->admitted_time, false);
	remove_wmmac_ts_info(p->tsid);
#endif
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_remain_chan(struct sprdwl_priv *priv, u8 vif_ctx_id,
			   struct ieee80211_channel *channel,
			   enum nl80211_channel_type channel_type,
			   u32 duration, u64 *cookie)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_remain_chan *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_REMAIN_CHAN);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_remain_chan *)msg->data;
	p->chan = ieee80211_frequency_to_channel(channel->center_freq);
	p->chan_type = channel_type;
	p->duraion = cpu_to_le32(duration);
	p->cookie = cpu_to_le64(*cookie);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_cancel_remain_chan(struct sprdwl_priv *priv,
				   u8 vif_ctx_id, u64 cookie)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_cancel_remain_chan *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_CANCEL_REMAIN_CHAN);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_cancel_remain_chan *)msg->data;
	p->cookie = cpu_to_le64(cookie);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}
#if 0
static int sprdwl_tx_data(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 channel,
			  u8 dont_wait_for_ack, u32 wait, u64 *cookie,
			  const u8 *buf, size_t len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_mgmt_tx *p;
	u16 datalen = sizeof(*p) + len;

	msg = sprdwl_cmd_getbuf(priv, datalen, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_TX_MGMT);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_mgmt_tx *)msg->data;

	p->chan = channel;
	p->dont_wait_for_ack = dont_wait_for_ack;
	p->wait = cpu_to_le32(wait);
	if (cookie)
		p->cookie = cpu_to_le64(*cookie);
	p->len = cpu_to_le16(len);
	memcpy(p->value, buf, len);

	return sprdwl_cmd_send_to_ic(priv, msg);
}
#endif

int sprdwl_tx_mgmt(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 channel,
		   u8 dont_wait_for_ack, u32 wait, u64 *cookie,
		   const u8 *buf, size_t len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_mgmt_tx *p;
	u16 datalen = sizeof(*p) + len;

	msg = sprdwl_cmd_getbuf(priv, datalen, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_TX_MGMT);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_mgmt_tx *)msg->data;

	p->chan = channel;
	p->dont_wait_for_ack = dont_wait_for_ack;
	p->wait = cpu_to_le32(wait);
	if (cookie)
		p->cookie = cpu_to_le64(*cookie);
	p->len = cpu_to_le16(len);
	memcpy(p->value, buf, len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_register_frame(struct sprdwl_priv *priv, u8 vif_ctx_id,
			  u16 type, u8 reg)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_register_frame *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_REGISTER_FRAME);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_register_frame *)msg->data;
	p->type = type;
	p->reg = reg;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_cqm_rssi(struct sprdwl_priv *priv, u8 vif_ctx_id,
			s32 rssi_thold, u32 rssi_hyst)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_cqm_rssi *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_CQM);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_cqm_rssi *)msg->data;
	p->rssih = cpu_to_le32(rssi_thold);
	p->rssil = cpu_to_le32(rssi_hyst);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_roam_offload(struct sprdwl_priv *priv, u8 vif_ctx_id,
				u8 sub_type, const u8 *data, u8 len)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_roam_offload_data *p;

	if (!(priv->fw_capa & SPRDWL_CAPA_11R_ROAM_OFFLOAD))	{
		wl_err("%s, not supported\n", __func__);
		return -ENOTSUPP;
	}
	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_ROAM_OFFLOAD);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_roam_offload_data *)msg->data;
	p->type = sub_type;
	p->len = len;
	memcpy(p->value, data, len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_tdls_mgmt(struct sprdwl_vif *vif, struct sk_buff *skb)
{
	int ret;

	/* temp debug use */
	if (skb_headroom(skb) < vif->ndev->needed_headroom)
		wl_err("%s skb head len err:%d %d\n",
		       __func__, skb_headroom(skb),
		       vif->ndev->needed_headroom);

	/*send TDLS mgmt through cmd port instead of data port,needed by CP2*/
	ret = sprdwl_send_tdlsdata_use_cmd(skb, vif, 1);
	if (ret) {
		wl_err("%s drop msg due to TX Err\n",
		       __func__);
		goto out;
	}

	vif->ndev->stats.tx_bytes += skb->len;
	vif->ndev->stats.tx_packets++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0)
	vif->ndev->trans_start = jiffies;
#else
	netif_trans_update(vif->ndev);
#endif
	wl_hex_dump(L_DBG, "TX packet: ", DUMP_PREFIX_OFFSET,
				 16, 1, skb->data, skb->len, 0);

out:
	return ret;
}

int sprdwl_send_tdls_cmd(struct sprdwl_vif *vif, u8 vif_ctx_id, const u8 *peer,
			 int oper)
{
	struct sprdwl_work *misc_work;
	struct sprdwl_tdls_work tdls;

	tdls.vif_ctx_id = vif_ctx_id;
	if (peer)
		ether_addr_copy(tdls.peer, peer);
	tdls.oper = oper;

	misc_work = sprdwl_alloc_work(sizeof(struct sprdwl_tdls_work));
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return -1;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_TDLS_CMD;
	memcpy(misc_work->data, &tdls, sizeof(struct sprdwl_tdls_work));

	sprdwl_queue_work(vif->priv, misc_work);
	return 0;
}

int sprdwl_tdls_oper(struct sprdwl_priv *priv, u8 vif_ctx_id, const u8 *peer,
			 int oper)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_tdls *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_TDLS);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_tdls *)msg->data;
	if (peer)
		ether_addr_copy(p->da, peer);
	p->tdls_sub_cmd_mgmt = oper;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_start_tdls_channel_switch(struct sprdwl_priv *priv, u8 vif_ctx_id,
					 const u8 *peer_mac, u8 primary_chan,
					 u8 second_chan_offset, u8 band)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_tdls *p;
	struct sprdwl_cmd_tdls_channel_switch chan_switch;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + sizeof(chan_switch),
				vif_ctx_id, SPRDWL_HEAD_RSP, WIFI_CMD_TDLS);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_tdls *)msg->data;
	p->tdls_sub_cmd_mgmt = SPRDWL_TDLS_START_CHANNEL_SWITCH;
	if (peer_mac)
		ether_addr_copy(p->da, peer_mac);
	p->initiator = 1;
	chan_switch.primary_chan = primary_chan;
	chan_switch.second_chan_offset = second_chan_offset;
	chan_switch.band = band;
	p->paylen = sizeof(chan_switch);
	memcpy(p->payload, &chan_switch, p->paylen);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_cancel_tdls_channel_switch(struct sprdwl_priv *priv, u8 vif_ctx_id,
					  const u8 *peer_mac)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_tdls *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_TDLS);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_tdls *)msg->data;
	p->tdls_sub_cmd_mgmt = SPRDWL_TDLS_CANCEL_CHANNEL_SWITCH;
	if (peer_mac)
		ether_addr_copy(p->da, peer_mac);
	p->initiator = 1;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_notify_ip(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 ip_type,
			 u8 *ip_addr)
{
	struct sprdwl_msg_buf *msg;
	u8 *ip_value;
	u8 ip_len;

	if (ip_type != SPRDWL_IPV4 && ip_type != SPRDWL_IPV6)
		return -EINVAL;
	ip_len = (ip_type == SPRDWL_IPV4) ?
		SPRDWL_IPV4_ADDR_LEN : SPRDWL_IPV6_ADDR_LEN;
	msg = sprdwl_cmd_getbuf(priv, ip_len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_NOTIFY_IP_ACQUIRED);
	if (!msg)
		return -ENOMEM;
	ip_value = (unsigned char *)msg->data;
	memcpy(ip_value, ip_addr, ip_len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_blacklist(struct sprdwl_priv *priv,
			 u8 vif_ctx_id, u8 sub_type, u8 num, u8 *mac_addr)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_blacklist *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + num * ETH_ALEN,
				vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_SET_BLACKLIST);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_blacklist *)msg->data;
	p->sub_type = sub_type;
	p->num = num;
	if (mac_addr)
		memcpy(p->mac, mac_addr, num * ETH_ALEN);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_whitelist(struct sprdwl_priv *priv, u8 vif_ctx_id,
			 u8 sub_type, u8 num, u8 *mac_addr)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_mac_addr *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + num * ETH_ALEN,
				vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_SET_WHITELIST);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_set_mac_addr *)msg->data;
	p->sub_type = sub_type;
	p->num = num;
	if (mac_addr)
		memcpy(p->mac, mac_addr, num * ETH_ALEN);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_mc_filter(struct sprdwl_priv *priv,  u8 vif_ctx_id,
			 u8 sub_type, u8 num, u8 *mac_addr)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_mac_addr *p;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + num * ETH_ALEN,
				vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_MULTICAST_FILTER);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_set_mac_addr *)msg->data;
	p->sub_type = sub_type;
	p->num = num;
	if (num && mac_addr)
		memcpy(p->mac, mac_addr, num * ETH_ALEN);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_npi_send_recv(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 *s_buf,
			 u16 s_len, u8 *r_buf, u16 *r_len)
{
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, s_len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_NPI_MSG);
	if (!msg)
		return -ENOMEM;
	memcpy(msg->data, s_buf, s_len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, r_len);
}

int sprdwl_set_11v_feature_support(struct sprdwl_priv *priv,
				   u8 vif_ctx_id, u16 val)
{
	struct sprdwl_msg_buf *msg = NULL;
	struct sprdwl_cmd_rsp_state_code state;
	struct sprdwl_cmd_11v *p = NULL;
	u16 rlen = sizeof(state);

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, 1, WIFI_CMD_11V);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_11v *)msg->data;

	p->cmd = SPRDWL_SUBCMD_SET;
	p->value = (val << 16) | val;
	/* len  only 8 =  cmd(2) + len(2) +value(4)*/
	p->len = 8;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT,
					(u8 *)&state, &rlen);
}

int sprdwl_set_11v_sleep_mode(struct sprdwl_priv *priv, u8 vif_ctx_id,
				  u8 status, u16 interval)
{
	struct sprdwl_msg_buf *msg = NULL;
	struct sprdwl_cmd_rsp_state_code state;
	struct sprdwl_cmd_11v *p = NULL;
	u16 rlen = sizeof(state);
	u32 value = 0;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id, 1, WIFI_CMD_11V);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_11v *)msg->data;

	p->cmd = SPRDWL_SUBCMD_ENABLE;
	/* 24-31 feature 16-23 status 0-15 interval */
	value = SPRDWL_11V_SLEEP << 8;
	value = (value | status) << 16;
	value = value | interval;
	p->value = value;
	/* len  only 8 =  cmd(2) + len(2) +value(4)*/
	p->len = 8;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT,
					(u8 *)&state, &rlen);
}

int sprdwl_send_ba_mgmt(struct sprdwl_priv *priv, u8 vif_ctx_id,
			void *data, u16 len)
{
	struct sprdwl_msg_buf *msg = NULL;

	msg = sprdwl_cmd_getbuf(priv, sizeof(struct sprdwl_cmd_ba),
				vif_ctx_id, 1, WIFI_CMD_BA);
	if (!msg)
		return -ENOMEM;

	memcpy(msg->data, data, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

int sprdwl_set_max_clients_allowed(struct sprdwl_priv *priv,
				   u8 vif_ctx_id, int n_clients)
{
	int *max;
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*max), vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_SET_MAX_CLIENTS_ALLOWED);
	if (!msg)
		return -ENOMEM;
	*(int *)msg->data = n_clients;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

void sprdwl_add_hang_cmd(struct sprdwl_vif *vif)
{
	struct sprdwl_work *misc_work;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	if (sprdwl_intf_is_exit(vif->priv)
		|| (tx_msg->hang_recovery_status == HANG_RECOVERY_ACKED
		&& cmd->cmd_id != WIFI_CMD_HANG_RECEIVED)) {
		complete(&cmd->completed);
	}
	misc_work = sprdwl_alloc_work(0);
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_HANG_RECEIVED;

	sprdwl_queue_work(vif->priv, misc_work);
}

void sprdwl_add_close_cmd(struct sprdwl_vif *vif, enum sprdwl_mode mode)
{
	struct sprdwl_work *misc_work;

	misc_work = sprdwl_alloc_work(1);
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_SEND_CLOSE;
	memcpy(misc_work->data, &mode, 1);

	sprdwl_queue_work(vif->priv, misc_work);
}

/* CP2 send WIFI_EVENT_HANG_RECOVERY to Driver,
* then Driver need to send a WIFI_CMD_HANG_RECEIVED cmd to CP2
* to notify that CP2 can reset credit now.
*/
int sprdwl_send_hang_received_cmd(struct sprdwl_priv *priv, u8 vif_ctx_id)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	msg = sprdwl_cmd_getbuf(priv, 0, vif_ctx_id, SPRDWL_HEAD_RSP,
				WIFI_CMD_HANG_RECEIVED);
	if (!msg)
		return -ENOMEM;
	tx_msg->hang_recovery_status = HANG_RECOVERY_ACKED;
	return sprdwl_cmd_send_recv(priv, msg,
			CMD_WAIT_TIMEOUT, NULL, NULL);
}

void sprdwl_send_assert_cmd(struct sprdwl_vif *vif, u8 cmd_id, u8 reason)
{
	struct sprdwl_work *misc_work;
	struct sprdwl_assert_info *assert_info;

	misc_work = sprdwl_alloc_work(sizeof(struct sprdwl_assert_info));
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_ASSERT;
	assert_info = (struct sprdwl_assert_info *)(misc_work->data);
	assert_info->cmd_id = cmd_id;
	assert_info->reason = reason;

	sprdwl_queue_work(vif->priv, misc_work);
}

/* add a reason to CMD assert
* 0:scan time out
* 1:rsp cnt lost
*/
int wlan_set_assert(struct sprdwl_priv *priv, u8 vif_ctx_id, u8 cmd_id, u8 reason)
{
#ifndef ATCMD_ASSERT
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_set_assert *p;

	return 0;
	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), vif_ctx_id,
			SPRDWL_HEAD_RSP, WIFI_CMD_ERR);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_set_assert *)msg->data;
	p->reason = reason;

	return sprdwl_cmd_send_recv(priv, msg,
			CMD_WAIT_TIMEOUT, NULL, NULL);
#else
	return sprdwl_atcmd_assert(priv, vif_ctx_id, cmd_id, reason);
#endif
}

int sprdwl_send_data2cmd(struct sprdwl_priv *priv, u8 vif_ctx_id,
				void *data, u16 len)
{
	struct sprdwl_msg_buf *msg = NULL;

	msg = sprdwl_cmd_getbuf(priv, len, vif_ctx_id,
			SPRDWL_HEAD_RSP, WIFI_CMD_TX_DATA);
	if (!msg)
		return -ENOMEM;
	memcpy(msg->data, data, len);
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}


int sprdwl_xmit_data2cmd(struct sk_buff *skb, struct net_device *ndev)
{
#define FLAG_SIZE  5
	struct sprdwl_vif *vif = netdev_priv(ndev);
	struct sprdwl_msg_buf *msg;
	u8 *temp_flag = "01234";
	struct tx_msdu_dscr *dscr;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;

	if (unlikely(atomic_read(&cmd->refcnt) > 0)) {
		wl_err("%s, cmd->refcnt = %d, Try later again\n",
			   __func__, atomic_read(&cmd->refcnt));
		return -EAGAIN;
	}

	if (skb->protocol == cpu_to_be16(ETH_P_PAE)) {
		u8 *data = (u8 *)(skb->data) + sizeof(struct ethhdr);
		struct sprdwl_eap_hdr *eap = (struct sprdwl_eap_hdr *)data;

		if (eap->type == EAP_PACKET_TYPE && eap->opcode == EAP_WSC_DONE) {
			wl_info("%s, EAP_WSC_DONE!\n", __func__);
			vif->wps_flag = 1;
		}
	}
	/*fill dscr header first*/
	if (sprdwl_intf_fill_msdu_dscr(vif, skb, SPRDWL_TYPE_CMD, 0)) {
		dev_kfree_skb(skb);
		return -EPERM;
	}
	/*alloc five byte for fw 16 byte need
	 *dscr:11+flag:5 =16
	 */
	skb_push(skb, FLAG_SIZE);
	memcpy(skb->data, temp_flag, FLAG_SIZE);
	/*malloc msg buffer*/
	msg = sprdwl_cmd_getbuf_atomic(vif->priv, skb->len, vif->ctx_id,
					   SPRDWL_HEAD_RSP, WIFI_CMD_TX_DATA);
	if (!msg) {
		wl_err("%s, %d, getmsgbuf fail, free skb\n",
			   __func__, __LINE__);
		dev_kfree_skb(skb);
		return -ENOMEM;
	}
	/*send group in BK to avoid FW hang*/
	dscr = (struct tx_msdu_dscr *)skb->data;
	if ((vif->mode == SPRDWL_MODE_AP ||
		 vif->mode == SPRDWL_MODE_P2P_GO) &&
		 (dscr->sta_lut_index < 6)) {
		dscr->buffer_info.msdu_tid = prio_1;
		wl_info("%s, %d, SOFTAP/GO group go as BK\n", __func__, __LINE__);
	}

	memcpy(msg->data, skb->data, skb->len);
	dev_kfree_skb(skb);

	return sprdwl_cmd_send_to_ic(vif->priv, msg);
}

int sprdwl_xmit_data2cmd_wq(struct sk_buff *skb, struct net_device *ndev)
{
#define FLAG_SIZE  5
	struct sprdwl_vif *vif = netdev_priv(ndev);
	u8 *temp_flag = "01234";
	struct tx_msdu_dscr *dscr;
	struct sprdwl_work *misc_work = NULL;

	/*fill dscr header first*/
	if (sprdwl_intf_fill_msdu_dscr(vif, skb, SPRDWL_TYPE_CMD, 0)) {
		dev_kfree_skb(skb);
		return -EPERM;
	}
	/*alloc five byte for fw 16 byte need
	 *dscr:11+flag:5 =16
	 */
	skb_push(skb, FLAG_SIZE);
	memcpy(skb->data, temp_flag, FLAG_SIZE),
	/*send group in BK to avoid FW hang*/
	dscr = (struct tx_msdu_dscr *)skb->data;
	if ((vif->mode == SPRDWL_MODE_AP ||
		vif->mode == SPRDWL_MODE_P2P_GO) &&
		(dscr->sta_lut_index < 6)) {
		dscr->buffer_info.msdu_tid = prio_1;
		wl_info("%s, %d, SOFTAP/GO group go as BK\n", __func__, __LINE__);
	}

	/*create work queue*/
	misc_work = sprdwl_alloc_work(skb->len);
	if (!misc_work) {
		wl_err("%s:work queue alloc failure\n", __func__);
		dev_kfree_skb(skb);
		return -1;
	}
	memcpy(misc_work->data, skb->data, skb->len);
	dev_kfree_skb(skb);
	misc_work->vif = vif;
	misc_work->id = SPRDWL_CMD_TX_DATA;
	sprdwl_queue_work(vif->priv, misc_work);

	return 0;
}

int sprdwl_send_vowifi_data_prot(struct sprdwl_priv *priv, u8 ctx_id,
				 void *data, int len)
{
	struct sprdwl_msg_buf *msg;

	wl_info("enter--at %s\n", __func__);

	if (priv == NULL)
		return -EINVAL;

	msg = sprdwl_cmd_getbuf(priv, 0, ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_VOWIFI_DATA_PROTECT);
	if (!msg)
		return -ENOMEM;

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

void sprdwl_vowifi_data_protection(struct sprdwl_vif *vif)
{
	struct sprdwl_work *misc_work;

	wl_info("enter--at %s\n", __func__);

	misc_work = sprdwl_alloc_work(0);
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_VOWIFI_DATA_PROTECTION;

	sprdwl_queue_work(vif->priv, misc_work);
}

void sprdwl_work_host_wakeup_fw(struct sprdwl_vif *vif)
{
	struct sprdwl_work *misc_work;

	misc_work = sprdwl_alloc_work(0);
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return;
	}
	if (!vif) {
		wl_err("%s vif is null!\n", __func__);
		return;
	}
	if (!vif->priv) {
		wl_err("%s priv is null!\n", __func__);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_HOST_WAKEUP_FW;

	sprdwl_queue_work(vif->priv, misc_work);
}

int sprdwl_cmd_host_wakeup_fw(struct sprdwl_priv *priv, u8 ctx_id)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_power_save *p;
	u8 r_buf = -1;
	u16 r_len = 1;
	int ret = 0;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_POWER_SAVE);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_power_save *)msg->data;
	p->sub_type = SPRDWL_HOST_WAKEUP_FW;
	p->value = 0;

	ret =  sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT,
					&r_buf, &r_len);

	if (!ret && (1 == r_buf)) {
		intf->fw_awake = 1;
		tx_up(tx_msg);
	} else {
		intf->fw_awake = 0;
		intf->fw_power_down = 1;
		tx_up(tx_msg);
		wl_err("host wakeup fw cmd failed, ret=%d\n", ret);
	}

	return ret;
}

int sprdwl_cmd_req_lte_concur(struct sprdwl_priv *priv, u8 ctx_id, u8 user_channel)
{
	struct sprdwl_msg_buf *msg;

	msg = sprdwl_cmd_getbuf(priv, sizeof(user_channel), ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_REQ_LTE_CONCUR);
	if (!msg)
		return -ENOMEM;

	*(u8 *)msg->data = user_channel;
	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

static int handle_rsp_status_err(u8 cmd_id, s8 status)
{
	int flag = 0;

	switch (cmd_id) {
	case WIFI_CMD_DOWNLOAD_INI:
		 if ((SPRDWL_CMD_STATUS_CRC_ERROR == status) ||
			(SPRDWL_CMD_STATUS_INI_INDEX_ERROR == status) ||
			(SPRDWL_CMD_STATUS_LENGTH_ERROR == status))
			flag = -1;
		 break;
	default:
		 flag = 0;
		 break;
	}

	return flag;
}

/* retrun the msg length or 0 */
unsigned short sprdwl_rx_rsp_process(struct sprdwl_priv *priv, u8 *msg)
{
	u16 plen;
	void *data;
	int handle_flag = 0;
	struct sprdwl_cmd *cmd = &g_sprdwl_cmd;
	struct sprdwl_cmd_hdr *hdr;

	if (unlikely(!cmd->init_ok)) {
		wl_info("%s cmd coming too early, drop it\n", __func__);
		return 0;
	}

	hdr = (struct sprdwl_cmd_hdr *)msg;
	plen = SPRDWL_GET_LE16(hdr->plen);

	/* 2048 use mac */
	/*TODO here ctx_id range*/
#ifndef CONFIG_P2P_INTF
	if (hdr->common.ctx_id > STAP_MODE_P2P_DEVICE ||
#else
	if (hdr->common.ctx_id >= STAP_MODE_COEXI_NUM ||
#endif
		hdr->cmd_id > WIFI_CMD_MAX ||
		plen > 2048) {
		wl_err("%s wrong CMD_RSP: ctx_id:%d;cmd_id:%d\n",
			   __func__, hdr->common.ctx_id,
			   hdr->cmd_id);
		return 0;
	}
	if (atomic_inc_return(&cmd->refcnt) >= SPRDWL_CMD_EXIT_VAL) {
		atomic_dec(&cmd->refcnt);
		wl_err("cmd->refcnt=%x\n", atomic_read(&cmd->refcnt));
		return 0;
	}
	data = kmalloc(plen, GFP_KERNEL);
	if (!data) {
		atomic_dec(&cmd->refcnt);
		wl_err("cmd->refcnt=%x\n", atomic_read(&cmd->refcnt));
		return plen;
	}
	memcpy(data, (void *)hdr, plen);

	spin_lock_bh(&cmd->lock);
	if (!cmd->data && SPRDWL_GET_LE32(hdr->mstime) == cmd->mstime &&
		hdr->cmd_id == cmd->cmd_id) {
		wl_debug("ctx_id %d recv rsp[%s]\n",
			hdr->common.ctx_id, cmd2str(hdr->cmd_id));
		if (unlikely(hdr->status != 0)) {
			wl_debug("%s ctx_id %d recv rsp[%s] status[%s]\n",
				   __func__, hdr->common.ctx_id,
				   cmd2str(hdr->cmd_id),
				   err2str(hdr->status));
			handle_flag = handle_rsp_status_err(hdr->cmd_id,
						hdr->status);
		}
		cmd->data = data;
		complete(&cmd->completed);
	} else {
		kfree(data);
		wl_err("%s ctx_id %d recv mismatched rsp[%s] status[%s]\n",
			   __func__, hdr->common.ctx_id,
			   cmd2str(hdr->cmd_id),
			   err2str(hdr->status));
		wl_err("%s mstime:[%u %u]\n", __func__,
			   SPRDWL_GET_LE32(hdr->mstime), cmd->mstime);
	}
	spin_unlock_bh(&cmd->lock);
	atomic_dec(&cmd->refcnt);
	wl_debug("cmd->refcnt=%x\n", atomic_read(&cmd->refcnt));

	if (0 != handle_flag)
		wlan_set_assert(priv, SPRDWL_MODE_NONE, hdr->cmd_id, HANDLE_FLAG_ERROR);

	return plen;
}

/* Events */
void sprdwl_event_station(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_event_new_station *sta =
		(struct sprdwl_event_new_station *)data;

	sprdwl_report_softap(vif, sta->is_connect,
				 sta->mac, sta->ie, sta->ie_len);
}

void sprdwl_event_scan_done(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_event_scan_done *p =
		(struct sprdwl_event_scan_done *)data;
	u8 bucket_id = 0;

	switch (p->type) {
	case SPRDWL_SCAN_DONE:
		sprdwl_scan_done(vif, false);
		wl_ndev_log(L_DBG, vif->ndev, "%s got %d BSSes\n", __func__,
				bss_count);
		break;
	case SPRDWL_SCHED_SCAN_DONE:
		sprdwl_sched_scan_done(vif, false);
		wl_ndev_log(L_DBG, vif->ndev, "%s schedule scan got %d BSSes\n",
				__func__, bss_count);
		break;
	case SPRDWL_GSCAN_DONE:
		bucket_id = ((struct sprdwl_event_gscan_done *)data)->bucket_id;
		sprdwl_gscan_done(vif, bucket_id);
		wl_ndev_log(L_DBG, vif->ndev, "%s gscan got %d bucketid done\n",
				__func__, bucket_id);
		break;
	case SPRDWL_SCAN_ERROR:
	default:
		sprdwl_scan_done(vif, true);
		sprdwl_sched_scan_done(vif, false);
		if (p->type == SPRDWL_SCAN_ERROR)
			wl_ndev_log(L_ERR, vif->ndev, "%s error!\n", __func__);
		else
			wl_ndev_log(L_ERR, vif->ndev, "%s invalid scan done type: %d\n",
				   __func__, p->type);
		break;
	}
	bss_count = 0;
}

void sprdwl_event_connect(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	u8 *pos = data;
	u8 status_code = 0;
	int left = len;
	struct sprdwl_connect_info conn_info;
#ifdef COMPAT_SAMPILE_CODE
	u8 compat_ver = 0;
	struct sprdwl_priv *priv = vif->priv;

	compat_ver = need_compat_operation(priv, WIFI_EVENT_CONNECT);
	if (compat_ver) {
		switch (compat_ver) {
		case VERSION_1:
			/*add data struct modification in here!*/
			break;
		case VERSION_2:
			/*add data struct modification in here!*/
			break;
		case VERSION_3:
			/*add data struct modification in here!*/
			break;
		default:
			break;
		}
	}
#endif

	/*init data struct*/
	memset(&conn_info, 0, sizeof(conn_info));
	/* the first byte is status code */
	memcpy(&conn_info.status, pos, sizeof(conn_info.status));
	if (conn_info.status != SPRDWL_CONNECT_SUCCESS &&
		conn_info.status != SPRDWL_ROAM_SUCCESS){
		/*Assoc response status code by set in the 3 byte if failure*/
		memcpy(&status_code, pos+2, sizeof(status_code));
		goto out;
	}
	pos += sizeof(conn_info.status);
	left -= sizeof(conn_info.status);

	/* parse BSSID */
	if (left < ETH_ALEN)
		goto out;
	conn_info.bssid = pos;
	pos += ETH_ALEN;
	left -= ETH_ALEN;

	/* get channel */
	if (left < sizeof(conn_info.channel))
		goto out;
	memcpy(&conn_info.channel, pos, sizeof(conn_info.channel));
	pos += sizeof(conn_info.channel);
	left -= sizeof(conn_info.channel);

	/* get signal */
	if (left < sizeof(conn_info.signal))
		goto out;
	memcpy(&conn_info.signal, pos, sizeof(conn_info.signal));
	pos += sizeof(conn_info.signal);
	left -= sizeof(conn_info.signal);

	/* parse REQ IE */
	if (left < 0)
		goto out;
	memcpy(&conn_info.req_ie_len, pos, sizeof(conn_info.req_ie_len));
	pos += sizeof(conn_info.req_ie_len);
	left -= sizeof(conn_info.req_ie_len);
	conn_info.req_ie = pos;
	pos += conn_info.req_ie_len;
	left -= conn_info.req_ie_len;

	/* parse RESP IE */
	if (left < 0)
		goto out;
	memcpy(&conn_info.resp_ie_len, pos, sizeof(conn_info.resp_ie_len));
	pos += sizeof(conn_info.resp_ie_len);
	left -= sizeof(conn_info.resp_ie_len);
	conn_info.resp_ie = pos;
	pos += conn_info.resp_ie_len;
	left -= conn_info.resp_ie_len;

	/* parse BEA IE */
	if (left < 0)
		goto out;
	memcpy(&conn_info.bea_ie_len, pos, sizeof(conn_info.bea_ie_len));
	pos += sizeof(conn_info.bea_ie_len);
	left -= sizeof(conn_info.bea_ie_len);
	conn_info.bea_ie = pos;
out:
	sprdwl_report_connection(vif, &conn_info, status_code);
}

void sprdwl_event_disconnect(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	u16 reason_code;

	memcpy(&reason_code, data, sizeof(reason_code));
#ifdef SYNC_DISCONNECT
	/*Report disconnection on version > 4.9.60, even though disconnect
	 is from wpas, otherwise it returns -EALREADY on next connect.*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 60)
	if (atomic_read(&vif->sync_disconnect_event)) {
		vif->disconnect_event_code = reason_code;
		atomic_set(&vif->sync_disconnect_event, 0);
		wake_up(&vif->disconnect_wq);
		wl_err("%s reason code = %d\n", __func__, reason_code);
	} else
#endif
#endif
	sprdwl_report_disconnection(vif, reason_code);
}

void sprdwl_event_mic_failure(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_event_mic_failure *mic_failure =
		(struct sprdwl_event_mic_failure *)data;

	sprdwl_report_mic_failure(vif, mic_failure->is_mcast,
				  mic_failure->key_id);
}

void sprdwl_event_remain_on_channel_expired(struct sprdwl_vif *vif,
						u8 *data, u16 len)
{
	sprdwl_report_remain_on_channel_expired(vif);
}

void sprdwl_event_mlme_tx_status(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_event_mgmt_tx_status *tx_status =
		(struct sprdwl_event_mgmt_tx_status *)data;

	sprdwl_report_mgmt_tx_status(vif, SPRDWL_GET_LE64(tx_status->cookie),
					 tx_status->buf,
					 SPRDWL_GET_LE16(tx_status->len),
					 tx_status->ack);
}

/* @flag: 1 for data, 0 for event */
void sprdwl_event_frame(struct sprdwl_vif *vif, u8 *data, u16 len, int flag)
{
	struct sprdwl_event_mgmt_frame *frame;
	u16 buf_len;
	u8 *buf = NULL;
	u8 channel, type;

	if (flag) {
		/* here frame maybe not 4 bytes align */
		frame = (struct sprdwl_event_mgmt_frame *)
			(data - sizeof(*frame) + len);
		buf = data - sizeof(*frame);
	} else {
		frame = (struct sprdwl_event_mgmt_frame *)data;
		buf = frame->data;
	}
	channel = frame->channel;
	type = frame->type;
	buf_len = SPRDWL_GET_LE16(frame->len);

	sprdwl_cfg80211_dump_frame_prot_info(0, 0, buf, buf_len);

	switch (type) {
	case SPRDWL_FRAME_NORMAL:
		sprdwl_report_rx_mgmt(vif, channel, buf, buf_len);
		break;
	case SPRDWL_FRAME_DEAUTH:
		sprdwl_report_mgmt_deauth(vif, buf, buf_len);
		break;
	case SPRDWL_FRAME_DISASSOC:
		sprdwl_report_mgmt_disassoc(vif, buf, buf_len);
		break;
	case SPRDWL_FRAME_SCAN:
		sprdwl_report_scan_result(vif, channel, frame->signal,
					  buf, buf_len);
		++bss_count;
		break;
	default:
		wl_ndev_log(L_ERR, vif->ndev, "%s invalid frame type: %d!\n",
			   __func__, type);
		break;
	}
}

void sprdwl_event_epno_results(struct sprdwl_vif *vif, u8 *data, u16 data_len)
{
	int i;
	u64 msecs, now;
	struct sk_buff *skb;
	struct nlattr *attr, *sub_attr;
	struct sprdwl_epno_results *epno_results;
	struct sprdwl_priv *priv = vif->priv;
	struct wiphy *wiphy = priv->wiphy;

	wl_hex_dump(L_DBG, "epno result:", DUMP_PREFIX_OFFSET,
				 16, 1, data, data_len, true);

	epno_results = (struct sprdwl_epno_results *)data;
	if (epno_results->nr_scan_results <= 0) {
		wl_err("%s invalid data\n", __func__);
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 83)
	skb = cfg80211_vendor_event_alloc(wiphy, &vif->wdev, data_len,
#else
	skb = cfg80211_vendor_event_alloc(wiphy, data_len,
#endif
					  SPRDWL_VENDOR_EVENT_EPNO_FOUND,
					  GFP_KERNEL);
	if (skb == NULL) {
		wl_ndev_log(L_ERR, vif->ndev, "skb alloc failed");
		return;
	}

	nla_put_u32(skb, GSCAN_RESULTS_REQUEST_ID, epno_results->request_id);
	nla_put_u32(skb, GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,
			epno_results->nr_scan_results);
	nla_put_u8(skb, GSCAN_RESULTS_SCAN_RESULT_MORE_DATA, 0);

	attr = nla_nest_start(skb, GSCAN_RESULTS_LIST);
	if (attr == NULL)
		goto failed;

	now = jiffies;

	if (now > epno_results->boot_time)
		msecs = jiffies_to_msecs(now - epno_results->boot_time);
	else {
		now += (MAX_JIFFY_OFFSET - epno_results->boot_time) + 1;
		msecs = jiffies_to_msecs(now);
	}

	for (i = 0; i < epno_results->nr_scan_results; i++) {
		sub_attr = nla_nest_start(skb, i);
		if (sub_attr == NULL)
			goto failed;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		nla_put_u64_64bit(skb, GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP, msecs, 0);
#else
		nla_put_u64(skb, GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP, msecs);
#endif
		nla_put(skb, GSCAN_RESULTS_SCAN_RESULT_BSSID, ETH_ALEN,
			epno_results->results[i].bssid);
		nla_put_u32(skb, GSCAN_RESULTS_SCAN_RESULT_CHANNEL,
				epno_results->results[i].channel);
		nla_put_s32(skb, GSCAN_RESULTS_SCAN_RESULT_RSSI,
				epno_results->results[i].rssi);
		nla_put_u32(skb, GSCAN_RESULTS_SCAN_RESULT_RTT,
				epno_results->results[i].rtt);
		nla_put_u32(skb, GSCAN_RESULTS_SCAN_RESULT_RTT_SD,
				epno_results->results[i].rtt_sd);
		nla_put_u16(skb, GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD,
				epno_results->results[i].beacon_period);
		nla_put_u16(skb, GSCAN_RESULTS_SCAN_RESULT_CAPABILITY,
				epno_results->results[i].capability);
		nla_put_string(skb, GSCAN_RESULTS_SCAN_RESULT_SSID,
				   epno_results->results[i].ssid);

		nla_nest_end(skb, sub_attr);
	}

	nla_nest_end(skb, attr);

	cfg80211_vendor_event(skb, GFP_KERNEL);
	wl_debug("report epno event success, count = %d\n",
		 epno_results->nr_scan_results);
	return;

failed:
	kfree(skb);
	wl_err("%s report epno event failed\n", __func__);
}

void sprdwl_event_gscan_frame(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	u32 report_event;
	u8 *pos = data;
	s32 avail_len = len;
	struct sprdwl_gscan_result *frame;
	u16 buf_len;
	u8 bucket_id = 0;

	report_event = *(u32 *)pos;
	avail_len -= sizeof(u32);
	pos += sizeof(u32);

	if (report_event & REPORT_EVENTS_EPNO)
		return sprdwl_event_epno_results(vif, pos, avail_len);

/*significant change result is different with gsan with, deal it specially*/
	if (report_event & REPORT_EVENTS_SIGNIFICANT_CHANGE) {
		sprdwl_vendor_cache_significant_change_result(vif,
				pos, avail_len);
		sprdwl_significant_change_event(vif);
		return;
	}

	while (avail_len > 0) {
		if (avail_len < sizeof(struct sprdwl_gscan_result)) {
			wl_ndev_log(L_ERR, vif->ndev,
				   "%s invalid available length: %d!\n",
				   __func__, avail_len);
			break;
		}

		bucket_id = *(u8 *)pos;
		pos += sizeof(u8);
		frame = (struct sprdwl_gscan_result *)pos;
		frame->ts = jiffies;
		buf_len = frame->ie_length;

		if ((report_event == REPORT_EVENTS_BUFFER_FULL) ||
			(report_event & REPORT_EVENTS_EACH_SCAN))
			sprdwl_vendor_cache_scan_result(vif, bucket_id, frame);
		else if (report_event & REPORT_EVENTS_FULL_RESULTS)
			sprdwl_vendor_report_full_scan(vif, frame);
		else if (report_event & REPORT_EVENTS_HOTLIST_RESULTS_FOUND ||
			report_event & REPORT_EVENTS_HOTLIST_RESULTS_LOST) {
			sprdwl_vendor_cache_hotlist_result(vif, frame);
		}

		avail_len -= sizeof(struct sprdwl_gscan_result) + buf_len + 1;
		pos += sizeof(struct sprdwl_gscan_result) + buf_len;

		wl_ndev_log(L_DBG, vif->ndev, "%s ch:%d id:%d len:%d aval:%d, report_event:%d\n",
				__func__, frame->channel, bucket_id,
				buf_len, avail_len, report_event);
	}

	if (report_event == REPORT_EVENTS_BUFFER_FULL)
		sprdwl_buffer_full_event(vif);
	else if (report_event & REPORT_EVENTS_EACH_SCAN)
		sprdwl_gscan_done(vif, bucket_id);
	else if (report_event & REPORT_EVENTS_HOTLIST_RESULTS_FOUND ||
		report_event & REPORT_EVENTS_HOTLIST_RESULTS_LOST) {
		sprdwl_hotlist_change_event(vif, report_event);
	}

}

void sprdwl_event_cqm(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_event_cqm *p;
	u8 rssi_event;

	p = (struct sprdwl_event_cqm *)data;
	switch (p->status) {
	case SPRDWL_CQM_RSSI_LOW:
		rssi_event = NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW;
		break;
	case SPRDWL_CQM_RSSI_HIGH:
		rssi_event = NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH;
		break;
	case SPRDWL_CQM_BEACON_LOSS:
		/* TODO wpa_supplicant not support the event ,
		 * so we workaround this issue
		 */
		rssi_event = NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW;
		vif->beacon_loss = 1;
		break;
	default:
		wl_ndev_log(L_ERR, vif->ndev, "%s invalid event!\n", __func__);
		return;
	}

	sprdwl_report_cqm(vif, rssi_event);
}

void sprdwl_event_tdls(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	unsigned char peer[ETH_ALEN];
	u8 oper;
	u16 reason_code;
	struct sprdwl_event_tdls *report_tdls = NULL;

	report_tdls = (struct sprdwl_event_tdls *)data;
	ether_addr_copy(&peer[0], &report_tdls->mac[0]);
	oper = report_tdls->tdls_sub_cmd_mgmt;

	if (SPRDWL_TDLS_TEARDOWN == oper)
		oper = NL80211_TDLS_TEARDOWN;
	else if (SPRDWL_TDLS_UPDATE_PEER_INFOR == oper)
		sprdwl_event_tdls_flow_count(vif, data, len);
	else {
		oper = NL80211_TDLS_SETUP;
		sprdwl_tdls_flow_flush(vif, peer, oper);
	}

	reason_code = 0;
	sprdwl_report_tdls(vif, peer, oper, reason_code);
}

int sprdwl_send_tdlsdata_use_cmd(struct sk_buff *skb,
				  struct sprdwl_vif *vif, u8 need_cp2_rsp)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_tdls *p;
	struct sprdwl_intf *intf;

	intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	msg = sprdwl_cmd_getbuf(vif->priv, sizeof(*p) + skb->len, vif->ctx_id,
		SPRDWL_HEAD_RSP, WIFI_CMD_TDLS);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_tdls *)msg->data;
	p->tdls_sub_cmd_mgmt = WLAN_TDLS_CMD_TX_DATA;
	ether_addr_copy(p->da, skb->data);
	p->paylen = skb->len;/*TBD*/
	memcpy(p->payload, skb->data, skb->len);

	if (need_cp2_rsp)
		return sprdwl_cmd_send_recv(vif->priv, msg,
			CMD_WAIT_TIMEOUT, NULL, NULL);
	else
		return sprdwl_cmd_send_recv_no_wait(vif->priv, msg);
}

inline void sprdwl_event_ba_mgmt(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	wlan_ba_session_event(vif->priv->hw_priv, data, len);
}

void sprdwl_event_suspend_resume(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_event_suspend_resume *suspend_resume = NULL;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	suspend_resume = (struct sprdwl_event_suspend_resume *)data;
	if ((1 == suspend_resume->status) &&
		(intf->suspend_mode == SPRDWL_PS_RESUMING)) {
		intf->suspend_mode = SPRDWL_PS_RESUMED;
		tx_up(tx_msg);
		wl_info("%s, %d,resumed,wakeuptx\n", __func__, __LINE__);
	}
}

void sprdwl_event_hang_recovery(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct event_hang_recovery *hang =
		(struct event_hang_recovery *)data;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;

	tx_msg->hang_recovery_status = hang->action;
	wl_info("%s, %d, action=%d, status=%d\n",
		__func__, __LINE__,
		hang->action,
		tx_msg->hang_recovery_status);
	if (hang->action == HANG_RECOVERY_BEGIN)
		sprdwl_add_hang_cmd(vif);
	if (hang->action == HANG_RECOVERY_END)
		tx_up(tx_msg);
}

void sprdwl_event_thermal_warn(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct event_thermal_warn *thermal =
		(struct event_thermal_warn *)data;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(vif->priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	enum sprdwl_mode mode;

	wl_info("%s, %d, action=%d, status=%d\n",
		__func__, __LINE__,
		thermal->action,
		tx_msg->thermal_status);
	if (tx_msg->thermal_status == THERMAL_WIFI_DOWN)
		return;
	tx_msg->thermal_status = thermal->action;
	switch (thermal->action) {
	case THERMAL_TX_RESUME:
		sprdwl_net_flowcontrl(intf->priv, SPRDWL_MODE_NONE, true);
		tx_up(tx_msg);
		break;
	case THERMAL_TX_STOP:
		wl_err("%s, %d, netif_stop_queue because of thermal warn\n",
			   __func__, __LINE__);
		sprdwl_net_flowcontrl(intf->priv, SPRDWL_MODE_NONE, false);
		break;
	case THERMAL_WIFI_DOWN:
		wl_err("%s, %d, close wifi because of thermal warn\n",
			   __func__, __LINE__);
		sprdwl_net_flowcontrl(intf->priv, SPRDWL_MODE_NONE, false);
		for (mode = SPRDWL_MODE_STATION; mode < SPRDWL_MODE_MAX; mode++) {
			if (intf->priv->fw_stat[mode] == SPRDWL_INTF_OPEN)
				sprdwl_add_close_cmd(vif, mode);
		}
		break;
	default:
		break;
	}
}

#define WIFI_EVENT_WFD_RATE 0x30
extern int wfd_notifier_call_chain(unsigned long val, void *v);

void sprdwl_wfd_mib_cnt(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct event_wfd_mib_cnt *wfd =
		(struct event_wfd_mib_cnt *)data;
	u32 tx_cnt, busy_cnt, wfd_rate = 0;

	wl_info("%s, %d, tp=%d, sum_tp=%d, drop=%d,%d,%d,%d, frame=%d, clear=%d, mib=%d\n",
		__func__, __LINE__,
		wfd->wfd_throughput, wfd->sum_tx_throughput,
		wfd->tx_mpdu_lost_cnt[0], wfd->tx_mpdu_lost_cnt[1], wfd->tx_mpdu_lost_cnt[2], wfd->tx_mpdu_lost_cnt[3],
		wfd->tx_frame_cnt, wfd->rx_clear_cnt, wfd->mib_cycle_cnt);

	if (!wfd->mib_cycle_cnt)
		return;

	tx_cnt = wfd->tx_frame_cnt / wfd->mib_cycle_cnt;
	busy_cnt = (10 * wfd->rx_clear_cnt) / wfd->mib_cycle_cnt;

	if (busy_cnt > 8)
		wfd_rate = wfd->sum_tx_throughput;
	else{
		if (tx_cnt)
			wfd_rate = wfd->sum_tx_throughput + wfd->sum_tx_throughput * (1 / tx_cnt) * ((10 - busy_cnt) / 10) / 2;
	}
	wl_info("%s, %d, wfd_rate=%d\n", __func__, __LINE__, wfd_rate);
	wfd_rate = 2;
	/* wfd_notifier_call_chain(WIFI_EVENT_WFD_RATE, (void *)(unsigned long)wfd_rate); */
}

int sprdwl_fw_power_down_ack(struct sprdwl_priv *priv, u8 ctx_id)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_power_save *p;
	int ret = 0;
	struct sprdwl_intf *intf = (struct sprdwl_intf *)(priv->hw_priv);
	struct sprdwl_tx_msg *tx_msg = (struct sprdwl_tx_msg *)intf->sprdwl_tx;
	enum sprdwl_mode mode = SPRDWL_MODE_NONE;
	int tx_num = 0;

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p), ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_POWER_SAVE);
	if (!msg)
		return -ENOMEM;

	p = (struct sprdwl_cmd_power_save *)msg->data;
	p->sub_type = SPRDWL_FW_PWR_DOWN_ACK;

	for (mode = SPRDWL_MODE_NONE; mode < SPRDWL_MODE_MAX; mode++) {
		int num = atomic_read(&tx_msg->tx_list[mode]->mode_list_num);

		tx_num += num;
	}

	if (tx_num > 0 ||
		!list_empty(&tx_msg->xmit_msg_list.to_send_list) ||
		!list_empty(&tx_msg->xmit_msg_list.to_free_list)) {
		if (intf->fw_power_down == 1)
			goto err;
		p->value = 0;
		intf->fw_power_down = 0;
		intf->fw_awake = 1;
	} else {
		p->value = 1;
		intf->fw_power_down = 1;
		intf->fw_awake = 0;
	}
	wl_info("%s, value=%d, fw_pwr_down=%d, fw_awake=%d, %d, %d, %d, %d\n",
		__func__,
		p->value,
		intf->fw_power_down,
		intf->fw_awake,
		atomic_read(&tx_msg->tx_list_qos_pool.ref),
		tx_num,
		list_empty(&tx_msg->xmit_msg_list.to_send_list),
		list_empty(&tx_msg->xmit_msg_list.to_free_list));

	ret =  sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);

	if (intf->fw_power_down == 1) {
		sprdwcn_bus_allow_sleep(WIFI);
		sprdwl_unboost();
	}

	if (ret)
		wl_err("host send data cmd failed, ret=%d\n", ret);

	return ret;
err:
	wl_err("%s donot ack FW_PWR_DOWN twice\n", __func__);
	sprdwl_intf_free_msg_buf(priv, msg);
	return -1;
}

void sprdwl_event_fw_power_down(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_work *misc_work;

	misc_work = sprdwl_alloc_work(0);
	if (!misc_work) {
		wl_err("%s out of memory\n", __func__);
		return;
	}
	misc_work->vif = vif;
	misc_work->id = SPRDWL_WORK_FW_PWR_DOWN;

	sprdwl_queue_work(vif->priv, misc_work);
}

void sprdwl_event_chan_changed(struct sprdwl_vif *vif, u8 *data, u16 len)
{
	struct sprdwl_chan_changed_info *p = (struct sprdwl_chan_changed_info *)data;
	u8 channel;
	u16 freq;
	struct wiphy *wiphy = vif->wdev.wiphy;
	struct ieee80211_channel *ch = NULL;
	struct cfg80211_chan_def chandef;

	if (p->initiator == 0) {
		wl_err("%s, unknowed event!\n", __func__);
	} else if (p->initiator == 1) {
		channel = p->target_channel;
		freq = 2412 + (channel-1) * 5;
		if (wiphy)
			ch = ieee80211_get_channel(wiphy, freq);
		else
			wl_err("%s, wiphy is null!\n", __func__);
		if (ch)
			/* we will be active on the channel */
			cfg80211_chandef_create(&chandef, ch,
						NL80211_CHAN_HT20);
		else
			wl_err("%s, ch is null!\n", __func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,9, 0))
		cfg80211_ch_switch_notify(vif->ndev, &chandef, 0);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,19, 2))
		cfg80211_ch_switch_notify(vif->ndev, &chandef, 0, 0);
#else
		cfg80211_ch_switch_notify(vif->ndev, &chandef);
#endif
	}
}

void sprdwl_event_coex_bt_on_off(u8 *data, u16 len)
{
	struct event_coex_mode_changed *coex_bt_on_off =
		(struct event_coex_mode_changed *)data;

	wl_info("%s, %d, action=%d\n",
		__func__, __LINE__,
		coex_bt_on_off->action);
	set_coex_bt_on_off(coex_bt_on_off->action);
}

#define E2S(x) \
{ \
	case x: \
		str = #x; \
		break; \
}

static const char *evt2str(u8 evt)
{
	const char *str = NULL;

	switch (evt) {
	E2S(WIFI_EVENT_CONNECT)
	E2S(WIFI_EVENT_DISCONNECT)
	E2S(WIFI_EVENT_SCAN_DONE)
	E2S(WIFI_EVENT_MGMT_FRAME)
	E2S(WIFI_EVENT_MGMT_TX_STATUS)
	E2S(WIFI_EVENT_REMAIN_CHAN_EXPIRED)
	E2S(WIFI_EVENT_MIC_FAIL)
	E2S(WIFI_EVENT_NEW_STATION)
	E2S(WIFI_EVENT_CQM)
	E2S(WIFI_EVENT_MEASUREMENT)
	E2S(WIFI_EVENT_TDLS)
	E2S(WIFI_EVENT_SDIO_SEQ_NUM)
	E2S(WIFI_EVENT_SDIO_FLOWCON)
	E2S(WIFI_EVENT_BA)
	E2S(WIFI_EVENT_RSSI_MONITOR)
	E2S(WIFI_EVENT_GSCAN_FRAME)
#ifdef DFS_MASTER
	E2S(WIFI_EVENT_RADAR_DETECTED)
#endif
	E2S(WIFI_EVENT_STA_LUT_INDEX)
	E2S(WIFI_EVENT_SUSPEND_RESUME)
	E2S(WIFI_EVENT_NAN)
	E2S(WIFI_EVENT_RTT)
	E2S(WIFI_EVENT_HANG_RECOVERY)
	E2S(WIFI_EVENT_THERMAL_WARN)
	E2S(WIFI_EVENT_WFD_MIB_CNT)
	E2S(WIFI_EVENT_FW_PWR_DOWN)
	default :
		return "WIFI_EVENT_UNKNOWN";
	}

	return str;
}

#undef E2S

/* retrun the msg length or 0 */
unsigned short sprdwl_rx_event_process(struct sprdwl_priv *priv, u8 *msg)
{
	struct sprdwl_cmd_hdr *hdr = (struct sprdwl_cmd_hdr *)msg;
	struct sprdwl_vif *vif;
	u8 ctx_id;
	u16 len, plen;
	u8 *data;

	ctx_id = hdr->common.ctx_id;
	/*TODO ctx_id range*/
#ifndef CONFIG_P2P_INTF
	if (ctx_id > STAP_MODE_P2P_DEVICE) {
#else
	if (ctx_id >= STAP_MODE_COEXI_NUM) {
#endif
		wl_info("%s invalid ctx_id: %d\n", __func__, ctx_id);
		return 0;
	}

	plen = SPRDWL_GET_LE16(hdr->plen);
	if (!priv) {
		wl_err("%s priv is NULL [%u]ctx_id %d recv[%s]len: %d\n",
			   __func__, le32_to_cpu(hdr->mstime), ctx_id,
			   evt2str(hdr->cmd_id), hdr->plen);
		return plen;
	}

	wl_debug("[%u]ctx_id %d recv[%s]len: %d\n",
		le32_to_cpu(hdr->mstime), ctx_id,
		evt2str(hdr->cmd_id), plen);

	wl_hex_dump(L_DBG, "EVENT: ", DUMP_PREFIX_OFFSET, 16, 1,
				 (u8 *)hdr, hdr->plen, 0);

	len = plen - sizeof(*hdr);
	vif = ctx_id_to_vif(priv, ctx_id);
	if (!vif) {
		wl_info("%s NULL vif for ctx_id: %d, len:%d\n",
			__func__, ctx_id, plen);
		return plen;
	}

	if (!((long)msg & 0x3)) {
		data = (u8 *)msg;
		data += sizeof(*hdr);
	} else {
		/* never into here when the dev is BA or MARLIN2,
		 * temply used as debug and safe
		 */
		WARN_ON(1);
		data = kmalloc(len, GFP_KERNEL);
		if (!data) {
			sprdwl_put_vif(vif);
			return plen;
		}
		memcpy(data, msg + sizeof(*hdr), len);
	}

	switch (hdr->cmd_id) {
	case WIFI_EVENT_CONNECT:
		sprdwl_event_connect(vif, data, len);
		break;
	case WIFI_EVENT_DISCONNECT:
		sprdwl_event_disconnect(vif, data, len);
		break;
	case WIFI_EVENT_REMAIN_CHAN_EXPIRED:
		sprdwl_event_remain_on_channel_expired(vif, data, len);
		break;
	case WIFI_EVENT_NEW_STATION:
		sprdwl_event_station(vif, data, len);
		break;
	case WIFI_EVENT_MGMT_FRAME:
		/* for old Marlin2 CP code or BA*/
		sprdwl_event_frame(vif, data, len, 0);
		break;
	case WIFI_EVENT_GSCAN_FRAME:
		sprdwl_event_gscan_frame(vif, data, len);
		break;
	case WIFI_EVENT_RSSI_MONITOR:
		sprdwl_event_rssi_monitor(vif, data, len);
		break;
	case WIFI_EVENT_SCAN_DONE:
		sprdwl_event_scan_done(vif, data, len);
		break;
	case WIFI_EVENT_SDIO_SEQ_NUM:
		break;
	case WIFI_EVENT_MIC_FAIL:
		sprdwl_event_mic_failure(vif, data, len);
		break;
	case WIFI_EVENT_CQM:
		sprdwl_event_cqm(vif, data, len);
		break;
	case WIFI_EVENT_MGMT_TX_STATUS:
		sprdwl_event_mlme_tx_status(vif, data, len);
		break;
	case WIFI_EVENT_TDLS:
		sprdwl_event_tdls(vif, data, len);
		break;
	case WIFI_EVENT_SUSPEND_RESUME:
		sprdwl_event_suspend_resume(vif, data, len);
		break;
#ifdef NAN_SUPPORT
	case WIFI_EVENT_NAN:
		sprdwl_event_nan(vif, data, len);
		break;
#endif /* NAN_SUPPORT */
	case WIFI_EVENT_STA_LUT_INDEX:
		sprdwl_event_sta_lut(vif, data, len);
		break;
	case WIFI_EVENT_BA:
		sprdwl_event_ba_mgmt(vif, data, len);
		break;
#ifdef DFS_MASTER
	case WIFI_EVENT_RADAR_DETECTED:
		sprdwl_11h_handle_radar_detected(vif, data, len);
		break;
#endif
#ifdef RTT_SUPPORT
	case WIFI_EVENT_RTT:
		sprdwl_event_ftm(vif, data, len);
		break;
#endif /* RTT_SUPPORT */
	case WIFI_EVENT_HANG_RECOVERY:
		sprdwl_event_hang_recovery(vif, data, len);
		break;
	case WIFI_EVENT_THERMAL_WARN:
		sprdwl_event_thermal_warn(vif, data, len);
		break;
	case WIFI_EVENT_WFD_MIB_CNT:
		sprdwl_wfd_mib_cnt(vif, data, len);
		break;
	case WIFI_EVENT_FW_PWR_DOWN:
		sprdwl_event_fw_power_down(vif, data, len);
		break;
	case WIFI_EVENT_SDIO_FLOWCON:
		break;
	case WIFI_EVENT_CHAN_CHANGED:
		sprdwl_event_chan_changed(vif, data, len);
		break;
	case WIFI_EVENT_COEX_BT_ON_OFF:
		sprdwl_event_coex_bt_on_off(data, len);
		break;
	default:
		wl_info("unsupported event: %d\n", hdr->cmd_id);
		break;
	}

	sprdwl_put_vif(vif);

	if ((long)msg & 0x3)
		kfree(data);

	return plen;
}

int sprdwl_set_tlv_data(struct sprdwl_priv *priv, u8 ctx_id,
			struct sprdwl_tlv_data *tlv, int length)
{
	struct sprdwl_msg_buf *msg;

	if (priv == NULL || tlv == NULL)
		return -EINVAL;

	msg = sprdwl_cmd_getbuf(priv, length, ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_SET_TLV);
	if (!msg)
		return -ENOMEM;

	memcpy(msg->data, tlv, length);

	wl_info("%s tlv type = %d\n", __func__, tlv->type);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

void sprdwl_set_tlv_elmt(u8 *addr, u16 type, u16 len, u8 *data)
{
	struct sprdwl_tlv_data *p = (struct sprdwl_tlv_data *)addr;

	p->type = type;
	p->len = len;
	memcpy(p->data, data, len);
}

int sprdwl_set_wowlan(struct sprdwl_priv *priv, int subcmd, void *pad, int pad_len)
{
	struct sprdwl_msg_buf *msg;
	struct wowlan_cmd {
		u8 sub_cmd_id;
		u8 pad_len;
		char pad[0];
	} *cmd;

	if (priv == NULL)
		return -EINVAL;

	msg = sprdwl_cmd_getbuf(priv, pad_len + sizeof(struct wowlan_cmd), SPRDWL_MODE_NONE,
			SPRDWL_HEAD_RSP, WIFI_CMD_SET_WOWLAN);
	if (!msg)
		return -ENOMEM;

	cmd = (struct wowlan_cmd *)msg->data;
	cmd->sub_cmd_id = subcmd;
	cmd->pad_len = pad_len;

	wl_debug("%s subcmd = %d, len = %d\n", __func__, cmd->sub_cmd_id, cmd->pad_len);
	if (pad_len)
		memcpy(cmd->pad, pad, pad_len);

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, NULL, NULL);
}

#ifdef SYNC_DISCONNECT
int sprdwl_sync_disconnect_event(struct sprdwl_vif *vif, unsigned int timeout)
{
	int ret;

	sprdwl_cmd_lock(&g_sprdwl_cmd);
	vif->disconnect_event_code = 0;
	ret = wait_event_timeout(vif->disconnect_wq,
				 atomic_read(&vif->sync_disconnect_event) == 0, timeout);
	sprdwl_cmd_unlock(&g_sprdwl_cmd);

	return ret;
}
#endif

int sprdwl_set_packet_offload(struct sprdwl_priv *priv, u8 vif_ctx_id,
			      u32 req, u8 enable, u32 interval,
			      u32 len, u8 *data)
{
	struct sprdwl_msg_buf *msg;
	struct sprdwl_cmd_packet_offload *p;
	struct sprdwl_cmd_packet_offload *packet = NULL;
	u16 r_len = sizeof(*packet);
	u8 r_buf[sizeof(*packet)];

	msg = sprdwl_cmd_getbuf(priv, sizeof(*p) + len, vif_ctx_id,
				SPRDWL_HEAD_RSP, WIFI_CMD_PACKET_OFFLOAD);
	if (!msg)
		return -ENOMEM;
	p = (struct sprdwl_cmd_packet_offload *)msg->data;

	p->enable = enable;
	p->req_id = req;
	if (enable) {
		p->period = interval;
		p->len = len;
		memcpy(p->data, data, len);
	}

	return sprdwl_cmd_send_recv(priv, msg, CMD_WAIT_TIMEOUT, r_buf, &r_len);
}
