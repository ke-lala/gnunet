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
 * @file src/mhd2/tls_multi_funcs.c
 * @brief  The implementation of MultiTLS wrapper functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "compat_calloc.h"
#include "sys_malloc.h"

#include "mhd_arr_num_elems.h"

#include "tls_multi_tls_lib.h"

#include "tls_multi_daemon_data.h"
#include "tls_multi_conn_data.h"
#include "tls_multi_funcs.h"

/* Include all supported TLS backends headers */
#if defined(MHD_SUPPORT_GNUTLS)
#  include "tls_gnu_funcs.h"
#endif
#if defined(MHD_SUPPORT_OPENSSL)
#  include "tls_open_funcs.h"
#endif
#if defined(MHD_SUPPORT_MBEDTLS)
#  include "tls_mbed_funcs.h"
#endif

#include "daemon_options.h"
#include "daemon_logger.h"

#ifdef mhd_USE_TLS_DEBUG_MESSAGES
#  include <stdio.h> /* For TLS debug printing */
#endif

#include "mhd_public_api.h"

#ifdef mhd_USE_TLS_DEBUG_MESSAGES
#  define mhd_M_DEBUG_PRINT(msg) \
        do { fprintf (stderr, "## MultiTLS: " msg "\n"); \
             fflush (stderr); } while (0)
#  define mhd_M_DEBUG_PRINT1(msg,arg1) \
        do { fprintf (stderr, "## MultiTLS: " msg "\n", arg1); \
             fflush (stderr); } while (0)
#else
#  define mhd_M_DEBUG_PRINT(msg)        ((void) 0)
#  define mhd_M_DEBUG_PRINT1(msg,arg1)  ((void) 0)
#endif


/* ** Global initialisation / de-initialisation ** */

MHD_INTERNAL void
mhd_tls_multi_global_init_once (void)
{
#if defined(MHD_SUPPORT_GNUTLS)
  mhd_tls_gnu_global_init_once ();
#endif
#if defined(MHD_SUPPORT_OPENSSL)
  mhd_tls_open_global_init_once ();
#endif
#if defined(MHD_SUPPORT_MBEDTLS)
  mhd_tls_mbed_global_init_once ();
#endif
}


MHD_INTERNAL void
mhd_tls_multi_global_deinit (void)
{
  /* Note: the order is reversed to match the initialisation */
#if defined(MHD_SUPPORT_OPENSSL)
  mhd_tls_open_global_deinit ();
#endif
#if defined(MHD_SUPPORT_GNUTLS)
  mhd_tls_gnu_global_deinit ();
#endif
#if defined(MHD_SUPPORT_MBEDTLS)
  mhd_tls_mbed_global_deinit ();
#endif
}


MHD_INTERNAL void
mhd_tls_multi_global_re_init (void)
{
#if defined(MHD_SUPPORT_GNUTLS)
  mhd_tls_gnu_global_re_init ();
#endif
#if defined(MHD_SUPPORT_OPENSSL)
  mhd_tls_open_global_re_init ();
#endif
#if defined(MHD_SUPPORT_MBEDTLS)
  mhd_tls_mbed_global_re_init ();
#endif
}


/* ** Daemon initialisation / de-initialisation ** */

