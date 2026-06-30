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
 * @file src/mhd2/h2/h2_proc_settings.c
 * @brief  Implementation of HTTP/2 connection settings processing functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"

#include "mhd_connection.h"

#include "h2_frame_types.h"
#include "h2_frame_init.h"
#include "h2_settings.h"

#include "h2_frame_codec.h"
#include "hpack/mhd_hpack_codec.h"
#include "h2_proc_conn.h"
#include "h2_proc_out.h"

#include "h2_proc_settings.h"


static bool
h2_has_space_for_ack (struct MHD_Connection *restrict c)
{
#ifndef NDEBUG
  if (1)
  {
    union mhd_H2FrameUnion h2frame;

    mhd_h2_frame_init_settings (&h2frame,
                                true);
    mhd_assert (0u == mhd_h2_frame_get_extra_hdr_size (&h2frame));
  }
#endif /* NDEBUG */
  return mhd_h2_out_buff_has_space_sz (c,
                                       mhd_H2_FR_HDR_BASE_SIZE);
}


static bool
h2_q_settings_ack (struct MHD_Connection *restrict c)
{
  union mhd_H2FrameUnion h2frame;

  mhd_h2_frame_init_settings (&h2frame,
                              true);
  /* The actual size should match free space check */
  mhd_assert (0u == mhd_h2_frame_get_extra_hdr_size (&h2frame));

  return mhd_h2_q_frame_no_payload (c,
                                    &h2frame);
}


