/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024-2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/conn_data_recv.c
 * @brief  The implementation of data receiving functions for connection
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "conn_data_recv.h"

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"

#include "mhd_connection.h"

#include "conn_timeout.h"
#include "stream_funcs.h"
#include "mhd_socket_error_funcs.h"
#include "sckt_recv.h"

#include "mhd_recv.h"

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_conn_data_recv (struct MHD_Connection *c,
                    bool has_err)
{
  void *buf;
  size_t buf_size;
  size_t received;
  enum mhd_SocketError res;

  mhd_assert (mhd_HTTP_STAGE_CLOSED != c->stage);
  mhd_assert (NULL != c->read_buffer);
  mhd_assert (c->read_buffer_size > c->read_buffer_offset);
  mhd_assert (! has_err || \
              (0 != (c->sk.ready & mhd_SOCKET_NET_STATE_ERROR_READY)));
  mhd_assert ((0 == (c->sk.ready & mhd_SOCKET_NET_STATE_ERROR_READY)) || \
              has_err);
  mhd_assert (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err);

  buf = c->read_buffer + c->read_buffer_offset;
  buf_size = c->read_buffer_size - c->read_buffer_offset;

  res = mhd_recv (c,
                  buf_size,
                  (char *) buf,
                  &received);

  if ((mhd_SOCKET_ERR_NO_ERROR != res) || has_err)
  {
    /* Handle errors */
    if ((mhd_SOCKET_ERR_NO_ERROR == res) && (0 == received))
    {
      c->sk.state.rmt_shut_wr = true;
      res = mhd_SOCKET_ERR_REMT_DISCONN;
    }

    if (has_err && (mhd_SOCKET_ERR_NO_ERROR == c->sk.state.discnt_err))
    {
      /* Try to get the real error from the socket */
      if (! mhd_SOCKET_ERR_IS_HARD (res) && c->sk.props.is_nonblck)
      {
        /* Re-try the last time with direct socket recv() to detect the error */
        uint_fast64_t dummy_buf;
        res = mhd_sckt_recv (&(c->sk),
                             sizeof(dummy_buf),
                             (char *) &dummy_buf,
                             &received);
      }
      if (mhd_SOCKET_ERR_IS_HARD (res))
      {
        c->sk.state.discnt_err = res;
        mhd_SCKT_NET_ST_SET_FLAG (&(c->sk.ready),
                                  mhd_SOCKET_NET_STATE_ERROR_READY);
      }
      else
      {
        c->sk.state.discnt_err = mhd_socket_error_get_from_socket (c->sk.fd);
        mhd_assert (mhd_SOCKET_ERR_NO_ERROR != c->sk.state.discnt_err);
      }
    }

    return;
  }

  if (0 == received)
    c->sk.state.rmt_shut_wr = true;

  c->read_buffer_offset += received;
  mhd_conn_update_activity_mark (c);
  return;
}


#if 0 // TODO: report disconnect
if ((bytes_read < 0) || socket_error)
{
  if (MHD_ERR_CONNRESET_ == bytes_read)
  {
    if ( (mhd_HTTP_STAGE_INIT < c->stage) &&
         (mhd_HTTP_STAGE_FULL_REQ_RECEIVED > c->stage) )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (c->daemon,
                _ ("Socket has been disconnected when reading request.\n"));
#endif
      c->discard_request = true;
    }
    MHD_connection_close_ (c,
                           MHD_REQUEST_TERMINATED_READ_ERROR);
    return;
  }

#ifdef HAVE_MESSAGES
  if (mhd_HTTP_STAGE_INIT != c->stage)
    MHD_DLOG (c->daemon,
              _ ("Connection socket is closed when reading " \
                 "request due to the error: %s\n"),
              (bytes_read < 0) ? str_conn_error_ (bytes_read) :
              "detected c closure");
#endif
  CONNECTION_CLOSE_ERROR (c,
                          NULL);
  return;
}

#if 0 // TODO: handle remote shut WR
if (0 == bytes_read)
{ /* Remote side closed c. */   // FIXME: Actually NOT!
  c->sk.state.rmt_shut_wr = true;
  if ( (mhd_HTTP_STAGE_INIT < c->stage) &&
       (mhd_HTTP_STAGE_FULL_REQ_RECEIVED > c->stage) )
  {
#ifdef HAVE_MESSAGES
    MHD_DLOG (c->daemon,
              _ ("Connection was closed by remote side with incomplete "
                 "request.\n"));
#endif
    c->discard_request = true;
    MHD_connection_close_ (c,
                           MHD_REQUEST_TERMINATED_CLIENT_ABORT);
  }
  else if (mhd_HTTP_STAGE_INIT == c->stage)
    /* This termination code cannot be reported to the application
     * because application has not been informed yet about this request */
    MHD_connection_close_ (c,
                           MHD_REQUEST_TERMINATED_COMPLETED_OK);
  else
    MHD_connection_close_ (c,
                           MHD_REQUEST_TERMINATED_WITH_ERROR);
  return;
}
#endif
#endif
