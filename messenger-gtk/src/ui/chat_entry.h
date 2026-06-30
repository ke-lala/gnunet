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
 * @file ui/chat_entry.h
 */

#ifndef UI_CHAT_ENTRY_H_
#define UI_CHAT_ENTRY_H_

#include "chat.h"

#include <gnunet/gnunet_chat_lib.h>

typedef struct UI_CHAT_ENTRY_Handle
{
  guint update;

  time_t timestamp;
  struct GNUNET_CHAT_Context *context;

  UI_CHAT_Handle *chat;
  GtkBuilder *builder;

  GtkWidget *entry_box;

  HdyAvatar *entry_avatar;

  GtkLabel *title_label;
  GtkLabel *timestamp_label;

  GtkLabel *text_label;
  GtkImage *read_receipt_image;
} UI_CHAT_ENTRY_Handle;

/**
 * Allocates and creates a new chat entry
 * handle to manage a chat in a list for
 * a given messenger application.
 *
 * @param app Messenger application
 * @param context Chat context
 * @return New chat entry handle
 */
UI_CHAT_ENTRY_Handle*
ui_chat_entry_new(MESSENGER_Application *app,
                  struct GNUNET_CHAT_Context *context);

/**
 * Updates a given chat entry handle with the
 * current state of a messenger application and
 * the chat context the chat entry is
 * representing.
 *
 * @param handle Chat entry handle
 * @param app Messenger application
 */
void
ui_chat_entry_update(UI_CHAT_ENTRY_Handle *handle,
                     MESSENGER_Application *app);

/**
 * Frees its resources and destroys a given
 * chat entry handle.
 *
 * @param handle Chat entry handle
 */
void
ui_chat_entry_delete(UI_CHAT_ENTRY_Handle *handle);

/**
 * Fully disposes all resources and handles
 * which are linked to a given chat entry
 * handle by a messenger application. The chat
 * entry handle will be deleted as well.
 *
 * @param handle Chat entry handle
 * @param app Messenger application
 */
void
ui_chat_entry_dispose(UI_CHAT_ENTRY_Handle *handle,
                      MESSENGER_Application *app);

#endif /* UI_CHAT_ENTRY_H_ */
