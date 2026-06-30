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
 * @file src/mhd2/sckt_recv.c
 * @brief  The definition of the mhd_sckt_recv() function
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_base_types.h"

#include "mhd_socket_type.h"
#include "sys_sockets_headers.h"
#include "mhd_sockets_macros.h"

#include "mhd_assert.h"

#include "mhd_limits.h"

#include "mhd_socket_error.h"

#include "mhd_conn_socket.h"

#include "mhd_socket_error_funcs.h"

#include "sckt_recv.h"


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4) enum mhd_SocketError
mhd_sckt_recv (struct mhd_ConnSocket *restrict sk,
               size_t buf_size,
               char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
               size_t *restrict received)
{
  ssize_t res;
  enum mhd_SocketError err;

  mhd_assert (MHD_INVALID_SOCKET != sk->fd);

  if (MHD_SCKT_SEND_MAX_SIZE_ < buf_size)
    buf_size = MHD_SCKT_SEND_MAX_SIZE_;

  res = mhd_sys_recv (sk->fd, buf, buf_size);
  if (0 <= res)
  {
    /* When the socket is blocking, always clean "recv-ready" flag
       after successful read as complete data could be already retrieved
       and no more data is pending.
       Note: blocking sockets are never used with edge-triggering. */
    if ((buf_size > (size_t) res)
        || ! sk->props.is_nonblck)
      mhd_SCKT_NET_ST_CLEAR_FLAG (&(sk->ready),
                                  mhd_SOCKET_NET_STATE_RECV_READY);

    *received = (size_t) res;

    return mhd_SOCKET_ERR_NO_ERROR; /* Success exit point */
  }

  err = mhd_socket_error_get_from_sys_err (mhd_SCKT_GET_LERR ());

  if (mhd_SOCKET_ERR_AGAIN == err)
    mhd_SCKT_NET_ST_CLEAR_FLAG (&(sk->ready),
                                mhd_SOCKET_NET_STATE_RECV_READY);

  return err; /* Failure exit point */
}
