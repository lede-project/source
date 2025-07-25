#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <marlin_platform.h>

#include "mdbg_type.h"
#include "rdc_debug.h"
#include "wcn_txrx.h"

#define WCN_DEBUG_RETRY_TIMES	1
#define WCN_DEBUG_MAX_PATH_LEN	110

/* size of cp2 log files, default is 20M. */
#ifdef CONFIG_CUSTOMIZE_UNISOC_DBG_FILESIZE
#define UNISOC_DBG_FILESIZE_DEFAULT CONFIG_CUSTOMIZE_UNISOC_DBG_FILESIZE
#else
#define UNISOC_DBG_FILESIZE_DEFAULT 20
#endif
/* num of cp2 log files. */
#ifdef CONFIG_CUSTOMIZE_UNISOC_DBG_FILENUM
#define UNISOC_DBG_FILENUM_DEFAULT CONFIG_CUSTOMIZE_UNISOC_DBG_FILENUM
#else
#define UNISOC_DBG_FILENUM_DEFAULT 2
#endif
/* path of cp2 log and mem files. */
#ifdef CONFIG_CUSTOMIZE_UNISOC_DBG_PATH
#define UNISOC_DBG_PATH_DEFAULT CONFIG_CUSTOMIZE_UNISOC_DBG_PATH
#else
#define UNISOC_DBG_PATH_DEFAULT "/data/unisoc_dbg"
#endif

/* size of cp2 log files, default is 20M. */
static unsigned int wcn_cp2_log_limit_size =
	UNISOC_DBG_FILESIZE_DEFAULT * 1024 * 1024;
/* num of cp2 log files. */
static unsigned int wcn_cp2_file_max_num = UNISOC_DBG_FILENUM_DEFAULT;
/* cover_old: when reached wcn_cp2_file_max_num will write from 0 file
 * again, otherwise log file will not be limited by wcn_cp2_file_max_num.
 */
static unsigned int wcn_cp2_log_cover_old = 1;
/* path of config file unisoc_cp2log_config.txt */
#define WCN_DEBUG_CFG_MAX_PATH_NUM	2
static char *wcn_cp2_config_path[WCN_DEBUG_CFG_MAX_PATH_NUM] = {
	"/data/unisoc_cp2log_config.txt",
	"/vendor/etc/wifi/unisoc_cp2log_config.txt"
};
/* path of cp2 log and mem files. */
#define WCN_UNISOC_DBG_MAX_PATH_NUM	3
static char *wcn_unisoc_dbg_path[WCN_UNISOC_DBG_MAX_PATH_NUM] = {
	UNISOC_DBG_PATH_DEFAULT,/* most of projects */
	"/data",		/* amlogic s905w... */
	"/mnt/UDISK"		/* allwinner r328... */
};

#define WCN_CP2_LOG_NAME       "/unisoc_cp2log_%%d.txt"
#define WCN_CP2_MEM_NAME       "/unisoc_cp2mem_%%d.mem"

static char wcn_cp2_log_num;
static char wcn_cp2_mem_num;
static loff_t wcn_cp2_log_pos;
static loff_t wcn_cp2_mem_pos;
static char wcn_cp2_log_path[WCN_DEBUG_MAX_PATH_LEN];
static char wcn_cp2_mem_path[WCN_DEBUG_MAX_PATH_LEN];
static char wcn_cp2_log_path_tmp[WCN_DEBUG_MAX_PATH_LEN];
static char wcn_unisoc_dbg_path_tmp[WCN_DEBUG_MAX_PATH_LEN];
static char debug_inited;
static char debug_user_inited;
static char config_inited;

static int wcn_mkdir(char *path)
{
	struct file *fp;

	/* check if the new dir is created. */
	fp = filp_open(path, O_DIRECTORY, 0644);
	if (IS_ERR(fp)) {
		WCN_INFO("open %s error!\n", path);
		return -1;
	}
	WCN_DEBUG("open %s success.\n", path);
	filp_close(fp, NULL);
	return 0;
}

