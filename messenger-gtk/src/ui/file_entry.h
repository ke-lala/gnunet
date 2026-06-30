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
 * @file ui/file_entry.h
 */

#ifndef UI_FILE_ENTRY_H_
#define UI_FILE_ENTRY_H_

#include <gnunet/gnunet_chat_lib.h>

#include "messenger.h"

typedef struct UI_FILE_ENTRY_Handle
{
  GtkBuilder *builder;

  GtkWidget *entry_box;

  GtkImage *file_image;
  GtkLabel *name_label;
  GtkLabel *size_label;
} UI_FILE_ENTRY_Handle;

/**
 * Allocates and creates a new file entry handle
 * to manage loading files for a given messenger
 * application.
 *
 * @param app Messenger application
 * @return New file entry handle
 */
UI_FILE_ENTRY_Handle*
ui_file_entry_new(MESSENGER_Application *app);

/**
 * Updates a file entry handle with a selected
 * file to represent it visually.
 *
 * @param handle File entry handle
 * @param file Chat file
 */
void
ui_file_entry_update(UI_FILE_ENTRY_Handle *handle,
                     struct GNUNET_CHAT_File *file);

/**
 * Frees its resources and destroys a given file
 * entry handle.
 *
 * @param handle File entry handle
 */
void
ui_file_entry_delete(UI_FILE_ENTRY_Handle *handle);

#endif /* UI_FILE_ENTRY_H_ */
