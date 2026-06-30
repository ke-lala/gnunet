/*
  This file is part of libmicrohttpd
  Copyright (C) 2016 Karlson2k (Evgeny Grin)

  This test tool is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2, or
  (at your option) any later version.

  This test tool is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file src/tests/unit/unit_str_test.h
 * @brief  Unit tests for mhd_str functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include <stdio.h>
#include <locale.h>
#include <string.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else  /* ! HAVE_INTTYPES_H */
#define PRIuFAST64      "llu"
#define PRIuPTR         "llu"
#define PRIXFAST64      "llX"
#endif /* ! HAVE_INTTYPES_H */
#include <stdint.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "mhd_str.h"
#include "mhd_str.c"
#include "mhdt_checks.h"
#include "mhdt_has_param.h"


static int verbose = 0; /* verbose level (0-3)*/

/* Locale names to test.
 * Functions must not depend of current current locale,
 * so result must be the same in any locale.
 */
static const char *const locale_names[] = {
  "C",
  "",        /* System default locale */
#if defined(_WIN32) && ! defined(__CYGWIN__)
  ".OCP",    /* W32 system default OEM code page */
  ".ACP",    /* W32 system default ANSI code page */
  ".65001",  /* UTF-8 */
  ".437",
  ".850",
  ".857",
  ".866",
  ".1250",
  ".1251",
  ".1252",
  "en",
  "english",
  "French_France",
  "Turkish_Turkey.1254",
  "de",
  "zh-Hans",
  "ru-RU.1251"
#if 0 /* Disabled extra checks */
  ,
  ".1254",
  ".20866",   /* number for KOI8-R */
  ".28591",   /* number for ISO-8859-1 */
  ".28595",   /* number for ISO-8859-5 */
  ".28599",   /* number for ISO-8859-9 */
  ".28605",   /* number for ISO-8859-15 */
  "en-US",
  "English-US",
  "en-US.437",
  "English_United States.437",
  "en-US.1252",
  "English_United States.1252",
  "English_United States.28591",
  "English_United States.65001",
  "fra",
  "french",
  "fr-FR",
  "fr-FR.850",
  "french_france.850",
  "fr-FR.1252",
  "French_france.1252",
  "French_france.28605",
  "French_France.65001",
  "de-DE",
  "de-DE.850",
  "German_Germany.850",
  "German_Germany.1250",
  "de-DE.1252",
  "German_Germany.1252",
  "German_Germany.28605",
  "German_Germany.65001",
  "tr",
  "trk",
  "turkish",
  "tr-TR",
  "tr-TR.1254",
  "tr-TR.857",
  "Turkish_Turkey.857",
  "Turkish_Turkey.28599",
  "Turkish_Turkey.65001",
  "ru",
  "ru-RU",
  "Russian",
  "ru-RU.866",
  "Russian_Russia.866",
  "Russian_Russia.1251",
  "Russian_Russia.20866",
  "Russian_Russia.28595",
  "Russian_Russia.65001",
  "zh-Hans.936",
  "chinese-simplified"
#endif /* Disabled extra checks */
#else /* ! _WIN32 || __CYGWIN__ */
  "C.UTF-8",
  "POSIX",
  "en",
  "en_US",
  "en_US.ISO-8859-1",
  "en_US.ISO_8859-1",
  "en_US.ISO8859-1",
  "en_US.iso88591",
  "en_US.ISO-8859-15",
  "en_US.DIS_8859-15",
  "en_US.ISO8859-15",
  "en_US.iso885915",
  "en_US.1252",
  "en_US.CP1252",
  "en_US.UTF-8",
  "en_US.utf8",
  "fr",
  "fr_FR",
  "fr_FR.850",
  "fr_FR.IBM850",
  "fr_FR.1252",
  "fr_FR.CP1252",
  "fr_FR.ISO-8859-1",
  "fr_FR.ISO_8859-1",
  "fr_FR.ISO8859-1",
  "fr_FR.iso88591",
  "fr_FR.ISO-8859-15",
  "fr_FR.DIS_8859-15",
  "fr_FR.ISO8859-15",
  "fr_FR.iso8859-15",
  "fr_FR.UTF-8",
  "fr_FR.utf8",
  "de",
  "de_DE",
  "de_DE.850",
  "de_DE.IBM850",
  "de_DE.1250",
  "de_DE.CP1250",
  "de_DE.1252",
  "de_DE.CP1252",
  "de_DE.ISO-8859-1",
  "de_DE.ISO_8859-1",
  "de_DE.ISO8859-1",
  "de_DE.iso88591",
  "de_DE.ISO-8859-15",
  "de_DE.DIS_8859-15",
  "de_DE.ISO8859-15",
  "de_DE.iso885915",
  "de_DE.UTF-8",
  "de_DE.utf8",
  "tr",
  "tr_TR",
  "tr_TR.1254",
  "tr_TR.CP1254",
  "tr_TR.857",
  "tr_TR.IBM857",
  "tr_TR.ISO-8859-9",
  "tr_TR.ISO8859-9",
  "tr_TR.iso88599",
  "tr_TR.UTF-8",
  "tr_TR.utf8",
  "ru",
  "ru_RU",
  "ru_RU.1251",
  "ru_RU.CP1251",
  "ru_RU.866",
  "ru_RU.IBM866",
  "ru_RU.KOI8-R",
  "ru_RU.koi8-r",
  "ru_RU.KOI8-RU",
  "ru_RU.ISO-8859-5",
  "ru_RU.ISO_8859-5",
  "ru_RU.ISO8859-5",
  "ru_RU.iso88595",
  "ru_RU.UTF-8",
  "zh_CN",
  "zh_CN.GB2312",
  "zh_CN.UTF-8",
#endif /* ! _WIN32 || __CYGWIN__ */
};

static const unsigned int locale_name_count = sizeof(locale_names)
                                              / sizeof(locale_names[0]);


/*
 *  Helper functions
 */

static int
set_test_locale (size_t num)
{
  if (num >= locale_name_count)
  {
    fprintf (stderr, "Unexpected number of locale.\n");
    exit (99);
  }
  if (verbose > 2)
    printf ("Setting locale \"%s\":", locale_names[num]);
  if (setlocale (LC_ALL, locale_names[num]))
  {
    if (verbose > 2)
      printf (" succeed.\n");
    return 1;
  }
  if (verbose > 2)
    printf (" failed.\n");
  return 0;
}


static const char *
get_current_locale_str (void)
{
  char const *loc_str = setlocale (LC_ALL, NULL);
  return loc_str ? loc_str : "unknown";
}


struct str_with_len
{
  const char *const str;
  const size_t len;
};

#define D_STR_W_LEN(s) {(s), (sizeof((s)) / sizeof(char)) - 1}

/*
 * Digits in string -> value tests
 */

struct str_with_value
{
  const struct str_with_len str;
  const size_t num_of_digt;
  const uint_fast64_t val;
};

/* valid string for conversion to unsigned integer value */
static const struct str_with_value dstrs_w_values[] = {
  /* simplest strings */
  {D_STR_W_LEN ("1"), 1, 1},
  {D_STR_W_LEN ("0"), 1, 0},
  {D_STR_W_LEN ("10000"), 5, 10000},

  /* all digits */
  {D_STR_W_LEN ("1234"), 4, 1234},
  {D_STR_W_LEN ("4567"), 4, 4567},
  {D_STR_W_LEN ("7890"), 4, 7890},
  {D_STR_W_LEN ("8021"), 4, 8021},
  {D_STR_W_LEN ("9754"), 4, 9754},
  {D_STR_W_LEN ("6392"), 4, 6392},

  /* various prefixes */
  {D_STR_W_LEN ("00000000"), 8, 0},
  {D_STR_W_LEN ("0755"), 4, 755},  /* not to be interpreted as octal value! */
  {D_STR_W_LEN ("002"), 3, 2},
  {D_STR_W_LEN ("0001"), 4, 1},
  {D_STR_W_LEN ("00000000000000000000000031295483"), 32, 31295483},

  /* numbers below and above limits */
  {D_STR_W_LEN ("127"), 3, 127},                /* 0x7F, SCHAR_MAX */
  {D_STR_W_LEN ("128"), 3, 128},                /* 0x80, SCHAR_MAX+1 */
  {D_STR_W_LEN ("255"), 3, 255},                /* 0xFF, UCHAR_MAX */
  {D_STR_W_LEN ("256"), 3, 256},                /* 0x100, UCHAR_MAX+1 */
  {D_STR_W_LEN ("32767"), 5, 32767},            /* 0x7FFF, INT16_MAX */
  {D_STR_W_LEN ("32768"), 5, 32768},            /* 0x8000, INT16_MAX+1 */
  {D_STR_W_LEN ("65535"), 5, 65535},            /* 0xFFFF, UINT16_MAX */
  {D_STR_W_LEN ("65536"), 5, 65536},            /* 0x10000, UINT16_MAX+1 */
  {D_STR_W_LEN ("2147483647"), 10, 2147483647}, /* 0x7FFFFFFF, INT32_MAX */
  {D_STR_W_LEN ("2147483648"), 10, UINT64_C (2147483648)}, /* 0x80000000, INT32_MAX+1 */
  {D_STR_W_LEN ("4294967295"), 10, UINT64_C (4294967295)}, /* 0xFFFFFFFF, UINT32_MAX */
  {D_STR_W_LEN ("4294967296"), 10, UINT64_C (4294967296)}, /* 0x100000000, UINT32_MAX+1 */
  {D_STR_W_LEN ("9223372036854775807"), 19, UINT64_C (9223372036854775807)}, /* 0x7FFFFFFFFFFFFFFF, INT64_MAX */
  {D_STR_W_LEN ("9223372036854775808"), 19, UINT64_C (9223372036854775808)}, /* 0x8000000000000000, INT64_MAX+1 */
  {D_STR_W_LEN ("18446744073709551615"), 20, UINT64_C (18446744073709551615)},  /* 0xFFFFFFFFFFFFFFFF, UINT64_MAX */

  /* random numbers */
  {D_STR_W_LEN ("10186753"), 8, 10186753},
  {D_STR_W_LEN ("144402566"), 9, 144402566},
  {D_STR_W_LEN ("151903144"), 9, 151903144},
  {D_STR_W_LEN ("139264621"), 9, 139264621},
  {D_STR_W_LEN ("730348"), 6, 730348},
  {D_STR_W_LEN ("21584377"), 8, 21584377},
  {D_STR_W_LEN ("709"), 3, 709},
  {D_STR_W_LEN ("54"), 2, 54},
  {D_STR_W_LEN ("8452"), 4, 8452},
  {D_STR_W_LEN ("17745098750013624977"), 20, UINT64_C (17745098750013624977)},
  {D_STR_W_LEN ("06786878769931678000"), 20, UINT64_C (6786878769931678000)},

  /* non-digit suffixes */
  {D_STR_W_LEN ("1234oa"), 4, 1234},
  {D_STR_W_LEN ("20h"), 2, 20},  /* not to be interpreted as hex value! */
  {D_STR_W_LEN ("0x1F"), 1, 0},  /* not to be interpreted as hex value! */
  {D_STR_W_LEN ("0564`~}"), 4, 564},
  {D_STR_W_LEN ("7240146.724"), 7, 7240146},
  {D_STR_W_LEN ("2,9"), 1, 2},
  {D_STR_W_LEN ("200+1"), 3, 200},
  {D_STR_W_LEN ("1a"), 1, 1},
  {D_STR_W_LEN ("2E"), 1, 2},
  {D_STR_W_LEN ("6c"), 1, 6},
  {D_STR_W_LEN ("8F"), 1, 8},
  {D_STR_W_LEN ("287416997! And the not too long string."), 9, 287416997}
};


