#ifndef __WCN_INTEGRATE_DEV_H__
#define __WCN_INTEGRATE_DEV_H__

#include "rf.h"

/* The name should be set the same as DTS */
#define WCN_MARLIN_DEV_NAME "wcn_btwf"
#define WCN_GNSS_DEV_NAME "wcn_gnss"

/*
 * ASIC: enable or disable vddwifipa and vddcon,
 * the interval time should more than 1ms.
 */
#define VDDWIFIPA_VDDCON_MIN_INTERVAL_TIME	(10000)	/* us */
#define VDDWIFIPA_VDDCON_MAX_INTERVAL_TIME	(30000)	/* us */

enum wcn_marlin_sub_sys {
	WCN_MARLIN_BLUETOOTH = 0,
	WCN_MARLIN_FM,
	WCN_MARLIN_WIFI,
	WCN_MARLIN_MDBG = 6,
	WCN_MARLIN_ALL = 7,
};

enum wcn_gnss_sub_sys {
	/*
	 * The value is different with wcn_marlin_sub_sys
	 * Or the start interface can't distinguish
	 * Marlin or GNSS
	 */
	WCN_GNSS = 16,
	WCN_GNSS_BD,
	WCN_GNSS_ALL,
};

#define WCN_BTWF_FILENAME "wcnmodem"
#define WCN_GNSS_FILENAME "gpsgl"
#define WCN_GNSS_BD_FILENAME "gpsbd"

/* NOTES:If DTS config more than REG_CTRL_CNT_MAX REGs */
#define REG_CTRL_CNT_MAX 8
/* NOTES:If DTS config more than REG_SHUTDOWN_CNT_MAX REGs */
#define REG_SHUTDOWN_CNT_MAX 4

#define WCN_INTEGRATE_PLATFORM_DEBUG 0
#define SUSPEND_RESUME_ENABLE 0

#define WCN_OPEN_MAX_CNT (0x10)

/* default VDDCON voltage is 1.6v, work voltage is 1.2v */
#define WCN_VDDCON_WORK_VOLTAGE (1200000)
/* default VDDCON voltage is 3.3v, work voltage is 3.0v */
#define WCN_VDDWIFIPA_WORK_VOLTAGE (3000000)

#define WCN_PROC_FILE_LENGTH_MAX (63)

#define FIRMWARE_FILEPATHNAME_LENGTH_MAX 256
#define WCN_MARLIN_MASK 0xcf /* Base on wcn_marlin_sub_sys */
#define WCN_MARLIN_BTWIFI_MASK 0x05
#define WCN_GNSS_MASK (1<<WCN_GNSS)
#define WCN_GNSS_BD_MASK (1<<WCN_GNSS_BD)
#define WCN_GNSS_ALL_MASK (WCN_GNSS_MASK|WCN_GNSS_BD_MASK)

/* type for base REGs */
enum {
	REGMAP_AON_APB = 0x0,	/* AON APB */
	REGMAP_PMU_APB,
	/*
	 * NOTES:SharkLE use it,but PIKE2 not.
	 * We should config the DTS for PIKE2 also.
	 */
	REGMAP_PUB_APB, /* SharkLE only:for ddr offset */
	REGMAP_ANLG_WRAP_WCN,
	REGMAP_ANLG_PHY_G5, /* SharkL3 only */
	REGMAP_ANLG_PHY_G6, /* SharkLE only */
	REGMAP_WCN_REG,	/* SharkL3 only:0x403A 0000 */
	REGMAP_TYPE_NR,
};

#if WCN_INTEGRATE_PLATFORM_DEBUG
enum wcn_integrate_platform_debug_case {
	NORMAL_CASE = 0,
	WCN_START_MARLIN_DEBUG,
	WCN_STOP_MARLIN_DEBUG,
	WCN_START_MARLIN_DDR_FIRMWARE_DEBUG,
	/* Next for GNSS */
	WCN_START_GNSS_DEBUG,
	WCN_STOP_GNSS_DEBUG,
	WCN_START_GNSS_DDR_FIRMWARE_DEBUG,
	/* Print Info */
	WCN_PRINT_INFO,
	WCN_BRINGUP_DEBUG,
};
#endif

