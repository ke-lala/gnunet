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

*/
/**
 * @file plugins/vlc_extractor.c
 * @brief plugin to extract metadata using libvlc
 * @author Christian Grothoff
 */
#include "platform.h"
#include "extractor.h"
#include <vlc/vlc.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

struct IORequest
{
  enum { IO_NONE, IO_READ, IO_SEEK, IO_OPEN, IO_CLOSE, IO_DONE } op;
  uint64_t offset;
  unsigned char *buf;
  size_t len;
  ssize_t result;
  uint64_t size;
  int open_ok;
  pthread_mutex_t mu;
  pthread_cond_t request_ready;
  pthread_cond_t response_ready;
  struct EXTRACTOR_ExtractContext *ec;
  int io_pending;
  int stop_requested;   /* player event fired; stop thread should act */
  int player_stopped;   /* set when MediaPlayerStopped fires */
  libvlc_media_player_t *player;  /* needed by the stop thread */
};


/* ------------------------------------------------------------------ */
/* VLC callbacks — these run in VLC's threads.                         */
/* They post an I/O request and sleep until the main thread services it */
/* ------------------------------------------------------------------ */


static int
open_cb (void *cls,
         void **datap,
         uint64_t *sizep)
{
  struct IORequest *io = cls;
  int ok;

  pthread_mutex_lock (&io->mu);
  io->op = IO_OPEN;
  io->io_pending = 1;
  pthread_cond_signal (&io->request_ready);
  pthread_cond_wait (&io->response_ready,
                     &io->mu);
  *datap = io;
  *sizep = io->size;
  ok = io->open_ok;
  pthread_mutex_unlock (&io->mu);
  return ok ? 0 : 1;
}


static ssize_t
read_cb (void *cls,
         unsigned char *buf,
         size_t len)
{
  struct IORequest *io = cls;
  ssize_t ret;

  pthread_mutex_lock (&io->mu);
  io->op  = IO_READ;
  io->buf = buf;
  io->len = len;
  io->io_pending = 1;
  pthread_cond_signal (&io->request_ready);
  pthread_cond_wait (&io->response_ready,
                     &io->mu);
  ret = io->result;
  pthread_mutex_unlock (&io->mu);
  return ret;
}


static int
seek_cb (void *cls,
         uint64_t offset)
{
  struct IORequest *io = cls;
  int ret;

  if (offset > INT64_MAX)
    return -1;
  pthread_mutex_lock (&io->mu);
  io->op     = IO_SEEK;
  io->offset = offset;
  io->io_pending = 1;
  pthread_cond_signal (&io->request_ready);
  pthread_cond_wait (&io->response_ready,
                     &io->mu);
  ret = (int) io->result;
  pthread_mutex_unlock (&io->mu);
  return ret;
}


/* close_cb IS the termination signal — no stop() needed */
static void
close_cb (void *cls)
{
  struct IORequest *io = cls;

  pthread_mutex_lock (&io->mu);
  io->op = IO_DONE;
  io->io_pending = 1;
  pthread_cond_signal (&io->request_ready);
  pthread_cond_wait (&io->response_ready,
                     &io->mu);
  pthread_mutex_unlock (&io->mu);
}


static void
player_stopped_event (const struct libvlc_event_t *p_event,
                      void *p_data)
{
  struct IORequest *io = p_data;

  pthread_mutex_lock (&io->mu);
  io->player_stopped = 1;
  pthread_cond_broadcast (&io->request_ready);
  pthread_mutex_unlock (&io->mu);
}


/* ------------------------------------------------------------------ */
/* Main-thread I/O dispatcher loop                                     */
/* Runs on the calling thread so ec->read / ec->seek are single-thread */
/* ------------------------------------------------------------------ */

