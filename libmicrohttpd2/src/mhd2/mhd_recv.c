/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_recv.c
 * @brief  The implementation of the mhd_recv() function
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_connection.h"

#include "mhd_assert.h"

#ifdef MHD_SUPPORT_HTTPS
#  include "mhd_tls_funcs.h"
#endif

#include "sckt_recv.h"

#include "mhd_recv.h"


#ifdef MHD_SUPPORT_HTTPS

static MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4) enum mhd_SocketError
mhd_recv_tls (struct MHD_Connection *restrict c,
              size_t buf_size,
              char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
              size_t *restrict received)
{
  /* TLS connection */
  enum mhd_SocketError res;

  mhd_assert (mhd_C_HAS_TLS (c));
  mhd_assert (0 != buf_size);

  res = mhd_tls_conn_recv (c->tls,
                           buf_size,
                           buf,
                           received);
  c->tls_has_data_in = mhd_TLS_BUF_NO_DATA; /* Updated with the actual value below */

  if (mhd_SOCKET_ERR_AGAIN == res)
    c->sk.ready = (enum mhd_SocketNetState) /* Clear 'recv-ready' */
                  (((unsigned int) c->sk.ready)
                   & (~(enum mhd_SocketNetState)
                      mhd_SOCKET_NET_STATE_RECV_READY));
  else if (mhd_SOCKET_ERR_NO_ERROR == res)
  {
    if (! c->sk.props.is_nonblck)
      c->sk.ready = (enum mhd_SocketNetState) /* Clear 'recv-ready' */
                    (((unsigned int) c->sk.ready)
                     & (~(enum mhd_SocketNetState)
                        mhd_SOCKET_NET_STATE_RECV_READY));
    if (*received == buf_size)
    {
      if (mhd_tls_conn_has_data_in (c->tls))
        c->tls_has_data_in = mhd_TLS_BUF_HAS_DATA_IN;
    }
#ifndef NDEBUG
    else
      mhd_assert (! mhd_tls_conn_has_data_in (c->tls));
#endif
  }

  return res;
}


#endif /* MHD_SUPPORT_HTTPS */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4) enum mhd_SocketError
mhd_recv (struct MHD_Connection *restrict c,
          size_t buf_size,
          char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
          size_t *restrict received)
{
  mhd_assert (mhd_HTTP_STAGE_CLOSED != c->stage);

#ifdef MHD_SUPPORT_HTTPS
  if (mhd_C_HAS_TLS (c))
    return mhd_recv_tls (c,
                         buf_size,
                         buf,
                         received);
#endif /* MHD_SUPPORT_HTTPS */

  return mhd_sckt_recv (&(c->sk),
                        buf_size,
                        buf,
                        received);
}
