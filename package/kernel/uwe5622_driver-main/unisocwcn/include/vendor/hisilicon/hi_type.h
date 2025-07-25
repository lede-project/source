/************************************************************************
 * Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 ************************************************************************
 File Name     : hi_type.h
 Version       : Initial Draft
 Author        : Hisilicon multimedia software group
 Created       : 2005/4/23
 Last Modified :
 Description   : Common data types of the system. CNcomment: CNend
 Function List :
 History       :
 ************************************************************************
 */

#ifndef __HI_TYPE_H__
#define __HI_TYPE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/*-----------------------------------------------------------------------
 * Defintion of basic data types. The data types are applicable to both
 * the application layer and kernel codes. *
 * CNcomment: CNend                                                   *
 *-----------------------------------------------------------------------
 */
/*************************** Structure Definition ***************************/
/** Constant Definition */
enum HI_BOOL {
	HI_FALSE = 0,
	HI_TRUE = 1,
};

#ifndef NULL
#define NULL                0L
#endif

#define HI_NULL             0L
#define HI_NULL_PTR         0L

#define HI_SUCCESS          0
#define HI_FAILURE          (-1)

#define HI_INVALID_HANDLE   (0xffffffff)

#define HI_INVALID_PTS      (0xffffffff)
#define HI_INVALID_TIME     (0xffffffff)

#define HI_OS_LINUX     0xabcd
#define HI_OS_WIN32     0xcdef

#ifdef _WIN32
#define HI_OS_TYPE      HI_OS_WIN32
#else
#define __OS_LINUX__
#define HI_OS_TYPE      HI_OS_LINUX
#endif

#ifdef HI_ADVCA_SUPPORT
#define __INIT__
#define __EXIT__
#else
#define __INIT__  __init
#define __EXIT__  __exit
#endif

/**
 * define of HI_HANDLE(unsigned int):
 *bit31                                                           bit0
 *|<----   16bit --------->|<---   8bit    --->|<---  8bit   --->|
 *|--------------------------------------------------------------|
 *|      HI_MOD_ID_E       |  mod defined data |     chnID       |
 *|--------------------------------------------------------------|
 *mod defined data: private data define by each module
 *(for example: sub-mod id), usually, set to 0.
 */

#define HI_HANDLE_MAKEHANDLE(mod, privatedata, chnid)\
	(unsigned int)((((mod) & 0xffff) << 16) |\
	(((privatedata) & 0xff) << 8) | ((chnid) & 0xff))

#define HI_HANDLE_GET_MODID(handle)     (((handle) >> 16) & 0xffff)
#define HI_HANDLE_GET_PriDATA(handle)   (((handle) >> 8) & 0xff)
#define HI_HANDLE_GET_CHNID(handle)     (((handle)) & 0xff)

#define UNUSED(x) ((x) = (x))

/** @} */  /** <!-- ==== Structure Definition end ==== */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __HI_TYPE_H__ */