/* valid hex string for conversion to unsigned integer value */
static const struct str_with_value xdstrs_w_values[] = {
  /* simplest strings */
  {D_STR_W_LEN ("1"), 1, 0x1},
  {D_STR_W_LEN ("0"), 1, 0x0},
  {D_STR_W_LEN ("10000"), 5, 0x10000},

  /* all digits */
  {D_STR_W_LEN ("1234"), 4, 0x1234},
  {D_STR_W_LEN ("4567"), 4, 0x4567},
  {D_STR_W_LEN ("7890"), 4, 0x7890},
  {D_STR_W_LEN ("8021"), 4, 0x8021},
  {D_STR_W_LEN ("9754"), 4, 0x9754},
  {D_STR_W_LEN ("6392"), 4, 0x6392},
  {D_STR_W_LEN ("abcd"), 4, 0xABCD},
  {D_STR_W_LEN ("cdef"), 4, 0xCDEF},
  {D_STR_W_LEN ("FEAB"), 4, 0xFEAB},
  {D_STR_W_LEN ("BCED"), 4, 0xBCED},
  {D_STR_W_LEN ("bCeD"), 4, 0xBCED},
  {D_STR_W_LEN ("1A5F"), 4, 0x1A5F},
  {D_STR_W_LEN ("12AB"), 4, 0x12AB},
  {D_STR_W_LEN ("CD34"), 4, 0xCD34},
  {D_STR_W_LEN ("56EF"), 4, 0x56EF},
  {D_STR_W_LEN ("7a9f"), 4, 0x7A9F},

  /* various prefixes */
  {D_STR_W_LEN ("00000000"), 8, 0x0},
  {D_STR_W_LEN ("0755"), 4, 0x755},  /* not to be interpreted as octal value! */
  {D_STR_W_LEN ("002"), 3, 0x2},
  {D_STR_W_LEN ("0001"), 4, 0x1},
  {D_STR_W_LEN ("00a"), 3, 0xA},
  {D_STR_W_LEN ("0F"), 2, 0xF},
  {D_STR_W_LEN ("0000000000000000000000003A29e4C3"), 32, 0x3A29E4C3},

  /* numbers below and above limits */
  {D_STR_W_LEN ("7F"), 2, 127},              /* 0x7F, SCHAR_MAX */
  {D_STR_W_LEN ("7f"), 2, 127},              /* 0x7F, SCHAR_MAX */
  {D_STR_W_LEN ("80"), 2, 128},              /* 0x80, SCHAR_MAX+1 */
  {D_STR_W_LEN ("fF"), 2, 255},              /* 0xFF, UCHAR_MAX */
  {D_STR_W_LEN ("Ff"), 2, 255},              /* 0xFF, UCHAR_MAX */
  {D_STR_W_LEN ("FF"), 2, 255},              /* 0xFF, UCHAR_MAX */
  {D_STR_W_LEN ("ff"), 2, 255},              /* 0xFF, UCHAR_MAX */
  {D_STR_W_LEN ("100"), 3, 256},             /* 0x100, UCHAR_MAX+1 */
  {D_STR_W_LEN ("7fff"), 4, 32767},          /* 0x7FFF, INT16_MAX */
  {D_STR_W_LEN ("7FFF"), 4, 32767},          /* 0x7FFF, INT16_MAX */
  {D_STR_W_LEN ("7Fff"), 4, 32767},          /* 0x7FFF, INT16_MAX */
  {D_STR_W_LEN ("8000"), 4, 32768},          /* 0x8000, INT16_MAX+1 */
  {D_STR_W_LEN ("ffff"), 4, 65535},          /* 0xFFFF, UINT16_MAX */
  {D_STR_W_LEN ("FFFF"), 4, 65535},          /* 0xFFFF, UINT16_MAX */
  {D_STR_W_LEN ("FffF"), 4, 65535},          /* 0xFFFF, UINT16_MAX */
  {D_STR_W_LEN ("10000"), 5, 65536},         /* 0x10000, UINT16_MAX+1 */
  {D_STR_W_LEN ("7FFFFFFF"), 8, 2147483647}, /* 0x7FFFFFFF, INT32_MAX */
  {D_STR_W_LEN ("7fffffff"), 8, 2147483647}, /* 0x7FFFFFFF, INT32_MAX */
  {D_STR_W_LEN ("7FFffFff"), 8, 2147483647}, /* 0x7FFFFFFF, INT32_MAX */
  {D_STR_W_LEN ("80000000"), 8, UINT64_C (2147483648)}, /* 0x80000000, INT32_MAX+1 */
  {D_STR_W_LEN ("FFFFFFFF"), 8, UINT64_C (4294967295)}, /* 0xFFFFFFFF, UINT32_MAX */
  {D_STR_W_LEN ("ffffffff"), 8, UINT64_C (4294967295)}, /* 0xFFFFFFFF, UINT32_MAX */
  {D_STR_W_LEN ("FfFfFfFf"), 8, UINT64_C (4294967295)}, /* 0xFFFFFFFF, UINT32_MAX */
  {D_STR_W_LEN ("100000000"), 9, UINT64_C (4294967296)}, /* 0x100000000, UINT32_MAX+1 */
  {D_STR_W_LEN ("7fffffffffffffff"), 16, UINT64_C (9223372036854775807)}, /* 0x7FFFFFFFFFFFFFFF, INT64_MAX */
  {D_STR_W_LEN ("7FFFFFFFFFFFFFFF"), 16, UINT64_C (9223372036854775807)}, /* 0x7FFFFFFFFFFFFFFF, INT64_MAX */
  {D_STR_W_LEN ("7FfffFFFFffFFffF"), 16, UINT64_C (9223372036854775807)}, /* 0x7FFFFFFFFFFFFFFF, INT64_MAX */
  {D_STR_W_LEN ("8000000000000000"), 16, UINT64_C (9223372036854775808)}, /* 0x8000000000000000, INT64_MAX+1 */
  {D_STR_W_LEN ("ffffffffffffffff"), 16, UINT64_C (18446744073709551615)},  /* 0xFFFFFFFFFFFFFFFF, UINT64_MAX */
  {D_STR_W_LEN ("FFFFFFFFFFFFFFFF"), 16, UINT64_C (18446744073709551615)},  /* 0xFFFFFFFFFFFFFFFF, UINT64_MAX */
  {D_STR_W_LEN ("FffFffFFffFFfFFF"), 16, UINT64_C (18446744073709551615)},  /* 0xFFFFFFFFFFFFFFFF, UINT64_MAX */

  /* random numbers */
  {D_STR_W_LEN ("10186753"), 8, 0x10186753},
  {D_STR_W_LEN ("144402566"), 9, 0x144402566},
  {D_STR_W_LEN ("151903144"), 9, 0x151903144},
  {D_STR_W_LEN ("139264621"), 9, 0x139264621},
  {D_STR_W_LEN ("730348"), 6, 0x730348},
  {D_STR_W_LEN ("21584377"), 8, 0x21584377},
  {D_STR_W_LEN ("709"), 3, 0x709},
  {D_STR_W_LEN ("54"), 2, 0x54},
  {D_STR_W_LEN ("8452"), 4, 0x8452},
  {D_STR_W_LEN ("22353EC6"), 8, 0x22353EC6},
  {D_STR_W_LEN ("307F1655"), 8, 0x307F1655},
  {D_STR_W_LEN ("1FCB7226"), 8, 0x1FCB7226},
  {D_STR_W_LEN ("82480560"), 8, 0x82480560},
  {D_STR_W_LEN ("7386D95"), 7, 0x7386D95},
  {D_STR_W_LEN ("EC3AB"), 5, 0xEC3AB},
  {D_STR_W_LEN ("6DD05"), 5, 0x6DD05},
  {D_STR_W_LEN ("C5DF"), 4, 0xC5DF},
  {D_STR_W_LEN ("6CE"), 3, 0x6CE},
  {D_STR_W_LEN ("CE6"), 3, 0xCE6},
  {D_STR_W_LEN ("ce6"), 3, 0xCE6},
  {D_STR_W_LEN ("F27"), 3, 0xF27},
  {D_STR_W_LEN ("8497D54277D7E1"), 14, UINT64_C (37321639124785121)},
  {D_STR_W_LEN ("8497d54277d7e1"), 14, UINT64_C (37321639124785121)},
  {D_STR_W_LEN ("8497d54277d7E1"), 14, UINT64_C (37321639124785121)},
  {D_STR_W_LEN ("8C8112D0A06"), 11, UINT64_C (9655374645766)},
  {D_STR_W_LEN ("8c8112d0a06"), 11, UINT64_C (9655374645766)},
  {D_STR_W_LEN ("8c8112d0A06"), 11, UINT64_C (9655374645766)},
  {D_STR_W_LEN ("1774509875001362"), 16, UINT64_C (1690064375898968930)},
  {D_STR_W_LEN ("0678687876998000"), 16, UINT64_C (466237428027981824)},

  /* non-digit suffixes */
  {D_STR_W_LEN ("1234oa"), 4, 0x1234},
  {D_STR_W_LEN ("20h"), 2, 0x20},
  {D_STR_W_LEN ("2CH"), 2, 0x2C},
  {D_STR_W_LEN ("2ch"), 2, 0x2C},
  {D_STR_W_LEN ("0x1F"), 1, 0x0},  /* not to be interpreted as hex prefix! */
  {D_STR_W_LEN ("0564`~}"), 4, 0x564},
  {D_STR_W_LEN ("0A64`~}"), 4, 0xA64},
  {D_STR_W_LEN ("056c`~}"), 4, 0X56C},
  {D_STR_W_LEN ("7240146.724"), 7, 0x7240146},
  {D_STR_W_LEN ("7E4c1AB.724"), 7, 0X7E4C1AB},
  {D_STR_W_LEN ("F24B1B6.724"), 7, 0xF24B1B6},
  {D_STR_W_LEN ("2,9"), 1, 0x2},
  {D_STR_W_LEN ("a,9"), 1, 0xA},
  {D_STR_W_LEN ("200+1"), 3, 0x200},
  {D_STR_W_LEN ("2cc+1"), 3, 0x2CC},
  {D_STR_W_LEN ("2cC+1"), 3, 0x2CC},
  {D_STR_W_LEN ("27416997! And the not too long string."), 8, 0x27416997},
  {D_STR_W_LEN ("27555416997! And the not too long string."), 11,
   0x27555416997},
  {D_STR_W_LEN ("416997And the not too long string."), 7, 0x416997A},
  {D_STR_W_LEN ("0F4C3Dabstract addition to make string even longer"), 8,
   0xF4C3DAB}
};


