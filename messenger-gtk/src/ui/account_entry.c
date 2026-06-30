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
 * @file ui/account_entry.c
 */

#include "account_entry.h"

#include "../account.h"
#include "../application.h"
#include "../contact.h"
#include "../ui.h"

UI_ACCOUNT_ENTRY_Handle*
ui_account_entry_new(MESSENGER_Application *app)
{
  g_assert(app);

  UI_ACCOUNT_ENTRY_Handle* handle = g_malloc(sizeof(UI_ACCOUNT_ENTRY_Handle));

  handle->account = NULL;
  handle->contact = NULL;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/account_entry.ui")
  );

  handle->entry_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "entry_box")
  );

  handle->entry_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "entry_avatar")
  );

  handle->entry_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "entry_label")
  );

  return handle;
}

void
ui_account_entry_set_account(UI_ACCOUNT_ENTRY_Handle* handle,
                             const struct GNUNET_CHAT_Account *account)
{
  g_assert(handle);

  if (handle->account)
    account_remove_name_avatar_from_info(handle->account, handle->entry_avatar);

  if (account)
  {
    account_add_name_avatar_to_info(account, handle->entry_avatar);

    ui_label_set_text(handle->entry_label, GNUNET_CHAT_account_get_name(account));
  }

  handle->account = account;
}

void
ui_account_entry_set_contact(UI_ACCOUNT_ENTRY_Handle* handle,
                             const struct GNUNET_CHAT_Contact *contact)
{
  g_assert(handle);

  if (handle->contact)
  {
    contact_remove_name_avatar_from_info(handle->contact, handle->entry_avatar);
    contact_remove_name_label_from_info(handle->contact, handle->entry_label);
  }

  if (contact)
  {
    contact_add_name_avatar_to_info(contact, handle->entry_avatar);
    contact_add_name_label_to_info(contact, handle->entry_label);
  }

  handle->contact = contact;
}

void
ui_account_entry_delete(UI_ACCOUNT_ENTRY_Handle *handle)
{
  g_assert(handle);

  ui_account_entry_set_account(handle, NULL);
  ui_account_entry_set_contact(handle, NULL);
  
  g_object_unref(handle->builder);

  g_free(handle);
}
