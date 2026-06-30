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
 * @file ui/new_account.h
 */

#ifndef UI_NEW_ACCOUNT_H_
#define UI_NEW_ACCOUNT_H_

#include "messenger.h"

typedef struct UI_NEW_ACCOUNT_Handle
{
  gchar *filename;

  GtkBuilder *builder;
  GtkDialog *dialog;

  HdyAvatar *account_avatar;
  GtkFileChooserButton *account_avatar_file;

  GtkEntry *account_entry;

  GtkButton *cancel_button;
  GtkButton *confirm_button;
} UI_NEW_ACCOUNT_Handle;

/**
 * Initializes a handle for the new account dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle New account dialog handle
 */
void
ui_new_account_dialog_init(MESSENGER_Application *app,
                           UI_NEW_ACCOUNT_Handle *handle);

/**
 * Triggers an update of a new account dialog handle
 * to process selected changes regarding the new 
 * account and finish account creation.
 *
 * @param app Messenger application
 * @param handle New account dialog handle
 */
void
ui_new_account_dialog_update(MESSENGER_Application *app,
                             UI_NEW_ACCOUNT_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given new account dialog handle.
 *
 * @param handle New account dialog handle
 */
void
ui_new_account_dialog_cleanup(UI_NEW_ACCOUNT_Handle *handle);

#endif /* UI_NEW_ACCOUNT_H_ */
