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
 * @file contact.c
 */

#include "contact.h"

#include "ui.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>
#include <string.h>

enum GNUNET_GenericReturnValue
contact_create_info(struct GNUNET_CHAT_Contact *contact)
{
  if ((!contact) || (GNUNET_CHAT_contact_get_user_pointer(contact)))
    return GNUNET_NO;

  MESSENGER_ContactInfo* info = g_malloc(sizeof(MESSENGER_ContactInfo));

  info->last_message = NULL;
  info->icon_file = NULL;
  info->icon = NULL;
  info->task = 0;

  info->name_labels = NULL;
  info->name_avatars = NULL;
  info->visible_widgets = NULL;

  GNUNET_CHAT_contact_set_user_pointer(contact, info);
  return GNUNET_YES;
}

void
contact_destroy_info(struct GNUNET_CHAT_Contact *contact)
{
  g_assert(contact);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  if (info->name_labels)
    g_list_free(info->name_labels);

  if (info->name_avatars)
    g_list_free(info->name_avatars);

  if (info->visible_widgets)
    g_list_free(info->visible_widgets);

  if (info->task)
    util_source_remove(info->task);

  if (info->icon)
    g_object_unref(info->icon);

  if (info->icon_file)
    g_object_unref(info->icon_file);

  g_free(info);

  GNUNET_CHAT_contact_set_user_pointer(contact, NULL);
}

void
contact_set_last_message_to_info(const struct GNUNET_CHAT_Contact *contact,
				                         void *message)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  info->last_message = message;
}

void*
contact_get_last_message_from_info(const struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return NULL;

  return info->last_message;
}

void
contact_add_name_label_to_info(const struct GNUNET_CHAT_Contact *contact,
			                         GtkLabel *label)
{
  g_assert(label);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  const char *name = GNUNET_CHAT_contact_get_name(contact);

  ui_label_set_text(label, name);

  info->name_labels = g_list_append(info->name_labels, label);
}

void
contact_remove_name_label_from_info(const struct GNUNET_CHAT_Contact *contact,
			                              GtkLabel *label)
{
  g_assert(label);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  if (info->name_labels)
    info->name_labels = g_list_remove(info->name_labels, label);
}

void
contact_add_name_avatar_to_info(const struct GNUNET_CHAT_Contact *contact,
			                          HdyAvatar *avatar)
{
  g_assert(avatar);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  const char *name = GNUNET_CHAT_contact_get_name(contact);

  ui_avatar_set_text(avatar, name);
  ui_avatar_set_icon(avatar, info->icon);

  info->name_avatars = g_list_append(info->name_avatars, avatar);
}

void
contact_remove_name_avatar_from_info(const struct GNUNET_CHAT_Contact *contact,
			                               HdyAvatar *avatar)
{
  g_assert(avatar);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  if (info->name_avatars)
    info->name_avatars = g_list_remove(info->name_avatars, avatar);
}

void
contact_add_visible_widget_to_info(const struct GNUNET_CHAT_Contact *contact,
                                   GtkWidget *widget)
{
  g_assert(widget);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  gboolean visible = (GNUNET_YES != GNUNET_CHAT_contact_is_blocked(contact));

  gtk_widget_set_visible(widget, visible);

  info->visible_widgets = g_list_append(info->visible_widgets, widget);
}

void
contact_remove_visible_widget_to_info(const struct GNUNET_CHAT_Contact *contact,
                                      GtkWidget *widget)
{
  g_assert(widget);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;
  
  if (info->visible_widgets)
    info->visible_widgets = g_list_remove(info->visible_widgets, widget);
}

void
contact_update_info(const struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  GList* list;
  const char *name = GNUNET_CHAT_contact_get_name(contact);

  gboolean visible = (GNUNET_YES != GNUNET_CHAT_contact_is_blocked(contact));

  for (list = info->name_labels; list; list = list->next)
    ui_label_set_text(GTK_LABEL(list->data), name);

  for (list = info->name_avatars; list; list = list->next)
    ui_avatar_set_text(HDY_AVATAR(list->data), name);

  for (list = info->name_avatars; list; list = list->next)
    ui_avatar_set_icon(HDY_AVATAR(list->data), info->icon);

  for (list = info->visible_widgets; list; list = list->next)
    gtk_widget_set_visible(GTK_WIDGET(list->data), visible);
}

