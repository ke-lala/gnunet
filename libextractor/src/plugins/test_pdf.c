/*
     This file is part of libextractor.
     Copyright (C) 2026 Vidyut Samanta and Christian Grothoff

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
 * @file plugins/test_pdf.c
 * @brief testcase for pdf plugin
 * @author Christian Grothoff
 */
#include "platform.h"
#include "test_lib.h"


/**
 * Main function for the PDF testcase.
 *
 * @param argc number of arguments (ignored)
 * @param argv arguments (ignored)
 * @return 0 on success
 */
int
main (int argc, char *argv[])
{
  struct SolutionData pdf_extract_sol[] = {
    {
      EXTRACTOR_METATYPE_MIMETYPE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "application/pdf",
      strlen ("application/pdf") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_TITLE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "GNU libextractor PDF Test",
      strlen ("GNU libextractor PDF Test") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_SUBJECT,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "Metadata extraction",
      strlen ("Metadata extraction") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_KEYWORDS,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "PDF, libextractor, test",
      strlen ("PDF, libextractor, test") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_AUTHOR_NAME,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "Vidyut Samanta",
      strlen ("Vidyut Samanta") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "WritePDF",
      strlen ("WritePDF") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_PRODUCED_BY_SOFTWARE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "libextractor testsuite",
      strlen ("libextractor testsuite") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_FORMAT,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "PDF 1.4",
      strlen ("PDF 1.4") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_PAGE_COUNT,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "1",
      strlen ("1") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_CREATION_DATE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "2020-01-15 12:30:00",
      strlen ("2020-01-15 12:30:00") + 1,
      0
    },
    {
      EXTRACTOR_METATYPE_MODIFICATION_DATE,
      EXTRACTOR_METAFORMAT_UTF8,
      "text/plain",
      "2020-01-16 08:00:00",
      strlen ("2020-01-16 08:00:00") + 1,
      0
    },
    { 0, 0, NULL, NULL, 0, -1 }
  };
  struct ProblemSet ps[] = {
    { "testdata/pdf_extract.pdf",
      pdf_extract_sol },
    { NULL, NULL }
  };
  return ET_main ("pdf", ps);
}


/* end of test_pdf.c */
