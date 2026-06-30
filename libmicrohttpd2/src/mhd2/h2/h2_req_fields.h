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
 * @file src/mhd2/h2/h2_req_fields.h
 * @brief  Declaration of HTTP/2 request fields functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_REQ_FIELDS_H
#define MHD_H2_REQ_FIELDS_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_sizet_type.h"

struct mhd_Buffer;              /* Forward declaration */
struct mhd_HpackDecContext;     /* Forward declaration */
struct mhd_H2ReqItemsBlock;     /* Forward declaration */
struct mhd_H2Stream;            /* Forward declaration */

enum MHD_FIXED_ENUM_ mhd_H2DecFieldsResult
{
  /**
   * The data was successfully decoded.
   * Some incomplete data at the end of the block may be left unprocessed.
   */
  mhd_H2_DEC_FIELDS_OK
  ,
  /**
   * Not enough space to add the decoded field
   */
  mhd_H2_DEC_FIELDS_NO_SPACE
  ,
  /**
   * Internal error while decoding the data.
   * It could be memory allocation error when dynamic table is increasing
   * within the allowed limits.
   */
  mhd_H2_DEC_FIELDS_INT_ERR
  ,
  /**
   * The encoded data is incorrectly encoded or broken
   */
  mhd_H2_DEC_FIELDS_BROKEN_DATA
  ,
  /**
   * The data encoded in a way that excessively use resources or bandwidth
   */
  mhd_H2_DEC_FIELDS_PROT_ABUSE
};

MHD_INTERNAL enum mhd_H2DecFieldsResult
mhd_h2_req_fields_decode (struct mhd_HpackDecContext *restrict hk_dec,
                          const struct mhd_Buffer *restrict enc_data,
                          bool are_trailers,
                          struct mhd_H2ReqItemsBlock *restrict ib,
                          size_t *restrict left_unprocessed)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_(1) MHD_FN_PAR_IN_(2)
MHD_FN_PAR_INOUT_(4) MHD_FN_PAR_OUT_ (5);

MHD_INTERNAL bool
mhd_h2_req_headers_preprocess (struct mhd_H2Stream *restrict s)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

MHD_INTERNAL bool
mhd_h2_req_uri_parse (struct mhd_H2Stream *restrict s)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

MHD_INTERNAL bool
mhd_h2_req_cookie_parse (struct mhd_H2Stream *restrict s)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);


#endif /* ! MHD_H2_REQ_FIELDS_H */
