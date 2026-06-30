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


static char tmp_bufs[4][4 * 1024]; /* should be enough for testing */
static size_t buf_idx = 0;

/* print non-printable chars as char codes */
static char *
n_prnt (const char *str)
{
  static char *buf;  /* should be enough for testing */
  static const size_t buf_size = sizeof(tmp_bufs[0]);
  const unsigned char *p = (const unsigned char *) str;
  size_t w_pos = 0;
  if (++buf_idx > 3)
    buf_idx = 0;
  buf = tmp_bufs[buf_idx];

  while (*p && w_pos + 1 < buf_size)
  {
    const unsigned char c = *p;
    if ((c == '\\') || (c == '"') )
    {
      if (w_pos + 2 >= buf_size)
        break;
      buf[w_pos++] = '\\';
      buf[w_pos++] = (char) c;
    }
    else if ((c >= 0x20) && (c <= 0x7E) )
      buf[w_pos++] = (char) c;
    else
    {
      if (w_pos + 4 >= buf_size)
        break;
      if (snprintf (buf + w_pos, buf_size - w_pos, "\\x%02hX", (short unsigned
                                                                int) c) != 4)
        break;
      w_pos += 4;
    }
    p++;
  }
  if (*p)
  {   /* not full string is printed */
      /* enough space for "..." ? */
    if (w_pos + 3 > buf_size)
      w_pos = buf_size - 4;
    buf[w_pos++] = '.';
    buf[w_pos++] = '.';
    buf[w_pos++] = '.';
  }
  buf[w_pos] = 0;
  return buf;
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

/* strings that should overflow uint64_t */
static const struct str_with_len str_ovflw[] = {
  D_STR_W_LEN ("18446744073709551616"),  /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("18446744073709551620"),
  D_STR_W_LEN ("18446744083709551615"),
  D_STR_W_LEN ("19234761020556472143"),
  D_STR_W_LEN ("184467440737095516150"),
  D_STR_W_LEN ("1844674407370955161500"),
  D_STR_W_LEN ("000018446744073709551616"),  /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("20000000000000000000"),
  D_STR_W_LEN ("020000000000000000000"),
  D_STR_W_LEN ("0020000000000000000000"),
  D_STR_W_LEN ("100000000000000000000"),
  D_STR_W_LEN ("434532891232591226417"),
  D_STR_W_LEN ("99999999999999999999"),
  D_STR_W_LEN ("18446744073709551616abcd"),  /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("20000000000000000000 suffix"),
  D_STR_W_LEN ("020000000000000000000x")
};

/* strings that should not be convertible to numeric value */
static const struct str_with_len str_no_num[] = {
  D_STR_W_LEN ("zero"),
  D_STR_W_LEN ("one"),
  D_STR_W_LEN ("\xb9\xb2\xb3"),                                    /* superscript "123" in ISO-8859-1/CP1252 */
  D_STR_W_LEN ("\xc2\xb9\xc2\xb2\xc2\xb3"),                        /* superscript "123" in UTF-8 */
  D_STR_W_LEN ("\xd9\xa1\xd9\xa2\xd9\xa3"),                        /* Arabic-Indic "١٢٣" in UTF-8 */
  D_STR_W_LEN ("\xdb\xb1\xdb\xb2\xdb\xb3"),                        /* Ext Arabic-Indic "۱۲۳" in UTF-8 */
  D_STR_W_LEN ("\xe0\xa5\xa7\xe0\xa5\xa8\xe0\xa5\xa9"),            /* Devanagari "१२३" in UTF-8 */
  D_STR_W_LEN ("\xe4\xb8\x80\xe4\xba\x8c\xe4\xb8\x89"),            /* Chinese "一二三" in UTF-8 */
  D_STR_W_LEN ("\xd2\xbb\xb6\xfe\xc8\xfd"),                        /* Chinese "一二三" in GB2312/CP936 */
  D_STR_W_LEN ("\x1B\x24\x29\x41\x0E\x52\x3B\x36\x7E\x48\x7D\x0F") /* Chinese "一二三" in ISO-2022-CN */
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

/* hex strings that should overflow uint64_t */
static const struct str_with_len strx_ovflw[] = {
  D_STR_W_LEN ("10000000000000000"),            /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("10000000000000001"),
  D_STR_W_LEN ("10000000000000002"),
  D_STR_W_LEN ("1000000000000000A"),
  D_STR_W_LEN ("11000000000000000"),
  D_STR_W_LEN ("010000000000000000"),           /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("000010000000000000000"),        /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("20000000000000000000"),
  D_STR_W_LEN ("020000000000000000000"),
  D_STR_W_LEN ("0020000000000000000000"),
  D_STR_W_LEN ("20000000000000000"),
  D_STR_W_LEN ("A0000000000000000"),
  D_STR_W_LEN ("F0000000000000000"),
  D_STR_W_LEN ("a0000000000000000"),
  D_STR_W_LEN ("11111111111111111"),
  D_STR_W_LEN ("CcCcCCccCCccCCccC"),
  D_STR_W_LEN ("f0000000000000000"),
  D_STR_W_LEN ("100000000000000000000"),
  D_STR_W_LEN ("434532891232591226417"),
  D_STR_W_LEN ("10000000000000000a"),
  D_STR_W_LEN ("10000000000000000E"),
  D_STR_W_LEN ("100000000000000000 and nothing"), /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("100000000000000000xx"),           /* 0x10000000000000000, UINT64_MAX+1 */
  D_STR_W_LEN ("99999999999999999999"),
  D_STR_W_LEN ("18446744073709551616abcd"),
  D_STR_W_LEN ("20000000000000000000 suffix"),
  D_STR_W_LEN ("020000000000000000000x")
};


static size_t
check_str_to_uint64_valid (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(dstrs_w_values)
               / sizeof(dstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      uint_fast64_t rv;
      size_t rs;
      const struct str_with_value *const t = dstrs_w_values + i;

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
      rv = 9435223;     /* some random value */
      rs = mhd_str_to_uint64 (t->str.str, &rv);
      if (rs != t->num_of_digt)
      {
        t_failed++;
        c_failed[i] = ! 0;
        fprintf (stderr,
                 "FAILED: mhd_str_to_uint64(\"%s\", ->%" PRIuFAST64
                 ") returned %"
                 PRIuPTR
                 ", while expecting %d."
                 " Locale: %s\n", n_prnt (t->str.str), rv, (uintptr_t) rs,
                 (int) t->num_of_digt, get_current_locale_str ());
      }
      if (rv != t->val)
      {
        t_failed++;
        c_failed[i] = ! 0;
        fprintf (stderr,
                 "FAILED: mhd_str_to_uint64(\"%s\", ->%" PRIuFAST64
                 ") converted string to value %"
                 PRIuFAST64 ","
                 " while expecting result %" PRIuFAST64 ". Locale: %s\n",
                 n_prnt (t->str.str), rv, rv,
                 t->val, get_current_locale_str ());
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_str_to_uint64(\"%s\", ->%" PRIuFAST64 ") == %" \
                PRIuPTR "\n",
                n_prnt (t->str.str), rv, rs);
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_all_chars (void)
{
  int c_failed[256]; /* from 0 to 255 */
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);
  size_t t_failed = 0;
  size_t j;

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    unsigned int c;
    uint_fast64_t test_val;

    set_test_locale (j);  /* setlocale() can be slow! */
    for (c = 0; c < n_checks; c++)
    {
      static const uint_fast64_t rnd_val = 24941852;
      size_t rs;
      if ((c >= '0') && (c <= '9') )
        continue;     /* skip digits */
      for (test_val = 0; test_val <= rnd_val && ! c_failed[c]; test_val +=
             rnd_val)
      {
        char test_str[] = "0123";
        uint_fast64_t rv = test_val;

        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */
        rs = mhd_str_to_uint64 (test_str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[c] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64(\"%s\", ->%" PRIuFAST64
                   ") returned %" PRIuPTR
                   ", while expecting zero."
                   " Locale: %s\n", n_prnt (test_str), rv, (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[c] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: %" PRIuFAST64 ", after call %" PRIuFAST64
                   "). Locale: %s\n",
                   n_prnt (test_str), test_val, rv, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[c])
      {
        char test_str[] = "0123";
        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */

        printf ("PASSED: mhd_str_to_uint64(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (test_str));
      }
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_overflow (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_ovflw) / sizeof(str_ovflw[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_ovflw + i;
      static const uint_fast64_t rnd_val = 2;
      uint_fast64_t test_val;

      for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
             rnd_val)
      {
        uint_fast64_t rv = test_val;

        rs = mhd_str_to_uint64 (t->str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64(\"%s\", ->%" PRIuFAST64
                   ") returned %" PRIuPTR
                   ", while expecting zero."
                   " Locale: %s\n", n_prnt (t->str), rv, (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: %" PRIuFAST64 ", after call %" PRIuFAST64
                   "). Locale: %s\n",
                   n_prnt (t->str), test_val, rv, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_str_to_uint64(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (t->str));
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_no_val (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_no_num) / sizeof(str_no_num[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_no_num + i;
      static const uint_fast64_t rnd_val = 74218431;
      uint_fast64_t test_val;

      for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
             rnd_val)
      {
        uint_fast64_t rv = test_val;

        rs = mhd_str_to_uint64 (t->str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64(\"%s\", ->%" PRIuFAST64
                   ") returned %" PRIuPTR
                   ", while expecting zero."
                   " Locale: %s\n", n_prnt (t->str), rv, (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: %" PRIuFAST64 ", after call %" PRIuFAST64
                   "). Locale: %s\n",
                   n_prnt (t->str), test_val, rv, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_str_to_uint64(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (t->str));
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_n_valid (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(dstrs_w_values)
               / sizeof(dstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      uint_fast64_t rv = 1235572;     /* some random value */
      size_t rs = 0;
      size_t len;
      const struct str_with_value *const t = dstrs_w_values + i;

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
      for (len = t->num_of_digt; len <= t->str.len + 1 && ! c_failed[i]; len++)
      {
        rs = mhd_str_to_uint64_n (t->str.str, len, &rv);
        if (rs != t->num_of_digt)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR ", ->%"
                   PRIuFAST64 ")"
                   " returned %" PRIuPTR ", while expecting %d. Locale: %s\n",
                   n_prnt (t->str.str), (uintptr_t) len, rv, (uintptr_t) rs,
                   (int) t->num_of_digt, get_current_locale_str ());
        }
        if (rv != t->val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR ", ->%"
                   PRIuFAST64 ")"
                   " converted string to value %" PRIuFAST64
                   ", while expecting result %" PRIuFAST64
                   ". Locale: %s\n", n_prnt (t->str.str), (uintptr_t) len, rv,
                   rv,
                   t->val, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR "..%"
                PRIuPTR ", ->%" PRIuFAST64 ")" " == %" PRIuPTR "\n",
                n_prnt (t->str.str),
                (uintptr_t) t->num_of_digt,
                (uintptr_t) t->str.len + 1, rv, rs);
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_n_all_chars (void)
{
  int c_failed[256]; /* from 0 to 255 */
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);
  size_t t_failed = 0;
  size_t j;

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    unsigned int c;
    uint_fast64_t test_val;

    set_test_locale (j);  /* setlocale() can be slow! */
    for (c = 0; c < n_checks; c++)
    {
      static const uint_fast64_t rnd_val = 98372558;
      size_t rs;
      size_t len;

      if ((c >= '0') && (c <= '9') )
        continue;     /* skip digits */

      for (len = 0; len <= 5; len++)
      {
        for (test_val = 0; test_val <= rnd_val && ! c_failed[c]; test_val +=
               rnd_val)
        {
          char test_str[] = "0123";
          uint_fast64_t rv = test_val;

          test_str[0] = (char) (unsigned char) c;        /* replace first char with non-digit char */
          rs = mhd_str_to_uint64_n (test_str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[c] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR ", ->%"
                     PRIuFAST64 ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (test_str), (uintptr_t) len, rv, (uintptr_t) rs,
                     get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[c] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: %" PRIuFAST64
                     ", after call %" PRIuFAST64 ")."
                     " Locale: %s\n",
                     n_prnt (test_str), (uintptr_t) len, test_val, rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[c])
      {
        char test_str[] = "0123";
        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */

        printf ("PASSED: mhd_str_to_uint64_n(\"%s\", 0..5, &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (test_str));
      }
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_n_overflow (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_ovflw) / sizeof(str_ovflw[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_ovflw + i;
      static const uint_fast64_t rnd_val = 3;
      size_t len;

      for (len = t->len; len <= t->len + 1; len++)
      {
        uint_fast64_t test_val;
        for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
               rnd_val)
        {
          uint_fast64_t rv = test_val;

          rs = mhd_str_to_uint64_n (t->str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR ", ->%"
                     PRIuFAST64 ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (t->str), (uintptr_t) len, rv, (uintptr_t) rs,
                     get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: %" PRIuFAST64
                     ", after call %" PRIuFAST64 ")."
                     " Locale: %s\n", n_prnt (t->str), (uintptr_t) len,
                     test_val, rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR "..%" PRIuPTR
                ", &ret_val) == 0,"
                " value of ret_val is unmodified\n", n_prnt (t->str),
                (uintptr_t) t->len,
                (uintptr_t) t->len + 1);
    }
  }
  return t_failed;
}


static size_t
check_str_to_uint64_n_no_val (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_no_num) / sizeof(str_no_num[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_no_num + i;
      static const uint_fast64_t rnd_val = 43255654342;
      size_t len;

      for (len = 0; len <= t->len + 1; len++)
      {
        uint_fast64_t test_val;
        for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
               rnd_val)
        {
          uint_fast64_t rv = test_val;

          rs = mhd_str_to_uint64_n (t->str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR ", ->%"
                     PRIuFAST64 ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (t->str), (uintptr_t) len, rv, (uintptr_t) rs,
                     get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_str_to_uint64_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: %" PRIuFAST64
                     ", after call %" PRIuFAST64 ")."
                     " Locale: %s\n", n_prnt (t->str), (uintptr_t) len,
                     test_val, rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_str_to_uint64_n(\"%s\", 0..%" PRIuPTR
                ", &ret_val) == 0,"
                " value of ret_val is unmodified\n", n_prnt (t->str),
                (uintptr_t) t->len + 1);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_valid (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(xdstrs_w_values)
               / sizeof(xdstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      uint_fast32_t rv;
      size_t rs;
      const struct str_with_value *const t = xdstrs_w_values + i;

      if (t->val != (uint_fast32_t) t->val)
        continue;     /* number is too high for this function */

      if (c_failed[i])
        continue;     /* skip already failed checks */

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: xdstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      rv = 1458532;     /* some random value */
      rs = mhd_strx_to_uint32 (t->str.str, &rv);
      if (rs != t->num_of_digt)
      {
        t_failed++;
        c_failed[i] = ! 0;
        fprintf (stderr,
                 "FAILED: mhd_strx_to_uint32(\"%s\", ->0x%" PRIXFAST64
                 ") returned %"
                 PRIuPTR " digits, while expecting %d."
                 " Locale: %s\n",
                 n_prnt (t->str.str), (uint_fast64_t) rv,
                 (uintptr_t) rs,
                 (int) t->num_of_digt,
                 get_current_locale_str ());
      }
      if (rv != t->val)
      {
        t_failed++;
        c_failed[i] = ! 0;
        fprintf (stderr,
                 "FAILED: mhd_strx_to_uint32(\"%s\", ->0x%" PRIXFAST64
                 ") converted string to value 0x%"
                 PRIXFAST64 ","
                 " while expecting result 0x%" PRIXFAST64 ". Locale: %s\n",
                 n_prnt (t->str.str), (uint_fast64_t) rv, (uint_fast64_t) rv,
                 t->val, get_current_locale_str ());
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint32(\"%s\", ->0x%" PRIXFAST64 ") == %"
                PRIuPTR "\n",
                n_prnt (t->str.str), (uint_fast64_t) rv, rs);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_all_chars (void)
{
  int c_failed[256]; /* from 0 to 255 */
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);
  size_t t_failed = 0;
  size_t j;

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    unsigned int c;
    uint_fast32_t test_val;

    set_test_locale (j);  /* setlocale() can be slow! */
    for (c = 0; c < n_checks; c++)
    {
      static const uint_fast32_t rnd_val = 234234;
      size_t rs;
      if (( (c >= '0') && (c <= '9') )
          || ( (c >= 'A') && (c <= 'F') )
          || ( (c >= 'a') && (c <= 'f') ))
        continue;     /* skip xdigits */
      for (test_val = 0; test_val <= rnd_val && ! c_failed[c]; test_val +=
             rnd_val)
      {
        char test_str[] = "0123";
        uint_fast32_t rv = test_val;

        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */
        rs = mhd_strx_to_uint32 (test_str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[c] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32(\"%s\", ->0x%" PRIXFAST64
                   ") returned %"
                   PRIuPTR " digits, while expecting zero."
                   " Locale: %s\n", n_prnt (test_str), (uint_fast64_t) rv,
                   (uintptr_t) rs, get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[c] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: 0x%" PRIXFAST64 ", after call 0x%" PRIXFAST64
                   "). Locale: %s\n",
                   n_prnt (test_str), (uint_fast64_t) test_val,
                   (uint_fast64_t) rv,
                   get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[c])
      {
        char test_str[] = "0123";
        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */

        printf ("PASSED: mhd_strx_to_uint32(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (test_str));
      }
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_overflow (void)
{
  size_t t_failed = 0;
  size_t i, j;
  static const size_t n_checks1 = sizeof(strx_ovflw) / sizeof(strx_ovflw[0]);
  int c_failed[(sizeof(strx_ovflw) / sizeof(strx_ovflw[0]))
               + (sizeof(xdstrs_w_values)
                  / sizeof(xdstrs_w_values[0]))];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      static const uint_fast32_t rnd_val = 74218431;
      uint_fast32_t test_val;
      const char *str;
      if (i < n_checks1)
      {
        const struct str_with_len *const t = strx_ovflw + i;
        str = t->str;
      }
      else
      {
        const struct str_with_value *const t = xdstrs_w_values + (i
                                                                  - n_checks1);
        if (t->val == (uint_fast32_t) t->val)
          continue;       /* check only strings that should overflow uint_fast32_t */
        str = t->str.str;
      }


      for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
             rnd_val)
      {
        uint_fast32_t rv = test_val;

        rs = mhd_strx_to_uint32 (str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32(\"%s\", ->0x%" PRIXFAST64
                   ") returned %"
                   PRIuPTR ", while expecting zero."
                   " Locale: %s\n", n_prnt (str), (uint_fast64_t) rv,
                   (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: 0x%" PRIXFAST64 ", after call 0x%" PRIXFAST64
                   "). Locale: %s\n",
                   n_prnt (str), (uint_fast64_t) test_val, (uint_fast64_t) rv,
                   get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint32(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (str));
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_no_val (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_no_num) / sizeof(str_no_num[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_no_num + i;
      static const uint_fast32_t rnd_val = 74218431;
      uint_fast32_t test_val;

      for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
             rnd_val)
      {
        uint_fast32_t rv = test_val;

        rs = mhd_strx_to_uint32 (t->str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32(\"%s\", ->0x%" PRIXFAST64
                   ") returned %"
                   PRIuPTR ", while expecting zero."
                   " Locale: %s\n", n_prnt (t->str), (uint_fast64_t) rv,
                   (uintptr_t) rs, get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: 0x%" PRIXFAST64 ", after call 0x%" PRIXFAST64
                   "). Locale: %s\n",
                   n_prnt (t->str), (uint_fast64_t) test_val,
                   (uint_fast64_t) rv,
                   get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint32(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (t->str));
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_n_valid (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(xdstrs_w_values)
               / sizeof(xdstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      uint_fast32_t rv = 2352932;      /* some random value */
      size_t rs = 0;
      size_t len;
      const struct str_with_value *const t = xdstrs_w_values + i;

      if (t->val != (uint_fast32_t) t->val)
        continue;     /* number is too high for this function */

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: xdstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      for (len = t->num_of_digt; len <= t->str.len + 1 && ! c_failed[i]; len++)
      {
        rs = mhd_strx_to_uint32_n (t->str.str, len, &rv);
        if (rs != t->num_of_digt)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR ", ->0x%"
                   PRIXFAST64 ")"
                   " returned %" PRIuPTR ", while expecting %d. Locale: %s\n",
                   n_prnt (t->str.str), (uintptr_t) len, (uint_fast64_t) rv,
                   (uintptr_t) rs,
                   (int) t->num_of_digt, get_current_locale_str ());
        }
        if (rv != t->val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR ", ->0x%"
                   PRIXFAST64 ")"
                   " converted string to value 0x%" PRIXFAST64
                   ", while expecting result 0x%" PRIXFAST64
                   ". Locale: %s\n", n_prnt (t->str.str), (uintptr_t) len,
                   (uint_fast64_t) rv, (uint_fast64_t) rv,
                   t->val, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf (
          "PASSED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR "..%" PRIuPTR
          ", ->0x%"
          PRIXFAST64 ")"
          " == %" PRIuPTR "\n", n_prnt (t->str.str),
          (uintptr_t) t->num_of_digt,
          (uintptr_t) t->str.len + 1, (uint_fast64_t) rv, rs);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_n_all_chars (void)
{
  int c_failed[256]; /* from 0 to 255 */
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);
  size_t t_failed = 0;
  size_t j;

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    unsigned int c;
    uint_fast32_t test_val;

    set_test_locale (j);  /* setlocale() can be slow! */
    for (c = 0; c < n_checks; c++)
    {
      static const uint_fast32_t rnd_val = 98372558;
      size_t rs;
      size_t len;

      if (( (c >= '0') && (c <= '9') )
          || ( (c >= 'A') && (c <= 'F') )
          || ( (c >= 'a') && (c <= 'f') ))
        continue;     /* skip xdigits */

      for (len = 0; len <= 5; len++)
      {
        for (test_val = 0; test_val <= rnd_val && ! c_failed[c]; test_val +=
               rnd_val)
        {
          char test_str[] = "0123";
          uint_fast32_t rv = test_val;

          test_str[0] = (char) (unsigned char) c;        /* replace first char with non-digit char */
          rs = mhd_strx_to_uint32_n (test_str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[c] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR ", ->0x%"
                     PRIXFAST64
                     ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (test_str), (uintptr_t) len, (uint_fast64_t) rv,
                     (uintptr_t) rs, get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[c] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: 0x%" PRIXFAST64
                     ", after call 0x%" PRIXFAST64 ")."
                     " Locale: %s\n",
                     n_prnt (test_str), (uintptr_t) len,
                     (uint_fast64_t) test_val,
                     (uint_fast64_t) rv, get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[c])
      {
        char test_str[] = "0123";
        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */

        printf ("PASSED: mhd_strx_to_uint32_n(\"%s\", 0..5, &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (test_str));
      }
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_n_overflow (void)
{
  size_t t_failed = 0;
  size_t i, j;
  static const size_t n_checks1 = sizeof(strx_ovflw) / sizeof(strx_ovflw[0]);
  int c_failed[(sizeof(strx_ovflw) / sizeof(strx_ovflw[0]))
               + (sizeof(xdstrs_w_values)
                  / sizeof(xdstrs_w_values[0]))];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      static const uint_fast32_t rnd_val = 4;
      size_t len;
      const char *str;
      size_t min_len, max_len;
      if (i < n_checks1)
      {
        const struct str_with_len *const t = strx_ovflw + i;
        str = t->str;
        min_len = t->len;
        max_len = t->len + 1;
      }
      else
      {
        const struct str_with_value *const t = xdstrs_w_values + (i
                                                                  - n_checks1);
        if (t->val == (uint_fast32_t) t->val)
          continue;       /* check only strings that should overflow uint_fast32_t */

        if (t->str.len < t->num_of_digt)
        {
          fprintf (stderr,
                   "ERROR: xdstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                   " to be less or equal to str.len (%u).\n",
                   (unsigned int) (i - n_checks1), (unsigned
                                                    int) t->num_of_digt,
                   (unsigned int) t->str.len);
          exit (99);
        }
        str = t->str.str;
        min_len = t->num_of_digt;
        max_len = t->str.len + 1;
      }

      for (len = min_len; len <= max_len; len++)
      {
        uint_fast32_t test_val;
        for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
               rnd_val)
        {
          uint_fast32_t rv = test_val;

          rs = mhd_strx_to_uint32_n (str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR ", ->0x%"
                     PRIXFAST64
                     ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (str), (uintptr_t) len, (uint_fast64_t) rv,
                     (uintptr_t) rs, get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: 0x%" PRIXFAST64
                     ", after call 0x%" PRIXFAST64 ")."
                     " Locale: %s\n", n_prnt (str), (uintptr_t) len,
                     (uint_fast64_t) test_val, (uint_fast64_t) rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR "..%" PRIuPTR
                ", &ret_val) == 0,"
                " value of ret_val is unmodified\n", n_prnt (str),
                (uintptr_t) min_len,
                (uintptr_t) max_len);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint32_n_no_val (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_no_num) / sizeof(str_no_num[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_no_num + i;
      static const uint_fast32_t rnd_val = 3214314212UL;
      size_t len;

      for (len = 0; len <= t->len + 1; len++)
      {
        uint_fast32_t test_val;
        for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
               rnd_val)
        {
          uint_fast32_t rv = test_val;

          rs = mhd_strx_to_uint32_n (t->str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR ", ->0x%"
                     PRIXFAST64
                     ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (t->str), (uintptr_t) len, (uint_fast64_t) rv,
                     (uintptr_t) rs, get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint32_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: 0x%" PRIXFAST64
                     ", after call 0x%" PRIXFAST64 ")."
                     " Locale: %s\n", n_prnt (t->str), (uintptr_t) len,
                     (uint_fast64_t) test_val, (uint_fast64_t) rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint32_n(\"%s\", 0..%" PRIuPTR
                ", &ret_val) == 0,"
                " value of ret_val is unmodified\n", n_prnt (t->str),
                (uintptr_t) t->len + 1);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_valid (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(xdstrs_w_values)
               / sizeof(xdstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      uint_fast64_t rv;
      size_t rs;
      const struct str_with_value *const t = xdstrs_w_values + i;

      if (c_failed[i])
        continue;     /* skip already failed checks */

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: xdstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      rv = 1458532;     /* some random value */
      rs = mhd_strx_to_uint64 (t->str.str, &rv);
      if (rs != t->num_of_digt)
      {
        t_failed++;
        c_failed[i] = ! 0;
        fprintf (stderr,
                 "FAILED: mhd_strx_to_uint64(\"%s\", ->0x%" PRIXFAST64
                 ") returned %"
                 PRIuPTR ", while expecting %d."
                 " Locale: %s\n", n_prnt (t->str.str), rv, (uintptr_t) rs,
                 (int) t->num_of_digt, get_current_locale_str ());
      }
      if (rv != t->val)
      {
        t_failed++;
        c_failed[i] = ! 0;
        fprintf (stderr,
                 "FAILED: mhd_strx_to_uint64(\"%s\", ->0x%" PRIXFAST64
                 ") converted string to value 0x%"
                 PRIXFAST64 ","
                 " while expecting result 0x%" PRIXFAST64 ". Locale: %s\n",
                 n_prnt (t->str.str), rv, rv,
                 t->val, get_current_locale_str ());
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint64(\"%s\", ->0x%" PRIXFAST64 ") == %"
                PRIuPTR "\n",
                n_prnt (t->str.str), rv, rs);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_all_chars (void)
{
  int c_failed[256]; /* from 0 to 255 */
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);
  size_t t_failed = 0;
  size_t j;

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    unsigned int c;
    uint_fast64_t test_val;

    set_test_locale (j);  /* setlocale() can be slow! */
    for (c = 0; c < n_checks; c++)
    {
      static const uint_fast64_t rnd_val = 234234;
      size_t rs;
      if (( (c >= '0') && (c <= '9') )
          || ( (c >= 'A') && (c <= 'F') )
          || ( (c >= 'a') && (c <= 'f') ))
        continue;     /* skip xdigits */
      for (test_val = 0; test_val <= rnd_val && ! c_failed[c]; test_val +=
             rnd_val)
      {
        char test_str[] = "0123";
        uint_fast64_t rv = test_val;

        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */
        rs = mhd_strx_to_uint64 (test_str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[c] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64(\"%s\", ->0x%" PRIXFAST64
                   ") returned %"
                   PRIuPTR ", while expecting zero."
                   " Locale: %s\n", n_prnt (test_str), rv, (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[c] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: 0x%" PRIXFAST64 ", after call 0x%" PRIXFAST64
                   "). Locale: %s\n",
                   n_prnt (test_str), test_val, rv, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[c])
      {
        char test_str[] = "0123";
        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */

        printf ("PASSED: mhd_strx_to_uint64(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (test_str));
      }
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_overflow (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(strx_ovflw) / sizeof(strx_ovflw[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = strx_ovflw + i;
      static const uint_fast64_t rnd_val = 74218431;
      uint_fast64_t test_val;

      for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
             rnd_val)
      {
        uint_fast64_t rv = test_val;

        rs = mhd_strx_to_uint64 (t->str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64(\"%s\", ->0x%" PRIXFAST64
                   ") returned %"
                   PRIuPTR ", while expecting zero."
                   " Locale: %s\n", n_prnt (t->str), rv, (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: 0x%" PRIXFAST64 ", after call 0x%" PRIXFAST64
                   "). Locale: %s\n",
                   n_prnt (t->str), test_val, rv, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint64(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (t->str));
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_no_val (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_no_num) / sizeof(str_no_num[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_no_num + i;
      static const uint_fast64_t rnd_val = 74218431;
      uint_fast64_t test_val;

      for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
             rnd_val)
      {
        uint_fast64_t rv = test_val;

        rs = mhd_strx_to_uint64 (t->str, &rv);
        if (rs != 0)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64(\"%s\", ->0x%" PRIXFAST64
                   ") returned %"
                   PRIuPTR ", while expecting zero."
                   " Locale: %s\n", n_prnt (t->str), rv, (uintptr_t) rs,
                   get_current_locale_str ());
        }
        else if (rv != test_val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64(\"%s\", &ret_val) modified value of ret_val"
                   " (before call: 0x%" PRIXFAST64 ", after call 0x%" PRIXFAST64
                   "). Locale: %s\n",
                   n_prnt (t->str), test_val, rv, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint64(\"%s\", &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (t->str));
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_n_valid (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(xdstrs_w_values) / sizeof(xdstrs_w_values[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      uint_fast64_t rv = 2352932;     /* some random value */
      size_t rs = 0;
      size_t len;
      const struct str_with_value *const t = xdstrs_w_values + i;

      if (t->str.len < t->num_of_digt)
      {
        fprintf (stderr,
                 "ERROR: xdstrs_w_values[%u] has wrong num_of_digt (%u): num_of_digt is expected"
                 " to be less or equal to str.len (%u).\n",
                 (unsigned int) i, (unsigned int) t->num_of_digt, (unsigned
                                                                   int) t->str.
                 len);
        exit (99);
      }
      for (len = t->num_of_digt; len <= t->str.len + 1 && ! c_failed[i]; len++)
      {
        rs = mhd_strx_to_uint64_n (t->str.str, len, &rv);
        if (rs != t->num_of_digt)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR ", ->0x%"
                   PRIXFAST64 ")"
                   " returned %" PRIuPTR ", while expecting %d. Locale: %s\n",
                   n_prnt (t->str.str), (uintptr_t) len, rv, (uintptr_t) rs,
                   (int) t->num_of_digt, get_current_locale_str ());
        }
        if (rv != t->val)
        {
          t_failed++;
          c_failed[i] = ! 0;
          fprintf (stderr,
                   "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR ", ->0x%"
                   PRIXFAST64 ")"
                   " converted string to value 0x%" PRIXFAST64
                   ", while expecting result 0x%" PRIXFAST64
                   ". Locale: %s\n", n_prnt (t->str.str), (uintptr_t) len, rv,
                   rv,
                   t->val, get_current_locale_str ());
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR "..%" PRIuPTR
                ", ->0x%"
                PRIXFAST64 ")"
                " == %" PRIuPTR "\n", n_prnt (t->str.str),
                (uintptr_t) t->num_of_digt,
                (uintptr_t) t->str.len + 1, rv, rs);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_n_all_chars (void)
{
  int c_failed[256]; /* from 0 to 255 */
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);
  size_t t_failed = 0;
  size_t j;

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    unsigned int c;
    uint_fast64_t test_val;

    set_test_locale (j);  /* setlocale() can be slow! */
    for (c = 0; c < n_checks; c++)
    {
      static const uint_fast64_t rnd_val = 98372558;
      size_t rs;
      size_t len;

      if (( (c >= '0') && (c <= '9') )
          || ( (c >= 'A') && (c <= 'F') )
          || ( (c >= 'a') && (c <= 'f') ))
        continue;     /* skip xdigits */

      for (len = 0; len <= 5; len++)
      {
        for (test_val = 0; test_val <= rnd_val && ! c_failed[c]; test_val +=
               rnd_val)
        {
          char test_str[] = "0123";
          uint_fast64_t rv = test_val;

          test_str[0] = (char) (unsigned char) c;        /* replace first char with non-digit char */
          rs = mhd_strx_to_uint64_n (test_str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[c] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR ", ->0x%"
                     PRIXFAST64
                     ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (test_str), (uintptr_t) len, rv, (uintptr_t) rs,
                     get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[c] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: 0x%" PRIXFAST64
                     ", after call 0x%" PRIXFAST64 ")."
                     " Locale: %s\n",
                     n_prnt (test_str), (uintptr_t) len, test_val, rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[c])
      {
        char test_str[] = "0123";
        test_str[0] = (char) (unsigned char) c;      /* replace first char with non-digit char */

        printf ("PASSED: mhd_strx_to_uint64_n(\"%s\", 0..5, &ret_val) == 0, "
                "value of ret_val is unmodified\n",
                n_prnt (test_str));
      }
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_n_overflow (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(strx_ovflw) / sizeof(strx_ovflw[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = strx_ovflw + i;
      static const uint_fast64_t rnd_val = 4;
      size_t len;

      for (len = t->len; len <= t->len + 1; len++)
      {
        uint_fast64_t test_val;
        for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
               rnd_val)
        {
          uint_fast64_t rv = test_val;

          rs = mhd_strx_to_uint64_n (t->str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR ", ->0x%"
                     PRIXFAST64
                     ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (t->str), (uintptr_t) len, rv, (uintptr_t) rs,
                     get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: 0x%" PRIXFAST64
                     ", after call 0x%" PRIXFAST64 ")."
                     " Locale: %s\n", n_prnt (t->str), (uintptr_t) len,
                     test_val, rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR "..%" PRIuPTR
                ", &ret_val) == 0,"
                " value of ret_val is unmodified\n", n_prnt (t->str),
                (uintptr_t) t->len,
                (uintptr_t) t->len + 1);
    }
  }
  return t_failed;
}


static size_t
check_strx_to_uint64_n_no_val (void)
{
  size_t t_failed = 0;
  size_t i, j;
  int c_failed[sizeof(str_no_num) / sizeof(str_no_num[0])];
  static const size_t n_checks = sizeof(c_failed) / sizeof(c_failed[0]);

  memset (c_failed, 0, sizeof(c_failed));

  for (j = 0; j < locale_name_count; j++)
  {
    set_test_locale (j);  /* setlocale() can be slow! */
    for (i = 0; i < n_checks; i++)
    {
      size_t rs;
      const struct str_with_len *const t = str_no_num + i;
      static const uint_fast64_t rnd_val = 3214314212UL;
      size_t len;

      for (len = 0; len <= t->len + 1; len++)
      {
        uint_fast64_t test_val;
        for (test_val = 0; test_val <= rnd_val && ! c_failed[i]; test_val +=
               rnd_val)
        {
          uint_fast64_t rv = test_val;

          rs = mhd_strx_to_uint64_n (t->str, len, &rv);
          if (rs != 0)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR ", ->0x%"
                     PRIXFAST64
                     ")"
                     " returned %" PRIuPTR
                     ", while expecting zero. Locale: %s\n",
                     n_prnt (t->str), (uintptr_t) len, rv, (uintptr_t) rs,
                     get_current_locale_str ());
          }
          else if (rv != test_val)
          {
            t_failed++;
            c_failed[i] = ! 0;
            fprintf (stderr,
                     "FAILED: mhd_strx_to_uint64_n(\"%s\", %" PRIuPTR
                     ", &ret_val)"
                     " modified value of ret_val (before call: 0x%" PRIXFAST64
                     ", after call 0x%" PRIXFAST64 ")."
                     " Locale: %s\n", n_prnt (t->str), (uintptr_t) len,
                     test_val, rv,
                     get_current_locale_str ());
          }
        }
      }
      if ((verbose > 1) && (j == locale_name_count - 1) && ! c_failed[i])
        printf ("PASSED: mhd_strx_to_uint64_n(\"%s\", 0..%" PRIuPTR
                ", &ret_val) == 0,"
                " value of ret_val is unmodified\n", n_prnt (t->str),
                (uintptr_t) t->len + 1);
    }
  }
  return t_failed;
}


static int
run_str_to_X_tests (void)
{
  size_t str_to_uint64_fails = 0;
  size_t str_to_uint64_n_fails = 0;
  size_t strx_to_uint32_fails = 0;
  size_t strx_to_uint32_n_fails = 0;
  size_t strx_to_uint64_fails = 0;
  size_t strx_to_uint64_n_fails = 0;
  size_t res;

  res = check_str_to_uint64_valid ();
  if (res != 0)
  {
    str_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_valid() failed.\n\n");
  }
  else if (verbose > 1)
    printf (
      "PASSED: testcase check_str_to_uint64_valid() successfully passed.\n\n");

  res = check_str_to_uint64_all_chars ();
  if (res != 0)
  {
    str_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_all_chars() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_all_chars() "
            "successfully passed.\n\n");

  res = check_str_to_uint64_overflow ();
  if (res != 0)
  {
    str_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_overflow() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_overflow() "
            "successfully passed.\n\n");

  res = check_str_to_uint64_no_val ();
  if (res != 0)
  {
    str_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_no_val() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_no_val() "
            "successfully passed.\n\n");

  if (str_to_uint64_fails)
    fprintf (stderr,
             "FAILED: function mhd_str_to_uint64() failed %lu time%s.\n\n",
             (unsigned long) str_to_uint64_fails,
             str_to_uint64_fails == 1 ? "" : "s");
  else if (verbose > 0)
    printf ("PASSED: function mhd_str_to_uint64() successfully "
            "passed all checks.\n\n");

  res = check_str_to_uint64_n_valid ();
  if (res != 0)
  {
    str_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_n_valid() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_n_valid() "
            "successfully passed.\n\n");

  res = check_str_to_uint64_n_all_chars ();
  if (res != 0)
  {
    str_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_n_all_chars() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_n_all_chars() "
            "successfully passed.\n\n");

  res = check_str_to_uint64_n_overflow ();
  if (res != 0)
  {
    str_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_n_overflow() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_n_overflow() "
            "successfully passed.\n\n");

  res = check_str_to_uint64_n_no_val ();
  if (res != 0)
  {
    str_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_str_to_uint64_n_no_val() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_str_to_uint64_n_no_val() "
            "successfully passed.\n\n");

  if (str_to_uint64_n_fails)
    fprintf (stderr,
             "FAILED: function mhd_str_to_uint64_n() failed %lu time%s.\n\n",
             (unsigned long) str_to_uint64_n_fails,
             str_to_uint64_n_fails == 1 ? "" : "s");
  else if (verbose > 0)
    printf ("PASSED: function mhd_str_to_uint64_n() successfully "
            "passed all checks.\n\n");

  res = check_strx_to_uint32_valid ();
  if (res != 0)
  {
    strx_to_uint32_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_valid() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_valid() "
            "successfully passed.\n\n");

  res = check_strx_to_uint32_all_chars ();
  if (res != 0)
  {
    strx_to_uint32_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_all_chars() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_all_chars() "
            "successfully passed.\n\n");

  res = check_strx_to_uint32_overflow ();
  if (res != 0)
  {
    strx_to_uint32_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_overflow() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_overflow() "
            "successfully passed.\n\n");

  res = check_strx_to_uint32_no_val ();
  if (res != 0)
  {
    strx_to_uint32_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_no_val() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_no_val() "
            "successfully passed.\n\n");

  if (strx_to_uint32_fails)
    fprintf (stderr,
             "FAILED: function mhd_strx_to_uint32() failed %lu time%s.\n\n",
             (unsigned long) strx_to_uint32_fails,
             strx_to_uint32_fails == 1 ? "" : "s");
  else if (verbose > 0)
    printf ("PASSED: function mhd_strx_to_uint32() successfully "
            "passed all checks.\n\n");

  res = check_strx_to_uint32_n_valid ();
  if (res != 0)
  {
    strx_to_uint32_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_n_valid() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_n_valid() "
            "successfully passed.\n\n");

  res = check_strx_to_uint32_n_all_chars ();
  if (res != 0)
  {
    strx_to_uint32_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_n_all_chars() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_n_all_chars() "
            "successfully passed.\n\n");

  res = check_strx_to_uint32_n_overflow ();
  if (res != 0)
  {
    strx_to_uint32_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_n_overflow() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_n_overflow() "
            "successfully passed.\n\n");

  res = check_strx_to_uint32_n_no_val ();
  if (res != 0)
  {
    strx_to_uint32_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint32_n_no_val() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint32_n_no_val() "
            "successfully passed.\n\n");

  if (strx_to_uint32_n_fails)
    fprintf (stderr,
             "FAILED: function mhd_strx_to_uint32_n() failed %lu time%s.\n\n",
             (unsigned long) strx_to_uint32_n_fails,
             strx_to_uint32_n_fails == 1 ? "" : "s");
  else if (verbose > 0)
    printf ("PASSED: function mhd_strx_to_uint32_n() successfully "
            "passed all checks.\n\n");

  res = check_strx_to_uint64_valid ();
  if (res != 0)
  {
    strx_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_valid() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_valid() "
            "successfully passed.\n\n");

  res = check_strx_to_uint64_all_chars ();
  if (res != 0)
  {
    strx_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_all_chars() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_all_chars() "
            "successfully passed.\n\n");

  res = check_strx_to_uint64_overflow ();
  if (res != 0)
  {
    strx_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_overflow() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_overflow() "
            "successfully passed.\n\n");

  res = check_strx_to_uint64_no_val ();
  if (res != 0)
  {
    strx_to_uint64_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_no_val() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_no_val() "
            "successfully passed.\n\n");

  if (strx_to_uint64_fails)
    fprintf (stderr,
             "FAILED: function mhd_strx_to_uint64() failed %lu time%s.\n\n",
             (unsigned long) strx_to_uint64_fails,
             strx_to_uint64_fails == 1 ? "" : "s");
  else if (verbose > 0)
    printf ("PASSED: function mhd_strx_to_uint64() successfully "
            "passed all checks.\n\n");

  res = check_strx_to_uint64_n_valid ();
  if (res != 0)
  {
    strx_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_n_valid() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_n_valid() "
            "successfully passed.\n\n");

  res = check_strx_to_uint64_n_all_chars ();
  if (res != 0)
  {
    strx_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_n_all_chars() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_n_all_chars() "
            "successfully passed.\n\n");

  res = check_strx_to_uint64_n_overflow ();
  if (res != 0)
  {
    strx_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_n_overflow() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_n_overflow() "
            "successfully passed.\n\n");

  res = check_strx_to_uint64_n_no_val ();
  if (res != 0)
  {
    strx_to_uint64_n_fails += res;
    fprintf (stderr,
             "FAILED: testcase check_strx_to_uint64_n_no_val() failed.\n\n");
  }
  else if (verbose > 1)
    printf ("PASSED: testcase check_strx_to_uint64_n_no_val() "
            "successfully passed.\n\n");

  if (strx_to_uint64_n_fails)
    fprintf (stderr,
             "FAILED: function mhd_strx_to_uint64_n() failed %lu time%s.\n\n",
             (unsigned long) strx_to_uint64_n_fails,
             strx_to_uint64_n_fails == 1 ? "" : "s");
  else if (verbose > 0)
    printf ("PASSED: function mhd_strx_to_uint64_n() successfully "
            "passed all checks.\n\n");

  if (str_to_uint64_fails || str_to_uint64_n_fails ||
      strx_to_uint32_fails || strx_to_uint32_n_fails ||
      strx_to_uint64_fails || strx_to_uint64_n_fails)
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

  return run_str_to_X_tests ();
}
