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
 * @file src/mhd2/h2/h2_req_item_struct.h
 * @brief  Definition of the structure for request items (headers, URI params)
 * @author Karlson2k (Evgeny Grin)
 *
 * The sizes of all strings are intentionally limited to 32 bits (4GiB).
 */

#ifndef MHD_H2_REQ_ITEM_STRUCT_H
#define MHD_H2_REQ_ITEM_STRUCT_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#include "mhd_str_types.h"

#include "h2_req_item_kinds.h"

/**
 * HTTP/2 request item
 */
struct mhd_H2ReqItem
{
  /**
   * The kind of the item
   */
  enum mhd_H2RequestItemKind kind;
  /**
   * The offset of the name of the header in the buffer
   */
  uint_least32_t offset;
  /**
   * The length of the name of the header (not including mandatory
   * zero-termination).
   */
  uint_least32_t name_len;
  /**
   * The length of the name of the header (not including mandatory
   * zero-termination).
   * The value is located of @a offset + @a name_len + 1 position in the buffer.
   */
  uint_least32_t val_len;
};

#endif /* ! MHD_H2_REQ_ITEM_STRUCT_H */
