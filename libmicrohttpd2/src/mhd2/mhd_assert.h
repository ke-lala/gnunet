/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2017-2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_assert.h
 * @brief  macros for mhd_assert()
 * @author Karlson2k (Evgeny Grin)
 */

/* Unlike POSIX version of 'assert.h', MHD version of 'assert' header
 * does not allow multiple redefinition of 'mhd_assert' macro within single
 * source file. */
#ifndef MHD_ASSERT_H
#define MHD_ASSERT_H 1

#include "mhd_sys_options.h"

#if ! defined(_DEBUG) && ! defined(NDEBUG)
#  ifndef DEBUG /* Used by some toolchains */
#    define NDEBUG 1 /* Use NDEBUG by default */
#  else  /* DEBUG */
#    define _DEBUG 1 /* Non-standart macro */
#  endif /* DEBUG */
#endif /* !_DEBUG && !NDEBUG */

#if defined(_DEBUG) && defined(NDEBUG)
#error Both _DEBUG and NDEBUG are defined
#endif /* _DEBUG && NDEBUG */

#ifdef NDEBUG
#  define mhd_assert(ignore) ((void) 0)
#else  /* ! NDEBUG */
#  ifdef HAVE_ASSERT
#    include <assert.h>
#    define mhd_assert(CHK) assert (CHK)
#  else  /* ! HAVE_ASSERT */
#    include <stdio.h>
#    ifdef HAVE_STDLIB_H
#      include <stdlib.h>
#    elif defined(HAVE_UNISTD_H)
#      include <unistd.h>
#    endif
#    ifdef MHD_HAVE_MHD_FUNC_
#      define mhd_assert(CHK) \
        do { \
          if (! (CHK)) { \
            fprintf (stderr, \
                     "%s:%s:%u Assertion failed: %s\nProgram aborted.\n", \
                     __FILE__, MHD_FUNC_, (unsigned) __LINE__, #CHK); \
            fflush (stderr); abort (); } \
        } while (0)
#    else
#      define mhd_assert(CHK) \
        do { \
          if (! (CHK)) { \
            fprintf (stderr, "%s:%u Assertion failed: %s\nProgram aborted.\n", \
                     __FILE__, (unsigned) __LINE__, #CHK); \
            fflush (stderr); abort (); } \
        } while (0)
#    endif
#  endif /* ! HAVE_ASSERT */
#endif /* NDEBUG */

#endif /* ! MHD_ASSERT_H */