MHD_INTERNAL MHD_FN_PURE_ bool
mhd_tls_multi_is_edge_trigg_supported (struct DaemonOptions *s)
{
  switch (s->tls)
  {
  case MHD_TLS_BACKEND_NONE:
    mhd_UNREACHABLE ();
    return false;
  case MHD_TLS_BACKEND_ANY:
#ifdef MHD_SUPPORT_GNUTLS
    if (mhd_tls_gnu_is_edge_trigg_supported (s)
        && mhd_tls_gnu_is_inited_fine ())
      return true;
#endif
#ifdef MHD_SUPPORT_OPENSSL
    if (mhd_tls_open_is_edge_trigg_supported (s)
        && mhd_tls_open_is_inited_fine ())
      return true;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
    if (mhd_tls_mbed_is_edge_trigg_supported (s)
        && mhd_tls_mbed_is_inited_fine ())
      return true;
#endif
    return false;
  case MHD_TLS_BACKEND_GNUTLS:
#ifdef MHD_SUPPORT_GNUTLS
    /* Ignore "backend inited" status here,
       it will be checked on daemon TLS init */
    return mhd_tls_gnu_is_edge_trigg_supported (s);
#endif
    break;
  case MHD_TLS_BACKEND_OPENSSL:
#ifdef MHD_SUPPORT_OPENSSL
    /* Ignore "backend inited" status here,
       it will be checked on daemon TLS init */
    return mhd_tls_open_is_edge_trigg_supported (s);
#endif
    break;
  case MHD_TLS_BACKEND_MBEDTLS:
#ifdef MHD_SUPPORT_MBEDTLS
    /* Ignore "backend inited" status here,
       it will be checked on daemon TLS init */
    return mhd_tls_mbed_is_edge_trigg_supported (s);
#endif
    break;
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return false;
}


/**
 * Initialise selected TLS backend for the daemon
 * @param route the selected TLS backend
 * @param d the daemon handle
 * @param s the daemon settings
 * @param d_tls the daemon's TLS settings, backend-specific data is allocated
 *              and initialised
 * @return #MHD_SC_OK on success (the backend-specific data is allocated),
 *         error code otherwise
 */
static MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (5) mhd_StatusCodeInt
tls_daemon_init_try (enum mhd_TlsMultiRoute route,
                     struct MHD_Daemon *restrict d,
                     bool sk_edge_trigg,
                     struct DaemonOptions *restrict s,
                     struct mhd_TlsMultiDaemonData *restrict d_tls)
{
  mhd_StatusCodeInt res;

#ifndef MHD_SUPPORT_OPENSSL
  (void) sk_edge_trigg; /* Unused, mute compiler warning */
#endif /* ! MHD_SUPPORT_OPENSSL */

  switch (route)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    if (! mhd_tls_gnu_is_inited_fine ())
      return MHD_SC_TLS_BACKEND_UNAVAILABLE;
    res = mhd_tls_gnu_daemon_init (d,
                                   sk_edge_trigg,
                                   s,
                                   &(d_tls->data.gnutls));
    if (MHD_SC_OK == res)
    {
      mhd_M_DEBUG_PRINT ("GnuTLS backend initialised successfully " \
                         "for the daemon");
      d_tls->choice = route;
      return MHD_SC_OK;
    }
    mhd_M_DEBUG_PRINT1 ("Failed to initialise GnuTLS backend for " \
                        "the daemon, error code: %u", (unsigned) res);
    return res;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    if (! mhd_tls_open_is_inited_fine ())
      return MHD_SC_TLS_BACKEND_UNAVAILABLE;
    res = mhd_tls_open_daemon_init (d,
                                    sk_edge_trigg,
                                    s,
                                    &(d_tls->data.openssl));
    if (MHD_SC_OK == res)
    {
      mhd_M_DEBUG_PRINT ("OpenSSL backend initialised successfully " \
                         "for the daemon");
      d_tls->choice = route;
      return MHD_SC_OK;
    }
    mhd_M_DEBUG_PRINT1 ("Failed to initialise OpenSSL backend for " \
                        "the daemon, error code: %u", (unsigned) res);
    return res;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    if (! mhd_tls_mbed_is_inited_fine ())
      return MHD_SC_TLS_BACKEND_UNAVAILABLE;
    res = mhd_tls_mbed_daemon_init (d,
                                    sk_edge_trigg,
                                    s,
                                    &(d_tls->data.mbedtls));
    if (MHD_SC_OK == res)
    {
      mhd_M_DEBUG_PRINT ("MbedTLS backend initialised successfully " \
                         "for the daemon");
      d_tls->choice = route;
      return MHD_SC_OK;
    }
    mhd_M_DEBUG_PRINT1 ("Failed to initialise MbedTLS backend for " \
                        "the daemon, error code: %u", (unsigned) res);
    return res;
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    break;
  }
  mhd_assert (0 && "Impossible value");
  mhd_UNREACHABLE ();
  return MHD_SC_INTERNAL_ERROR;
}


MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (4) mhd_StatusCodeInt
mhd_tls_multi_daemon_init (struct MHD_Daemon *restrict d,
                           bool sk_edge_trigg,
                           struct DaemonOptions *restrict s,
                           struct mhd_TlsMultiDaemonData **restrict p_d_tls)
{
  mhd_StatusCodeInt res;
  struct mhd_TlsMultiDaemonData *restrict d_tls;
  d_tls = (struct mhd_TlsMultiDaemonData *)
          mhd_calloc (1,
                      sizeof (struct mhd_TlsMultiDaemonData));
  if (NULL == d_tls)
    return MHD_SC_DAEMON_MEM_ALLOC_FAILURE;

  res = MHD_SC_INTERNAL_ERROR; /* Mute compiler warning, the value should not be used */
  switch (s->tls)
  {
  case MHD_TLS_BACKEND_ANY:
    if (1)
    {
      size_t i;
      enum mhd_TlsMultiRoute backends[] = {
#ifdef MHD_SUPPORT_GNUTLS
        mhd_TLS_MULTI_ROUTE_GNU,
#endif
#ifdef MHD_SUPPORT_OPENSSL
        mhd_TLS_MULTI_ROUTE_OPEN,
#endif
#ifdef MHD_SUPPORT_MBEDTLS
        mhd_TLS_MULTI_ROUTE_MBED,
#endif
        mhd_TLS_MULTI_ROUTE_NONE  /* Not used */
      };
      /* Try backends one-by-one */
      for (i = 0; i < mhd_ARR_NUM_ELEMS (backends) - 1; ++i)
      {
        res = tls_daemon_init_try (backends[i],
                                   d,
                                   sk_edge_trigg,
                                   s,
                                   d_tls);
        if (MHD_SC_OK == res)
          break;
      }
    }
    break;
#ifdef MHD_SUPPORT_GNUTLS
  case MHD_TLS_BACKEND_GNUTLS:
    mhd_assert (mhd_tls_gnu_is_inited_fine ()); /* Must be checked earlier */
    res = tls_daemon_init_try (mhd_TLS_MULTI_ROUTE_GNU,
                               d,
                               sk_edge_trigg,
                               s,
                               d_tls);
    break;
#endif /* MHD_SUPPORT_GNUTLS */
#ifdef MHD_SUPPORT_OPENSSL
  case MHD_TLS_BACKEND_OPENSSL:
    mhd_assert (mhd_tls_open_is_inited_fine ()); /* Must be checked earlier */
    res = tls_daemon_init_try (mhd_TLS_MULTI_ROUTE_OPEN,
                               d,
                               sk_edge_trigg,
                               s,
                               d_tls);
#endif /* MHD_SUPPORT_OPENSSL */
    break;
#ifdef MHD_SUPPORT_MBEDTLS
  case MHD_TLS_BACKEND_MBEDTLS:
    mhd_assert (mhd_tls_mbed_is_inited_fine ()); /* Must be checked earlier */
    res = tls_daemon_init_try (mhd_TLS_MULTI_ROUTE_MBED,
                               d,
                               sk_edge_trigg,
                               s,
                               d_tls);
#endif /* MHD_SUPPORT_MBEDTLS */
    break;

#ifndef MHD_SUPPORT_GNUTLS
  case MHD_TLS_BACKEND_GNUTLS:
#endif /* ! MHD_SUPPORT_GNUTLS */
#ifndef MHD_SUPPORT_OPENSSL
  case MHD_TLS_BACKEND_OPENSSL:
#endif /* ! MHD_SUPPORT_OPENSSL */
#ifndef MHD_SUPPORT_MBEDTLS
  case MHD_TLS_BACKEND_MBEDTLS:
#endif /* ! MHD_SUPPORT_MBEDTLS */
  case MHD_TLS_BACKEND_NONE:
  default:
    mhd_UNREACHABLE ();
    res = MHD_SC_TLS_BACKEND_UNSUPPORTED;
    break;
  }
  mhd_assert (NULL != d_tls);
  if (MHD_SC_OK == res)
  {
    *p_d_tls = d_tls;
    return MHD_SC_OK;
  }
  free (d_tls);
  return res;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_tls_multi_daemon_deinit (struct mhd_TlsMultiDaemonData *restrict d_tls)
{
  switch (d_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    mhd_tls_gnu_daemon_deinit (d_tls->data.gnutls);
    break;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    mhd_tls_open_daemon_deinit (d_tls->data.openssl);
    break;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    mhd_tls_mbed_daemon_deinit (d_tls->data.mbedtls);
    break;
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  free (d_tls);
}


#ifdef mhd_HAVE_TLS_THREAD_CLEANUP
/**
 * Perform clean-up of TLS resources before thread closing.
 * Must be called before thread is closed, after any use of TLS functions
 * in the thread, but before de-initialisation of daemon's TLS data.
 * @param d_tls the pointer to the daemon's TLS settings
 */
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_INOUT_ (1) void
mhd_tls_multi_thread_cleanup (struct mhd_TlsMultiDaemonData *restrict d_tls)
{
  switch (d_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    mhd_tls_gnu_thread_cleanup (d_tls->data.gnutls);
    break;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    mhd_tls_open_thread_cleanup (d_tls->data.openssl);
    break;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    mhd_tls_mbed_thread_cleanup (d_tls->data.mbedtls);
    break;
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
}


#endif /* mhd_HAVE_TLS_THREAD_CLEANUP */

/* ** Connection initialisation / de-initialisation ** */

MHD_INTERNAL size_t
mhd_tls_multi_conn_get_tls_size (struct mhd_TlsMultiDaemonData *restrict d_tls)
{
  size_t data_size;

  data_size = sizeof(struct mhd_TlsMultiConnData);
  switch (d_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    data_size += mhd_tls_gnu_conn_get_tls_size (d_tls->data.gnutls);
    break;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    data_size += mhd_tls_open_conn_get_tls_size (d_tls->data.openssl);
    break;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    data_size += mhd_tls_mbed_conn_get_tls_size (d_tls->data.mbedtls);
    break;
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
  }

  return data_size;
}


MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (3) bool
mhd_tls_multi_conn_init (const struct mhd_TlsMultiDaemonData *restrict d_tls,
                         struct mhd_ConnSocket *sk,
                         struct mhd_TlsMultiConnData *restrict c_tls)
{
  c_tls->choice = d_tls->choice;
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    /* Assume the same alignment requirements for both structures */
    c_tls->data.gnutls = (struct mhd_TlsGnuConnData *) (c_tls + 1);
    return mhd_tls_gnu_conn_init (d_tls->data.gnutls,
                                  sk,
                                  c_tls->data.gnutls);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    /* Assume the same alignment requirements for both structures */
    c_tls->data.openssl = (struct mhd_TlsOpenConnData *) (c_tls + 1);
    return mhd_tls_open_conn_init (d_tls->data.openssl,
                                   sk,
                                   c_tls->data.openssl);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    /* Assume the same alignment requirements for both structures */
    c_tls->data.mbedtls = (struct mhd_TlsMbedConnData *) (c_tls + 1);
    return mhd_tls_mbed_conn_init (d_tls->data.mbedtls,
                                   sk,
                                   c_tls->data.mbedtls);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }

  return false;
}


/**
 * De-initialise connection TLS settings.
 * The provided pointer is not freed/deallocated.
 * @param c_tls the initialised connection TLS settings
 */
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_tls_multi_conn_deinit (struct mhd_TlsMultiConnData *restrict c_tls)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    mhd_tls_gnu_conn_deinit (c_tls->data.gnutls);
    break;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    mhd_tls_open_conn_deinit (c_tls->data.openssl);
    break;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    mhd_tls_mbed_conn_deinit (c_tls->data.mbedtls);
    break;
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
  }
}