static gboolean
_task_update_avatars(gpointer data)
{
  g_assert(data);

  MESSENGER_ContactInfo *info = (MESSENGER_ContactInfo*) data;

  info->task = 0;

  GList* list;
  for (list = info->name_avatars; list; list = list->next)
    ui_avatar_set_icon(HDY_AVATAR(list->data), info->icon);

  return FALSE;
}

static void
_info_profile_downloaded(void *cls,
                         struct GNUNET_CHAT_File *file,
                         uint64_t completed,
                         uint64_t size)
{
  g_assert((cls) && (file));

  MESSENGER_ContactInfo* info = (MESSENGER_ContactInfo*) cls;

  if (completed < size)
    return;

  const char *preview = GNUNET_CHAT_file_open_preview(file);

  if (!preview)
    return;

  GFile *file_object = g_file_new_for_path(preview);

  if (!file_object)
    return;

  if (!(info->icon_file))
    goto skip_comparison;

  if (g_file_equal(info->icon_file, file_object))
  {
    g_object_unref(file_object);
    return;
  }

  g_object_unref(info->icon_file);

skip_comparison:
  info->icon_file = file_object;

  if (info->icon)
    g_object_unref(info->icon);

  info->icon = g_file_icon_new(file_object);

  if (!(info->task))
    info->task = util_idle_add(G_SOURCE_FUNC(_task_update_avatars), info);
}

static enum GNUNET_GenericReturnValue
_info_iterate_attribute(MESSENGER_ContactInfo* info,
                        struct GNUNET_CHAT_Handle *handle,
                        struct GNUNET_CHAT_Contact *contact,
                        const char *name,
                        const char *value)
{
  g_assert((info) && (handle) && (contact) && (name));

  if ((0 != strcmp(name, GNUNET_CHAT_ATTRIBUTE_AVATAR)) || (!value))
    return GNUNET_YES;

  struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_uri_parse(value, NULL);

  if (!uri)
    return GNUNET_YES;

  struct GNUNET_CHAT_File *file = GNUNET_CHAT_request_file(handle, uri);

  if (!file)
    goto skip_file;

  if (GNUNET_YES == GNUNET_CHAT_file_is_ready(file))
    _info_profile_downloaded(
      info,
      file,
      GNUNET_CHAT_file_get_local_size(file),
      GNUNET_CHAT_file_get_size(file)
    );
  else if (GNUNET_YES != GNUNET_CHAT_file_is_downloading(file))
    GNUNET_CHAT_file_start_download(
      file,
      _info_profile_downloaded,
      info
    );

skip_file:
  GNUNET_CHAT_uri_destroy(uri);
  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
_handle_iterate_attribute(void *cls,
                          struct GNUNET_CHAT_Handle *handle,
                          const char *name,
                          const char *value)
{
  g_assert((cls) && (handle) && (name));

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) cls;

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return GNUNET_NO;

  return _info_iterate_attribute(
    info,
    handle,
    contact,
    name,
    value
  );
}

static enum GNUNET_GenericReturnValue
_contact_iterate_attribute(void *cls,
                           struct GNUNET_CHAT_Contact *contact,
                           const char *name,
                           const char *value)
{
  g_assert((cls) && (contact) && (name));

  struct GNUNET_CHAT_Handle *handle = (struct GNUNET_CHAT_Handle*) cls;

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return GNUNET_NO;

  return _info_iterate_attribute(
    info,
    handle,
    contact,
    name,
    value
  );
}

void
contact_update_attributes(struct GNUNET_CHAT_Contact *contact,
                          MESSENGER_Application *app)
{
  g_assert(app);

  MESSENGER_ContactInfo* info = GNUNET_CHAT_contact_get_user_pointer(contact);

  if (!info)
    return;

  if (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
    GNUNET_CHAT_get_attributes(
      app->chat.messenger.handle,
      _handle_iterate_attribute,
      contact
    );
  else
    GNUNET_CHAT_contact_get_attributes(
      contact,
      _contact_iterate_attribute,
      app->chat.messenger.handle
    );
}
