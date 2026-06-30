/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2017-2025 Evgeny Grin (Karlson2k)
  Copyright (C) 2019 ng0

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
 * @file src/mhd2/mhd_send.h
 * @brief Declarations of send() wrappers.
 * @author Karlson2k (Evgeny Grin)
 * @author ng0 (N. Gillmann)
 */

#ifndef MHD_SEND_H
#define MHD_SEND_H

#include "mhd_sys_options.h"

#include "sys_base_types.h"
#include "sys_bool_type.h"
#include "mhd_socket_error.h"

struct MHD_Connection; /* forward declaration */
struct mhd_iovec_track; /* forward declaration */

/**
 * Initialises static variables.
 * Need to be called only once.
 */
void
mhd_send_init_once (void);


/**
 * Send buffer to the client, push data from network buffer if requested
 * and full buffer is sent.
 *
 * @param connection the MHD_Connection structure
 * @param buf_size the size of the @a buf (in bytes)
 * @param buf content of the buffer to send
 * @param push_data set to true to force push the data to the network from
 *                  system buffers (usually set for the last piece of data),
 *                  set to false to prefer holding incomplete network packets
 *                  (more data will be send for the same reply).
 * @param[out] sent the pointer to get amount of actually sent bytes
 * @return mhd_SOCKET_ERR_NO_ERROR if send succeed (the @a sent gets
 *         the sent size) or socket error
 */
MHD_INTERNAL enum mhd_SocketError
mhd_send_data (struct MHD_Connection *restrict connection,
               size_t buf_size,
               const char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
               bool push_data,
               size_t *restrict sent)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_OUT_ (5);


/**
 * Send reply header with optional reply body.
 *
 * @param connection the MHD_Connection structure
 * @param header content of header to send
 * @param header_size the size of the @a header (in bytes)
 * @param never_push_hdr set to true to disable internal algorithm
 *                       that can push automatically header data
 *                       alone to the network
 * @param body content of the body to send (optional, may be NULL)
 * @param body_size the size of the @a body (in bytes)
 * @param complete_response set to true if complete response
 *                          is provided by @a header and @a body,
 *                          set to false if additional body data
 *                          will be sent later
 * @param[out] sent the pointer to get amount of actually sent bytes
 *                  in total (from both buffers combined)
 * @return mhd_SOCKET_ERR_NO_ERROR if send succeed (the @a sent gets
 *         the sent size) or socket error
 */
MHD_INTERNAL enum mhd_SocketError
mhd_send_hdr_and_body (struct MHD_Connection *restrict connection,
                       size_t header_size,
                       const char *restrict header,
                       bool never_push_hdr,
                       size_t body_size,
                       const char *restrict body,
                       bool complete_response,
                       size_t *restrict sent)
MHD_FN_PAR_NONNULL_(1) MHD_FN_PAR_NONNULL_(3)
MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_IN_SIZE_ (6,5) MHD_FN_PAR_OUT_ (8);

#if defined(mhd_USE_SENDFILE)
/**
 * Function for sending responses backed by file FD.
 *
 * @param c the MHD connection structure
 * @param[out] sent the pointer to get amount of actually sent bytes
 *                  in total (from both buffers combined)
 * @return mhd_SOCKET_ERR_NO_ERROR if send succeed (the @a sent gets
 *         the sent size) or socket error
 */
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (2) enum mhd_SocketError
mhd_send_sendfile (struct MHD_Connection *restrict c,
                   size_t *restrict sent)
MHD_FN_PAR_NONNULL_ALL_;

#endif


/**
 * Function for sending responses backed by a an array of memory buffers.
 *
 * @param connection the MHD connection structure
 * @param r_iov the pointer to iov response structure with tracking
 * @param push_data set to true to force push the data to the network from
 *                  system buffers (usually set for the last piece of data),
 *                  set to false to prefer holding incomplete network packets
 *                  (more data will be send for the same reply).
 * @param[out] sent the pointer to get amount of actually sent bytes
 *                  in total (from both buffers combined)
 * @return mhd_SOCKET_ERR_NO_ERROR if send succeed (the @a sent gets
 *         the sent size) or socket error
 */
MHD_INTERNAL enum mhd_SocketError
mhd_send_iovec (struct MHD_Connection *restrict connection,
                struct mhd_iovec_track *const restrict r_iov,
                bool push_data,
                size_t *restrict sent)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (4);


#endif /* MHD_SEND_H */
