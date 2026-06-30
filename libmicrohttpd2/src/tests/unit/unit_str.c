/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff
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
 * @file src/test/unit/unit_str.c
 * @brief  Unit tests for string functions
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "../mhd2/mhd_str.h"
#include "../mhd2/mhd_str.c"
#include "mhdt_checks.h"

#include "mhdt_has_param.h"

int
main (int argc,
      char *const *argv)
{
  if (mhdt_has_param (argc, argv, "-s") ||
      mhdt_has_param (argc, argv, "--silent"))
    MHDT_set_verbosity (MHDT_VERB_LVL_SILENT);
  else
    MHDT_set_verbosity (MHDT_VERB_LVL_VERBOSE);

  MHDT_EXPECT_TRUE (mhd_str_equal_caseless ("ab",
                                            "AB"));
  MHDT_EXPECT_TRUE (mhd_str_equal_caseless ("ab",
                                            "ab"));
  MHDT_EXPECT_TRUE (mhd_str_equal_caseless ("",
                                            ""));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless ("ab",
                                             "ABc"));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless ("a b",
                                             "ab"));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless ("",
                                             " "));
  /* Note: our caseless ONLY refers to US-ASCII */
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless ("Ä",
                                             "ä"));

  MHDT_EXPECT_TRUE (mhd_str_equal_caseless_n ("ab\0x",
                                              "AB\0y",
                                              4));
  MHDT_EXPECT_TRUE (mhd_str_equal_caseless_n ("abc",
                                              "abd",
                                              2));
  MHDT_EXPECT_TRUE (mhd_str_equal_caseless_n ("",
                                              "",
                                              0));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless_n ("ab",
                                               "ABc",
                                               3));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless_n ("a b",
                                               "ab",
                                               2));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless_n ("",
                                               " ",
                                               1));
  /* Note: our caseless ONLY refers to US-ASCII,
     Need 3 here because Umlaut is equal in first
     byte and diffes in 2nd byte. */
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless_n ("xÄbb",
                                               "xäbb",
                                               3));
  MHDT_EXPECT_TRUE (mhd_str_equal_caseless_n ("ab\0x",
                                              "AB\0y",
                                              4));

  MHDT_EXPECT_TRUE (mhd_str_equal_caseless_bin_n ("ab\0x",
                                                  "AB\0x",
                                                  4));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless_bin_n ("ab\0x",
                                                   "AB\0y",
                                                   4));
  MHDT_EXPECT_FALSE (mhd_str_equal_caseless_bin_n ("ab\0x",
                                                   "AB\0y",
                                                   4));

  MHDT_EXPECT_TRUE (mhd_str_equal_lowercase_bin_n ("AB",
                                                   "ab",
                                                   2));
  MHDT_EXPECT_TRUE (mhd_str_equal_lowercase_bin_n ("aB",
                                                   "ab",
                                                   2));
  MHDT_EXPECT_TRUE (mhd_str_is_lowercase_bin_n (0,
                                                ""));
  MHDT_EXPECT_TRUE (mhd_str_is_lowercase_bin_n (2,
                                                "ab"));
  MHDT_EXPECT_TRUE (mhd_str_is_lowercase_bin_n (2,
                                                "abC"));
  MHDT_EXPECT_TRUE (mhd_str_is_lowercase_bin_n (2,
                                                "xä"));
  MHDT_EXPECT_TRUE (mhd_str_is_lowercase_bin_n (2,
                                                "xÄ"));
  MHDT_EXPECT_FALSE (mhd_str_is_lowercase_bin_n (2,
                                                 "aB"));

  MHDT_TEST_RESULT ();

  return MHDT_FINAL_RESULT (argv[0]);
}
