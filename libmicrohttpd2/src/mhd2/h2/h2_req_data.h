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
 * @file src/mhd2/h2/h2_req_data.h
 * @brief  Definition of HTTP/2 request data structure
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_REQ_DATA_H
#define MHD_H2_REQ_DATA_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "http_method.h"

struct MHD_Connection; /* Forward declaration */


enum MHD_FIXED_ENUM_ mhd_H2ReqStage
{
  /**
   * Headers are not completely received.
   * Processing the opening HEADERS frame or receiving and processing
   * CONTINUATION frames.
   */
  mhd_H2_REQ_STAGE_HEADERS_INCOMPLETE
  ,
  mhd_H2_REQ_STAGE_HEADERS_DECODING
  ,
  mhd_H2_REQ_STAGE_HEADERS_PROCESSING
  ,
  /**
   * Headers are completely received.
   * DATA frames or second HEADERS frame (containing trailers) can be received.
   */
  mhd_H2_REQ_STAGE_HEADERS_COMPLETE
  ,
  /**
   * Trailers are not completely received.
   * Processing the second HEADERS frame (which started trailers) or receiving
   * and processing CONTINUATION frames.
   */
  mhd_H2_REQ_STAGE_TRAILERS_INCOMPLETE
  ,
  mhd_H2_REQ_STAGE_TRAILERS_DECODING
  ,
  mhd_H2_REQ_STAGE_TRAILERS_PROCESSING
  ,
  /**
   * The client must not send any HEADERS or DATA frames.
   */
  mhd_H2_REQ_STAGE_END_STREAM
  ,
  /**
   * Any frames ignored with RST_STREAM.
   */
  mhd_H2_REQ_STAGE_BROKEN

};

struct mhd_H2RequestData
{
  /**
   * Always 'true'
   */
  bool is_http2;

  enum mhd_H2ReqStage stage;

  /**
   * 'true' when 'end stream' flag was received. The stage could be still
   * #mhd_H2_REQ_STAGE_HEADERS_INCOMPLETE or
   * #mhd_H2_REQ_STAGE_TRAILERS_INCOMPLETE as CONTINUATION frames are being
   * processed
   */
  bool got_end_stream;

  enum mhd_HTTP_Method method;

  uint_fast64_t cntn_size;

  /**
   * Position of ":method" pseudo-header in request items block.
   * Set to #mhd_H2_REQ_ITEM_POS_INVALID if not available.
   */
  size_t pos_method;
  /**
   * Position of ":path" pseudo-header in request items block.
   * Set to #mhd_H2_REQ_ITEM_POS_INVALID if not available.
   */
  size_t pos_path;
  /**
   * Position of ":authority" pseudo-header or "Host" header in request items
   * block.
   * Set to #mhd_H2_REQ_ITEM_POS_INVALID if not available.
   */
  size_t pos_authority;

  struct mhd_ApplicationAction app_act;

  /**
   * Set to 'true' when application gets any information about this
   * request or stream.
   */
  bool app_seen;

  void *app_context;
};

#endif /* ! MHD_H2_REQ_DATA_H */
