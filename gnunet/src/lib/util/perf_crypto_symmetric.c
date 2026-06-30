/*
     This file is part of GNUnet.
     Copyright (C) 2002, 2003, 2004, 2006 GNUnet e.V.

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
 * @author Christian Grothoff
 * @file util/perf_crypto_symmetric.c
 * @brief measure performance of encryption function
 */

#include "platform.h"
#include "gnunet_util_lib.h"


static void
perfEncrypt ()
{
  unsigned int i;
  unsigned char buf[64 * 1024];
  unsigned char rbuf[64 * 1024];
  struct GNUNET_CRYPTO_AeadSecretKey sk;
  struct GNUNET_CRYPTO_AeadNonce iv;
  struct GNUNET_CRYPTO_AeadMac mac;

  GNUNET_CRYPTO_aead_create_key (&sk);

  memset (buf, 1, sizeof(buf));
  for (i = 0; i < 1024; i++)
  {
    memset (&iv, (int8_t) i, sizeof(iv));
    GNUNET_CRYPTO_aead_encrypt (sizeof buf,
                                buf,
                                0,
                                NULL,
                                &sk,
                                &iv,
                                rbuf,
                                &mac);
    GNUNET_CRYPTO_aead_decrypt (sizeof(buf),
                                rbuf,
                                0,
                                NULL,
                                &sk,
                                &iv,
                                &mac,
                                buf);
  }
  memset (rbuf, 1, sizeof(rbuf));
  GNUNET_assert (0 == memcmp (rbuf, buf, sizeof(buf)));
}


int
main (int argc, char *argv[])
{
  struct GNUNET_TIME_Absolute start;

  start = GNUNET_TIME_absolute_get ();
  perfEncrypt ();
  printf ("Encrypt perf took %s\n",
          GNUNET_STRINGS_relative_time_to_string (
            GNUNET_TIME_absolute_get_duration (start),
            GNUNET_YES));
  return 0;
}


/* end of perf_crypto_symmetric.c */
