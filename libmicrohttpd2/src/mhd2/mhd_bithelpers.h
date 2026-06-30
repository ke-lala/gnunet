/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2019-2025 Karlson2k (Evgeny Grin)

  GNU libmicrohttpd is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  GNU libmicrohttpd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Alternatively, you can redistribute GNU libmicrohttpd and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version, together
  with the eCos exception, as follows:

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, or you compile this
    file and link it with other works to produce a work based on this
    file, this file does not by itself cause the resulting work to be
    covered by the GNU General Public License. However the source code
    for this file must still be made available in accordance with
    section (3) of the GNU General Public License v2.

    This exception does not invalidate any other reasons why a work
    based on this file might be covered by the GNU General Public
    License.

  You should have received copies of the GNU Lesser General Public
  License and the GNU General Public License along with this library;
  if not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file src/mhd2/mhd_bithelpers.h
 * @brief  Bit manipulation helpers
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_BITHELPERS_H
#define MHD_BITHELPERS_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"
#include <string.h>

#include "mhd_assert.h"

#if defined(_MSC_FULL_VER)
/* Clang-cl produces a function call instead of intrinsics if optimisations
   are turned off. */
#  if (! defined(__clang__)) || defined(__OPTIMIZE__)
/* Declarations for VC & Clang-cl built-ins */
#    include <intrin.h>
#    define mhd_HAS_VC_INTRINSICS         1
#  endif /* (! __clang__) || (__OPTIMIZE__) */
#endif /* _MSC_FULL_VER  */
#include "mhd_byteorder.h"

#ifdef CHAR_BIT
#  if CHAR_BIT != 8
#error CHAR_BIT different from 8 is not supported
#  endif
#endif

#ifndef __has_builtin
#  define mhd_HAS_BUILTIN(x) (0)
#else
#  define mhd_HAS_BUILTIN(x) __has_builtin (x)
#endif

#if defined(_MSC_FULL_VER)
#pragma warning(push)
/* Disable C4505 "unreferenced local function has been removed" */
#pragma warning(disable:4505)
#endif /* _MSC_FULL_VER */

mhd_DATA_TRUNCATION_RUNTIME_CHECK_DISABLE

#if defined(MHD_HAVE___BUILTIN_BSWAP16) || \
  mhd_HAS_BUILTIN (__builtin_bswap16)
#  define mhd_BYTES_SWAP16(value16)  \
        ((uint16_t)__builtin_bswap16 ((uint16_t) value16))
#elif defined(mhd_HAS_VC_INTRINSICS)
#  ifndef __clang__
#    pragma intrinsic(_byteswap_ushort)
#  endif /* ! __clang__ */
#  define mhd_BYTES_SWAP16(value16)  \
        ((uint16_t)_byteswap_ushort ((uint16_t) value16))
#else  /* ! mhd_HAS_BUILTIN(__builtin_bswap32) */
mhd_static_inline uint16_t
mhd_BYTES_SWAP16 (uint16_t value16)
{
  return (uint16_t) ((value16 << 8u) | (value16 >> 8u));
}


#endif /* mhd_BYTES_SWAP16 */


#ifdef MHD_HAVE___BUILTIN_BSWAP32
#  define mhd_BYTES_SWAP32(value32)  \
        ((uint32_t) __builtin_bswap32 ((uint32_t) value32))
#elif defined(mhd_HAS_VC_INTRINSICS)
#  ifndef __clang__
#    pragma intrinsic(_byteswap_ulong)
#  endif /* ! __clang__ */
#  define mhd_BYTES_SWAP32(value32)  \
        ((uint32_t) _byteswap_ulong ((uint32_t) value32))
#elif \
  mhd_HAS_BUILTIN (__builtin_bswap32)
#  define mhd_BYTES_SWAP32(value32)  \
        ((uint32_t) __builtin_bswap32 ((uint32_t) value32))
#else  /* ! mhd_HAS_BUILTIN(__builtin_bswap32) */
mhd_static_inline uint32_t
mhd_BYTES_SWAP32 (uint32_t value32)
{
  uint32_t ret;
  ret  = (uint32_t) (value32 << 24u);
  ret |= (uint32_t) ((value32 << 8u) & (0x00FF0000u));
  ret |= (uint32_t) ((value32 >> 8u) & (0x0000FF00u));
  ret |= (uint32_t) (value32 >> 24u);
  return ret;
}


#endif /* ! mhd_HAS_BUILTIN(__builtin_bswap32) */


#ifdef MHD_HAVE___BUILTIN_BSWAP64
#  define mhd_BYTES_SWAP64(value64) \
        ((uint64_t) __builtin_bswap64 ((uint64_t) value64))
#elif defined(mhd_HAS_VC_INTRINSICS)
#  ifndef __clang__
#    pragma intrinsic(_byteswap_uint64)
#  endif /* ! __clang__ */
#  define mhd_BYTES_SWAP64(value64)  \
        ((uint64_t) _byteswap_uint64 ((uint64_t) value64))
#elif \
  mhd_HAS_BUILTIN (__builtin_bswap64)
#  define mhd_BYTES_SWAP64(value64) \
        ((uint64_t) __builtin_bswap64 ((uint64_t) value64))
#else  /* ! mhd_HAS_BUILTIN(__builtin_bswap64) */
mhd_static_inline uint64_t
mhd_BYTES_SWAP64(uint64_t value64)
{
  uint64_t ret;
  ret  = (uint64_t) (value64 << 56u);
  ret |= (uint64_t) ((value64 << 40u) & (0x00FF000000000000u));
  ret |= (uint64_t) ((value64 << 24u) & (0x0000FF0000000000u));
  ret |= (uint64_t) ((value64 <<  8u) & (0x000000FF00000000u));
  ret |= (uint64_t) ((value64 >>  8u) & (0x00000000FF000000u));
  ret |= (uint64_t) ((value64 >> 24u) & (0x0000000000FF0000u));
  ret |= (uint64_t) ((value64 >> 40u) & (0x000000000000FF00u));
  ret |= (uint64_t) (value64 >> 56u);
  return ret;
}
#endif /* ! mhd_HAS_BUILTIN(__builtin_bswap64) */

