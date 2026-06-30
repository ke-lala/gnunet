/*
     This file is part of libextractor.
     Copyright (C) 2004, 2009, 2012, 2025 Vidyut Samanta and Christian Grothoff

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

     This code was based on AVInfo 1.0 alpha 11
     (c) George Shuklin, gs]AT[shounen.ru, 2002-2004
     http://shounen.ru/soft/avinfo/

     and bitcollider 0.6.0
     (PD) 2004 The Bitzi Corporation
     http://bitzi.com/
 */
/**
 * @file plugins/riff_extractor.c
 * @brief plugin to support RIFF files (AVI, ANI, and others)
 * @author Christian Grothoff
 *
 * RIFF structure:
 *   "RIFF" (4 bytes)
 *   file-size - 8 (4 bytes, little-endian)
 *   form type (4 bytes): "AVI ", "WAVE", "ANI ", etc.
 *   chunks...
 *
 * Each chunk:
 *   chunk ID (4 bytes)
 *   data size (4 bytes, little-endian, excludes the 8-byte header)
 *   data (size bytes, padded to even offset)
 *
 * LIST chunks carry a 4-byte list type immediately after their size,
 * followed by sub-chunks.  LIST INFO is globally standardised and
 * appears in all RIFF-based formats.
 */
#include "platform.h"
#include "extractor.h"
#include <math.h>


/**
 * Read a little-endian uint32 from @a data.
 */
static uint32_t
fread_le (const char *data)
{
  uint32_t result = 0;

  for (unsigned int x = 0; x < 4; x++)
    result |= ((unsigned char) data[x]) << (x * 8);
  return result;
}


/**
 * Round @a num to the nearest integer (avoids depending on C99 round()).
 */
static double
round_double (double num)
{
  return floor (num + 0.5);
}


/**
 * Emit a UTF-8 string as metadata of the given type.
 * Returns from the calling function if proc signals abort.
 */
#define ADD(s, t) do { \
          if (0 != ec->proc (ec->cls, "riff", (t), \
                             EXTRACTOR_METAFORMAT_UTF8, \
                             "text/plain", (s), strlen (s) + 1)) \
          return; \
} while (0)


/**
 * Maximum bytes we read from a single LIST INFO sub-chunk value.
 */
#define INFO_VALUE_MAX 1024

/**
 * Maximum number of chunks we scan at each nesting level to guard
 * against malformed files.
 */
#define MAX_CHUNKS 512


/**
 * Mapping from a LIST INFO four-CC to a libextractor meta type.
 */
struct InfoTag
{
  char id[4];
  enum EXTRACTOR_MetaType type;
};

static const struct InfoTag INFO_TAGS[] = {
  { "INAM", EXTRACTOR_METATYPE_TITLE },
  { "IART", EXTRACTOR_METATYPE_ARTIST },
  { "ICOP", EXTRACTOR_METATYPE_COPYRIGHT },
  { "ICRD", EXTRACTOR_METATYPE_CREATION_DATE },
  { "IGNR", EXTRACTOR_METATYPE_GENRE },
  { "IKEY", EXTRACTOR_METATYPE_KEYWORDS },
  { "ISFT", EXTRACTOR_METATYPE_CREATED_BY_SOFTWARE },
  { "ICMT", EXTRACTOR_METATYPE_COMMENT },
  { "ISRC", EXTRACTOR_METATYPE_SOURCE },
  { "ISBJ", EXTRACTOR_METATYPE_SUBJECT },
  { "ITRK", EXTRACTOR_METATYPE_TRACK_NUMBER },
  { "IPRD", EXTRACTOR_METATYPE_ALBUM },
  { "ILNG", EXTRACTOR_METATYPE_LANGUAGE },
};


/**
 * Video metadata accumulated from an AVI LIST hdrl.
 */
