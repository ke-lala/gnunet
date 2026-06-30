/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/h2/h2_req_get_items.h
 * @brief  Declarations of HTTP/2 request items public getters
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_REQ_GET_ITEMS_H
#define MHD_H2_REQ_GET_ITEMS_H 1

#include "mhd_sys_options.h"

#include "sys_sizet_type.h"

#include "h2_req_item_kinds.h"

#include "mhd_public_api.h"


MHD_INTERNAL bool
mhd_h2_request_get_value_n (struct MHD_Request *restrict r,
                            enum MHD_ValueKind kind,
                            size_t key_len,
                            const char *restrict key,
                            struct MHD_StringNullable *restrict value_out)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (4,3)
MHD_FN_PAR_CSTR_ (4) MHD_FN_PAR_OUT_ (5);

MHD_INTERNAL size_t
mhd_h2_request_get_values_cb (struct MHD_Request *r,
                              enum MHD_ValueKind kind,
                              MHD_NameValueIterator iterator,
                              void *iterator_cls)
MHD_FN_PAR_NONNULL_ (1);

#endif /* ! MHD_H2_REQ_GET_ITEMS_H */
