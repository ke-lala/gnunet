/*
     This file is part of libextractor.
     Copyright (C) 2004, 2005, 2006, 2009, 2012 Vidyut Samanta and Christian Grothoff

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
 * @file plugins/mpeg_extractor.c
 * @brief plugin to support MPEG files
 * @author Christian Grothoff
 */
#include "platform.h"
#include "extractor.h"
#include <mpeg2dec/mpeg2.h>


/**
 * Give meta data to extractor.
 *
 * @param t type of the meta data
 * @param s meta data value in utf8 format
 */
#define ADD(s,t) do { if (0 != ec->proc (ec->cls, "mpeg", t, \
                                         EXTRACTOR_METAFORMAT_UTF8, \
                                         "text/plain", s, strlen (s) \
                                         + 1)) goto EXIT; \
} while (0)


/**
 * Main entry method for the 'video/mpeg' extraction plugin.
 *
 * @param ec extraction context provided to the plugin
 */
void
EXTRACTOR_mpeg_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  mpeg2dec_t *handle;
  const mpeg2_info_t *info;
  void *buf;
  ssize_t avail;
  mpeg2_state_t state;
  char format[256];
  char lformat[256];
  char gop_format[256];
  int have_gop;
  uint64_t fsize;
  unsigned int fail_count;
  int did_seek;
  int fmt1;
  int fmt2;
  int mime;
  int fpal;
  int fntsc;
  int fsecam;
  int fmac;

  if (NULL == (handle = mpeg2_init ()))
    return;
  if (NULL == (info = mpeg2_info (handle)))
  {
    mpeg2_close (handle);
    return;
  }
  fsize = ec->get_size (ec->cls);
  buf = NULL;
  have_gop = 0;
  fail_count = 0;
  did_seek = 0;
  fmt1 = 0;
  fmt2 = 0;
  mime = 0;
  fpal = 0;
  fntsc = 0;
  fsecam = 0;
  fmac = 0;
  lformat[0] = '\0';
  while (1)
  {
    state = mpeg2_parse (handle);
    switch (state)
    {
    case STATE_BUFFER:
      if (fail_count > 16)
        goto EXIT; /* do not read large non-mpeg files */
      fail_count++;
      if (0 >= (avail = ec->read (ec->cls,
                                  &buf,
                                  16 * 1024)))
        goto EXIT;
      mpeg2_buffer (handle, buf, buf + avail);
      break;
    case STATE_SEQUENCE:
      fail_count = 0;
      format[0] = fsize;
      format[0]++;
      if (0 == mime)
      {
        mime = 1;
        ADD ("video/mpeg", EXTRACTOR_METATYPE_MIMETYPE);
      }
      snprintf (format,
                sizeof(format), "%ux%u",
                info->sequence->width, info->sequence->height);
      if (0 != strcmp (lformat,
                       format))
      {
        strcpy (lformat,
                format);
        ADD (format, EXTRACTOR_METATYPE_IMAGE_DIMENSIONS);
      }
      switch (info->sequence->flags & SEQ_VIDEO_FORMAT_UNSPECIFIED)
      {
      case SEQ_VIDEO_FORMAT_PAL:
        if (0 == fpal)
        {
          fpal = 1;
          ADD ("PAL", EXTRACTOR_METATYPE_BROADCAST_TELEVISION_SYSTEM);
        }
        break;
      case SEQ_VIDEO_FORMAT_NTSC:
        if (0 == fntsc)
        {
          fntsc = 1;
          ADD ("NTSC", EXTRACTOR_METATYPE_BROADCAST_TELEVISION_SYSTEM);
        }
        break;
      case SEQ_VIDEO_FORMAT_SECAM:
        if (0 == fsecam)
        {
          fsecam = 1;
          ADD ("SECAM", EXTRACTOR_METATYPE_BROADCAST_TELEVISION_SYSTEM);
        }
        break;
      case SEQ_VIDEO_FORMAT_MAC:
        if (0 == fmac)
        {
          fmac = 1;
          ADD ("MAC", EXTRACTOR_METATYPE_BROADCAST_TELEVISION_SYSTEM);
        }
        break;
      default:
        break;
      }
      if ((info->sequence->flags & SEQ_FLAG_MPEG2) > 0)
      {
        if (0 == fmt1)
        {
          fmt1 = 1;
          ADD ("MPEG2", EXTRACTOR_METATYPE_FORMAT_VERSION);
        }
      }
      else
      {
        if (0 == fmt2)
        {
          fmt2 = 1;
          ADD ("MPEG1", EXTRACTOR_METATYPE_FORMAT_VERSION);
        }
      }
      if ( (0 == did_seek) &&
           (fsize != -1) &&
           (fsize > 1024 * 256 * 2) )
      {
        /* skip to the end of the mpeg for speed */
        did_seek = 1;
        ec->seek (ec->cls,
                  fsize - 256 * 1024,
                  SEEK_SET);
      }
      break;
    case STATE_GOP:
      fail_count = 0;
      if ( (NULL != info->gop) &&
           (0 != info->gop->pictures) )
      {
        snprintf (gop_format,
                  sizeof (gop_format),
                  "%02u:%02u:%02u (%u frames)",
                  info->gop->hours,
                  info->gop->minutes,
                  info->gop->seconds,
                  info->gop->pictures);
        have_gop = 1;
      }
      break;
    case STATE_SLICE:
      fail_count = 0;
      break;
    case STATE_END:
      fail_count = 0;
      break;
    case STATE_INVALID:
      goto EXIT;
    default:
      break;
    }
  }
EXIT:
  if (1 == have_gop)
    ADD (gop_format, EXTRACTOR_METATYPE_DURATION);
  mpeg2_close (handle);
}


/* end of mpeg_extractor.c */
