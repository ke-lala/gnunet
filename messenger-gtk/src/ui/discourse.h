/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file ui/discourse.h
 */

#ifndef UI_DISCOURSE_H_
#define UI_DISCOURSE_H_

#include "messenger.h"

#include <gnunet/gnunet_chat_lib.h>

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <pthread.h>

typedef struct UI_DISCOURSE_Handle
{
  MESSENGER_Application *app;
  struct GNUNET_CHAT_Context *context;

  struct GNUNET_CHAT_Discourse *voice_discourse;
  struct GNUNET_CHAT_Discourse *video_discourse;

  gboolean muted;
  gboolean streaming;

  GtkWindow *parent;

  GtkBuilder *builder;
  HdyWindow *window;

  HdyHeaderBar *title_bar;
  GtkButton *back_button;
  GtkButton *details_button;

  HdyFlap *details_flap;

  GtkStack *discourse_stack;
  GtkWidget *offline_page;
  GtkWidget *members_page;

  GtkFlowBox *members_flowbox;

  GtkButton *microphone_button;
  GtkButton *camera_button;
  GtkButton *screen_button;
  GtkVolumeButton *speakers_button;

  GtkStack *microphone_stack;
  GtkWidget *microphone_on_icon;
  GtkWidget *microphone_off_icon;

  GtkStack *call_stack;
  GtkWidget *call_start_button;
  GtkWidget *call_stop_button;

  GtkButton *close_details_button;
  GtkListBox *contacts_listbox;
} UI_DISCOURSE_Handle;

/**
 * Initializes a handle for the discourse window
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Discourse window handle
 */
void
ui_discourse_window_init(MESSENGER_Application *app,
                         UI_DISCOURSE_Handle *handle);

/**
 * Updates a handle for the discourse window with
 * a given chat context.
 *
 * @param handle Discourse window handle
 * @param context Chat context
 */
void
ui_discourse_window_update(UI_DISCOURSE_Handle *handle,
                           struct GNUNET_CHAT_Context *context);

/**
 * Cleans up the allocated resources and resets the
 * state of a given discourse window handle.
 *
 * @param handle Discourse window handle
 */
void
ui_discourse_window_cleanup(UI_DISCOURSE_Handle *handle);

#endif /* UI_DISCOURSE_H_ */
