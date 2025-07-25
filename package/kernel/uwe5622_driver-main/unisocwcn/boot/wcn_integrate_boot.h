#ifndef __WCN_INTEGRATE_BOOT_H__
#define __WCN_INTEGRATE_BOOT_H__

enum {
	WCN_POWER_STATUS_OFF = 0,
	WCN_POWER_STATUS_ON,
};

int start_integrate_wcn(u32 subsys);
int stop_integrate_wcn(u32 subsys);
int start_integrate_wcn_truely(u32 subsys);
int stop_integrate_wcn_truely(u32 subsys);
int wcn_proc_native_start(void *arg);
int wcn_proc_native_stop(void *arg);
void wcn_boot_init(void);
void wcn_power_wq(struct work_struct *pwork);
void wcn_device_poweroff(void);
void gnss_write_efuse_data(void);
void wcn_marlin_write_efuse(void);
#endif
