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
 * @file src/mhd2/h2/h2_proc_conn.c
 * @brief  Implementation of HTTP/2 connection processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_connection.h"

#include "h2_frame_types.h"

#include "h2_frame_init.h"
#include "h2_frame_codec.h"

#include "h2_proc_settings.h"
#include "h2_conn_streams.h"
#include "h2_proc_out.h"

#include "h2_proc_conn.h"


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_process_first_fr (struct MHD_Connection *restrict c)
{
  union mhd_H2FrameUnion first_fr;
  struct mhd_Buffer payload;
  enum mhd_H2FrameDecodeResult dec_res;
  bool ret;

  mhd_assert (mhd_C_IS_HTTP2 (c));
  mhd_assert (! c->h2.state.init.got_setns);

  dec_res = mhd_h2_frame_decode (c->read_buffer_offset - c->h2.buff.r_cur_frame,
                                 (uint8_t *) c->read_buffer
                                 + c->h2.buff.r_cur_frame,
                                 mhd_H2_STNG_DEF_MAX_FRAME_SIZE, /* Settings are not yet ACKed */
                                 &first_fr,
                                 &payload);
  if (mhd_H2_FRAME_DEC_ERR_IS_HARD (dec_res))
  {
    mhd_h2_conn_finish (c,
                        mhd_H2_F_DEC_CONN_ERR_F_SIZE == dec_res ?
                        mhd_H2_ERR_FRAME_SIZE_ERROR : mhd_H2_ERR_PROTOCOL_ERROR,
                        true);
    return false;
  }

  if (mhd_H2_F_DEC_OK != dec_res)
    return false;       /* Not yet complete */

  if ((mhd_H2_FRAME_IDS_SETTINGS_ID != first_fr.selector.type) ||
      (first_fr.settings.ack))
  {
    mhd_h2_conn_finish (c,
                        mhd_H2_ERR_PROTOCOL_ERROR,
                        true);
    return false;
  }

  ret = mhd_h2_proc_first_settings (c,
                                    &payload);

  c->h2.buff.r_cur_frame += mhd_h2_frame_get_total_size (&first_fr);

  return ret;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_process_in_goaway (struct MHD_Connection *restrict c,
                               uint_least32_t last_stream_id,
                               enum mhd_H2ErrorCode err)
{
  c->h2.state.recvd_goaway.occurred = true;
  c->h2.state.recvd_goaway.code = err;
  c->h2.peer.stream_id_limit = last_stream_id;

  // TODO: close all streams with higher IDs

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_update_stream_init_window (struct MHD_Connection *restrict c,
                                       uint_least32_t init_wind_size)
{
  mhd_assert (0x7FFFFFFFu >= init_wind_size);
  (void) c; // TODO: change window in all streams

  c->h2.peer.stream_init_win_sz = init_wind_size;
  return true;
}


static MHD_FN_PAR_NONNULL_ALL_ bool
conn_win_update (struct MHD_Connection *restrict c)
{
  mhd_assert (0 <= c->h2.state.recv_window);
  /* Dumb algorithm: if receive windows is less than three quarters of the full
   * window size, then bump to the full size. */
  if ((c->h2.rcv_cfg.conn_full_win_sz - c->h2.rcv_cfg.conn_full_win_sz / 4) >=
      (uint_least32_t) c->h2.state.recv_window)
  {
    const uint_least32_t incr =
      (uint_least32_t)
      (c->h2.rcv_cfg.conn_full_win_sz
       - (uint_least32_t) c->h2.state.recv_window);
    mhd_assert (0x7FFFFFFFu >= incr);
    if (! mhd_h2_q_window_update (c,
                                  0u,
                                  incr))
      return false;
    c->h2.state.recv_window = (int_least32_t) c->h2.rcv_cfg.conn_full_win_sz;
  }
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_process_changes (struct MHD_Connection *restrict c)
{
  if (! conn_win_update (c))
    return false;

  if (! mhd_h2_conn_maintain_streams_all (c))
    return false;

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_finish (struct MHD_Connection *restrict c,
                    enum mhd_H2ErrorCode err,
                    bool forced)
{
  if (! mhd_h2_q_goaway (c,
                         err))
  {
    if (! forced)
      return false;

    c->h2.state.sent_goaway.occurred = true;
    c->h2.state.sent_goaway.code = (uint_least32_t) err;

    c->h_layer.state = mhd_HTTP_LAYER_BROKEN;
    return true;
  }

  c->h2.state.sent_goaway.occurred = true;
  c->h2.state.sent_goaway.code = (uint_least32_t) err;

  c->h_layer.state = mhd_HTTP_LAYER_CLOSING;
  return true;
}
