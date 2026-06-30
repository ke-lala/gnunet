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
 * @file ui/new_tag.h
 */

#ifndef UI_NEW_TAG_H_
#define UI_NEW_TAG_H_

#include "messenger.h"

typedef void
(*UI_NEW_TAG_Callback) (
  MESSENGER_Application *app,
	GList *selected,
	const char *tag,
  gpointer user_data
);

typedef struct UI_NEW_TAG_Handle
{
  GList *selected;
  gpointer user_data;

  UI_NEW_TAG_Callback callback;

  GtkBuilder *builder;
  GtkDialog *dialog;

  HdyAvatar *tag_avatar;
  GtkEntry *tag_entry;

  GtkButton *cancel_button;
  GtkButton *confirm_button;
} UI_NEW_TAG_Handle;

/**
 * Initializes a handle for the new tag dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle New tag dialog handle
 */
void
ui_new_tag_dialog_init(MESSENGER_Application *app,
                       UI_NEW_TAG_Handle *handle);

/**
 * Links a custom list and a callback to a
 * given new tag dialog which will be used 
 * to handle the event of tagging.
 *
 * @param handle New tag dialog handle
 * @param callback New tag callback
 * @param selected Selected messages
 * @param user_data User data
 */
void
ui_new_tag_dialog_link(UI_NEW_TAG_Handle *handle,
                       UI_NEW_TAG_Callback callback,
                       GList *selected,
                       gpointer user_data);

/**
 * Cleans up the allocated resources and resets the
 * state of a given new tag dialog handle.
 *
 * @param handle New tag dialog handle
 */
void
ui_new_tag_dialog_cleanup(UI_NEW_TAG_Handle *handle);

#endif /* UI_NEW_TAG_H_ */
