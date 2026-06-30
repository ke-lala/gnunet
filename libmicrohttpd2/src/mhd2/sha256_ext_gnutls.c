/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2022-2023 Evgeny Grin (Karlson2k)

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
 * @file microhttpd/sha256_ext_gnutls.c
 * @brief  Wrapper for SHA-256 calculation performed by GnuTLS library
 * @author Karlson2k (Evgeny Grin)
 */
#include <gnutls/crypto.h>
#define MHD_SHA256_Context struct hash_hd_st
#include "sha256_ext.h"
#include "mhd_assert.h"


/**
 * Initialise structure for SHA-256 calculation, allocate resources.
 *
 * This function must not be called more than one time for @a ctx.
 *
 * @param ctx the calculation context
 */
void
mhd_SHA256_init_one_time (struct mhd_Sha256CtxExt *ctx)
{
  ctx->handle = NULL;
  ctx->ext_error = gnutls_hash_init (&ctx->handle,
                                     GNUTLS_DIG_SHA256);
  if ( (0 != ctx->ext_error) &&
       (NULL != ctx->handle) )
  {
    /* GnuTLS may return initialisation error and set the handle at the
       same time. Such handle cannot be used for calculations.
       Note: GnuTLS may also return an error and NOT set the handle. */
    gnutls_free (ctx->handle);
    ctx->handle = NULL;
  }

  /* If handle is NULL, the error must be set */
  mhd_assert ((NULL != ctx->handle) || (0 != ctx->ext_error));
  /* If error is set, the handle must be NULL */
  mhd_assert ((0 == ctx->ext_error) || (NULL == ctx->handle));
}


/**
 * Process portion of bytes.
 *
 * @param ctx the calculation context
 * @param size number of bytes in @a data, must not be 0
 * @param data bytes to add to hash
 */
void
mhd_SHA256_update (struct mhd_Sha256CtxExt *ctx,
                   size_t size,
                   const uint8_t *data)
{
  mhd_assert (0 != size);

  if (0 == ctx->ext_error)
    ctx->ext_error = gnutls_hash (ctx->handle,
                                  data,
                                  size);
}


/**
 * Finalise SHA-256 calculation, return digest, reset hash calculation.
 *
 * @param ctx the calculation context
 * @param[out] digest set to the hash, must be #mhd_SHA256_DIGEST_SIZE bytes
 */
void
mhd_SHA256_finish_reset (struct mhd_Sha256CtxExt *ctx,
                         uint8_t digest[mhd_SHA256_DIGEST_SIZE])
{
  if (0 == ctx->ext_error)
    gnutls_hash_output (ctx->handle,
                        digest);
}


/**
 * Free allocated resources.
 *
 * @param ctx the calculation context
 */
void
mhd_SHA256_deinit (struct mhd_Sha256CtxExt *ctx)
{
  if (NULL != ctx->handle)
  {
    gnutls_hash_deinit (ctx->handle,
                        NULL);
    ctx->handle = NULL;
  }
}