static size_t
check_str_from_uint16 (void)
{
  size_t t_failed = 0;
  size_t i, j;
  char buf[70];
  const char *erase =
    "-@=sd#+&(pdiren456qwe#@C3S!DAS45AOIPUQWESAdFzxcv1s*()&#$%34`"
    "32452d098poiden45SADFFDA3S4D3SDFdfgsdfgsSADFzxdvs$*()&#2342`"
    "adsf##$$@&*^%*^&56qwe#3C@S!DAScFAOIP$#%#$Ad1zs3v1$*()&#1228`";
  int c_failed[sizeof(dstrs_w_values)
               / sizeof(dstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      const struct str_with_value *const t = dstrs_w_values + i;
      size_t b_size;
      size_t rs;

      if (c_failed[i])
        continue;     /* skip already failed checks */

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      if ('0' == t->str.str[0])
        continue;  /* Skip strings prefixed with zeros */
      if (t->num_of_digt != t->str.len)
        continue;  /* Skip strings with suffixes */
      if ((t->val & 0xFFFFu) != t->val)
        continue;  /* Too large value to convert */
      if (sizeof(buf) < t->str.len + 1)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has too long (%u) string, "
                 "size of 'buf' should be increased.\n",
                 (unsigned int) i, (unsigned int) t->str.len);
        exit (99);
      }
      rs = 0; /* Only to mute compiler warning */
      for (b_size = 0; b_size <= t->str.len + 1; ++b_size)
      {
        /* fill buffer with pseudo-random values */
        memcpy (buf, erase, sizeof(buf));

        rs = mhd_uint16_to_str ((uint_least16_t) t->val, buf, b_size);

        if (t->num_of_digt > b_size)
        {
          /* Must fail, buffer is too small for result */
          if (0 != rs)
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint16_to_str(%" PRIuFAST64 ", -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting 0."
                     " Locale: %s\n", t->val, (int) b_size, (uintptr_t) rs,
                     get_current_locale_str ());
          }
        }
        else
        {
          if (t->num_of_digt != rs)
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint16_to_str(%" PRIuFAST64 ", -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting %d."
                     " Locale: %s\n", t->val, (int) b_size, (uintptr_t) rs,
                     (int) t->num_of_digt, get_current_locale_str ());
          }
          else if (0 != memcmp (buf, t->str.str, t->num_of_digt))
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint16_to_str(%" PRIuFAST64 ", -> \"%.*s\","
                     " %d) returned %" PRIuPTR "."
                     " Locale: %s\n", t->val, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,  get_current_locale_str ());
          }
          else if (0 != memcmp (buf + rs, erase + rs, sizeof(buf) - rs))
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint16_to_str(%" PRIuFAST64 ", -> \"%.*s\","
                     " %d) returned %" PRIuPTR
                     " and touched data after the resulting string."
                     " Locale: %s\n", t->val, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,  get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_uint16_to_str(%" PRIuFAST64 ", -> \"%.*s\", %d) "
                "== %" PRIuPTR "\n",
                t->val, (int) rs, buf, (int) b_size - 1, (uintptr_t) rs);
    }
  }
  return t_failed;
}


static size_t
check_str_from_uint64 (void)
{
  size_t t_failed = 0;
  size_t i, j;
  char buf[70];
  const char *erase =
    "-@=sd#+&(pdirenDSFGSe#@C&S!DAS*!AOIPUQWESAdFzxcvSs*()&#$%KH`"
    "32452d098poiden45SADFFDA3S4D3SDFdfgsdfgsSADFzxdvs$*()&#2342`"
    "adsf##$$@&*^%*^&56qwe#3C@S!DAScFAOIP$#%#$Ad1zs3v1$*()&#1228`";
  int c_failed[sizeof(dstrs_w_values)
               / sizeof(dstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      const struct str_with_value *const t = dstrs_w_values + i;
      size_t b_size;
      size_t rs;

      if (c_failed[i])
        continue;     /* skip already failed checks */

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      if ('0' == t->str.str[0])
        continue;  /* Skip strings prefixed with zeros */
      if (t->num_of_digt != t->str.len)
        continue;  /* Skip strings with suffixes */
      if (sizeof(buf) < t->str.len + 1)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has too long (%u) string, "
                 "size of 'buf' should be increased.\n",
                 (unsigned int) i, (unsigned int) t->str.len);
        exit (99);
      }
      rs = 0; /* Only to mute compiler warning */
      for (b_size = 0; b_size <= t->str.len + 1; ++b_size)
      {
        /* fill buffer with pseudo-random values */
        memcpy (buf, erase, sizeof(buf));

        rs = mhd_uint64_to_str (t->val, buf, b_size);

        if (t->num_of_digt > b_size)
        {
          /* Must fail, buffer is too small for result */
          if (0 != rs)
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint64_to_str(%" PRIuFAST64 ", -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting 0."
                     " Locale: %s\n", t->val, (int) b_size, (uintptr_t) rs,
                     get_current_locale_str ());
          }
        }
        else
        {
          if (t->num_of_digt != rs)
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint64_to_str(%" PRIuFAST64 ", -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting %d."
                     " Locale: %s\n", t->val, (int) b_size, (uintptr_t) rs,
                     (int) t->num_of_digt, get_current_locale_str ());
          }
          else if (0 != memcmp (buf, t->str.str, t->num_of_digt))
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint64_to_str(%" PRIuFAST64 ", -> \"%.*s\","
                     " %d) returned %" PRIuPTR "."
                     " Locale: %s\n", t->val, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,  get_current_locale_str ());
          }
          else if (0 != memcmp (buf + rs, erase + rs, sizeof(buf) - rs))
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint64_to_str(%" PRIuFAST64 ", -> \"%.*s\","
                     " %d) returned %" PRIuPTR
                     " and touched data after the resulting string."
                     " Locale: %s\n", t->val, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,  get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_uint64_to_str(%" PRIuFAST64 ", -> \"%.*s\", %d) "
                "== %" PRIuPTR "\n",
                t->val, (int) rs, buf, (int) b_size - 1, (uintptr_t) rs);
    }
  }
  return t_failed;
}


static size_t
check_strx_from_uint32 (void)
{
  size_t t_failed = 0;
  size_t i, j;
  char buf[70];
  const char *erase =
    "jrlkjssfhjfvrjntJHLJ$@%$#adsfdkj;k$##$%#$%FGDF%$#^FDFG%$#$D`"
    ";skjdhjflsdkjhdjfalskdjhdfalkjdhf$%##%$$#%FSDGFSDDGDFSSDSDF`"
    "#5#$%#$#$DFSFDDFSGSDFSDF354FDDSGFDFfdssfddfswqemn,.zxih,.sx`";
  int c_failed[sizeof(xdstrs_w_values)
               / sizeof(xdstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      const struct str_with_value *const t = xdstrs_w_values + i;
      size_t b_size;
      size_t rs;

      if (c_failed[i])
        continue;     /* skip already failed checks */

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      if ('0' == t->str.str[0])
        continue;  /* Skip strings prefixed with zeros */
      if (t->num_of_digt != t->str.len)
        continue;  /* Skip strings with suffixes */
      if ((t->val & 0xFFFFFFFFu) != t->val)
        continue;  /* Too large value to convert */
      if (sizeof(buf) < t->str.len + 1)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has too long (%u) string, "
                 "size of 'buf' should be increased.\n",
                 (unsigned int) i, (unsigned int) t->str.len);
        exit (99);
      }
      rs = 0; /* Only to mute compiler warning */
      for (b_size = 0; b_size <= t->str.len + 1; ++b_size)
      {
        /* fill buffer with pseudo-random values */
        memcpy (buf, erase, sizeof(buf));

        rs = mhd_uint32_to_strx ((uint_fast32_t) t->val, buf, b_size);

        if (t->num_of_digt > b_size)
        {
          /* Must fail, buffer is too small for result */
          if (0 != rs)
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint32_to_strx(0x%" PRIXFAST64 ", -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting 0."
                     " Locale: %s\n", t->val, (int) b_size, (uintptr_t) rs,
                     get_current_locale_str ());
          }
        }
        else
        {
          if (t->num_of_digt != rs)
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint32_to_strx(0x%" PRIXFAST64 ", -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting %d."
                     " Locale: %s\n", t->val, (int) b_size, (uintptr_t) rs,
                     (int) t->num_of_digt, get_current_locale_str ());
          }
          else if (0 == mhd_str_equal_caseless_bin_n (buf, t->str.str,
                                                      t->num_of_digt))
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint32_to_strx(0x%" PRIXFAST64
                     ", -> \"%.*s\","
                     " %d) returned %" PRIuPTR "."
                     " Locale: %s\n", t->val, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,  get_current_locale_str ());
          }
          else if (sizeof(buf) <= rs)
          {
            fprintf (stderr,
                     "ERROR: dstrs_w_values[%u] has string with too many"
                     "(%u) digits, size of 'buf' should be increased.\n",
                     (unsigned int) i, (unsigned int) rs);
            exit (99);
          }
          else if (0 != memcmp (buf + rs, erase + rs, sizeof(buf) - rs))
          {
            if (0 == c_failed[i])
              t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_uint32_to_strx(0x%" PRIXFAST64
                     ", -> \"%.*s\","
                     " %d) returned %" PRIuPTR
                     " and touched data after the resulting string."
                     " Locale: %s\n", t->val, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,  get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_uint32_to_strx(0x%" PRIXFAST64
                ", -> \"%.*s\", %d) "
                "== %" PRIuPTR "\n",
                t->val, (int) rs, buf, (int) b_size - 1, (uintptr_t) rs);
    }
  }
  return t_failed;
}