#if defined(__SIZEOF_INT128__) || \
  (defined(BITINT_MAXWIDTH) && (BITINT_MAXWIDTH >= 128))
/* swap 128-bit value if supported by compiler.
 * Parameter value128 must be unsigned 128-bit constant or variable, otherwise
 * macro will not work properly.
 * Warning: evaluate arguments multiple times!
 */
#  define mhd_BYTES_SWAP128(value128) \
        (((((value128) >>   0u) & 0xFFu) << 120u) | \
         ((((value128) >>   8u) & 0xFFu) << 112u) | \
         ((((value128) >>  16u) & 0xFFu) << 104u) | \
         ((((value128) >>  24u) & 0xFFu) <<  96u) | \
         ((((value128) >>  32u) & 0xFFu) <<  88u) | \
         ((((value128) >>  40u) & 0xFFu) <<  80u) | \
         ((((value128) >>  48u) & 0xFFu) <<  72u) | \
         ((((value128) >>  56u) & 0xFFu) <<  64u) | \
         ((((value128) >>  64u) & 0xFFu) <<  56u) | \
         ((((value128) >>  72u) & 0xFFu) <<  48u) | \
         ((((value128) >>  80u) & 0xFFu) <<  40u) | \
         ((((value128) >>  88u) & 0xFFu) <<  32u) | \
         ((((value128) >>  96u) & 0xFFu) <<  24u) | \
         ((((value128) >> 104u) & 0xFFu) <<  16u) | \
         ((((value128) >> 112u) & 0xFFu) <<   8u) | \
         ((((value128) >> 120u) & 0xFFu) <<   0u))
#endif

/* mhd_PUT_64BIT_LE (addr, value64)
 * put 64-bit value64 to addr in little-endian mode.
 */
/* Slow version that works with unaligned addr and with any byte order */
mhd_static_inline void
mhd_PUT_64BIT_LE_SLOW(void *addr, uint64_t value64)
{
  uint8_t *const dst = (uint8_t *) addr;
  dst[0] = (uint8_t) value64;
  dst[1] = (uint8_t) (value64 >> 8u);
  dst[2] = (uint8_t) (value64 >> 16u);
  dst[3] = (uint8_t) (value64 >> 24u);
  dst[4] = (uint8_t) (value64 >> 32u);
  dst[5] = (uint8_t) (value64 >> 40u);
  dst[6] = (uint8_t) (value64 >> 48u);
  dst[7] = (uint8_t) (value64 >> 56u);
}
#if mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_PUT_64BIT_LE(addr, value64)             \
        ((*(uint64_t*) (addr)) = (uint64_t) (value64))
#elif mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_PUT_64BIT_LE(addr, value64)             \
        ((*(uint64_t*) (addr)) = mhd_BYTES_SWAP64 (value64))
#else  /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
#  define mhd_PUT_64BIT_LE(addr, value64) \
        mhd_PUT_64BIT_LE_SLOW ((addr),(value64))
/* Indicate that mhd_PUT_64BIT_LE does not need aligned pointer */
#  define mhd_PUT_64BIT_LE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */

/* Put result safely to unaligned address */
#ifdef mhd_PUT_64BIT_LE_ALLOW_UNALIGNED
#  define mhd_PUT_64BIT_LE_UNALIGN(addr, value64)        \
        mhd_PUT_64BIT_LE ((addr),(value64))
#else  /* ! mhd_PUT_64BIT_LE_ALLOW_UNALIGNED */
#  define mhd_PUT_64BIT_LE_UNALIGN(addr, value64)           \
        do { uint64_t mhd__aligned_dst;                      \
             mhd_PUT_64BIT_LE (&mhd__aligned_dst, (value64)); \
             memcpy ((addr), &mhd__aligned_dst,               \
                     sizeof(mhd__aligned_dst)); } while (0)
#endif /* ! mhd_PUT_64BIT_LE_ALLOW_UNALIGNED */


/* mhd_PUT_32BIT_LE (addr, value32)
 * put 32-bit value32 to addr in little-endian mode.
 */
/* Slow version that works with unaligned addr and with any byte order */
mhd_static_inline void
mhd_PUT_32BIT_LE_SLOW(void *addr, uint32_t value32)
{
  uint8_t *const dst = (uint8_t *) addr;
  dst[0] = (uint8_t) value32;
  dst[1] = (uint8_t) (value32 >> 8u);
  dst[2] = (uint8_t) (value32 >> 16u);
  dst[3] = (uint8_t) (value32 >> 24u);
}
#if mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_PUT_32BIT_LE(addr,value32)             \
        ((*(uint32_t*) (addr)) = (uint32_t) (value32))
#elif mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_PUT_32BIT_LE(addr, value32)            \
        ((*(uint32_t*) (addr)) = mhd_BYTES_SWAP32 (value32))
#else  /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
#  define mhd_PUT_32BIT_LE(addr, value32) \
        mhd_PUT_32BIT_LE_SLOW ((addr),(value32))
/* Indicate that mhd_PUT_32BIT_LE does not need aligned pointer */
#  define mhd_PUT_32BIT_LE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */

/* Put result safely to unaligned address */
#ifdef mhd_PUT_32BIT_LE_ALLOW_UNALIGNED
#  define mhd_PUT_32BIT_LE_UNALIGN(addr, value32)        \
        mhd_PUT_32BIT_LE ((addr),(value32))