static void
run_io_loop (struct IORequest *io)
{
  struct EXTRACTOR_ExtractContext *ec = io->ec;

  pthread_mutex_lock (&io->mu);
  while (1)
  {
    /* wait for VLC to post a request */
    while (! io->io_pending)
      pthread_cond_wait (&io->request_ready,
                         &io->mu);
    io->io_pending = 0;

    switch (io->op)
    {
    case IO_OPEN:
      {
        uint64_t sz = ec->get_size (ec->cls);

        io->size = sz;
        io->open_ok = (sz != UINT64_MAX);
        io->op = IO_NONE;
        pthread_cond_signal (&io->response_ready);
        break;
      }
    case IO_READ:
      {
        void *data;
        ssize_t r = ec->read (ec->cls,
                              &data,
                              io->len);

        if (r > 0)
          memcpy (io->buf,
                  data,
                  (size_t) r);
        io->result = r;
        io->op = IO_NONE;
        pthread_cond_signal (&io->response_ready);
        break;
      }
    case IO_SEEK:
      {
        io->result = ec->seek (ec->cls,
                               (int64_t) io->offset,
                               SEEK_SET);
        io->op = IO_NONE;
        pthread_cond_signal (&io->response_ready);
        break;
      }
    case IO_CLOSE:
      io->op = IO_NONE;
      pthread_cond_signal (&io->response_ready);   /* ack close */
      break;

    case IO_DONE:
      io->op = IO_NONE;
      pthread_cond_signal (&io->response_ready);  /* ack close_cb */
      pthread_mutex_unlock (&io->mu);
      return;

    default:
      io->op = IO_NONE;
      break;
    }
  }
}


/* ------------------------------------------------------------------ */
/* extract() and my_logger unchanged from your version                 */
/* ------------------------------------------------------------------ */

static void
my_logger (void *data,
           int level,
           const libvlc_log_t *ctx,
           const char *fmt,
           va_list args)
{
#if 0
  vfprintf (stderr,
            fmt,
            args);
  fprintf (stderr, "\n");
#endif
}


static void
extract (struct EXTRACTOR_ExtractContext *ec,
         libvlc_media_t *media)
{
  struct
  {
    enum libvlc_meta_t vt;
    enum EXTRACTOR_MetaType mt;
  } map[] = {
    { libvlc_meta_Title,
      EXTRACTOR_METATYPE_TITLE },
    { libvlc_meta_Artist,
      EXTRACTOR_METATYPE_ARTIST },
    { libvlc_meta_Genre,
      EXTRACTOR_METATYPE_GENRE },
    { libvlc_meta_Copyright,
      EXTRACTOR_METATYPE_COPYRIGHT },
    { libvlc_meta_Album,
      EXTRACTOR_METATYPE_ALBUM },
    { libvlc_meta_TrackNumber,
      EXTRACTOR_METATYPE_TRACK_NUMBER },
    { libvlc_meta_Description,
      EXTRACTOR_METATYPE_DESCRIPTION },
    { libvlc_meta_Rating,
      EXTRACTOR_METATYPE_RATING },
    { libvlc_meta_Date,
      EXTRACTOR_METATYPE_CREATION_TIME },
    { libvlc_meta_Setting,
      EXTRACTOR_METATYPE_UNKNOWN },
    { libvlc_meta_URL,
      EXTRACTOR_METATYPE_URL },
    { libvlc_meta_Language,
      EXTRACTOR_METATYPE_LANGUAGE },
    { libvlc_meta_NowPlaying,
      EXTRACTOR_METATYPE_UNKNOWN },
    { libvlc_meta_Publisher,
      EXTRACTOR_METATYPE_PUBLISHER },
    { libvlc_meta_EncodedBy,
      EXTRACTOR_METATYPE_ENCODED_BY },
    { libvlc_meta_ArtworkURL,
      EXTRACTOR_METATYPE_URL },
    { libvlc_meta_TrackID,
      EXTRACTOR_METATYPE_TRACK_NUMBER },
    { libvlc_meta_TrackTotal,
      EXTRACTOR_METATYPE_UNKNOWN },
    { libvlc_meta_Director,
      EXTRACTOR_METATYPE_MOVIE_DIRECTOR },
    { libvlc_meta_Season,
      EXTRACTOR_METATYPE_SHOW_SEASON_NUMBER },
    { libvlc_meta_Episode,
      EXTRACTOR_METATYPE_SHOW_EPISODE_NUMBER },
    { libvlc_meta_ShowName,
      EXTRACTOR_METATYPE_SHOW_NAME },
    { libvlc_meta_Actors,
      EXTRACTOR_METATYPE_PERFORMER },
    { libvlc_meta_AlbumArtist,
      EXTRACTOR_METATYPE_ARTIST },
    { libvlc_meta_DiscNumber,
      EXTRACTOR_METATYPE_DISC_NUMBER },
    { libvlc_meta_DiscTotal,
      EXTRACTOR_METATYPE_UNKNOWN },
    { 0, 0 }
  };

  for (unsigned int i = 0;
       0 != map[i].mt;
       i++)
  {
    char *meta;

    meta = libvlc_media_get_meta (media,
                                  map[i].vt);
    if (NULL == meta)
      continue;
    ec->proc (ec->cls,
              "vlc",
              map[i].mt,
              EXTRACTOR_METAFORMAT_UTF8, /* ??? */
              "text/plain",
              meta,
              strlen (meta) + 1);
    free (meta);
  }
}


