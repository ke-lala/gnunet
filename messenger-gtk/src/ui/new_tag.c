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
 * @file ui/new_tag.c
 */

#include "new_tag.h"

#include "../application.h"
#include "../ui.h"

#include <gnunet/gnunet_common.h>

static void
_add_new_tag(MESSENGER_Application *app,
             UI_NEW_TAG_Handle *handle)
{
  g_assert((app) && (handle));

  char *tag = ui_entry_get_text(handle->tag_entry);

  if (handle->callback)
    handle->callback(
      app,
      handle->selected,
      tag,
      handle->user_data
    );
  
  if (tag)
    GNUNET_free(tag);
  
  gtk_window_close(GTK_WINDOW(handle->dialog));
}

static void
handle_tag_entry_changed(GtkEditable *editable,
                         gpointer user_data)
{
  g_assert((editable) && (user_data));

  HdyAvatar *avatar = HDY_AVATAR(user_data);
  GtkEntry *entry = GTK_ENTRY(editable);

  const gchar *text = gtk_entry_get_text(entry);

  hdy_avatar_set_text(avatar, text);
}

static void
handle_tag_entry_activate(UNUSED GtkEntry *entry,
                          gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  _add_new_tag(app, &(app->ui.new_tag));

  gtk_window_close(GTK_WINDOW(app->ui.new_tag.dialog));
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

  _add_new_tag(app, &(app->ui.new_tag));

  gtk_window_close(GTK_WINDOW(app->ui.new_tag.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_new_tag_dialog_cleanup(&(app->ui.new_tag));
}

void
ui_new_tag_dialog_init(MESSENGER_Application *app,
                       UI_NEW_TAG_Handle *handle)
{
  g_assert((app) && (handle));

  handle->selected = NULL;
  handle->user_data = NULL;
  handle->callback = NULL;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/new_tag.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "new_tag_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->tag_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "tag_avatar")
  );

  handle->tag_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "tag_entry")
  );

  g_signal_connect(
    handle->tag_entry,
    "changed",
    G_CALLBACK(handle_tag_entry_changed),
    handle->tag_avatar
  );

  g_signal_connect(
    handle->tag_entry,
    "activate",
    G_CALLBACK(handle_tag_entry_activate),
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

void
ui_new_tag_dialog_link(UI_NEW_TAG_Handle *handle,
                       UI_NEW_TAG_Callback callback,
                       GList *selected,
                       gpointer user_data)
{
  g_assert((handle) && (callback));

  handle->selected = selected;
  handle->user_data = user_data;
  handle->callback = callback;
}

void
ui_new_tag_dialog_cleanup(UI_NEW_TAG_Handle *handle)
{
  g_assert(handle);

  if (handle->selected)
    g_list_free(handle->selected);

  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
