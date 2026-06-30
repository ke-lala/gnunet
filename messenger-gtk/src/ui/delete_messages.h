/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file ui/delete_messages.h
 */

#ifndef UI_DELETE_MESSAGES_H_
#define UI_DELETE_MESSAGES_H_

#include "messenger.h"

typedef void
(*UI_DELETE_MESSAGES_Callback) (
  MESSENGER_Application *app,
	GList *selected,
	gulong delay
);

typedef struct UI_DELETE_MESSAGES_Handle
{
  GList *selected;

  UI_DELETE_MESSAGES_Callback callback;

  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkListStore *delay_store;

  GtkComboBox *delay_combobox;
  GtkCheckButton *hide_checkbox;

  GtkButton *cancel_button;
  GtkButton *confirm_button;
} UI_DELETE_MESSAGES_Handle;

/**
 * Initializes a handle for the delete messages
 * dialog of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Delete messages dialog handle
 */
void
ui_delete_messages_dialog_init(MESSENGER_Application *app,
                               UI_DELETE_MESSAGES_Handle *handle);

/**
 * Links a custom list and a callback to a
 * given delete messages dialog which will be
 * used to handle the event of deletion.
 *
 * @param handle Delete messages dialog handle
 * @param callback Delete messages callback
 * @param selected Selected messages
 */
void
ui_delete_messages_dialog_link(UI_DELETE_MESSAGES_Handle *handle,
                               UI_DELETE_MESSAGES_Callback callback,
                               GList *selected);

/**
 * Cleans up the allocated resources and resets the
 * state of a given delete messages dialog handle.
 *
 * @param handle Delete messages dialog handle
 */
void
ui_delete_messages_dialog_cleanup(UI_DELETE_MESSAGES_Handle *handle);

#endif /* UI_DELETE_MESSAGES_H_ */