static int wcn_find_cp2_file_num(char *path, loff_t *pos)
{
	int i;
	struct file *fp_size;
	/*first file whose size less than wcn_cp2_log_limit_size*/
	int first_small_file = 0;
	char first_file_set = 0;
	int first_file_size = 0;
	char wcn_cp2_file_path[WCN_DEBUG_MAX_PATH_LEN];
	int config_size = 0;
	int num = 0;
	int exist_file_num = 0;


	if (wcn_cp2_log_cover_old) {
		for (i = 0; i < wcn_cp2_file_max_num; i++) {
			sprintf(wcn_cp2_file_path, path, i);
			fp_size = filp_open(wcn_cp2_file_path, O_RDONLY, 0);
			if (IS_ERR(fp_size)) {
				WCN_INFO("%s: Error, config file not found. want config file:%s \n",
					__func__, wcn_cp2_file_path);
				break;
			}
			exist_file_num++;
			config_size = (int)fp_size->f_inode->i_size;
			if ((config_size < wcn_cp2_log_limit_size) &&
				(first_file_set == 0)) {
				first_small_file = i;
				first_file_set = 1;
				first_file_size = config_size;
			}
		}
		/* file number reaches max num*/
		if (i == wcn_cp2_file_max_num) {
			num = first_small_file;
			*pos = first_file_size;
			/* If all the exist files reached wcn_cp2_log_limit_size
			 * empty the 0 file.
			 */
			if (first_file_set == 0) {
				struct file *filp = NULL;

				sprintf(wcn_cp2_file_path, path, 0);
				WCN_INFO("%s: empty:%s\n", __func__,
					wcn_cp2_file_path);
				filp = filp_open(wcn_cp2_file_path,
					O_CREAT | O_RDWR | O_TRUNC, 0644);
				if (IS_ERR(filp))
					WCN_INFO("%s: can not empty:%s\n",
						__func__, wcn_cp2_file_path);
				else
					filp_close(filp, NULL);
			}
		} else {
			/* in case all exist files reached log_limit_size,
			 * the file number still not reach file_max_num.
			 */
			if ((first_file_set == 0) && (exist_file_num != 0)) {
				/* use a new file */
				num = i;
				*pos = 0;
			} else {
				num = first_small_file;
				*pos = first_file_size;
			}
		}
	} else {
		struct file *fp = NULL;

		num = 0;
		*pos = 0;
		sprintf(wcn_cp2_file_path, path, 0);
		fp = filp_open(wcn_cp2_file_path,
			O_CREAT | O_RDWR | O_TRUNC, 0644);
		if (IS_ERR(fp)) {
			WCN_INFO("%s :%s file is not exit\n",
				__func__, wcn_cp2_file_path);
		} else
			filp_close(fp, NULL);
	}
	return num;
}

