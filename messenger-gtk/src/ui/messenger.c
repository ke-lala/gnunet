/*
   This file is part of GNUnet.
   Copyright (C) 2021--2026 GNUnet e.V.

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
 * @file ui/messenger.c
 */

#include "messenger.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gtk-3.0/gdk/gdkkeys.h>
#include <stdio.h>

#include "account_entry.h"
#include "chat_entry.h"
#include "chat_title.h"
#include "contacts.h"
#include "message.h"
#include "new_contact.h"
#include "new_group.h"
#include "new_lobby.h"
#include "new_platform.h"
#include "settings.h"

#include "../account.h"
#include "../application.h"
#include "../secret.h"
#include "../ui.h"

static void
handle_user_details_folded(GObject* object,
                           GParamSpec* pspec,
                           gpointer user_data)
{
  g_assert((object) && (pspec) && (user_data));

  HdyFlap* flap = HDY_FLAP(object);
  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  const gboolean revealed = hdy_flap_get_reveal_flap(flap);

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->chats_search_entry),
    !revealed
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->chats_search_button),
    !revealed
  );

  GValue value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, !revealed);

  gtk_container_child_set_property(
    GTK_CONTAINER(handle->leaflet_title),
    GTK_WIDGET(handle->main_bar),
    "navigatable",
    &value
  );

  gtk_container_child_set_property(
    GTK_CONTAINER(handle->leaflet_chat),
    handle->main_box,
    "navigatable",
    &value
  );

  g_value_unset(&value);
}

static void
handle_profile_button_click(UNUSED GtkButton* button,
                            gpointer user_data)
{
  g_assert(user_data);

  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;
  MESSENGER_Application *app = handle->app;

  hdy_flap_set_reveal_flap(handle->flap_user_details, FALSE);

  ui_contact_info_dialog_init(app, &(app->ui.contact_info));
  ui_contact_info_dialog_update(&(app->ui.contact_info), NULL, FALSE);

  gtk_widget_show(GTK_WIDGET(app->ui.contact_info.dialog));
}

static gboolean
_flap_user_details_reveal_switch(gpointer user_data)
{
  g_assert(user_data);

  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;
  HdyFlap* flap = handle->flap_user_details;

  if (TRUE == hdy_flap_get_reveal_flap(flap)) {
    hdy_flap_set_reveal_flap(flap, FALSE);
  } else {
    hdy_flap_set_reveal_flap(flap, TRUE);
  }

  gtk_widget_set_sensitive(GTK_WIDGET(handle->chats_search_entry), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->chats_listbox), TRUE);
  return FALSE;
}

static void
handle_user_details_via_button_click(UNUSED GtkButton* button,
                                     gpointer user_data)
{
  g_assert(user_data);

  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  gtk_widget_set_sensitive(GTK_WIDGET(handle->chats_search_entry), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->chats_listbox), FALSE);
  util_idle_add(
    G_SOURCE_FUNC(_flap_user_details_reveal_switch),
    handle
  );
}

static void
handle_lobby_button_click(UNUSED GtkButton* button,
                          gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

  ui_new_lobby_dialog_init(app, &(app->ui.new_lobby));

  gtk_widget_show(GTK_WIDGET(app->ui.new_lobby.dialog));
}

static void
_switch_details_revealer_visibility(UI_MESSENGER_Handle *handle,
                                    gboolean state)
{
  g_assert(handle);

  GtkRevealer *revealer = handle->account_details_revealer;
  GtkImage *symbol = handle->account_details_symbol;

  gtk_revealer_set_reveal_child(revealer, state);
  gtk_image_set_from_icon_name(
    symbol,
    state?
    "go-up-symbolic" :
    "go-down-symbolic",
    GTK_ICON_SIZE_BUTTON
  );
}

