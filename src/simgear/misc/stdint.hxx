#ifndef _STDINT_HXX
#define _STDINT_HXX 1

// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
//
// Written by Curtis Olson - http://www.flightgear.org/~curt
// Started September 2001.
//
// This file is in the Public Domain, and comes with no warranty.
//


//////////////////////////////////////////////////////////////////////
//
//  There are many sick systems out there:
//
//  check for sizeof(float) and sizeof(double)
//  if sizeof(float) != 4 this code must be patched
//  if sizeof(double) != 8 this code must be patched
//
//  Those are comments I fetched out of glibc source:
//  - s390 is big-endian
//  - Sparc is big-endian, but v9 supports endian conversion
//    on loads/stores and GCC supports such a mode.  Be prepared.
//  - The MIPS architecture has selectable endianness.
//  - x86_64 is little-endian.
//  - CRIS is little-endian.
//  - m68k is big-endian.
//  - Alpha is little-endian.
//  - PowerPC can be little or big endian.
//  - SH is bi-endian but with a big-endian FPU.
//  - hppa1.1 big-endian.
//  - ARM is (usually) little-endian but with a big-endian FPU.
//
//////////////////////////////////////////////////////////////////////


#ifdef _MSC_VER
typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed __int64   int64_t;
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned __int64 uint64_t;

typedef int ssize_t;
#elif defined(sgi) || defined(__sun)
# include <sys/types.h>
#else
# include <stdint.h>
#endif


inline uint16_t sg_bswap_16(uint16_t x) {
    x = (x >> 8) | (x << 8);
    return x;
}

inline uint32_t sg_bswap_32(uint32_t x) {
    x = ((x >>  8) & 0x00FF00FFL) | ((x <<  8) & 0xFF00FF00L);
    x = (x >> 16) | (x << 16);
    return x;
}

inline uint64_t sg_bswap_64(uint64_t x) {
    x = ((x >>  8) & 0x00FF00FF00FF00FFLL) | ((x <<  8) & 0xFF00FF00FF00FF00LL);
    x = ((x >> 16) & 0x0000FFFF0000FFFFLL) | ((x << 16) & 0xFFFF0000FFFF0000LL);
    x = (x >> 32) | (x << 32);
    return x;
}


inline bool sgIsLittleEndian() {
    static const int sgEndianTest = 1;
    return (*((char *) &sgEndianTest ) != 0);
}

inline bool sgIsBigEndian() {
    static const int sgEndianTest = 1;
    return (*((char *) &sgEndianTest ) == 0);
}

inline void sgEndianSwap(uint16_t *x) { *x = sg_bswap_16(*x); }
inline void sgEndianSwap(uint32_t *x) { *x = sg_bswap_32(*x); }
inline void sgEndianSwap(uint64_t *x) { *x = sg_bswap_64(*x); }

#endif // !_STDINT_HXX
