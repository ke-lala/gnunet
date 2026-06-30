/*
 * This file is part of libextractor.
 * Copyright (C) 2021 Christian Grothoff
 *
 * libextractor is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3, or (at your
 * option) any later version.
 *
 * libextractor is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libextractor; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
/**
 * @file plugins/real_extractor.c
 * @brief plugin to support REAL files
 * @author Christian Grothoff
 */
#include "platform.h"
#include "extractor.h"

struct MediaProperties
{
  uint32_t object_id;
  uint32_t size;
  uint16_t object_version;        /* must be 0 */
  uint16_t stream_number;
  uint32_t max_bit_rate;
  uint32_t avg_bit_rate;
  uint32_t max_packet_size;
  uint32_t avg_packet_size;
  uint32_t start_time;
  uint32_t preroll;
  uint32_t duration;
  uint8_t stream_name_size;
  uint8_t data[0];                /* variable length section */
  /*
     uint8_t[stream_name_size]     stream_name;
     uint8_t                       mime_type_size;
     uint8_t[mime_type_size]       mime_type;
     uint32_t                      type_specific_len;
     uint8_t[type_specific_len]    type_specific_data;
   */
};

struct ContentDescription
{
  uint32_t object_id;
  uint32_t size;
  uint16_t object_version;        /* must be 0 */
  uint16_t title_len;
  uint8_t data[0];                /* variable length section */
  /*
     uint8_t[title_len]  title;
     uint16_t    author_len;
     uint8_t[author_len]  author;
     uint16_t    copyright_len;
     uint8_t[copyright_len]  copyright;
     uint16_t    comment_len;
     uint8_t[comment_len]  comment;
   */
};
/* author, copyright and comment are supposed to be ASCII */


#define REAL_HEADER 0x2E524d46
#define MDPR_HEADER 0x4D445052
#define CONT_HEADER 0x434F4e54
#define RAFF4_HEADER 0x2E7261FD


/**
 * Give meta data to LE.
 *
 * @param s utf-8 string meta data value
 * @param t type of the meta data
 */
#define ADD(s,t) do { \
    if (0 != ec->proc (ec->cls, "real", t, \
                       EXTRACTOR_METAFORMAT_C_STRING, \
                       "text/plain", s, strlen (s) + 1)) \
    { return; } \
} while (0)


static void
processMediaProperties (const struct MediaProperties *prop,
                        struct EXTRACTOR_ExtractContext *ec)
{
  uint8_t mime_type_size;
  uint32_t prop_size;

  prop_size = ntohl (prop->size);
  if (prop_size <= sizeof (struct MediaProperties))
    return;
  if (0 != prop->object_version)
    return;
  if (prop_size <= prop->stream_name_size + sizeof (uint8_t)
      + sizeof (struct MediaProperties))
    return;
  mime_type_size = prop->data[prop->stream_name_size];
  if (prop_size > prop->stream_name_size + sizeof (uint8_t)
      + mime_type_size + sizeof (struct MediaProperties))
  {
    char data[mime_type_size + 1];

    memcpy (data,
            &prop->data[prop->stream_name_size + 1],
            mime_type_size);
    data[mime_type_size] = '\0';
    ADD (data,
         EXTRACTOR_METATYPE_MIMETYPE);
  }
}


static void
processContentDescription (const struct ContentDescription *prop,
                           struct EXTRACTOR_ExtractContext *ec)
{
  uint16_t author_len;
  uint16_t copyright_len;
  uint16_t comment_len;
  uint16_t title_len;
  uint32_t prop_size;

  prop_size = ntohl (prop->size);
  if (prop_size <= sizeof (struct ContentDescription))
    return;
  if (0 != prop->object_version)
    return;
  title_len = ntohs (prop->title_len);
  if (prop_size <=
      title_len
      + sizeof (struct ContentDescription))
    return;
  if (title_len > 0)
  {
    char title[title_len + 1];

    memcpy (title,
            &prop->data[0],
            title_len);
    title[title_len] = '\0';
    ADD (title,
         EXTRACTOR_METATYPE_TITLE);
  }
  if (prop_size <=
      title_len
      + sizeof (uint16_t)
      + sizeof (struct ContentDescription))
    return;
  author_len = ntohs (*(uint16_t *) &prop->data[title_len]);
  if (prop_size <=
      title_len
      + sizeof (uint16_t)
      + author_len
      + sizeof (struct ContentDescription))
    return;
  if (author_len > 0)
  {
    char author[author_len + 1];

    memcpy (author,
            &prop->data[title_len
                        + sizeof (uint16_t)],
            author_len);
    author[author_len] = '\0';
    ADD (author,
         EXTRACTOR_METATYPE_AUTHOR_NAME);
  }
  if (prop_size <=
      title_len
      + sizeof (uint16_t)
      + author_len
      + sizeof (uint16_t)
      + sizeof (struct ContentDescription))
    return;
  copyright_len = ntohs (*(uint16_t *) &prop->data[title_len
                                                   + author_len
                                                   + sizeof (uint16_t)]);
  if (prop_size <=
      title_len
      + sizeof (uint16_t)
      + author_len
      + sizeof (uint16_t)
      + copyright_len
      + sizeof (struct ContentDescription))
    return;
  if (copyright_len > 0)
  {
    char copyright[copyright_len + 1];

    memcpy (copyright,
            &prop->data[title_len
                        + sizeof (uint16_t) * 2
                        + author_len],
            copyright_len);
    copyright[copyright_len] = '\0';
    ADD (copyright,
         EXTRACTOR_METATYPE_COPYRIGHT);
  }

