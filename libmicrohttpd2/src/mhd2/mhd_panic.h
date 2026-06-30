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
 * @file src/mhd2/mhd_panic.h
 * @brief  MHD_PANIC() macro and declarations of the related functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_PANIC_H
#define MHD_PANIC_H 1

#include "mhd_sys_options.h"

#ifndef BUILDING_MHD_LIB
/* Simplified implementation, utilised by unit tests that use some parts of
   the library code directly. */
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#elif defined(HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#define MHD_PANIC(msg) \
        do { fprintf (stderr,"Unrecoverable error: %s\n", msg); abort (); } \
        while (0)

#else  /* BUILDING_MHD_LIB */
/* Fully functional implementation for the library */

/**
 * Internal panic handler
 * @param file the name of the file where the panic was triggered
 * @param func the name of the function where the panic was triggered
 * @param line the number of the line where the panic was triggered
 * @param message the message with the description of the panic
 */
MHD_NORETURN_ MHD_INTERNAL void
mhd_panic (const char *file,
           const char *func,
           unsigned int line,
           const char *message);


#ifdef MHD_PANIC
#error MHD_PANIC macro is already defined. Check other headers.
#endif /* MHD_PANIC */

#ifdef MHD_SUPPORT_LOG_FUNCTIONALITY
#  ifdef MHD_HAVE_MHD_FUNC_
/**
 * Panic processing for unrecoverable errors.
 *
 * @param msg the error message string
 */
#    define MHD_PANIC(msg) \
        mhd_panic (__FILE__, MHD_FUNC_, __LINE__, msg)
#  else
#    include "sys_null_macro.h"
/**
 * Panic processing for unrecoverable errors.
 *
 * @param msg the error message string
 */
#    define MHD_PANIC(msg) \
        mhd_panic (__FILE__, NULL, __LINE__, msg)
#  endif
#else
#  include "sys_null_macro.h"
/**
 * Panic processing for unrecoverable errors.
 *
 * @param msg the error message string
 */
#    define MHD_PANIC(msg) \
        mhd_panic (NULL, NULL, __LINE__, NULL)
#endif

/**
 * Initialise panic handler to default value
 */
MHD_INTERNAL void
mhd_panic_init_default (void);

#endif /* BUILDING_MHD_LIB */

#endif /* ! MHD_PANIC_H */