static const struct str_with_value duint8_w_values_p1[] = {
  {D_STR_W_LEN ("0"), 1, 0},
  {D_STR_W_LEN ("1"), 1, 1},
  {D_STR_W_LEN ("2"), 1, 2},
  {D_STR_W_LEN ("3"), 1, 3},
  {D_STR_W_LEN ("4"), 1, 4},
  {D_STR_W_LEN ("5"), 1, 5},
  {D_STR_W_LEN ("6"), 1, 6},
  {D_STR_W_LEN ("7"), 1, 7},
  {D_STR_W_LEN ("8"), 1, 8},
  {D_STR_W_LEN ("9"), 1, 9},
  {D_STR_W_LEN ("10"), 2, 10},
  {D_STR_W_LEN ("11"), 2, 11},
  {D_STR_W_LEN ("12"), 2, 12},
  {D_STR_W_LEN ("13"), 2, 13},
  {D_STR_W_LEN ("14"), 2, 14},
  {D_STR_W_LEN ("15"), 2, 15},
  {D_STR_W_LEN ("16"), 2, 16},
  {D_STR_W_LEN ("17"), 2, 17},
  {D_STR_W_LEN ("18"), 2, 18},
  {D_STR_W_LEN ("19"), 2, 19},
  {D_STR_W_LEN ("20"), 2, 20},
  {D_STR_W_LEN ("21"), 2, 21},
  {D_STR_W_LEN ("22"), 2, 22},
  {D_STR_W_LEN ("23"), 2, 23},
  {D_STR_W_LEN ("24"), 2, 24},
  {D_STR_W_LEN ("25"), 2, 25},
  {D_STR_W_LEN ("26"), 2, 26},
  {D_STR_W_LEN ("27"), 2, 27},
  {D_STR_W_LEN ("28"), 2, 28},
  {D_STR_W_LEN ("29"), 2, 29},
  {D_STR_W_LEN ("30"), 2, 30},
  {D_STR_W_LEN ("31"), 2, 31},
  {D_STR_W_LEN ("32"), 2, 32},
  {D_STR_W_LEN ("33"), 2, 33},
  {D_STR_W_LEN ("34"), 2, 34},
  {D_STR_W_LEN ("35"), 2, 35},
  {D_STR_W_LEN ("36"), 2, 36},
  {D_STR_W_LEN ("37"), 2, 37},
  {D_STR_W_LEN ("38"), 2, 38},
  {D_STR_W_LEN ("39"), 2, 39},
  {D_STR_W_LEN ("40"), 2, 40},
  {D_STR_W_LEN ("41"), 2, 41},
  {D_STR_W_LEN ("42"), 2, 42},
  {D_STR_W_LEN ("43"), 2, 43},
  {D_STR_W_LEN ("44"), 2, 44},
  {D_STR_W_LEN ("45"), 2, 45},
  {D_STR_W_LEN ("46"), 2, 46},
  {D_STR_W_LEN ("47"), 2, 47},
  {D_STR_W_LEN ("48"), 2, 48},
  {D_STR_W_LEN ("49"), 2, 49},
  {D_STR_W_LEN ("50"), 2, 50},
  {D_STR_W_LEN ("51"), 2, 51},
  {D_STR_W_LEN ("52"), 2, 52},
  {D_STR_W_LEN ("53"), 2, 53},
  {D_STR_W_LEN ("54"), 2, 54},
  {D_STR_W_LEN ("55"), 2, 55},
  {D_STR_W_LEN ("56"), 2, 56},
  {D_STR_W_LEN ("57"), 2, 57},
  {D_STR_W_LEN ("58"), 2, 58},
  {D_STR_W_LEN ("59"), 2, 59},
  {D_STR_W_LEN ("60"), 2, 60},
  {D_STR_W_LEN ("61"), 2, 61},
  {D_STR_W_LEN ("62"), 2, 62},
  {D_STR_W_LEN ("63"), 2, 63},
  {D_STR_W_LEN ("64"), 2, 64},
  {D_STR_W_LEN ("65"), 2, 65},
  {D_STR_W_LEN ("66"), 2, 66},
  {D_STR_W_LEN ("67"), 2, 67},
  {D_STR_W_LEN ("68"), 2, 68},
  {D_STR_W_LEN ("69"), 2, 69},
  {D_STR_W_LEN ("70"), 2, 70},
  {D_STR_W_LEN ("71"), 2, 71},
  {D_STR_W_LEN ("72"), 2, 72},
  {D_STR_W_LEN ("73"), 2, 73},
  {D_STR_W_LEN ("74"), 2, 74},
  {D_STR_W_LEN ("75"), 2, 75},
  {D_STR_W_LEN ("76"), 2, 76},
  {D_STR_W_LEN ("77"), 2, 77},
  {D_STR_W_LEN ("78"), 2, 78},
  {D_STR_W_LEN ("79"), 2, 79},
  {D_STR_W_LEN ("80"), 2, 80},
  {D_STR_W_LEN ("81"), 2, 81},
  {D_STR_W_LEN ("82"), 2, 82},
  {D_STR_W_LEN ("83"), 2, 83},
  {D_STR_W_LEN ("84"), 2, 84},
  {D_STR_W_LEN ("85"), 2, 85},
  {D_STR_W_LEN ("86"), 2, 86},
  {D_STR_W_LEN ("87"), 2, 87},
  {D_STR_W_LEN ("88"), 2, 88},
  {D_STR_W_LEN ("89"), 2, 89},
  {D_STR_W_LEN ("90"), 2, 90},
  {D_STR_W_LEN ("91"), 2, 91},
  {D_STR_W_LEN ("92"), 2, 92},
  {D_STR_W_LEN ("93"), 2, 93},
  {D_STR_W_LEN ("94"), 2, 94},
  {D_STR_W_LEN ("95"), 2, 95},
  {D_STR_W_LEN ("96"), 2, 96},
  {D_STR_W_LEN ("97"), 2, 97},
  {D_STR_W_LEN ("98"), 2, 98},
  {D_STR_W_LEN ("99"), 2, 99},
  {D_STR_W_LEN ("100"), 3, 100},
  {D_STR_W_LEN ("101"), 3, 101},
  {D_STR_W_LEN ("102"), 3, 102},
  {D_STR_W_LEN ("103"), 3, 103},
  {D_STR_W_LEN ("104"), 3, 104},
  {D_STR_W_LEN ("105"), 3, 105},
  {D_STR_W_LEN ("106"), 3, 106},
  {D_STR_W_LEN ("107"), 3, 107},
  {D_STR_W_LEN ("108"), 3, 108},
  {D_STR_W_LEN ("109"), 3, 109},
  {D_STR_W_LEN ("110"), 3, 110},
  {D_STR_W_LEN ("111"), 3, 111},
  {D_STR_W_LEN ("112"), 3, 112},
  {D_STR_W_LEN ("113"), 3, 113},
  {D_STR_W_LEN ("114"), 3, 114},
  {D_STR_W_LEN ("115"), 3, 115},
  {D_STR_W_LEN ("116"), 3, 116},
  {D_STR_W_LEN ("117"), 3, 117},
  {D_STR_W_LEN ("118"), 3, 118},
  {D_STR_W_LEN ("119"), 3, 119},
  {D_STR_W_LEN ("120"), 3, 120},
  {D_STR_W_LEN ("121"), 3, 121},
  {D_STR_W_LEN ("122"), 3, 122},
  {D_STR_W_LEN ("123"), 3, 123},
  {D_STR_W_LEN ("124"), 3, 124},
  {D_STR_W_LEN ("125"), 3, 125},
  {D_STR_W_LEN ("126"), 3, 126},
  {D_STR_W_LEN ("127"), 3, 127},
  {D_STR_W_LEN ("128"), 3, 128},
  {D_STR_W_LEN ("129"), 3, 129},
  {D_STR_W_LEN ("130"), 3, 130},
  {D_STR_W_LEN ("131"), 3, 131},
  {D_STR_W_LEN ("132"), 3, 132},
  {D_STR_W_LEN ("133"), 3, 133},
  {D_STR_W_LEN ("134"), 3, 134},
  {D_STR_W_LEN ("135"), 3, 135},
  {D_STR_W_LEN ("136"), 3, 136},
  {D_STR_W_LEN ("137"), 3, 137},
  {D_STR_W_LEN ("138"), 3, 138},
  {D_STR_W_LEN ("139"), 3, 139},
  {D_STR_W_LEN ("140"), 3, 140},
  {D_STR_W_LEN ("141"), 3, 141},
  {D_STR_W_LEN ("142"), 3, 142},
  {D_STR_W_LEN ("143"), 3, 143},
  {D_STR_W_LEN ("144"), 3, 144},
  {D_STR_W_LEN ("145"), 3, 145},
  {D_STR_W_LEN ("146"), 3, 146},
  {D_STR_W_LEN ("147"), 3, 147},
  {D_STR_W_LEN ("148"), 3, 148},
  {D_STR_W_LEN ("149"), 3, 149},
  {D_STR_W_LEN ("150"), 3, 150},
  {D_STR_W_LEN ("151"), 3, 151},
  {D_STR_W_LEN ("152"), 3, 152},
  {D_STR_W_LEN ("153"), 3, 153},
  {D_STR_W_LEN ("154"), 3, 154},
  {D_STR_W_LEN ("155"), 3, 155},
  {D_STR_W_LEN ("156"), 3, 156},
  {D_STR_W_LEN ("157"), 3, 157},
  {D_STR_W_LEN ("158"), 3, 158},
  {D_STR_W_LEN ("159"), 3, 159},
  {D_STR_W_LEN ("160"), 3, 160},
  {D_STR_W_LEN ("161"), 3, 161},
  {D_STR_W_LEN ("162"), 3, 162},
  {D_STR_W_LEN ("163"), 3, 163},
  {D_STR_W_LEN ("164"), 3, 164},
  {D_STR_W_LEN ("165"), 3, 165},
  {D_STR_W_LEN ("166"), 3, 166},
  {D_STR_W_LEN ("167"), 3, 167},
  {D_STR_W_LEN ("168"), 3, 168},
  {D_STR_W_LEN ("169"), 3, 169},
  {D_STR_W_LEN ("170"), 3, 170},
  {D_STR_W_LEN ("171"), 3, 171},
  {D_STR_W_LEN ("172"), 3, 172},
  {D_STR_W_LEN ("173"), 3, 173},
  {D_STR_W_LEN ("174"), 3, 174},
  {D_STR_W_LEN ("175"), 3, 175},
  {D_STR_W_LEN ("176"), 3, 176},
  {D_STR_W_LEN ("177"), 3, 177},
  {D_STR_W_LEN ("178"), 3, 178},
  {D_STR_W_LEN ("179"), 3, 179},
  {D_STR_W_LEN ("180"), 3, 180},
  {D_STR_W_LEN ("181"), 3, 181},
  {D_STR_W_LEN ("182"), 3, 182},
  {D_STR_W_LEN ("183"), 3, 183},
  {D_STR_W_LEN ("184"), 3, 184},
  {D_STR_W_LEN ("185"), 3, 185},
  {D_STR_W_LEN ("186"), 3, 186},
  {D_STR_W_LEN ("187"), 3, 187},
  {D_STR_W_LEN ("188"), 3, 188},
  {D_STR_W_LEN ("189"), 3, 189},
  {D_STR_W_LEN ("190"), 3, 190},
  {D_STR_W_LEN ("191"), 3, 191},
  {D_STR_W_LEN ("192"), 3, 192},
  {D_STR_W_LEN ("193"), 3, 193},
  {D_STR_W_LEN ("194"), 3, 194},
  {D_STR_W_LEN ("195"), 3, 195},
  {D_STR_W_LEN ("196"), 3, 196},
  {D_STR_W_LEN ("197"), 3, 197},
  {D_STR_W_LEN ("198"), 3, 198},
  {D_STR_W_LEN ("199"), 3, 199},
  {D_STR_W_LEN ("200"), 3, 200},
  {D_STR_W_LEN ("201"), 3, 201},
  {D_STR_W_LEN ("202"), 3, 202},
  {D_STR_W_LEN ("203"), 3, 203},
  {D_STR_W_LEN ("204"), 3, 204},
  {D_STR_W_LEN ("205"), 3, 205},
  {D_STR_W_LEN ("206"), 3, 206},
  {D_STR_W_LEN ("207"), 3, 207},
  {D_STR_W_LEN ("208"), 3, 208},
  {D_STR_W_LEN ("209"), 3, 209},
  {D_STR_W_LEN ("210"), 3, 210},
  {D_STR_W_LEN ("211"), 3, 211},
  {D_STR_W_LEN ("212"), 3, 212},
  {D_STR_W_LEN ("213"), 3, 213},
  {D_STR_W_LEN ("214"), 3, 214},
  {D_STR_W_LEN ("215"), 3, 215},
  {D_STR_W_LEN ("216"), 3, 216},
  {D_STR_W_LEN ("217"), 3, 217},
  {D_STR_W_LEN ("218"), 3, 218},
  {D_STR_W_LEN ("219"), 3, 219},
  {D_STR_W_LEN ("220"), 3, 220},
  {D_STR_W_LEN ("221"), 3, 221},
  {D_STR_W_LEN ("222"), 3, 222},
  {D_STR_W_LEN ("223"), 3, 223},
  {D_STR_W_LEN ("224"), 3, 224},
  {D_STR_W_LEN ("225"), 3, 225},
  {D_STR_W_LEN ("226"), 3, 226},
  {D_STR_W_LEN ("227"), 3, 227},
  {D_STR_W_LEN ("228"), 3, 228},
  {D_STR_W_LEN ("229"), 3, 229},
  {D_STR_W_LEN ("230"), 3, 230},
  {D_STR_W_LEN ("231"), 3, 231},
  {D_STR_W_LEN ("232"), 3, 232},
  {D_STR_W_LEN ("233"), 3, 233},
  {D_STR_W_LEN ("234"), 3, 234},
  {D_STR_W_LEN ("235"), 3, 235},
  {D_STR_W_LEN ("236"), 3, 236},
  {D_STR_W_LEN ("237"), 3, 237},
  {D_STR_W_LEN ("238"), 3, 238},
  {D_STR_W_LEN ("239"), 3, 239},
  {D_STR_W_LEN ("240"), 3, 240},
  {D_STR_W_LEN ("241"), 3, 241},
  {D_STR_W_LEN ("242"), 3, 242},
  {D_STR_W_LEN ("243"), 3, 243},
  {D_STR_W_LEN ("244"), 3, 244},
  {D_STR_W_LEN ("245"), 3, 245},
  {D_STR_W_LEN ("246"), 3, 246},
  {D_STR_W_LEN ("247"), 3, 247},
  {D_STR_W_LEN ("248"), 3, 248},
  {D_STR_W_LEN ("249"), 3, 249},
  {D_STR_W_LEN ("250"), 3, 250},
  {D_STR_W_LEN ("251"), 3, 251},
  {D_STR_W_LEN ("252"), 3, 252},
  {D_STR_W_LEN ("253"), 3, 253},
  {D_STR_W_LEN ("254"), 3, 254},
  {D_STR_W_LEN ("255"), 3, 255},
};

