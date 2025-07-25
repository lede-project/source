#include "reg_domain.h"
#include "sprdwl.h"
#include "wl_intf.h"

static const struct ieee80211_regdomain world_regdom = {
	.n_reg_rules = 7,
	.alpha2 =  "00",
	.reg_rules = {
		/* IEEE 802.11b/g, channels 1..11 */
		REG_RULE(2412-10, 2462+10, 40, 6, 20, 0),
		/* IEEE 802.11b/g, channels 12..13. */
		REG_RULE(2467-10, 2472+10, 40, 6, 20, NL80211_RRF_AUTO_BW),
		/* IEEE 802.11 channel 14 - Only JP enables
		 * this and for 802.11b only */
		REG_RULE(2484-10, 2484+10, 20, 6, 20,
			NL80211_RRF_NO_OFDM),
		/* IEEE 802.11a, channel 36..48 */
		REG_RULE(5180-10, 5240+10, 160, 6, 20,
			NL80211_RRF_AUTO_BW),

		/* IEEE 802.11a, channel 52..64 - DFS required */
		REG_RULE(5260-10, 5320+10, 160, 6, 20,
			NL80211_RRF_DFS |
			NL80211_RRF_AUTO_BW),

		/* IEEE 802.11a, channel 100..144 - DFS required */
		REG_RULE(5500-10, 5720+10, 160, 6, 20,
			NL80211_RRF_DFS),

		/* IEEE 802.11a, channel 149..165 */
		REG_RULE(5745-10, 5825+10, 80, 6, 20, 0),
	}
};

const struct ieee80211_regdomain regdom_cn = {
	.n_reg_rules = 4,
	.alpha2 = "CN",
	.reg_rules = {
	/* IEEE 802.11b/g, channels 1..13 */
	REG_RULE(2412-10, 2472+10, 40, 6, 20, 0),
	/* IEEE 802.11a, channel 36..48 */
	REG_RULE(5180-10, 5240+10, 160, 6, 20,
			NL80211_RRF_AUTO_BW),
	/* IEEE 802.11a, channel 52..64 - DFS required */
	REG_RULE(5260-10, 5320+10, 160, 6, 20,
			NL80211_RRF_DFS |
			NL80211_RRF_AUTO_BW),
	/* channels 149..165 */
	REG_RULE(5745-10, 5825+10, 80, 6, 20, 0),
	}
};

const struct ieee80211_regdomain regdom_us01 = {
	.n_reg_rules = 6,
	.reg_rules = {
	/* channels 1..11 */
	SPRD_REG_RULE(2412-10, 2462+10, 40, 0),
	/* channels 36..48 */
	SPRD_REG_RULE(5180-10, 5240+10, 40, 0),
	/* channels 56..64 */
	SPRD_REG_RULE(5260-10, 5320+10, 40, NL80211_RRF_DFS),
	/* channels 100..118 */
	SPRD_REG_RULE(5500-10, 5590+10, 40, NL80211_RRF_DFS),
	/* channels 132..140 */
	SPRD_REG_RULE(5660-10, 5700+10, 40, NL80211_RRF_DFS),
	/* channels 149..165 */
	SPRD_REG_RULE(5745-10, 5825+10, 40, 0) }
};

const struct ieee80211_regdomain regdom_us = {
	.n_reg_rules = 5,
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
	/* channels 1..11 */
	SPRD_REG_RULE(2412-10, 2462+10, 40, 0),
	/* channels 36..48 */
	SPRD_REG_RULE(5180-10, 5240+10, 80, NL80211_RRF_AUTO_BW),
	/* channels 52..64 */
	SPRD_REG_RULE(5260-10, 5320+10, 80, NL80211_RRF_DFS | NL80211_RRF_AUTO_BW),
	/* channels 100..140 */
	SPRD_REG_RULE(5500-10, 5720+10, 160, NL80211_RRF_DFS),
	/* channels 149..165 */
	SPRD_REG_RULE(5745-10, 5825+10, 80, 0) }
};



