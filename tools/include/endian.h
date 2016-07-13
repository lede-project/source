#ifndef __endian_compat_h
#define __endian_compat_h

#if defined(__linux__) || defined(__CYGWIN__)
#include <byteswap.h>
#include_next <endian.h>
#elif defined(__APPLE__)
#include <machine/endian.h>
#include <machine/byte_order.h>
#include <libkern/OSByteOrder.h>
#define bswap_16(x) NXSwapShort(x)
#define bswap_32(x) NXSwapInt(x)
#define bswap_64(x) NXSwapLongLong(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#elif defined(__OpenBSD__)
#include <sys/types.h>
#define bswap_16(x) __swap16(x)
#define bswap_32(x) __swap32(x)
#define bswap_64(x) __swap64(x)
#else
#include <machine/endian.h>
#define bswap_16(x) swap16(x)
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)
#endif

#ifndef __BYTE_ORDER
#define __BYTE_ORDER BYTE_ORDER
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN
#endif
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif

#endif