static void
handle_account_details_button_click(UNUSED GtkButton* button,
                                    gpointer user_data)
{
  g_assert(user_data);

  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  GtkRevealer *revealer = handle->account_details_revealer;

  gboolean old_state = gtk_revealer_get_reveal_child(revealer);

  _switch_details_revealer_visibility(handle, !old_state);
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
    _switch_details_revealer_visibility(&(app->ui.messenger), FALSE);
    hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

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

  if (row == app->ui.messenger.add_account_listbox_row)
  {
    _switch_details_revealer_visibility(&(app->ui.messenger), FALSE);
    hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);

    ui_new_account_dialog_init(app, &(app->ui.new_account));

    gtk_widget_show(GTK_WIDGET(app->ui.new_account.dialog));
    return;
  }

  struct GNUNET_CHAT_Account *account = (struct GNUNET_CHAT_Account*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.data)
  );

  if (!account)
    return;

  const struct GNUNET_CHAT_Account *current;
  const char *name;


  application_chat_lock(app);
  current = GNUNET_CHAT_get_connected(app->chat.messenger.handle);
  name = GNUNET_CHAT_account_get_name(account);
  application_chat_unlock(app);

  if (account == current)
    return;

  secret_operation_lookup(app, name, &_account_secret_lookup, account);
}

static void
handle_new_contact_button_click(UNUSED GtkButton* button,
                                gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);
  ui_new_contact_dialog_init(app, &(app->ui.new_contact));
  gtk_widget_show(GTK_WIDGET(app->ui.new_contact.dialog));
}

static void
handle_new_group_button_click(UNUSED GtkButton* button,
                              gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);
  ui_new_group_dialog_init(app, &(app->ui.new_group));
  gtk_widget_show(GTK_WIDGET(app->ui.new_group.dialog));
}

static void
handle_new_platform_button_click(UNUSED GtkButton* button,
                                 gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);
  ui_new_platform_dialog_init(app, &(app->ui.new_platform));
  gtk_widget_show(GTK_WIDGET(app->ui.new_platform.dialog));
}

static void
handle_contacts_button_click(UNUSED GtkButton* button,
                             gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);
  ui_contacts_dialog_init(app, &(app->ui.contacts));
  gtk_widget_show(GTK_WIDGET(app->ui.contacts.dialog));
}

static void
handle_settings_button_click(UNUSED GtkButton* button,
                             gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);
  ui_settings_dialog_init(app, &(app->ui.settings));
  gtk_widget_show(GTK_WIDGET(app->ui.settings.dialog));
}

static void
handle_about_button_click(UNUSED GtkButton* button,
                          gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  hdy_flap_set_reveal_flap(HDY_FLAP(app->ui.messenger.flap_user_details), FALSE);
  ui_about_dialog_init(app, &(app->ui.about));
  gtk_widget_show(GTK_WIDGET(app->ui.about.dialog));
}

static void
handle_chats_listbox_row_activated(UNUSED GtkListBox* listbox,
                                   GtkListBoxRow* row,
                                   gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (!gtk_list_box_row_get_selectable(row))
    return;

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.ui)
  );

  if ((!entry) || (!(entry->chat)) || (!(entry->chat->chat_box)))
    return;

  UI_MESSENGER_Handle *ui = &(app->ui.messenger);
  GList *children = gtk_container_get_children(GTK_CONTAINER(ui->leaflet_chat));

  if ((children) && (children->next))
    hdy_leaflet_set_visible_child(ui->leaflet_chat, GTK_WIDGET(children->next->data));

  if (children)
    g_list_free(children);

  gtk_stack_set_visible_child(ui->chats_stack, entry->chat->chat_box);
  gtk_stack_set_visible_child(ui->chat_title_stack, entry->chat->title->chat_title_box);
}

static gint
handle_chats_listbox_sort_func(GtkListBoxRow* row0,
                               GtkListBoxRow* row1,
                               gpointer user_data)
{
  g_assert((row0) && (row1) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if ((!gtk_list_box_row_get_selectable(row0)) ||
      (!gtk_list_box_row_get_selectable(row1)))
    return 0;

  UI_CHAT_ENTRY_Handle *entry0 = (UI_CHAT_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row0), app->quarks.ui)
  );

  UI_CHAT_ENTRY_Handle *entry1 = (UI_CHAT_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row1), app->quarks.ui)
  );

  if ((!entry0) || (!entry1))
    return 0;

  time_t timestamp0 = entry0->timestamp;
  time_t timestamp1 = entry1->timestamp;

  const double diff = difftime(timestamp0, timestamp1);

  if (diff > +0.0)
    return -1;
  else if (diff < -0.0)
    return +1;
  else
    return 0;
}

