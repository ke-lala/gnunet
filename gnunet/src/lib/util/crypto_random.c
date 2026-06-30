/*
     This file is part of GNUnet.  Copyright (C) 2001-2014 Christian Grothoff
     (and other contributing authors)

     GNUnet is free software: you can redistribute it and/or modify it
     under the terms of the GNU Affero General Public License as published
     by the Free Software Foundation, either version 3 of the License,
     or (at your option) any later version.

     GNUnet is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Affero General Public License for more details.

     You should have received a copy of the GNU Affero General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     SPDX-License-Identifier: AGPL3.0-or-later

 */

/**
 * @file util/crypto_random.c
 * @brief functions to gather random numbers
 * @author Christian Grothoff
 */

#include "platform.h"
#include "gnunet_util_lib.h"
#include <sodium.h>

#define LOG(kind, ...) GNUNET_log_from (kind, "util-crypto-random", __VA_ARGS__)

#define LOG_STRERROR(kind, syscall) \
        GNUNET_log_from_strerror (kind, "util-crypto-random", syscall)


/* TODO: ndurner, move this to plibc? */
/* The code is derived from glibc, obviously */
#if ! HAVE_RANDOM || ! HAVE_SRANDOM
#ifdef RANDOM
#undef RANDOM
#endif
#ifdef SRANDOM
#undef SRANDOM
#endif
#define RANDOM() glibc_weak_rand32 ()
#define SRANDOM(s) glibc_weak_srand32 (s)
#if defined(RAND_MAX)
#undef RAND_MAX
#endif
#define RAND_MAX 0x7fffffff /* Hopefully this is correct */

static int32_t glibc_weak_rand32_state = 1;


void
glibc_weak_srand32 (int32_t s)
{
  glibc_weak_rand32_state = s;
}


int32_t
glibc_weak_rand32 ()
{
  int32_t val = glibc_weak_rand32_state;

  val = ((glibc_weak_rand32_state * 1103515245) + 12345) & 0x7fffffff;
  glibc_weak_rand32_state = val;
  return val;
}


#endif


/**
 * @ingroup crypto
 * Zero out @a buffer, securely against compiler optimizations.
 * Used to delete key material.
 *
 * @param buffer the buffer to zap
 * @param length buffer length
 */
void
GNUNET_CRYPTO_zero_keys (void *buffer, size_t length)
{
#if HAVE_MEMSET_S
  memset_s (buffer, length, 0, length);
#elif HAVE_EXPLICIT_BZERO
  explicit_bzero (buffer, length);
#else
  volatile unsigned char *p = buffer;
  while (length--)
    *p++ = 0;
#endif
}


void
GNUNET_CRYPTO_random_block (void *buffer,
                            size_t length)
{
  randombytes_buf (buffer,
                   length);
}


uint32_t
GNUNET_CRYPTO_random_u32 (uint32_t max)
{
  GNUNET_assert (max > 0);

  return randombytes_uniform (max);
}


unsigned int *
GNUNET_CRYPTO_random_permute (unsigned int n)
{
  unsigned int *ret;
  unsigned int i;
  unsigned int tmp;
  uint32_t x;

  GNUNET_assert (n > 0);
  ret = GNUNET_malloc (n * sizeof(unsigned int));
  for (i = 0; i < n; i++)
    ret[i] = i;
  for (i = n - 1; i > 0; i--)
  {
    x = GNUNET_CRYPTO_random_u32 (i + 1);
    tmp = ret[x];
    ret[x] = ret[i];
    ret[i] = tmp;
  }
  return ret;
}


uint64_t
GNUNET_CRYPTO_random_u64 (uint64_t max)
{
  GNUNET_assert (max > 0);
  return randombytes_uniform (max);
}


void
GNUNET_CRYPTO_random_timeflake (struct GNUNET_Uuid *uuid)
{
  struct GNUNET_TIME_Absolute now;
  uint64_t ms;
  uint64_t be;
  char *base;

  GNUNET_CRYPTO_random_block (uuid,
                              sizeof (struct GNUNET_Uuid));
  now = GNUNET_TIME_absolute_get ();
  ms = now.abs_value_us / GNUNET_TIME_UNIT_MILLISECONDS.rel_value_us;
  be = GNUNET_htonll (ms);
  base = (char *) &be;
  memcpy (uuid,
          base + 2,
          sizeof (be) - 2);
}


void
GNUNET_CRYPTO_random_init (void);

/**
 * Initialize sodium.
 */
void __attribute__ ((constructor))
GNUNET_CRYPTO_random_init ()
{
  GNUNET_assert (-1 != sodium_init ());
}


/* end of crypto_random.c */