static const struct str_with_value duint8_w_values_p2[] = {
  {D_STR_W_LEN ("00"), 2, 0},
  {D_STR_W_LEN ("01"), 2, 1},
  {D_STR_W_LEN ("02"), 2, 2},
  {D_STR_W_LEN ("03"), 2, 3},
  {D_STR_W_LEN ("04"), 2, 4},
  {D_STR_W_LEN ("05"), 2, 5},
  {D_STR_W_LEN ("06"), 2, 6},
  {D_STR_W_LEN ("07"), 2, 7},
  {D_STR_W_LEN ("08"), 2, 8},
  {D_STR_W_LEN ("09"), 2, 9},
  {D_STR_W_LEN ("10"), 2, 10},
  {D_STR_W_LEN ("11"), 2, 11},
  {D_STR_W_LEN ("12"), 2, 12},
  {D_STR_W_LEN ("13"), 2, 13},
  {D_STR_W_LEN ("14"), 2, 14},
  {D_STR_W_LEN ("15"), 2, 15},
  {D_STR_W_LEN ("16"), 2, 16},
  {D_STR_W_LEN ("17"), 2, 17},
  {D_STR_W_LEN ("18"), 2, 18},
  {D_STR_W_LEN ("19"), 2, 19},
  {D_STR_W_LEN ("20"), 2, 20},
  {D_STR_W_LEN ("21"), 2, 21},
  {D_STR_W_LEN ("22"), 2, 22},
  {D_STR_W_LEN ("23"), 2, 23},
  {D_STR_W_LEN ("24"), 2, 24},
  {D_STR_W_LEN ("25"), 2, 25},
  {D_STR_W_LEN ("26"), 2, 26},
  {D_STR_W_LEN ("27"), 2, 27},
  {D_STR_W_LEN ("28"), 2, 28},
  {D_STR_W_LEN ("29"), 2, 29},
  {D_STR_W_LEN ("30"), 2, 30},
  {D_STR_W_LEN ("31"), 2, 31},
  {D_STR_W_LEN ("32"), 2, 32},
  {D_STR_W_LEN ("33"), 2, 33},
  {D_STR_W_LEN ("34"), 2, 34},
  {D_STR_W_LEN ("35"), 2, 35},
  {D_STR_W_LEN ("36"), 2, 36},
  {D_STR_W_LEN ("37"), 2, 37},
  {D_STR_W_LEN ("38"), 2, 38},
  {D_STR_W_LEN ("39"), 2, 39},
  {D_STR_W_LEN ("40"), 2, 40},
  {D_STR_W_LEN ("41"), 2, 41},
  {D_STR_W_LEN ("42"), 2, 42},
  {D_STR_W_LEN ("43"), 2, 43},
  {D_STR_W_LEN ("44"), 2, 44},
  {D_STR_W_LEN ("45"), 2, 45},
  {D_STR_W_LEN ("46"), 2, 46},
  {D_STR_W_LEN ("47"), 2, 47},
  {D_STR_W_LEN ("48"), 2, 48},
  {D_STR_W_LEN ("49"), 2, 49},
  {D_STR_W_LEN ("50"), 2, 50},
  {D_STR_W_LEN ("51"), 2, 51},
  {D_STR_W_LEN ("52"), 2, 52},
  {D_STR_W_LEN ("53"), 2, 53},
  {D_STR_W_LEN ("54"), 2, 54},
  {D_STR_W_LEN ("55"), 2, 55},
  {D_STR_W_LEN ("56"), 2, 56},
  {D_STR_W_LEN ("57"), 2, 57},
  {D_STR_W_LEN ("58"), 2, 58},
  {D_STR_W_LEN ("59"), 2, 59},
  {D_STR_W_LEN ("60"), 2, 60},
  {D_STR_W_LEN ("61"), 2, 61},
  {D_STR_W_LEN ("62"), 2, 62},
  {D_STR_W_LEN ("63"), 2, 63},
  {D_STR_W_LEN ("64"), 2, 64},
  {D_STR_W_LEN ("65"), 2, 65},
  {D_STR_W_LEN ("66"), 2, 66},
  {D_STR_W_LEN ("67"), 2, 67},
  {D_STR_W_LEN ("68"), 2, 68},
  {D_STR_W_LEN ("69"), 2, 69},
  {D_STR_W_LEN ("70"), 2, 70},
  {D_STR_W_LEN ("71"), 2, 71},
  {D_STR_W_LEN ("72"), 2, 72},
  {D_STR_W_LEN ("73"), 2, 73},
  {D_STR_W_LEN ("74"), 2, 74},
  {D_STR_W_LEN ("75"), 2, 75},
  {D_STR_W_LEN ("76"), 2, 76},
  {D_STR_W_LEN ("77"), 2, 77},
  {D_STR_W_LEN ("78"), 2, 78},
  {D_STR_W_LEN ("79"), 2, 79},
  {D_STR_W_LEN ("80"), 2, 80},
  {D_STR_W_LEN ("81"), 2, 81},
  {D_STR_W_LEN ("82"), 2, 82},
  {D_STR_W_LEN ("83"), 2, 83},
  {D_STR_W_LEN ("84"), 2, 84},
  {D_STR_W_LEN ("85"), 2, 85},
  {D_STR_W_LEN ("86"), 2, 86},
  {D_STR_W_LEN ("87"), 2, 87},
  {D_STR_W_LEN ("88"), 2, 88},
  {D_STR_W_LEN ("89"), 2, 89},
  {D_STR_W_LEN ("90"), 2, 90},
  {D_STR_W_LEN ("91"), 2, 91},
  {D_STR_W_LEN ("92"), 2, 92},
  {D_STR_W_LEN ("93"), 2, 93},
  {D_STR_W_LEN ("94"), 2, 94},
  {D_STR_W_LEN ("95"), 2, 95},
  {D_STR_W_LEN ("96"), 2, 96},
  {D_STR_W_LEN ("97"), 2, 97},
  {D_STR_W_LEN ("98"), 2, 98},
  {D_STR_W_LEN ("99"), 2, 99},
  {D_STR_W_LEN ("100"), 3, 100},
  {D_STR_W_LEN ("101"), 3, 101},
  {D_STR_W_LEN ("102"), 3, 102},
  {D_STR_W_LEN ("103"), 3, 103},
  {D_STR_W_LEN ("104"), 3, 104},
  {D_STR_W_LEN ("105"), 3, 105},
  {D_STR_W_LEN ("106"), 3, 106},
  {D_STR_W_LEN ("107"), 3, 107},
  {D_STR_W_LEN ("108"), 3, 108},
  {D_STR_W_LEN ("109"), 3, 109},
  {D_STR_W_LEN ("110"), 3, 110},
  {D_STR_W_LEN ("111"), 3, 111},
  {D_STR_W_LEN ("112"), 3, 112},
  {D_STR_W_LEN ("113"), 3, 113},
  {D_STR_W_LEN ("114"), 3, 114},
  {D_STR_W_LEN ("115"), 3, 115},
  {D_STR_W_LEN ("116"), 3, 116},
  {D_STR_W_LEN ("117"), 3, 117},
  {D_STR_W_LEN ("118"), 3, 118},
  {D_STR_W_LEN ("119"), 3, 119},
  {D_STR_W_LEN ("120"), 3, 120},
  {D_STR_W_LEN ("121"), 3, 121},
  {D_STR_W_LEN ("122"), 3, 122},
  {D_STR_W_LEN ("123"), 3, 123},
  {D_STR_W_LEN ("124"), 3, 124},
  {D_STR_W_LEN ("125"), 3, 125},
  {D_STR_W_LEN ("126"), 3, 126},
  {D_STR_W_LEN ("127"), 3, 127},
  {D_STR_W_LEN ("128"), 3, 128},
  {D_STR_W_LEN ("129"), 3, 129},
  {D_STR_W_LEN ("130"), 3, 130},
  {D_STR_W_LEN ("131"), 3, 131},
  {D_STR_W_LEN ("132"), 3, 132},
  {D_STR_W_LEN ("133"), 3, 133},
  {D_STR_W_LEN ("134"), 3, 134},
  {D_STR_W_LEN ("135"), 3, 135},
  {D_STR_W_LEN ("136"), 3, 136},
  {D_STR_W_LEN ("137"), 3, 137},
  {D_STR_W_LEN ("138"), 3, 138},
  {D_STR_W_LEN ("139"), 3, 139},
  {D_STR_W_LEN ("140"), 3, 140},
  {D_STR_W_LEN ("141"), 3, 141},
  {D_STR_W_LEN ("142"), 3, 142},
  {D_STR_W_LEN ("143"), 3, 143},
  {D_STR_W_LEN ("144"), 3, 144},
  {D_STR_W_LEN ("145"), 3, 145},
  {D_STR_W_LEN ("146"), 3, 146},
  {D_STR_W_LEN ("147"), 3, 147},
  {D_STR_W_LEN ("148"), 3, 148},
  {D_STR_W_LEN ("149"), 3, 149},
  {D_STR_W_LEN ("150"), 3, 150},
  {D_STR_W_LEN ("151"), 3, 151},
  {D_STR_W_LEN ("152"), 3, 152},
  {D_STR_W_LEN ("153"), 3, 153},
  {D_STR_W_LEN ("154"), 3, 154},
  {D_STR_W_LEN ("155"), 3, 155},
  {D_STR_W_LEN ("156"), 3, 156},
  {D_STR_W_LEN ("157"), 3, 157},
  {D_STR_W_LEN ("158"), 3, 158},
  {D_STR_W_LEN ("159"), 3, 159},
  {D_STR_W_LEN ("160"), 3, 160},
  {D_STR_W_LEN ("161"), 3, 161},
  {D_STR_W_LEN ("162"), 3, 162},
  {D_STR_W_LEN ("163"), 3, 163},
  {D_STR_W_LEN ("164"), 3, 164},
  {D_STR_W_LEN ("165"), 3, 165},
  {D_STR_W_LEN ("166"), 3, 166},
  {D_STR_W_LEN ("167"), 3, 167},
  {D_STR_W_LEN ("168"), 3, 168},
  {D_STR_W_LEN ("169"), 3, 169},
  {D_STR_W_LEN ("170"), 3, 170},
  {D_STR_W_LEN ("171"), 3, 171},
  {D_STR_W_LEN ("172"), 3, 172},
  {D_STR_W_LEN ("173"), 3, 173},
  {D_STR_W_LEN ("174"), 3, 174},
  {D_STR_W_LEN ("175"), 3, 175},
  {D_STR_W_LEN ("176"), 3, 176},
  {D_STR_W_LEN ("177"), 3, 177},
  {D_STR_W_LEN ("178"), 3, 178},
  {D_STR_W_LEN ("179"), 3, 179},
  {D_STR_W_LEN ("180"), 3, 180},
  {D_STR_W_LEN ("181"), 3, 181},
  {D_STR_W_LEN ("182"), 3, 182},
  {D_STR_W_LEN ("183"), 3, 183},
  {D_STR_W_LEN ("184"), 3, 184},
  {D_STR_W_LEN ("185"), 3, 185},
  {D_STR_W_LEN ("186"), 3, 186},
  {D_STR_W_LEN ("187"), 3, 187},
  {D_STR_W_LEN ("188"), 3, 188},
  {D_STR_W_LEN ("189"), 3, 189},
  {D_STR_W_LEN ("190"), 3, 190},
  {D_STR_W_LEN ("191"), 3, 191},
  {D_STR_W_LEN ("192"), 3, 192},
  {D_STR_W_LEN ("193"), 3, 193},
  {D_STR_W_LEN ("194"), 3, 194},
  {D_STR_W_LEN ("195"), 3, 195},
  {D_STR_W_LEN ("196"), 3, 196},
  {D_STR_W_LEN ("197"), 3, 197},
  {D_STR_W_LEN ("198"), 3, 198},
  {D_STR_W_LEN ("199"), 3, 199},
  {D_STR_W_LEN ("200"), 3, 200},
  {D_STR_W_LEN ("201"), 3, 201},
  {D_STR_W_LEN ("202"), 3, 202},
  {D_STR_W_LEN ("203"), 3, 203},
  {D_STR_W_LEN ("204"), 3, 204},
  {D_STR_W_LEN ("205"), 3, 205},
  {D_STR_W_LEN ("206"), 3, 206},
  {D_STR_W_LEN ("207"), 3, 207},
  {D_STR_W_LEN ("208"), 3, 208},
  {D_STR_W_LEN ("209"), 3, 209},
  {D_STR_W_LEN ("210"), 3, 210},
  {D_STR_W_LEN ("211"), 3, 211},
  {D_STR_W_LEN ("212"), 3, 212},
  {D_STR_W_LEN ("213"), 3, 213},
  {D_STR_W_LEN ("214"), 3, 214},
  {D_STR_W_LEN ("215"), 3, 215},
  {D_STR_W_LEN ("216"), 3, 216},
  {D_STR_W_LEN ("217"), 3, 217},
  {D_STR_W_LEN ("218"), 3, 218},
  {D_STR_W_LEN ("219"), 3, 219},
  {D_STR_W_LEN ("220"), 3, 220},
  {D_STR_W_LEN ("221"), 3, 221},
  {D_STR_W_LEN ("222"), 3, 222},
  {D_STR_W_LEN ("223"), 3, 223},
  {D_STR_W_LEN ("224"), 3, 224},
  {D_STR_W_LEN ("225"), 3, 225},
  {D_STR_W_LEN ("226"), 3, 226},
  {D_STR_W_LEN ("227"), 3, 227},
  {D_STR_W_LEN ("228"), 3, 228},
  {D_STR_W_LEN ("229"), 3, 229},
  {D_STR_W_LEN ("230"), 3, 230},
  {D_STR_W_LEN ("231"), 3, 231},
  {D_STR_W_LEN ("232"), 3, 232},
  {D_STR_W_LEN ("233"), 3, 233},
  {D_STR_W_LEN ("234"), 3, 234},
  {D_STR_W_LEN ("235"), 3, 235},
  {D_STR_W_LEN ("236"), 3, 236},
  {D_STR_W_LEN ("237"), 3, 237},
  {D_STR_W_LEN ("238"), 3, 238},
  {D_STR_W_LEN ("239"), 3, 239},
  {D_STR_W_LEN ("240"), 3, 240},
  {D_STR_W_LEN ("241"), 3, 241},
  {D_STR_W_LEN ("242"), 3, 242},
  {D_STR_W_LEN ("243"), 3, 243},
  {D_STR_W_LEN ("244"), 3, 244},
  {D_STR_W_LEN ("245"), 3, 245},
  {D_STR_W_LEN ("246"), 3, 246},
  {D_STR_W_LEN ("247"), 3, 247},
  {D_STR_W_LEN ("248"), 3, 248},
  {D_STR_W_LEN ("249"), 3, 249},
  {D_STR_W_LEN ("250"), 3, 250},
  {D_STR_W_LEN ("251"), 3, 251},
  {D_STR_W_LEN ("252"), 3, 252},
  {D_STR_W_LEN ("253"), 3, 253},
  {D_STR_W_LEN ("254"), 3, 254},
  {D_STR_W_LEN ("255"), 3, 255}
};

