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
 * @file src/mhd2/h2/h2_proc_in.c
 * @brief  Implementation of HTTP/2 connection incoming data processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_buffer.h"
#include "mhd_connection.h"

#include <string.h>

#include "h2_frame_types.h"
#include "h2_frame_codec.h"

#include "h2_conn_streams.h"
#include "h2_proc_settings.h"
#include "h2_proc_out.h"
#include "h2_proc_conn.h"

#include "h2_proc_in.h"


enum mhd_H2ProcInFrameResult
{
  mhd_H2_PROC_IN_FRAME_CONTINUE = mhd_H2_STNGS_PROC_OK,
  mhd_H2_PROC_IN_FRAME_BLOCKED_BY_OUT = mhd_H2_STNGS_PROC_NO_OUT_BUFF,
  mhd_H2_PROC_IN_FRAME_CONN_BROKEN = mhd_H2_STNGS_PROC_STNGS_ERR
};

/**
 * Process incoming frame
 * @param c the connection to use
 * @param h2frame the frame information
 * @param payload the frame payload (excluding known extra headers)
 * @return 'true' if frame was successfully and completely processed,
 *         'false' if frame should be processed again later or if connection
 *         if in closing or broken state (and no incoming frames should be
 *         processed anymore)
 */
static MHD_FN_PAR_NONNULL_ALL_ bool
process_inc_frame (struct MHD_Connection *restrict c,
                   const union mhd_H2FrameUnion *h2frame,
                   struct mhd_Buffer *payload)
{
  if (0u != c->h2.state.continuation_stream_id)
  {
    if ((mhd_H2_FRAME_IDS_CONTINUATION_ID != h2frame->selector.type)
        || (c->h2.state.continuation_stream_id !=
            h2frame->continuation.stream_id))
      return ! mhd_h2_conn_finish (c,
                                   mhd_H2_ERR_PROTOCOL_ERROR,
                                   false);
  }
  else
    mhd_assert (0u == c->h2.buff.unproc_hdrs_size);

