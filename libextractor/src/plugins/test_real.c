/*
     This file is part of libextractor.
     Copyright (C) 2012 Vidyut Samanta and Christian Grothoff

     libextractor is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     libextractor is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libextractor; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
*/
/**
 * @file plugins/test_real.c
 * @brief testcase for real plugin
 * @author Christian Grothoff
 */
#include "platform.h"
#include "test_lib.h"


/**
 * Main function for the REAL testcase.
 *
 * @param argc number of arguments (ignored)
 * @param argv arguments (ignored)
 * @return 0 on success
 */
int
main (int argc, char *argv[])
{
  struct SolutionData real_audiosig_sol[] = {
    {
      EXTRACTOR_METATYPE_MIMETYPE,
      EXTRACTOR_METAFORMAT_C_STRING,
      "text/plain",
      "audio/x-pn-realaudio",
      strlen ("audio/x-pn-realaudio") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_TITLE,
      EXTRACTOR_METAFORMAT_C_STRING,
      "text/plain",
      "Welcome!",
      strlen ("Welcome!") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_COPYRIGHT,
      EXTRACTOR_METAFORMAT_C_STRING,
      "text/plain",
      "1998, RealNetworks, Inc.",
      strlen ("1998, RealNetworks, Inc.") + 1,
      0
    },
    { 0, 0, NULL, NULL, 0, -1 }
  };
  struct SolutionData real_ra3_sol[] = {
    {
      EXTRACTOR_METATYPE_MIMETYPE,
      EXTRACTOR_METAFORMAT_C_STRING,
      "text/plain",
      "audio/vnd.rn-realaudio",
      strlen ("audio/vnd.rn-realaudio") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_TITLE,
      EXTRACTOR_METAFORMAT_C_STRING,
      "text/plain",
      "Song of Welcome",
      strlen ("Song of Welcome") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_AUTHOR_NAME,
      EXTRACTOR_METAFORMAT_C_STRING,
      "text/plain",
      "Investiture Service",
      strlen ("Investiture Service") + 1,
      0
    },
    { 0, 0, NULL, NULL, 0, -1 }
  };
  struct ProblemSet ps[] = {
    { "testdata/audiosig.rm",
      real_audiosig_sol },
    { "testdata/ra3.ra",
      real_ra3_sol },
    { NULL, NULL }
  };
  return ET_main ("real", ps);
}


/* end of test_real.c */