static const struct str_with_value duint8_w_values_p3[] = {
  {D_STR_W_LEN ("000"), 3, 0},
  {D_STR_W_LEN ("001"), 3, 1},
  {D_STR_W_LEN ("002"), 3, 2},
  {D_STR_W_LEN ("003"), 3, 3},
  {D_STR_W_LEN ("004"), 3, 4},
  {D_STR_W_LEN ("005"), 3, 5},
  {D_STR_W_LEN ("006"), 3, 6},
  {D_STR_W_LEN ("007"), 3, 7},
  {D_STR_W_LEN ("008"), 3, 8},
  {D_STR_W_LEN ("009"), 3, 9},
  {D_STR_W_LEN ("010"), 3, 10},
  {D_STR_W_LEN ("011"), 3, 11},
  {D_STR_W_LEN ("012"), 3, 12},
  {D_STR_W_LEN ("013"), 3, 13},
  {D_STR_W_LEN ("014"), 3, 14},
  {D_STR_W_LEN ("015"), 3, 15},
  {D_STR_W_LEN ("016"), 3, 16},
  {D_STR_W_LEN ("017"), 3, 17},
  {D_STR_W_LEN ("018"), 3, 18},
  {D_STR_W_LEN ("019"), 3, 19},
  {D_STR_W_LEN ("020"), 3, 20},
  {D_STR_W_LEN ("021"), 3, 21},
  {D_STR_W_LEN ("022"), 3, 22},
  {D_STR_W_LEN ("023"), 3, 23},
  {D_STR_W_LEN ("024"), 3, 24},
  {D_STR_W_LEN ("025"), 3, 25},
  {D_STR_W_LEN ("026"), 3, 26},
  {D_STR_W_LEN ("027"), 3, 27},
  {D_STR_W_LEN ("028"), 3, 28},
  {D_STR_W_LEN ("029"), 3, 29},
  {D_STR_W_LEN ("030"), 3, 30},
  {D_STR_W_LEN ("031"), 3, 31},
  {D_STR_W_LEN ("032"), 3, 32},
  {D_STR_W_LEN ("033"), 3, 33},
  {D_STR_W_LEN ("034"), 3, 34},
  {D_STR_W_LEN ("035"), 3, 35},
  {D_STR_W_LEN ("036"), 3, 36},
  {D_STR_W_LEN ("037"), 3, 37},
  {D_STR_W_LEN ("038"), 3, 38},
  {D_STR_W_LEN ("039"), 3, 39},
  {D_STR_W_LEN ("040"), 3, 40},
  {D_STR_W_LEN ("041"), 3, 41},
  {D_STR_W_LEN ("042"), 3, 42},
  {D_STR_W_LEN ("043"), 3, 43},
  {D_STR_W_LEN ("044"), 3, 44},
  {D_STR_W_LEN ("045"), 3, 45},
  {D_STR_W_LEN ("046"), 3, 46},
  {D_STR_W_LEN ("047"), 3, 47},
  {D_STR_W_LEN ("048"), 3, 48},
  {D_STR_W_LEN ("049"), 3, 49},
  {D_STR_W_LEN ("050"), 3, 50},
  {D_STR_W_LEN ("051"), 3, 51},
  {D_STR_W_LEN ("052"), 3, 52},
  {D_STR_W_LEN ("053"), 3, 53},
  {D_STR_W_LEN ("054"), 3, 54},
  {D_STR_W_LEN ("055"), 3, 55},
  {D_STR_W_LEN ("056"), 3, 56},
  {D_STR_W_LEN ("057"), 3, 57},
  {D_STR_W_LEN ("058"), 3, 58},
  {D_STR_W_LEN ("059"), 3, 59},
  {D_STR_W_LEN ("060"), 3, 60},
  {D_STR_W_LEN ("061"), 3, 61},
  {D_STR_W_LEN ("062"), 3, 62},
  {D_STR_W_LEN ("063"), 3, 63},
  {D_STR_W_LEN ("064"), 3, 64},
  {D_STR_W_LEN ("065"), 3, 65},
  {D_STR_W_LEN ("066"), 3, 66},
  {D_STR_W_LEN ("067"), 3, 67},
  {D_STR_W_LEN ("068"), 3, 68},
  {D_STR_W_LEN ("069"), 3, 69},
  {D_STR_W_LEN ("070"), 3, 70},
  {D_STR_W_LEN ("071"), 3, 71},
  {D_STR_W_LEN ("072"), 3, 72},
  {D_STR_W_LEN ("073"), 3, 73},
  {D_STR_W_LEN ("074"), 3, 74},
  {D_STR_W_LEN ("075"), 3, 75},
  {D_STR_W_LEN ("076"), 3, 76},
  {D_STR_W_LEN ("077"), 3, 77},
  {D_STR_W_LEN ("078"), 3, 78},
  {D_STR_W_LEN ("079"), 3, 79},
  {D_STR_W_LEN ("080"), 3, 80},
  {D_STR_W_LEN ("081"), 3, 81},
  {D_STR_W_LEN ("082"), 3, 82},
  {D_STR_W_LEN ("083"), 3, 83},
  {D_STR_W_LEN ("084"), 3, 84},
  {D_STR_W_LEN ("085"), 3, 85},
  {D_STR_W_LEN ("086"), 3, 86},
  {D_STR_W_LEN ("087"), 3, 87},
  {D_STR_W_LEN ("088"), 3, 88},
  {D_STR_W_LEN ("089"), 3, 89},
  {D_STR_W_LEN ("090"), 3, 90},
  {D_STR_W_LEN ("091"), 3, 91},
  {D_STR_W_LEN ("092"), 3, 92},
  {D_STR_W_LEN ("093"), 3, 93},
  {D_STR_W_LEN ("094"), 3, 94},
  {D_STR_W_LEN ("095"), 3, 95},
  {D_STR_W_LEN ("096"), 3, 96},
  {D_STR_W_LEN ("097"), 3, 97},
  {D_STR_W_LEN ("098"), 3, 98},
  {D_STR_W_LEN ("099"), 3, 99},
  {D_STR_W_LEN ("100"), 3, 100},
  {D_STR_W_LEN ("101"), 3, 101},
  {D_STR_W_LEN ("102"), 3, 102},
  {D_STR_W_LEN ("103"), 3, 103},
  {D_STR_W_LEN ("104"), 3, 104},
  {D_STR_W_LEN ("105"), 3, 105},
  {D_STR_W_LEN ("106"), 3, 106},
  {D_STR_W_LEN ("107"), 3, 107},
  {D_STR_W_LEN ("108"), 3, 108},
  {D_STR_W_LEN ("109"), 3, 109},
  {D_STR_W_LEN ("110"), 3, 110},
  {D_STR_W_LEN ("111"), 3, 111},
  {D_STR_W_LEN ("112"), 3, 112},
  {D_STR_W_LEN ("113"), 3, 113},
  {D_STR_W_LEN ("114"), 3, 114},
  {D_STR_W_LEN ("115"), 3, 115},
  {D_STR_W_LEN ("116"), 3, 116},
  {D_STR_W_LEN ("117"), 3, 117},
  {D_STR_W_LEN ("118"), 3, 118},
  {D_STR_W_LEN ("119"), 3, 119},
  {D_STR_W_LEN ("120"), 3, 120},
  {D_STR_W_LEN ("121"), 3, 121},
  {D_STR_W_LEN ("122"), 3, 122},
  {D_STR_W_LEN ("123"), 3, 123},
  {D_STR_W_LEN ("124"), 3, 124},
  {D_STR_W_LEN ("125"), 3, 125},
  {D_STR_W_LEN ("126"), 3, 126},
  {D_STR_W_LEN ("127"), 3, 127},
  {D_STR_W_LEN ("128"), 3, 128},
  {D_STR_W_LEN ("129"), 3, 129},
  {D_STR_W_LEN ("130"), 3, 130},
  {D_STR_W_LEN ("131"), 3, 131},
  {D_STR_W_LEN ("132"), 3, 132},
  {D_STR_W_LEN ("133"), 3, 133},
  {D_STR_W_LEN ("134"), 3, 134},
  {D_STR_W_LEN ("135"), 3, 135},
  {D_STR_W_LEN ("136"), 3, 136},
  {D_STR_W_LEN ("137"), 3, 137},
  {D_STR_W_LEN ("138"), 3, 138},
  {D_STR_W_LEN ("139"), 3, 139},
  {D_STR_W_LEN ("140"), 3, 140},
  {D_STR_W_LEN ("141"), 3, 141},
  {D_STR_W_LEN ("142"), 3, 142},
  {D_STR_W_LEN ("143"), 3, 143},
  {D_STR_W_LEN ("144"), 3, 144},
  {D_STR_W_LEN ("145"), 3, 145},
  {D_STR_W_LEN ("146"), 3, 146},
  {D_STR_W_LEN ("147"), 3, 147},
  {D_STR_W_LEN ("148"), 3, 148},
  {D_STR_W_LEN ("149"), 3, 149},
  {D_STR_W_LEN ("150"), 3, 150},
  {D_STR_W_LEN ("151"), 3, 151},
  {D_STR_W_LEN ("152"), 3, 152},
  {D_STR_W_LEN ("153"), 3, 153},
  {D_STR_W_LEN ("154"), 3, 154},
  {D_STR_W_LEN ("155"), 3, 155},
  {D_STR_W_LEN ("156"), 3, 156},
  {D_STR_W_LEN ("157"), 3, 157},
  {D_STR_W_LEN ("158"), 3, 158},
  {D_STR_W_LEN ("159"), 3, 159},
  {D_STR_W_LEN ("160"), 3, 160},
  {D_STR_W_LEN ("161"), 3, 161},
  {D_STR_W_LEN ("162"), 3, 162},
  {D_STR_W_LEN ("163"), 3, 163},
  {D_STR_W_LEN ("164"), 3, 164},
  {D_STR_W_LEN ("165"), 3, 165},
  {D_STR_W_LEN ("166"), 3, 166},
  {D_STR_W_LEN ("167"), 3, 167},
  {D_STR_W_LEN ("168"), 3, 168},
  {D_STR_W_LEN ("169"), 3, 169},
  {D_STR_W_LEN ("170"), 3, 170},
  {D_STR_W_LEN ("171"), 3, 171},
  {D_STR_W_LEN ("172"), 3, 172},
  {D_STR_W_LEN ("173"), 3, 173},
  {D_STR_W_LEN ("174"), 3, 174},
  {D_STR_W_LEN ("175"), 3, 175},
  {D_STR_W_LEN ("176"), 3, 176},
  {D_STR_W_LEN ("177"), 3, 177},
  {D_STR_W_LEN ("178"), 3, 178},
  {D_STR_W_LEN ("179"), 3, 179},
  {D_STR_W_LEN ("180"), 3, 180},
  {D_STR_W_LEN ("181"), 3, 181},
  {D_STR_W_LEN ("182"), 3, 182},
  {D_STR_W_LEN ("183"), 3, 183},
  {D_STR_W_LEN ("184"), 3, 184},
  {D_STR_W_LEN ("185"), 3, 185},
  {D_STR_W_LEN ("186"), 3, 186},
  {D_STR_W_LEN ("187"), 3, 187},
  {D_STR_W_LEN ("188"), 3, 188},
  {D_STR_W_LEN ("189"), 3, 189},
  {D_STR_W_LEN ("190"), 3, 190},
  {D_STR_W_LEN ("191"), 3, 191},
  {D_STR_W_LEN ("192"), 3, 192},
  {D_STR_W_LEN ("193"), 3, 193},
  {D_STR_W_LEN ("194"), 3, 194},
  {D_STR_W_LEN ("195"), 3, 195},
  {D_STR_W_LEN ("196"), 3, 196},
  {D_STR_W_LEN ("197"), 3, 197},
  {D_STR_W_LEN ("198"), 3, 198},
  {D_STR_W_LEN ("199"), 3, 199},
  {D_STR_W_LEN ("200"), 3, 200},
  {D_STR_W_LEN ("201"), 3, 201},
  {D_STR_W_LEN ("202"), 3, 202},
  {D_STR_W_LEN ("203"), 3, 203},
  {D_STR_W_LEN ("204"), 3, 204},
  {D_STR_W_LEN ("205"), 3, 205},
  {D_STR_W_LEN ("206"), 3, 206},
  {D_STR_W_LEN ("207"), 3, 207},
  {D_STR_W_LEN ("208"), 3, 208},
  {D_STR_W_LEN ("209"), 3, 209},
  {D_STR_W_LEN ("210"), 3, 210},
  {D_STR_W_LEN ("211"), 3, 211},
  {D_STR_W_LEN ("212"), 3, 212},
  {D_STR_W_LEN ("213"), 3, 213},
  {D_STR_W_LEN ("214"), 3, 214},
  {D_STR_W_LEN ("215"), 3, 215},
  {D_STR_W_LEN ("216"), 3, 216},
  {D_STR_W_LEN ("217"), 3, 217},
  {D_STR_W_LEN ("218"), 3, 218},
  {D_STR_W_LEN ("219"), 3, 219},
  {D_STR_W_LEN ("220"), 3, 220},
  {D_STR_W_LEN ("221"), 3, 221},
  {D_STR_W_LEN ("222"), 3, 222},
  {D_STR_W_LEN ("223"), 3, 223},
  {D_STR_W_LEN ("224"), 3, 224},
  {D_STR_W_LEN ("225"), 3, 225},
  {D_STR_W_LEN ("226"), 3, 226},
  {D_STR_W_LEN ("227"), 3, 227},
  {D_STR_W_LEN ("228"), 3, 228},
  {D_STR_W_LEN ("229"), 3, 229},
  {D_STR_W_LEN ("230"), 3, 230},
  {D_STR_W_LEN ("231"), 3, 231},
  {D_STR_W_LEN ("232"), 3, 232},
  {D_STR_W_LEN ("233"), 3, 233},
  {D_STR_W_LEN ("234"), 3, 234},
  {D_STR_W_LEN ("235"), 3, 235},
  {D_STR_W_LEN ("236"), 3, 236},
  {D_STR_W_LEN ("237"), 3, 237},
  {D_STR_W_LEN ("238"), 3, 238},
  {D_STR_W_LEN ("239"), 3, 239},
  {D_STR_W_LEN ("240"), 3, 240},
  {D_STR_W_LEN ("241"), 3, 241},
  {D_STR_W_LEN ("242"), 3, 242},
  {D_STR_W_LEN ("243"), 3, 243},
  {D_STR_W_LEN ("244"), 3, 244},
  {D_STR_W_LEN ("245"), 3, 245},
  {D_STR_W_LEN ("246"), 3, 246},
  {D_STR_W_LEN ("247"), 3, 247},
  {D_STR_W_LEN ("248"), 3, 248},
  {D_STR_W_LEN ("249"), 3, 249},
  {D_STR_W_LEN ("250"), 3, 250},
  {D_STR_W_LEN ("251"), 3, 251},
  {D_STR_W_LEN ("252"), 3, 252},
  {D_STR_W_LEN ("253"), 3, 253},
  {D_STR_W_LEN ("254"), 3, 254},
  {D_STR_W_LEN ("255"), 3, 255}
};


