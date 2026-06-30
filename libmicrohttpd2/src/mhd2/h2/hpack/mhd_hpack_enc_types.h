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
 * @file src/mhd2/h2/hpack/mhd_hpack_enc_types.h
 * @brief  The definition of HPACK encoding context data
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_HPACK_ENC_TYPES_H
#define MHD_HPACK_ENC_TYPES_H 1

#include "mhd_sys_options.h"

#include "sys_sizet_type.h"

struct mhd_HpackDTblContext; /* forward declaration */

/**
 * HPACK encoder context
 */
struct mhd_HpackEncContext
{
  /**
   * Dynamic table data
   */
  struct mhd_HpackDTblContext *dyn;

  /**
   * The latest set dynamic table maximum size.
   * If it is different from the current size set in the @a dyn, then the
   * @a dyn is resized before adding new field.
   * Set initially to the default size of the dynamic table.
   */
  size_t dyn_size_new;

  /**
   * The smallest dynamic table size used after the last Dynamic Table Size
   * Update message.
   * If this value is different from the current size set in the @a dyn, then
   * the fields from dynamic table are evicted to specified size before adding
   * new fields.
   * Set initially to the default size of the dynamic table.
   */
  size_t dyn_size_smallest;

  /**
   * Last reported to peer size of the dynamic table.
   * If @a new_dyn_size or @a smallest_dyn_size are different then
   * Dynamic Table Size Update messages are added automatically before the
   * first encoded header.
   * Set initially to the default size of the dynamic table.
   */
  size_t dyn_size_peer;
};

#endif /* ! MHD_HPACK_ENC_TYPES_H */
