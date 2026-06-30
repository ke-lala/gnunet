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
 * @file ui/contact_entry.h
 */

#ifndef UI_CONTACT_ENTRY_H_
#define UI_CONTACT_ENTRY_H_

#include "messenger.h"

#include <gnunet/gnunet_chat_lib.h>

typedef struct UI_CONTACT_ENTRY_Handle
{
  struct GNUNET_CHAT_Contact *contact;

  GtkBuilder *builder;

  GtkWidget *entry_box;
  HdyAvatar *entry_avatar;

  GtkLabel *title_label;
  GtkLabel *subtitle_label;
} UI_CONTACT_ENTRY_Handle;

/**
 * Allocates and creates a new contact entry
 * handle to manage a contact in a list for
 * a given messenger application.
 *
 * @param app Messenger application
 * @return New contact entry handle
 */
UI_CONTACT_ENTRY_Handle*
ui_contact_entry_new(MESSENGER_Application *app);

/**
 * Sets the content of the given contact entry
 * handle respectively to visually represent a
 * selected chat contact.
 *
 * @param handle Contact entry handle
 * @param contact Chat contact
 */
void
ui_contact_entry_set_contact(UI_CONTACT_ENTRY_Handle* handle,
                             struct GNUNET_CHAT_Contact *contact);

/**
 * Frees its resources and destroys a given
 * contact entry handle.
 *
 * @param handle Contact entry handle
 */
void
ui_contact_entry_delete(UI_CONTACT_ENTRY_Handle *handle);

#endif /* UI_CONTACT_ENTRY_H_ */