  switch (h2frame->selector.type)
  {
  case mhd_H2_FRAME_IDS_DATA_ID:
    mhd_assert (0);
    return mhd_h2_conn_finish (c,
                               mhd_H2_ERR_PROTOCOL_ERROR,
                               false);
  case mhd_H2_FRAME_IDS_HEADERS_ID:
    return mhd_h2_conn_streamid_in_headers (c,
                                            h2frame->headers.stream_id,
                                            h2frame->headers.end_stream,
                                            h2frame->headers.end_headers,
                                            payload);
  case mhd_H2_FRAME_IDS_RST_STREAM_ID:
    return mhd_h2_conn_streamid_in_rst_stream (c,
                                               h2frame->rst_stream.stream_id,
                                               h2frame->rst_stream.error_code);
    return mhd_H2_PROC_IN_FRAME_CONTINUE;
  case mhd_H2_FRAME_IDS_SETTINGS_ID:
    return (mhd_H2_STNGS_PROC_OK == mhd_h2_proc_new_settings (c,
                                                              payload));
  case mhd_H2_FRAME_IDS_PUSH_PROMISE_ID:
    return ! mhd_h2_conn_finish (c,
                                 mhd_H2_ERR_PROTOCOL_ERROR,
                                 false);
  case mhd_H2_FRAME_IDS_PING_ID:
    return mhd_h2_q_ping (c,
                          true,
                          h2frame->ping.opaque_data);
  case mhd_H2_FRAME_IDS_GOAWAY_ID:
    return mhd_h2_conn_process_in_goaway (c,
                                          h2frame->goaway.last_stream_id,
                                          h2frame->goaway.error_code);
  case mhd_H2_FRAME_IDS_WINDOW_UPDATE_ID:
    if (0u != h2frame->window_update.stream_id)
    {
      return
        mhd_h2_conn_streamid_window_incr (
        c,
        h2frame->window_update.stream_id,
        h2frame->window_update.window_size_increment);
    }
    if ((h2frame->window_update.window_size_increment
         + (uint_least32_t) c->h2.state.send_window) > 0x7FFFFFFFu)
      return ! mhd_h2_conn_finish (c,
                                   mhd_H2_ERR_FLOW_CONTROL_ERROR,
                                   false);
    c->h2.state.send_window +=
      (int_least32_t) h2frame->window_update.window_size_increment;
    return true;
  case mhd_H2_FRAME_IDS_CONTINUATION_ID:
    return
      mhd_h2_conn_streamid_in_continuation (c,
                                            h2frame->continuation.stream_id,
                                            h2frame->continuation.end_headers,
                                            payload);
  case mhd_H2_FRAME_IDS_PRIORITY_ID:
  default:
    break; /* Ignored */
  }
  /* Ignored types of frame */
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_process_in_data (struct MHD_Connection *restrict c)
{
  mhd_assert (mhd_HTTP_LAYER_CONNECTED == c->h_layer.state);

  mhd_assert (c->read_buffer_size >= c->read_buffer_offset);
  mhd_assert (c->read_buffer_offset >= c->h2.buff.r_cur_frame);

  if (c->h2.buff.r_cur_frame == c->read_buffer_offset)
    return true; /* Shortcut, nothing to process */

  do
  {
    union mhd_H2FrameUnion h2frame;
    enum mhd_H2FrameDecodeResult dec_res;
    struct mhd_Buffer payload;
    bool proc_next;
    uint8_t *const frame_buff =
      (uint8_t *) c->read_buffer + c->h2.buff.r_cur_frame;
    const size_t buff_left = c->read_buffer_offset - c->h2.buff.r_cur_frame;

    proc_next = true;
    dec_res = mhd_h2_frame_decode (buff_left,
                                   frame_buff,
                                   c->h2.rcv_cfg.max_frame_size,
                                   &h2frame,
                                   &payload);
    switch (dec_res)
    {
    case mhd_H2_F_DEC_OK:
      proc_next = process_inc_frame (c,
                                     &h2frame,
                                     &payload);
      proc_next = (proc_next && (mhd_HTTP_LAYER_CONNECTED == c->h_layer.state));
      if (c->h2.state.top_seen_stream_id < h2frame.selector.stream_id)
        c->h2.state.top_seen_stream_id = h2frame.selector.stream_id;
      break;
    case mhd_H2_F_DEC_F_PAYLOAD_INCOMPLETE:
    case mhd_H2_F_DEC_F_HEADER_INCOMPLETE:
      proc_next = false;
      break;
    case mhd_H2_F_DEC_STREAM_ERR_F_SIZE:
    case mhd_H2_F_DEC_STREAM_ERR_PROT:
      if (mhd_h2_frame_get_total_size (&h2frame) > buff_left)
      {
        /* The frame is not yet complete */
        proc_next = false;
        break;
      }
      proc_next =
        mhd_h2_conn_streamid_abort (
          c,
          h2frame.selector.stream_id,
          (mhd_H2_F_DEC_STREAM_ERR_F_SIZE == dec_res) ?
          mhd_H2_ERR_FRAME_SIZE_ERROR : mhd_H2_ERR_PROTOCOL_ERROR);
      if (c->h2.state.top_seen_stream_id < h2frame.selector.stream_id)
        c->h2.state.top_seen_stream_id = h2frame.selector.stream_id;
      break;
    case mhd_H2_F_DEC_CONN_ERR_F_SIZE:
      mhd_h2_conn_finish (c,
                          mhd_H2_ERR_FRAME_SIZE_ERROR,
                          false);
      return false;
    case mhd_H2_F_DEC_CONN_ERR_PROT:
      mhd_h2_conn_finish (c,
                          mhd_H2_ERR_PROTOCOL_ERROR,
                          false);
      return false;
      break;
    default:
      break; /* ignore unknown types */
    }

    if (! proc_next)
      break;

    mhd_assert (mhd_HTTP_LAYER_CONNECTED == c->h_layer.state);

    mhd_assert ((c->read_buffer_offset - c->h2.buff.r_cur_frame) >=
                mhd_h2_frame_get_total_size (&h2frame));

    c->h2.buff.r_cur_frame += mhd_h2_frame_get_total_size (&h2frame);
  } while (c->h2.buff.r_cur_frame < c->read_buffer_offset);

  mhd_assert (c->read_buffer_offset >= c->h2.buff.r_cur_frame);
  if (mhd_HTTP_LAYER_CONNECTED == c->h_layer.state)
  {
    const size_t data_left = c->read_buffer_offset - c->h2.buff.r_cur_frame;

    mhd_assert (data_left <= c->read_buffer_offset);

    memmove (c->read_buffer,
             c->read_buffer + c->h2.buff.unproc_hdrs_pos,
             c->h2.buff.unproc_hdrs_size);
    c->h2.buff.unproc_hdrs_pos = 0u;

    memmove (c->read_buffer + c->h2.buff.unproc_hdrs_size,
             c->read_buffer + c->h2.buff.r_cur_frame,
             data_left);

    c->h2.buff.r_cur_frame = c->h2.buff.unproc_hdrs_size;
    c->read_buffer_offset = c->h2.buff.r_cur_frame + data_left;
  }

  return (mhd_HTTP_LAYER_CONNECTED == c->h_layer.state);
}
