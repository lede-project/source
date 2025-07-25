#ifndef __WCN_DUMP_INTEGRATE_H__
#define __WCN_DUMP_INTEGRATE_H__

#define WCN_DUMP_END_STRING "marlin_memdump_finish"
#define WCN_CP2_STATUS_DUMP_REG	0x6a6b6c6d

#define DUMP_PACKET_SIZE	(1024)

typedef void (*gnss_dump_callback) (void);
void mdbg_dump_gnss_register(
			gnss_dump_callback callback_func, void *para);
void mdbg_dump_gnss_unregister(void);
int mdbg_snap_shoot_iram(void *buf);
void mdbg_dump_mem(void);
int dump_arm_reg(void);
#endif
