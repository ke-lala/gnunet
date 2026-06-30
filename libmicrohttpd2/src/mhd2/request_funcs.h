/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2022-2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/request_funcs.h
 * @brief  The declaration of the request internal functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_REQUEST_FUNCS_H
#define MHD_REQUEST_FUNCS_H 1

#include "mhd_sys_options.h"
#include "sys_bool_type.h"
#include "mhd_str_types.h"
#include "mhd_public_api.h"

struct MHD_Stream; /* forward declaration */

/**
 * Add field to the request.
 * The memory allocated in the request memory pool
 *
 * @param s the stream to use
 * @param kind the kind of the field to add
 * @param name the name of the field to add, the string is not copied,
 *             only copied the pointer value
 * @param value the value of the field to add, the string is not copied,
 *              only copied the pointer value
 * @return true if succeed,
 *         false if memory cannot be allocated
 */
MHD_INTERNAL bool
mhd_stream_add_field (struct MHD_Stream *restrict s,
                      enum MHD_ValueKind kind,
                      const struct MHD_String *restrict name,
                      const struct MHD_String *restrict value)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Add field to the request.
 * The memory allocated in the request memory pool
 * The value can have NULL string ("no value").
 *
 * @param s the stream to use
 * @param kind the kind of the field to add
 * @param name the name of the field to add, the string is not copied,
 *             only copied the pointer value
 * @param value the value of the field to add, the string is not copied,
 *              only copied the pointer value
 * @return true if succeed,
 *         false if memory cannot be allocated
 */
MHD_INTERNAL bool
mhd_stream_add_field_nullable (struct MHD_Stream *restrict s,
                               enum MHD_ValueKind kind,
                               const struct MHD_String *restrict name,
                               const struct MHD_StringNullable *restrict value)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_REQUEST_FUNCS_H */
