/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff

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
 * @file microhttpd/md5_ext_openssl.c
 * @brief  Wrapper for MD5 calculation performed by OpenSSL library
 * @author Christian grothoff
 */

#include <openssl/evp.h>
#define MHD_MD5_Context EVP_MD_CTX
#include "md5_ext.h"
#include "mhd_assert.h"


/**
 * Initialise structure for MD5 calculation, allocate resources.
 *
 * This function must not be called more than one time for @a ctx.
 *
 * @param ctx the calculation context
 */
void
mhd_MD5_init_one_time (struct mhd_Md5CtxExt *ctx)
{
  ctx->ext_error = 0;
  ctx->handle = EVP_MD_CTX_new ();
  if (NULL == ctx->handle)
  {
    ctx->ext_error = 1; /* Allocation failure */
    return;
  }
  if (1 != EVP_DigestInit_ex (ctx->handle,
                              EVP_md5 (),
                              NULL))
  {
    ctx->ext_error = 1; /* Initialization failure */
    mhd_MD5_deinit (ctx);
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
mhd_MD5_update (struct mhd_Md5CtxExt *ctx,
                size_t size,
                const uint8_t *data)
{
  mhd_assert (0 != size);

  if (0 == ctx->ext_error)
  {
    if (1 != EVP_DigestUpdate (ctx->handle,
                               data,
                               size))
      ctx->ext_error = 1;
  }
}


/**
 * Finalise MD5 calculation, return digest, reset hash calculation.
 *
 * @param ctx the calculation context
 * @param[out] digest set to the hash, must be #mhd_MD5_DIGEST_SIZE bytes
 */
void
mhd_MD5_finish_reset (struct mhd_Md5CtxExt *ctx,
                      uint8_t digest[mhd_MD5_DIGEST_SIZE])
{
  unsigned int len;

  if (0 != ctx->ext_error)
    return;
  if (1 != EVP_DigestFinal_ex (ctx->handle,
                               digest,
                               &len))
  {
    ctx->ext_error = 1;
    return;
  }
  mhd_assert (mhd_MD5_DIGEST_SIZE == len);
  /* Reset for potential reuse */
  if (1 != EVP_DigestInit_ex (ctx->handle,
                              EVP_md5 (),
                              NULL))
  {
    ctx->ext_error = 1;
    mhd_MD5_deinit (ctx);
  }
}


/**
 * Free allocated resources.
 *
 * @param[in] ctx the calculation context
 */
void
mhd_MD5_deinit (struct mhd_Md5CtxExt *ctx)
{
  if (NULL != ctx->handle)
  {
    EVP_MD_CTX_free (ctx->handle);
    ctx->handle = NULL;
  }
}
