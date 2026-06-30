/*
     This file is part of libextractor.
     Copyright (C) 2008 Heikki Lindholm
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
 * @file plugins/applefile_extractor.c
 * @brief plugin to support AppleSingle and AppleDouble files (RFC 1740)
 * @author Heikki Lindholm
 * @author Christian Grothoff
 */
#include "platform.h"
#include "extractor.h"
#include "pack.h"


#define APPLESINGLE_SIGNATURE "\x00\x05\x16\x00"
#define APPLEDOUBLE_SIGNATURE "\x00\x05\x16\x07"


typedef struct
{
  unsigned char magic[4];
  unsigned int version;
  char homeFileSystem[16]; /* v1: ASCII  v2: zero-filled */
  unsigned short entries;
} ApplefileHeader;

#define APPLEFILE_HEADER_SIZE 26
#define APPLEFILE_HEADER_SPEC "4bW16bH"
#define APPLEFILE_HEADER_FIELDS(p) \
        & (p)->magic, \
        &(p)->version, \
        &(p)->homeFileSystem, \
        &(p)->entries


typedef struct
{
  unsigned int id;
  unsigned int offset;
  unsigned int length;
} ApplefileEntryDescriptor;

#define APPLEFILE_ENTRY_DESCRIPTOR_SIZE 12
#define APPLEFILE_ENTRY_DESCRIPTOR_SPEC "WWW"
#define APPLEFILE_ENTRY_DESCRIPTOR_FIELDS(p) \
        & (p)->id, \
        &(p)->offset, \
        &(p)->length


/* Entry type IDs (RFC 1740 §5) */
#define AED_ID_DATA_FORK           1
#define AED_ID_RESOURCE_FORK       2
#define AED_ID_REAL_NAME           3
#define AED_ID_COMMENT             4
#define AED_ID_ICON_BW             5
#define AED_ID_ICON_COLOUR         6
#define AED_ID_FILE_DATES_INFO     8
#define AED_ID_FINDER_INFO         9
#define AED_ID_MACINTOSH_FILE_INFO 10
#define AED_ID_PRODOS_FILE_INFO    11
#define AED_ID_MSDOS_FILE_INFO     12
#define AED_ID_SHORT_NAME          13
#define AED_ID_AFP_FILE_INFO       14
#define AED_ID_DIRECTORY_ID        15


/**
 * Emit a UTF-8 string as metadata of type @a t.
 * Returns from the enclosing function if proc signals abort.
 */
#define ADD(s, t) do { \
          if (0 != ec->proc (ec->cls, \
                             "applefile", \
                             (t), \
                             EXTRACTOR_METAFORMAT_UTF8, \
                             "text/plain", \
                             (s), \
                             strlen (s) + 1)) \
          return; \
} while (0)


/**
 * Main entry method for the 'application/applefile' extraction plugin.
 *
 * @param ec extraction context provided to the plugin
 */