#else  /* ! mhd_PUT_32BIT_LE_ALLOW_UNALIGNED */
#  define mhd_PUT_32BIT_LE_UNALIGN(addr, value32)           \
        do { uint32_t mhd__aligned_dst;                      \
             mhd_PUT_32BIT_LE (&mhd__aligned_dst, (value32)); \
             memcpy ((addr), &mhd__aligned_dst,               \
                     sizeof(mhd__aligned_dst)); } while (0)
#endif /* ! mhd_PUT_32BIT_LE_ALLOW_UNALIGNED */


/* mhd_GET_32BIT_LE (addr)
 * get little-endian 32-bit value stored at addr
 * and return it in native-endian mode.
 */
/* Slow version that works with unaligned addr and with any byte order */
mhd_static_inline uint32_t
mhd_GET_32BIT_LE_SLOW(const void *addr)
{
  const uint8_t *const src = (const uint8_t *) addr;
  uint32_t ret;
  ret  = (uint32_t) src[0];
  ret |= (uint32_t) (((uint32_t) src[1]) << 8u);
  ret |= (uint32_t) (((uint32_t) src[2]) << 16u);
  ret |= (uint32_t) (((uint32_t) src[3]) << 24u);
  return ret;
}
#if mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_GET_32BIT_LE(addr)             \
        (*(const uint32_t*) (addr))
#elif mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_GET_32BIT_LE(addr)             \
        mhd_BYTES_SWAP32 (*(const uint32_t*) (addr))
#else  /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
#  define mhd_GET_32BIT_LE(addr) mhd_GET_32BIT_LE_SLOW ((addr))
/* Indicate that mhd_GET_32BIT_LE does not need aligned pointer */
#  define mhd_GET_32BIT_LE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */


#ifdef mhd_GET_32BIT_LE_ALLOW_UNALIGNED
#  define mhd_GET_32BIT_LE_UNALIGN(addr) mhd_GET_32BIT_LE (addr)
#else  /* ! mhd_GET_32BIT_LE_ALLOW_UNALIGNED */
/* Get value safely from an unaligned address */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ uint32_t
mhd_GET_32BIT_LE_UNALIGN (const void *addr)
{
  uint32_t aligned_src;
  memcpy (&aligned_src, addr, sizeof(aligned_src));
  return mhd_GET_32BIT_LE (&aligned_src);
}
#endif /* ! mhd_GET_32BIT_LE_ALLOW_UNALIGNED */


/* mhd_GET_64BIT_LE (addr)
 * get little-endian 64-bit value stored at addr
 * and return it in native-endian mode.
 */
/* Slow version that works with unaligned addr and with any byte order */
mhd_static_inline uint64_t
mhd_GET_64BIT_LE_SLOW (const void *addr)
{
  const uint8_t *const src = (const uint8_t *) addr;
  uint64_t ret;
  ret  = (uint64_t) src[0];
  ret |= (uint64_t) src[1] <<  8u;
  ret |= (uint64_t) src[2] << 16u;
  ret |= (uint64_t) src[3] << 24u;
  ret |= (uint64_t) src[4] << 32u;
  ret |= (uint64_t) src[5] << 40u;
  ret |= (uint64_t) src[6] << 48u;
  ret |= (uint64_t) src[7] << 56u;
  return ret;
}
#if mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_GET_64BIT_LE(addr)             \
        (*(const uint64_t*) (addr))
#elif mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_GET_64BIT_LE(addr)             \
        mhd_BYTES_SWAP64 (*(const uint64_t*) (addr))
#else  /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
#  define mhd_GET_64BIT_LE(addr)        mhd_GET_64BIT_LE_SLOW ((addr))
/* Indicate that mhd_GET_64BIT_LE does not need aligned pointer */
#  define mhd_GET_64BIT_LE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_BIG_ENDIAN */
#ifdef mhd_GET_64BIT_LE_ALLOW_UNALIGNED
#  define mhd_GET_64BIT_LE_UNALIGN(addr) mhd_GET_64BIT_LE (addr)
#else  /* !mhd_GET_64BIT_LE_ALLOW_UNALIGNED */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ uint64_t
mhd_GET_64BIT_LE_UNALIGN (const void *addr)
{
  uint64_t aligned_src;
  memcpy(&aligned_src, addr, sizeof(aligned_src));
  return mhd_GET_64BIT_LE(&aligned_src);
}
#endif


/* mhd_GET_UINTFAST32_LE(addr)
 * Get uint_fast32_t value at the addr as little endian.
 */
#if SIZEOF_UINT_FAST32_T == 4
#  define mhd_GET_UINTFAST32_LE(addr) \
        mhd_GET_32BIT_LE ((addr))
#elif SIZEOF_UINT_FAST32_T == 8
#  define mhd_GET_UINTFAST32_LE(addr) \
        mhd_GET_64BIT_LE ((addr))
#else /* future-proof */
mhd_static_inline uint_fast32_t
mhd_GET_UINTFAST32_LE(const void *addr)
{
  const uint8_t *const src = (const uint8_t *) addr;
  size_t i;
  uint_fast32_t ret = 0;
  for (i = 0; i < sizeof(ret); ++i)
    ret |= (uint_fast32_t) (((uint_fast32_t) src[i]) << (8u * i));
  return ret;
}
#endif


/* mhd_PUT_64BIT_BE (addr, value64)
 * put native-endian 64-bit value64 to addr
 * in big-endian mode.
 */