int log_rx_callback(void *addr, unsigned int len)
{
	ssize_t ret;
	loff_t file_size = 0;
	struct file *filp;
	static int retry;

	WCN_DEBUG("log_rx_callback\n");
	if ((debug_inited == 0) && (debug_user_inited == 0))
		return 0;

	if (retry > WCN_DEBUG_RETRY_TIMES)
		return 0;

retry:
	filp = filp_open(wcn_cp2_log_path, O_CREAT | O_RDWR | O_APPEND, 0644);
	if (IS_ERR(filp)) {
		if (retry > 0)
			WCN_ERR("%s open %s error no.%ld retry:%d\n", __func__,
				wcn_cp2_log_path, PTR_ERR(filp), retry);

		/*in case the path no longer exist, creat path or change path.*/
		if ((PTR_ERR(filp) == -ENOENT) && (retry == 0)) {
			retry = 1;
			WCN_DEBUG("%s: %s is no longer exist!\n",
				  __func__, wcn_cp2_log_path);
			if (wcn_mkdir(wcn_unisoc_dbg_path_tmp) == 0) {
				wcn_set_log_file_path(
					wcn_unisoc_dbg_path_tmp,
					strlen(
					wcn_unisoc_dbg_path_tmp));
				goto retry;
			}
			return PTR_ERR(filp);
		}
		retry++;
		if (PTR_ERR(filp) == -EACCES)
			WCN_ERR("%s: Permission denied.\n", __func__);
		else if (PTR_ERR(filp) == -ENOMEM)
			WCN_ERR("%s: no memory in system,"
				"please delete old log file.\n",
				__func__);
		return PTR_ERR(filp);
	}

	file_size = wcn_cp2_log_pos;

	if (file_size > wcn_cp2_log_limit_size) {
		filp_close(filp, NULL);
		wcn_cp2_log_pos = 0;

		if (wcn_cp2_log_cover_old) {
			if ((wcn_cp2_log_num + 1) < wcn_cp2_file_max_num) {
				wcn_cp2_log_num++;
				sprintf(wcn_cp2_log_path, wcn_cp2_log_path_tmp,
					wcn_cp2_log_num);
			} else if ((wcn_cp2_log_num + 1) ==
				wcn_cp2_file_max_num) {
				wcn_cp2_log_num = 0;
				sprintf(wcn_cp2_log_path, wcn_cp2_log_path_tmp,
					wcn_cp2_log_num);
			} else {
				WCN_INFO("%s error log num:%d\n",
					__func__, wcn_cp2_log_num);
				wcn_cp2_log_num = 0;
				sprintf(wcn_cp2_log_path, wcn_cp2_log_path_tmp,
					wcn_cp2_log_num);
			}
		} else {
			wcn_cp2_log_num++;
			sprintf(wcn_cp2_log_path, wcn_cp2_log_path_tmp,
				wcn_cp2_log_num);
		}

		WCN_INFO("%s cp2 log file is %s\n", __func__,
			 wcn_cp2_log_path);
		filp = filp_open(wcn_cp2_log_path,
				 O_CREAT | O_RDWR | O_TRUNC, 0644);
		if (IS_ERR(filp)) {
			WCN_ERR("%s open wcn log file error no. %d\n",
				__func__, (int)IS_ERR(filp));
			return PTR_ERR(filp);
		}
	}

#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
	ret = kernel_write(filp, (void *)addr, len, &wcn_cp2_log_pos);
#else
	ret = kernel_write(filp, addr, len, wcn_cp2_log_pos);
#endif

	if (ret != len) {
		WCN_ERR("wcn log write to file failed: %zd\n", ret);
		filp_close(filp, NULL);
		return ret < 0 ? ret : -ENODEV;
	}
	wcn_cp2_log_pos += ret;
	filp_close(filp, NULL);

	return 0;
}

int dumpmem_rx_callback(void *addr, unsigned int len)
{
	ssize_t ret;
	struct file *filp;
	static int first_time_open = 1;
	static int retry;

	WCN_INFO("dumpmem_rx_callback\n");

	if (retry > WCN_DEBUG_RETRY_TIMES)
		return 0;

retry:
	if (first_time_open)
		filp = filp_open(wcn_cp2_mem_path,
			O_CREAT | O_RDWR | O_TRUNC, 0644);
	else
		filp = filp_open(wcn_cp2_mem_path,
			O_CREAT | O_RDWR | O_APPEND, 0644);
	if (IS_ERR(filp)) {
		if (retry > 0)
			WCN_ERR("%s open %s error no.%ld retry:%d\n", __func__,
				wcn_cp2_mem_path, PTR_ERR(filp), retry);

		/*in case the path no longer exist, creat path or change path.*/
		if ((PTR_ERR(filp) == -ENOENT) && (retry == 0)) {
			retry = 1;
			WCN_DEBUG("%s: %s is no longer exist!\n",
				  __func__, wcn_cp2_mem_path);
			if (wcn_mkdir(wcn_unisoc_dbg_path_tmp) == 0) {
				wcn_set_log_file_path(
					wcn_unisoc_dbg_path_tmp,
					strlen(
					wcn_unisoc_dbg_path_tmp));
				goto retry;
			}
			return PTR_ERR(filp);
		}
		retry++;
		return PTR_ERR(filp);
	}

	if (first_time_open)
		first_time_open = 0;

#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
	ret = kernel_write(filp, (void *)addr, len, &wcn_cp2_mem_pos);
#else
	ret = kernel_write(filp, addr, len, wcn_cp2_mem_pos);
#endif

	if (ret != len) {
		WCN_ERR("wcn mem write to file failed: %zd\n", ret);
		filp_close(filp, NULL);
		return ret < 0 ? ret : -ENODEV;
	}

	wcn_cp2_mem_pos += ret;
	filp_close(filp, NULL);

	return 0;
}

