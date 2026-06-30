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
 * @file src/mhd2/tls_mbed_conn_data.h
 * @brief  The definition of MbedTLS connection-specific data structures
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_MBED_CONN_DATA_H
#define MHD_TLS_MBED_CONN_DATA_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_MBEDTLS
#error This header can be used only if MbedTLS is enabled
#endif

#include "sys_bool_type.h"
#include "sys_sizet_type.h"

#include "tls_mbed_tls_lib.h"

#include "mhd_socket_error.h"

struct mhd_ConnSocket; /* Forward declaration */

#ifndef NDEBUG
struct mhd_TlsMbedConnDebug
{
  bool is_inited;
  bool is_tls_handshake_completed;
  bool is_failed;
};
#endif /* ! NDEBUG */

/**
 * The state for the current "custom transport" operation
 */
struct mhd_TlsMbedConnCstmTrtState
{
  /**
   * 'true' if recv() callback has been called
   */
  bool recv_called;
  /**
   * The result of last call of recv().
   * #mhd_SOCKET_ERR_NO_ERROR if recv() has not been called.
   */
  enum mhd_SocketError recv_res;
  /**
   * 'true' if send() callback has been called
   */
  bool send_called;
  /**
   * The result of last call of send().
   * #mhd_SOCKET_ERR_NO_ERROR if send() has not been called.
   */
  enum mhd_SocketError send_res;
  /**
   * The size of the send() data before TLS encryption.
   * Zero if no application data is being sent.
   */
  size_t send_unenc_size;
};

/**
 * Data for connection's "custom transport"
 */
struct mhd_TlsMbedConnCstmTrtData
{
  /**
   * The pointer to the socket information data
   */
  struct mhd_ConnSocket *sk;
  /**
   * The state for the current "custom transport" operation
   */
  struct mhd_TlsMbedConnCstmTrtState state;
};

/**
 * The structure with connection-specific MbedTLS data
 *
 * @note Unlike other TLS backends this struct contains MbedTLS data itself,
 *       not just pointers.
 */
struct mhd_TlsMbedConnData
{
  /**
   * MbedTLS session data
   */
  mbedtls_ssl_context sess;

  /**
 * Data for connection's "custom transport"
   */
  struct mhd_TlsMbedConnCstmTrtData tr;
  /**
   * 'true' is already received data in waiting in TLS buffers
   */
  bool recv_data_in_buff;
  /**
   * 'true' if sent TLS shutdown "alert"
   */
  bool shut_tls_wr_sent;

  /**
   * 'true' if received EOF (the peer initiated TLS shut down)
   */
  bool shut_tls_wr_received;
#ifndef NDEBUG
  /**
   * Debugging data
   */
  struct mhd_TlsMbedConnDebug dbg;
#endif /* ! NDEBUG */
};

#endif /* ! MHD_TLS_MBED_CONN_DATA_H */
