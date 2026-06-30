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
 * @file ui/chat_title.c
 */

#include "chat_title.h"

#include "chat.h"
#include "file_load_entry.h"
#include "delete_messages.h"
#include "message.h"

#include "../contact.h"
#include "../ui.h"

static void
handle_back_button_click(UNUSED GtkButton *button,
			                   gpointer user_data)
{
  g_assert(user_data);

  HdyLeaflet *leaflet = HDY_LEAFLET(user_data);

  GList *children = gtk_container_get_children(GTK_CONTAINER(leaflet));

  if (children) {
    hdy_leaflet_set_visible_child(leaflet, GTK_WIDGET(children->data));
    g_list_free(children);
  }
}

static gboolean
_flap_chat_details_reveal_switch(gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  HdyFlap* flap = handle->flap_chat_details;

  hdy_flap_set_reveal_flap(flap, !hdy_flap_get_reveal_flap(flap));

  gtk_widget_set_sensitive(GTK_WIDGET(handle->messages_listbox), TRUE);
  return FALSE;
}

static void
handle_chat_details_via_button_click(UNUSED GtkButton* button,
				                             gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  gtk_widget_set_sensitive(GTK_WIDGET(handle->messages_listbox), FALSE);
  util_idle_add(
    G_SOURCE_FUNC(_flap_chat_details_reveal_switch),
    handle
  );
}

static void
handle_popover_via_button_click(UNUSED GtkButton *button,
				                        gpointer user_data)
{
  g_assert(user_data);

  GtkPopover *popover = GTK_POPOVER(user_data);

  if (gtk_widget_is_visible(GTK_WIDGET(popover)))
    gtk_popover_popdown(popover);
  else
    gtk_popover_popup(popover);
}

static void
handle_chat_selection_close_button_click(UNUSED GtkButton *button,
					                               gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  gtk_list_box_unselect_all(handle->messages_listbox);
}

void
_new_tag_callback(MESSENGER_Application *app,
                  GList *selected,
                  const char *tag,
                  gpointer user_data)
{
  g_assert((app) && (user_data));

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  UI_MESSAGE_Handle *message;

  if ((!(handle->context)) || (!tag))
    goto unselect;

  while (selected)
  {
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(selected->data);

    if (!row)
      goto skip_row;

    message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row),
      app->quarks.ui
    );

    if ((!message) || (!(message->msg)))
      goto skip_row;

    application_chat_lock(app);

    GNUNET_CHAT_context_send_tag(
      handle->context,
      message->msg,
      tag
    );

    application_chat_unlock(app);

  skip_row:
    selected = selected->next;
  }

unselect:
  gtk_list_box_unselect_all(handle->messages_listbox);
}

static void
handle_chat_selection_tag_button_click(UNUSED GtkButton *button,
					                             gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  MESSENGER_Application *app = handle->app;

  GList *selected = gtk_list_box_get_selected_rows(handle->messages_listbox);

  ui_new_tag_dialog_init(app, &(app->ui.new_tag));

  ui_new_tag_dialog_link(
    &(app->ui.new_tag),
    _new_tag_callback,
    selected,
    handle
  );

  gtk_widget_show(GTK_WIDGET(app->ui.new_tag.dialog));
}

void
_delete_messages_callback(MESSENGER_Application *app,
                          GList *selected,
                          gulong delay)
{
  g_assert(app);

  UI_MESSAGE_Handle *message;

  while (selected)
  {
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(selected->data);

    if (!row)
      goto skip_row;

    message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row),
      app->quarks.ui
    );

    if ((!message) || (!(message->msg)))
      goto skip_row;

    application_chat_lock(app);

    GNUNET_CHAT_message_delete(
    	message->msg,
    	delay
    );

    application_chat_unlock(app);

  skip_row:
    selected = selected->next;
  }
}

