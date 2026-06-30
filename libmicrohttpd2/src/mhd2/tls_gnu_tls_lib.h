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
 * @file src/mhd2/tls_gnu_tls_lib.h
 * @brief  The wrapper for GnuTLS header
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_GNU_TLS_LIB_H
#define MHD_TLS_GNU_TLS_LIB_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_GNUTLS
#error This header can be used only if GnuTLS is enabled
#endif

#include <gnutls/gnutls.h>

#ifndef GNUTLS_VERSION_NUMBER
#error GNUTLS_VERSION_NUMBER is not defined
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030600
/**
 * Indicate that RC7919 defaults are used
 */
#  define mhd_TLS_GNU_HAS_RFC7919_DEFS       1
#endif

#ifndef mhd_TLS_GNU_HAS_RFC7919_DEFS
#  if GNUTLS_VERSION_NUMBER >= 0x030506
/**
 * Use gnutls_certificate_set_known_dh_params() function to set DH parameters
 */
#    define mhd_TLS_GNU_DH_PARAMS_USE_KNOWN       1
#  else
/**
 * TLS backend needs encoded Diffie-Hellman parameters
 */
#    define mhd_TLS_GNU_DH_PARAMS_NEEDS_PKCS3    1
#  endif
#endif

#if GNUTLS_VERSION_NUMBER >= 0x020104
/**
 * Defined if gnutls_set_default_priority() function is available
 */
#  define mhd_TLS_GNU_HAS_SET_DEF_PRIORITY      1
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030300
/**
 * Defined if NULL is treated as "default priorities" when used as "priorities"
 * argument for gnutls_priority_init() and gnutls_priority_init2()
 */
#  define mhd_TLS_GNU_TREATS_NULL_AS_DEF_PRIORITY        1
#endif

#if ! defined(mhd_TLS_GNU_TREATS_NULL_AS_DEF_PRIORITY) \
  && defined(mhd_TLS_GNU_HAS_SET_DEF_PRIORITY)
/**
 * Defined if NULL in priorities cache is treated as indication that default
 * priorities must used for connection / session
 */
#  define mhd_TLS_GNU_NULL_PRIO_CACHE_MEANS_DEF_PRIORITY        1
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030603
/**
 * Defined if gnutls_priority_init2() function and the flag
 * GNUTLS_PRIORITY_INIT_DEF_APPEND are available
 */
#  define mhd_TLS_GNU_HAS_PRIORITY_INIT2        1
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030501
/**
 * Defined if gnutls_priority_init{,2}() functions support "@KEYWORD1,@KEYWORD2"
 * to as fallback values.
 */
#  define mhd_TLS_GNU_SUPPORTS_MULTI_KEYWORDS_PRIORITY  1
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030402
/**
 * Defined if GNUTLS_NO_SIGNAL flag is available for gnutls_init() function
 */
#  define mhd_TLS_GNU_HAS_NO_SIGNAL     1
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030109
/**
 * Defined if transport_set_int() function is available
 */
#  define mhd_TLS_GNU_HAS_TRANSP_SET_INT        1
#endif

#if GNUTLS_VERSION_NUMBER >= 0x030200
/**
 * Defined if gnutls_alpn_set_protocols() and
 * gnutls_alpn_get_selected_protocol() function are available
 */
#  define mhd_TLS_GNU_HAS_ALPN        1
#endif

#endif /* ! MHD_TLS_GNU_TLS_LIB_H */
