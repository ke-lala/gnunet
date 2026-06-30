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
 * @file src/mhd2/response_destroy.c
 * @brief  The declarations of internal functions for response deletion
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"
#include "response_destroy.h"
#include "mhd_response.h"

#include "mhd_assert.h"
#include "mhd_panic.h"
#include "mhd_atomic_counter.h"

#include "sys_malloc.h"

#include "mhd_public_api.h"

#include "response_add_header.h"
#include "response_funcs.h"
#include "response_from.h"

#ifdef MHD_SUPPORT_AUTH_DIGEST
#  include "response_auth_digest.h"
#endif

#define mhd_RESPONSE_DESTOYED "Attempt to use destroyed response, " \
        "re-use non-reusable response or wrong MHD_Response pointer"

/**
 * Perform full response de-initialisation, with cleaning-up / freeing
 * all content data and headers.
 * The response settings (if any) must be already freed.
 * @param r the response to free
 */
static MHD_FN_PAR_NONNULL_ (1) void
response_full_deinit (struct MHD_Response *restrict r)
{
#ifdef MHD_SUPPORT_AUTH_DIGEST
  mhd_response_remove_auth_digest_headers (r);
#endif
  mhd_response_remove_all_headers (r);
  if (NULL != r->special_resp.spec_hdr)
    free (r->special_resp.spec_hdr);
  if (r->reuse.reusable)
    mhd_response_deinit_reusable (r);
  mhd_response_deinit_content_data (r);

  r->was_destroyed = true;
  free (r);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_response_dec_use_count (struct MHD_Response *restrict r)
{
  mhd_assert (r->frozen);
  if (r->was_destroyed)
    MHD_PANIC (mhd_RESPONSE_DESTOYED);

  if (r->reuse.reusable)
  {
    if (1 != mhd_atomic_counter_get_dec (&(r->reuse.counter)))
      return; /* The response is still used somewhere */
  }

  response_full_deinit (r);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_response_inc_use_count (struct MHD_Response *restrict r)
{
  mhd_assert (r->frozen);
  if (r->was_destroyed)
    MHD_PANIC (mhd_RESPONSE_DESTOYED);

  if (! r->reuse.reusable)
    return;

  mhd_atomic_counter_inc (&(r->reuse.counter));
}


MHD_EXTERN_
MHD_FN_PAR_NONNULL_ (1) void
MHD_response_destroy (struct MHD_Response *response)
{
  if (response->was_destroyed)
    MHD_PANIC (mhd_RESPONSE_DESTOYED);

  if (! response->frozen)
  {
    /* This response has been never used for actions */
    mhd_assert (NULL != response->settings);
    free (response->settings);
#ifndef NDEBUG
    /* Decrement counter to avoid triggering assert in deinit function */
    if (response->reuse.reusable)
      mhd_assert (1 == mhd_atomic_counter_get_dec (&(response->reuse.counter)));
#endif
    response_full_deinit (response);
    return;
  }

  mhd_response_dec_use_count (response);
}
