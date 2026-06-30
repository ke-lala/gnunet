/*
     This file is part of libextractor.
     Copyright (C) 2002, 2003, 2009, 2026 Vidyut Samanta and Christian Grothoff

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
 * @file plugins/pdf_extractor.cc
 * @brief plugin to support PDF files
 * @author Vidyut Samanta
 * @author Christian Grothoff
 */
#include "platform.h"
#include "extractor.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-global.h>
#include <string>
#include <vector>


/**
 * Sanity bound on the size of a PDF we are willing to buffer
 * in memory (1 GB).  libpoppler needs the whole document, and
 * its raw-data loader takes an `int` length.
 */
#define MAX_PDF_SIZE (1024LL * 1024LL * 1024LL)


/**
 * Entry in the mapping from poppler accessors to LE types.
 */
struct Matches
{
  /**
   * Accessor on the poppler document returning the value.
   */
  poppler::ustring (poppler::document::*get) () const;

  /**
   * Corresponding meta data type in LE.
   */
  enum EXTRACTOR_MetaType type;
};


/**
 * Map from poppler document info accessors to LE types.
 *
 * Note that we deliberately map "Creator" to
 * #EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE: we believe that
 * Adobe's "Creator" is not a person nor an organisation, but
 * just a piece of software.
 */
static const struct Matches tmap[] = {
  { &poppler::document::get_title,    EXTRACTOR_METATYPE_TITLE },
  { &poppler::document::get_subject,  EXTRACTOR_METATYPE_SUBJECT },
  { &poppler::document::get_keywords, EXTRACTOR_METATYPE_KEYWORDS },
  { &poppler::document::get_author,   EXTRACTOR_METATYPE_AUTHOR_NAME },
  { &poppler::document::get_creator,  EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE },
  { &poppler::document::get_producer, EXTRACTOR_METATYPE_PRODUCED_BY_SOFTWARE },
  { NULL, EXTRACTOR_METATYPE_RESERVED }
};


/**
 * Silence libpoppler: we do not want parsing diagnostics on
 * stderr of the plugin child process.
 *
 * @param msg the message (ignored)
 * @param cls closure (ignored)
 */
static void
quiet_error (const std::string &msg,
             void *cls)
{
  (void) msg;
  (void) cls;
}


/**
 * Hand a UTF-8 string to the meta data processor, after
 * stripping trailing whitespace.  Empty values are skipped.
 *
 * @param ec extraction context
 * @param type meta data type to use
 * @param val UTF-8 bytes (need not be 0-terminated)
 * @return 0 to continue extracting, 1 if @a ec asked us to stop
 */
static int
add_utf8 (struct EXTRACTOR_ExtractContext *ec,
          enum EXTRACTOR_MetaType type,
          std::vector<char> val)
{
  size_t len = val.size ();

  /*
   * Avoid outputting trailing whitespace.  Note that ISO-8859
   * NBSP (0xA0) becomes 0xC2 0xA0 in UTF-8 and is intentionally
   * not stripped here.
   */
  while ((0 < len) &&
         ((' ' == val[len - 1]) ||
          ('\r' == val[len - 1]) ||
          ('\n' == val[len - 1]) ||
          ('\t' == val[len - 1]) ||
          ('\v' == val[len - 1]) ||
          ('\f' == val[len - 1])))
    len--;
  if (0 == len)
    return 0;
  std::string s (val.data (), len);
  if (0 != ec->proc (ec->cls,
                     "pdf",
                     type,
                     EXTRACTOR_METAFORMAT_UTF8,
                     "text/plain",
                     s.c_str (),
                     s.size () + 1))
    return 1;
  return 0;
}


/**
 * Hand a 0-terminated C string to the meta data processor.
 *
 * @param ec extraction context
 * @param type meta data type to use
 * @param s the string
 * @return 0 to continue extracting, 1 if @a ec asked us to stop
 */
static int
add_str (struct EXTRACTOR_ExtractContext *ec,
         enum EXTRACTOR_MetaType type,
         const char *s)
{
  if ((NULL == s) || ('\0' == s[0]))
    return 0;
  if (0 != ec->proc (ec->cls,
                     "pdf",
                     type,
                     EXTRACTOR_METAFORMAT_UTF8,
                     "text/plain",
                     s,
                     strlen (s) + 1))
    return 1;
  return 0;
}


