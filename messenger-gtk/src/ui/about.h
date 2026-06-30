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
 * @file about.h
 */

#ifndef UI_ABOUT_H_
#define UI_ABOUT_H_

#include "messenger.h"

typedef struct UI_ABOUT_Handle
{
  GtkBuilder *builder;
  GtkAboutDialog *dialog;

  GtkButton *close_button;
} UI_ABOUT_Handle;

/**
 * Initializes a handle for the about dialog of
 * a given messenger application.
 *
 * @param app Messenger application
 * @param handle About dialog handle
 */
void
ui_about_dialog_init(MESSENGER_Application *app,
                     UI_ABOUT_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given about dialog handle.
 *
 * @param handle About dialog handle
 */
void
ui_about_dialog_cleanup(UI_ABOUT_Handle *handle);

#endif /* UI_ABOUT_H_ */
