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
 * @file src/mhd2/tls_mbed_daemon_data.h
 * @brief  The definition of MbedTLS daemon-specific data structures
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_MBED_DAEMON_DATA_H
#define MHD_TLS_MBED_DAEMON_DATA_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_MBEDTLS
#error This header can be used only if MbedTLS is enabled
#endif

#include "tls_mbed_tls_lib.h"

/**
 * The structure with daemon-specific MbedTLS data.
 *
 * @note Unlike other TLS backends this struct contains MbedTLS data itself,
 *       not just pointers.
 */
struct mhd_TlsMbedDaemonData
{
  /**
   * The daemon TLS configuration.
   */
  mbedtls_ssl_config tls_conf;

  /**
   * The certificates chain
   */
  mbedtls_x509_crt cert_chain;

  /**
   * The private key
   */
  mbedtls_pk_context prv_key;

#ifdef MBEDTLS_SSL_ALPN
  /**
   * Enabled protocols for ALPN
   */
  const char *alpn_prots[4];
#endif /* MBEDTLS_SSL_ALPN */
};

#endif /* ! MHD_TLS_MBED_DAEMON_DATA_H */
