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
 * @file ui/files.h
 */

#ifndef UI_FILES_H_
#define UI_FILES_H_

#include "messenger.h"

typedef struct UI_FILES_Handle
{
  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkStack *dialog_stack;
  GtkWidget *list_box;
  GtkWidget *info_box;

  GtkSearchEntry *file_search_entry;
  GtkListBox *files_listbox;

  GtkLabel *name_label;
  GtkProgressBar *storage_progress_bar;
  GtkButton *delete_file_button;
  GtkButton *play_pause_button;

  GtkStack *play_icon_stack;
  GtkWidget *play_icon_image;
  GtkWidget *pause_icon_image;

  GtkButton *back_button;
  GtkButton *close_button;
} UI_FILES_Handle;

/**
 * Initializes a handle for the files dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Files dialog handle
 */
void
ui_files_dialog_init(MESSENGER_Application *app,
                     UI_FILES_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given files dialog handle.
 *
 * @param handle Files dialog handle
 */
void
ui_files_dialog_cleanup(UI_FILES_Handle *handle);

#endif /* UI_FILES_H_ */
