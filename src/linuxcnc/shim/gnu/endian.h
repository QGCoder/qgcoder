#pragma once
/// \file
/// A stand-in for glibc's <endian.h>.
///
/// rtapi_byteorder.h includes it to work out the byte order. It has a branch
/// for FreeBSD's <sys/endian.h> and none for macOS, which keeps the same
/// definitions in <machine/endian.h>, so macOS falls through to the glibc
/// include and does not find it. Windows has no equivalent at all.
///
/// Every target qgcoder builds for on either platform - x86, x86-64, ARM64 -
/// is little-endian, so the answer is known.
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412

#define __BYTE_ORDER       __LITTLE_ENDIAN
#define __FLOAT_WORD_ORDER __LITTLE_ENDIAN

#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif
#ifndef BIG_ENDIAN
#  define BIG_ENDIAN __BIG_ENDIAN
#endif
#ifndef BYTE_ORDER
#  define BYTE_ORDER __BYTE_ORDER
#endif
