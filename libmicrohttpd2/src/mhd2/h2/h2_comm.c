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
 * @file src/mhd2/h2/h2_comm.c
 * @brief  Implementation of HTTP/2 connection communication functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include <string.h>

#include "mhd_unreachable.h"
#include "mhd_constexpr.h"
#include "mhd_assert.h"

#include "mhd_connection.h"
#include "mhd_daemon.h"

#include "mempool_funcs.h"

#ifdef MHD_SUPPORT_HTTPS
#  include "mhd_tls_funcs.h"
#endif

#include "respond_with_error.h"
#include "stream_funcs.h"
#include "daemon_logger.h"

#include "h2_req_items_funcs.h"
#include "h2_frame_codec.h"
#include "h2_proc_settings.h"
#include "h2_proc_conn.h"
#include "h2_proc_in.h"
#include "h2_conn_streams.h"
#include "hpack/mhd_hpack_codec.h"

#include "h2_comm.h"

#ifndef HAVE_NULL_PTR_ALL_ZEROS
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_blank_init (struct MHD_Connection *restrict c)
{
  c->write_buffer = NULL;
}


#endif /* HAVE_NULL_PTR_ALL_ZEROS */

/**
 * HTTP/2 connection preface
 *
 * Extracted from RFC 9113, Section 3.4
 */
mhd_constexpr uint8_t mhd_h2_preface[mhd_H2_PREFACE_LEN] = {
  0x50u, 0x52u, 0x49u, 0x20u, 0x2Au, 0x20u, 0x48u, 0x54u, 0x54u, 0x50u,
  0x2Fu, 0x32u, 0x2Eu, 0x30u, 0x0Du, 0x0Au, 0x0Du, 0x0Au, 0x53u, 0x4Du,
  0x0Du, 0x0Au, 0x0Du, 0x0Au
};


/**
 * Result of HTTP/2 preface check
 */
enum MHD_FIXED_ENUM_ mhd_H2PrefaceCheckResult
{
  /**
   * Received data matches HTTP/2 preface
   */
  mhd_H2_PREFACE_CHECK_IS_HTTP2
  ,
  /**
   * Received data does not match HTTP/2 preface
   */
  mhd_H2_PREFACE_CHECK_IS_NOT_HTTP2
  ,
  /**
   * Not enough data has been received
   */
  mhd_H2_PREFACE_CHECK_NEED_MORE_DATA
};

/**
 * Check HTTP/2 connection preface
 * @param c the connection to process
 * @return enum mhd_H2PrefaceCheckResult status code
 */
mhd_static_inline MHD_FN_PAR_NONNULL_ALL_ enum mhd_H2PrefaceCheckResult
mhd_h2_check_preface (struct MHD_Connection *restrict c)
{
  bool have_enough_data;

  mhd_assert (mhd_HTTP_LAYER_PREFACE == c->h_layer.state);
  mhd_assert (NULL != c->read_buffer);
  mhd_assert (mhd_H2_PREFACE_LEN <= c->read_buffer_size);

  if (0u == c->read_buffer_offset)
    return mhd_H2_PREFACE_CHECK_NEED_MORE_DATA;

  have_enough_data = (mhd_H2_PREFACE_LEN <= c->read_buffer_offset);

  if (0 !=
      memcmp (mhd_h2_preface,
              c->read_buffer,
              have_enough_data ? mhd_H2_PREFACE_LEN : c->read_buffer_offset))
    return mhd_H2_PREFACE_CHECK_IS_NOT_HTTP2;

  return (have_enough_data ?
          mhd_H2_PREFACE_CHECK_IS_HTTP2 : mhd_H2_PREFACE_CHECK_NEED_MORE_DATA);
}


#define ERR_RSP_H2_NOT_SUPPORTED \
        "<html><head><title>HTTP/2 is not supported</title></head>" \
        "<body>HTTP/2 protocol is not supported.</body></html>"

#define ERR_RSP_H2_WITH_ALPN_HTTP1 \
        "<html><head><title>HTTP/2 without matching ALPN</title></head>" \
        "<body>ALPN selected HTTP/1.x protocol, HTTP/2 cannot be used " \
        "over TLS if ALPN selected another application protocol.</body></html>"

#define ERR_RSP_H2_WITHOUT_ALPN \
        "<html><head><title>HTTP/2 without ALPN on HTTPS</title></head>" \
        "<body>HTTP/2 cannot be used over TLS without ALPN.</body></html>"

