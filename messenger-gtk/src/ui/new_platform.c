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
 * @file ui/new_platform.c
 */

#include "new_platform.h"

#include "../application.h"
#include "../ui.h"

static void
_open_new_platform(GtkEntry *entry,
                   MESSENGER_Application *app)
{
  g_assert((entry) && (app));

  const gchar *topic = gtk_entry_get_text(entry);

  GString *topic_string = g_string_new(topic);

  application_chat_lock(app);

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_group_create(
    app->chat.messenger.handle,
    topic_string->str
  );

  g_string_prepend_c(topic_string, '#');
  GNUNET_CHAT_group_set_name(group, topic_string->str);

  application_chat_unlock(app);

  g_string_free(topic_string, TRUE);
}

static void
handle_platform_entry_changed(GtkEditable *editable,
                              gpointer user_data)
{
  g_assert((editable) && (user_data));

  HdyAvatar *avatar = HDY_AVATAR(user_data);
  GtkEntry *entry = GTK_ENTRY(editable);

  const gchar *text = gtk_entry_get_text(entry);

  hdy_avatar_set_text(avatar, text);

  GString *topic_string = g_string_new(text);

  g_string_prepend_c(topic_string, '#');
  hdy_avatar_set_text(avatar, topic_string->str);

  g_string_free(topic_string, TRUE);
}

static void
handle_platform_entry_activate(GtkEntry *entry,
                               gpointer user_data)
{
  g_assert((entry) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _open_new_platform(entry, app);

  gtk_window_close(GTK_WINDOW(app->ui.new_platform.dialog));
}

static void
handle_cancel_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_confirm_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _open_new_platform(app->ui.new_platform.platform_entry, app);

  gtk_window_close(GTK_WINDOW(app->ui.new_platform.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_new_platform_dialog_cleanup((UI_NEW_PLATFORM_Handle*) user_data);
}

void
ui_new_platform_dialog_init(MESSENGER_Application *app,
                            UI_NEW_PLATFORM_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/new_platform.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "new_platform_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->platform_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "platform_avatar")
  );

  handle->platform_avatar_file = GTK_FILE_CHOOSER_BUTTON(
    gtk_builder_get_object(handle->builder, "platform_avatar_file")
  );

  handle->platform_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "platform_entry")
  );

  g_signal_connect(
    handle->platform_entry,
    "changed",
    G_CALLBACK(handle_platform_entry_changed),
    handle->platform_avatar
  );

  g_signal_connect(
    handle->platform_entry,
    "activate",
    G_CALLBACK(handle_platform_entry_activate),
    app
  );

  handle->cancel_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
    handle->cancel_button,
    "clicked",
    G_CALLBACK(handle_cancel_button_click),
    handle->dialog
  );

  handle->confirm_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "confirm_button")
  );

  g_signal_connect(
    handle->confirm_button,
    "clicked",
    G_CALLBACK(handle_confirm_button_click),
    app
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );
}

void
ui_new_platform_dialog_cleanup(UI_NEW_PLATFORM_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
