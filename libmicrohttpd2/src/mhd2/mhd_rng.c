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
 * @file src/mhd2/mhd_rng.c
 * @brief generate random numbers using the best available method;
 *   we begin by trying the TLS libraries, then common operating-system
 *   specific methods, then fall back to /dev/urandom or /dev/random
 *   and if nothing works hash our entropy pool mixing in data from
 *   the context
 * @author Christian Grothoff
 */

#include "mhd_sys_options.h"
#include "mhd_digest_auth_data.h"

#include <stddef.h>
#include <string.h>
#include <errno.h>

#if defined(MHD_SUPPORT_OPENSSL)
#include <openssl/rand.h>
#endif
#if defined(MHD_SUPPORT_GNUTLS)
#include <gnutls/crypto.h>
#endif
#if defined(MHD_SUPPORT_MBEDTLS)
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#endif
#if defined(__linux__)
#include <sys/random.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__FreeBSD__)
#include <sys/random.h>
#else
#include <stdio.h>
#include <unistd.h>
#endif

#ifdef MHD_SUPPORT_SHA512_256
#  include "mhd_sha512_256.h"
#endif /* MHD_SUPPORT_SHA512_256 */
#ifdef MHD_SUPPORT_SHA256
#  include "mhd_sha256.h"
#endif
#ifdef MHD_SUPPORT_MD5
#  include "mhd_md5.h"
#endif

#include "mhd_mono_clock.h"
#include "mhd_atomic_counter.h"

#include "mhd_rng.h"


MHD_INTERNAL bool
mhd_rng (size_t buf_size,
         uint8_t buf[MHD_FN_PAR_DYN_ARR_SIZE_ (buf_size)])
{
 #if defined(MHD_SUPPORT_OPENSSL)
  /* OpenSSL - RAND_bytes() */
  mhd_assert (buf_size < INT_MAX);
  if (1 == RAND_bytes ((unsigned char *) buf,
                       (int) buf_size))
    return true;
#endif
#if defined(MHD_SUPPORT_GNUTLS)
  /* GnuTLS - gnutls_rnd() */
  if (0 ==
      gnutls_rnd (GNUTLS_RND_RANDOM, buf,
                  buf_size))
    return true;
#endif
#if defined(MHD_SUPPORT_MBEDTLS)
  {
    /* mbedTLS - requires entropy context and DRBG */
    static mbedtls_entropy_context entropy;
    static mbedtls_ctr_drbg_context ctr_drbg;
    static int initialized = 0;

    if (0 == initialized)
    {
      mbedtls_entropy_init (&entropy);
      mbedtls_ctr_drbg_init (&ctr_drbg);

      if (0 !=
          mbedtls_ctr_drbg_seed (&ctr_drbg,
                                 mbedtls_entropy_func,
                                 &entropy,
                                 NULL,
                                 0))
      {
        initialized = -1;
      }
      else
      {
        initialized = 1;
      }
    }

    if ( (1 == initialized) &&
         (0 ==
          mbedtls_ctr_drbg_random (&ctr_drbg,
                                   (unsigned char *) buf,
                                   buf_size)) )
      return true;
  }
#endif
#if defined(__linux__) && defined(__GLIBC__) && \
  (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
  {
    /* Modern Linux with glibc >= 2.25 - getrandom() syscall */
    size_t offset = 0;
    unsigned char *ptr = (unsigned char *) buf;

    while (1)
    {
      ssize_t ret;

      ret = getrandom (ptr + offset,
                       buf_size - offset,
                       0);
      if (ret < 0)
      {
        if (EINTR == errno)
        {
          continue;
        }
        break; /* failure */
      }
      offset += (size_t) ret;
      if (offset == buf_size)
        return true;
    }
  }
#elif defined(_WIN32) || defined(_WIN64)
  /* Windows - BCryptGenRandom() */
  if (STATUS_SUCCESS ==
      BCryptGenRandom (NULL,
                       (PUCHAR) buf,
                       (ULONG) buf_size,
                       BCRYPT_USE_SYSTEM_PREFERRED_RNG))
    return true;
#elif defined(__FreeBSD__)
  /* FreeBSD - arc4random_buf() */
  arc4random_buf (buf,
                  buf_size);
  return true;
#else
  /* Generic UNIX fallback - read from /dev/urandom */
  {
    static int tried = 0;
    static FILE *fp = NULL;

    if ( (0 == tried) &&
         (NULL == fp) )
    {
      tried = 1;
      fp = fopen ("/dev/urandom",
                  "rb");
      if (NULL == fp)
      {
        /* Try /dev/random as last resort */
        fp = fopen ("/dev/random",
                    "rb");
      }
    }
    if (NULL != fp)
    {
      size_t bytes_read;

      bytes_read = fread (buf,
                          1,
                          buf_size,
                          fp);
      if (bytes_read == buf_size)
        return true;
    }
  }
#endif
  return false;
}