struct platform_proc_file_entry {
	char			*name;
	struct proc_dir_entry	*platform_proc_dir_entry;
	struct wcn_device	*wcn_dev;
	unsigned int		flag;
};

#define MAX_PLATFORM_ENTRY_NUM		0x10
enum {
	BE_SEGMFG   = (0x1 << 4),
	BE_RDONLY   = (0x1 << 5),
	BE_WRONLY   = (0x1 << 6),
	BE_CPDUMP   = (0x1 << 7),
	BE_MNDUMP   = (0x1 << 8),
	BE_RDWDT    = (0x1 << 9),
	BE_RDWDTS   = (0x1 << 10),
	BE_RDLDIF   = (0x1 << 11),
	BE_LD	    = (0x1 << 12),
	BE_CTRL_ON  = (0x1 << 13),
	BE_CTRL_OFF	= (0x1 << 14),
};

enum {
	CP_NORMAL_STATUS = 0,
	CP_STOP_STATUS,
	CP_MAX_STATUS,
};

struct wcn_platform_fs {
	struct proc_dir_entry		*platform_proc_dir_entry;
	struct platform_proc_file_entry entrys[MAX_PLATFORM_ENTRY_NUM];
};

struct wcn_proc_data {
	int (*start)(void *arg);
	int (*stop)(void *arg);
};

struct wcn_init_data {
	char		*devname;
	phys_addr_t	base;		/* CP base addr */
	u32		maxsz;		/* CP max size */
	int		(*start)(void *arg);
	int		(*stop)(void *arg);
	int		(*suspend)(void *arg);
	int		(*resume)(void *arg);
	int		type;
};

/* CHIP if include GNSS */
#define WCN_INTERNAL_INCLUD_GNSS_VAL (0)
#define WCN_INTERNAL_NOINCLUD_GNSS_VAL (0xab520)

/* WIFI cali */
#define WIFI_CALIBRATION_FLAG_VALUE	(0xefeffefe)
#define WIFI_CALIBRATION_FLAG_CLEAR_VALUE	(0x12345678)
/*GNSS cali*/
#define GNSS_CALIBRATION_FLAG_CLEAR_ADDR (0x00150028)
#define GNSS_CALIBRATION_FLAG_CLEAR_VALUE (0)
#define GNSS_CALIBRATION_FLAG_CLEAR_ADDR_CP \
	(GNSS_CALIBRATION_FLAG_CLEAR_ADDR + 0x300000)
#define GNSS_WAIT_CP_INIT_COUNT	(256)
#define GNSS_CALI_DONE_FLAG (0x1314520)
#define GNSS_WAIT_CP_INIT_POLL_TIME_MS	(20)	/* 20ms */

struct wifi_calibration {
	struct wifi_config_t config_data;
	struct wifi_cali_t cali_data;
};

/* wifi efuse data, default value comes from PHY team */
#define WIFI_EFUSE_BLOCK_COUNT (3)

#define MARLIN_CP_INIT_READY_MAGIC	(0xababbaba)
#define MARLIN_CP_INIT_START_MAGIC	(0x5a5a5a5a)
#define MARLIN_CP_INIT_SUCCESS_MAGIC	(0x13579bdf)
#define MARLIN_CP_INIT_FALIED_MAGIC	(0x88888888)

#define MARLIN_WAIT_CP_INIT_POLL_TIME_MS	(20)	/* 20ms */
#define MARLIN_WAIT_CP_INIT_COUNT	(256)
#define MARLIN_WAIT_CP_INIT_MAX_TIME (20000)
#define WCN_WAIT_SLEEP_MAX_COUNT (32)

