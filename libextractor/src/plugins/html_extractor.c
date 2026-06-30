/*
     This file is part of libextractor.
     Copyright (C) 2002, 2003, 2004, 2005, 2009, 2012 Vidyut Samanta and Christian Grothoff

     libextractor is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 2, or (at your
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
 * @file plugins/html_extractor.c
 * @brief plugin to support HTML files
 * @author Christian Grothoff
 */
#include "platform.h"
#include "extractor.h"
#include <magic.h>
#if HAVE_TIDY_H
#include <tidy.h>
#include <tidybuffio.h>
#elif HAVE_TIDY_TIDY_H
#include <tidy/tidy.h>
#include <tidy/tidybuffio.h>
#else
Broken build, fix tidy detection.
#endif

/**
 * Mapping of HTML META names to LE types.
 */
static struct
{
  /**
   * HTML META name.
   */
  const char *name;

  /**
   * Corresponding LE type.
   */
  enum EXTRACTOR_MetaType type;
} tagmap[] = {
  { "author", EXTRACTOR_METATYPE_AUTHOR_NAME },
  { "dc.author", EXTRACTOR_METATYPE_AUTHOR_NAME },
  { "title", EXTRACTOR_METATYPE_TITLE },
  { "dc.title", EXTRACTOR_METATYPE_TITLE},
  { "description", EXTRACTOR_METATYPE_DESCRIPTION },
  { "dc.description", EXTRACTOR_METATYPE_DESCRIPTION },
  { "subject", EXTRACTOR_METATYPE_SUBJECT},
  { "dc.subject", EXTRACTOR_METATYPE_SUBJECT},
  { "date", EXTRACTOR_METATYPE_UNKNOWN_DATE },
  { "dc.date", EXTRACTOR_METATYPE_UNKNOWN_DATE},
  { "publisher", EXTRACTOR_METATYPE_PUBLISHER },
  { "dc.publisher", EXTRACTOR_METATYPE_PUBLISHER},
  { "rights", EXTRACTOR_METATYPE_RIGHTS },
  { "dc.rights", EXTRACTOR_METATYPE_RIGHTS },
  { "copyright", EXTRACTOR_METATYPE_COPYRIGHT },
  { "language", EXTRACTOR_METATYPE_LANGUAGE },
  { "keywords", EXTRACTOR_METATYPE_KEYWORDS },
  { "abstract", EXTRACTOR_METATYPE_ABSTRACT },
  { "formatter", EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE },
  { "dc.creator", EXTRACTOR_METATYPE_CREATOR},
  { "dc.identifier", EXTRACTOR_METATYPE_URI },
  { "dc.format", EXTRACTOR_METATYPE_FORMAT },
  { NULL, EXTRACTOR_METATYPE_RESERVED }
};


/**
 * Global handle to MAGIC data.
 */
static magic_t magic;


/**
 * Map 'meta' tag to LE type.
 *
 * @param tag tag to map
 * @return EXTRACTOR_METATYPE_RESERVED if the type was not found
 */
static enum EXTRACTOR_MetaType
tag_to_type (const char *tag)
{
  unsigned int i;

  for (i = 0; NULL != tagmap[i].name; i++)
    if (0 == strcasecmp (tag,
                         tagmap[i].name))
      return tagmap[i].type;
  return EXTRACTOR_METATYPE_RESERVED;
}


/**
 * Function called by libtidy for error reporting.
 *
 * @param doc tidy doc being processed
 * @param lvl report level
 * @param line input line
 * @param col input column
 * @param mssg message
 * @return FALSE (no output)
 */
static Bool TIDY_CALL
report_cb (TidyDoc doc,
           TidyReportLevel lvl,
           uint line,
           uint col,
           ctmbstr mssg)
{
  return 0;
}


/**
 * Input callback: get next byte of input.
 *
 * @param sourceData our 'struct EXTRACTOR_ExtractContext'
 * @return next byte of input, EndOfStream on errors and EOF
 */
static int TIDY_CALL
get_byte_cb (void *sourceData)
{
  struct EXTRACTOR_ExtractContext *ec = sourceData;
  void *data;

  if (1 !=
      ec->read (ec->cls,
                &data, 1))
    return EndOfStream;
  return *(unsigned char*) data;
}


/**
 * Input callback: unget last byte of input.
 *
 * @param sourceData our 'struct EXTRACTOR_ExtractContext'
 * @param bt byte to unget (ignored)
 */
static void TIDY_CALL
unget_byte_cb (void *sourceData, byte bt)
{
  struct EXTRACTOR_ExtractContext *ec = sourceData;

  (void) ec->seek (ec->cls, -1, SEEK_CUR);
}


/**
 * Input callback: check for EOF.
 *
 * @param sourceData our 'struct EXTRACTOR_ExtractContext'
 * @return true if we are at the EOF
 */
