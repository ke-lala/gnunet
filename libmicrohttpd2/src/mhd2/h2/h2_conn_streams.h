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
 * @file src/mhd2/h2/h2_conn_streams.h
 * @brief  Declarations of HTTP/2 connection streams processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_CONN_STREAMS_H
#define MHD_H2_CONN_STREAMS_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

#include "h2_err_codes.h"

struct MHD_Connection;  /* forward declaration */
struct mhd_Buffer;      /* forward declaration */
struct mhd_H2Stream;    /* forward declaration */

enum mhd_H2RequestProblemType
{
  mhd_H2_REQ_PRBLM_FIELD_CHARS
  ,
  mhd_H2_REQ_PRBLM_FORBIDDEN_HEADER
  ,
  mhd_H2_REQ_PRBLM_CNTNT_LEN_WRONG
  ,
  mhd_H2_REQ_PRBLM_HOST_HDR_WRONG_EXTRA
  ,
  mhd_H2_REQ_PRBLM_HEADERS_NO_ENDSTREAM
  ,
  mhd_H2_REQ_PRBLM_PSEUDOHDR_IN_TRAILER
  ,
  mhd_H2_REQ_PRBLM_UNKNOWN_PSEUDOHDR
  ,
  mhd_H2_REQ_PRBLM_PSEUDOHDR_AFTER_HDR
  ,
  mhd_H2_REQ_PRBLM_PSEUDOHDR_MISSING
  ,
  mhd_H2_REQ_PRBLM_PSEUDOHDR_DUP
  ,
  mhd_H2_REQ_PRBLM_PSEUDOHDR_EXTRA
  ,
  mhd_H2_REQ_PRBLM_FLOW_CONTROL
  ,

  mhd_H2_REQ_PRBLM_HEADERS_TOO_LARGE
  ,
  mhd_H2_REQ_PRBLM_INT_ERROR
};

/**
 * Handle malformed request.
 * Send error reply, if possible; abort the request.
 * @param s the stream to handle
 * @param problem_type the type of the problem
 * @return always 'false'
 */
MHD_INTERNAL bool
mhd_h2_stream_req_problem (struct mhd_H2Stream *restrict s,
                           enum mhd_H2RequestProblemType problem_type)
MHD_FN_PAR_NONNULL_ALL_;


/**
 * Perform early abort of the stream, typically due to some error in processing.
 * @param s the stream to abort
 * @param err the error code to use
 * @return always 'false'
 */
MHD_INTERNAL bool
mhd_h2_stream_abort (struct mhd_H2Stream *restrict s,
                     enum mhd_H2ErrorCode err)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL bool
mhd_h2_conn_streamid_in_headers (struct MHD_Connection *restrict c,
                                 uint_least32_t stream_id,
                                 bool end_stream,
                                 bool end_headers,
                                 struct mhd_Buffer *restrict payload)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL bool
mhd_h2_conn_streamid_in_continuation (struct MHD_Connection *restrict c,
                                      uint_least32_t stream_id,
                                      bool end_headers,
                                      struct mhd_Buffer *payload)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL bool
mhd_h2_conn_streamid_in_data (struct MHD_Connection *restrict c,
                              uint_least32_t stream_id,
                              bool end_stream,
                              struct mhd_Buffer *restrict payload)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL bool
mhd_h2_conn_streamid_in_rst_stream (struct MHD_Connection *restrict c,
                                    uint_least32_t stream_id,
                                    enum mhd_H2ErrorCode err)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL bool
mhd_h2_conn_streamid_window_incr (struct MHD_Connection *restrict c,
                                  uint_least32_t stream_id,
                                  uint_least32_t incr)
MHD_FN_PAR_NONNULL_ALL_;

MHD_INTERNAL bool
mhd_h2_conn_streamid_abort (struct MHD_Connection *restrict c,
                            uint_least32_t stream_id,
                            enum mhd_H2ErrorCode err)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Process maintenance of all streams in the connection
 * @param c the connection to use
 * @return 'true' if all connections have been processed,
 *         'false' is output buffer has no space for required frame or
 *                 if connection is broken
 */
MHD_INTERNAL bool
mhd_h2_conn_maintain_streams_all  (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Process sending queue, send replies.
 * @param c the connection to use
 * @return 'true' if all connections have been processed,
 *         'false' is output buffer has no space for required frame or
 *                 if connection is broken
 */
MHD_INTERNAL bool
mhd_h2_conn_process_streams_sending_queue (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Force close of all connection's streams.
 * @param c the connection to use
 */
MHD_INTERNAL void
mhd_h2_conn_close_streams_all  (struct MHD_Connection *restrict c)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_H2_CONN_STREAMS_H */
