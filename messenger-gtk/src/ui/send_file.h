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
 * @file ui/send_file.h
 */

#ifndef UI_SEND_FILE_H_
#define UI_SEND_FILE_H_

#include "messenger.h"

typedef struct UI_SEND_FILE_Handle
{
  GList *contact_entries;

  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkDrawingArea *file_drawing_area;
  GtkFileChooserButton *file_chooser_button;

  gulong file_draw_signal;

  GtkButton *cancel_button;
  GtkButton *send_button;

  GdkPixbuf *image;
  GdkPixbufAnimation *animation;
  GdkPixbufAnimationIter *animation_iter;

  guint redraw_animation;
} UI_SEND_FILE_Handle;

/**
 * Initializes a handle for the send file dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Send file dialog handle
 */
void
ui_send_file_dialog_init(MESSENGER_Application *app,
                         UI_SEND_FILE_Handle *handle);

/**
 * Updates a given send file dialog handle with
 * a certain filename to pre-determine which file
 * gets selected by the dialog as default.
 *
 * @param handle Send file dialog handle
 * @param filename Custom filename
 */
void
ui_send_file_dialog_update(UI_SEND_FILE_Handle *handle,
                           const gchar *filename);

/**
 * Cleans up the allocated resources and resets the
 * state of a given send file dialog handle.
 *
 * @param handle Send file dialog handle
 */
void
ui_send_file_dialog_cleanup(UI_SEND_FILE_Handle *handle);

#endif /* UI_SEND_FILE_H_ */
