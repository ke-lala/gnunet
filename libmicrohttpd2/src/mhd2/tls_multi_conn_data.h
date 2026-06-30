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
 * @file src/mhd2/tls_multi_conn_data.h
 * @brief  The definition of MultiTLS connection-specific data structures
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_MULTI_CONN_DATA_H
#define MHD_TLS_MULTI_CONN_DATA_H 1

#include "mhd_sys_options.h"

#include "tls_multi_tls_lib.h"

#ifndef MHD_USE_MULTITLS
#error This header can be used only if MultiTLS is enabled
#endif

#ifdef MHD_SUPPORT_GNUTLS
struct mhd_TlsGnuConnData;      /* forward declaration */
#endif
#ifdef MHD_SUPPORT_OPENSSL
struct mhd_TlsOpenConnData;     /* forward declaration */
#endif
#ifdef MHD_SUPPORT_MBEDTLS
struct mhd_TlsMbedConnData;     /* forward declaration */
#endif

/**
 * The pointer to the underlying TLS backend connection data
 */
struct mhd_TlsMultiConnRoutePtr
{
#ifdef MHD_SUPPORT_GNUTLS
  /**
   * Pointer to GnuTLS connection-specific data
   */
  struct mhd_TlsGnuConnData *gnutls;
#endif
#ifdef MHD_SUPPORT_OPENSSL
  /**
   * Pointer to OpenSSL connection-specific data
   */
  struct mhd_TlsOpenConnData *openssl;
#endif
#ifdef MHD_SUPPORT_MBEDTLS
  /**
   * Pointer to OpenSSL connection-specific data
   */
  struct mhd_TlsMbedConnData *mbedtls;
#endif
};

/**
 * The structure with connection-specific MultiTLS data
 */
struct mhd_TlsMultiConnData
{
  /**
   * The underlying TLS backend choice
   */
  enum mhd_TlsMultiRoute choice;

  /**
   * The pointer to the underlying TLS backend connection data
   */
  struct mhd_TlsMultiConnRoutePtr data;
};

#endif /* ! MHD_TLS_MULTI_CONN_DATA_H */