struct AviState
{
  uint32_t us_per_frame;  /* dwMicroSecPerFrame from avih */
  uint32_t total_frames;  /* dwTotalFrames from avih */
  uint32_t width;         /* dwWidth from avih */
  uint32_t height;        /* dwHeight from avih */
  char codec[5];          /* fccHandler from first vids strh, NUL-terminated */
  int have_avih;
  int have_codec;
};


/**
 * Seek to @a pos, then read @a want bytes into *@a data.
 *
 * @return bytes read, or -1 on seek failure
 */
static ssize_t
seek_and_read (struct EXTRACTOR_ExtractContext *ec,
               uint64_t pos,
               void **data,
               size_t want)
{
  if ((int64_t) pos !=
      ec->seek (ec->cls,
                (int64_t) pos,
                SEEK_SET))
    return -1;
  return ec->read (ec->cls,
                   data,
                   want);
}


/**
 * Parse sub-chunks of a LIST INFO chunk and emit all recognised tags.
 *
 * @param ec    extraction context
 * @param start file offset of the first sub-chunk (immediately after list type)
 * @param end   file offset one past the last byte of the enclosing LIST chunk
 * @return 0 to continue, 1 if proc signalled abort
 */
static int
parse_list_info (struct EXTRACTOR_ExtractContext *ec,
                 uint64_t start,
                 uint64_t end)
{
  uint64_t pos = start;

  for (unsigned int n = 0; (pos + 8 <= end) && (n < MAX_CHUNKS); n++)
  {
    void *data;
    ssize_t got;
    char id[4];
    uint32_t csz;

    got = seek_and_read (ec,
                         pos,
                         &data,
                         8);
    if (got < 8)
      break;
    memcpy (id,
            data,
            4);
    csz = fread_le ((const char *) data + 4);
    if ( (pos + 8 + csz > end) ||
         (pos + 8 + csz < pos) )
    {
      /* terminate if chunk overflows the list */
      break;
    }
    if (0 == csz)
    {
      /* skip empty chunk */
      pos += 8;
      continue;
    }

    for (unsigned int i = 0;
         i < sizeof (INFO_TAGS) / sizeof (INFO_TAGS[0]);
         i++)
    {
      size_t rlen = (csz < INFO_VALUE_MAX) ? (size_t) csz : INFO_VALUE_MAX;
      char buf[INFO_VALUE_MAX + 1];
      size_t slen;

      if (0 !=
          memcmp (id,
                  INFO_TAGS[i].id,
                  4))
        continue;
      got = seek_and_read (ec,
                           pos + 8,
                           &data,
                           rlen);
      if (got <= 0)
        break;
      slen = (size_t) got;
      memcpy (buf,
              data,
              slen);
      /* strip trailing NULs and spaces that some encoders pad with */
      while ( (slen > 0) &&
              ( ('\0' == buf[slen - 1]) ||
                (' ' == buf[slen - 1]) ) )
        slen--;
      if (0 == slen)
        break;
      buf[slen] = '\0';
      if (0 != ec->proc (ec->cls,
                         "riff",
                         INFO_TAGS[i].type,
                         EXTRACTOR_METAFORMAT_UTF8,
                         "text/plain",
                         buf,
                         slen + 1))
        return 1;
      break;
    }
    pos += 8 + csz + (csz & 1);
  }
  return 0;
}


/**
 * Parse sub-chunks of a LIST strl; extract the codec fourcc from the
 * first video stream header found.
 *
 * @param ec    extraction context
 * @param start file offset of the first sub-chunk
 * @param end   file offset one past the last byte of the strl LIST
 * @param state AVI state to update
 */
static void
parse_strl (struct EXTRACTOR_ExtractContext *ec,
            uint64_t start,
            uint64_t end,
            struct AviState *state)
{
  uint64_t pos = start;

