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
 * @file src/mhd2/mhd_tls_common.h
 * @brief  The TLS functions and data common for all TLS backends
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_TLS_COMMON_H
#define MHD_TLS_COMMON_H 1

#include "mhd_sys_options.h"

#ifndef MHD_SUPPORT_HTTPS
#error This header should be used only if HTTPS is enabled
#endif

struct DaemonOptions; /* Forward declaration */

/* ** General information function ** */

/**
 * Result of TLS backend availability check
 */
enum mhd_TlsBackendAvailable
{
  /**
   * The TLS backend is available and can be used
   */
  mhd_TLS_BACKEND_AVAIL_OK = 0
  ,
  /**
   * The TLS backend support is not enabled in this MHD build
   */
  mhd_TLS_BACKEND_AVAIL_NOT_SUPPORTED
  ,
  /**
   * The TLS backend supported, but not available
   */
  mhd_TLS_BACKEND_AVAIL_NOT_AVAILABLE
};

/**
 * Check whether the requested TLS backend is available
 * @param s the daemon settings
 * @return 'mhd_TLS_BACKEND_AVAIL_OK' if requested backend is available,
 *         error code otherwise
 */
MHD_INTERNAL enum mhd_TlsBackendAvailable
mhd_tls_is_backend_available (struct DaemonOptions *s)
MHD_FN_PAR_NONNULL_ALL_ MHD_FN_MUST_CHECK_RESULT_;

#endif /* ! MHD_TLS_COMMON_H */
