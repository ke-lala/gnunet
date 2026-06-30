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
 * @file ui/settings.c
 */

#include "settings.h"

#include "../application.h"
#include "../request.h"
#include "../ui.h"

#include "contact_entry.h"
#include "files.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal/background.h>
#endif

static gboolean
handle_general_switch_state(UNUSED GtkSwitch *widget,
                            gboolean state,
                            gpointer user_data)
{
  g_assert(user_data);

  gboolean *setting = (gboolean*) user_data;
  *setting = state;
  return FALSE;
}

static void
_request_background_callback(MESSENGER_Application *app,
                             gboolean success,
                             gboolean error,
                             gpointer user_data)
{
  g_assert((app) && (user_data));

  GtkSwitch *widget = GTK_SWITCH(user_data);

  if (error) {
    gtk_widget_set_sensitive(GTK_WIDGET(widget), !success);
    gtk_switch_set_active(widget, success);
    return;
  }
  
  gboolean *setting = (gboolean*) (
    g_object_get_qdata(G_OBJECT(widget), app->quarks.data)
  );

  handle_general_switch_state(widget, success, setting);
}

static gboolean
handle_background_switch_state(GtkSwitch *widget,
			                         gboolean state,
			                         gpointer user_data)
{
  g_assert((widget) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  gboolean *setting = (gboolean*) (
    g_object_get_qdata(G_OBJECT(widget), app->quarks.data)
  );

  if ((!state) || (!gtk_widget_is_sensitive(GTK_WIDGET(widget))))
    return handle_general_switch_state(widget, state, setting);

  XdpBackgroundFlags flags = XDP_BACKGROUND_FLAG_NONE;

  if (&(app->settings.autostart) == setting)
    flags |= XDP_BACKGROUND_FLAG_AUTOSTART;
  if (&(app->settings.background_task) == setting)
    flags |= XDP_BACKGROUND_FLAG_ACTIVATABLE;

  request_new_background(
    app,
    flags,
    _request_background_callback,
    widget
  );

  gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
  return FALSE;
}

static gboolean
handle_inverted_switch_state(GtkSwitch *widget,
                             gboolean state,
                             gpointer user_data)
{
  g_assert((widget) && (user_data));

  return handle_general_switch_state(widget, !state, user_data);
}

static void
handle_general_combo_box_change(GtkComboBox *widget,
				                        gpointer user_data)
{
  g_assert((widget) && (user_data));

  gulong *delay = (gulong*) user_data;
  GtkTreeModel *model = gtk_combo_box_get_model(widget);

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter(widget, &iter))
    gtk_tree_model_get(model, &iter, 1, delay, -1);
}

int
_leave_group_iteration(UNUSED void *cls,
                       UNUSED struct GNUNET_CHAT_Handle *handle,
                       struct GNUNET_CHAT_Group *group)
{
  g_assert(group);

  GNUNET_CHAT_group_leave(group);
  return GNUNET_YES;
}

int
_delete_contact_iteration(UNUSED void *cls,
                          UNUSED struct GNUNET_CHAT_Handle *handle,
                          struct GNUNET_CHAT_Contact *contact)
{
  g_assert(contact);

  GNUNET_CHAT_contact_delete(contact);
  return GNUNET_YES;
}

static void
handle_leave_chats_button_click(UNUSED GtkButton* button,
				                        gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  application_chat_lock(app);

  GNUNET_CHAT_iterate_groups(
    app->chat.messenger.handle,
    _leave_group_iteration,
    NULL
  );

  GNUNET_CHAT_iterate_contacts(
    app->chat.messenger.handle,
    _delete_contact_iteration,
    NULL
  );

  application_chat_unlock(app);
}

