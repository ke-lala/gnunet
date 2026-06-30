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
 * @file ui/tag.c
 */

#include "tag.h"

#include "../application.h"
#include "../ui.h"

UI_TAG_Handle*
ui_tag_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_TAG_Handle* handle = g_malloc(sizeof(UI_TAG_Handle));

  memset(handle, 0, sizeof(*handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/tag.ui")
  );

  handle->tag_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "tag_label")
  );

  g_object_set_qdata(
    G_OBJECT(handle->tag_label),
    app->quarks.ui,
    handle
  );

  return handle;
}

void
ui_tag_set_message(UI_TAG_Handle* handle,
                   MESSENGER_Application *app,
                   const struct GNUNET_CHAT_Message *message)
{
  g_assert((handle) && (message));

  if (GNUNET_CHAT_KIND_TAG != GNUNET_CHAT_message_get_kind(message))
    return;

  const char *tag_value = GNUNET_CHAT_message_get_text(message);

  GString *label = g_string_new("#");
  g_string_append(label, tag_value);

  ui_label_set_text(handle->tag_label, label->str);
  g_string_free(label, TRUE);

  g_object_set_qdata(
    G_OBJECT(handle->tag_label),
    app->quarks.data,
    (gpointer) message
  );
}

void
ui_tag_delete(UI_TAG_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  g_free(handle);
}
