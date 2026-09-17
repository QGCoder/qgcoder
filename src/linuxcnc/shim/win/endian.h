#pragma once
/// \file
/// A stand-in for glibc's <endian.h>, which Windows has no equivalent of.
///
/// rtapi_byteorder.h includes it to work out the byte order. Every Windows
/// target qgcoder builds for - x86, x86-64, ARM64 - is little-endian, so the
/// answer is known. Windows only.
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