  for (unsigned int n = 0; (pos + 8 <= end) && (n < MAX_CHUNKS); n++)
  {
    void *data;
    ssize_t got;
    char id[4];
    uint32_t csz;

    got = seek_and_read (ec,
                         pos,
                         &data,
                         8);
    if (got < 8)
      break;
    memcpy (id,
            data,
            4);
    csz = fread_le ((const char *) data + 4);

    if (! state->have_codec &&
        (0 == memcmp (id,
                      "strh",
                      4)) &&
        (csz >= 8) &&
        (pos + 8 + csz <= end))
    {
      /* strh layout: fccType[4] fccHandler[4] ... */
      got = seek_and_read (ec,
                           pos + 8,
                           &data,
                           8);
      if ( (got >= 8) &&
           (0 == memcmp (data,
                         "vids",
                         4)))
      {
        memcpy (state->codec,
                (const char *) data + 4,
                4);
        state->codec[4] = '\0';
        state->have_codec = 1;
      }
    }

    if (0 == csz)
      break;
    pos += 8 + csz + (csz & 1);
  }
}


/**
 * Parse sub-chunks of LIST hdrl in an AVI file; fills @a state with
 * frame timing, dimensions, and the video codec.
 *
 * @param ec    extraction context
 * @param start file offset of the first sub-chunk (after list type "hdrl")
 * @param end   file offset one past the last byte of the hdrl LIST
 * @param state AVI state to update
 */
static void
parse_hdrl (struct EXTRACTOR_ExtractContext *ec,
            uint64_t start,
            uint64_t end,
            struct AviState *state)
{
  uint64_t pos = start;

  for (unsigned int n = 0; (pos + 8 <= end) && (n < MAX_CHUNKS); n++)
  {
    void *data;
    ssize_t got;
    char id[4];
    uint32_t csz;

    got = seek_and_read (ec,
                         pos,
                         &data,
                         8);
    if (got < 8)
      break;
    memcpy (id,
            data,
            4);
    csz = fread_le ((const char *) data + 4);

    if (! state->have_avih &&
        (0 == memcmp (id, "avih", 4)) &&
        (csz >= 40) &&
        (pos + 8 + csz <= end))
    {
      /* AVIMAINHEADER layout (all DWORDs, little-endian):
           [0]  dwMicroSecPerFrame
           [4]  dwMaxBytesPerSec
           [8]  dwPaddingGranularity
           [12] dwFlags
           [16] dwTotalFrames
           [20] dwInitialFrames
           [24] dwStreams
           [28] dwSuggestedBufferSize
           [32] dwWidth
           [36] dwHeight           */
      got = seek_and_read (ec,
                           pos + 8,
                           &data,
                           40);
      if (got >= 40)
      {
        const char *d = data;

        state->us_per_frame = fread_le (&d[0]);
        state->total_frames = fread_le (&d[16]);
        state->width        = fread_le (&d[32]);
        state->height       = fread_le (&d[36]);
        state->have_avih    = 1;
      }
    }
    else if ((0 == memcmp (id,
                           "LIST",
                           4)) &&
             (csz >= 4) &&
             (pos + 8 + csz <= end))
    {
      got = seek_and_read (ec,
                           pos + 8,
                           &data,
                           4);
      if ( (got >= 4) &&
           (0 == memcmp (data,
                         "strl",
                         4)))
        parse_strl (ec,
                    pos + 12,
                    pos + 8 + csz,
                    state);
    }

    if (0 == csz)
      break;
    pos += 8 + csz + (csz & 1);
  }
}


/**
 * Main entry method for the RIFF extraction plugin.
 * Handles any RIFF-based format; extracts LIST INFO tags universally
 * and AVI video stream metadata for "AVI " files.
 *
 * @param ec extraction context provided to the plugin
 */
void
EXTRACTOR_riff_extract_method (struct EXTRACTOR_ExtractContext *ec);

