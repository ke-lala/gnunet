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
 * @file ui/settings.h
 */

#ifndef UI_SETTINGS_H_
#define UI_SETTINGS_H_

#include "messenger.h"

typedef struct UI_SETTINGS_Handle
{
  GtkBuilder *builder;
  HdyPreferencesWindow *dialog;

  GtkSwitch *start_on_login_switch;
  GtkSwitch *run_in_background_switch;

  GtkSwitch *enable_notifications_switch;
  GtkSwitch *notification_sounds_switch;

  GtkLabel *blocked_label;
  GtkWidget *blocked_scrolled_window;
  GtkListBox *blocked_listbox;
  GtkSwitch *read_receipts_switch;

  GtkComboBox *auto_delete_combo_box;

  GtkSwitch *auto_accept_invitations_switch;
  GtkComboBox *delete_invitations_combo_box;
  GtkButton *delete_invitations_button;

  GtkSwitch *auto_accept_files_switch;
  GtkFileChooserButton *download_folder_button;
  GtkComboBox *delete_files_combo_box;
  GtkButton *show_files_button;
  GtkButton *delete_files_button;

  GtkComboBox *leave_chats_combo_box;
  GtkButton *leave_chats_button;

  gboolean open_files;
} UI_SETTINGS_Handle;

/**
 * Initializes a handle for the settings dialog
 * of a given messenger application.
 *
 * @param app Messenger application
 * @param handle Settings dialog handle
 */
void
ui_settings_dialog_init(MESSENGER_Application *app,
                        UI_SETTINGS_Handle *handle);

/**
 * Cleans up the allocated resources and resets the
 * state of a given settings dialog handle.
 *
 * @param handle Settings dialog handle
 */
void
ui_settings_dialog_cleanup(UI_SETTINGS_Handle *handle);

#endif /* UI_SETTINGS_H_ */
