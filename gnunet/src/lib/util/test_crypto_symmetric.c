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
 * @file util/test_crypto_symmetric.c
 * @brief test for AES ciphers
 */

#include "platform.h"
#include "gnunet_util_lib.h"

#define TESTSTRING "Hello World!"
#define TESTAAD "Some test additional data"

static int
testAead ()
{
  struct GNUNET_CRYPTO_AeadSecretKey key;
  struct GNUNET_CRYPTO_AeadNonce nonce;
  struct GNUNET_CRYPTO_AeadMac mac;
  unsigned char result[strlen (TESTSTRING) + 1];
  enum GNUNET_GenericReturnValue ret;
  unsigned char res[strlen (TESTSTRING) + 1];

  GNUNET_CRYPTO_random_block (&nonce,
                              sizeof nonce);
  GNUNET_CRYPTO_aead_create_key (&key);
  ret =
    GNUNET_CRYPTO_aead_encrypt (strlen (TESTSTRING) + 1,
                                (unsigned char*) TESTSTRING,
                                strlen (TESTAAD) + 1,
                                (unsigned char*) TESTAAD,
                                &key,
                                &nonce,
                                result,
                                &mac);
  if (GNUNET_OK != ret)
  {
    printf ("AEAD API test failed: %d\n", ret);
    return 1;
  }
  ret =
    GNUNET_CRYPTO_aead_decrypt (strlen (TESTSTRING) + 1,
                                result,
                                strlen (TESTAAD) + 1,
                                (unsigned char*) TESTAAD,
                                &key,
                                &nonce,
                                &mac,
                                res);
  if (GNUNET_OK != ret)
  {
    printf ("AEAD API test failed: %d\n", ret);
    return 1;
  }
  if (0 != memcmp (res, TESTSTRING, strlen (TESTSTRING) + 1))
  {
    printf ("symciphertest failed: %s != %s\n", res, TESTSTRING);
    return 1;
  }
  else
    return 0;
}


int
main (int argc, char *argv[])
{
  int failureCount = 0;

  GNUNET_log_setup ("test-crypto-symmetric", "WARNING", NULL);
  failureCount += testAead ();

  if (failureCount != 0)
  {
    printf ("%d TESTS FAILED!\n", failureCount);
    return -1;
  }
  return 0;
}


/* end of test_crypto_aes.c */
