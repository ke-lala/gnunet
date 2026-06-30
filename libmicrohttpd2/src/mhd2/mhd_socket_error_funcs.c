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
 * @file src/mhd2/mhd_socket_error_funcs.c
 * @brief  The definition of mhd_SocketError-related functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"
#include "mhd_socket_error_funcs.h"
#include "sys_sockets_headers.h"
#include "mhd_sockets_macros.h"
#include "sys_sockets_types.h"

MHD_INTERNAL enum mhd_SocketError
mhd_socket_error_get_from_sys_err (int socket_err)
{
  if (mhd_SCKT_ERR_IS_EAGAIN (socket_err))
    return mhd_SOCKET_ERR_AGAIN;
  else if (mhd_SCKT_ERR_IS_EINTR (socket_err))
    return mhd_SOCKET_ERR_INTR;
  else if (mhd_SCKT_ERR_IS_CONNRESET (socket_err))
    return mhd_SOCKET_ERR_CONNRESET;
  else if (mhd_SCKT_ERR_IS_CONN_BROKEN (socket_err))
    return mhd_SOCKET_ERR_CONN_BROKEN;
  else if (mhd_SCKT_ERR_IS_PIPE (socket_err))
    return mhd_SOCKET_ERR_PIPE;
  else if (mhd_SCKT_ERR_IS_NOTCONN (socket_err))
    return mhd_SOCKET_ERR_NOTCONN;
  else if (mhd_SCKT_ERR_IS_LOW_MEM (socket_err))
    return mhd_SOCKET_ERR_NOMEM;
  else if (mhd_SCKT_ERR_IS_BADF (socket_err))
    return mhd_SOCKET_ERR_BADF;
  else if (mhd_SCKT_ERR_IS_EINVAL (socket_err))
    return mhd_SOCKET_ERR_INVAL;
  else if (mhd_SCKT_ERR_IS_OPNOTSUPP (socket_err))
    return mhd_SOCKET_ERR_OPNOTSUPP;
  else if (mhd_SCKT_ERR_IS_NOTSOCK (socket_err))
    return mhd_SOCKET_ERR_NOTSOCK;

  return mhd_SOCKET_ERR_OTHER;
}


MHD_INTERNAL enum mhd_SocketError
mhd_socket_error_get_from_socket (MHD_Socket fd)
{
#if defined(SOL_SOCKET) && defined(SO_ERROR)
  enum mhd_SocketError err;
  int sock_err;
  socklen_t optlen = sizeof (sock_err);

  sock_err = 0;
  if ((0 == mhd_getsockopt (fd,
                            SOL_SOCKET,
                            SO_ERROR,
                            (void *) &sock_err,
                            &optlen))
      && (sizeof(sock_err) == optlen))
    return mhd_socket_error_get_from_sys_err (sock_err);

  err = mhd_socket_error_get_from_sys_err (mhd_SCKT_GET_LERR ());
  if ((mhd_SOCKET_ERR_NOTSOCK == err) ||
      (mhd_SOCKET_ERR_BADF == err))
    return err;
#endif /* SOL_SOCKET && SO_ERROR */
  return mhd_SOCKET_ERR_NOT_CHECKED;
}
