#ifndef __WCN_WRAPPER_H__
#define __WCN_WRAPPER_H__
#include <linux/version.h>
#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
#ifndef timespec
#define timespec timespec64
#define timespec_to_ns timespec64_to_ns
#define getnstimeofday ktime_get_real_ts64
#define do_gettimeofday ktime_get_real_ts64
#define timeval_to_ns timespec64_to_ns
#define timeval timespec64
#endif

#elif KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE
#define timeval timespec
#define do_gettimeofday getnstimeofday
#define timeval_to_ns timespec_to_ns

#endif
#endif//__WCN_WRAPPER_H__
