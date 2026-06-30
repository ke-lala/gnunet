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
 * @file src/mhd2/h2/h2_stream_data.h
 * @brief  Definition of structure for HTTP/2 stream data
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_STREAM_DATA_H
#define MHD_H2_STREAM_DATA_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_dlinked_list.h"

#include "h2_err_codes.h"

#include "h2_req_data.h"

#define mhd_H2_REQ_ITEM_POS_INVALID     ((size_t) (~((size_t) 0u)))

#ifndef MHD_SIZE_UNKNOWN
#  define MHD_SIZE_UNKNOWN ((uint_least64_t) 0xFFFFFFFFFFFFFFFFu)
#endif /* MHD_SIZE_UNKNOWN */

struct MHD_Connection;  /* forward declaration */
struct MHD_Response;    /* forward declaration */

enum mhd_H2ReplyStage
{
  /**
   * Headers should be formed and sent
   */
  mhd_H2_RPL_STAGE_HEADERS_INCOMPLETE = 0,
  mhd_H2_RPL_STAGE_HEADERS_COMPLETE,
  mhd_H2_RPL_STAGE_TRAILERS_INCOMPLETE,
  mhd_H2_RPL_STAGE_END_STREAM,
  mhd_H2_RPL_STAGE_BROKEN
};

struct mhd_H2ReplyFieldsData
{
  bool auto_cntn_len;

  size_t num_sent;
};

struct mhd_H2ReplyData
{
  /**
   * Response to transmit (initially NULL).
   */
  struct MHD_Response *response;

  enum mhd_H2ReplyStage stage;

  struct mhd_H2ReplyFieldsData fields;

  uint_fast64_t cntn_read_pos;


  bool send_content;
};

struct mhd_H2StreamState
{
  int_least32_t send_window;

  int_least32_t recv_window;

  bool rcvd_rst_stream;
  enum mhd_H2ErrorCode peer_err;

  bool sent_rst_stream;
  enum mhd_H2ErrorCode mhd_err;
};

struct mhd_H2Stream; /* Forward declaration */

mhd_DLINKEDL_LINKS_DEF (mhd_H2Stream);

struct mhd_H2Stream
{
  /**
   * Must be always 'true'
   */
  bool is_h2;

  uint_least32_t stream_id;

  /**
   * The links in the container
   */
  mhd_DLNKDL_LINKS (mhd_H2Stream, streams);

  /**
   * The links in the sending list (when sending only)
   */
  mhd_DLNKDL_LINKS (mhd_H2Stream, send_q);

  /**
   * Pointer to the connection structure which is processing this stream
   */
  struct MHD_Connection *c;

  struct mhd_H2RequestData req;

  struct mhd_H2ReplyData rpl;

  struct mhd_H2StreamState state;
};

#endif /* ! MHD_H2_STREAM_DATA_H */
