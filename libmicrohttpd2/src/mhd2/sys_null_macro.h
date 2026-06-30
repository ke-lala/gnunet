/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024-2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/sys_null_macro.h
 * @brief  The header for the system 'NULL' constant
 * @author Karlson2k (Evgeny Grin)
 *
 * This header tries to include the minimal header that defines 'NULL' macro.
 */

#ifndef MHD_SYS_NULL_MACRO_H
#define MHD_SYS_NULL_MACRO_H 1

#include "mhd_sys_options.h"

#include "sys_sizet_type.h" /* Should provide NULL */

#ifndef NULL
#  ifndef __cplusplus
#    if defined(__STDC_VERSION__) && ((__STDC_VERSION__ + 0) >= 202311)
#      define NULL      nullptr
#    else  /* < C23 */
#      define NULL      ((void *) 0)
#    endif /* < C23 */
#  else  /* __cplusplus */
#    if defined(__cpp_nullptr) && ((__cpp_nullptr + 0) >= 200704)
#      define NULL      nullptr
#    else
#      define NULL      0
#    endif
#  endif /* __cplusplus */
#endif

#endif /* ! MHD_SYS_NULL_MACRO_H */