  if (prop_size <=
      title_len
      + sizeof (uint16_t)
      + author_len
      + sizeof (uint16_t)
      + copyright_len
      + sizeof (uint16_t)
      + sizeof (struct ContentDescription))
    return;
  comment_len = ntohs (*(uint16_t *) &prop->data[title_len
                                                 + author_len
                                                 + copyright_len
                                                 + 2 * sizeof (uint16_t)]);
  if (prop_size <
      title_len
      + sizeof (uint16_t)
      + author_len
      + sizeof (uint16_t)
      + copyright_len
      + sizeof (uint16_t)
      + comment_len
      + sizeof (struct ContentDescription))
    return;

  if (comment_len > 0)
  {
    char comment[comment_len + 1];

    memcpy (comment,
            &prop->data[title_len
                        + sizeof (uint16_t) * 3
                        + author_len
                        + copyright_len],
            comment_len);
    comment[comment_len] = '\0';
    ADD (comment,
         EXTRACTOR_METATYPE_COMMENT);
  }
}


struct RAFF_Header
{
  uint16_t version;
};

struct RAFF3_Header
{
  uint8_t unknown[10];
  uint32_t data_size;
  /*
     uint8_t tlen;
     uint8_t title[tlen];
     uint8_t alen;
     uint8_t author[alen];
     uint8_t clen;
     uint8_t copyright[clen];
     uint8_t aplen;
     uint8_t app[aplen]; */
};


#define RAFF3_HDR_SIZE 14


struct RAFF4_Header
{
  uint16_t version;
  uint16_t revision;
  uint16_t header_length;
  uint16_t compression_type;
  uint32_t granularity;
  uint32_t total_bytes;
  uint32_t bytes_per_minute;
  uint32_t bytes_per_minute2;
  uint16_t interleave_factor;
  uint16_t interleave_block_size;
  uint32_t user_data;
  float sample_rate;
  uint16_t sample_size;
  uint16_t channels;
  uint8_t interleave_code[5];
  uint8_t compression_code[5];
  uint8_t is_interleaved;
  uint8_t copy_byte;
  uint8_t stream_type;
  /*
     uint8_t tlen;
     uint8_t title[tlen];
     uint8_t alen;
     uint8_t author[alen];
     uint8_t clen;
     uint8_t copyright[clen];
     uint8_t aplen;
     uint8_t app[aplen]; */
};

#define RAFF4_HDR_SIZE 53


static void
extract_raff3 (struct EXTRACTOR_ExtractContext *ec,
               const void *ptr,
               size_t size)
{
  const uint8_t *data = ptr;
  uint8_t tlen;
  uint8_t alen;
  uint8_t clen;
  uint8_t aplen;

  if (size <= RAFF3_HDR_SIZE + 8)
    return;
  tlen = data[8 + RAFF3_HDR_SIZE];
  if (tlen + RAFF3_HDR_SIZE + 12 > size)
    return;
  if (tlen > 0)
  {
    char x[tlen + 1];

    memcpy (x,
            &data[9 + RAFF3_HDR_SIZE],
            tlen);
    x[tlen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_TITLE);
  }
  alen = data[9 + tlen + RAFF3_HDR_SIZE];
  if (tlen + alen + RAFF3_HDR_SIZE + 12 > size)
    return;
  if (alen > 0)
  {
    char x[alen + 1];

    memcpy (x,
            &data[10 + RAFF3_HDR_SIZE + tlen],
            alen);
    x[alen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_AUTHOR_NAME);
  }
  clen = data[10 + tlen + alen + RAFF3_HDR_SIZE];
  if (tlen + alen + clen + RAFF3_HDR_SIZE + 12 > size)
    return;
  if (clen > 0)
  {
    char x[clen + 1];

    memcpy (x,
            &data[11 + RAFF4_HDR_SIZE + tlen + alen],
            clen);
    x[clen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_COPYRIGHT);
  }
  aplen = data[11 + tlen + clen + alen + RAFF3_HDR_SIZE];
  if (tlen + alen + clen + aplen + RAFF3_HDR_SIZE + 12 > size)
    return;
  if (aplen > 0)
  {
    char x[aplen + 1];

    memcpy (x,
            &data[12 + RAFF4_HDR_SIZE + tlen + alen + clen],
            aplen);
    x[aplen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_UNKNOWN);
  }
}