/**
 * Perform switching connection to HTTP/2 mode
 * @param c the connection to switch
 * @return 'true' if switched successfully,
 *         'false' if connection is broken and should be closed
 */
static MHD_FN_PAR_NONNULL_ALL_ bool
h2_switch_to_h2 (struct MHD_Connection *restrict c)
{
  size_t buff_size;
  mhd_assert (mhd_HTTP_LAYER_PREFACE == c->h_layer.state);
  mhd_assert (mhd_H2_PREFACE_LEN <= c->read_buffer_offset);
  mhd_assert (mhd_HTTP_VER_FAM_NOT_SET == c->h_layer.fam);

  mhd_DLINKEDL_INIT_LIST_D (&(c->h2.streams.active));
  mhd_DLINKEDL_INIT_LIST_D (&(c->h2.streams.send_q));
  c->h2.streams.num_streams = 0u;

  c->h2.state.init.got_setns = false;
  c->h2.state.init.sent_setns = false;

  c->h2.state.sent_setns_noakc = 0u;

  c->h2.state.send_window = mhd_H2_STNG_DEF_INIT_WIN_SIZE;
  c->h2.state.recv_window = mhd_H2_STNG_DEF_INIT_WIN_SIZE;

  c->h2.state.top_seen_stream_id = 0u;

  // TODO: make some parameters configurable
  c->h2.rcv_cfg.stream_init_win_sz = 0x7FFFFFFFu;
  c->h2.rcv_cfg.conn_full_win_sz = 0x7FFFFFFFu;
  c->h2.rcv_cfg.max_frame_size = mhd_H2_STNG_DEF_MAX_FRAME_SIZE;
  c->h2.rcv_cfg.max_header_list = 16u * 1024u;
  c->h2.rcv_cfg.max_concur_streams = 100u;

  c->h2.peer.stream_init_win_sz = mhd_H2_STNG_DEF_INIT_WIN_SIZE;
  c->h2.peer.max_frame_size = mhd_H2_STNG_DEF_MAX_FRAME_SIZE;
  c->h2.peer.max_header_list = (uint_least32_t) (~((uint_least32_t) 0u));
  c->h2.peer.max_concur_streams = (uint_least32_t) (~((uint_least32_t) 0u));

  buff_size = mhd_pool_get_size (c->pool);
  c->read_buffer =
    (char *)
    mhd_pool_reset (c->pool,
                    c->read_buffer,
                    c->read_buffer_offset,
                    buff_size);
  mhd_assert (NULL != c->read_buffer);
  c->read_buffer_size = buff_size;
  c->h2.buff.r_cur_frame = mhd_H2_PREFACE_LEN;

  c->h2.mem.send_pool =
    mhd_pool_create (c->daemon->conns.cfg.mem_pool_size,
                     c->daemon->conns.cfg.mem_pool_zeroing);
  if (NULL != c->h2.mem.send_pool)
  {
    buff_size = mhd_pool_get_size (c->h2.mem.send_pool);
    c->write_buffer =
      (char *)
      mhd_pool_allocate (c->h2.mem.send_pool,
                         buff_size,
                         false);
    mhd_assert (NULL != c->write_buffer);
    c->write_buffer_size = buff_size;
    c->write_buffer_append_offset = 0u;
    c->write_buffer_send_offset = 0u;

    if (mhd_hpack_dec_init (&(c->h2.hk_dec)))
    {
      if (mhd_hpack_enc_init (&(c->h2.hk_enc)))
      {
        // TODO: make the size configurable
        c->h2.mem.req_ib = mhd_h2_items_block_create (16u * 1024);

        if (NULL != c->h2.mem.req_ib)
        {
          c->h_layer.fam = mhd_HTTP_VER_FAM_2;
          c->h_layer.state = mhd_HTTP_LAYER_CONNECTED;

          return true; /* Success exit point */
        }
        /* Clean-up */
        mhd_hpack_enc_deinit (&(c->h2.hk_enc));
      }
      mhd_hpack_dec_deinit (&(c->h2.hk_dec));
    }
    c->write_buffer = NULL;
    mhd_pool_destroy (c->h2.mem.send_pool);
    c->h2.mem.send_pool = NULL;
    mhd_LOG_MSG (c->daemon, MHD_SC_H2_CONN_MEM_ALLOC_FAILURE, \
                 "Failed to allocate memory for the HTTP/2 "
                 "connection resources.");
  }
  else
    mhd_LOG_MSG (c->daemon, MHD_SC_POOL_MEM_ALLOC_FAILURE, \
                 "Failed to allocate memory for the HTTP/2 send buffer.");

  mhd_conn_start_closing_no_sys_res (c);
  c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_SET;
  c->h_layer.state = mhd_HTTP_LAYER_BROKEN;
  return false;
}