/* begin : for gnss module */
/* record efuse, GNSS_EFUSE_DATA_OFFSET is defined in gnss.h */
#define GNSS_EFUSE_BLOCK_COUNT (3)
#define GNSS_EFUSE_ENABLE_ADDR (0x150024)
#define GNSS_EFUSE_ENABLE_VALUE (0x20190E0E)
/* end: for gnss */

#define WCN_BOOT_CP2_OK 0
#define WCN_BOOT_CP2_ERR_DONW_IMG 1
#define WCN_BOOT_CP2_ERR_BOOT 2
struct wcn_device {
	char	*name;
	/* DTS info: */

	/*
	 * wcn and gnss ctrl_reg num
	 * from ctrl-reg[0] to ctrl-reg[ctrl-probe-num - 1]
	 * need init in the driver probe stage
	 */
	u32	ctrl_probe_num;
	u32	ctrl_reg[REG_CTRL_CNT_MAX]; /* offset */
	u32	ctrl_mask[REG_CTRL_CNT_MAX];
	u32	ctrl_value[REG_CTRL_CNT_MAX];
	/*
	 * Some REGs Read and Write has about 0x1000 offset;
	 * REG_write - REG_read=0x1000, the DTS value is write value
	 */
	u32	ctrl_rw_offset[REG_CTRL_CNT_MAX];
	u32	ctrl_us_delay[REG_CTRL_CNT_MAX];
	u32	ctrl_type[REG_CTRL_CNT_MAX]; /* the value is pmu or apb */
	struct	regmap *rmap[REGMAP_TYPE_NR];
	u32	reg_nr;
	/* Shut down group */
	u32	ctrl_shutdown_reg[REG_SHUTDOWN_CNT_MAX];
	u32	ctrl_shutdown_mask[REG_SHUTDOWN_CNT_MAX];
	u32	ctrl_shutdown_value[REG_SHUTDOWN_CNT_MAX];
	u32	ctrl_shutdown_rw_offset[REG_SHUTDOWN_CNT_MAX];
	u32	ctrl_shutdown_us_delay[REG_SHUTDOWN_CNT_MAX];
	u32	ctrl_shutdown_type[REG_SHUTDOWN_CNT_MAX];
	/* struct regmap *rmap_shutdown[REGMAP_TYPE_NR]; */
	u32	reg_shutdown_nr;	/* REG_SHUTDOWN_CNT_MAX */
	phys_addr_t	base_addr;
	bool	download_status;
	char	*file_path;
	char	*file_path_ext;
	char	firmware_path[FIRMWARE_FILEPATHNAME_LENGTH_MAX];
	char	firmware_path_ext[FIRMWARE_FILEPATHNAME_LENGTH_MAX];
	u32	file_length;
	/* FS OPS info: */
	struct	wcn_platform_fs platform_fs;
	int	status;
	u32	wcn_open_status;	/* marlin or gnss subsys status */
	u32	boot_cp_status;
	/* driver OPS */
	int	(*start)(void *arg);
	int	(*stop)(void *arg);
	u32	maxsz;
	struct	mutex power_lock;
	u32	power_state;
	struct regulator *vddwifipa;
	struct mutex vddwifipa_lock;
	char	*write_buffer;
	struct	delayed_work power_wq;
	struct	work_struct load_wq;
	struct	delayed_work cali_wq;
	struct	completion download_done;
};

struct wcn_device_manage {
	struct wcn_device *btwf_device;
	struct wcn_device *gnss_device;
	struct regulator *vddwcn;
	struct mutex vddwcn_lock;
	int vddwcn_en_count;
	int gnss_type;
	bool vddcon_voltage_setted;
	bool btwf_calibrated;
};

extern struct wcn_device_manage s_wcn_device;

static inline bool wcn_dev_is_marlin(struct wcn_device *dev)
{
	return dev == s_wcn_device.btwf_device;
}

static inline bool wcn_dev_is_gnss(struct wcn_device *dev)
{
	return dev == s_wcn_device.gnss_device;
}

void wcn_cpu_bootup(struct wcn_device *wcn_dev);
#endif
