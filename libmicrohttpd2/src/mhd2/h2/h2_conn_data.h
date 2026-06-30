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
 * @file src/mhd2/h2/h2_conn_data.h
 * @brief  Definition of HTTP/2 connection-specific data
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_H2_CONN_DATA_H
#define MHD_H2_CONN_DATA_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_dlinked_list.h"

#include "hpack/mhd_hpack_dec_types.h"
#include "hpack/mhd_hpack_enc_types.h"

/**
 * The size in bytes of HTTP/2 preface
 */
#define mhd_H2_PREFACE_LEN      (24u)

struct mhd_MemoryPool; /* Forward declaration */
struct mhd_H2ReqItemsBlock; /* Forward declaration */
struct mhd_H2Stream; /* Forward declaration */

mhd_DLINKEDL_LIST_DEF (mhd_H2Stream);

struct mhd_H2ConnRecvCtrlData
{
  uint_least32_t stream_init_win_sz;

  uint_least32_t conn_full_win_sz;

  uint_least32_t max_frame_size; // TODO: implement update when ACKed by peer

  uint_least32_t max_header_list;

  uint_least32_t max_concur_streams;
};

struct mhd_H2ConnMemData
{
  struct mhd_MemoryPool *send_pool;

  /**
   * Request items block (headers, footers, URI arguments, cookies etc.)
   */
  struct mhd_H2ReqItemsBlock *req_ib;
};

struct mhd_H2GoawayData
{
  bool occurred;

  uint_least32_t code;
};

struct mhd_H2ConnStateInitData
{
  /**
   * 'true' if the first peer settings has been received and processed
   */
  bool got_setns;
  /**
   * 'true' if the first settings frame has been sent
   */
  bool sent_setns;
};

struct mhd_H2ConnStateData
{
  struct mhd_H2ConnStateInitData init;
  /**
   * Number of sent settings not ACKed yet by peer
   */
  uint_fast64_t sent_setns_noakc;

  int_least32_t send_window;

  int_least32_t recv_window;
  /**
   * Highest Stream ID of any received stream
   */
  uint_least32_t top_seen_stream_id;
  /**
   * Highest Stream ID of the stream for which application callback is called
   */
  uint_least32_t top_proc_stream_id;
  /**
   * Highest Stream ID of the stream for which RST_STREAM was sent
   */
  uint_least32_t top_rst_stream_id;
  /**
   * If set to non-zero then the next incoming frame must be a CONTINUATION
   * frame with specified Stream ID.
   */
  uint_least32_t continuation_stream_id;

  struct mhd_H2GoawayData sent_goaway;

  struct mhd_H2GoawayData recvd_goaway;
};

struct mhd_H2ConnPeerData
{
  uint_least32_t stream_init_win_sz;

  /**
   * Maximum outgoing frame size to use.
   * If peer set value smaller than allowed by the settings, then this value
   * is in this field. Otherwise, this field indicates maximum size allowed
   * by the settings.
   */
  uint_least32_t max_frame_size;

  uint_least32_t max_header_list;

  uint_least32_t max_concur_streams;

  /**
   * Stream ID specified as the last stream in peer GOAWAY message
   */
  uint_least32_t stream_id_limit; // TODO check value when getting new streams
};

struct mhd_H2ConnStreams
{
  /**
   * The list of all not finished streams
   */
  mhd_DLNKDL_LIST (mhd_H2Stream, active);

  /**
   * The sending queue
   */
  mhd_DLNKDL_LIST (mhd_H2Stream, send_q);

  /**
   * The total number streams in the @a active list
   */
  size_t num_streams;
};

struct mhd_H2ConnBuffData
{
  /**
   * The offset in the read buffer of the frame to be processes / being processed
   */
  size_t r_cur_frame;

  /**
   * Position of unprocessed part of HEADERS/CONTINUATION payload.
   * Used only when the next frame is CONTINUATION.
   */
  size_t unproc_hdrs_pos;

  /**
   * Size of unprocessed part of HEADERS/CONTINUATION payload.
   * Must be non-zero only when the next frame is CONTINUATION.
   */
  size_t unproc_hdrs_size;
};

#ifndef NDEBUG
struct mhd_H2ConnDebug
{
  volatile bool w_buff_updating;
  volatile bool h2_deinited;
};
#endif /* ! NDEBUG */

struct mhd_H2ConnData
{
  struct mhd_H2ConnRecvCtrlData rcv_cfg;

  struct mhd_H2ConnMemData mem;

  struct mhd_H2ConnStateData state;

  struct mhd_H2ConnPeerData peer;

  struct mhd_HpackDecContext hk_dec;

  struct mhd_HpackEncContext hk_enc;

  struct mhd_H2ConnStreams streams;

  struct mhd_H2ConnBuffData buff;
#ifndef NDEBUG
  struct mhd_H2ConnDebug dbg;
#endif /* ! NDEBUG */
};

#ifndef NDEBUG
#  define mhd_H2_W_BUFF_UPDATING_SET(h2data) \
        do {(h2data)->dbg.w_buff_updating = true;} while (0)
#  define mhd_H2_W_BUFF_UPDATING_CLEAR(h2data) \
        do {(h2data)->dbg.w_buff_updating = false;} while (0)
#else  /* NDEBUG */
#  define mhd_H2_W_BUFF_UPDATING_SET(h2data)    ((void) 0)
#  define mhd_H2_W_BUFF_UPDATING_CLEAR(h2data)  ((void) 0)
#endif /* NDEBUG */

#endif /* ! MHD_H2_CONN_DATA_H */
