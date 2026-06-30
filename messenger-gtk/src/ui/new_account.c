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
 * @file ui/new_account.c
 */

#include "new_account.h"

#include "../application.h"
#include "../contact.h"
#include "../file.h"
#include "../ui.h"

static void
_open_new_account(GtkEntry *entry, 
                  MESSENGER_Application *app)
{
  g_assert((entry) && (app));

  const gchar *name = gtk_entry_get_text(entry);

  application_chat_lock(app);

  const enum GNUNET_GenericReturnValue result = GNUNET_CHAT_account_create(
    app->chat.messenger.handle, name
  );

  application_chat_unlock(app);

  if (GNUNET_OK != result)
    return;

  app->ui.state = MESSENGER_STATE_NEW_ACCOUNT;

  gtk_list_box_unselect_all(app->ui.messenger.accounts_listbox);
  gtk_widget_set_sensitive(GTK_WIDGET(app->ui.new_account.dialog), FALSE);
}

static void
handle_account_entry_changed(GtkEditable *editable,
                             gpointer user_data)
{
  g_assert((editable) && (user_data));

  UI_NEW_ACCOUNT_Handle *handle = (UI_NEW_ACCOUNT_Handle*) user_data;

  const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));

  hdy_avatar_set_text(handle->account_avatar, text);

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->confirm_button),
    (text) && (strlen(text) > 0)
  );
}

static void
handle_account_entry_activate(UNUSED GtkEntry *entry,
                              gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _open_new_account(app->ui.new_account.account_entry, app);
}

static void
handle_account_avatar_file_update_preview(GtkFileChooser *file_chooser,
                                          gpointer user_data)
{
  g_assert((file_chooser) && (user_data));

  HdyAvatar *avatar = HDY_AVATAR(user_data);

  gboolean have_preview = false;
  gchar *filename = gtk_file_chooser_get_preview_filename(file_chooser);

  if ((!filename) || (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)))
    goto skip_preview;

  GFile *file = g_file_new_for_path(filename);

  if (!file)
    goto skip_icon;

  GIcon *icon = g_file_icon_new(file);

  if (!icon)
    goto skip_avatar;

  hdy_avatar_set_loadable_icon(avatar, G_LOADABLE_ICON(icon));
  g_object_unref(icon);
  have_preview = true;

skip_avatar:
  g_object_unref(file);

skip_icon:
  g_free(filename);

skip_preview:
  gtk_file_chooser_set_preview_widget_active(file_chooser, have_preview);
}

static void
handle_account_avatar_file_set(GtkFileChooserButton *button,
                               gpointer user_data)
{
  g_assert(user_data);

  GtkFileChooser *file_chooser = GTK_FILE_CHOOSER(button);
  UI_NEW_ACCOUNT_Handle *handle = (UI_NEW_ACCOUNT_Handle*) user_data;

  if (handle->filename)
    g_free(handle->filename);

  handle->filename = gtk_file_chooser_get_preview_filename(file_chooser);
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

  _open_new_account(app->ui.new_account.account_entry, app);
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_new_account_dialog_cleanup(&(app->ui.new_account));

  if (MESSENGER_STATE_MAIN_WINDOW == app->ui.state)
    return;

  gtk_widget_destroy(GTK_WIDGET(app->ui.messenger.main_window));
}

void
ui_new_account_dialog_init(MESSENGER_Application *app,
                           UI_NEW_ACCOUNT_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/new_account.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "new_account_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->account_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "account_avatar")
  );

  handle->account_avatar_file = GTK_FILE_CHOOSER_BUTTON(
    gtk_builder_get_object(handle->builder, "account_avatar_file")
  );

  g_signal_connect(
    handle->account_avatar_file,
    "update-preview",
    G_CALLBACK(handle_account_avatar_file_update_preview),
    handle->account_avatar
  );

  g_signal_connect(
    handle->account_avatar_file,
    "file-set",
    G_CALLBACK(handle_account_avatar_file_set),
    handle
  );

  handle->account_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "account_entry")
  );

  g_signal_connect(
    handle->account_entry,
    "changed",
    G_CALLBACK(handle_account_entry_changed),
    handle
  );

  g_signal_connect(
    handle->account_entry,
    "activate",
    G_CALLBACK(handle_account_entry_activate),
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
    app
  );
}

static void
_cb_file_upload(void *cls,
                struct GNUNET_CHAT_File *file,
                uint64_t completed,
                uint64_t size)
{
  g_assert((cls) && (file));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  file_update_upload_info(file, completed, size);

  if (completed < size)
    return;

  struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_file_get_uri(file);

  if (!uri)
    return;

  char *uri_string = GNUNET_CHAT_uri_to_string(uri);

  if (uri_string)
  {
    GNUNET_CHAT_set_attribute(
      app->chat.messenger.handle,
      GNUNET_CHAT_ATTRIBUTE_AVATAR,
      uri_string
    );

    GNUNET_free(uri_string);
  }

  GNUNET_CHAT_uri_destroy(uri);
}

void
ui_new_account_dialog_update(MESSENGER_Application *app,
                             UI_NEW_ACCOUNT_Handle *handle)
{
  g_assert((app) && (handle));

  gtk_widget_set_sensitive(GTK_WIDGET(app->ui.new_account.dialog), TRUE);
  gtk_window_close(GTK_WINDOW(app->ui.new_account.dialog));

  if (!(handle->filename))
    return;

  GNUNET_CHAT_upload_file(
    app->chat.messenger.handle,
    handle->filename,
    _cb_file_upload,
    app
  );
  
  g_free(handle->filename);
  handle->filename = NULL;
}

void
ui_new_account_dialog_cleanup(UI_NEW_ACCOUNT_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  if (handle->filename)
    g_free(handle->filename);

  memset(handle, 0, sizeof(*handle));
}
