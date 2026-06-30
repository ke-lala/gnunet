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
 * @file ui/contacts.h
 */

#ifndef UI_CONTACTS_H_
#define UI_CONTACTS_H_

#include "messenger.h"

typedef struct UI_CONTACTS_Handle
{
  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkSearchEntry *contact_search_entry;

  GtkListBox *contacts_listbox;

  GtkButton *close_button;
} UI_CONTACTS_Handle;

/**
 * Initializes a handle for the contacts dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Contacts dialog handle
 */
void
ui_contacts_dialog_init(MESSENGER_Application *app,
                        UI_CONTACTS_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given contacts dialog handle.
 *
 * @param handle Contacts dialog handle
 */
void
ui_contacts_dialog_cleanup(UI_CONTACTS_Handle *handle);

#endif /* UI_CONTACTS_H_ */
