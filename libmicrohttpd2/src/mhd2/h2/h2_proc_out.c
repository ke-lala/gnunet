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
 * @file src/mhd2/h2/h2_proc_out.c
 * @brief  Implementation of HTTP/2 connection outgoing data processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"

#include "mhd_connection.h"

#include "h2_err_codes.h"
#include "h2_frame_types.h"
#include "h2_frame_init.h"
#include "h2_frame_codec.h"

#include "h2_proc_in.h"

#include "h2_proc_out.h"


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_out_buff_has_space_sz (struct MHD_Connection *restrict c,
                              size_t space_needed)
{
  size_t have_buff_space;
  mhd_assert (c->write_buffer_size >= c->write_buffer_append_offset);

  mhd_assert (! c->h2.dbg.w_buff_updating);
  mhd_H2_W_BUFF_UPDATING_SET (&(c->h2));

  have_buff_space = c->write_buffer_size - c->write_buffer_append_offset;

  mhd_H2_W_BUFF_UPDATING_CLEAR (&(c->h2));

  return (space_needed <= have_buff_space);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_out_buff_has_space_fr (struct MHD_Connection *restrict c,
                              union mhd_H2FrameUnion *restrict h2frame)
{
  return mhd_h2_out_buff_has_space_sz (c,
                                       mhd_h2_frame_get_total_size (h2frame));
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_out_buff_acquire_fr_w_payload_l (
  struct MHD_Connection *restrict c,
  const union mhd_H2FrameUnion *restrict h2frame,
  uint_least32_t full_payload_limit,
  struct mhd_Buffer *restrict buff,
  size_t *restrict payload_offset)
{
  const size_t extra_hdr_size = mhd_h2_frame_get_extra_hdr_size (h2frame);
  const size_t padding_size = mhd_h2_frame_get_padding_size (h2frame);
  size_t w_buff_space;

  mhd_assert (! c->h2.dbg.w_buff_updating);
  mhd_H2_W_BUFF_UPDATING_SET (&(c->h2));
  mhd_assert (c->write_buffer_size >= c->write_buffer_append_offset);

  if (full_payload_limit > c->h2.peer.max_frame_size)
    full_payload_limit = (uint_least32_t) c->h2.peer.max_frame_size;

  w_buff_space = c->write_buffer_size - c->write_buffer_append_offset;
  if (((mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_size + padding_size)
       >= w_buff_space) ||
      ((extra_hdr_size + padding_size) >= full_payload_limit))
  {
    mhd_H2_W_BUFF_UPDATING_CLEAR (&(c->h2));
    return false;
  }

  mhd_assert (c->h2.peer.max_frame_size > mhd_H2_FR_HDR_BASE_SIZE);

  if (full_payload_limit < (w_buff_space - mhd_H2_FR_HDR_BASE_SIZE))
    w_buff_space = full_payload_limit + mhd_H2_FR_HDR_BASE_SIZE;

  buff->data = c->write_buffer + c->write_buffer_append_offset;
  buff->size = w_buff_space - padding_size;
  *payload_offset = mhd_H2_FR_HDR_BASE_SIZE + extra_hdr_size;

  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_out_buff_acquire_fr_w_payload (
  struct MHD_Connection *restrict c,
  const union mhd_H2FrameUnion *restrict h2frame,
  struct mhd_Buffer *restrict buff,
  size_t *restrict payload_offset)
{
  return
    mhd_h2_out_buff_acquire_fr_w_payload_l (c,
                                            h2frame,
                                            0xFFFFFFFFu,
                                            buff,
                                            payload_offset);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_out_buff_unlock (struct MHD_Connection *restrict c,
                        size_t size_used)
{
  mhd_assert (c->h2.dbg.w_buff_updating);
  mhd_assert (c->write_buffer_size >= size_used);
  mhd_assert ((c->write_buffer_size - size_used)
              >= c->write_buffer_append_offset);

  c->write_buffer_append_offset += size_used;
  mhd_H2_W_BUFF_UPDATING_CLEAR (&(c->h2));
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_q_frame_no_payload (struct MHD_Connection *restrict c,
                           union mhd_H2FrameUnion *restrict h2frame)
{
  size_t fr_hdr_size;
  size_t w_buff_space;
  bool succeed;

  mhd_assert (mhd_h2_frame_get_extra_hdr_size (h2frame) ==
              h2frame->selector.length);

  fr_hdr_size = mhd_H2_FR_HDR_BASE_SIZE
                + mhd_h2_frame_get_extra_hdr_size (h2frame);

  mhd_assert (! c->h2.dbg.w_buff_updating);
  mhd_H2_W_BUFF_UPDATING_SET (&(c->h2));

  succeed = false;
  w_buff_space = c->write_buffer_size - c->write_buffer_append_offset;
  if (fr_hdr_size <= w_buff_space)
  {
    uint8_t *w_buff;
    size_t written;
    w_buff = (uint8_t *) c->write_buffer + c->write_buffer_append_offset;

    written = mhd_h2_frame_hdr_encode (h2frame,
                                       w_buff_space,
                                       w_buff);
    mhd_assert (fr_hdr_size == written);
    c->write_buffer_append_offset += written;

    succeed = true;
  }

  mhd_H2_W_BUFF_UPDATING_CLEAR (&(c->h2));
  return succeed;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_q_rst_stream (struct MHD_Connection *restrict c,
                     uint_least32_t stream_id,
                     enum mhd_H2ErrorCode err)
{
  union mhd_H2FrameUnion h2frame;

  mhd_assert (0u != stream_id);
  if (c->h2.state.top_rst_stream_id < stream_id)
    c->h2.state.top_rst_stream_id = stream_id;

  mhd_h2_frame_init_rst_stream (&h2frame,
                                stream_id,
                                err);

  return mhd_h2_q_frame_no_payload (c,
                                    &h2frame);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_q_ping (struct MHD_Connection *restrict c,
               bool ack,
               const uint8_t opaque_data[MHD_FN_PAR_FIX_ARR_SIZE_ (8)])
{
  union mhd_H2FrameUnion h2frame;

  mhd_h2_frame_init_ping (&h2frame,
                          ack,
                          opaque_data);

  return mhd_h2_q_frame_no_payload (c,
                                    &h2frame);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_q_goaway (struct MHD_Connection *restrict c,
                 enum mhd_H2ErrorCode err)
{
  union mhd_H2FrameUnion h2frame;

  mhd_h2_frame_init_goaway (&h2frame,
                            c->h2.state.top_proc_stream_id,
                            err);

  return mhd_h2_q_frame_no_payload (c,
                                    &h2frame);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_q_window_update (struct MHD_Connection *restrict c,
                        uint_least32_t stream_id,
                        uint_least32_t win_size_incr)
{
  union mhd_H2FrameUnion h2frame;

  mhd_assert (0u != win_size_incr);

  mhd_h2_frame_init_window_update (&h2frame,
                                   stream_id,
                                   win_size_incr);

  return mhd_h2_q_frame_no_payload (c,
                                    &h2frame);
}
