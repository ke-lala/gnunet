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
 * @file src/mhd2/sckt_send.h
 * @brief  The declaration of the mhd_sckt_send() function
 * @author Karlson2k (Evgeny Grin)
 *
 * The function is defined in the mhd_send.c file
 */

#ifndef MHD_SCKT_SEND_H
#define MHD_SCKT_RECV_H 1

#include "mhd_sys_options.h"

#include "sys_sizet_type.h"

#include "mhd_socket_error.h"

struct mhd_ConnSocket; /* Forward declaration */

/**
 * Send the data over the network socket.
 *
 * Clear #mhd_SOCKET_NET_STATE_SEND_READY in @a sk->ready if necessary.
 *
 * @param sk the socket data
 * @param buf_size the size of the data @a buf buffer
 * @param buf the buffer with the data to send
 * @param push_data set to 'false' if it is know that the data in the @a buf
 *                  is incomplete (message or chunk),
 *                  set to 'true' if the data is complete or the final part
 * @param[out] sent the pointer to variable to set the size of the data
 *                  actually sent
 * @return mhd_SOCKET_ERR_NO_ERROR if receive succeed (the @a received gets
 *         the received size) or socket error
 */
MHD_INTERNAL enum mhd_SocketError
mhd_sckt_send (struct mhd_ConnSocket *restrict sk,
               size_t buf_size,
               const char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
               bool push_data,
               size_t *restrict sent)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_OUT_ (5);


#endif /* ! MHD_SCKT_RECV_H */