/**
 * Extract information using libvlc
 *
 * @param ec extraction context
 */
void
EXTRACTOR_vlc_extract_method (struct EXTRACTOR_ExtractContext *ec);

void
EXTRACTOR_vlc_extract_method (struct EXTRACTOR_ExtractContext *ec)
{
  struct IORequest io = {
    .op = IO_NONE,
    .ec = ec,
  };
  libvlc_instance_t *vlc;
  libvlc_media_t *media;
  libvlc_media_player_t *player;

  pthread_mutex_init (&io.mu,
                      NULL);
  pthread_cond_init (&io.request_ready,
                     NULL);
  pthread_cond_init (&io.response_ready,
                     NULL);

  {
    sigset_t set;

    signal (SIGCHLD,
            SIG_DFL);
    sigemptyset (&set);
    sigaddset (&set,
               SIGPIPE);
    pthread_sigmask (SIG_BLOCK,
                     &set,
                     NULL);
  }

  {
    const char *argv[] = {
      "--no-video",
      "--no-audio",
      "--no-plugins-cache",
      NULL
    };

    vlc = libvlc_new (3,
                      argv);
  }
  if (NULL == vlc)
    goto cleanup;

  libvlc_log_set (vlc,
                  &my_logger,
                  NULL);

  /* Pass &io as closure — callbacks use it to marshal I/O to main thread */
  media = libvlc_media_new_callbacks (vlc,
                                      &open_cb,
                                      &read_cb,
                                      &seek_cb,
                                      &close_cb,
                                      &io);
  if (NULL == media)
  {
    libvlc_release (vlc);
    goto cleanup;
  }

  /* Create a player — this is what actually drives the input thread */
  player = libvlc_media_player_new_from_media (media);
  libvlc_media_release (media);   /* player holds its own reference */
  if (NULL == player)
  {
    libvlc_release (vlc);
    goto cleanup;
  }

  {
    libvlc_event_manager_t *pem = libvlc_media_player_event_manager (player);

    libvlc_event_attach (pem,
                         libvlc_MediaPlayerStopped,
                         &player_stopped_event,
                         &io);
  }

  /* Mute and suppress output completely */
  libvlc_audio_set_mute (player,
                         1);

  /* play() causes VLC to actually open and read the media */
  libvlc_media_player_play (player);

  /* Service I/O until close_cb fires (input thread fully exited) */
  run_io_loop (&io);

  /* Stop immediately — we got what we needed */
  libvlc_media_player_stop (player);

  /* Wait for the player to confirm it has fully stopped */
  pthread_mutex_lock (&io.mu);
  while (! io.player_stopped)
    pthread_cond_wait (&io.request_ready, &io.mu);
  pthread_mutex_unlock (&io.mu);

  /* Get the player's internal media copy to read metadata from */
  {
    libvlc_media_t *played_media = libvlc_media_player_get_media (player);

    if (NULL != played_media)
    {
      extract (ec,
               played_media);
      libvlc_media_release (played_media);
    }
  }
  libvlc_media_player_release (player);
  libvlc_release (vlc);

cleanup:
  pthread_cond_destroy  (&io.response_ready);
  pthread_cond_destroy  (&io.request_ready);
  pthread_mutex_destroy (&io.mu);
}


/* end of vlc_extractor.c */
