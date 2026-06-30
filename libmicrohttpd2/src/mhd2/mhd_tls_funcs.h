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
 * @file src/mhd2/mhd_tls_funcs.h
 * @brief  The TLS backend functions generic declaration, mapped to specific TLS
 *         backend at compile-time
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_FUNCS_H
#define MHD_TLS_FUNCS_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

#include "mhd_tls_choice.h"
#ifndef MHD_SUPPORT_HTTPS
#error This header should be used only if HTTPS is enabled
#endif

#if defined(MHD_USE_MULTITLS)
#  include "tls_multi_funcs.h"
#elif defined(MHD_SUPPORT_GNUTLS)
#  include "tls_gnu_funcs.h"
#elif defined(MHD_SUPPORT_OPENSSL)
#  include "tls_open_funcs.h"
#elif defined(MHD_SUPPORT_MBEDTLS)
#  include "tls_mbed_funcs.h"
#endif

/* ** Global initialisation / de-initialisation ** */

/**
 * Perform one-time global initialisation of TLS backend
 */
#define mhd_tls_global_init_once()        mhd_TLS_FUNC (_global_init_once)()

/**
 * Perform de-initialisation of TLS backend
 */
#define mhd_tls_global_deinit()           mhd_TLS_FUNC (_global_deinit)()

/**
 * Perform re-initialisation of TLS backend
 */
#define mhd_tls_global_re_init()          mhd_TLS_FUNC (_global_re_init)()

/* ** Daemon initialisation / de-initialisation ** */

/**
 * Check whether the selected backend supports edge-triggered sockets polling
 * @param s the daemon settings
 * @return 'true' if the backend supports edge-triggered sockets polling,
 *         'false' if edge-triggered sockets polling cannot be used
 */
#define mhd_tls_is_edge_trigg_supported(s) \
        mhd_TLS_FUNC (_is_edge_trigg_supported)((s))

/**
 * Allocate and initialise daemon TLS parameters
 * @param d the daemon handle
 * @param et if 'true' then sockets polling uses edge-triggering
 * @param s the daemon settings
 * @param p_d_tls the pointer to variable to set the pointer to
 *                the daemon's TLS settings (allocated by this function)
 * @return #MHD_SC_OK on success (p_d_tls set to the allocated settings),
 *         error code otherwise
 */
#define mhd_tls_daemon_init(d,et,s,p_d_tls)        \
        mhd_TLS_FUNC (_daemon_init)((d),(et),(s),(p_d_tls))

/**
 * De-initialise daemon TLS parameters (and free memory allocated for TLS
 * settings)
 * @param d_tls the pointer to  the daemon's TLS settings
 */
#define mhd_tls_daemon_deinit(d_tls)    \
        mhd_TLS_FUNC (_daemon_deinit)((d_tls))

/**
 * Perform clean-up of TLS resources before thread closing.
 * Must be called before thread is closed, after any use of TLS functions
 * in the thread, but before de-initialisation of daemon's TLS data.
 * @param d_tls the pointer to the daemon's TLS settings
 */
#define mhd_tls_thread_cleanup(d_tls)    \
        mhd_TLS_FUNC (_thread_cleanup)((d_tls))


/* ** Connection initialisation / de-initialisation ** */

/**
 * Get size size of the connection's TLS settings
 * @param d_tls the pointer to  the daemon's TLS settings
 */
#define mhd_tls_conn_get_tls_size(d_tls)     \
        mhd_TLS_FUNC (_conn_get_tls_size)(d_tls)

/**
 * Initialise connection TLS settings
 * @param d_tls the daemon TLS settings
 * @param sk data about the socket for the connection
 * @param[out] c_tls the pointer to the allocated space for
 *                   the connection TLS settings
 * @return 'true' on success,
 *         'false' otherwise
 */
#define mhd_tls_conn_init(d_tls,sk,c_tls)       \
        mhd_TLS_FUNC (_conn_init)((d_tls),(sk),(c_tls))

/**
 * De-initialise connection TLS settings.
 * The provided pointer is not freed/deallocated.
 * @param c_tls the initialised connection TLS settings
 */
#define mhd_tls_conn_deinit(c_tls)       \
        mhd_TLS_FUNC (_conn_deinit)((c_tls))


/* ** TLS connection establishing ** */

/**
 * Perform TLS handshake
 * @param c_tls the connection TLS handle
 * @return #mhd_TLS_PROCED_SUCCESS if completed successfully
 *         or other enum mhd_TlsProcedureResult values
 */