static const struct str_with_value *duint8_w_values_p[3] =
{duint8_w_values_p1, duint8_w_values_p2, duint8_w_values_p3};

static size_t
check_str_from_uint8_pad (void)
{
  int i;
  uint8_t pad;
  size_t t_failed = 0;

  if ((256 != sizeof(duint8_w_values_p1) / sizeof(duint8_w_values_p1[0])) ||
      (256 != sizeof(duint8_w_values_p2) / sizeof(duint8_w_values_p2[0])) ||
      (256 != sizeof(duint8_w_values_p3) / sizeof(duint8_w_values_p3[0])))
  {
    fprintf (stderr,
             "ERROR: wrong number of items in duint8_w_values_p*.\n");
    exit (99);
  }
  for (pad = 0; pad <= 3; pad++)
  {
    size_t table_num;
    if (0 != pad)
      table_num = pad - 1;
    else
      table_num = 0;

    for (i = 0; i <= 255; i++)
    {
      const struct str_with_value *const t = duint8_w_values_p[table_num] + i;
      size_t b_size;
      size_t rs;
      char buf[8];

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      if (sizeof(buf) < t->str.len + 1)
      {
        fprintf (stderr,
                 "ERROR: dstrs_w_values[%u] has too long (%u) string, "
                 "size of 'buf' should be increased.\n",
                 (unsigned int) i, (unsigned int) t->str.len);
        exit (99);
      }
      for (b_size = 0; b_size <= t->str.len + 1; ++b_size)
      {
        /* fill buffer with pseudo-random values */
        memset (buf, '#', sizeof(buf));

        rs = mhd_uint8_to_str_pad ((uint8_t) t->val, pad, buf, b_size);

        if (t->num_of_digt > b_size)
        {
          /* Must fail, buffer is too small for result */
          if (0 != rs)
          {
            t_failed++;
            fprintf (stderr,
                     "FAILED: mhd_uint8_to_str_pad(%" PRIuFAST64 ", %d, -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting 0.\n", t->val, (int) pad, (int) b_size,
                     (uintptr_t) rs);
          }
          else if (0 != memcmp (buf + b_size,
                                "##########",
                                sizeof(buf) - b_size))
          {
            t_failed++;
            fprintf (stderr,
                     "FAILED: mhd_uint8_to_str_pad(%" PRIuFAST64 ", %d,"
                     " -> \"%.*s\", %d) returned %" PRIuPTR
                     " and touched memory outside provided buffer.\n"
                     "The tail of the buffer must be \"%.*s\", "
                     "but it is \"%.*s\".\n",
                     t->val, (int) pad, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs,
                     (int) (sizeof(buf) - b_size),
                     "##########",
                     (int) (sizeof(buf) - b_size),
                     buf + b_size);
          }
        }
        else
        {
          if (t->num_of_digt != rs)
          {
            t_failed++;
            fprintf (stderr,
                     "FAILED: mhd_uint8_to_str_pad(%" PRIuFAST64 ", %d, -> buf,"
                     " %d) returned %" PRIuPTR
                     ", while expecting %d.\n", t->val, (int) pad,
                     (int) b_size, (uintptr_t) rs, (int) t->num_of_digt);
          }
          else if (0 != memcmp (buf, t->str.str, t->num_of_digt))
          {
            t_failed++;
            fprintf (stderr,
                     "FAILED: mhd_uint8_to_str_pad(%" PRIuFAST64 ", %d, "
                     "-> \"%.*s\", %d) returned %" PRIuPTR ".\n",
                     t->val, (int) pad, (int) rs, buf,
                     (int) b_size, (uintptr_t) rs);
          }
          else if (0 != memcmp (buf + rs, "########", sizeof(buf) - rs))
          {
            t_failed++;
            fprintf (stderr,
                     "FAILED: mhd_uint8_to_str_pad(%" PRIuFAST64 ", %d,"
                     " -> \"%.*s\", %d) returned %" PRIuPTR
                     " and touched data after the resulting string.\n",
                     t->val, (int) pad, (int) rs, buf, (int) b_size,
                     (uintptr_t) rs);
          }
        }
      }
    }
  }
  if ((verbose > 1) && (0 == t_failed))
    printf ("PASSED: mhd_uint8_to_str_pad.\n");

  return t_failed;
}


