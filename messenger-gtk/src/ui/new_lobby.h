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
 * @file ui/new_lobby.h
 */

#ifndef UI_NEW_LOBBY_H_
#define UI_NEW_LOBBY_H_

#include "messenger.h"

#include <cairo/cairo.h>
#include <gdk/gdkpixbuf.h>
#include <qrencode.h>

typedef struct UI_NEW_LOBBY_Handle
{
  GtkBuilder *builder;
  GtkDialog *dialog;

  GtkInfoBar *warning_info_bar;

  GtkStack *stack;
  GtkWidget *generate_box;
  GtkWidget *copy_box;

  GtkComboBox *expiration_combo_box;

  GtkStack *preview_stack;
  GtkWidget *fail_box;

  GtkSpinner *loading_spinner;

  GtkDrawingArea *id_drawing_area;
  GtkEntry *id_entry;

  gulong id_draw_signal;

  GtkButton *cancel_button;
  GtkButton *generate_button;
  GtkButton *copy_button;

  QRcode *qr;
} UI_NEW_LOBBY_Handle;

/**
 * Initializes a handle for the new lobby dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle New lobby dialog handle
 */
void
ui_new_lobby_dialog_init(MESSENGER_Application *app,
                         UI_NEW_LOBBY_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given new lobby dialog handle.
 *
 * @param handle New lobby dialog handle
 */
void
ui_new_lobby_dialog_cleanup(UI_NEW_LOBBY_Handle *handle);

#endif /* UI_NEW_LOBBY_H_ */