#define mhd_tls_conn_handshake(c_tls)       \
        mhd_TLS_FUNC (_conn_handshake)((c_tls))

/**
 * Perform shutdown of TLS layer
 * @param c_tls the connection TLS handle
 * @return #mhd_TLS_PROCED_SUCCESS if completed successfully
 *         or other enum mhd_TlsProcedureResult values
 */
#define mhd_tls_conn_shutdown(c_tls)       \
        mhd_TLS_FUNC (_conn_shutdown)((c_tls))

/* ** Data sending and receiving over TLS connection ** */

/**
 * Receive the data from the remote side over TLS connection
 *
 * @param c_tls the connection TLS handle
 * @param buf_size the size of the @a buf buffer
 * @param[out] buf the buffer to fill with the received data
 * @param[out] received the pointer to variable to get the size of the data
 *                      actually put to the @a buffer
 * @return mhd_SOCKET_ERR_NO_ERROR if receive succeed (the @a received gets
 *         the received size) or socket error
 */
#define mhd_tls_conn_recv(c_tls,buf_size,buf,received)  \
        mhd_TLS_FUNC (_conn_recv)((c_tls),(buf_size),(buf),(received))

/**
 * Check whether any incoming data is pending in the TLS buffers
 *
 * @param c_tls the connection TLS handle
 * @return 'true' if any incoming remote data is already pending (the TLS recv()
 *          call can be performed),
 *         'false' otherwise
 */
#define mhd_tls_conn_has_data_in(c_tls)       \
        mhd_TLS_FUNC (_conn_has_data_in)((c_tls))

/**
 * Send data to the remote side over TLS connection
 *
 * @param c_tls the connection TLS handle
 * @param buf_size the size of the @a buf (in bytes)
 * @param buf content of the buffer to send
 * @param push_data set to 'false' if it is know that the data in the @a buf
 *                  is incomplete (message or chunk),
 *                  set to 'true' if the data is complete or the final part
 * @param[out] sent the pointer to get amount of actually sent bytes
 * @return mhd_SOCKET_ERR_NO_ERROR if send succeed (the @a sent gets
 *         the sent size) or socket error
 */
#define mhd_tls_conn_send(c_tls,buf_size,buf,push_data,sent)      \
        mhd_TLS_FUNC (_conn_send)((c_tls),(buf_size),(buf),(push_data),(sent))


/* ** TLS connection information ** */

/**
 * Check whether the connection is using "custom transport" functions.
 * "Custom transport" means that data sending and receiving over system
 * sockets is performed by MHD callbacks.
 * When "custom transport" is used, backend TLS send/recv functions are:
 * * perform additional syscalls (socket options) for data pushing/buffering,
 * * change socket states like corked, NO_DELAY, both by syscalls and in
 *   MHD socket metadata,
 * * set disconnect error from the system reported socket error.
 *
 * @param c_tls the connection TLS handle
 * @return boolean 'true' if custom transport is used,
 *         boolean 'false' otherwise
 */
#define mhd_tls_conn_has_cstm_tr(c_tls)       \
        mhd_TLS_FUNC (_conn_has_cstm_tr)((c_tls))

/**
 * Get the TLS session used in connection
 * @param c_tls the connection TLS handle
 * @param tls_ver_out the pointer to variable to be set to the TLS version
 */
#define mhd_tls_conn_get_tls_sess(c_tls,tls_sess_out) \
        mhd_TLS_FUNC (_conn_get_tls_sess)((c_tls),(tls_sess_out))

/**
 * Get the TLS version used in connection
 * @param c_tls the connection TLS handle
 * @param tls_ver_out the pointer to variable to be set to the TLS version
 * @return 'true' is TLS version information set successfully,
 *         'false' if TLS version information cannot be obtained or mapped
 */
#define mhd_tls_conn_get_tls_ver(c_tls,tls_ver_out)     \
        mhd_TLS_FUNC (_conn_get_tls_ver)((c_tls),(tls_ver_out))

/**
 * Get a protocol selected by ALPN
 * @param c_tls the connection TLS handle
 * @return the selected protocol code
 */
#define mhd_tls_conn_get_alpn_prot(c_tls)       \
        mhd_TLS_FUNC (_conn_get_alpn_prot)((c_tls))

#endif /* ! MHD_TLS_FUNCS_H */
