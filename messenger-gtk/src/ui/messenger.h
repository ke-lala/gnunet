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
 * @file ui/messenger.h
 */

#ifndef UI_MESSENGER_H_
#define UI_MESSENGER_H_

#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>

#include <gnunet/gnunet_chat_lib.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef struct UI_MESSENGER_Handle
{
  MESSENGER_Application *app;

  GList *chat_entries;
  guint chat_selection;
  guint account_refresh;

  GtkBuilder *builder;
  GtkApplicationWindow *main_window;

  HdyLeaflet *leaflet_title;
  HdyLeaflet *leaflet_chat;
  HdyFlap *flap_user_details;

  GtkWidget *nav_box;
  GtkWidget *main_box;

  HdyHeaderBar *nav_bar;
  HdyHeaderBar *main_bar;

  GtkButton *profile_button;
  HdyAvatar *profile_avatar;
  GtkLabel *profile_label;
  GtkLabel *profile_key_label;

  GtkButton *hide_user_details_button;
  GtkButton *lobby_button;
  GtkButton *account_details_button;
  GtkImage *account_details_symbol;

  GtkRevealer *account_details_revealer;
  GtkListBox *accounts_listbox;
  GtkListBoxRow *add_account_listbox_row;

  GtkButton *new_contact_button;
  GtkButton *new_group_button;
  GtkButton *new_platform_button;
  GtkButton *contacts_button;
  GtkButton *settings_button;
  GtkButton *about_button;

  GtkStack *chats_title_stack;
  GtkWidget *title_box;
  GtkWidget *search_box;

  GtkButton *user_details_button;
  GtkButton *chats_search_button;
  
  GtkSearchEntry *chats_search_entry;
  GtkStack *search_icon_stack;

  GtkListBox *chats_listbox;

  GtkStack *chats_stack;
  GtkWidget *no_chat_box;

  GtkStack *chat_title_stack;
} UI_MESSENGER_Handle;

/**
 * Initializes a handle for the messenger main
 * window of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Messenger window handle
 */
void
ui_messenger_init(MESSENGER_Application *app,
                  UI_MESSENGER_Handle *handle);

/**
 * Refreshes a given messenger window handle with
 * the data of the current state from a given
 * messenger application.
 *
 * @param app Messenger application
 * @param handle Messenger window handle
 */
void
ui_messenger_refresh(MESSENGER_Application *app,
                     UI_MESSENGER_Handle *handle);

/**
 * Returns whether a certain chat context is currently
 * visually represented via a chat UI handle.
 *
 * @param handle Messenger window handle
 * @param context Chat context
 * @return true if the context is viewed, otherwise false
 */
gboolean
ui_messenger_is_context_active(UI_MESSENGER_Handle *handle,
                               struct GNUNET_CHAT_Context *context);

/**
 * Cleans up the allocated resources and resets the
 * state of a given messenger window handle.
 *
 * @param handle Messenger window handle
 */
void
ui_messenger_cleanup(UI_MESSENGER_Handle *handle);

#endif /* UI_MESSENGER_H_ */
