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
 * @file src/mhd2/tls_open_funcs.h
 * @brief  The declarations of OpenSSL interface wrapper functions
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_OPEN_FUNCS_H
#define MHD_TLS_OPEN_FUNCS_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_OPENSSL
#error This header can be used only if OpenSSL is enabled
#endif

/* Sanity check */
#ifndef mhd_HAVE_TLS_THREAD_CLEANUP
#error mhd_HAVE_TLS_THREAD_CLEANUP macro must be defined when OpenSSL is used
#endif

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_status_code_int.h"

#include "mhd_tls_enums.h"
#include "mhd_socket_error.h"

/**
 * The structure with daemon-specific OpenSSL data
 */
struct mhd_TlsOpenDaemonData;    /* Forward declaration */

/**
 * The structure with connection-specific OpenSSL data
 */
struct mhd_TlsOpenConnData;      /* Forward declaration */

union MHD_ConnInfoDynamicTlsSess; /* Forward declaration */

struct mhd_StctTlsVersion;       /* Forward declaration */


/* ** Global initialisation / de-initialisation ** */

/**
 * Globally initialise OpenSSL backend.
 * Once initialised, this backend cannot be de-initialised.
 */
MHD_INTERNAL void
mhd_tls_open_global_init_once (void);

/* No-op for OpenSSL backend */
#define mhd_tls_open_global_re_init()    ((void) 0)

/* No-op for OpenSSL backend */
#define mhd_tls_open_global_deinit()     ((void) 0)

/**
 * Check whether OpenSSL backend was successfully initialised globally
 * @return 'true' if backend has been successfully initialised,
 *         'false' if backend cannot be used
 */
MHD_INTERNAL bool
mhd_tls_open_is_inited_fine (void)
MHD_FN_PURE_;


/* ** Daemon initialisation / de-initialisation ** */

struct MHD_Daemon;      /* Forward declaration */
struct DaemonOptions;   /* Forward declaration */

/**
 * Check whether OpenSSL backend supports edge-triggered sockets polling
 * @param s the daemon settings
 * @return 'true' if the backend supports edge-triggered sockets polling,
 *         'false' if edge-triggered sockets polling cannot be used
 */
#define mhd_tls_open_is_edge_trigg_supported(s) (! ! 0)


/**
 * Allocate and initialise daemon TLS parameters
 * @param d the daemon handle
 * @param sk_edge_trigg if 'true' then sockets polling uses edge-triggering
 * @param s the daemon settings
 * @param p_d_tls the pointer to variable to set the pointer to
 *                the daemon's TLS settings (allocated by this function)
 * @return #MHD_SC_OK on success (p_d_tls set to the allocated settings),
 *         error code otherwise
 */
MHD_INTERNAL mhd_StatusCodeInt
mhd_tls_open_daemon_init (struct MHD_Daemon *restrict d,
                          bool sk_edge_trigg,
                          struct DaemonOptions *restrict s,
                          struct mhd_TlsOpenDaemonData **restrict p_d_tls)
MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (4);

/**
 * De-initialise daemon TLS parameters (and free memory allocated for TLS
 * settings)
 * @param d_tls the pointer to the daemon's TLS settings
 */
MHD_INTERNAL void
mhd_tls_open_daemon_deinit (struct mhd_TlsOpenDaemonData *restrict d_tls)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/**
 * Perform clean-up of TLS resources before thread closing.
 * Must be called before thread is closed, after any use of TLS functions
 * in the thread, but before de-initialisation of daemon's TLS data.
 * @param d_tls the pointer to the daemon's TLS settings
 */
MHD_INTERNAL void
mhd_tls_open_thread_cleanup (struct mhd_TlsOpenDaemonData *restrict d_tls)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_INOUT_ (1);

/* ** Connection initialisation / de-initialisation ** */

struct mhd_ConnSocket; /* Forward declaration */

/**
 * Get size size of the connection's TLS settings
 */
MHD_INTERNAL size_t
mhd_tls_open_conn_get_tls_size_v (void);

/**
 * Get size size of the connection's TLS settings
 * @param d_tls the pointer to  the daemon's TLS settings
 */
#define mhd_tls_open_conn_get_tls_size(d_tls) \
        mhd_tls_open_conn_get_tls_size_v ()

