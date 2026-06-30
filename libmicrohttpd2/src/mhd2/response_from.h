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
 * @file src/mhd2/response_from.h
 * @brief  The declarations of internal functions for response creation and
 *         deletion
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_RESPONSE_FROM_H
#define MHD_RESPONSE_FROM_H 1

#include "mhd_sys_options.h"
#include "sys_base_types.h"

struct MHD_Response; /* forward declaration */

/**
 * Deinit / free / cleanup content data of the response
 * @param r the response to use
 */
MHD_INTERNAL void
mhd_response_deinit_content_data (struct MHD_Response *restrict r)
MHD_FN_PAR_NONNULL_ (1);

/**
 * Create special internal-only response for sending automatic error messages
 * @param sc the HTTP status code (enum MHD_HTTP_StatusCode)
 * @param cntn_len the length of the @a cntn
 * @param cntn the response content
 * @param spec_hdr_len the length of the @a spec_hdr
 * @param spec_hdr the special string to be used as a header string
 * @return the response object if succeed,
 *         NULL if failed (out of memory)
 */
MHD_INTERNAL struct MHD_Response *
mhd_response_special_for_error (unsigned int sc,
                                size_t cntn_len,
                                const char *cntn,
                                size_t spec_hdr_len,
                                char *spec_hdr)
MHD_FN_PAR_CSTR_(3) MHD_FN_PAR_CSTR_(5);


#endif /* ! MHD_RESPONSE_FROM_H */