/**
 * Report a date (given as a `time_t`) in ISO-8601 / UTC.
 *
 * @param ec extraction context
 * @param type meta data type to use
 * @param t the time, `(time_t) -1` or 0 if absent
 * @return 0 to continue extracting, 1 if @a ec asked us to stop
 */
static int
add_date (struct EXTRACTOR_ExtractContext *ec,
          enum EXTRACTOR_MetaType type,
          time_t t)
{
  char buf[32];
  struct tm tv;

  if (((time_t) -1 == t) || (0 == t))
    return 0;
  if (NULL == gmtime_r (&t, &tv))
    return 0;
  if (0 == strftime (buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", &tv))
    return 0;
  return add_str (ec, type, buf);
}


/**
 * Read the entire input into @a buf.
 *
 * @param ec extraction context
 * @param[out] buf buffer to fill with the file contents
 * @return 0 on success, -1 on error
 */
static int
read_all (struct EXTRACTOR_ExtractContext *ec,
          std::vector<char> &buf)
{
  uint64_t size;

  size = ec->get_size (ec->cls);
  if ((UINT64_MAX == size) ||
      (0 == size) ||
      (size > MAX_PDF_SIZE))
    return -1;
  if (0 != ec->seek (ec->cls, 0, SEEK_SET))
    return -1;
  buf.reserve ((size_t) size);
  while (buf.size () < size)
  {
    void *data;
    ssize_t got;

    got = ec->read (ec->cls,
                    &data,
                    (size_t) (size - buf.size ()));
    if ((got <= 0) || (NULL == data))
      break;
    buf.insert (buf.end (),
                static_cast<char *> (data),
                static_cast<char *> (data) + got);
  }
  if (buf.empty ())
    return -1;
  return 0;
}


/**
 * Main entry method for the PDF extraction plugin.
 *
 * @param ec extraction context provided to the plugin
 */
extern "C" void
EXTRACTOR_pdf_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  void *hdr;
  std::vector<char> buf;
  poppler::document *doc;
  int major;
  int minor;
  char ver[32];
  char pages[16];

  if (4 != ec->read (ec->cls, &hdr, 4))
    return;
  if (0 != memcmp ("%PDF", hdr, 4))
    return;
  if (0 != read_all (ec, buf))
    return;

  poppler::set_debug_error_function (&quiet_error, NULL);
  doc = poppler::document::load_from_raw_data (buf.data (),
                                               (int) buf.size ());
  if (NULL == doc)
    return;
  /* An encrypted document we cannot open exposes no usable meta data. */
  if (doc->is_locked ())
  {
    delete doc;
    return;
  }

  if (0 != add_str (ec,
                    EXTRACTOR_METATYPE_MIMETYPE,
                    "application/pdf"))
    goto CLEANUP;
  for (unsigned int i = 0; NULL != tmap[i].get; i++)
    if (0 != add_utf8 (ec,
                       tmap[i].type,
                       (doc->*tmap[i].get)().to_utf8 ()))
      goto CLEANUP;
  doc->get_pdf_version (&major, &minor);
  snprintf (ver, sizeof (ver), "PDF %d.%d", major, minor);
  if (0 != add_str (ec, EXTRACTOR_METATYPE_FORMAT, ver))
    goto CLEANUP;
  snprintf (pages, sizeof (pages), "%d", doc->pages ());
  if (0 != add_str (ec, EXTRACTOR_METATYPE_PAGE_COUNT, pages))
    goto CLEANUP;
  if (0 != add_date (ec,
                     EXTRACTOR_METATYPE_CREATION_DATE,
                     doc->get_creation_date_t ()))
    goto CLEANUP;
  if (0 != add_date (ec,
                     EXTRACTOR_METATYPE_MODIFICATION_DATE,
                     doc->get_modification_date_t ()))
    goto CLEANUP;
CLEANUP:
  delete doc;
}


/* end of pdf_extractor.cc */