/* Slow version that works with unaligned addr and with any byte order */
mhd_static_inline void
mhd_PUT_64BIT_BE_SLOW(void *addr, uint64_t value64)
{
  uint8_t *const dst = (uint8_t *) addr;
  dst[0] = (uint8_t) (value64 >> 56u);
  dst[1] = (uint8_t) (value64 >> 48u);
  dst[2] = (uint8_t) (value64 >> 40u);
  dst[3] = (uint8_t) (value64 >> 32u);
  dst[4] = (uint8_t) (value64 >> 24u);
  dst[5] = (uint8_t) (value64 >> 16u);
  dst[6] = (uint8_t) (value64 >> 8u);
  dst[7] = (uint8_t) value64;
}
#if mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_PUT_64BIT_BE(addr, value64)             \
        ((*(uint64_t*) (addr)) = (uint64_t) (value64))
#elif mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_PUT_64BIT_BE(addr, value64)             \
        ((*(uint64_t*) (addr)) = mhd_BYTES_SWAP64 (value64))
#else  /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
#  define mhd_PUT_64BIT_BE(addr, value64) mhd_PUT_64BIT_BE_SLOW (addr, value64)
/* Indicate that mhd_PUT_64BIT_BE does not need aligned pointer */
#  define mhd_PUT_64BIT_BE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */

/* Put result safely to unaligned address */
#ifdef mhd_PUT_64BIT_BE_ALLOW_UNALIGNED
#  define mhd_PUT_64BIT_BE_UNALIGN(addr, value64)        \
        mhd_PUT_64BIT_BE ((addr),(value64))
#else  /* ! mhd_PUT_64BIT_BE_ALLOW_UNALIGNED */
#  define mhd_PUT_64BIT_BE_UNALIGN(addr, value64)           \
        do { uint64_t mhd__aligned_dst;                      \
             mhd_PUT_64BIT_BE (&mhd__aligned_dst, (value64)); \
             memcpy ((addr), &mhd__aligned_dst,               \
                     sizeof(mhd__aligned_dst)); } while (0)
#endif /* ! mhd_PUT_64BIT_BE_ALLOW_UNALIGNED */


/* mhd_GET_64BIT_BE (addr)
 * load 64-bit value located at addr in big endian mode.
 */
#if mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_GET_64BIT_BE(addr)             \
        (*(const uint64_t*) (addr))
#elif mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_GET_64BIT_BE(addr)             \
        mhd_BYTES_SWAP64 (*(const uint64_t*) (addr))
#else  /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
mhd_static_inline uint64_t
mhd_GET_64BIT_BE(const void *addr)
{
  const uint8_t *const src = (const uint8_t *) addr;
  uint64_t ret;
  ret  = (uint64_t) (((uint64_t) src[0]) << 56u);
  ret |= (uint64_t) (((uint64_t) src[1]) << 48u);
  ret |= (uint64_t) (((uint64_t) src[2]) << 40u);
  ret |= (uint64_t) (((uint64_t) src[3]) << 32u);
  ret |= (uint64_t) (((uint64_t) src[4]) << 24u);
  ret |= (uint64_t) (((uint64_t) src[5]) << 16u);
  ret |= (uint64_t) (((uint64_t) src[6]) << 8u);
  ret |= (uint64_t) src[7];
  return ret;
}
/* Indicate that mhd_GET_64BIT_BE does not need aligned pointer */
#  define mhd_GET_64BIT_BE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */


/* mhd_PUT_32BIT_BE (addr, value32)
 * put native-endian 32-bit value32 to addr
 * in big-endian mode.
 */
#if mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_PUT_32BIT_BE(addr, value32)             \
        ((*(uint32_t*) (addr)) = (uint32_t) (value32))
#elif mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_PUT_32BIT_BE(addr, value32)             \
        ((*(uint32_t*) (addr)) = mhd_BYTES_SWAP32 (value32))
#else  /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
mhd_static_inline void
mhd_PUT_32BIT_BE(void *addr, uint32_t value32)
{
  uint8_t *const dst = (uint8_t *) addr;
  dst[0] = (uint8_t) (value32 >> 24u);
  dst[1] = (uint8_t) (value32 >> 16u);
  dst[2] = (uint8_t) (value32 >> 8u);
  dst[3] = (uint8_t) value32;
}
/* Indicate that mhd_PUT_32BIT_BE does not need aligned pointer */
#  define mhd_PUT_32BIT_BE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */

/* Put result safely to unaligned address */
#ifdef mhd_PUT_32BIT_BE_ALLOW_UNALIGNED
#  define mhd_PUT_32BIT_BE_UNALIGN(addr, value32)        \
        mhd_PUT_32BIT_BE ((addr),(value32))
#else  /* ! mhd_PUT_32BIT_BE_ALLOW_UNALIGNED */
#  define mhd_PUT_32BIT_BE_UNALIGN(addr, value32)           \
        do { uint32_t mhd__aligned_dst;                      \
             mhd_PUT_32BIT_BE (&mhd__aligned_dst, (value32)); \
             memcpy ((addr), &mhd__aligned_dst,               \
                     sizeof(mhd__aligned_dst)); } while (0)
#endif /* ! mhd_PUT_32BIT_BE_ALLOW_UNALIGNED */


/* mhd_GET_32BIT_BE (addr)
 * get big-endian 32-bit value stored at addr
 * and return it in native-endian mode.
 */
#if mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_GET_32BIT_BE(addr)             \
        (*(const uint32_t*) (addr))
#elif mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_GET_32BIT_BE(addr)             \
        mhd_BYTES_SWAP32 (*(const uint32_t*) (addr))
#else  /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
mhd_static_inline uint32_t
mhd_GET_32BIT_BE(const void *addr)
{
  const uint8_t *const src = (const uint8_t *) addr;
  uint32_t ret;
  ret  = (uint32_t) (((uint32_t) src[0]) << 24u);
  ret |= (uint32_t) (((uint32_t) src[1]) << 16u);
  ret |= (uint32_t) (((uint32_t) src[2]) << 8u);
  ret |= (uint32_t) src[3];
  return ret;
}
/* Indicate that mhd_GET_32BIT_BE does not need aligned pointer */
#  define mhd_GET_32BIT_BE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */

#ifdef mhd_GET_32BIT_BE_ALLOW_UNALIGNED
#  define mhd_GET_32BIT_BE_UNALIGN(addr) mhd_GET_32BIT_BE ((addr))
#else  /* ! mhd_GET_32BIT_LE_ALLOW_UNALIGNED */
/* Get value safely from an unaligned address */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ uint32_t
mhd_GET_32BIT_BE_UNALIGN (const void *addr)
{
  uint32_t aligned_src;
  memcpy (&aligned_src,
          addr,
          sizeof(aligned_src));
  return mhd_GET_32BIT_BE (&aligned_src);
}
#endif /* ! mhd_GET_32BIT_BE_ALLOW_UNALIGNED */


/* mhd_PUT_UINTFAST32_BE(addr, uif32)
 * Put uint_fast32_t value to the addr in big endian mode.
 * Complete uint_fast32_t value is written, regardless the number of bits
 * in this uint_fast32_t (it can be more than 32 bits).
 */
#if SIZEOF_UINT_FAST32_T == 4
#  define mhd_PUT_UINTFAST32_BE(addr, uif32) \
        mhd_PUT_32BIT_BE ((addr),(uif32))
#elif SIZEOF_UINT_FAST32_T == 8
#  define mhd_PUT_UINTFAST32_BE(addr, uif32) \
        mhd_PUT_64BIT_BE ((addr),(uif32))
#else /* future-proof */
mhd_static_inline void
mhd_PUT_UINTFAST32_BE(void *addr, uint_fast32_t uif32)
{
  uint8_t *const dst = (uint8_t *) addr;
  size_t i;
  for (i = 0; i < sizeof(uif32); ++i)
    dst[i] = (uint8_t) (uif32 >> ((sizeof(uif32) - 1 - i) * 8));
}
#endif

/* mhd_GET_UINTFAST32_BE(addr)
 * Get uint_fast32_t value at the addr as big endian.
 */
#if SIZEOF_UINT_FAST32_T == 4
#  define mhd_GET_UINTFAST32_BE(addr) \
        mhd_GET_32BIT_BE ((addr))
#elif SIZEOF_UINT_FAST32_T == 8
#  define mhd_GET_UINTFAST32_BE(addr) \
        mhd_GET_64BIT_BE ((addr))
#else /* future-proof */
mhd_static_inline uint_fast32_t
mhd_GET_UINTFAST32_BE(const void *addr)
{
  size_t i;
  uint_fast32_t ret = 0;

  for (i = 0; i < sizeof(ret); ++i)
  {
    ret <<= 8u;
    ret |= (uint_fast32_t) (((const uint8_t*) addr)[i]);
  }
  return ret;
}
#endif


/* mhd_PUT_16BIT_BE (addr, value16)
 * put 16-bit value16 to addr
 * in big-endian mode.
 */
#if mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_PUT_16BIT_BE(addr, value16)             \
        ((*(uint16_t*) (addr)) = (uint16_t) (value16))
#elif mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_PUT_16BIT_BE(addr, value16)             \
        ((*(uint16_t*) (addr)) = mhd_BYTES_SWAP16 (value16))
#else  /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
mhd_static_inline void
mhd_PUT_16BIT_BE(void *addr, uint16_t value16)
{
  uint8_t *const dst = (uint8_t *) addr;
  dst[0] = (uint8_t) (value16 >> 8u);
  dst[1] = (uint8_t) (value16 >> 0u);
}
/* Indicate that mhd_PUT_16BIT_BE does not need aligned pointer */
#  define mhd_PUT_16BIT_BE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */

/* Put result safely to unaligned address */
#ifdef mhd_PUT_16BIT_BE_ALLOW_UNALIGNED
#  define mhd_PUT_16BIT_BE_UNALIGN(addr, value16)        \
        mhd_PUT_16BIT_BE ((addr),(value16))
#else  /* ! mhd_PUT_16BIT_BE_ALLOW_UNALIGNED */
#  define mhd_PUT_16BIT_BE_UNALIGN(addr, value16)           \
        do { uint16_t mhd__aligned_dst;                      \
             mhd_PUT_16BIT_BE (&mhd__aligned_dst, (value16)); \
             memcpy ((addr), &mhd__aligned_dst,               \
                     sizeof(mhd__aligned_dst)); } while (0)
#endif /* ! mhd_PUT_16BIT_BE_ALLOW_UNALIGNED */


/* mhd_GET_16BIT_BE (addr)
 * get big-endian 16-bit value stored at addr
 */
#if mhd_BYTE_ORDER == mhd_BIG_ENDIAN
#  define mhd_GET_16BIT_BE(addr)             \
        (*(const uint16_t*) (addr))
#elif mhd_BYTE_ORDER == mhd_LITTLE_ENDIAN
#  define mhd_GET_16BIT_BE(addr)             \
        mhd_BYTES_SWAP16 (*(const uint16_t*) (addr))
#else  /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */
/* Endianness was not detected or non-standard like PDP-endian */
mhd_static_inline uint16_t
mhd_GET_16BIT_BE(const void *addr)
{
  const uint8_t *const src = (const uint8_t *) addr;
  return (uint16_t) ((((uint16_t) src[0]) << 8u) | ((uint16_t) src[1]));
}
/* Indicate that mhd_GET_16BIT_BE does not need aligned pointer */
#  define mhd_GET_16BIT_BE_ALLOW_UNALIGNED 1
#endif /* mhd_BYTE_ORDER != mhd_LITTLE_ENDIAN */