static gboolean
handle_chats_listbox_filter_func(GtkListBoxRow *row,
                                 gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if ((!gtk_list_box_row_get_selectable(row)) ||
      (gtk_list_box_row_is_selected(row)))
    return TRUE;

  const gchar *filter = gtk_entry_get_text(
    GTK_ENTRY(app->ui.messenger.chats_search_entry)
  );

  if (!filter)
    return TRUE;

  UI_CHAT_ENTRY_Handle *entry = (UI_CHAT_ENTRY_Handle*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.ui)
  );

  if ((!entry) || (!(entry->title_label)))
    return FALSE;

  const gchar *title = gtk_label_get_text(entry->title_label);

  if (!title)
    return FALSE;

  return g_str_match_string(filter, title, TRUE);
}

static void
handle_search_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) user_data;

  if (handle->search_box == gtk_stack_get_visible_child(handle->chats_title_stack))
  {
    gtk_stack_set_visible_child(handle->chats_title_stack, handle->title_box);
    gtk_entry_set_text(GTK_ENTRY(handle->chats_search_entry), "");
  }
  else
    gtk_stack_set_visible_child(handle->chats_title_stack, handle->search_box);
}

static void
handle_chats_search_changed(UNUSED GtkSearchEntry *search,
                            gpointer user_data)
{
  g_assert(user_data);

  GtkListBox *listbox = GTK_LIST_BOX(user_data);

  gtk_list_box_invalidate_filter(listbox);
}

static void
handle_main_window_destroy(UNUSED GtkWidget *window,
                           gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  if (app->parent)
    xdp_parent_free(app->parent);

  app->parent = NULL;
#endif

  ui_messenger_cleanup(&(app->ui.messenger));
  
  account_cleanup_infos();

  application_exit(app, MESSENGER_QUIT);
}