/* unit of log_file_limit_size is MByte. */
int wcn_set_log_file_limit_size(unsigned int log_file_limit_size)
{
	wcn_cp2_log_limit_size = log_file_limit_size * 1024 * 1024;
	WCN_INFO("%s = %d bytes\n", __func__, wcn_cp2_log_limit_size);
	return 0;
}

int wcn_set_log_file_max_num(unsigned int log_file_max_num)
{
	wcn_cp2_file_max_num = log_file_max_num;
	WCN_INFO("%s = %d\n", __func__, wcn_cp2_file_max_num);
	return 0;
}

int wcn_set_log_file_cover_old(unsigned int is_cover_old)
{
	if (is_cover_old == 1) {
		wcn_cp2_log_cover_old = is_cover_old;
		WCN_INFO("%s will cover old files!\n", __func__);
		return 0;
	} else if (is_cover_old == 0) {
		wcn_cp2_log_cover_old = is_cover_old;
		WCN_INFO("%s NOT cover old files!\n", __func__);
		return 0;
	}
	WCN_ERR("%s param is invalid!\n", __func__);
	return -1;
}

int wcn_set_log_file_path(char *path, unsigned int path_len)
{
	char wcn_cp2_log_path_user_tmp[WCN_DEBUG_MAX_PATH_LEN] = {0};
	char wcn_cp2_mem_path_user_tmp[WCN_DEBUG_MAX_PATH_LEN] = {0};
	char wcn_cp2_log_path_user[WCN_DEBUG_MAX_PATH_LEN] = {0};
	char wcn_cp2_mem_path_user[WCN_DEBUG_MAX_PATH_LEN] = {0};
	char log_name[] = WCN_CP2_LOG_NAME;
	char mem_name[] = WCN_CP2_MEM_NAME;
	char wcn_cp2_log_num_user;
	char wcn_cp2_mem_num_user;
	loff_t wcn_cp2_log_pos_user = 0;
	struct file *filp;

	/* 10 is the len of 0xFFFFFFFF to decimal num*/
	if (path_len > (WCN_DEBUG_MAX_PATH_LEN - 10)) {
		WCN_ERR("%s: log path is too long:%d", __func__, path_len);
		return -1;
	}

	sprintf(wcn_cp2_log_path_user_tmp, path);
	sprintf(wcn_cp2_log_path_user_tmp + path_len, log_name);
	sprintf(wcn_cp2_mem_path_user_tmp, path);
	sprintf(wcn_cp2_mem_path_user_tmp + path_len, mem_name);

	wcn_cp2_log_num_user =
		wcn_find_cp2_file_num(wcn_cp2_log_path_user_tmp,
			&wcn_cp2_log_pos_user);
	sprintf(wcn_cp2_log_path_user, wcn_cp2_log_path_user_tmp,
		wcn_cp2_log_num_user);

	wcn_cp2_mem_num_user = 0; //only one mem_dump file
	sprintf(wcn_cp2_mem_path_user,
		wcn_cp2_mem_path_user_tmp, wcn_cp2_mem_num_user);

	//check if the new path is valid.
	filp = filp_open(wcn_cp2_log_path_user,
		O_CREAT | O_RDWR | O_APPEND, 0644);
	if (IS_ERR(filp)) {
		WCN_ERR("new path [%s] is invalid %d\n", wcn_cp2_log_path_user,
			(int)IS_ERR(filp));
		return PTR_ERR(filp);
	}
	filp_close(filp, NULL);

	debug_inited = 0;
	debug_user_inited = 0;

	strcpy(wcn_cp2_log_path, wcn_cp2_log_path_user);
	strcpy(wcn_cp2_mem_path, wcn_cp2_mem_path_user);
	strcpy(wcn_cp2_log_path_tmp, wcn_cp2_log_path_user_tmp);
	wcn_cp2_log_pos = wcn_cp2_log_pos_user;
	wcn_cp2_mem_pos = 0;
	wcn_cp2_log_num = wcn_cp2_log_num_user;
	wcn_cp2_mem_num = wcn_cp2_mem_num_user;

	WCN_INFO("%s cp2 log/mem file: %s, logpos=%d, %s\n", __func__,
		 wcn_cp2_log_path, (int)wcn_cp2_log_pos, wcn_cp2_mem_path);

	debug_user_inited = 1;

	return 0;
}

