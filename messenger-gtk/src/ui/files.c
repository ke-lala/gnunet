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
 * @file ui/files.c
 */

#include "files.h"

#include "file_entry.h"
#include "../application.h"
#include "../ui.h"
#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>

static void
handle_back_button_click(GtkButton *button,
                         gpointer user_data)
{
  g_assert(user_data);

  UI_FILES_Handle *handle = (UI_FILES_Handle*) user_data;
  
  gtk_stack_set_visible_child(handle->dialog_stack, handle->list_box);
  gtk_widget_set_visible(GTK_WIDGET(button), false);
}

static void
handle_close_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);

  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_files_listbox_row_activated(UNUSED GtkListBox* listbox,
                                   GtkListBoxRow* row,
                                   gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
    return;

  UI_FILES_Handle *handle = &(app->ui.files);

  struct GNUNET_CHAT_File *file = (struct GNUNET_CHAT_File*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.data)
  );

  application_chat_lock(app);

  const gdouble progress = (
    (gdouble) GNUNET_CHAT_file_get_local_size(file) /
    GNUNET_CHAT_file_get_size(file)
  );

  gtk_progress_bar_set_fraction(handle->storage_progress_bar, progress);

  if (GNUNET_YES == GNUNET_CHAT_file_is_downloading(file))
    gtk_stack_set_visible_child(handle->play_icon_stack, handle->pause_icon_image);
  else
    gtk_stack_set_visible_child(handle->play_icon_stack, handle->play_icon_image);

  gtk_widget_set_visible(
    GTK_WIDGET(handle->play_pause_button),
    GNUNET_YES != GNUNET_CHAT_file_is_ready(file)
  );

  application_chat_unlock(app);

  UI_FILE_ENTRY_Handle *entry = (UI_FILE_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.ui)
  );

  const gchar *name = gtk_label_get_text(entry->name_label);
  const gchar *size = gtk_label_get_text(entry->size_label);

  gtk_label_set_text(handle->name_label, name);
  gtk_progress_bar_set_text(handle->storage_progress_bar, size);

  gtk_stack_set_visible_child(handle->dialog_stack, handle->info_box);
  gtk_widget_set_visible(GTK_WIDGET(handle->back_button), true);
}

static gboolean
handle_files_listbox_filter_func(GtkListBoxRow *row,
                                 gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
    return TRUE;

  const gchar *filter = gtk_entry_get_text(
    GTK_ENTRY(app->ui.files.file_search_entry)
  );

  if (!filter)
    return TRUE;

  UI_FILE_ENTRY_Handle *entry = (UI_FILE_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.ui)
  );

  if (!entry)
    return FALSE;

  const gchar *name = gtk_label_get_text(entry->name_label);

  if (!name)
    return FALSE;

  return g_str_match_string(filter, name, TRUE);
}

static void
handle_file_search_entry_search_changed(UNUSED GtkSearchEntry* search_entry,
                                        gpointer user_data)
{
  g_assert(user_data);

  GtkListBox *listbox = GTK_LIST_BOX(user_data);

  gtk_list_box_invalidate_filter(listbox);
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  ui_files_dialog_cleanup((UI_FILES_Handle*) user_data);
}

static enum GNUNET_GenericReturnValue
_iterate_files(void *cls,
               UNUSED struct GNUNET_CHAT_Handle *handle,
               struct GNUNET_CHAT_File *file)
{
  g_assert((cls) && (file));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;
  GtkListBox *listbox = app->ui.files.files_listbox;

  UI_FILE_ENTRY_Handle* entry = ui_file_entry_new(app);
  ui_file_entry_update(entry, file);

  gtk_list_box_prepend(listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), app->quarks.data, file);
  g_object_set_qdata_full(
    G_OBJECT(row),
    app->quarks.ui,
    entry,
    (GDestroyNotify) ui_file_entry_delete
  );

  return GNUNET_YES;
}

void
ui_files_dialog_init(MESSENGER_Application *app,
                     UI_FILES_Handle *handle)
{
  g_assert((app) && (handle));

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/files.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "files_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->dialog_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "dialog_stack")
  );

  handle->list_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "list_box")
  );

  handle->info_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "info_box")
  );

  handle->file_search_entry = GTK_SEARCH_ENTRY(
    gtk_builder_get_object(handle->builder, "file_search_entry")
  );

  handle->files_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "files_listbox")
  );

  gtk_list_box_set_filter_func(
    handle->files_listbox,
    handle_files_listbox_filter_func,
    app,
    NULL
  );

  g_signal_connect(
    handle->file_search_entry,
    "search-changed",
    G_CALLBACK(handle_file_search_entry_search_changed),
    handle->files_listbox
  );

  g_signal_connect(
    handle->files_listbox,
    "row-activated",
    G_CALLBACK(handle_files_listbox_row_activated),
    app
  );

  handle->name_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "name_label")
  );

  handle->storage_progress_bar = GTK_PROGRESS_BAR(
    gtk_builder_get_object(handle->builder, "storage_progress_bar")
  );

  handle->delete_file_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "delete_file_button")
  );

  handle->play_pause_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "play_pause_button")
  );

  handle->play_icon_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "play_icon_stack")
  );

  handle->play_icon_image = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "play_icon_image")
  );

  handle->pause_icon_image = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "pause_icon_image")
  );

  handle->back_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
    handle->back_button,
    "clicked",
    G_CALLBACK(handle_back_button_click),
    handle
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

  GNUNET_CHAT_iterate_files(
    app->chat.messenger.handle,
    _iterate_files,
    app
  );

  gtk_list_box_invalidate_filter(handle->files_listbox);
}

void
ui_files_dialog_cleanup(UI_FILES_Handle *handle)
{
  g_assert(handle);

  if (handle->builder)
    g_object_unref(handle->builder);

  memset(handle, 0, sizeof(*handle));
}
