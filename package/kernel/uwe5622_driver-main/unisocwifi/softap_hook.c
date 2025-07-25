#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>

#include "sprdwl.h"
#include "wl_core.h"
#define SOFTAP_INI_PATH "/data/misc/wifi/softap.ini"

static bool is_valid_channel(struct wiphy *wiphy, int chn)
{
	int i;
	struct ieee80211_supported_band *bands;

	if (chn < 15) {
		if (chn < 1)
			return false;
		return true;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	bands = wiphy->bands[NL80211_BAND_5GHZ];
#else
	bands = wiphy->bands[IEEE80211_BAND_5GHZ];
#endif
	for (i = 0; i < bands->n_channels; i++)
		if (chn == bands->channels[i].hw_value)
			return true;

	return false;
}

static int sprdwl_get_softap_chan(u8 *path)
{
	int ret;
	int chn;
	struct file *fp = NULL;
#ifdef setfs
	mm_segment_t fs;
#endif
	char buf[64] = {0};

	if (path == NULL)
		return -EINVAL;

	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		wl_err("Open file: %s failed(%ld)\n", path, PTR_ERR(fp));
		return -EINVAL;
	}

#ifdef setfs
	fs = get_fs();
	set_fs(get_ds());
#endif

	ret = kernel_read(fp, buf, sizeof(buf), &fp->f_pos);

	filp_close(fp, NULL);
#ifdef setfs
	set_fs(fs);
#endif

	if (ret <= 0) {
		wl_err("read file failed, ret = %d\n", ret);
		return -EINVAL;
	}

	ret = sscanf(buf, "channel=%d\n", &chn);
	if (ret != 1) {
		wl_err("Keywords channel not found in %s\n", path);
		return -EINVAL;
	}

	return chn;
}

void sprdwl_hook_reset_channel(struct wiphy *wiphy,
				   struct cfg80211_ap_settings *settings)
{
	u8 *ie, *ds_param_ch;
	int channel = 1;
	struct ieee80211_mgmt *mgmt;
	struct ieee80211_ht_operation *oper;

	/* Read channel from file /data/misc/wifi/softap.ini */
	channel = sprdwl_get_softap_chan(SOFTAP_INI_PATH);
	if (channel < 0)
		return;

	if (!is_valid_channel(wiphy, channel)) {
		wl_err("%s channel(%d) invalid\n", __func__, channel);
		return;
	}

	mgmt = (struct ieee80211_mgmt *)settings->beacon.head;
	ie = (u8 *)cfg80211_find_ie(WLAN_EID_DS_PARAMS,
					&mgmt->u.beacon.variable[0],
					settings->beacon.head_len);
	if (ie == NULL) {
		wl_err("IE WLAN_EID_DS_PARAMS not found in beacon\n");
		return;
	}

	ds_param_ch = ie + 2;

	ie = (u8 *)cfg80211_find_ie(WLAN_EID_HT_OPERATION,
					settings->beacon.tail,
					settings->beacon.tail_len);
	if (ie == NULL) {
		wl_err("IE WLAN_EID_HT_OPERATION not found in beacon\n");
		return;
	}

	ie += 2;
	oper = (struct ieee80211_ht_operation *)ie;

	wl_info("%s done, reset channel %d -> %d\n", __func__,
		   oper->primary_chan, channel);

	*ds_param_ch = oper->primary_chan = channel;
}
