/*
   This file is part of GNUnet.
   Copyright (C) 2022 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author Tobias Frisch
 * @file ui/play_media.h
 */

#ifndef UI_PLAY_MEDIA_H_
#define UI_PLAY_MEDIA_H_

#include "messenger.h"

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <pthread.h>

typedef struct UI_PLAY_MEDIA_Handle
{
  gboolean fullscreen;

  GstElement *pipeline;
  GstElement *sink;

  GtkWindow *parent;

  GtkBuilder *builder;
  HdyWindow *window;

  GtkRevealer *header_revealer;
  HdyHeaderBar *title_bar;
  GtkButton *back_button;

  HdyFlap *controls_flap;

  GtkStack *preview_stack;
  GtkWidget *fail_box;
  GtkWidget *video_box;

  GtkButton *play_pause_button;
  GtkStack *play_symbol_stack;

  GtkVolumeButton *volume_button;
  GtkLabel *timeline_label;
  GtkProgressBar *timeline_progress_bar;
  GtkScale* timeline_scale;

  GtkButton *settings_button;

  GtkButton *fullscreen_button;
  GtkStack *fullscreen_symbol_stack;

  guint timeline;
  guint motion_lost;

  guint timeline_signal;

  pthread_t video_tid;
} UI_PLAY_MEDIA_Handle;

/**
 * Returns whether the file extension of a given
 * filename is supported for playing the type of
 * media.
 *
 * @param filename Filename of potential media file
 * @return TRUE if the extension is supported for playback, otherwise FALSE
 */
gboolean
ui_play_media_window_supports_file_extension(const gchar *filename);

/**
 * Initializes a handle for the play media window
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Play media window handle
 */
void
ui_play_media_window_init(MESSENGER_Application *app,
                          UI_PLAY_MEDIA_Handle *handle);

/**
 * Updates a handle for the play media window with
 * a specific uri to play a media file from.
 *
 * @param handle Play media window handle
 * @param uri URI of media file
 * @param file Media file handle or NULL
 */
void
ui_play_media_window_update(UI_PLAY_MEDIA_Handle *handle,
                            const gchar *uri,
                            const struct GNUNET_CHAT_File *file);

/**
 * Cleans up the allocated resources and resets the
 * state of a given play media window handle.
 *
 * @param handle Play media window handle
 */
void
ui_play_media_window_cleanup(UI_PLAY_MEDIA_Handle *handle);

#endif /* UI_PLAY_MEDIA_H_ */
