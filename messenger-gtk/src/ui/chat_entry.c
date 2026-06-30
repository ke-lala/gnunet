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
 * @file ui/chat_entry.c
 */

#include "chat_entry.h"

#include "chat_title.h"
#include "message.h"

#include "../application.h"
#include "../contact.h"
#include "../discourse.h"
#include "../ui.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_chat_lib.h>

UI_CHAT_ENTRY_Handle*
ui_chat_entry_new(MESSENGER_Application *app,
                  struct GNUNET_CHAT_Context *context)
{
  g_assert((app) && (context));

  UI_CHAT_ENTRY_Handle* handle = g_malloc(sizeof(UI_CHAT_ENTRY_Handle));

  memset(handle, 0, sizeof(*handle));

  handle->timestamp = ((time_t) -1);
  handle->context = context;

  handle->chat = ui_chat_new(app, handle->context);
  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/chat_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->entry_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "entry_avatar")
  );

  handle->title_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "title_label")
  );

  handle->timestamp_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "timestamp_label")
  );

  handle->text_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "text_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
    gtk_builder_get_object(handle->builder, "read_receipt_image")
  );

  GNUNET_CHAT_context_set_user_pointer(
    handle->context,
    handle
  );

  return handle;
}

static void
_chat_entry_update_contact(UI_CHAT_ENTRY_Handle *handle,
                           MESSENGER_Application *app,
                           struct GNUNET_CHAT_Contact* contact)
{
  g_assert((handle) && (app));

  struct GNUNET_CHAT_Contact *prev = g_object_get_qdata(
    G_OBJECT(handle->entry_avatar),
    app->quarks.data
  );

  if (prev)
  {
    contact_remove_name_label_from_info(contact, handle->title_label);
    contact_remove_name_avatar_from_info(contact, handle->entry_avatar);
  }
  
  if (contact)
  {
    contact_add_name_label_to_info(contact, handle->title_label);
    contact_add_name_avatar_to_info(contact, handle->entry_avatar);
  }

  g_object_set_qdata(
    G_OBJECT(handle->entry_avatar),
    app->quarks.data,
    contact
  );
}

void
ui_chat_entry_update(UI_CHAT_ENTRY_Handle *handle,
		                 MESSENGER_Application *app)
{
  g_assert((handle) && (app));

  struct GNUNET_CHAT_Contact* contact;
  struct GNUNET_CHAT_Group* group;

  contact = GNUNET_CHAT_context_get_contact(handle->context);
  group = GNUNET_CHAT_context_get_group(handle->context);

  const char *icon = "action-unavailable-symbolic";

  _chat_entry_update_contact(handle, app, contact);

  if (contact)
    icon = "avatar-default-symbolic";
  else if (group)
  {
    const char *title = GNUNET_CHAT_group_get_name(group);

    if ((title) && ('#' == *title))
      icon = "network-wired-symbolic";
    else
      icon = "system-users-symbolic";

    ui_label_set_text(handle->title_label, title);
    ui_avatar_set_text(handle->entry_avatar, title);
  }

  hdy_avatar_set_icon_name(handle->entry_avatar, icon);

  if (!(handle->chat))
    return;

  ui_chat_update(handle->chat, app);

  GList *rows = gtk_container_get_children(
    GTK_CONTAINER(handle->chat->messages_listbox)
  );

  if (!rows)
    return;

  UI_MESSAGE_Handle *last_message = NULL;
  for (GList *row = rows; row; row = row->next)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row->data), app->quarks.ui
    );

    if (!message)
      continue;

    last_message = message;
  }

  g_list_free(rows);

  if (!last_message)
    return;

  handle->timestamp = last_message->timestamp;

  GDateTime *dt_now = g_date_time_new_now_local();
  GDateTime *dt_message = g_date_time_new_from_unix_local(
    (gint64) handle->timestamp
  );

  GTimeSpan span = g_date_time_difference(dt_now, dt_message);
  gchar *time = NULL;

  if (span > 7 * G_TIME_SPAN_DAY)
    time = g_date_time_format(dt_message, "%F");
  else if (span > 2 * G_TIME_SPAN_DAY)
    time = g_date_time_format(dt_message, "%A");
  else if (span > G_TIME_SPAN_DAY)
    time = g_date_time_format(dt_message, _("Yesterday"));
  else
    time = g_date_time_format(dt_message, "%R");

  g_date_time_unref(dt_now);
  g_date_time_unref(dt_message);

  const gchar *text = gtk_label_get_text(last_message->text_label);

  if (group)
  {
    GString *message_text = g_string_new(NULL);

    g_string_append_printf(
      message_text,
      "%s: %s",
      gtk_label_get_text(last_message->sender_label),
      text
    );

    gtk_label_set_text(handle->text_label, message_text->str);
    g_string_free(message_text, TRUE);
  }
  else
    gtk_label_set_text(handle->text_label, text);

  if (time)
  {
    gtk_label_set_text(handle->timestamp_label, time);
    g_free(time);
  }

  gtk_widget_set_visible(
    GTK_WIDGET(handle->read_receipt_image),
    last_message->read_receipt_image? gtk_widget_is_visible(
      GTK_WIDGET(last_message->read_receipt_image)
    ) : FALSE
  );

  gtk_list_box_invalidate_sort(app->ui.messenger.chats_listbox);
}

static enum GNUNET_GenericReturnValue
_ui_chat_entry_delete_discourses (void *cls,
                                  UNUSED struct GNUNET_CHAT_Context *context,
                                  struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert(discourse);

  if (GNUNET_YES == GNUNET_CHAT_discourse_is_open(discourse))
    GNUNET_CHAT_discourse_close(discourse);

  discourse_destroy_info(discourse);
  return GNUNET_YES;
}

void
ui_chat_entry_delete(UI_CHAT_ENTRY_Handle *handle)
{
  g_assert(handle);

  ui_chat_delete(handle->chat);

  GNUNET_CHAT_context_iterate_discourses(
    handle->context,
    _ui_chat_entry_delete_discourses,
    NULL
  );

  g_object_unref(handle->builder);

  if (handle->update)
    util_source_remove(handle->update);

  if (handle->context)
    GNUNET_CHAT_context_set_user_pointer(handle->context, NULL);

  g_free(handle);
}

void
ui_chat_entry_dispose(UI_CHAT_ENTRY_Handle *handle,
		                  MESSENGER_Application *app)
{
  g_assert((handle) && (handle->entry_box));

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  util_source_remove_by_data(handle);

  ui->chat_entries = g_list_remove(ui->chat_entries, handle);

  gtk_container_remove(
    GTK_CONTAINER(ui->chats_listbox),
    gtk_widget_get_parent(handle->entry_box)
  );

  _chat_entry_update_contact(handle, app, NULL);

  gtk_container_remove(
    GTK_CONTAINER(ui->chat_title_stack),
    handle->chat->title->chat_title_box
  );

  gtk_container_remove(
    GTK_CONTAINER(ui->chats_stack),
    handle->chat->chat_box
  );

  ui_chat_entry_delete(handle);
}