#ifdef mhd_GET_16BIT_BE_ALLOW_UNALIGNED
#  define mhd_GET_16BIT_BE_UNALIGN(addr) mhd_GET_16BIT_BE ((addr))
#else  /* ! mhd_GET_16BIT_LE_ALLOW_UNALIGNED */
/* Get value safely from an unaligned address */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ uint16_t
mhd_GET_16BIT_BE_UNALIGN (const void *addr)
{
  uint16_t aligned_src;
  memcpy (&aligned_src,
          addr,
          sizeof(aligned_src));
  return mhd_GET_16BIT_BE (&aligned_src);
}
#endif /* ! mhd_GET_16BIT_BE_ALLOW_UNALIGNED */

/**
 * Rotate right 32-bit value by number of bits.
 */
#if defined(mhd_HAS_VC_INTRINSICS)
#  ifndef __clang__
#    pragma intrinsic(_rotr)
#  endif /* ! __clang__ */
#  define mhd_ROTR32(value32, bits) \
        ((uint32_t) _rotr ((uint32_t) (value32), (int) (bits)))
#elif mhd_HAS_BUILTIN (__builtin_stdc_rotate_right)
#  define mhd_ROTR32(value32, bits) \
        (__builtin_stdc_rotate_right ((uint32_t) (value32), (bits)))
#elif mhd_HAS_BUILTIN (__builtin_rotateright32)
#  define mhd_ROTR32(value32, bits) \
        ((uint32_t) __builtin_rotateright32 ((value32), (bits)))
#else  /* ! __builtin_rotateright32 */
mhd_static_inline uint32_t
mhd_ROTR32 (uint32_t value32, unsigned int bits)
{
  bits &= 31u;
  return (value32 >> bits) | (value32 << ((32u - bits) & 31u));
}


#endif /* ! __builtin_rotateright32 */


/**
 * Rotate left 32-bit value by number of bits.
 */
#if defined(mhd_HAS_VC_INTRINSICS)
#  ifndef __clang__
#    pragma intrinsic(_rotl)
#  endif /* ! __clang__ */
#  define mhd_ROTL32(value32, bits) \
        ((uint32_t) _rotl ((uint32_t) (value32), (int) (bits)))
#elif mhd_HAS_BUILTIN (__builtin_stdc_rotate_left)
#  define mhd_ROTL32(value32, bits) \
        (__builtin_stdc_rotate_left ((uint32_t) (value32), (bits)))
#elif mhd_HAS_BUILTIN (__builtin_rotateleft32)
#  define mhd_ROTL32(value32, bits) \
        ((uint32_t) __builtin_rotateleft32 ((value32), (bits)))
#else  /* ! __builtin_rotateleft32 */
mhd_static_inline uint32_t
mhd_ROTL32 (uint32_t value32, unsigned int bits)
{
  bits &= 31u;
  return (value32 << bits) | (value32 >> ((32u - bits) & 31u));
}


#endif /* ! __builtin_rotateleft32 */


/**
 * Rotate right 64-bit value by number of bits.
 */
#if defined(mhd_HAS_VC_INTRINSICS)
#  ifndef __clang__
#    pragma intrinsic(_rotr64)
#  endif /* ! __clang__ */
#  define mhd_ROTR64(value64, bits) \
        ((uint64_t) _rotr64 ((uint64_t) (value64), (int) (bits)))
#elif mhd_HAS_BUILTIN (__builtin_stdc_rotate_right)
#  define mhd_ROTR64(value64, bits) \
        (__builtin_stdc_rotate_right ((uint64_t) (value64), (bits)))
#elif mhd_HAS_BUILTIN (__builtin_rotateright64)
#  define mhd_ROTR64(value64, bits) \
        ((uint64_t) __builtin_rotateright64 ((value64), (bits)))
#else  /* ! __builtin_rotateright64 */
mhd_static_inline uint64_t
mhd_ROTR64 (uint64_t value64, unsigned int bits)
{
  bits &= 63u;
  return (value64 >> bits) | (value64 << ((64u - bits) & 63u));
}


#endif /* ! __builtin_rotateright64 */


/**
 * @def mhd_LEADING_ZEROS32NZ
 * Count leading (most-significant) zero bits in a non-zero 32-bit value.
 * The result is undefined if the argument is zero or does not fit in 32 bits.
 */
#if defined(MHD_HAVE___BUILTIN_CLZ) && 4 == SIZEOF_UNSIGNED_INT
#  define mhd_LEADING_ZEROS32NZ(val32) \
        ((uint_least8_t) __builtin_clz ((unsigned int) (val32)))
#elif defined(MHD_HAVE___BUILTIN_CLZL) && 4 == SIZEOF_UNSIGNED_LONG
#  define mhd_LEADING_ZEROS32NZ(val32) \
        ((uint_least8_t) __builtin_clzl ((unsigned long) (val32)))
#elif defined(MHD_HAVE___BUILTIN_CLZG1) && 4 <= SIZEOF_UINT_LEAST32_T
#  if 4 == SIZEOF_UINT_LEAST32_T
#    define mhd_LEADING_ZEROS32NZ(val32) \
        ((uint_least8_t) __builtin_clzg ((uint_least32_t) (val32)))
#  else  /* 4 < SIZEOF_UINT_LEAST32_T */
#    define mhd_LEADING_ZEROS32NZ(val32) \
        ((uint_least8_t) (__builtin_clzg ((uint_least32_t) (val32)) \
                          - ((sizeof(uint_least32_t) - 4u) * 8u)))
#  endif /* 4 < SIZEOF_UINT_LEAST32_T */
#endif /* MHD_HAVE___BUILTIN_CLZG1 && 4 <= SIZEOF_UINT_LEAST32_T */


/**
 * @def mhd_BIT_WIDTH32NZ
 * Return the smallest number of bits needed to represent the value.
 * The result is undefined if the argument is zero or does not fit in 32 bits.
 */
