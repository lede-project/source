#ifdef SOFTAP_HOOK
void sprdwl_hook_reset_channel(struct wiphy *wiphy,
			       struct cfg80211_ap_settings *set);
#else
static inline void sprdwl_hook_reset_channel(struct wiphy *wiphy,
					     struct cfg80211_ap_settings *set)
{
}
#endif
