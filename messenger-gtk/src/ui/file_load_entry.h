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
 * @file ui/file_load_entry.h
 */

#ifndef UI_FILE_LOAD_ENTRY_H_
#define UI_FILE_LOAD_ENTRY_H_

#include <gnunet/gnunet_chat_lib.h>

#include "messenger.h"

typedef struct UI_CHAT_TITLE_Handle UI_CHAT_TITLE_Handle;

typedef struct UI_FILE_LOAD_ENTRY_Handle
{
  UI_CHAT_TITLE_Handle *chat_title;

  GtkBuilder *builder;

  GtkWidget *entry_box;

  GtkImage *file_image;
  GtkLabel *file_label;

  GtkProgressBar *load_progress_bar;

  GtkButton *cancel_button;
} UI_FILE_LOAD_ENTRY_Handle;

/**
 * Allocates and creates a new file load entry
 * handle to manage loading files for a given
 * messenger application.
 *
 * @param app Messenger application
 * @return New file load entry handle
 */
UI_FILE_LOAD_ENTRY_Handle*
ui_file_load_entry_new(MESSENGER_Application *app);

/**
 * Frees its resources and destroys a given file
 * load entry handle.
 *
 * @param handle File load entry handle
 */
void
ui_file_load_entry_delete(UI_FILE_LOAD_ENTRY_Handle *handle);

#endif /* UI_FILE_LOAD_ENTRY_H_ */
