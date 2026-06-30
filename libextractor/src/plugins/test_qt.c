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
 * @file plugins/test_qt.c
 * @brief testcase for the QuickTime/MP4 plugin
 * @author Christian Grothoff
 */
#include "platform.h"
#include "test_lib.h"


/**
 * Main function for the QuickTime testcase.
 *
 * @param argc number of arguments (ignored)
 * @param argv arguments (ignored)
 * @return 0 on success
 */
int
main (int argc, char *argv[])
{
  struct SolutionData qt_sorenson_sol[] = {
    {
      EXTRACTOR_METATYPE_DURATION,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "5s",
      strlen ("5s") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_IMAGE_DIMENSIONS,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "190x240",
      strlen ("190x240") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_LANGUAGE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "English",
      strlen ("English") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_TITLE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "QuickTime Sample Movie",
      strlen ("QuickTime Sample Movie") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_COPYRIGHT,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "\xa9 Apple Computer, Inc. 2001",
      strlen ("\xa9 Apple Computer, Inc. 2001") + 1,
      0
    },
    { 0, 0, NULL, NULL, 0, -1 }
  };
  struct ProblemSet ps[] = {
    { "testdata/gstreamer_sample_sorenson.mov",
      qt_sorenson_sol },
    { NULL, NULL }
  };
  return ET_main ("qt", ps);
}


/* end of test_qt.c */