/* ** TLS connection establishing ** */

MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
enum mhd_TlsProcedureResult
mhd_tls_multi_conn_handshake (struct mhd_TlsMultiConnData *restrict c_tls)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_handshake (c_tls->data.gnutls);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_handshake (c_tls->data.openssl);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_handshake (c_tls->data.mbedtls);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return mhd_TLS_PROCED_FAILED;
}


MHD_INTERNAL MHD_FN_MUST_CHECK_RESULT_ MHD_FN_PAR_NONNULL_ALL_
enum mhd_TlsProcedureResult
mhd_tls_multi_conn_shutdown (struct mhd_TlsMultiConnData *restrict c_tls)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_shutdown (c_tls->data.gnutls);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_shutdown (c_tls->data.openssl);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_shutdown (c_tls->data.mbedtls);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return mhd_TLS_PROCED_FAILED;
}


/* ** Data receiving and sending ** */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (4) enum mhd_SocketError
mhd_tls_multi_conn_recv (struct mhd_TlsMultiConnData *restrict c_tls,
                         size_t buf_size,
                         char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                         size_t *restrict received)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_recv (c_tls->data.gnutls,
                                  buf_size,
                                  buf,
                                  received);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_recv (c_tls->data.openssl,
                                   buf_size,
                                   buf,
                                   received);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_recv (c_tls->data.mbedtls,
                                   buf_size,
                                   buf,
                                   received);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return mhd_SOCKET_ERR_INTERNAL;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ MHD_FN_PURE_ bool