/**
 * Perform de-initialisation of HTTP/2-specific data and start connection
 * closing
 * @param c the connection to de-initialise
 */
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_conn_h2_deinit_start_closing (struct MHD_Connection *restrict c)
{
  mhd_assert (mhd_C_IS_HTTP2 (c));
  mhd_assert ((mhd_HTTP_LAYER_CONNECTED == c->h_layer.state) ||
              (mhd_HTTP_LAYER_CLOSING == c->h_layer.state) ||
              (mhd_HTTP_LAYER_BROKEN == c->h_layer.state));
  mhd_assert (! c->h2.dbg.h2_deinited);

  mhd_h2_conn_close_streams_all (c);

  mhd_h2_items_block_destroy (c->h2.mem.req_ib);
  c->h2.mem.req_ib = NULL;
  mhd_hpack_enc_deinit (&(c->h2.hk_enc));
  mhd_hpack_dec_deinit (&(c->h2.hk_dec));

  mhd_assert (NULL != c->write_buffer);
  mhd_assert (NULL != c->h2.mem.send_pool);
  c->write_buffer_send_offset = 0u;
  c->write_buffer_append_offset = 0u;
  c->write_buffer_size = 0u;
  c->write_buffer = NULL;
  mhd_pool_destroy (c->h2.mem.send_pool);
  c->h2.mem.send_pool = NULL;

#ifndef NDEBUG
  c->h2.dbg.h2_deinited = true;
#endif /* ! NDEBUG */

  if ((mhd_HTTP_LAYER_BROKEN == c->h_layer.state) ||
      (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err) ||
      (0 != (c->sk.ready & mhd_SOCKET_NET_STATE_ERROR_READY)))
    mhd_conn_start_closing_h2_hard (c);
  else
    mhd_conn_start_closing_h2_soft (c);

  if (mhd_HTTP_LAYER_CLOSED > c->h_layer.state)
    c->h_layer.state = mhd_HTTP_LAYER_CLOSED;
}


/**
 * Handle detection of HTTP/2 preface
 * @param c the connection to process
 * @return 'true' if connection should be continued to be processed as
 *                HTTP connection (possibly to send already scheduled error
 *                response),
 *         'false' if connection is broken and should be closed
 */
static MHD_FN_PAR_NONNULL_ALL_ bool
h2_handle_preface_found (struct MHD_Connection *restrict c)
{
  const bool allow_on_alpn_mismatch =
    (c->daemon->req_cfg.strictness <= MHD_PSL_EXTRA_PERMISSIVE);
  const bool allow_without_alpn =
    (c->daemon->req_cfg.strictness <= MHD_PSL_PERMISSIVE);

  mhd_assert (mhd_HTTP_LAYER_PREFACE == c->h_layer.state);
  mhd_assert (mhd_H2_PREFACE_LEN <= c->read_buffer_offset);

  if (! mhd_D_IS_HTTP2_ENABLED (c->daemon))
  {
    c->h_layer.state = mhd_HTTP_LAYER_CONNECTED;
    c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_2;
    mhd_RESPOND_WITH_ERROR_STATIC (c,
                                   MHD_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
                                   ERR_RSP_H2_NOT_SUPPORTED);
    return true; /* Send error response */
  }

#ifdef MHD_SUPPORT_HTTPS
  if (mhd_C_HAS_TLS (c))
  {
    switch (mhd_tls_conn_get_alpn_prot (c->tls))
    {
    case mhd_TLS_ALPN_PROT_HTTP1_0:
    case mhd_TLS_ALPN_PROT_HTTP1_1:
      if (! allow_on_alpn_mismatch)
      {
        c->h_layer.state = mhd_HTTP_LAYER_CONNECTED;
        c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_2;
        mhd_RESPOND_WITH_ERROR_STATIC (c,
                                       MHD_HTTP_STATUS_BAD_REQUEST,
                                       ERR_RSP_H2_WITH_ALPN_HTTP1);
        return true; /* Send error response */
      }
      break;
    case mhd_TLS_ALPN_PROT_HTTP2:
      break;
    case mhd_TLS_ALPN_PROT_NOT_SELECTED:
    case mhd_TLS_ALPN_PROT_ERROR:
      if (! allow_without_alpn)
      {
        c->h_layer.state = mhd_HTTP_LAYER_CONNECTED;
        c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_2;
        mhd_RESPOND_WITH_ERROR_STATIC (c,
                                       MHD_HTTP_STATUS_BAD_REQUEST,
                                       ERR_RSP_H2_WITHOUT_ALPN);
        return true; /* Send error response */
      }
      break;
    default:
      mhd_UNREACHABLE ();
      return false;
    }
  }
#endif /* MHD_SUPPORT_HTTPS */
  return h2_switch_to_h2 (c);
}


