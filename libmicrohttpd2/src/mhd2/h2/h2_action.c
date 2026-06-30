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
 * @file src/mhd2/h2/h2_action.c
 * @brief  Implementation of HTTP/2 action creators
 * @author Karlson2k (Evgeny Grin)
 */


#include "mhd_sys_options.h"

#include "mhd_assert.h"

#include "mhd_action.h"
#include "mhd_response.h"

#include "h2_req_data.h"

#include "h2_action.h"

MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ bool
mhd_h2_act_is_resp_h2_compatible (const struct mhd_H2RequestData *restrict req,
                                  const struct MHD_Response *restrict response)
{
  mhd_assert (req->is_http2);

  // TODO: move new two checks to the unified (HTTP/1.x and HTTP/2) code
  if ((mhd_HTTP_METHOD_CONNECT == req->method) &&
      (200 <= response->sc) && (299 >= response->sc))
    return false;

  if ((MHD_NO != response->cfg.head_only) &&
      (200 <= response->sc) &&
      (MHD_HTTP_STATUS_NOT_MODIFIED != response->sc) &&
      (MHD_HTTP_STATUS_NO_CONTENT != response->sc) &&
      (mhd_HTTP_METHOD_HEAD != req->method))
    return false;

  // TODO: support digest auth with HTTP/2
  if (mhd_RESP_HAS_AUTH_DIGEST (response))
    return false;

  // TODO: implement callback for the next response
  if (MHD_HTTP_STATUS_OK > response->sc)
    return false;

  // TODO: work with all types
  if ((mhd_RESPONSE_CONTENT_DATA_BUFFER != response->cntn_dtype)
      && (mhd_RESPONSE_CONTENT_DATA_FILE != response->cntn_dtype)
      && (mhd_RESPONSE_CONTENT_DATA_IOVEC != response->cntn_dtype))
    return false;
  if ((mhd_RESPONSE_CONTENT_DATA_FILE == response->cntn_dtype) &&
      response->cntn.file.is_pipe)
    return false;

  return true;
}