static int
run_str_from_X_tests (void)
{
  size_t str_from_uint16;
  size_t str_from_uint64;
  size_t strx_from_uint32;
  size_t str_from_uint8_pad;
  size_t failures;

  failures = 0;

  str_from_uint16 = check_str_from_uint16 ();
  if (str_from_uint16 != 0)
  {
    fprintf (stderr,
             "FAILED: testcase check_str_from_uint16() failed.\n\n");
    failures += str_from_uint16;
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_from_uint16() successfully "
            "passed.\n\n");

  str_from_uint64 = check_str_from_uint64 ();
  if (str_from_uint64 != 0)
  {
    fprintf (stderr,
             "FAILED: testcase check_str_from_uint16() failed.\n\n");
    failures += str_from_uint64;
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_from_uint16() successfully "
            "passed.\n\n");
  strx_from_uint32 = check_strx_from_uint32 ();
  if (strx_from_uint32 != 0)
  {
    fprintf (stderr,
             "FAILED: testcase check_strx_from_uint32() failed.\n\n");
    failures += strx_from_uint32;
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_from_uint32() successfully "
            "passed.\n\n");

  str_from_uint8_pad = check_str_from_uint8_pad ();
  if (str_from_uint8_pad != 0)
  {
    fprintf (stderr,
             "FAILED: testcase check_str_from_uint8_pad() failed.\n\n");
    failures += str_from_uint8_pad;
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_from_uint8_pad() successfully "
            "passed.\n\n");

  if (failures)
  {
    if (verbose > 0)
      printf ("At least one test failed.\n");

    return 1;
  }

  if (verbose > 0)
    printf ("All tests passed successfully.\n");

  return 0;
}


int
main (int argc, char *argv[])
{
  if (mhdt_has_param (argc, argv, "-v") ||
      mhdt_has_param (argc, argv, "--verbose") ||
      mhdt_has_param (argc, argv, "--verbose1"))
    MHDT_set_verbosity (MHDT_VERB_LVL_BASIC);
  if (mhdt_has_param (argc, argv, "-vv") ||
      mhdt_has_param (argc, argv, "--verbose2"))
    MHDT_set_verbosity (MHDT_VERB_LVL_VERBOSE);

  return run_str_from_X_tests ();
}
