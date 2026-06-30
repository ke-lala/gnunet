/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024 GNUnet e.V.

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
 * @file ui/chat.h
 */

#ifndef UI_CHAT_H_
#define UI_CHAT_H_

#include <gstreamer-1.0/gst/gst.h>
#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>
#include <stdio.h>

#include <gnunet/gnunet_chat_lib.h>

#define UI_CHAT_SEND_BUTTON_HOLD_INTERVAL 500000 // in microseconds

typedef struct MESSENGER_Application MESSENGER_Application;
typedef struct UI_MESSAGE_Handle UI_MESSAGE_Handle;
typedef struct UI_PICKER_Handle UI_PICKER_Handle;
typedef struct UI_CHAT_TITLE_Handle UI_CHAT_TITLE_Handle;

typedef struct UI_CHAT_Handle
{
  gint64 send_pressed_time;

  gboolean recorded;
  gboolean playing;

  char recording_filename [PATH_MAX];

  guint record_timer;
  guint record_time;

  guint play_timer;

  GstElement *record_pipeline;
  GstElement *record_sink;

  GstElement *play_pipeline;
  GstElement *play_sink;

  guint record_watch;
  guint play_watch;

  MESSENGER_Application *app;
  struct GNUNET_CHAT_Context *context;

  UI_CHAT_TITLE_Handle *title;
  gdouble edge_value;

  GtkBuilder *builder;
  GtkWidget *chat_box;

  HdyFlap *flap_chat_details;

  HdySearchBar *chat_search_bar;
  GtkSearchEntry *chat_search_entry;

  GtkLabel *chat_details_label;
  GtkButton *hide_chat_details_button;
  GtkBox *chat_details_contacts_box;
  GtkBox *chat_details_files_box;
  GtkBox *chat_details_media_box;

  HdyAvatar *chat_details_avatar;

  GtkButton *reveal_identity_button;
  GtkButton *discourse_button;
  GtkStack *block_stack;
  GtkButton *block_button;
  GtkButton *unblock_button;
  GtkButton *leave_chat_button;

  GtkSwitch *chat_notifications_switch;

  GtkScrolledWindow *chat_scrolled_window;

  GtkListBox *chat_contacts_listbox;
  GtkListBox *chat_files_listbox;
  GtkFlowBox *chat_media_flowbox;
  GtkListBox *messages_listbox;

  GtkStack *send_stack;
  GtkWidget *send_text_box;
  GtkWidget *send_recording_box;

  GtkButton *attach_file_button;
  GtkTextView *send_text_view;
  GtkButton *emoji_button;
  GtkButton *send_record_button;
  GtkImage *send_record_symbol;

  GtkPopover *send_popover;
  GtkButton *send_later_button;
  GtkButton *send_now_button;

  GtkButton *recording_close_button;
  GtkButton *recording_play_button;
  GtkImage *play_pause_symbol;
  GtkLabel *recording_label;
  GtkProgressBar *recording_progress_bar;

  GtkRevealer *picker_revealer;

  UI_PICKER_Handle *picker;
} UI_CHAT_Handle;

/**
 * Allocates and creates a new chat handle
 * to manage a chat for a given messenger
 * application.
 *
 * @param app Messenger application
 * @param context Chat context
 * @return New chat handle
 */
UI_CHAT_Handle*
ui_chat_new(MESSENGER_Application *app,
            struct GNUNET_CHAT_Context *context);

/**
 * Updates a given chat handle with the current
 * state of a messenger application and the chat
 * context the chat is representing.
 *
 * @param handle Chat handle
 * @param app Messenger application
 */
void
ui_chat_update(UI_CHAT_Handle *handle,
               MESSENGER_Application *app);

/**
 * Frees its resources and destroys a given
 * chat handle.
 *
 * @param handle Chat handle
 */
void
ui_chat_delete(UI_CHAT_Handle *handle);

/**
 * Add a message handle to a given chat handle
 * to get listed by it for a messenger
 * application.
 *
 * @param handle Chat handle
 * @param app Messenger application
 * @param message Message handle
 */
void
ui_chat_add_message(UI_CHAT_Handle *handle,
                    MESSENGER_Application *app,
                    UI_MESSAGE_Handle *message);

/**
 * Removes a message handle from a given chat
 * handle to remove it from its list for a
 * messenger application.
 *
 * @param handle Chat handle
 * @param app Messenger application
 * @param message Message handle
 */
void
ui_chat_remove_message(UI_CHAT_Handle *handle,
                       MESSENGER_Application *app,
                       UI_MESSAGE_Handle *message);

#endif /* UI_CHAT_H_ */
