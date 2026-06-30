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
 * @file about.c
 */

#include "about.h"

#include "../application.h"
#include "../ui.h"

static void
handle_close_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);

  GtkAboutDialog *dialog = GTK_ABOUT_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_about_dialog_cleanup((UI_ABOUT_Handle*) user_data);
}

void
ui_about_dialog_init(MESSENGER_Application *app,
                     UI_ABOUT_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/about.ui")
  );

  handle->dialog = GTK_ABOUT_DIALOG(
    gtk_builder_get_object(handle->builder, "about_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  gtk_about_dialog_set_program_name(
    handle->dialog,
    MESSENGER_APPLICATION_APPNAME
  );

  gtk_about_dialog_set_version(
    handle->dialog,
    MESSENGER_APPLICATION_VERSION
  );

  gtk_about_dialog_set_logo_icon_name(
    handle->dialog,
    MESSENGER_APPLICATION_ID
  );

  handle->close_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "close_button")
  );

  g_signal_connect(
    handle->close_button,
    "clicked",
    G_CALLBACK(handle_close_button_click),
    handle->dialog
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );
}

void
ui_about_dialog_cleanup(UI_ABOUT_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