void
EXTRACTOR_applefile_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  void *data;
  ssize_t got;
  ApplefileHeader header;
  uint64_t file_size;

  /* read and unpack the 26-byte file header */
  got = ec->read (ec->cls, &data, APPLEFILE_HEADER_SIZE);
  if (got < APPLEFILE_HEADER_SIZE)
    return;
  EXTRACTOR_common_cat_unpack (data,
                               APPLEFILE_HEADER_SPEC,
                               APPLEFILE_HEADER_FIELDS (&header));

  if ((0 != memcmp (header.magic, APPLESINGLE_SIGNATURE, 4)) &&
      (0 != memcmp (header.magic, APPLEDOUBLE_SIGNATURE, 4)))
    return;

  if (0 != ec->proc (ec->cls,
                     "applefile",
                     EXTRACTOR_METATYPE_MIMETYPE,
                     EXTRACTOR_METAFORMAT_UTF8,
                     "text/plain",
                     "application/applefile",
                     strlen ("application/applefile") + 1))
    return;

  if ((header.version != 0x00010000) && (header.version != 0x00020000))
    return;

  file_size = ec->get_size (ec->cls);

  for (unsigned int i = 0; i < header.entries; i++)
  {
    ApplefileEntryDescriptor dsc;
    uint64_t desc_pos = (uint64_t) APPLEFILE_HEADER_SIZE
                        + (uint64_t) i * APPLEFILE_ENTRY_DESCRIPTOR_SIZE;

    if ((int64_t) desc_pos !=
        ec->seek (ec->cls, (int64_t) desc_pos, SEEK_SET))
      return;
    got = ec->read (ec->cls, &data, APPLEFILE_ENTRY_DESCRIPTOR_SIZE);
    if (got < APPLEFILE_ENTRY_DESCRIPTOR_SIZE)
      return;
    EXTRACTOR_common_cat_unpack (data,
                                 APPLEFILE_ENTRY_DESCRIPTOR_SPEC,
                                 APPLEFILE_ENTRY_DESCRIPTOR_FIELDS (&dsc));

    switch (dsc.id)
    {
    case AED_ID_DATA_FORK:
      {
        /* Report the data-fork size; no seek needed, length is in the
           descriptor itself. */
        char s[14];

        if (dsc.length >= 1000000000)
          snprintf (s, 13, "%.2f %s", dsc.length / 1000000000.0, _ ("GB"));
        else if (dsc.length >= 1000000)
          snprintf (s, 13, "%.2f %s", dsc.length / 1000000.0, _ ("MB"));
        else if (dsc.length >= 1000)
          snprintf (s, 13, "%.2f %s", dsc.length / 1000.0, _ ("KB"));
        else
          snprintf (s, 13, "%.2f %s", (double) dsc.length, _ ("Bytes"));
        ADD (s, EXTRACTOR_METATYPE_EMBEDDED_FILE_SIZE);
        break;
      }

    case AED_ID_REAL_NAME:
      if ((dsc.length > 0) &&
          (dsc.length < 2048) &&
          ((uint64_t) dsc.offset + dsc.length < file_size))
      {
        char s[2048];

        if ((int64_t) dsc.offset !=
            ec->seek (ec->cls, (int64_t) dsc.offset, SEEK_SET))
          return;
        got = ec->read (ec->cls, &data, dsc.length);
        if (got > 0)
        {
          memcpy (s, data, got);
          s[got] = '\0';
          ADD (s, EXTRACTOR_METATYPE_FILENAME);
        }
      }
      break;

    case AED_ID_COMMENT:
      if ((dsc.length > 0) &&
          (dsc.length < 65536) &&
          ((uint64_t) dsc.offset + dsc.length < file_size))
      {
        if ((int64_t) dsc.offset !=
            ec->seek (ec->cls, (int64_t) dsc.offset, SEEK_SET))
          return;
        got = ec->read (ec->cls, &data, dsc.length);
        if (got > 0)
        {
          char *s = malloc ((size_t) got + 1);

          if (NULL != s)
          {
            memcpy (s, data, got);
            s[got] = '\0';
            if (0 != ec->proc (ec->cls,
                               "applefile",
                               EXTRACTOR_METATYPE_COMMENT,
                               EXTRACTOR_METAFORMAT_UTF8,
                               "text/plain",
                               s,
                               (size_t) got + 1))
            {
              free (s);
              return;
            }
            free (s);
          }
        }
      }
      break;

    case AED_ID_FINDER_INFO:
      /* Finder info block: first 4 bytes = file type, next 4 = creator */
      if ((dsc.length >= 8) &&
          ((uint64_t) dsc.offset + dsc.length < file_size))
      {
        char type_s[5];
        char creator_s[5];

        if ((int64_t) dsc.offset !=
            ec->seek (ec->cls, (int64_t) dsc.offset, SEEK_SET))
          return;
        got = ec->read (ec->cls, &data, 8);
        if (got < 8)
          break;
        /* copy both before any further read or proc call */
        memcpy (type_s, data, 4);
        type_s[4] = '\0';
        memcpy (creator_s, (const char *) data + 4, 4);
        creator_s[4] = '\0';

        if (0 != ec->proc (ec->cls,
                           "applefile",
                           EXTRACTOR_METATYPE_FINDER_FILE_TYPE,
                           EXTRACTOR_METAFORMAT_C_STRING,
                           "text/plain",
                           type_s,
                           strlen (type_s) + 1))
          return;
        if (0 != ec->proc (ec->cls,
                           "applefile",
                           EXTRACTOR_METATYPE_FINDER_FILE_CREATOR,
                           EXTRACTOR_METAFORMAT_C_STRING,
                           "text/plain",
                           creator_s,
                           strlen (creator_s) + 1))
          return;
      }
      break;

    default:
      break;
    }
  }
}


/* end of applefile_extractor.c */
