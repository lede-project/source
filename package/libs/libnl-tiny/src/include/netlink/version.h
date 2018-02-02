/*
 * netlink/version.h	Compile Time Versioning Information
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_VERSION_H_
#define NETLINK_VERSION_H_

#define LIBNL_STRING "libnl"
#define LIBNL_VERSION "2.0"

#define LIBNL_VER_NUM ((LIBNL_VER_MAJ) << 16 | (LIBNL_VER_MIN) << 8 | (LIBNL_VER_MIC))
#define LIBNL_VER_MAJ 2
#define LIBNL_VER_MIN 0
#define LIBNL_VER_MIC 0

/* Run-time version information */

extern const int        nl_ver_num;
extern const int        nl_ver_maj;
extern const int        nl_ver_min;
extern const int        nl_ver_mic;

#endif
