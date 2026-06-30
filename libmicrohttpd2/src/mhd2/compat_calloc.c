/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2014-2024 Karlson2k (Evgeny Grin)

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
 * @file src/mhd2/compat_calloc.c
 * @brief  The implementation of the calloc() replacement
 * @author Karlson2k (Evgeny Grin)
 */
#include "mhd_sys_options.h"

#include "compat_calloc.h"
#ifndef HAVE_CALLOC

#include <string.h> /* for memset() */
#include "sys_malloc.h"


#ifdef __has_builtin
#  if __has_builtin (__builtin_mul_overflow)
#    define MHD_HAVE_MUL_OVERFLOW 1
#  endif
#elif defined(__GNUC__) && __GNUC__ + 0 >= 5
#  define MHD_HAVE_MUL_OVERFLOW 1
#endif /* __GNUC__ >= 5 */

MHD_INTERNAL void *
mhd_calloc (size_t nelem, size_t elsize)
{
  size_t alloc_size;
  void *ptr;
#ifdef MHD_HAVE_MUL_OVERFLOW
  if (__builtin_mul_overflow (nelem, elsize, &alloc_size) || (0 == alloc_size))
    return NULL;
#else  /* ! MHD_HAVE_MUL_OVERFLOW */
  alloc_size = nelem * elsize;
  if ((0 == alloc_size) || (elsize != alloc_size / nelem))
    return NULL;
#endif /* ! MHD_HAVE_MUL_OVERFLOW */
  ptr = malloc (alloc_size);
  if (NULL == ptr)
    return NULL;
  memset (ptr, 0, alloc_size);
  return ptr;
}


#endif /* ! HAVE_CALLOC */
