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
 * @file src/mhd2/request_funcs.c
 * @brief  The definition of the request internal functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_cntnr_ptr.h"

#include "mhd_request.h"
#include "mhd_connection.h"

#include "stream_funcs.h"
#include "mhd_dlinked_list.h"

#include "request_funcs.h"


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_stream_add_field_nullable (struct MHD_Stream *restrict s,
                               enum MHD_ValueKind kind,
                               const struct MHD_String *restrict name,
                               const struct MHD_StringNullable *restrict value)
{
  struct mhd_RequestField *f;
  struct MHD_Connection *const c =
    mhd_CNTNR_PTR (s, struct MHD_Connection, h1_stream);

  f = (struct mhd_RequestField *)
      mhd_stream_alloc_memory (c, sizeof(struct mhd_RequestField));
  if (NULL == f)
    return false;

  f->field.nv.name = *name;
  f->field.nv.value = *value;
  f->field.kind = kind;
  mhd_DLINKEDL_INIT_LINKS (f, fields);

  mhd_DLINKEDL_INS_LAST (&(c->rq),f,fields);

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_stream_add_field (struct MHD_Stream *restrict s,
                      enum MHD_ValueKind kind,
                      const struct MHD_String *restrict name,
                      const struct MHD_String *restrict value)
{
  struct MHD_StringNullable value2;

  value2.len = value->len;
  value2.cstr = value->cstr;

  return mhd_stream_add_field_nullable (s, kind, name, &value2);
}