static void wcn_config_log_file(void)
{
	struct file *filp;
	loff_t offset = 0;
	struct file *fp_size;
	int config_size = 0;
	int read_len = 0;
	char *buf;
	char *buf_end;
	char *limit_size = "wcn_cp2_log_limit_size=";
	char *max_num = "wcn_cp2_file_max_num=";
	char *cover_old = "wcn_cp2_file_cover_old=";
	char *log_path = "wcn_cp2_log_path=";
	char *cc = NULL;
	int config_limit_size = 0;
	int config_max_num = 0;
	int index = 0;

	for (index = 0; index < WCN_DEBUG_CFG_MAX_PATH_NUM; index++) {
		fp_size = filp_open(wcn_cp2_config_path[index], O_RDONLY, 0);
		if (IS_ERR(fp_size)) {
			WCN_INFO("%s: Error, config file not found. want config file:%s \n",
				__func__, wcn_cp2_config_path[index]);
		}
		else {
			config_size = (int)fp_size->f_inode->i_size;
			WCN_INFO("%s: find config file:%s size:%d\n",
				 __func__, wcn_cp2_config_path[index],
				 config_size);
			break;
		}
	}
	if (index == WCN_DEBUG_CFG_MAX_PATH_NUM) {
		WCN_INFO("%s: there is no unisoc_cp2log_config.txt\n",
			 __func__);
		return;
	}

	buf = kzalloc(config_size+1, GFP_KERNEL);
	if (!buf) {
		WCN_ERR("%s:no more space[%d]!\n", __func__, config_size+1);
		return;
	}
	buf_end = buf + config_size;

	filp = filp_open(wcn_cp2_config_path[index], O_RDONLY, 0);
	if (IS_ERR(filp)) {
		WCN_ERR("%s: can not open log config file:%s\n",
			__func__, wcn_cp2_config_path[index]);
		goto out;
	}

#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
	read_len = kernel_read(filp, (void *)buf, config_size, &offset);
#else
	read_len = kernel_read(filp, offset, buf, config_size);
#endif

	if (read_len <= 0) {
		WCN_ERR("%s: can not read config file read_len:%d\n",
			__func__, read_len);
		goto out1;
	}

	buf[config_size+1] = '\0';
	//WCN_INFO("config_file:%s\n", buf);

	/* config wcn_cp2_log_limit_size */
	cc = strstr(buf, limit_size);
	if (cc == NULL || cc >= buf_end) {
		WCN_INFO("can not find limit_size in config file!\n");
		goto config_max_num;
	} else {
		config_limit_size =
			simple_strtol(cc + strlen(limit_size), &cc, 10);
		if (config_limit_size == 0) {
			WCN_ERR("config_limit_size invalid!\n");
			goto config_max_num;
		}
	}
	if ((cc[0] == 'M') || (cc[0] == 'm'))
		config_limit_size = config_limit_size*1024*1024;
	else if ((cc[0] == 'K') || (cc[0] == 'k'))
		config_limit_size = config_limit_size*1024;

	wcn_cp2_log_limit_size = config_limit_size;

config_max_num:
	/* config wcn_cp2_file_max_num */
	cc = strstr(buf, max_num);
	if (cc == NULL || cc >= buf_end) {
		WCN_INFO("can not find max_num in config file!\n");
		goto config_cover_old;
	} else {
		config_max_num = simple_strtol(cc + strlen(max_num), &cc, 10);
		if (config_max_num == 0) {
			WCN_ERR("config_max_num invalid!\n");
			goto config_cover_old;
		}
	}
	wcn_cp2_file_max_num = config_max_num;

config_cover_old:
	/* config wcn_cp2_log_cover_old */
	cc = strstr(buf, cover_old);
	if (cc == NULL || cc >= buf_end) {
		WCN_INFO("can not find cover_old in config file!\n");
		goto config_new_path;
	} else {
		if (strncmp(cc + strlen(cover_old), "true", 4) == 0)
			wcn_cp2_log_cover_old = 1;
		else if (strncmp(cc + strlen(cover_old), "TRUE", 4) == 0)
			wcn_cp2_log_cover_old = 1;
		else if (strncmp(cc + strlen(cover_old), "false", 5) == 0)
			wcn_cp2_log_cover_old = 0;
		else if (strncmp(cc + strlen(cover_old), "FALSE", 5) == 0)
			wcn_cp2_log_cover_old = 0;
		else
			WCN_ERR("%s param is invalid!\n", cover_old);
	}

config_new_path:
	/* config wcn_cp2_log_path */
	cc = strstr(buf, log_path);
	if (cc == NULL || cc >= buf_end) {
		WCN_INFO("can not find log_path in config file!\n");
	} else {
		char *path_start = cc + strlen(log_path) + 1;
		char *path_end = strstr(path_start, "\"");

		if (path_end == NULL || path_end >= buf_end) {
			WCN_ERR("can not find log_path_end in config file!\n");
		} else {
			path_end[0] = '\0';
			wcn_set_log_file_path(path_start, path_end-path_start);
		}
	}

out1:
	filp_close(filp, NULL);
out:
	kfree(buf);
}