void
ui_messenger_init(MESSENGER_Application *app,
                  UI_MESSENGER_Handle *handle)
{
  g_assert((app) && (handle));

  memset(handle, 0, sizeof(*handle));
  handle->app = app;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/messenger.ui")
  );

  handle->main_window = GTK_APPLICATION_WINDOW(
    gtk_builder_get_object(handle->builder, "main_window")
  );

  gtk_window_set_startup_id(
    GTK_WINDOW(handle->main_window),
    MESSENGER_APPLICATION_ID
  );

  gtk_window_set_icon_name(
    GTK_WINDOW(handle->main_window),
    MESSENGER_APPLICATION_ID
  );

  gtk_application_add_window(
    app->application,
    GTK_WINDOW(handle->main_window)
  );

  gtk_window_set_default_size(
    GTK_WINDOW(handle->main_window),
    1100, 700
  );

  handle->leaflet_title = HDY_LEAFLET(
    gtk_builder_get_object(handle->builder, "leaflet_title")
  );

  handle->leaflet_chat = HDY_LEAFLET(
    gtk_builder_get_object(handle->builder, "leaflet_chat")
  );

  g_object_bind_property(
    handle->leaflet_chat,
    "visible_child_name",
    handle->leaflet_title,
    "visible_child_name",
    G_BINDING_SYNC_CREATE
  );

  handle->flap_user_details = HDY_FLAP(
    gtk_builder_get_object(handle->builder, "flap_user_details")
  );

  g_signal_connect(
    handle->flap_user_details,
    "notify::reveal-flap",
    G_CALLBACK(handle_user_details_folded),
    handle
  );

  handle->nav_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "nav_box")
  );

  handle->main_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "main_box")
  );

  handle->nav_bar = HDY_HEADER_BAR(
    gtk_builder_get_object(handle->builder, "nav_bar")
  );

  handle->main_bar = HDY_HEADER_BAR(
    gtk_builder_get_object(handle->builder, "main_bar")
  );

  GtkLabel* application_name_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "application-name-label")
  );
  
  GtkLabel* application_version_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "application-version-label")
  );

  gtk_label_set_text(
    application_name_label,
    MESSENGER_APPLICATION_APPNAME
  );
  
  gtk_label_set_text(
    application_version_label,
    MESSENGER_APPLICATION_VERSION
  );

  g_object_bind_property(
    handle->leaflet_chat,
    "folded",
    handle->main_bar,
    "show-close-button",
    G_BINDING_INVERT_BOOLEAN
  );
  
  handle->profile_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "profile_button")
  );

  g_signal_connect(
    handle->profile_button,
    "clicked",
    G_CALLBACK(handle_profile_button_click),
    handle
  );

  handle->profile_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "profile_avatar")
  );

  handle->profile_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "profile_label")
  );

  handle->profile_key_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "profile_key_label")
  );

  handle->hide_user_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "hide_user_details_button")
  );

  g_signal_connect(
    handle->hide_user_details_button,
    "clicked",
    G_CALLBACK(handle_user_details_via_button_click),
    handle
  );

  handle->lobby_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "lobby_button")
  );

  g_signal_connect(
    handle->lobby_button,
    "clicked",
    G_CALLBACK(handle_lobby_button_click),
    app
  );

  handle->account_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "account_details_button")
  );

  handle->account_details_symbol = GTK_IMAGE(
    gtk_builder_get_object(handle->builder, "account_details_symbol")
  );

  handle->account_details_revealer = GTK_REVEALER(
    gtk_builder_get_object(handle->builder, "account_details_revealer")
  );

  g_signal_connect(
    handle->account_details_button,
    "clicked",
    G_CALLBACK(handle_account_details_button_click),
    handle
  );

  handle->accounts_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "accounts_listbox")
  );

  handle->add_account_listbox_row = GTK_LIST_BOX_ROW(
    gtk_builder_get_object(handle->builder, "add_account_listbox_row")
  );

  g_signal_connect(
    handle->accounts_listbox,
    "row-activated",
    G_CALLBACK(handle_accounts_listbox_row_activated),
    app
  );

  handle->new_contact_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "new_contact_button")
  );

  handle->new_group_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "new_group_button")
  );

  handle->new_platform_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "new_platform_button")
  );

  g_signal_connect(
    handle->new_contact_button,
    "clicked",
    G_CALLBACK(handle_new_contact_button_click),
    app
  );

  g_signal_connect(
    handle->new_group_button,
    "clicked",
    G_CALLBACK(handle_new_group_button_click),
    app
  );

  g_signal_connect(
    handle->new_platform_button,
    "clicked",
    G_CALLBACK(handle_new_platform_button_click),
    app
  );

  handle->contacts_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "contacts_button")
  );

  handle->settings_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "settings_button")
  );

  g_signal_connect(
    handle->contacts_button,
    "clicked",
    G_CALLBACK(handle_contacts_button_click),
    app
  );

  g_signal_connect(
    handle->settings_button,
    "clicked",
    G_CALLBACK(handle_settings_button_click),
    app
  );

  handle->about_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "about_button")
  );

  g_signal_connect(
    handle->about_button,
    "clicked",
    G_CALLBACK(handle_about_button_click),
    app
  );

  handle->chats_title_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "chats_title_stack")
  );

  handle->title_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "title_box")
  );

  handle->search_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "search_box")
  );

  g_object_bind_property(
    handle->leaflet_chat,
    "folded",
    handle->nav_bar,
    "hexpand",
    G_BINDING_SYNC_CREATE
  );

  handle->user_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "user_details_button")
  );

  g_signal_connect(
    handle->user_details_button,
    "clicked",
    G_CALLBACK(handle_user_details_via_button_click),
    handle
  );

  handle->chats_search_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "chats_search_button")
  );

  g_signal_connect(
    handle->chats_search_button,
    "clicked",
    G_CALLBACK(handle_search_button_click),
    handle
  );

  handle->chats_search_entry = GTK_SEARCH_ENTRY(
    gtk_builder_get_object(handle->builder, "chats_search_entry")
  );

  handle->search_icon_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "search_icon_stack")
  );

  g_object_bind_property(
    handle->chats_title_stack,
    "visible_child_name",
    handle->search_icon_stack,
    "visible_child_name",
    G_BINDING_SYNC_CREATE
  );

  handle->chats_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "chats_listbox")
  );

  gtk_list_box_set_sort_func(
    handle->chats_listbox,
    handle_chats_listbox_sort_func,
    app,
    NULL
  );

  gtk_list_box_set_filter_func(
    handle->chats_listbox,
    handle_chats_listbox_filter_func,
    app,
    NULL
  );

  g_signal_connect(
    handle->chats_search_entry,
    "search-changed",
    G_CALLBACK(handle_chats_search_changed),
    handle->chats_listbox
  );

  g_signal_connect(
    handle->chats_listbox,
    "row-activated",
    G_CALLBACK(handle_chats_listbox_row_activated),
    app
  );

  handle->chats_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "chats_stack")
  );

  handle->no_chat_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "no_chat_box")
  );

  handle->chat_title_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "chat_title_stack")
  );

  g_signal_connect(
    handle->main_window,
    "destroy",
    G_CALLBACK(handle_main_window_destroy),
    app
  );
}

