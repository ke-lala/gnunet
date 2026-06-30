/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_unreachable.h
 * @brief  The definition of the mhd_UNREACHABLE() macro
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_UNREACHABLE_H
#define MHD_UNREACHABLE_H 1

#include "mhd_sys_options.h"

#if ! defined(NDEBUG)
#  include "mhd_assert.h"
#elif defined (MHD_UNREACHABLE_NEEDS_STDDEF_H)
#  include <stddef.h>
#endif

/**
 * mhd_UNREACHABLE() should be used in locations where it is known in advance
 * that the code must be not reachable.
 * It should give compiler a hint to exclude some code paths from the final
 * binary.
 */
#ifdef NDEBUG
#  ifdef MHD_UNREACHABLE_KEYWORD
#    define mhd_UNREACHABLE()           MHD_UNREACHABLE_KEYWORD
#    define mhd_UNREACHABLE_D(descr)    MHD_UNREACHABLE_KEYWORD
#  else
#    define mhd_UNREACHABLE()           ((void) 0)
#    define mhd_UNREACHABLE_D(descr)    ((void) 0)
#  endif
#else
#  define mhd_UNREACHABLE()     \
        mhd_assert (0 && "This code should be unreachable")
#  define mhd_UNREACHABLE_D(descr) \
        mhd_assert (0 && descr)
#endif

#endif /* ! MHD_UNREACHABLE_H */
