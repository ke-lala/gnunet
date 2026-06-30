/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2015-2024 Evgeny Grin (Karlson2k)
  Copyright (C) 2007-2020 Daniel Pittman and Christian Grothoff

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
 * @file src/mhd2/data_process.c
 * @brief  The implementation of data receiving, sending and processing
 *         functions for connection
 * @author Karlson2k (Evgeny Grin)
 *
 * Based on the MHD v0.x code by Daniel Pittman, Christian Grothoff and other
 * contributors.
 */

#include "mhd_sys_options.h"

#include "conn_data_process.h"
#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_assume.h"
#include "mhd_unreachable.h"
#include "mhd_constexpr.h"

#include <string.h>

#include "mhd_daemon.h"
#include "mhd_connection.h"

#include "mhd_socket_error_funcs.h"

#include "daemon_logger.h"

#include "mhd_comm_layer_state.h"
#ifdef MHD_SUPPORT_HTTPS
#  include "conn_tls_check.h"
#endif /* MHD_SUPPORT_HTTPS */

#include "conn_data_recv.h"
#include "conn_data_send.h"
#include "stream_process_states.h"
#include "mhd_comm_layer_state.h"


mhd_static_inline enum mhd_CommLayerState
process_conn_layer (struct MHD_Connection *restrict c)
{
#ifdef MHD_SUPPORT_HTTPS
  if (mhd_C_HAS_TLS (c))
    return mhd_conn_tls_check (c);
#endif /* MHD_SUPPORT_HTTPS */

  return mhd_COMM_LAYER_OK;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_conn_process_recv_send_data (struct MHD_Connection *restrict c)
{
  bool send_ready_state_known;
  bool has_sock_err;
  bool data_processed;

  data_processed = false;

  mhd_assert (! c->suspended);
  if (c->resuming)
  {
    /* Fully resume the connection + call app callbacks for the data */
    if (! mhd_conn_process_data (c))
      return false;

    data_processed = true;
  }

  if (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err)
  {
    if (! mhd_conn_process_data (c))
      return false;
  }

  switch (process_conn_layer (c))
  {
  case mhd_COMM_LAYER_OK:
    break;        /* Connected, the data */
  case mhd_COMM_LAYER_PROCESSING:
    return true;  /* Not yet fully connected, too early for the data */
  case mhd_COMM_LAYER_BROKEN:
    return false; /* Connection is broken */
  default:
    mhd_UNREACHABLE ();
    return false;
  }

  /* The "send-ready" state is known if system polling call is edge-triggered
     (it always checks for both send- and recv-ready) or if connection needs
     sending (therefore "send-ready" was explicitly checked by sockets polling
     call). */
  send_ready_state_known =
    ((mhd_D_HAS_EDGE_TRIGG (c->daemon)) ||
     (0 != (MHD_EVENT_LOOP_INFO_SEND & c->event_loop_info)));
  has_sock_err =
    (0 != (mhd_SOCKET_NET_STATE_ERROR_READY & c->sk.ready));
  mhd_assert (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err);

  if (0 != (MHD_EVENT_LOOP_INFO_RECV & c->event_loop_info))
  {
    bool use_recv;
    use_recv = (0 != (mhd_SOCKET_NET_STATE_RECV_READY
                      & (c->sk.ready | mhd_C_HAS_TLS_DATA_IN (c))));
    use_recv = use_recv ||
               (has_sock_err && c->sk.props.is_nonblck);

    if (use_recv)
    {
      mhd_conn_data_recv (c, has_sock_err);
      mhd_assert (! has_sock_err ||
                  (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err));
      if (! mhd_C_IS_HTTP2 (c))
      {
        if (! mhd_conn_process_data (c))
          return false;
        data_processed = true;
        mhd_ASSUME (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err);
      }
    }
  }

  if (0 != (MHD_EVENT_LOOP_INFO_SEND & c->event_loop_info))
  {
    bool use_send;
    /* Perform sending if:
     * + connection is ready for sending or
     * + just formed send data, connection send ready status is not known and
     *   connection socket is non-blocking
     * + detected network error on the connection, to check for the error */
    /* Assuming that after finishing receiving phase, connection send system
       buffers should have some space as sending was performed before receiving
       or has not been performed yet. */
    use_send = (0 != (mhd_SOCKET_NET_STATE_SEND_READY & c->sk.ready));

    /* Do not try to send if connection is broken when receiving */
    use_send = use_send && (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err);

    if (! mhd_C_IS_HTTP2 (c))
    {
      use_send = use_send ||
                 (data_processed && (! send_ready_state_known)
                  && c->sk.props.is_nonblck);
      use_send = use_send ||
                 (has_sock_err && c->sk.props.is_nonblck);
    }

    if (use_send)
    {
      mhd_conn_data_send (c);
      mhd_assert (! has_sock_err ||
                  (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err));
      if (! mhd_C_IS_HTTP2 (c))
      {
        if (! mhd_conn_process_data (c))
          return false;
        data_processed = true;
        mhd_ASSUME (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err);
      }
    }
  }

  if (mhd_SCKT_NET_ST_HAS_FLAG (c->sk.ready,
                                mhd_SOCKET_NET_STATE_ERROR_READY) &&
      (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err))
  {
    c->sk.state.discnt_err = mhd_socket_error_get_from_socket (c->sk.fd);
    mhd_ASSUME (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err);
  }

  if (! data_processed ||
      mhd_C_IS_HTTP2 (c) ||
      (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err))
    return mhd_conn_process_data (c);

  return true;
}
