/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2021-2022 Karlson2k (Evgeny Grin)

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
 * @file src/mhd2/mhd_align.h
 * @brief  types alignment macros
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_ALIGN_H
#define MHD_ALIGN_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#if (defined(__GNUC__) && __GNUC__ < 4 && __GNUC_MINOR__ < 9 && \
  ! defined(__clang__)) || \
  (defined(__clang__) && __clang_major__ < 8) || \
  (defined(__clang__) && __clang_major__ < 11 && \
  defined(__apple_build_version__))
/* GCC before 4.9 and clang before 8.0 have incorrect implementation of 'alignof()'
   which returns preferred alignment instead of minimal required alignment */
#  define mhd_SYS_ALIGNOF_UNRELIABLE 1
#elif defined(_MSC_VER) && ! defined(__clang__) && _MSC_VER < 1900
/* MSVC has the same problem as old GCC versions:
   '__alignof()' may return "preferred" alignment instead of "required". */
#  define mhd_SYS_ALIGNOF_UNRELIABLE 1
#endif /* _MSC_VER < 1900 */

#ifndef mhd_ALIGNOF
#  ifndef mhd_SYS_ALIGNOF_UNRELIABLE
#    ifdef HAVE_C_ALIGNOF
#      if ! defined(__STDC_VERSION__) || (__STDC_VERSION__ + 0) < 202311
#        ifdef HAVE_STDALIGN_H
#          include <stdalign.h>
#        endif /* HAVE_STDALIGN_H */
#      endif
#      if defined(alignof) || defined(__alignof_is_defined) || \
  (defined(__STDC_VERSION__) && (__STDC_VERSION__ + 0) >= 202311)
#        define mhd_ALIGNOF(TYPE) alignof(TYPE)
#      endif
#    endif /* HAVE_C_ALIGNOF */

#    ifndef mhd_ALIGNOF
#      if defined(_MSC_VER) && ! defined(__clang__) && _MSC_VER >= 1700
#        define mhd_ALIGNOF(TYPE) __alignof (TYPE)
#      endif /* _MSC_VER >= 1700 */
#    endif /* !mhd_ALIGNOF */
#  endif
#endif

#ifndef mhd_ALIGNOF
#  include "sys_offsetof.h"
#  define mhd_ALIGNOF(TYPE) \
        offsetof (struct { char mhd__s; TYPE mhd__m; }, mhd__m)
#endif

/* Provide a limited set of alignment macros */
/* The set could be extended as needed */
#define mhd_UINT32_ALIGN mhd_ALIGNOF (uint_least32_t)
#define mhd_UINT64_ALIGN mhd_ALIGNOF (uint_least64_t)
#define mhd_UINT_FAST32_ALIGN mhd_ALIGNOF (uint_fast32_t)


#if defined(HAVE_ATTR_ALIGNED)
#  define mhd_ALIGNED(num)      __attribute__((aligned (num)))
#elif defined(HAVE_DECLSPEC_ALIGN)
#  define mhd_ALIGNED(num)      __declspec(align (num)))
#endif

#ifdef HAVE_C_ALIGNAS
#  if ! defined(__STDC_VERSION__) || (__STDC_VERSION__ + 0) < 202311
#    ifdef HAVE_STDALIGN_H
#      include <stdalign.h>
#    endif /* HAVE_STDALIGN_H */
#  endif /* before C23 */
#  if defined(alignas) || defined(__alignas_is_defined) || \
  (defined(__STDC_VERSION__) && (__STDC_VERSION__ + 0) >= 202311)
#    define mhd_ALIGNED_AS(type)      alignas (type)
#    ifndef mhd_ALIGNED
#      define mhd_ALIGNED(num)        alignas (num)
#    endif /* ! mhd_ALIGNED */
#  endif /* alignas || C23 or later */
#endif /* ! HAVE_C_ALIGNAS */

#ifndef mhd_ALIGNED
#  define mhd_ALIGNED(num)      /* Empty fallback */
#endif

#ifndef mhd_ALIGNED_AS
#  define mhd_ALIGNED_AS(type)          mhd_ALIGNED (mhd_ALIGNOF (type))
#endif

#if defined(MHD_ALIGNMENT_LARGE_NUM) && (MHD_ALIGNMENT_LARGE_NUM + 0) >= 8
#  define mhd_ALIGNED_8         mhd_ALIGNED (8)
#else
#  define mhd_ALIGNED_8         mhd_ALIGNED_AS (void*)
#endif

#if defined(MHD_ALIGNMENT_LARGE_NUM) && (MHD_ALIGNMENT_LARGE_NUM + 0) >= 16
#  define mhd_ALIGNED_16        mhd_ALIGNED (16)
#else
#  define mhd_ALIGNED_16        mhd_ALIGNED_8
#endif

#if defined(MHD_ALIGNMENT_LARGE_NUM) && (MHD_ALIGNMENT_LARGE_NUM + 0) >= 32
#  define mhd_ALIGNED_32        mhd_ALIGNED (32)
#else
#  define mhd_ALIGNED_32        mhd_ALIGNED_16
#endif

#if defined(MHD_ALIGNMENT_LARGE_NUM) && (MHD_ALIGNMENT_LARGE_NUM + 0) >= 64
#  define mhd_ALIGNED_64        mhd_ALIGNED (64)
#else
#  define mhd_ALIGNED_64        mhd_ALIGNED_32
#endif

#if defined(MHD_ALIGNMENT_LARGE_NUM) && (MHD_ALIGNMENT_LARGE_NUM + 0) >= 128
#  define mhd_ALIGNED_128       mhd_ALIGNED (128)
#else
#  define mhd_ALIGNED_128       mhd_ALIGNED_64
#endif

#if defined(MHD_ALIGNMENT_LARGE_NUM) && (MHD_ALIGNMENT_LARGE_NUM + 0) >= 256
#  define mhd_ALIGNED_256       mhd_ALIGNED (256)
#else
#  define mhd_ALIGNED_256       mhd_ALIGNED_128
#endif

#endif /* ! MHD_ALIGN_H */
