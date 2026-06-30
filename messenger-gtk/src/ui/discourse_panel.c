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
 * @file ui/discourse_panel.c
 */

#include "discourse_panel.h"

#include "../contact.h"
#include "../ui.h"

UI_DISCOURSE_PANEL_Handle*
ui_discourse_panel_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_DISCOURSE_PANEL_Handle* handle = g_malloc(sizeof(UI_DISCOURSE_PANEL_Handle));

  handle->contact = NULL;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/discourse_panel.ui")
  );

  handle->panel_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "panel_box")
  );

  handle->panel_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "panel_stack")
  );

  handle->avatar_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "avatar_box")
  );

  handle->panel_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "panel_avatar")
  );

  handle->panel_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "panel_label")
  );

  handle->video_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "video_box")
  );

  return handle;
}

void
ui_discourse_panel_set_contact(UI_DISCOURSE_PANEL_Handle* handle,
                               const struct GNUNET_CHAT_Contact *contact)
{
  g_assert(handle);

  if (handle->contact)
  {
    contact_remove_name_avatar_from_info(handle->contact, handle->panel_avatar);
    contact_remove_name_label_from_info(handle->contact, handle->panel_label);
  }

  if (contact)
  {
    contact_add_name_avatar_to_info(contact, handle->panel_avatar);
    contact_add_name_label_to_info(contact, handle->panel_label);
  }

  handle->contact = contact;
}

void
ui_discourse_panel_delete(UI_DISCOURSE_PANEL_Handle *handle)
{
  g_assert(handle);

  ui_discourse_panel_set_contact(handle, NULL);
  
  g_object_unref(handle->builder);

  g_free(handle);
}