static int
_messenger_iterate_accounts(void *cls,
                            struct GNUNET_CHAT_Handle *handle,
                            struct GNUNET_CHAT_Account *account)
{
  g_assert((cls) && (handle) && (account));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;
  UI_MESSENGER_Handle *ui = &(app->ui.messenger);

  UI_ACCOUNT_ENTRY_Handle *entry = ui_account_entry_new(app);

  ui_account_entry_set_account(entry, account);

  gtk_list_box_prepend(ui->accounts_listbox, entry->entry_box);

  GtkWidget *row = gtk_widget_get_parent(entry->entry_box);

  g_object_set_qdata(G_OBJECT(row), app->quarks.data, account);

  if (account == GNUNET_CHAT_get_connected(handle))
    gtk_widget_activate(row);

  ui_account_entry_delete(entry);
  return GNUNET_YES;
}

static void
_clear_accounts_listbox(GtkWidget *widget,
                        gpointer data)
{
  g_assert((widget) && (data));

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(widget);
  GtkListBox *listbox = GTK_LIST_BOX(data);

  if (!gtk_list_box_row_get_selectable(row))
    return;

  gtk_container_remove(
    GTK_CONTAINER(listbox),
    widget
  );
}

static gboolean
_close_messenger_missing_account(gpointer data)
{
  g_assert(data);

  UI_MESSENGER_Handle *handle = (UI_MESSENGER_Handle*) data;  

  gtk_window_close(GTK_WINDOW(handle->main_window));
  return FALSE;
}

void
ui_messenger_refresh(MESSENGER_Application *app,
                     UI_MESSENGER_Handle *handle)
{
  g_assert((app) && (handle));

  if (!(handle->accounts_listbox))
    return;

  gtk_container_foreach(
    GTK_CONTAINER(handle->accounts_listbox),
    _clear_accounts_listbox,
    handle->accounts_listbox
  );

  application_chat_lock(app);

  GNUNET_CHAT_iterate_accounts(
    app->chat.messenger.handle,
    _messenger_iterate_accounts,
    app
  );

  application_chat_unlock(app);

  if (gtk_list_box_get_selected_row(handle->accounts_listbox))
    return;
  
  gtk_widget_hide(GTK_WIDGET(handle->main_window));

  handle->account_refresh = util_idle_add(
    G_SOURCE_FUNC(_close_messenger_missing_account),
    handle
  );
}

gboolean
ui_messenger_is_context_active(UI_MESSENGER_Handle *handle,
                               struct GNUNET_CHAT_Context *context)
{
  g_assert((handle) && (context));

  if (!gtk_window_is_active(GTK_WINDOW(handle->main_window)))
    return FALSE;

  UI_CHAT_ENTRY_Handle *entry = GNUNET_CHAT_context_get_user_pointer(context);

  if ((!entry) || (!(entry->entry_box)))
    return FALSE;

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  if (!row)
    return FALSE;

  return gtk_list_box_row_is_selected(row);
}

void
ui_messenger_cleanup(UI_MESSENGER_Handle *handle)
{
  g_assert(handle);

  g_object_unref(handle->builder);

  if (handle->chat_entries)
    g_list_free_full(handle->chat_entries, (GDestroyNotify) ui_chat_entry_delete);

  if (handle->chat_selection)
    util_source_remove(handle->chat_selection);

  if (handle->account_refresh)
    util_source_remove(handle->account_refresh);

  memset(handle, 0, sizeof(*handle));
}
