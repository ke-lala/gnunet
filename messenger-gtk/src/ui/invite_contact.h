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
 * @file ui/invite_contact.h
 */

#ifndef UI_INVITE_CONTACT_H_
#define UI_INVITE_CONTACT_H_

#include "messenger.h"

typedef struct UI_INVITE_CONTACT_Handle
{
  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkSearchEntry *contact_search_entry;

  GtkListBox *contacts_listbox;

  GtkButton *close_button;
} UI_INVITE_CONTACT_Handle;

/**
 * Initializes a handle for the invite contact dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Invite contact dialog handle
 */
void
ui_invite_contact_dialog_init(MESSENGER_Application *app,
                              UI_INVITE_CONTACT_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given invite contact dialog handle.
 *
 * @param handle Invite contact dialog handle
 */
void
ui_invite_contact_dialog_cleanup(UI_INVITE_CONTACT_Handle *handle);

#endif /* UI_INVITE_CONTACT_H_ */
