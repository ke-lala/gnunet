/*
     This file is part of libextractor.
     Copyright (C) 2021 Christian Grothoff

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

NOTE: This plugin is not yet working. Somehow libvlc never calls any of the IO callbacks.

*/
/**
 * @file plugins/vlc_extractor.c
 * @brief plugin to extract metadata using libvlc
 * @author Christian Grothoff
 */
#include <vlc/vlc.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


static void
extract (void *ptr,
         libvlc_media_t *media)
{
  struct
  {
    enum libvlc_meta_t vt;
  } map[] = {
    { libvlc_meta_Title, },
    { libvlc_meta_Artist, },
    { libvlc_meta_Genre, },
    { libvlc_meta_Copyright, },
    { libvlc_meta_Album, },
    { libvlc_meta_TrackNumber, },
    { libvlc_meta_Description, },
    { libvlc_meta_Rating, },
    { libvlc_meta_Date, },
    { libvlc_meta_Setting, },
    { libvlc_meta_URL, },
    { libvlc_meta_Language, },
    { libvlc_meta_NowPlaying, },
    { libvlc_meta_Publisher, },
    { libvlc_meta_EncodedBy, },
    { libvlc_meta_ArtworkURL, },
    { libvlc_meta_TrackID, },
    { libvlc_meta_TrackTotal, },
    { libvlc_meta_Director, },
    { libvlc_meta_Season, },
    { libvlc_meta_Episode, },
    { libvlc_meta_ShowName, },
    { libvlc_meta_Actors, },
    { libvlc_meta_AlbumArtist, },
    { libvlc_meta_DiscNumber, },
    { libvlc_meta_DiscTotal, },
    { -1 }
  };

  for (unsigned int i = 0;
       -1 != map[i].vt;
       i++)
  {
    char *meta;

    fprintf (stderr,
             ".");
    meta = libvlc_media_get_meta (media,
                                  map[i].vt);
    if (NULL == meta)
      continue;
    fprintf (stderr,
             "Found %d: %s\n",
             map[i].vt,
             meta);
    free (meta);
  }
}


static void
media_ready (const struct libvlc_event_t *p_event,
             void *p_data)
{
  fprintf (stderr,
           "media status: %d, %d\n",
           p_event->type == libvlc_MediaParsedChanged,
           p_event->u.media_parsed_changed.new_status);
  if (p_event->u.media_parsed_changed.new_status ==
      libvlc_media_parsed_status_done)
  {
    fprintf (stderr,
             "media ready\n");
  }
}


static void
my_logger (void *data,
           int level,
           const libvlc_log_t *ctx,
           const char *fmt,
           va_list args)
{
  vfprintf (stderr,
            fmt,
            args);
  fprintf (stderr, "\n");
}


/**
 * Extract information using libvlc
 *
 * @param ec extraction context
 */
void
main (int argc,
      char **argv)
{
  libvlc_instance_t *vlc;
  libvlc_media_t *media;
  libvlc_event_manager_t *em;

  {
    sigset_t set;

    signal (SIGCHLD, SIG_DFL);
    sigemptyset (&set);
    sigaddset (&set, SIGPIPE);
    pthread_sigmask (SIG_BLOCK, &set, NULL);
  }

  {
    const char *argv[] = {
      "-v",
      "3",
      NULL
    };
    vlc = libvlc_new (2, argv);
  }
  if (NULL == vlc)
    return;
  libvlc_log_set (vlc,
                  &my_logger,
                  NULL);
  if (0)
  {
    media = libvlc_media_new_path (vlc,
                                   argv[1]);
  }
  else
  {
    int fd = open (argv[1],
                   O_RDONLY);
    if (-1 == fd)
    {
      fprintf (stderr,
               "Open %s failed: %s\n",
               argv[1],
               strerror (errno));
      libvlc_release (vlc);
      return;
    }
    media = libvlc_media_new_fd (vlc,
                                 fd);
  }
  if (NULL == media)
  {
    fprintf (stderr,
             "Opening path `%s' failed!\n",
             argv[1]);
    libvlc_release (vlc);
    return;
  }

  em = libvlc_media_event_manager (media);
  libvlc_event_attach (em,
                       libvlc_MediaParsedChanged,
                       &media_ready,
                       NULL);
  fprintf (stderr,
           "Triggering parser\n");
  {
    int status;

    status = libvlc_media_parse_with_options (media,
                                              libvlc_media_fetch_local
                                              | libvlc_media_parse_network
                                              | libvlc_media_fetch_network,
                                              30000); /* 30s timeout */
    fprintf (stderr,
             "Status: %d\n",
             status);
  }
  fprintf (stderr,
           "Sleeping\n");
  sleep (2);
  extract (NULL,
           media);
  libvlc_media_release (media);
  libvlc_release (vlc);
}


/* end of vlc_extractor.c */