static Bool TIDY_CALL
eof_cb (void *sourceData)
{
  struct EXTRACTOR_ExtractContext *ec = sourceData;

  return ec->seek (ec->cls, 0, SEEK_CUR) == ec->get_size (ec->cls);
}


/**
 * Main entry method for the 'text/html' extraction plugin.
 *
 * @param ec extraction context provided to the plugin
 */
void
EXTRACTOR_html_extract_method (struct EXTRACTOR_ExtractContext *ec);

void
EXTRACTOR_html_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  TidyDoc doc;
  TidyNode head;
  TidyNode child;
  TidyNode title;
  TidyInputSource src;
  const char *name;
  TidyBuffer tbuf;
  TidyAttr attr;
  enum EXTRACTOR_MetaType type;
  ssize_t iret;
  void *data;
  const char *mime;

  if (-1 == (iret = ec->read (ec->cls,
                              &data,
                              16 * 1024)))
    return;
  if (NULL == (mime = magic_buffer (magic, data, iret)))
    return;
  if (0 != strncmp (mime,
                    "text/html",
                    strlen ("text/html")))
    return; /* not HTML */

  if (0 != ec->seek (ec->cls, 0, SEEK_SET))
    return; /* seek failed !? */

  tidyInitSource (&src, ec,
                  &get_byte_cb,
                  &unget_byte_cb,
                  &eof_cb);
  if (NULL == (doc = tidyCreate ()))
    return;
  tidySetReportFilter (doc, &report_cb);
  tidySetAppData (doc, ec);
  if (0 > tidyParseSource (doc, &src))
  {
    tidyRelease (doc);
    return;
  }
  if (1 != tidyStatus (doc))
  {
    tidyRelease (doc);
    return;
  }
  if (NULL == (head = tidyGetHead (doc)))
  {
    fprintf (stderr, "no head\n");
    tidyRelease (doc);
    return;
  }
  for (child = tidyGetChild (head); NULL != child; child = tidyGetNext (child))
  {
    switch (tidyNodeGetType (child))
    {
    case TidyNode_Root:
      break;
    case TidyNode_DocType:
      break;
    case TidyNode_Comment:
      break;
    case TidyNode_ProcIns:
      break;
    case TidyNode_Text:
      break;
    case TidyNode_CDATA:
      break;
    case TidyNode_Section:
      break;
    case TidyNode_Asp:
      break;
    case TidyNode_Jste:
      break;
    case TidyNode_Php:
      break;
    case TidyNode_XmlDecl:
      break;
    case TidyNode_Start:
    case TidyNode_StartEnd:
      name = tidyNodeGetName (child);
      if ( (0 == strcasecmp (name, "title")) &&
           (NULL != (title = tidyGetChild (child))) )
      {
        tidyBufInit (&tbuf);
        tidyNodeGetValue (doc, title, &tbuf);
        /* add 0-termination */
        tidyBufPutByte (&tbuf, 0);
        if (0 !=
            ec->proc (ec->cls,
                      "html",
                      EXTRACTOR_METATYPE_TITLE,
                      EXTRACTOR_METAFORMAT_UTF8,
                      "text/plain",
                      (const char *) tbuf.bp,
                      tbuf.size))
        {
          tidyBufFree (&tbuf);
          goto CLEANUP;
        }
        tidyBufFree (&tbuf);
        break;
      }
      if (0 == strcasecmp (name, "meta"))
      {
        if (NULL == (attr = tidyAttrGetById (child,
                                             TidyAttr_NAME)))
          break;
        if (EXTRACTOR_METATYPE_RESERVED ==
            (type = tag_to_type (tidyAttrValue (attr))))
          break;
        if (NULL == (attr = tidyAttrGetById (child,
                                             TidyAttr_CONTENT)))
          break;
        name = tidyAttrValue (attr);
        if (0 !=
            ec->proc (ec->cls,
                      "html",
                      type,
                      EXTRACTOR_METAFORMAT_UTF8,
                      "text/plain",
                      name,
                      strlen (name) + 1))
          goto CLEANUP;
        break;
      }
      break;
    case TidyNode_End:
      break;
    default:
      break;
    }
  }
CLEANUP:
  tidyRelease (doc);
}


/**
 * Initialize glib and load magic file.
 */
void __attribute__ ((constructor))
html_gobject_init (void);

void __attribute__ ((constructor))
html_gobject_init ()
{
  magic = magic_open (MAGIC_MIME_TYPE);
  if (0 != magic_load (magic, NULL))
  {
    /* FIXME: how to deal with errors? */
  }
}


/**
 * Destructor for the library, cleans up.
 */
void __attribute__ ((destructor))
html_ltdl_fini (void);

void __attribute__ ((destructor))
html_ltdl_fini ()
{
  if (NULL != magic)
  {
    magic_close (magic);
    magic = NULL;
  }
}


/* end of html_extractor.c */