#if defined(mhd_HAS_VC_INTRINSICS) && 4 == SIZEOF_UNSIGNED_LONG && \
  (defined(_M_X64) || defined(_M_IX86) \
  || defined(_M_ARM) || defined(_M_ARM64) \
  || defined(__i386__) || defined(__x86_64__) \
  || defined(__arm__) || defined(__aarch64__))
#  ifndef __clang__
#    pragma intrinsic(_BitScanReverse)
#  endif /* ! __clang__ */
mhd_static_inline uint_least8_t
mhd_bh_func_bit_width32nz(uint_least32_t val32)
{
  unsigned long idx;
  (void) _BitScanReverse(&idx, (unsigned long) val32);
  return (uint_least8_t) (idx + 1u);
}
#  define mhd_BIT_WIDTH32NZ(val32)  mhd_bh_func_bit_width32nz ((val32))
#endif /* mhd_HAS_VC_INTRINSICS && 4 == SIZEOF_UNSIGNED_LONG && (x86 || ARM) */


/**
 * @def mhd_LEADING_ZEROS32
 * Count leading (most-significant) zero bits in a 32-bit value.
 * If the argument is zero then 32 is returned.
 * The result is undefined if the argument does not fit in 32 bits.
 */

#if defined(MHD_HAVE___BUILTIN_CLZG2) && 4 <= SIZEOF_UINT_LEAST32_T
#  if 4 == SIZEOF_UINT_LEAST32_T
#    define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) __builtin_clzg ((uint_least32_t) (val32),32u))
#  else  /* 4 < SIZEOF_UINT_LEAST32_T */
#    define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) (__builtin_clzg ((uint_least32_t) (val32), \
                                          sizeof(uint_least32_t) * 8u) \
                          - ((sizeof(uint_least32_t) - 4u) * 8u)))
#  endif /* 4 < SIZEOF_UINT_LEAST32_T */
#elif defined(mhd_HAS_VC_INTRINSICS) && 4 == SIZEOF_UNSIGNED_LONG && \
  (defined(_M_ARM) || defined(_M_ARM64) \
  || defined(__arm__) || defined(__aarch64__)) && \
  ( (! defined(__clang__)) \
  || (((__clang_major__ + 0) >= 18) && defined(__aarch64__)) )
/* Support for _CountLeadingZeros() was added only in clang 18 */
#  ifndef __clang__
#    pragma intrinsic(_CountLeadingZeros)
#  endif /* ! __clang__ */
#  define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) _CountLeadingZeros ((unsigned long) (val32)))
#elif mhd_HAS_BUILTIN (__builtin_stdc_leading_zeros)
#  if 4 == SIZEOF_UINT_LEAST32_T
#    define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) \
         __builtin_stdc_leading_zeros ((uint_least32_t) (val32)))
#  else  /* 4 < SIZEOF_UINT_LEAST32_T */
#    define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) \
         (__builtin_stdc_leading_zeros ((uint_least32_t) (val32)) \
          - ((sizeof(uint_least32_t) - 4u) * 8u)))
#  endif /* 4 < SIZEOF_UINT_LEAST32_T */
#endif /* __builtin_stdc_leading_zeros */


/**
 * @def mhd_BIT_WIDTH32
 * Return the smallest number of bits needed to represent the value.
 * If the argument is zero then zero is returned.
 * The result is undefined if the argument does not fit in 32 bits.
 */
#if defined(mhd_HAS_VC_INTRINSICS) && 4 == SIZEOF_UNSIGNED_LONG && \
  (defined(_M_X64) || defined(_M_IX86) \
  || defined(_M_ARM) || defined(_M_ARM64) \
  || defined(__i386__) || defined(__x86_64__) \
  || defined(__arm__) || defined(__aarch64__))
#  ifndef __clang__
#    pragma intrinsic(_BitScanReverse)
#  endif /* ! __clang__ */
mhd_static_inline uint_least8_t
mhd_bh_func_bit_width32(uint_least32_t val32)
{
  unsigned long idx;
  if (0 == _BitScanReverse(&idx, (unsigned long) val32))
    return 0u;
  return (uint_least8_t) (idx + 1u);
}
#  define mhd_BIT_WIDTH32(val32)  mhd_bh_func_bit_width32 ((val32))
#elif mhd_HAS_BUILTIN (__builtin_stdc_bit_width)
#  define mhd_BIT_WIDTH32(val32) \
        ((uint_least8_t) __builtin_stdc_bit_width ((uint_least32_t) (val32)))
#endif /* __builtin_stdc_bit_width */


/* ** Use compiler-optimised implementation for missing functionality ** */
#ifndef mhd_LEADING_ZEROS32NZ
#  ifdef mhd_BIT_WIDTH32NZ
#    define mhd_LEADING_ZEROS32NZ(val32) \
        ((uint_least8_t) (32u - mhd_BIT_WIDTH32NZ ((val32))))
#  endif /* mhd_BIT_WIDTH32NZ */
#endif /* ! mhd_LEADING_ZEROS32NZ */

#ifndef mhd_BIT_WIDTH32NZ
#  ifdef mhd_LEADING_ZEROS32NZ
#    define mhd_BIT_WIDTH32NZ(val32) \
        ((uint_least8_t) (32u - mhd_LEADING_ZEROS32NZ ((val32))))
#  endif /* mhd_LEADING_ZEROS32NZ */
#endif /* ! mhd_BIT_WIDTH32NZ */

#ifndef mhd_LEADING_ZEROS32
#  ifdef mhd_BIT_WIDTH32
#    define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) (32u - mhd_BIT_WIDTH32 ((val32))))
#  endif /* mhd_BIT_WIDTH32 */
#endif /* ! mhd_LEADING_ZEROS32 */

#ifndef mhd_BIT_WIDTH32
#  ifdef mhd_LEADING_ZEROS32
#    define mhd_BIT_WIDTH32(val32) \
        ((uint_least8_t) (32u - mhd_LEADING_ZEROS32 ((val32))))