static void
handle_chat_selection_delete_button_click(UNUSED GtkButton *button,
					                                gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  MESSENGER_Application *app = handle->app;

  GList *selected = gtk_list_box_get_selected_rows(handle->messages_listbox);

  if (app->settings.hide_delete_dialog)
  {
    _delete_messages_callback(app, selected, 0);
    
    if (selected)
      g_list_free(selected);
  }
  else
  {
    ui_delete_messages_dialog_init(app, &(app->ui.delete_messages));

    ui_delete_messages_dialog_link(
      &(app->ui.delete_messages),
      _delete_messages_callback,
      selected
    );

    gtk_widget_show(GTK_WIDGET(app->ui.delete_messages.dialog));
  }
}

static void
handle_search_button_click(UNUSED GtkButton *button,
			                     gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  hdy_search_bar_set_search_mode(
    handle->chat_search_bar,
    !hdy_search_bar_get_search_mode(handle->chat_search_bar)
  );
}

UI_CHAT_TITLE_Handle*
ui_chat_title_new(MESSENGER_Application *app,
                  UI_CHAT_Handle *chat)
{
  g_assert((app) && (chat));

  UI_CHAT_TITLE_Handle *handle = g_malloc(sizeof(UI_CHAT_TITLE_Handle));
  UI_MESSENGER_Handle *messenger = &(app->ui.messenger);

  handle->contact = NULL;

  handle->chat = chat;
  handle->loads = NULL;

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/chat_title.ui")
  );

  handle->chat_title_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "chat_title_box")
  );

  handle->back_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "back_button")
  );

  g_object_bind_property(
    messenger->leaflet_chat,
    "folded",
    handle->back_button,
    "visible",
    G_BINDING_SYNC_CREATE
  );

  g_signal_connect(
    handle->back_button,
    "clicked",
    G_CALLBACK(handle_back_button_click),
    messenger->leaflet_chat
  );

  handle->chat_title_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "chat_title_stack")
  );

  handle->title_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "title_box")
  );

  handle->selection_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "selection_box")
  );

  handle->chat_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "chat_avatar")
  );

  handle->chat_title = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "chat_title")
  );

  handle->chat_subtitle = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "chat_subtitle")
  );

  handle->chat_load_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "chat_load_button")
  );

  handle->chat_load_popover = GTK_POPOVER(
    gtk_builder_get_object(handle->builder, "chat_load_popover")
  );

  handle->chat_load_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "chat_load_listbox")
  );

  g_signal_connect(
    handle->chat_load_button,
    "clicked",
    G_CALLBACK(handle_popover_via_button_click),
    handle->chat_load_popover
  );

  handle->chat_search_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "chat_search_button")
  );

  g_signal_connect(
    handle->chat_search_button,
    "clicked",
    G_CALLBACK(handle_search_button_click),
    handle->chat
  );

  handle->chat_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "chat_details_button")
  );

  g_signal_connect(
    handle->chat_details_button,
    "clicked",
    G_CALLBACK(handle_chat_details_via_button_click),
    handle->chat
  );

  handle->selection_close_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "selection_close_button")
  );

  handle->selection_count_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "selection_count_label")
  );

  handle->selection_tag_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "selection_tag_button")
  );

  handle->selection_delete_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "selection_delete_button")
  );

  g_signal_connect(
    handle->selection_close_button,
    "clicked",
    G_CALLBACK(handle_chat_selection_close_button_click),
    handle->chat
  );

  g_signal_connect(
    handle->selection_tag_button,
    "clicked",
    G_CALLBACK(handle_chat_selection_tag_button_click),
    handle->chat
  );

  g_signal_connect(
    handle->selection_delete_button,
    "clicked",
    G_CALLBACK(handle_chat_selection_delete_button_click),
    handle->chat
  );

  return handle;
}

