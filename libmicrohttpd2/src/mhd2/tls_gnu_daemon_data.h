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
 * @file src/mhd2/tls_gnu_daemon_data.h
 * @brief  The definition of GnuTLS daemon-specific data structures
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_GNU_DAEMON_DATA_H
#define MHD_TLS_GNU_DAEMON_DATA_H 1

#include "mhd_sys_options.h"

#include "sys_bool_type.h"

#ifndef MHD_SUPPORT_GNUTLS
#error This header can be used only if GnuTLS is enabled
#endif

#include "tls_gnu_tls_lib.h"

/**
 * The structure with daemon-specific GnuTLS data
 */
struct mhd_TlsGnuDaemonData
{
  /**
   * The credentials
   */
  gnutls_certificate_credentials_t cred;

#ifdef mhd_TLS_GNU_DH_PARAMS_NEEDS_PKCS3
  /**
   * Diffie-Hellman parameters
   */
  gnutls_dh_params_t dh_params;
#endif

  /**
   * TLS priorities cache
   */
  gnutls_priority_t pri_cache;

#ifdef mhd_TLS_GNU_HAS_ALPN
  /**
   * Enabled protocols for ALPN
   */
  gnutls_datum_t alpn_prots[3];

  /**
   * Number of elements set in the @a alpn_prots
   */
  unsigned int num_alpn_prots;
#endif /* mhd_TLS_GNU_HAS_ALPN */
};

#endif /* ! MHD_TLS_GNU_DAEMON_DATA_H */
