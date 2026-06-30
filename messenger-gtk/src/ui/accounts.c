/*
   This file is part of GNUnet.
   Copyright (C) 2022--2026 GNUnet e.V.

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
 * @file ui/accounts.c
 */

#include "accounts.h"

#include <gnunet/gnunet_chat_lib.h>

#include "account_entry.h"

#include "../account.h"
#include "../application.h"
#include "../secret.h"
#include "../ui.h"

static void
handle_close_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);
  
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static gboolean
_open_new_account_dialog(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_new_account_dialog_init(app, &(app->ui.new_account));

  gtk_widget_show(GTK_WIDGET(app->ui.new_account.dialog));
  return FALSE;
}

static void
_account_secret_lookup(MESSENGER_Application *app,
                       const char *secret,
                       uint32_t secret_len,
                       gboolean success,
                       gboolean error,
                       gpointer user_data)
{
  g_assert((app) && (user_data));

  struct GNUNET_CHAT_Account *account = user_data;

  if (error)
  {
    fprintf(stderr, "ERROR: Looking up secret failed\n");
  }
  else if ((success) && (secret) && (secret_len > 0))
  {
    application_chat_lock(app);
    GNUNET_CHAT_connect(app->chat.messenger.handle, account, secret, secret_len);
    application_chat_unlock(app);
  }
  else
  {
    const char *name;

    application_chat_lock(app);
    name = GNUNET_CHAT_account_get_name(account);
    application_chat_unlock(app);

    secret_operation_generate(app, name, &_account_secret_lookup, account);
  }
}

static void
handle_accounts_listbox_row_activated(UNUSED GtkListBox* listbox,
                                      GtkListBoxRow* row,
                                      gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  // Drop activations of rows which do not contain accounts
  if (!gtk_list_box_row_get_selectable(row))
  {
    app->ui.accounts.show_queued = util_idle_add(
	    G_SOURCE_FUNC(_open_new_account_dialog), app
    );

    goto close_dialog;
  }

  struct GNUNET_CHAT_Account *account = (struct GNUNET_CHAT_Account*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.data)
  );

  if (!account)
    goto close_dialog;

  app->ui.state = MESSENGER_STATE_MAIN_WINDOW;

  gtk_list_box_unselect_all(app->ui.messenger.accounts_listbox);
  gtk_widget_set_sensitive(GTK_WIDGET(app->ui.accounts.dialog), FALSE);

  const char *name;

  application_chat_lock(app);
  name = GNUNET_CHAT_account_get_name(account);
  application_chat_unlock(app);

  secret_operation_lookup(app, name, &_account_secret_lookup, account);

close_dialog:
  gtk_window_close(GTK_WINDOW(app->ui.accounts.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
                      gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  ui_accounts_dialog_cleanup(&(app->ui.accounts), app);

  if ((app->ui.accounts.show_queued) ||
      (MESSENGER_STATE_MAIN_WINDOW == app->ui.state))
    return;

  gtk_widget_destroy(GTK_WIDGET(app->ui.messenger.main_window));
}

static int
_iterate_accounts(void *cls,
                  UNUSED struct GNUNET_CHAT_Handle *handle,
                  struct GNUNET_CHAT_Account *account)
{
  g_assert((cls) && (account));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  UI_ACCOUNT_ENTRY_Handle *entry = ui_account_entry_new(app);

  ui_account_entry_set_account(entry, account);

  gtk_list_box_prepend(app->ui.accounts.accounts_listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), app->quarks.data, account);

  g_object_set_qdata(
    G_OBJECT(row),
    app->quarks.ui,
    entry
  );

  return GNUNET_YES;
}

void
ui_accounts_dialog_init(MESSENGER_Application *app,
                        UI_ACCOUNTS_Handle *handle)
{
  g_assert((app) && (handle));

  handle->show_queued = 0;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/accounts.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "accounts_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->accounts_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "accounts_listbox")
  );

  g_signal_connect(
    handle->accounts_listbox,
    "row-activated",
    G_CALLBACK(handle_accounts_listbox_row_activated),
    app
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
    app
  );
}

static void
_ui_accounts_cleanup_listbox(UI_ACCOUNTS_Handle *handle,
                             MESSENGER_Application *app)
{
  g_assert(handle);

  GList *list = gtk_container_get_children(
    GTK_CONTAINER(handle->accounts_listbox)
  );

  GList *item = list;
  while (item)
  {
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(item->data);

    if ((!row) || (!gtk_list_box_row_get_selectable(row)))
      goto skip_row;

    UI_ACCOUNT_ENTRY_Handle *entry = g_object_get_qdata(
      G_OBJECT(row),
      app->quarks.ui
    );

    gtk_container_remove(
      GTK_CONTAINER(handle->accounts_listbox),
      GTK_WIDGET(row)
    );

    ui_account_entry_delete(entry);

  skip_row:
    item = item->next;
  }

  if (list)
    g_list_free(list);
}

void
ui_accounts_dialog_refresh(MESSENGER_Application *app,
                           UI_ACCOUNTS_Handle *handle)
{
  g_assert((app) && (handle));

  if (!(handle->accounts_listbox))
    return;

  _ui_accounts_cleanup_listbox(handle, app);

  application_chat_lock(app);

  GNUNET_CHAT_iterate_accounts(
    app->chat.messenger.handle,
    _iterate_accounts,
    app
  );

  application_chat_unlock(app);
}

void
ui_accounts_dialog_cleanup(UI_ACCOUNTS_Handle *handle,
                           MESSENGER_Application *app)
{
  g_assert((handle) && (app));

  _ui_accounts_cleanup_listbox(handle, app);

  g_object_unref(handle->builder);

  guint show = handle->show_queued;
  memset(handle, 0, sizeof(*handle));
  handle->show_queued = show;
}