static void
handle_show_files_button_click(UNUSED GtkButton* button,
				                       gpointer user_data)
{
  g_assert(user_data);

  UI_SETTINGS_Handle *handle = (UI_SETTINGS_Handle*) user_data;

  handle->open_files = true;

  gtk_window_close(GTK_WINDOW(handle->dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;
  UI_SETTINGS_Handle *handle = &(app->ui.settings);

  if (handle->open_files)
  {
    ui_files_dialog_init(app, &(app->ui.files));
    gtk_widget_show(GTK_WIDGET(app->ui.files.dialog));
  }

  ui_settings_dialog_cleanup(handle);
}

static void
_set_combobox_to_active_by_delay(GtkComboBox *widget,
                                 gulong delay)
{
  g_assert(widget);

  GtkTreeModel *model = gtk_combo_box_get_model(widget);

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_first(model, &iter))
    return;

  gulong value;

  do {
    gtk_tree_model_get(model, &iter, 1, &value, -1);

    if (value == delay)
      goto set_active;

  } while (gtk_tree_model_iter_next(model, &iter));

  return;
set_active:
  gtk_combo_box_set_active_iter(widget, &iter);
}

static enum GNUNET_GenericReturnValue
_count_blocked_contacts(void *cls,
                        UNUSED struct GNUNET_CHAT_Handle *handle,
                        struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  if (GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact))
    return GNUNET_YES;

  guint *count = (guint*) cls;

  if (GNUNET_YES == GNUNET_CHAT_contact_is_blocked(contact))
    *count = (*count) + 1;

  return GNUNET_YES;
}

static enum GNUNET_GenericReturnValue
_iterate_blocked_contacts(void *cls,
                        UNUSED struct GNUNET_CHAT_Handle *handle,
                        struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (contact));

  if ((GNUNET_YES == GNUNET_CHAT_contact_is_owned(contact)) ||
      (GNUNET_YES != GNUNET_CHAT_contact_is_blocked(contact)))
    return GNUNET_YES;

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  UI_CONTACT_ENTRY_Handle *entry = ui_contact_entry_new(app);
  ui_contact_entry_set_contact(entry, contact);

  gtk_list_box_prepend(
    app->ui.settings.blocked_listbox,
    entry->entry_box
  );

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), app->quarks.data, contact);

  g_object_set_qdata_full(
    G_OBJECT(row),
    app->quarks.ui,
    entry,
    (GDestroyNotify) ui_contact_entry_delete
  );

  return GNUNET_YES;
}

