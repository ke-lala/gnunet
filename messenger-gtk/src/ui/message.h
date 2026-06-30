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
 * @file ui/message.h
 */

#ifndef UI_MESSAGE_H_
#define UI_MESSAGE_H_

#include <stdbool.h>

#include <glib-2.0/glib.h>
#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>

#include <gnunet/gnunet_chat_lib.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef enum UI_MESSAGE_Type
{
  UI_MESSAGE_DEFAULT 	= 0,
  UI_MESSAGE_SENT 	= 1,
  UI_MESSAGE_STATUS 	= 2
} UI_MESSAGE_Type;

typedef void (*UI_MESSAGE_StatusCallback)(MESSENGER_Application *app,
                                          gboolean status,
                                          gpointer user_data);

typedef struct UI_MESSAGE_Handle
{
  UI_MESSAGE_Type type;

  time_t timestamp;
  struct GNUNET_CHAT_Message *msg;
  struct GNUNET_CHAT_Contact *contact;

  UI_MESSAGE_StatusCallback status_cb;
  gpointer status_cls;

  GtkBuilder *builder [2];
  GtkWidget *message_box;
  GtkFlowBox *tag_flow_box;

  HdyAvatar *sender_avatar;
  GtkLabel *sender_label;
  GtkImage *private_image;

  GtkRevealer *deny_revealer;
  GtkRevealer *accept_revealer;

  GtkButton *deny_button;
  GtkButton *accept_button;

  GtkLabel *timestamp_label;
  GtkImage *read_receipt_image;

  GtkStack *content_stack;

  GtkLabel *text_label;

  GtkRevealer *file_revealer;
  GtkLabel *filename_label;
  GtkProgressBar *file_progress_bar;
  GtkButton *file_button;
  GtkImage *file_status_image;

  GtkDrawingArea *preview_drawing_area;

  GtkRevealer *media_revealer;
  GtkImage *media_type_image;
  GtkLabel *media_label;
  GtkProgressBar *media_progress_bar;
  GtkButton *media_button;

  MESSENGER_Application *app;
} UI_MESSAGE_Handle;

/**
 * Allocates and creates a new message handle
 * to represent a message for a given messenger
 * application by selected message type.
 *
 * @param app Messenger application
 * @param type Message type
 * @return New message handle
 */
UI_MESSAGE_Handle*
ui_message_new(MESSENGER_Application *app,
               UI_MESSAGE_Type type);

/**
 * Refreshes the visual state of the read receipt
 * from a given message handle.
 *
 * @param handle Message handle
 */
void
ui_message_refresh(UI_MESSAGE_Handle *handle);

/**
 * Updates a given message handle with the
 * current data from a messenger application
 * and a selected chat message.
 *
 * @param handle Message handle
 * @param app Messenger application
 * @param message Chat message
 */
void
ui_message_update(UI_MESSAGE_Handle *handle,
                  MESSENGER_Application *app,
                  struct GNUNET_CHAT_Message *message);

/**
 * Sets the contact of a given message handle
 * to track contact information widgets.
 *
 * @param handle Message handle
 * @param contact Contact
 */
void
ui_message_set_contact(UI_MESSAGE_Handle *handle,
                       struct GNUNET_CHAT_Contact *contact);

/**
 * Sets the callback and closure of a given message
 * handle for actions of a status message.
 *
 * @param handle Message handle
 * @param cb Status callback
 * @param cls Status closure
 */
void
ui_message_set_status_callback(UI_MESSAGE_Handle *handle,
                               UI_MESSAGE_StatusCallback cb,
                               gpointer cls);

/**
 * Adds a widget to represent a given tag message
 * to another message handle.
 *
 * @param handle Message handle
 * @param app Messenger application
 * @param tag_message Chat tag message
 */
void
ui_message_add_tag(UI_MESSAGE_Handle *handle,
                   MESSENGER_Application *app,
                   struct GNUNET_CHAT_Message *tag_message);

/**
 * Remove a widget representing a given tag message
 * from another message handle.
 *
 * @param handle Message handle
 * @param app Messenger application
 * @param tag_message Chat tag message
 */
void
ui_message_remove_tag(UI_MESSAGE_Handle *handle,
                      MESSENGER_Application *app,
                      struct GNUNET_CHAT_Message *tag_message);

/**
 * Frees its resources and destroys a given
 * message handle.
 *
 * @param handle Message handle
 * @param app Messenger application
 */
void
ui_message_delete(UI_MESSAGE_Handle *handle,
                  MESSENGER_Application *app);

#endif /* UI_MESSAGE_H_ */