static void
extract_raff4 (struct EXTRACTOR_ExtractContext *ec,
               const void *ptr,
               size_t size)
{
  const uint8_t *data = ptr;
  uint8_t tlen;
  uint8_t alen;
  uint8_t clen;
  uint8_t aplen;

  if (size <= RAFF4_HDR_SIZE + 16 + 4)
    return;
  tlen = data[16 + RAFF4_HDR_SIZE];
  if (tlen + RAFF4_HDR_SIZE + 20 > size)
    return;
  alen = data[17 + tlen + RAFF4_HDR_SIZE];
  if (tlen + alen + RAFF4_HDR_SIZE + 20 > size)
    return;
  clen = data[18 + tlen + alen + RAFF4_HDR_SIZE];
  if (tlen + alen + clen + RAFF4_HDR_SIZE + 20 > size)
    return;
  aplen = data[19 + tlen + clen + alen + RAFF4_HDR_SIZE];
  if (tlen + alen + clen + aplen + RAFF4_HDR_SIZE + 20 > size)
    return;
  if (tlen > 0)
  {
    char x[tlen + 1];

    memcpy (x,
            &data[17 + RAFF4_HDR_SIZE],
            tlen);
    x[tlen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_TITLE);
  }
  if (alen > 0)
  {
    char x[alen + 1];

    memcpy (x,
            &data[18 + RAFF4_HDR_SIZE + tlen],
            alen);
    x[alen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_AUTHOR_NAME);
  }
  if (clen > 0)
  {
    char x[clen + 1];

    memcpy (x,
            &data[19 + RAFF4_HDR_SIZE + tlen + alen],
            clen);
    x[clen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_COPYRIGHT);
  }
  if (aplen > 0)
  {
    char x[aplen + 1];

    memcpy (x,
            &data[20 + RAFF4_HDR_SIZE + tlen + alen + clen],
            aplen);
    x[aplen] = '\0';
    ADD (x,
         EXTRACTOR_METATYPE_UNKNOWN);
  }
}


static void
extract_raff (struct EXTRACTOR_ExtractContext *ec,
              const void *ptr,
              size_t size)
{
  const uint8_t *data = ptr;
  const struct RAFF_Header *hdr;

  /* HELIX */
  if (size <= sizeof (*hdr) + 4)
    return;
  ADD ("audio/vnd.rn-realaudio",
       EXTRACTOR_METATYPE_MIMETYPE);
  hdr = (const struct RAFF_Header *) &data[4];
  switch (ntohs (hdr->version))
  {
  case 3:
    extract_raff3 (ec,
                   ptr,
                   size);
    break;
  case 4:
    extract_raff4 (ec,
                   ptr,
                   size);
    break;
  }
}


/* old real format */
static void
extract_real (struct EXTRACTOR_ExtractContext *ec,
              const void *data,
              size_t size)
{
  uint64_t off = 0;
  size_t pos = 0;

  while (1)
  {
    uint32_t length;

    if ( (pos + 8 > size) ||
         (pos + 8 < pos) ||
         (pos + (length = ntohl (((uint32_t *) (data + pos))[1])) > size) )
    {
      uint64_t noff;
      void *in;
      ssize_t isize;

      noff = ec->seek (ec->cls,
                       off + pos,
                       SEEK_SET);
      if (-1 == noff)
        return;
      isize = ec->read (ec->cls,
                        &in,
                        32 * 1024);
      if (isize < 8)
        return;
      data = in;
      size = isize;
      off = noff;
      pos = 0;
    }
    if (length <= 8)
      return;
    if ( (pos + length > size) ||
         (pos + length < pos) )
      return;
    switch (ntohl (((uint32_t *) (data + pos))[0]))
    {
    case MDPR_HEADER:
      processMediaProperties (data + pos,
                              ec);
      pos += length;
      break;
    case CONT_HEADER:
      processContentDescription (data + pos,
                                 ec);
      pos += length;
      break;
    case REAL_HEADER:          /* treat like default */
    default:
      pos += length;
      break;
    }
  }
}


/**
 * "extract" metadata from a REAL file
 *
 * @param ec extraction context
 */
void
EXTRACTOR_real_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  void *data;
  size_t n;

  n = ec->read (ec->cls,
                &data,
                sizeof (struct RAFF4_Header) + 4 * 256);
  if (n < sizeof (uint32_t))
    return;
  switch (ntohl (*(uint32_t *) data))
  {
  case RAFF4_HEADER:
    extract_raff (ec,
                  data,
                  n);
    break;
  case REAL_HEADER:
    extract_real (ec,
                  data,
                  n);
    break;
  }
}


/* end of real_extractor.c */