const struct ieee80211_regdomain regdom_cz_nl = {
	.n_reg_rules = 5,
	.reg_rules = {
	/* channels 1..11 */
	SPRD_REG_RULE(2412-10, 2462+10, 40, 0),
	/* channels 12,13 */
	SPRD_REG_RULE(2467-10, 2472+10, 40, 0),
	/* channels 36..48 */
	SPRD_REG_RULE(5180-10, 5240+10, 80, 0),
	/* channels 52..64 */
	SPRD_REG_RULE(5260-10, 5320+10, 80, NL80211_RRF_DFS),
	/* channels 100..140 */
	SPRD_REG_RULE(5500-10, 5700+10, 160, NL80211_RRF_DFS) }
};

const struct ieee80211_regdomain regdom_jp = {
	.n_reg_rules = 7,
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
	/* channels 1..13 */
	SPRD_REG_RULE(2412-10, 2472+10, 40, 0),
	/* channels 14 */
	SPRD_REG_RULE(2484-10, 2484+10, 20, NL80211_RRF_NO_OFDM),
	/* channels 184..196 */
	SPRD_REG_RULE(4920-10, 4980+10, 40, 0),
	/* channels 8..16 */
	SPRD_REG_RULE(5040-10, 5080+10, 40, 0),
	/* channels 36..48 */
	SPRD_REG_RULE(5180-10, 5240+10, 80, NL80211_RRF_AUTO_BW),
	/* channels 52..64 */
	SPRD_REG_RULE(5260-10, 5320+10, 80, NL80211_RRF_DFS | NL80211_RRF_AUTO_BW),
	/* channels 100..140 */
	SPRD_REG_RULE(5500-10, 5700+10, 160, NL80211_RRF_DFS) }
};

const struct ieee80211_regdomain regdom_tr = {
	.n_reg_rules = 4,
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
	/* channels 1..13 */
	SPRD_REG_RULE(2412-10, 2472+10, 40, 0),
	/* channels 36..48 */
	SPRD_REG_RULE(5180-10, 5240+10, 80, NL80211_RRF_AUTO_BW),
	/* channels 52..64 */
	SPRD_REG_RULE(5260-10, 5320+10, 80, NL80211_RRF_DFS | NL80211_RRF_AUTO_BW),
	/* channels 100..140 */
	SPRD_REG_RULE(5500-10, 5700+10, 160, NL80211_RRF_DFS) }
};

const struct sprd_regdomain sprd_regdom_00 = {
	.country_code = "00",
	.prRegdRules = &world_regdom
};

const struct sprd_regdomain sprd_regdom_us01 = {
	.country_code = "US01",
	.prRegdRules = &regdom_us01
};

const struct sprd_regdomain sprd_regdom_us = {
	.country_code = "US",
	.prRegdRules = &regdom_us
};

const struct sprd_regdomain sprd_regdom_cn = {
	.country_code = "CN",
	.prRegdRules = &regdom_cn
};

const struct sprd_regdomain sprd_regdom_nl = {
	.country_code = "NL",
	.prRegdRules = &regdom_cz_nl
};

const struct sprd_regdomain sprd_regdom_cz = {
	.country_code = "CZ",
	.prRegdRules = &regdom_cz_nl
};

const struct sprd_regdomain sprd_regdom_jp = {
	.country_code = "JP",
	.prRegdRules = &regdom_jp
};

const struct sprd_regdomain sprd_regdom_tr = {
	.country_code = "TR",
	.prRegdRules = &regdom_tr
};

const struct sprd_regdomain *g_prRegRuleTable[] = {
	&sprd_regdom_00,
	/*&sprd_regdom_us01,*/
	&sprd_regdom_us,
	&sprd_regdom_cn,
	&sprd_regdom_nl,
	&sprd_regdom_cz,
	&sprd_regdom_jp,
	&sprd_regdom_tr,
	NULL /* this NULL SHOULD be at the end of the array */
};

const struct ieee80211_regdomain *getRegdomainFromSprdDB(char *alpha2)
{
	u8 idx;
	const struct sprd_regdomain *prRegd;

	idx = 0;
	while (g_prRegRuleTable[idx]) {
		prRegd = g_prRegRuleTable[idx];

		if ((prRegd->country_code[0] == alpha2[0]) &&
			(prRegd->country_code[1] == alpha2[1])/* &&
			(prRegd->country_code[2] == alpha2[2]) &&
			(prRegd->country_code[3] == alpha2[3])*/)
			return prRegd->prRegdRules;

		idx++;
	}

	wl_info("%s(): Error, wrong country = %s\n",
			__func__, alpha2);
	wl_info("Set as default 00\n");

	return &world_regdom; /*default world wide*/
}