mhd_tls_multi_conn_has_cstm_tr (struct mhd_TlsMultiConnData *restrict c_tls)
{
  (void) c_tls; /* Could be unused if all underling functions are actually macros */
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_has_cstm_tr (c_tls->data.gnutls);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_has_cstm_tr (c_tls->data.openssl);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_has_cstm_tr (c_tls->data.mbedtls);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return false;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_tls_multi_conn_has_data_in (struct mhd_TlsMultiConnData *restrict c_tls)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_has_data_in (c_tls->data.gnutls);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_has_data_in (c_tls->data.openssl);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_has_data_in (c_tls->data.mbedtls);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return false;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (5) enum mhd_SocketError
mhd_tls_multi_conn_send (struct mhd_TlsMultiConnData *restrict c_tls,
                         size_t buf_size,
                         const char buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)],
                         bool push_data,
                         size_t *restrict sent)
{
  (void) push_data; /* Could be unused if not supported by all backends */
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_send (c_tls->data.gnutls,
                                  buf_size,
                                  buf,
                                  push_data,
                                  sent);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_send (c_tls->data.openssl,
                                   buf_size,
                                   buf,
                                   push_data,
                                   sent);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_send (c_tls->data.mbedtls,
                                   buf_size,
                                   buf,
                                   push_data,
                                   sent);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return mhd_SOCKET_ERR_INTERNAL;
}


/* ** TLS connection information ** */

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (2) void
mhd_tls_multi_conn_get_tls_sess (
  struct mhd_TlsMultiConnData *restrict c_tls,
  union MHD_ConnInfoDynamicTlsSess *restrict tls_sess_out)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    mhd_tls_gnu_conn_get_tls_sess (c_tls->data.gnutls,
                                   tls_sess_out);
    break;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    mhd_tls_open_conn_get_tls_sess (c_tls->data.openssl,
                                    tls_sess_out);
    break;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    mhd_tls_mbed_conn_get_tls_sess (c_tls->data.mbedtls,
                                    tls_sess_out);
    break;
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_ (2) bool
mhd_tls_multi_conn_get_tls_ver (struct mhd_TlsMultiConnData *restrict c_tls,
                                struct mhd_StctTlsVersion *restrict tls_ver_out)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_get_tls_ver (c_tls->data.gnutls,
                                         tls_ver_out);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_get_tls_ver (c_tls->data.openssl,
                                          tls_ver_out);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_get_tls_ver (c_tls->data.mbedtls,
                                          tls_ver_out);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return false;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ enum mhd_TlsAlpnProt
mhd_tls_multi_conn_get_alpn_prot (struct mhd_TlsMultiConnData *restrict c_tls)
{
  switch (c_tls->choice)
  {
#ifdef MHD_SUPPORT_GNUTLS
  case mhd_TLS_MULTI_ROUTE_GNU:
    return mhd_tls_gnu_conn_get_alpn_prot (c_tls->data.gnutls);
#endif
#ifdef MHD_SUPPORT_OPENSSL
  case mhd_TLS_MULTI_ROUTE_OPEN:
    return mhd_tls_open_conn_get_alpn_prot (c_tls->data.openssl);
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  case mhd_TLS_MULTI_ROUTE_MBED:
    return mhd_tls_mbed_conn_get_alpn_prot (c_tls->data.mbedtls);
#endif
  case mhd_TLS_MULTI_ROUTE_NONE:
  default:
    mhd_UNREACHABLE ();
    break;
  }
  return mhd_TLS_ALPN_PROT_NOT_SELECTED;
}