#define ERR_RSP_H2_REQUIRED \
        "<html><head><title>HTTP/2 is required</title></head>" \
        "<body>HTTP/2 protocol is required</body></html>"

#define ERR_RSP_NOT_H2_WITH_ALPN_H2 \
        "<html><head><title>ALPN selected HTTP/2</title></head>" \
        "<body>ALPN selected HTTP/2 protocol, " \
        "only HTTP/2 communication could be used.</body></html>"

/**
 * Handle absence of HTTP/2 preface
 * @param c the connection to process
 * @return 'true' if connection should be continued to be processed as
 *                HTTP connection (possibly to send already scheduled error
 *                response),
 *         'false' if connection is broken and should be closed
 */
static MHD_FN_PAR_NONNULL_ALL_ bool
h2_handle_preface_not_found (struct MHD_Connection *restrict c)
{
  const bool allow_on_alpn_mismatch =
    (c->daemon->req_cfg.strictness <= MHD_PSL_EXTRA_PERMISSIVE);

  mhd_assert (mhd_HTTP_LAYER_PREFACE == c->h_layer.state);
  mhd_assert (0u != c->read_buffer_offset);

#ifdef MHD_SUPPORT_HTTPS
  if (mhd_C_HAS_TLS (c))
  {
    switch (mhd_tls_conn_get_alpn_prot (c->tls))
    {
    case mhd_TLS_ALPN_PROT_HTTP1_0:
    case mhd_TLS_ALPN_PROT_HTTP1_1:
    case mhd_TLS_ALPN_PROT_NOT_SELECTED:
    case mhd_TLS_ALPN_PROT_ERROR:
      break;
    case mhd_TLS_ALPN_PROT_HTTP2:
      if (! allow_on_alpn_mismatch)
      {
        c->h_layer.state = mhd_HTTP_LAYER_BROKEN;
        c->h_layer.fam = mhd_HTTP_VER_FAM_INVALID;
        mhd_conn_start_closing (c,
                                mhd_CONN_CLOSE_H2_PREFACE_MISSING,
                                mhd_MSG4LOG ("No valid HTTP/2 preface on " \
                                             "TLS connection with 'h2' "
                                             "selected by ALPN"));
        return false;
      }
      break;
    default:
      mhd_UNREACHABLE ();
      c->h_layer.state = mhd_HTTP_LAYER_BROKEN;
      c->h_layer.fam = mhd_HTTP_VER_FAM_INVALID;
      return false;
      break;
    }
  }
#endif /* MHD_SUPPORT_HTTPS */

  if (! mhd_D_IS_HTTP1_ENABLED (c->daemon))
  {
    c->h_layer.state = mhd_HTTP_LAYER_CONNECTED;
    c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_2;
    mhd_RESPOND_WITH_ERROR_STATIC (c,
                                   MHD_HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
                                   ERR_RSP_H2_REQUIRED);
    return true; /* Send error response */
  }

  c->h_layer.state = mhd_HTTP_LAYER_CONNECTED;
  c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_2;
  /* The data in the receive buffer will be re-interpreted as HTTP/1.x request */
  return true;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_INTERNAL enum mhd_CommLayerState
mhd_h2_process_preface (struct MHD_Connection *restrict c)
{
  mhd_assert (mhd_HTTP_LAYER_PREFACE == c->h_layer.state);

  switch (mhd_h2_check_preface (c))
  {
  case mhd_H2_PREFACE_CHECK_IS_HTTP2:
    return
      h2_handle_preface_found (c) ? mhd_COMM_LAYER_OK : mhd_COMM_LAYER_BROKEN;
  case mhd_H2_PREFACE_CHECK_IS_NOT_HTTP2:
    return
      h2_handle_preface_not_found (c) ?
      mhd_COMM_LAYER_OK : mhd_COMM_LAYER_BROKEN;
  case mhd_H2_PREFACE_CHECK_NEED_MORE_DATA:
    return mhd_COMM_LAYER_PROCESSING;
  default:
    break;
  }
  mhd_UNREACHABLE ();
  return mhd_COMM_LAYER_BROKEN;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_h2_conn_state_update (struct MHD_Connection *restrict c)
{
  unsigned int new_state;

  mhd_assert (mhd_C_IS_HTTP2 (c));

  mhd_assert (0u == (c->event_loop_info & MHD_EVENT_LOOP_INFO_CLEANUP));
#ifdef MHD_SUPPORT_UPGRADE
  mhd_assert (0u == (c->event_loop_info & MHD_EVENT_LOOP_INFO_UPGRADED));
#endif /* MHD_SUPPORT_UPGRADE */

  new_state = 0u;

  if (c->read_buffer_offset < c->read_buffer_size)
    new_state |= (unsigned int) MHD_EVENT_LOOP_INFO_RECV;

  if (c->write_buffer_send_offset < c->write_buffer_append_offset)
    new_state |= (unsigned int) MHD_EVENT_LOOP_INFO_SEND;

  c->event_loop_info = (enum MHD_ConnectionEventLoopInfo) new_state;
}


static MHD_FN_PAR_NONNULL_ALL_ void
h2_conn_manage_buff_out (struct MHD_Connection *restrict c)
{
  mhd_assert (c->write_buffer_send_offset <= c->write_buffer_append_offset);
  if (c->write_buffer_send_offset == c->write_buffer_append_offset)
  {
    c->write_buffer_send_offset = 0u;
    c->write_buffer_append_offset = 0u;
  }
  else if ((0u != c->write_buffer_append_offset) &&
           (c->write_buffer_send_offset > c->write_buffer_size / 128))
  {
    const size_t left_unsent =
      c->write_buffer_append_offset - c->write_buffer_send_offset;
    memmove (c->write_buffer,
             c->write_buffer + c->write_buffer_send_offset,
             left_unsent);
    c->write_buffer_send_offset = 0u;
    c->write_buffer_append_offset = left_unsent;
  }
}


static MHD_FN_PAR_NONNULL_ALL_ void
h2_conn_process_data_inner (struct MHD_Connection *restrict c)
{
  /* Check whether first SETTINGS frame was send (queued).
     It is a part of connection initialisation. */
  if (! c->h2.state.init.sent_setns)
  {
    if (! mhd_h2_q_settings_first_fr (c))
      return;
  }
  /* Check whether first peer SETTINGS frame was received.
     It is a part of connection initialisation. */
  if (! c->h2.state.init.got_setns)
  {
    if (! mhd_h2_conn_process_first_fr (c))
      return; /* HTTP/2 cannot be processed */
  }

  h2_conn_manage_buff_out (c);

  /* Process incoming data.
     When incoming frames are processed, short control frames (such as
     PING ACK, SETTINGS ACK, RST_STREAM) could be added to the sending buffer.
     If connection is broken or output buffer has no space to add required
     frame, this connection processing is stopped here until the output buffer
     got more space. */
  if (! mhd_h2_conn_process_in_data (c))
    return;

  /* Close broken streams, update receive windows.
     Short control frames, like WINDOW_UPDATE, RST_STREAM could be added to the
     sending buffer.
     If connection is broken or output buffer has no space to add required
     frame, this connection processing is stopped here until the output buffer
     got more space. */
  if (! mhd_h2_conn_process_changes (c))
    return;

  /* Finally send the replies (if output buffer still have any space).  */
  mhd_h2_conn_process_streams_sending_queue (c);

  return;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_conn_process_data (struct MHD_Connection *restrict c)
{
  mhd_assert (mhd_C_IS_HTTP2 (c));

  mhd_assert (mhd_HTTP_LAYER_CONNECTED <= c->h_layer.state);
  mhd_assert (mhd_HTTP_LAYER_CLOSING >= c->h_layer.state);

  if (mhd_HTTP_LAYER_CLOSING == c->h_layer.state)
  {
    if ((c->write_buffer_append_offset == c->write_buffer_send_offset)
        || (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err))
    {
      mhd_h2_conn_h2_deinit_start_closing (c);
      return false;
    }
    return true;
  }

  h2_conn_process_data_inner (c);

  mhd_assert (mhd_HTTP_LAYER_CLOSED != c->h_layer.state);
  if ((mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err) ||
      (mhd_HTTP_LAYER_CLOSING == c->h_layer.state))
  {
    mhd_h2_conn_h2_deinit_start_closing (c);
    return false;
  }

  return true;
}