MHD_INTERNAL bool
mhd_h2_proc_first_settings (struct MHD_Connection *restrict c,
                            const struct mhd_Buffer *restrict stngs_payload)
{
  bool ack_succeed;

  /* The payload size must be checked by frame decoder */
  mhd_assert (0u == (stngs_payload->size % 6u));

  mhd_assert (h2_has_space_for_ack (c));

  if (stngs_payload->size > 6u)
  {
    size_t pos;

    for (pos = 0u; pos < (stngs_payload->size - 5u); pos += 6u)
    {
      struct mhd_H2Setting stng;
      mhd_h2_setting_decode ((uint8_t *) stngs_payload->data + pos,
                             &stng);

      switch (stng.identifier)
      {
      case mhd_H2_STNGS_HEADER_TABLE_SIZE:
        if (mhd_DTBL_MAX_SIZE >= stng.value)
        {
          if (4096u >= stng.value) // TODO: take the limit from the daemon
            mhd_hpack_enc_set_dyn_size (&(c->h2.hk_enc),
                                        (size_t) stng.value);
        }
        break;
      case mhd_H2_STNGS_ENABLE_PUSH:
        /* Ignored */
        break;
      case mhd_H2_STNGS_CONCURRENT_STREAMS:
        c->h2.peer.max_concur_streams = stng.value;
        break;
      case mhd_H2_STNGS_INITIAL_WINDOW_SIZE:
        if (0x7FFFFFFFu < stng.value)
        {
          mhd_h2_conn_finish (c,
                              mhd_H2_ERR_FLOW_CONTROL_ERROR,
                              true);
          return false; /* Failure exit point */
        }

        /* Set the initial size. No streams should be modified as no
           streams have been started yet. */
        c->h2.peer.stream_init_win_sz = stng.value;
        break;
      case mhd_H2_STNGS_MAX_FRAME_SIZE:
        if ((mhd_H2_STNG_MIN_MAX_FRAME_SIZE > stng.value) ||
            (mhd_H2_STNG_MAX_MAX_FRAME_SIZE < stng.value))
        {
          mhd_h2_conn_finish (c,
                              mhd_H2_ERR_PROTOCOL_ERROR,
                              true);
          return false; /* Failure exit point */
        }
#if 0 // TODO: use limit from the daemon settings
        if (mhd_H2_STNG_DEF_MAX_FRAME_SIZE >= stng.value)
          c->h2.peer.max_frame_size = stng.value;
#endif
        break;
      case mhd_H2_STNGS_MAX_HEADER_LIST_SIZE:
        c->h2.peer.max_header_list = stng.value;
        break;
      case mhd_H2_STNGS_ENABLE_CONNECT_PROTOCOL:
      case mhd_H2_STNGS_NO_RFC7540_PRIORITIES:
        /* Ignored */
        break;
#ifndef mhd_USE_ENUM_BASE_T
      case mhd_H2_STNGS_SENTINEL:
#endif /*mhd_USE_ENUM_BASE_T */
      default:
        /* Unknown setting ignored */
        break;
      }
    }
  }
  c->h2.state.init.got_setns = true;

  ack_succeed = h2_q_settings_ack (c);
  mhd_assert (ack_succeed);
  (void) ack_succeed;

  return true; /* Success exit point */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ enum mhd_H2SettingsProcessResult
mhd_h2_proc_new_settings (struct MHD_Connection *restrict c,
                          const struct mhd_Buffer *restrict stngs_payload)
{
  (void) c; (void) stngs_payload; // TODO: implement
  return mhd_H2_STNGS_PROC_STNGS_ERR;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_q_settings_first_fr (struct MHD_Connection *restrict c)
{
  size_t fr_hdr_size;
  size_t final_fr_hdr_size;
  size_t payload_space;
  uint8_t *payload;
  size_t payload_pos;
  bool set_all_settings;
  struct mhd_Buffer buff;
  union mhd_H2FrameUnion h2frame;

  mhd_h2_frame_init_settings (&h2frame,
                              false);

  /* This is the first data sent by the server */
  mhd_assert (0u == c->write_buffer_append_offset);

  if (! mhd_h2_out_buff_acquire_fr_w_payload (c,
                                              &h2frame,
                                              &buff,
                                              &fr_hdr_size))
  {
    mhd_H2_W_BUFF_UPDATING_CLEAR (&(c->h2));
    mhd_h2_conn_finish (c,
                        mhd_H2_ERR_INTERNAL_ERROR,
                        true);
    return false;
  }

  payload = (uint8_t *) buff.data + fr_hdr_size;
  payload_space = buff.size - fr_hdr_size;

  if (payload_space != (payload_space & 0xFFFFFFFFu))
    payload_space = 0xFFFFFFFFu;

  payload_pos = 0u;

  // TODO: use configurable values for settings
  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_h2_setting_encode3 (mhd_H2_STNGS_HEADER_TABLE_SIZE,
                            (uint_least32_t) c->h2.hk_dec.max_allowed_dyn_size,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
  }

  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_h2_setting_encode3 (mhd_H2_STNGS_ENABLE_PUSH,
                            0u,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
  }

  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_assert (0x7FFFFFFFu >= c->h2.rcv_cfg.stream_init_win_sz);
    mhd_h2_setting_encode3 (mhd_H2_STNGS_INITIAL_WINDOW_SIZE,
                            c->h2.rcv_cfg.stream_init_win_sz,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
  }

  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_assert (0x7FFFFFFFu >= c->h2.rcv_cfg.max_frame_size);
    mhd_h2_setting_encode3 (mhd_H2_STNGS_MAX_FRAME_SIZE,
                            c->h2.rcv_cfg.max_frame_size,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
  }

  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_h2_setting_encode3 (mhd_H2_STNGS_MAX_HEADER_LIST_SIZE,
                            c->h2.rcv_cfg.max_header_list,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
  }

  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_h2_setting_encode3 (mhd_H2_STNGS_ENABLE_CONNECT_PROTOCOL,
                            0u,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
  }

  if (payload_space >= payload_pos + mhd_H2_SETTING_SIZE)
  {
    mhd_h2_setting_encode3 (mhd_H2_STNGS_NO_RFC7540_PRIORITIES,
                            1u,
                            payload + payload_pos);
    payload_pos += mhd_H2_SETTING_SIZE;
    set_all_settings = true;
  }
  else
    set_all_settings = false;

  if (! set_all_settings)
  {
    mhd_H2_W_BUFF_UPDATING_CLEAR (&(c->h2));
    mhd_h2_conn_finish (c,
                        mhd_H2_ERR_INTERNAL_ERROR,
                        true);
    return false;
  }

  mhd_assert (0u == payload_pos % mhd_H2_SETTING_SIZE);

  mhd_h2_frame_set_payload_size (&h2frame,
                                 payload_pos);

  final_fr_hdr_size = mhd_h2_frame_hdr_encode (&h2frame,
                                               buff.size,
                                               (uint8_t *) buff.data);
  mhd_assert (fr_hdr_size == final_fr_hdr_size);
  mhd_h2_out_buff_unlock (c,
                          final_fr_hdr_size + payload_pos);

  c->h2.state.init.sent_setns = true;
  ++c->h2.state.sent_setns_noakc;

  return true;
}