void
EXTRACTOR_riff_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  void *data;
  ssize_t got;
  char form_type[4];
  uint32_t riff_size;
  uint64_t file_size;
  uint64_t riff_end;
  const char *mime;
  int is_avi;
  struct AviState avi;

  /* need at least "RIFF" + size + form type */
  got = ec->read (ec->cls,
                  &data,
                  12);
  if (got < 12)
    return;
  if (0 != memcmp (data,
                   "RIFF",
                   4))
    return;

  riff_size  = fread_le ((const char *) data + 4);
  memcpy (form_type,
          (const char *) data + 8,
          4);

  file_size = ec->get_size (ec->cls);
  /* riff_size counts bytes after the 8-byte RIFF header */
  riff_end = (uint64_t) riff_size + 8;
  if (riff_end > file_size)
    riff_end = file_size;

  /* map known form types to MIME strings */
  if (0 == memcmp (form_type,
                   "AVI ",
                   4))
    mime = "video/x-msvideo";
  else if ((0 == memcmp (form_type,
                         "ANI ",
                         4)) ||
           (0 == memcmp (form_type,
                         "ACON",
                         4)))
    mime = "application/x-navi-animation";
  else if (0 == memcmp (form_type,
                        "RMID",
                        4))
    mime = "audio/midi";
  else
    mime = NULL;   /* unknown or handled by another plugin (e.g. WAVE) */

  if (NULL != mime)
    ADD (mime,
         EXTRACTOR_METATYPE_MIMETYPE);

  is_avi = (0 == memcmp (form_type,
                         "AVI ",
                         4));
  memset (&avi,
          0,
          sizeof (avi));

  /* scan top-level chunks */
  {
    uint64_t pos = 12;

    for (unsigned int n = 0; (pos + 8 <= riff_end) && (n < MAX_CHUNKS); n++)
    {
      char id[4];
      uint32_t csz;

      got = seek_and_read (ec,
                           pos,
                           &data,
                           8);
      if (got < 8)
        break;
      memcpy (id,
              data,
              4);
      csz = fread_le ((const char *) data + 4);

      if (pos + 8 + (uint64_t) csz > riff_end)
        break;   /* chunk overflows the declared file size */

      if ( (0 == memcmp (id,
                         "LIST",
                         4)) &&
           (csz >= 4) )
      {
        char list_type[4];

        got = seek_and_read (ec,
                             pos + 8,
                             &data,
                             4);
        if (got >= 4)
        {
          memcpy (list_type,
                  data,
                  4);

          if (0 == memcmp (list_type,
                           "INFO",
                           4))
          {
            if (0 != parse_list_info (ec,
                                      pos + 12,
                                      pos + 8 + csz))
              return;
          }
          else if (is_avi &&
                   (0 == memcmp (list_type,
                                 "hdrl",
                                 4)))
          {
            parse_hdrl (ec,
                        pos + 12,
                        pos + 8 + csz,
                        &avi);
          }
        }
      }

      if (0 == csz)
        break;
      pos += 8 + csz + (csz & 1);
    }
  }

  /* emit AVI video metadata once we've scanned all chunks */
  if (is_avi &&
      avi.have_avih &&
      avi.have_codec &&
      (avi.us_per_frame > 0))
  {
    unsigned int fps =
      (unsigned int) round_double (1.0e6 / (double) avi.us_per_frame);

    if (fps > 0)
    {
      unsigned int duration =
        (unsigned int) round_double ((double) avi.total_frames * 1000.0
                                     / (double) fps);
      char format[256];

      snprintf (format,
                sizeof (format),
                _ ("codec: %s, %u fps, %u ms"),
                avi.codec,
                fps,
                duration);
      ADD (format,
           EXTRACTOR_METATYPE_FORMAT);
      snprintf (format,
                sizeof (format),
                "%ux%u",
                (unsigned int) avi.width,
                (unsigned int) avi.height);
      ADD (format,
           EXTRACTOR_METATYPE_IMAGE_DIMENSIONS);
    }
  }
}


/* end of riff_extractor.c */