#  endif /* mhd_LEADING_ZEROS32 */
#endif /* ! mhd_BIT_WIDTH32 */

#if ! defined(mhd_LEADING_ZEROS32NZ)
#  ifdef mhd_LEADING_ZEROS32
#    define mhd_LEADING_ZEROS32NZ(val32)        mhd_LEADING_ZEROS32 ((val32))
#    define mhd_BIT_WIDTH32NZ(val32)            mhd_BIT_WIDTH32 ((val32))
#  endif /* mhd_LEADING_ZEROS32 */
#else   /* mhd_LEADING_ZEROS32NZ */
#  if ! defined(mhd_LEADING_ZEROS32)

mhd_static_inline uint_least8_t
mhd_bh_func_leading_zeros32(uint_least32_t val32)
{
  if (0u == val32)
    return 32u;
  return mhd_LEADING_ZEROS32NZ(val32);
}

mhd_static_inline uint_least8_t
mhd_bh_func_bit_width32(uint_least32_t val32)
{
  if (0u == val32)
    return 0u;
  return mhd_BIT_WIDTH32NZ(val32);
}

#    define mhd_LEADING_ZEROS32(val32)  mhd_bh_func_leading_zeros32 ((val32))
#    define mhd_BIT_WIDTH32(val32)      mhd_bh_func_bit_width32 ((val32))
#  endif /* ! mhd_LEADING_ZEROS32 */
#endif /* mhd_LEADING_ZEROS32NZ */

#if defined(mhd_LEADING_ZEROS32NZ) || defined(mhd_BIT_WIDTH32NZ) \
  || defined(mhd_LEADING_ZEROS32) || defined(mhd_BIT_WIDTH32)
/* If at least one compiler-optimised function is detected, all macros must be
   defined as all of them can be based on a single base function. */
#  if ! defined(mhd_LEADING_ZEROS32NZ)
#error mhd_LEADING_ZEROS32NZ() must be defined
#  endif
#  if ! defined(mhd_BIT_WIDTH32NZ)
#error mhd_BIT_WIDTH32NZ() must be defined
#  endif
#  if ! defined(mhd_LEADING_ZEROS32)
#error mhd_LEADING_ZEROS32() must be defined
#  endif
#  if ! defined(mhd_BIT_WIDTH32)
#error mhd_BIT_WIDTH32() must be defined
#  endif
#else
/* No compiler-optimised base version. Use fallback implementation. */


mhd_static_inline uint_least8_t
mhd_bh_func_bit_width32(uint_least32_t val32)
{
  uint_fast8_t cal_width = 0u;
  uint_fast8_t check_bits;
  uint_fast32_t val_left = (uint_fast32_t) (val32 & 0xFFFFFFFFu);
  mhd_assert (val32 == val_left);

  /* Branchless code without any tables.
     Should have a good performance even with a cold cache. */
  check_bits = (uint_fast8_t) ((0 != (val_left >> 16u)) * 16u);
  cal_width += check_bits;
  val_left >>= check_bits;

  check_bits = (uint_fast8_t) ((0 != (val_left >> 8u)) * 8u);
  cal_width += check_bits;
  val_left >>= check_bits;

  check_bits = (uint_fast8_t) ((0 != (val_left >> 4u)) * 4u);
  cal_width += check_bits;
  val_left >>= check_bits;

  check_bits = (uint_fast8_t) ((0 != (val_left >> 2u)) * 2u);
  cal_width += check_bits;
  val_left >>= check_bits;

  check_bits = (uint_fast8_t) ((0 != (val_left >> 1u)) * 1u);
  cal_width += check_bits;
  val_left >>= check_bits;

  return (uint_least8_t) (cal_width + val_left);
}

#  define mhd_LEADING_ZEROS32NZ(val32) \
        ((uint_least8_t) \
         (32u - mhd_bh_func_bit_width32 ((uint_least32_t) (val32))))
#  define mhd_BIT_WIDTH32NZ(val32)      mhd_bh_func_bit_width32 ((val32))
#  define mhd_LEADING_ZEROS32(val32) \
        ((uint_least8_t) \
         (32u - mhd_bh_func_bit_width32 ((uint_least32_t) (val32))))
#  define mhd_BIT_WIDTH32(val32)        mhd_bh_func_bit_width32 ((val32))
#endif

/**
 * @def mhd_LEADING_ONES32
 * Count leading (most-significant) ones in a 32-bit value.
 * The argument is always treated as a 32-bit value; any higher-order bits
 * (if present) are ignored.
 * @note Unlike other related macros, this one explicitly trims (or extends)
 *       the argument to 32 bits. Do not use signed types or narrower types
 *       as argument as they may produce unexpected results.
 */
#if mhd_HAS_BUILTIN (__builtin_stdc_leading_ones) && \
  4 == SIZEOF_UINT_LEAST32_T
#  define mhd_LEADING_ONES32(val32) \
        ((uint_least8_t) __builtin_stdc_leading_ones ((uint_least32_t) (val32)))
#else  /* ! __builtin_stdc_leading_ones || 4 != SIZEOF_UINT_LEAST32_T */
#  define mhd_LEADING_ONES32(val32)          \
        mhd_LEADING_ZEROS32 ((uint_least32_t) \
                             (0xFFFFFFFFu & ~((uint_least32_t) (val32))))
#endif /* ! __builtin_stdc_leading_ones || 4 != SIZEOF_UINT_LEAST32_T */


mhd_DATA_TRUNCATION_RUNTIME_CHECK_RESTORE

#if defined(_MSC_FULL_VER)
/* Restore warnings */
#pragma warning(pop)
#endif /* _MSC_FULL_VER */

#endif /* ! MHD_BITHELPERS_H */
