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
 * @file account.c
 */

#include "account.h"

#include "contact.h"
#include "ui.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>
#include <string.h>

static GList *infos = NULL;

enum GNUNET_GenericReturnValue
account_create_info(struct GNUNET_CHAT_Account *account)
{
  if ((!account) || (GNUNET_CHAT_account_get_user_pointer(account)))
    return GNUNET_NO;

  MESSENGER_AccountInfo *info = g_malloc(sizeof(MESSENGER_AccountInfo));

  info->account = account;

  info->icon_file = NULL;
  info->icon = NULL;
  info->task = 0;

  info->name_avatars = NULL;

  infos = g_list_append(infos, info);

  GNUNET_CHAT_account_set_user_pointer(account, info);
  return GNUNET_YES;
}

static void
_destroy_account_info(gpointer data)
{
  g_assert(data);

  MESSENGER_AccountInfo *info = (MESSENGER_AccountInfo*) data;

  if (info->name_avatars)
    g_list_free(info->name_avatars);

  if (info->task)
    util_source_remove(info->task);

  if (info->icon)
    g_object_unref(info->icon);

  if (info->icon_file)
    g_object_unref(info->icon_file);

  GNUNET_CHAT_account_set_user_pointer(info->account, NULL);

  g_free(info);
}

void
account_destroy_info(struct GNUNET_CHAT_Account *account)
{
  g_assert(account);

  MESSENGER_AccountInfo* info = GNUNET_CHAT_account_get_user_pointer(account);

  if (!info)
    return;

  if (infos)
    infos = g_list_remove(infos, info);

  _destroy_account_info(info);
}

void
account_cleanup_infos()
{
  if (!infos)
    return;

  g_list_free_full(infos, (GDestroyNotify) _destroy_account_info);

  infos = NULL;
}

void
account_add_name_avatar_to_info(const struct GNUNET_CHAT_Account *account,
                                HdyAvatar *avatar)
{
  g_assert(avatar);

  MESSENGER_AccountInfo *info = GNUNET_CHAT_account_get_user_pointer(account);

  if (!info)
    return;

  const char *name = GNUNET_CHAT_account_get_name(account);

  ui_avatar_set_text(avatar, name);
  ui_avatar_set_icon(avatar, info->icon);

  info->name_avatars = g_list_append(info->name_avatars, avatar);  
}

void
account_switch_name_avatar_to_info(const struct GNUNET_CHAT_Account *account,
                                   HdyAvatar *avatar)
{
  g_assert(avatar);

  MESSENGER_AccountInfo *info = GNUNET_CHAT_account_get_user_pointer(account);

  if (!info)
    return;

  if (g_list_find(info->name_avatars, avatar))
    return;

  GList *list = infos;
  while (list)
  {
    MESSENGER_AccountInfo *other = (MESSENGER_AccountInfo*) list->data;

    if (g_list_find(other->name_avatars, avatar))
    {
      account_remove_name_avatar_from_info(other->account, avatar);
      break;
    }

    list = g_list_next(list);
  }

  account_add_name_avatar_to_info(account, avatar);
}

void
account_remove_name_avatar_from_info(const struct GNUNET_CHAT_Account *account,
                                     HdyAvatar *avatar)
{
  g_assert(avatar);

  MESSENGER_AccountInfo *info = GNUNET_CHAT_account_get_user_pointer(account);

  if (!info)
    return;

  if (info->name_avatars)
    info->name_avatars = g_list_remove(info->name_avatars, avatar);
}

static gboolean
_task_update_avatars(gpointer data)
{
  g_assert(data);

  MESSENGER_AccountInfo *info = (MESSENGER_AccountInfo*) data;

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

  struct GNUNET_CHAT_Account *account = (struct GNUNET_CHAT_Account*) cls;

  MESSENGER_AccountInfo *info = GNUNET_CHAT_account_get_user_pointer(account);

  if ((!info) || (completed < size))
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
_account_iterate_attribute(void *cls,
                           struct GNUNET_CHAT_Account *account,
                           const char *name,
                           const char *value)
{
  g_assert((cls) && (account) && (name));

  struct GNUNET_CHAT_Handle *handle = (struct GNUNET_CHAT_Handle*) cls;

  MESSENGER_AccountInfo *info = GNUNET_CHAT_account_get_user_pointer(account);

  if (!info)
    return GNUNET_NO;

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
      info->account,
      file,
      GNUNET_CHAT_file_get_local_size(file),
      GNUNET_CHAT_file_get_size(file)
    );
  else if (GNUNET_YES != GNUNET_CHAT_file_is_downloading(file))
    GNUNET_CHAT_file_start_download(
      file,
      _info_profile_downloaded,
      info->account
    );

skip_file:
  GNUNET_CHAT_uri_destroy(uri);
  return GNUNET_YES;
}

void
account_update_attributes(const struct GNUNET_CHAT_Account *account,
                          MESSENGER_Application *app)
{
  g_assert(app);

  MESSENGER_AccountInfo *info = GNUNET_CHAT_account_get_user_pointer(account);

  if ((!info) || (!(info->account)))
    return;

  GNUNET_CHAT_account_get_attributes(
    app->chat.messenger.handle,
    info->account,
    _account_iterate_attribute,
    app->chat.messenger.handle
  );
}
