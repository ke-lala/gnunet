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
 * @file ui/chat_title.h
 */

#ifndef UI_CHAT_TITLE_H_
#define UI_CHAT_TITLE_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>

#include <gnunet/gnunet_chat_lib.h>

typedef struct MESSENGER_Application MESSENGER_Application;
typedef struct UI_CHAT_Handle UI_CHAT_Handle;
typedef struct UI_FILE_LOAD_ENTRY_Handle UI_FILE_LOAD_ENTRY_Handle;

typedef struct UI_CHAT_TITLE_Handle
{
  const struct GNUNET_CHAT_Contact *contact;

  UI_CHAT_Handle *chat;
  GList *loads;

  GtkBuilder *builder;
  GtkWidget *chat_title_box;

  GtkButton *back_button;

  GtkStack *chat_title_stack;
  GtkWidget *title_box;
  GtkWidget *selection_box;

  HdyAvatar *chat_avatar;
  GtkLabel *chat_title;
  GtkLabel *chat_subtitle;

  GtkButton *chat_load_button;
  GtkPopover *chat_load_popover;
  GtkListBox *chat_load_listbox;

  GtkButton *chat_search_button;
  GtkButton *chat_details_button;

  GtkButton *selection_close_button;
  GtkLabel *selection_count_label;
  GtkButton *selection_tag_button;
  GtkButton *selection_delete_button; 
} UI_CHAT_TITLE_Handle;

/**
 * Allocates and creates a new chat title 
 * handle to manage a chat for a given 
 * messenger application.
 *
 * @param app Messenger application
 * @param chat Chat handle
 * @return New chat title handle
 */
UI_CHAT_TITLE_Handle*
ui_chat_title_new(MESSENGER_Application *app,
                  UI_CHAT_Handle *chat);

/**
 * Updates a given chat title handle with 
 * the current state of a messenger application 
 * and the chat context the chat is representing.
 *
 * @param handle Chat title handle
 * @param app Messenger application
 * @param subtitle Default subtitle value
 */
void
ui_chat_title_update(UI_CHAT_TITLE_Handle *handle,
                     MESSENGER_Application *app,
                     const gchar *subtitle);

/**
 * Frees its resources and destroys a given
 * chat handle.
 *
 * @param handle Chat title handle
 */
void
ui_chat_title_delete(UI_CHAT_TITLE_Handle *handle);

/**
 * Add a file load entry handle to a given chat 
 * title handle to get listed by it.
 *
 * @param handle Chat title handle
 * @param file_load File load entry handle
 */
void
ui_chat_title_add_file_load(UI_CHAT_TITLE_Handle *handle,
                            UI_FILE_LOAD_ENTRY_Handle *file_load);

/**
 * Removes a file load entry handle from a given
 * chat title handle to remove it from its list.
 *
 * @param handle Chat title handle
 * @param file_load File load entry handle
 */
void
ui_chat_title_remove_file_load(UI_CHAT_TITLE_Handle *handle,
                               UI_FILE_LOAD_ENTRY_Handle *file_load);

#endif /* UI_CHAT_H_ */
