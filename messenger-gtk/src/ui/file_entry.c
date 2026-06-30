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
 * @file ui/file_entry.c
 */

#include "file_entry.h"

#include "../application.h"
#include "../ui.h"
#include <gnunet/gnunet_chat_lib.h>
#include <stdint.h>

UI_FILE_ENTRY_Handle*
ui_file_entry_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_FILE_ENTRY_Handle* handle = g_malloc(sizeof(UI_FILE_ENTRY_Handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/file_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->file_image = GTK_IMAGE(
    gtk_builder_get_object(handle->builder, "file_image")
  );

  handle->name_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "name_label")
  );

  handle->size_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "size_label")
  );

  return handle;
}

void
ui_file_entry_update(UI_FILE_ENTRY_Handle *handle,
                     struct GNUNET_CHAT_File *file)
{
  g_assert((handle) && (file));

  ui_label_set_text(handle->name_label, GNUNET_CHAT_file_get_name(file));
  ui_label_set_size(handle->size_label, GNUNET_CHAT_file_get_size(file));
}

void
ui_file_entry_delete(UI_FILE_ENTRY_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  g_free(handle);
}
