/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file ui/file_load_entry.c
 */

#include "file_load_entry.h"

#include "../application.h"
#include "../ui.h"

#include "chat_title.h"

static void
handle_cancel_button_click(GNUNET_UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  UI_FILE_LOAD_ENTRY_Handle* handle = (UI_FILE_LOAD_ENTRY_Handle*) user_data;

  if (handle->chat_title)
    ui_chat_title_remove_file_load(handle->chat_title, handle);

  // TODO: cancel upload?
}

UI_FILE_LOAD_ENTRY_Handle*
ui_file_load_entry_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_FILE_LOAD_ENTRY_Handle* handle = g_malloc(sizeof(UI_FILE_LOAD_ENTRY_Handle));

  handle->chat_title = NULL;

  handle->builder = ui_builder_from_resource(
      application_get_resource_path(app, "ui/file_load_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
      gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->file_image = GTK_IMAGE(
      gtk_builder_get_object(handle->builder, "file_image")
  );

  handle->file_label = GTK_LABEL(
      gtk_builder_get_object(handle->builder, "file_label")
  );

  handle->load_progress_bar = GTK_PROGRESS_BAR(
      gtk_builder_get_object(handle->builder, "load_progress_bar")
  );

  handle->cancel_button = GTK_BUTTON(
      gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
      handle->cancel_button,
      "clicked",
      G_CALLBACK(handle_cancel_button_click),
      handle
  );

  return handle;
}

void
ui_file_load_entry_delete(UI_FILE_LOAD_ENTRY_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  g_free(handle);
}
