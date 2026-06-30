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
 * @file src/mhd2/mhd_postfield_int.h
 * @brief  The definition of the internal struct mhd_PostFieldInt
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_POSTFIELD_INT_H
#define MHD_POSTFIELD_INT_H 1

#include "mhd_sys_options.h"

#include "sys_base_types.h"

/**
 * Position and length of a string in some buffer
 */
struct mhd_PositionAndLength
{
  /**
   * The position
   */
  size_t pos;
  /**
   * The length
   */
  size_t len;
};

struct mhd_PostFieldInt
{
  /**
   * The name of the field.
   * May start at zero position.
   */
  struct mhd_PositionAndLength name;
  /**
   * The field data
   * If not set or defined then position is zero.
   */
  struct mhd_PositionAndLength value;
  /**
   * The filename if provided (only for "multipart/form-data")
   * If not set or defined then position is zero.
   */
  struct mhd_PositionAndLength filename;
  /**
   * The Content-Type if provided (only for "multipart/form-data")
   * If not set or defined then position is zero.
   */
  struct mhd_PositionAndLength content_type;
  /**
   * The Transfer-Encoding if provided (only for "multipart/form-data")
   * If not set or defined then position is zero.
   */
  struct mhd_PositionAndLength transfer_encoding;
#if 0  // TODO: support processing in connection buffer
  /**
   * If 'true' then all strings are in the "large shared buffer".
   * If 'false' then all strings are in the stream buffer.
   */
  bool buf_is_lbuf;
#endif
};

#endif /* ! MHD_POSTFIELD_INT_H */