/**
 * Initialise connection TLS settings
 * @param d_tls the daemon TLS settings
 * @param sk data about the socket for the connection
 * @param[out] c_tls the pointer to the allocated space for
 *                   the connection TLS settings
 * @return 'true' on success,
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_tls_open_conn_init (const struct mhd_TlsOpenDaemonData *restrict d_tls,
                        const struct mhd_ConnSocket *sk,
                        struct mhd_TlsOpenConnData *restrict c_tls)
MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (3);

/**
 * De-initialise connection TLS settings.
 * The provided pointer is not freed/deallocated.
 * @param c_tls the initialised connection TLS settings
 */
MHD_INTERNAL void
mhd_tls_open_conn_deinit (struct mhd_TlsOpenConnData *restrict c_tls)
MHD_FN_PAR_NONNULL_ALL_;


/* ** TLS connection establishing ** */

/**
 * Perform TLS handshake
 * @param c_tls the connection TLS handle
 * @return #mhd_TLS_PROCED_SUCCESS if completed successfully
 *         or other enum mhd_TlsProcedureResult values
 */
MHD_INTERNAL enum mhd_TlsProcedureResult
mhd_tls_open_conn_handshake (struct mhd_TlsOpenConnData *restrict c_tls)
MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_;

/**
 * Perform shutdown of TLS layer
 * @param c_tls the connection TLS handle
 * @return #mhd_TLS_PROCED_SUCCESS if completed successfully
 *         or other enum mhd_TlsProcedureResult values
 */
MHD_INTERNAL enum mhd_TlsProcedureResult
mhd_tls_open_conn_shutdown (struct mhd_TlsOpenConnData *restrict c_tls)
MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_;


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
MHD_INTERNAL enum mhd_SocketError
mhd_tls_open_conn_recv (struct mhd_TlsOpenConnData *restrict c_tls,
                        size_t buf_size,
                        char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                        size_t *restrict received)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4);

/**
 * Check whether any incoming data is pending in the TLS buffers
 *
 * @param c_tls the connection TLS handle
 * @return 'true' if any incoming remote data is already pending (the TLS recv()
 *          call can be performed),
 *         'false' otherwise
 */
MHD_INTERNAL bool
mhd_tls_open_conn_has_data_in (struct mhd_TlsOpenConnData *restrict c_tls)
MHD_FN_PAR_NONNULL_ALL_;

/**
 * Send data to the remote side over TLS connection
 *
 * @param c_tls the connection TLS handle
 * @param buf_size the size of the @a buf (in bytes)
 * @param buf content of the buffer to send
 * @param[out] sent the pointer to get amount of actually sent bytes
 * @return mhd_SOCKET_ERR_NO_ERROR if send succeed (the @a sent gets
 *         the sent size) or socket error
 */
MHD_INTERNAL enum mhd_SocketError
mhd_tls_open_conn_send4 (struct mhd_TlsOpenConnData *restrict c_tls,
                         size_t buf_size,
                         const char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                         size_t *restrict sent)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_IN_SIZE_ (3,2) MHD_FN_PAR_OUT_ (4);

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
#define mhd_tls_open_conn_send(c_tls,buf_size,buf,push_data,sent) \
        mhd_tls_open_conn_send4 (c_tls,buf_size,buf,sent)


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
#define mhd_tls_open_conn_has_cstm_tr(c_tls)    (! ! 0)

/**
 * Get the TLS session used in connection
 * @param c_tls the connection TLS handle
 * @param tls_sess_out the pointer to variable to be set to the TLS session
 *                     handle
 */
MHD_INTERNAL void
mhd_tls_open_conn_get_tls_sess (
  struct mhd_TlsOpenConnData *restrict c_tls,
  union MHD_ConnInfoDynamicTlsSess *restrict tls_sess_out)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (2);

/**
 * Get the TLS version used in connection
 * @param c_tls the connection TLS handle
 * @param tls_ver_out the pointer to variable to be set to the TLS version
 * @return always 'true'
 */
MHD_INTERNAL bool
mhd_tls_open_conn_get_tls_ver (struct mhd_TlsOpenConnData *restrict c_tls,
                               struct mhd_StctTlsVersion *restrict tls_ver_out)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PAR_OUT_ (2);

/**
 * Get a protocol selected by ALPN
 * @param c_tls the connection TLS handle
 * @return the selected protocol code
 */
MHD_INTERNAL enum mhd_TlsAlpnProt
mhd_tls_open_conn_get_alpn_prot (struct mhd_TlsOpenConnData *restrict c_tls)
MHD_FN_PAR_NONNULL_ALL_;

#endif /* ! MHD_TLS_OPEN_FUNCS_H */