void
apply_custom_regulatory(struct wiphy *pWiphy,
							const struct ieee80211_regdomain *pRegdom)
{
	u32 band_idx, ch_idx;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;

	for (band_idx = 0; band_idx < 2; band_idx++) {
		sband = pWiphy->bands[band_idx];
		if (!sband)
			continue;

		for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
			chan = &sband->channels[ch_idx];

			chan->flags = 0;
		}
	}

	/* update to kernel */
	wiphy_apply_custom_regulatory(pWiphy, pRegdom);
}

void ShowChannel(struct wiphy *pWiphy)
{
	u32 band_idx, ch_idx;
	u32 ch_count;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;

	ch_count = 0;
	for (band_idx = 0; band_idx < 2; band_idx++) {
		sband = pWiphy->bands[band_idx];
		if (!sband)
			continue;

		for (ch_idx = 0; ch_idx < sband->n_channels; ch_idx++) {
			chan = &sband->channels[ch_idx];

			if (chan->flags & IEEE80211_CHAN_DISABLED) {
				wl_info("disable channels[%d][%d]: ch%d (freq = %d) flags=0x%x\n",
					band_idx, ch_idx, chan->hw_value, chan->center_freq, chan->flags);
				continue;
			}

			/* Allowable channel */
			if (ch_count == TOTAL_2G_5G_CHANNEL_NUM) {
				wl_info("%s(): no buffer to store channel information.\n", __func__);
				break;
			}

			wl_info("channels[%d][%d]: ch%d (freq = %d) flgs=0x%x\n",
				band_idx, ch_idx, chan->hw_value, chan->center_freq, chan->flags);

			ch_count++;
		}
	}
}

static bool freq_in_rule_band(const struct ieee80211_freq_range *freq_range,
				  u32 freq_khz)
{
#define ONE_GHZ_IN_KHZ	1000000
	/*
	 * From 802.11ad: directional multi-gigabit (DMG):
	 * Pertaining to operation in a frequency band containing a channel
	 * with the Channel starting frequency above 45 GHz.
	 */
	u32 limit = freq_khz > 45 * ONE_GHZ_IN_KHZ ?
			10 * ONE_GHZ_IN_KHZ : 2 * ONE_GHZ_IN_KHZ;
	if (abs(freq_khz - freq_range->start_freq_khz) <= limit)
		return true;
	if (abs(freq_khz - freq_range->end_freq_khz) <= limit)
		return true;
	return false;
#undef ONE_GHZ_IN_KHZ
}
static bool reg_does_bw_fit(const struct ieee80211_freq_range *freq_range,
				u32 center_freq_khz, u32 bw_khz)
{
	u32 start_freq_khz, end_freq_khz;

	start_freq_khz = center_freq_khz - (bw_khz/2);
	end_freq_khz = center_freq_khz + (bw_khz/2);

	if (start_freq_khz >= freq_range->start_freq_khz &&
		end_freq_khz <= freq_range->end_freq_khz)
		return true;

	return false;
}
const struct ieee80211_reg_rule *
sprd_freq_reg_info_regd(u32 center_freq,
		   const struct ieee80211_regdomain *regd)
{
	int i;
	bool band_rule_found = false;
	bool bw_fits = false;

	if (!regd)
		return ERR_PTR(-EINVAL);

	for (i = 0; i < regd->n_reg_rules; i++) {
		const struct ieee80211_reg_rule *rr;
		const struct ieee80211_freq_range *fr = NULL;

		rr = &regd->reg_rules[i];
		fr = &rr->freq_range;

		/*
		 * We only need to know if one frequency rule was
		 * was in center_freq's band, that's enough, so lets
		 * not overwrite it once found
		 */
		if (!band_rule_found)
			band_rule_found = freq_in_rule_band(fr, center_freq);

		bw_fits = reg_does_bw_fit(fr, center_freq, MHZ_TO_KHZ(20));

		if (band_rule_found && bw_fits)
			return rr;
	}

	if (!band_rule_found)
		return ERR_PTR(-ERANGE);

	return ERR_PTR(-EINVAL);
}