static void
_chat_update_contact(UI_CHAT_TITLE_Handle *handle,
                     const struct GNUNET_CHAT_Contact* contact)
{
  g_assert(handle);

  if (handle->contact)
  {
    contact_remove_name_label_from_info(handle->contact, handle->chat_title);
    contact_remove_name_avatar_from_info(handle->contact, handle->chat_avatar);

    contact_remove_name_label_from_info(handle->contact, handle->chat->chat_details_label);
    contact_remove_name_avatar_from_info(handle->contact, handle->chat->chat_details_avatar);
  }
  
  if (contact)
  {
    contact_add_name_label_to_info(contact, handle->chat_title);
    contact_add_name_avatar_to_info(contact, handle->chat_avatar);

    contact_add_name_label_to_info(contact, handle->chat->chat_details_label);
    contact_add_name_avatar_to_info(contact, handle->chat->chat_details_avatar);
  }

  handle->contact = contact;
}

void
ui_chat_title_update(UI_CHAT_TITLE_Handle *handle,
                     MESSENGER_Application *app,
                     const gchar *subtitle)
{
  g_assert((handle) && (app));

  UI_CHAT_Handle *chat = handle->chat;

  struct GNUNET_CHAT_Contact* contact;
  struct GNUNET_CHAT_Group* group;

  contact = GNUNET_CHAT_context_get_contact(chat->context);
  group = GNUNET_CHAT_context_get_group(chat->context);

  const char *icon = "action-unavailable-symbolic";

  GString *sub = g_string_new(subtitle? subtitle : "");

  _chat_update_contact(handle, contact);

  if (contact)
    icon = "avatar-default-symbolic";
  else if (group)
  {
    const char *title = GNUNET_CHAT_group_get_name(group);

    if ((title) && ('#' == *title))
      icon = "network-wired-symbolic";
    else
      icon = "system-users-symbolic";

    g_string_append_printf(
      sub,
      _("%d members"),
      GNUNET_CHAT_group_iterate_contacts(group, NULL, NULL)
    );

    ui_label_set_text(handle->chat_title, title);
    ui_avatar_set_text(handle->chat_avatar, title);
    
    ui_label_set_text(handle->chat->chat_details_label, title);
    ui_avatar_set_text(handle->chat->chat_details_avatar, title);
  }

  hdy_avatar_set_icon_name(handle->chat_avatar, icon);
  hdy_avatar_set_icon_name(handle->chat->chat_details_avatar, icon);

  if (sub->len > 0)
    gtk_label_set_text(handle->chat_subtitle, sub->str);

  g_string_free(sub, TRUE);
}

void
ui_chat_title_delete(UI_CHAT_TITLE_Handle *handle)
{
  g_assert(handle);

  _chat_update_contact(handle, NULL);

  if (handle->loads)
    g_list_free_full(handle->loads, (GDestroyNotify) ui_file_load_entry_delete);

  g_object_unref(handle->builder);

  g_free(handle);
}

void
ui_chat_title_add_file_load(UI_CHAT_TITLE_Handle *handle,
                            UI_FILE_LOAD_ENTRY_Handle *file_load)
{
  g_assert((handle) && (file_load));

  gtk_container_add(
    GTK_CONTAINER(handle->chat_load_listbox),
    file_load->entry_box
  );

  handle->loads = g_list_append(handle->loads, file_load);

  gtk_widget_show(GTK_WIDGET(handle->chat_load_button));

  file_load->chat_title = handle;
}

void
ui_chat_title_remove_file_load(UI_CHAT_TITLE_Handle *handle,
                               UI_FILE_LOAD_ENTRY_Handle *file_load)
{
  g_assert((handle) && (file_load) && (handle == file_load->chat_title) &&
		(file_load->entry_box));

  handle->loads = g_list_remove(handle->loads, file_load);

  gtk_container_remove(
    GTK_CONTAINER(handle->chat_load_listbox),
    gtk_widget_get_parent(file_load->entry_box)
  );

  if (handle->loads)
    return;

  if (gtk_widget_is_visible(GTK_WIDGET(handle->chat_load_popover)))
    gtk_popover_popdown(handle->chat_load_popover);

  gtk_widget_hide(GTK_WIDGET(handle->chat_load_button));

  file_load->chat_title = NULL;
}