int wcn_debug_init(void)
{
	int ret = 0;
	unsigned char i;

	WCN_DEBUG("%s entry\n", __func__);

	/* config cp2 log if there is a config file.*/
	if (config_inited == 0) {
		wcn_config_log_file();
		config_inited = 1;
		WCN_INFO("%s unisoc cp2 log: limit_size:[%d Byte], "
			 "log_file_num:[%d], cover_old:[%d-%s]\n",
			 __func__, wcn_cp2_log_limit_size,
			 wcn_cp2_file_max_num,
			 wcn_cp2_log_cover_old,
			 (wcn_cp2_log_cover_old ?
			 "cover_old" :
			 "NOT_cover_old"));
	}

	if (debug_inited || debug_user_inited) {
		WCN_DEBUG("%s log path %s already initialized\n",
			  __func__, wcn_cp2_log_path);
		return 0;
	}

	for (i = 0; i < WCN_UNISOC_DBG_MAX_PATH_NUM; i++) {
		if (wcn_mkdir(wcn_unisoc_dbg_path[i]) == 0) {
			ret = wcn_set_log_file_path(wcn_unisoc_dbg_path[i],
				strlen(wcn_unisoc_dbg_path[i]));
			if (!ret) {
				sprintf(wcn_unisoc_dbg_path_tmp,
					wcn_unisoc_dbg_path[i]);
				debug_inited = 1;
				return 0;
			} else
				return ret;
		}
	}
	return -1;
}
