/*
 * Copyright (C) 2017 Spreadtrum Communications Inc.
 *
 * This file is dual-licensed: you can use it either under the terms
 * of the GPL or the X11 license, at your option. Note that this dual
 * licensing only applies to this file, and not this project as a
 * whole.
 *
 */
#ifndef _WCN_GLB_REG_H_
#define _WCN_GLB_REG_H_


#ifndef CONFIG_CHECK_DRIVER_BY_CHIPID
#ifdef CONFIG_UWE5621
#include "uwe5621_glb.h"
#endif

#ifdef CONFIG_UWE5622
#include "uwe5622_glb.h"
#endif

#ifdef CONFIG_UWE5623
#include "uwe5623_glb.h"
#endif

#else
#include "uwe562x_glb.h"
#endif

#endif