void
ui_settings_dialog_init(MESSENGER_Application *app,
                        UI_SETTINGS_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/settings.ui")
  );

  handle->dialog = HDY_PREFERENCES_WINDOW(
    gtk_builder_get_object(handle->builder, "settings_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->start_on_login_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "start_on_login_switch")
  );

  gtk_switch_set_active(
    handle->start_on_login_switch,
    app->settings.autostart
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->start_on_login_switch),
    !(app->settings.autostart)
  );

  g_object_set_qdata(
    G_OBJECT(handle->start_on_login_switch),
    app->quarks.data,
    &(app->settings.autostart)
  );

  g_signal_connect(
    handle->start_on_login_switch,
    "state-set",
    G_CALLBACK(handle_background_switch_state),
    app
  );

  handle->run_in_background_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "run_in_background_switch")
  );

  gtk_switch_set_active(
    handle->run_in_background_switch,
    app->settings.background_task
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->run_in_background_switch),
    !(app->settings.background_task)
  );

  g_object_set_qdata(
    G_OBJECT(handle->run_in_background_switch),
    app->quarks.data,
    &(app->settings.background_task)
  );

  g_signal_connect(
    handle->run_in_background_switch,
    "state-set",
    G_CALLBACK(handle_background_switch_state),
    app
  );

  handle->enable_notifications_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "enable_notifications_switch")
  );

  handle->notification_sounds_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "notification_sounds_switch")
  );

  gtk_switch_set_active(
    handle->enable_notifications_switch,
    !(app->settings.disable_notifications)
  );

  gtk_switch_set_active(
    handle->notification_sounds_switch,
    app->settings.play_notification_sounds
  );

  g_signal_connect(
    handle->enable_notifications_switch,
    "state-set",
    G_CALLBACK(handle_inverted_switch_state),
    &(app->settings.disable_notifications)
  );

  g_signal_connect(
    handle->notification_sounds_switch,
    "state-set",
    G_CALLBACK(handle_general_switch_state),
    &(app->settings.play_notification_sounds)
  );

  handle->blocked_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "blocked_label")
  );

  handle->blocked_scrolled_window = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "blocked_scrolled_window")
  );

  handle->blocked_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "blocked_listbox")
  );

  guint blocked_count = 0;
  GNUNET_CHAT_iterate_contacts(
    app->chat.messenger.handle,
    _count_blocked_contacts,
    &blocked_count
  );

  gtk_widget_set_size_request(
    handle->blocked_scrolled_window,
    0,
    56 * (blocked_count > 3? 3 : blocked_count)
  );

  gtk_widget_set_visible(handle->blocked_scrolled_window, blocked_count > 0);

  GString *blocked_text = g_string_new(NULL);
  if (blocked_text)
  {
    g_string_printf(
      blocked_text,
      _("%u blocked contacts"),
      blocked_count
    );

    gtk_label_set_text(
      handle->blocked_label,
      blocked_text->str
    );

    g_string_free(blocked_text, TRUE);
  }

  GNUNET_CHAT_iterate_contacts(
    app->chat.messenger.handle,
    _iterate_blocked_contacts,
    app
  );

  gtk_list_box_invalidate_filter(handle->blocked_listbox);

  handle->read_receipts_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "read_receipts_switch")
  );

  gtk_switch_set_active(
    handle->read_receipts_switch,
    app->settings.send_read_receipts
  );

  g_signal_connect(
    handle->read_receipts_switch,
    "state-set",
    G_CALLBACK(handle_general_switch_state),
    &(app->settings.send_read_receipts)
  );

  handle->auto_delete_combo_box = GTK_COMBO_BOX(
    gtk_builder_get_object(handle->builder, "auto_delete_combo_box")
  );

  _set_combobox_to_active_by_delay(
    handle->auto_delete_combo_box,
    app->settings.auto_delete_delay
  );

  g_signal_connect(
    handle->auto_delete_combo_box,
    "changed",
    G_CALLBACK(handle_general_combo_box_change),
    &(app->settings.auto_delete_delay)
  );

  handle->auto_accept_invitations_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "auto_accept_invitations_switch")
  );

  gtk_switch_set_active(
    handle->auto_accept_invitations_switch,
    app->settings.accept_all_invitations
  );

  g_signal_connect(
    handle->auto_accept_invitations_switch,
    "state-set",
    G_CALLBACK(handle_general_switch_state),
    &(app->settings.accept_all_invitations)
  );

  handle->delete_invitations_combo_box = GTK_COMBO_BOX(
    gtk_builder_get_object(handle->builder, "delete_invitations_combo_box")
  );

  _set_combobox_to_active_by_delay(
    handle->delete_invitations_combo_box,
    app->settings.delete_invitations_delay
  );

  g_signal_connect(
    handle->delete_invitations_combo_box,
    "changed",
    G_CALLBACK(handle_general_combo_box_change),
    &(app->settings.delete_invitations_delay)
  );

  handle->delete_invitations_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "delete_invitations_button")
  );

  handle->auto_accept_files_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "auto_accept_files_switch")
  );

  gtk_switch_set_active(
    handle->auto_accept_files_switch,
    app->settings.accept_all_files
  );

  g_signal_connect(
    handle->auto_accept_files_switch,
    "state-set",
    G_CALLBACK(handle_general_switch_state),
    &(app->settings.accept_all_files)
  );

  handle->download_folder_button = GTK_FILE_CHOOSER_BUTTON(
    gtk_builder_get_object(handle->builder, "download_folder_button")
  );

  handle->delete_files_combo_box = GTK_COMBO_BOX(
    gtk_builder_get_object(handle->builder, "delete_files_combo_box")
  );

  _set_combobox_to_active_by_delay(
    handle->delete_files_combo_box,
    app->settings.delete_files_delay
  );

  g_signal_connect(
    handle->delete_files_combo_box,
    "changed",
    G_CALLBACK(handle_general_combo_box_change),
    &(app->settings.delete_files_delay)
  );

  handle->show_files_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "show_files_button")
  );

  g_signal_connect(
    handle->show_files_button,
    "clicked",
    G_CALLBACK(handle_show_files_button_click),
    handle
  );

  handle->delete_files_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "delete_files_button")
  );

  handle->leave_chats_combo_box = GTK_COMBO_BOX(
    gtk_builder_get_object(handle->builder, "leave_chats_combo_box")
  );

  _set_combobox_to_active_by_delay(
    handle->leave_chats_combo_box,
    app->settings.leave_chats_delay
  );

  g_signal_connect(
    handle->leave_chats_combo_box,
    "changed",
    G_CALLBACK(handle_general_combo_box_change),
    &(app->settings.leave_chats_delay)
  );

  handle->leave_chats_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "leave_chats_button")
  );

  g_signal_connect(
    handle->leave_chats_button,
    "clicked",
    G_CALLBACK(handle_leave_chats_button_click),
    app
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    app
  );
}

void
ui_settings_dialog_cleanup(UI_SETTINGS_Handle *handle)
{
  g_assert(handle);

  if (handle->builder)
    g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
