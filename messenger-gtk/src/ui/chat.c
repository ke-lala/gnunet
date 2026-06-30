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
 * @file ui/chat.c
 */

#include "chat.h"

#include <gdk/gdkkeysyms.h>
#include <gnunet/gnunet_chat_lib.h>
#include <stdlib.h>

#include "chat_entry.h"
#include "chat_title.h"
#include "file_entry.h"
#include "file_load_entry.h"
#include "media_preview.h"
#include "message.h"
#include "messenger.h"
#include "picker.h"
#include "account_entry.h"

#include "../application.h"
#include "../file.h"
#include "../ui.h"

static void
handle_chat_details_folded(GObject* object,
                           GParamSpec* pspec,
                           gpointer user_data)
{
  g_assert((object) && (pspec) && (user_data));

  HdyFlap* flap = HDY_FLAP(object);
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  UI_MESSENGER_Handle *messenger = &(handle->app->ui.messenger);

  const gboolean revealed = hdy_flap_get_reveal_flap(flap);

  hdy_leaflet_set_can_swipe_back(messenger->leaflet_title, !revealed);
  hdy_leaflet_set_can_swipe_back(messenger->leaflet_chat, !revealed);

  if (handle->title)
  {
    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->title->back_button),
      !revealed
    );

    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->title->chat_search_button),
      !revealed
    );
  }

  GValue value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, !revealed);

  gtk_container_child_set_property(
    GTK_CONTAINER(messenger->leaflet_title),
    GTK_WIDGET(messenger->nav_bar),
    "navigatable",
    &value
  );

  gtk_container_child_set_property(
    GTK_CONTAINER(messenger->leaflet_chat),
    messenger->nav_box,
    "navigatable",
    &value
  );

  g_value_unset(&value);
}

static gboolean
_flap_chat_details_reveal_switch(gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  HdyFlap* flap = handle->flap_chat_details;

  gboolean revealed = hdy_flap_get_reveal_flap(flap);

  hdy_flap_set_reveal_flap(flap, !revealed);

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
handle_chat_contacts_listbox_row_activated(GtkListBox *listbox,
                                           GtkListBoxRow *row,
                                           gpointer user_data)
{
  g_assert((listbox) && (row) && (user_data));

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  MESSENGER_Application *app = handle->app;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(listbox), app->quarks.widget)
  );

  if (!text_view)
    return;

  if (!gtk_list_box_row_get_selectable(row))
  {
    ui_invite_contact_dialog_init(app, &(app->ui.invite_contact));

    g_object_set_qdata(
      G_OBJECT(app->ui.invite_contact.contacts_listbox),
      app->quarks.widget,
      text_view
    );

    gtk_widget_show(GTK_WIDGET(app->ui.invite_contact.dialog));
    return;
  }

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.data)
  );

  if (!contact)
    return;

  hdy_flap_set_reveal_flap(handle->flap_chat_details, FALSE);

  application_chat_lock(app);

  ui_contact_info_dialog_init(app, &(app->ui.contact_info));
  ui_contact_info_dialog_update(&(app->ui.contact_info), contact, FALSE);

  application_chat_unlock(app);

  gtk_widget_show(GTK_WIDGET(app->ui.contact_info.dialog));
}

static void
handle_chat_messages_listbox_size_allocate(UNUSED GtkWidget *widget,
                                           UNUSED GdkRectangle *allocation,
                                           gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(
      handle->chat_scrolled_window
  );

  const gdouble value = gtk_adjustment_get_value(adjustment);
  const gdouble upper = gtk_adjustment_get_upper(adjustment);
  const gdouble page_size = gtk_adjustment_get_page_size(adjustment);

  const gdouble edge_value = upper - page_size;

  if (value >= handle->edge_value)
    gtk_adjustment_set_value(adjustment, edge_value);

  handle->edge_value = upper - page_size;
}

static void
handle_reveal_identity_button_click(GtkButton *button,
                                    gpointer user_data)
{
  g_assert((button) && (user_data));

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  MESSENGER_Application *app = handle->app;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(G_OBJECT(button), app->quarks.data)
  );

  if (!contact)
    return;

  hdy_flap_set_reveal_flap(handle->flap_chat_details, FALSE);

  application_chat_lock(app);

  ui_contact_info_dialog_init(app, &(app->ui.contact_info));
  ui_contact_info_dialog_update(&(app->ui.contact_info), contact, TRUE);

  application_chat_unlock(app);

  gtk_widget_show(GTK_WIDGET(app->ui.contact_info.dialog));
}

static void
handle_discourse_button_click(GtkButton *button,
                              gpointer user_data)
{
  g_assert((button) && (user_data));

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  MESSENGER_Application *app = handle->app;

  hdy_flap_set_reveal_flap(handle->flap_chat_details, FALSE);

  ui_discourse_window_init(app, &(app->ui.discourse));
  ui_discourse_window_update(&(app->ui.discourse), handle->context);

  gtk_widget_show(GTK_WIDGET(app->ui.discourse.window));
}

static void
handle_block_button_click(UNUSED GtkButton *button,
                          gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(G_OBJECT(handle->block_stack), handle->app->quarks.data)
  );

  if (!contact)
    return;

  application_chat_lock(handle->app);
  GNUNET_CHAT_contact_set_blocked(contact, GNUNET_YES);
  application_chat_unlock(handle->app);

  gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->unblock_button));
}

static void
handle_unblock_button_click(UNUSED GtkButton *button,
                            gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) (
    g_object_get_qdata(G_OBJECT(handle->block_stack), handle->app->quarks.data)
  );

  if (!contact)
    return;

  application_chat_lock(handle->app);
  GNUNET_CHAT_contact_set_blocked(contact, GNUNET_NO);
  application_chat_unlock(handle->app);

  gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->block_button));
}

static void
handle_leave_chat_button_click(UNUSED GtkButton *button,
                               gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  if ((!handle) || (!(handle->context)))
    return;

  application_chat_lock(handle->app);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_context_get_contact(
    handle->context
  );

  struct GNUNET_CHAT_Group *group = GNUNET_CHAT_context_get_group(
    handle->context
  );

  if (contact)
    GNUNET_CHAT_contact_delete(contact);
  else if (group)
    GNUNET_CHAT_group_leave(group);

  application_chat_unlock(handle->app);

  UI_CHAT_ENTRY_Handle *entry = GNUNET_CHAT_context_get_user_pointer(
    handle->context
  );

  if ((!entry) || (!(handle->app)))
    return;

  ui_chat_entry_dispose(entry, handle->app);
}

static gint
handle_chat_messages_sort(GtkListBoxRow* row0,
                          GtkListBoxRow* row1,
                          gpointer user_data)
{
  g_assert((row0) && (row1) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle *message0 = (UI_MESSAGE_Handle*) (
    g_object_get_qdata(G_OBJECT(row0), app->quarks.ui)
  );

  UI_MESSAGE_Handle *message1 = (UI_MESSAGE_Handle*) (
    g_object_get_qdata(G_OBJECT(row1), app->quarks.ui)
  );

  if ((!message0) || (!message1))
    return 0;

  time_t timestamp0 = message0->timestamp;
  time_t timestamp1 = message1->timestamp;

  const double diff = difftime(timestamp0, timestamp1);

  if (diff < -0.0)
    return -1;
  else if (diff > +0.0)
    return +1;
  else
    return 0;
}

struct FilterTags
{
  const gchar *filter;
  gboolean matching;
};

static enum GNUNET_GenericReturnValue
_iterate_message_tags(void *cls,
                      struct GNUNET_CHAT_Message *message)
{
  g_assert((cls) && (message));

  struct FilterTags *filterTags = (struct FilterTags*) cls;

  const char *text = GNUNET_CHAT_message_get_text(message);
  if (!text)
    return GNUNET_YES;

  gchar *_text = g_locale_to_utf8(text, -1, NULL, NULL, NULL);
  if (!_text)
    return GNUNET_YES;

  if (g_strstr_len(_text, -1, filterTags->filter))
    filterTags->matching = TRUE;

  g_free(_text);
  return filterTags->matching? GNUNET_NO : GNUNET_YES;
}

static gboolean
handle_chat_messages_filter(GtkListBoxRow *row,
                            gpointer user_data)
{
  g_assert((row) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkListBox *listbox = GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(row)));

  if (!listbox)
    return TRUE;

  UI_CHAT_Handle *chat = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(listbox), app->quarks.ui)
  );

  if (!chat)
    return TRUE;

  const gchar *filter = gtk_entry_get_text(
    GTK_ENTRY(chat->chat_search_entry)
  );

  if (!filter)
    return TRUE;

  UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) (
    g_object_get_qdata(G_OBJECT(row), app->quarks.ui)
  );

  if (!message)
    return TRUE;

  const gchar *sender = gtk_label_get_text(message->sender_label);
  const gchar *text = gtk_label_get_text(message->text_label);

  gboolean result = FALSE;

  if (sender)
    result |= g_str_match_string(filter, sender, TRUE);

  if (text)
    result |= g_str_match_string(filter, text, TRUE);

  if (('#' == *filter) && (message->msg))
  {
    struct FilterTags filterTags;
    filterTags.filter = &(filter[1]);
    filterTags.matching = FALSE;

    application_chat_lock(app);

    GNUNET_CHAT_message_iterate_tags(
      message->msg,
      _iterate_message_tags,
      &filterTags
    );

    application_chat_unlock(app);

    result |= filterTags.matching;
  }

  return result;
}

static void
handle_chat_messages_selected_rows_changed(GtkListBox *listbox,
                                           gpointer user_data)
{
  g_assert((listbox) && (user_data));

  UI_CHAT_TITLE_Handle *handle = (UI_CHAT_TITLE_Handle*) user_data;

  GList *selected = gtk_list_box_get_selected_rows(listbox);
  uint32_t count = 0;

  GList *item = selected;
  while (item)
  {
    count++;
    item = item->next;
  }

  if (selected)
    g_list_free(selected);

  GString *counter = g_string_new("");
  g_string_append_printf(counter, "%u", count);
  gtk_label_set_text(handle->selection_count_label, counter->str);
  g_string_free(counter, TRUE);

  gtk_widget_set_sensitive(GTK_WIDGET(handle->selection_tag_button), count == 1);

  if (count > 0)
    gtk_stack_set_visible_child(handle->chat_title_stack, handle->selection_box);
  else
    gtk_stack_set_visible_child(handle->chat_title_stack, handle->title_box);
}

static void
handle_attach_file_button_click(GtkButton *button,
                                gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(button), app->quarks.widget)
  );

  if (!text_view)
    return;

  GtkWidget *dialog = gtk_file_chooser_dialog_new(
    _("Select file"),
    GTK_WINDOW(app->ui.messenger.main_window),
    GTK_FILE_CHOOSER_ACTION_OPEN,
    _("Cancel"),
    GTK_RESPONSE_CANCEL,
    _("Confirm"),
    GTK_RESPONSE_ACCEPT,
    NULL
  );

  if (GTK_RESPONSE_ACCEPT != gtk_dialog_run(GTK_DIALOG(dialog)))
    goto close_dialog;

  gchar *filename = gtk_file_chooser_get_filename(
    GTK_FILE_CHOOSER(dialog)
  );

  if (!filename)
    return;

  ui_send_file_dialog_init(app, &(app->ui.send_file));
  ui_send_file_dialog_update(&(app->ui.send_file), filename);

  g_free(filename);

  g_object_set_qdata(
    G_OBJECT(app->ui.send_file.send_button),
    app->quarks.widget,
    text_view
  );

  gtk_widget_show(GTK_WIDGET(app->ui.send_file.dialog));

close_dialog:
  gtk_widget_destroy(dialog);
}

static void
_update_send_record_symbol(GtkTextBuffer *buffer,
                           GtkImage *symbol,
                           gboolean picker_revealed)
{
  g_assert((buffer) && (symbol));

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  gtk_image_set_from_icon_name(
    symbol,
    (0 < strlen(text)) || (picker_revealed)?
    "mail-send-symbolic" :
    "audio-input-microphone-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  g_free(text);
}

static void
handle_send_text_buffer_changed(GtkTextBuffer *buffer,
                                gpointer user_data)
{
  g_assert((buffer) && (user_data));

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  _update_send_record_symbol(
    buffer,
    handle->send_record_symbol,
    gtk_revealer_get_child_revealed(handle->picker_revealer)
  );
}

static gboolean
_send_text_from_view(MESSENGER_Application *app,
                     UI_CHAT_Handle *handle,
                     GtkTextView *text_view,
                     gint64 action_time)
{
  g_assert((app) && (handle) && (text_view));

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (0 == strlen(text))
  {
    g_free(text);
    return FALSE;
  }

  if (action_time >= UI_CHAT_SEND_BUTTON_HOLD_INTERVAL)
  {
    gtk_popover_popup(handle->send_popover);
    return FALSE;
  }

  if (handle->context)
  {
    application_chat_lock(app);
    GNUNET_CHAT_context_send_text(handle->context, text);
    application_chat_unlock(app);
  }

  g_free(text);
  gtk_text_buffer_delete(buffer, &start, &end);
  return TRUE;
}

static void
_drop_any_recording(UI_CHAT_Handle *handle)
{
  g_assert(handle);

  if ((handle->play_pipeline) && (handle->playing))
  {
    gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
    handle->playing = FALSE;
  }

  _update_send_record_symbol(
    gtk_text_view_get_buffer(handle->send_text_view),
    handle->send_record_symbol,
    FALSE
  );

  gtk_stack_set_visible_child(handle->send_stack, handle->send_text_box);

  if (handle->recording_filename[0])
    remove(handle->recording_filename);

  handle->recording_filename[0] = 0;
  handle->recorded = FALSE;
}

static void
handle_sending_recording_upload_file(UNUSED void *cls,
                                     struct GNUNET_CHAT_File *file,
                                     uint64_t completed,
                                     uint64_t size)
{
  g_assert(file);

  UI_FILE_LOAD_ENTRY_Handle *file_load = cls;

  gtk_progress_bar_set_fraction(
    file_load->load_progress_bar,
    1.0 * completed / size
  );

  file_update_upload_info(file, completed, size);

  if ((completed >= size) && (file_load->chat_title))
    ui_chat_title_remove_file_load(file_load->chat_title, file_load);
}

static void
handle_send_record_button_click(GtkButton *button,
                                gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if ((handle->recorded) && (handle->context) &&
      (handle->recording_filename[0]) &&
      (!gtk_revealer_get_child_revealed(handle->picker_revealer)))
  {
    UI_FILE_LOAD_ENTRY_Handle *file_load = ui_file_load_entry_new(app);

    ui_label_set_text(file_load->file_label, handle->recording_filename);
    gtk_progress_bar_set_fraction(file_load->load_progress_bar, 0.0);

    application_chat_lock(app);

    struct GNUNET_CHAT_File *file = GNUNET_CHAT_context_send_file(
      handle->context,
      handle->recording_filename,
      handle_sending_recording_upload_file,
      file_load
    );

    if (file)
    {
      file_create_info(file);

      ui_chat_title_add_file_load(handle->title, file_load);
    }
    else if (file_load)
      ui_file_load_entry_delete(file_load);

    application_chat_unlock(app);

    _drop_any_recording(handle);
    return;
  }

  if (gtk_stack_get_visible_child(handle->send_stack) != handle->send_text_box)
    return;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(button), app->quarks.widget)
  );

  _send_text_from_view(app, handle, text_view, handle->send_pressed_time);
}

static void
handle_send_later_button_click(GtkButton *button,
                               gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  handle->send_pressed_time = 0;

  if (gtk_widget_is_visible(GTK_WIDGET(handle->send_popover)))
    gtk_popover_popdown(handle->send_popover);

  if (gtk_stack_get_visible_child(handle->send_stack) != handle->send_text_box)
    return;

  // TODO
}

static void
handle_send_now_button_click(GtkButton *button,
                             gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if (gtk_widget_is_visible(GTK_WIDGET(handle->send_popover)))
    gtk_popover_popdown(handle->send_popover);

  if (gtk_stack_get_visible_child(handle->send_stack) != handle->send_text_box)
    return;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(handle->send_record_button), app->quarks.widget)
  );

  _send_text_from_view(app, handle, text_view, 0);
}

static gboolean
handle_send_record_button_pressed(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer user_data)
{
  g_assert((widget) && (event) && (user_data));

  GdkEventButton *ev = (GdkEventButton*) event;
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if (1 != ev->button)
    return FALSE;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(widget), app->quarks.widget)
  );

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(widget), app->quarks.ui)
  );

  handle->send_pressed_time = g_get_monotonic_time();

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
  const size_t text_len = strlen(text);

  g_free(text);

  if (0 < text_len)
    return FALSE;

  if ((handle->recorded) || (!(handle->record_pipeline)) ||
      (handle->recording_filename[0]) ||
      (gtk_revealer_get_child_revealed(handle->picker_revealer)) ||
      (handle->send_text_box != gtk_stack_get_visible_child(handle->send_stack)))
    return FALSE;

  strcpy(handle->recording_filename, "/tmp/rec_XXXXXX.ogg");

  int fd = mkstemps(handle->recording_filename, 4);

  if (-1 == fd)
    return FALSE;
  else
    close(fd);

  if ((handle->play_pipeline) && (handle->playing))
  {
    gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
    handle->playing = FALSE;
  }

  gtk_image_set_from_icon_name(
    handle->play_pause_symbol,
    "media-playback-start-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  gtk_image_set_from_icon_name(
    handle->send_record_symbol,
    "media-record-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  gtk_label_set_text(handle->recording_label, "00:00:00");
  gtk_progress_bar_set_fraction(handle->recording_progress_bar, 0.0);

  gtk_widget_set_sensitive(GTK_WIDGET(handle->recording_play_button), FALSE);
  gtk_stack_set_visible_child(handle->send_stack, handle->send_recording_box);

  g_object_set(
    G_OBJECT(handle->record_sink),
    "location",
    handle->recording_filename,
    NULL
  );

  gst_element_set_state(handle->record_pipeline, GST_STATE_PLAYING);

  return TRUE;
}

static gboolean
handle_send_record_button_released(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer user_data)
{
  g_assert((widget) && (event) && (user_data));

  GdkEventButton *ev = (GdkEventButton*) event;
  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(widget), app->quarks.ui)
  );

  if ((gtk_stack_get_visible_child(handle->send_stack) == handle->send_text_box) &&
      (3 == ev->button))
  {
    handle->send_pressed_time = UI_CHAT_SEND_BUTTON_HOLD_INTERVAL;

    handle_send_record_button_click(GTK_BUTTON(widget), user_data);
    return FALSE;
  }
  else if (1 != ev->button)
    return FALSE;

  GtkTextView *text_view = GTK_TEXT_VIEW(
    g_object_get_qdata(G_OBJECT(widget), app->quarks.widget)
  );

  handle->send_pressed_time = g_get_monotonic_time() - handle->send_pressed_time;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
  const size_t text_len = strlen(text);

  g_free(text);

  if (0 < text_len)
    return FALSE;

  if ((handle->recorded) || (!(handle->record_pipeline)) ||
      (!(handle->recording_filename[0])) ||
      (gtk_revealer_get_child_revealed(handle->picker_revealer)) ||
      (handle->send_recording_box != gtk_stack_get_visible_child(
	  handle->send_stack)))
    return FALSE;

  gtk_widget_set_sensitive(GTK_WIDGET(handle->recording_play_button), TRUE);

  gst_element_set_state(handle->record_pipeline, GST_STATE_NULL);
  handle->recorded = TRUE;

  gtk_image_set_from_icon_name(
    handle->send_record_symbol,
    "mail-send-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  return TRUE;
}

static gboolean
handle_send_text_key_press (GtkWidget *widget,
                            GdkEventKey *event,
                            gpointer user_data)
{
  g_assert((widget) && (event) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  if ((event->state & GDK_SHIFT_MASK) ||
      ((event->keyval != GDK_KEY_Return) &&
       (event->keyval != GDK_KEY_KP_Enter)))
    return FALSE;
  
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) (
    g_object_get_qdata(G_OBJECT(widget), app->quarks.ui)
  );

  return _send_text_from_view(app, handle, GTK_TEXT_VIEW(widget), 0);
}

static void
handle_recording_close_button_click(UNUSED GtkButton *button,
                                    gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  _drop_any_recording(handle);
}

static void
_stop_playing_recording(UI_CHAT_Handle *handle,
                        gboolean reset_bar)
{
  g_assert(handle);

  gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
  handle->playing = FALSE;

  gtk_image_set_from_icon_name(
    handle->play_pause_symbol,
    "media-playback-start-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

  gtk_progress_bar_set_fraction(
    handle->recording_progress_bar,
    reset_bar? 0.0 : 1.0
  );

  if (handle->play_timer)
  {
    util_source_remove(handle->play_timer);
    handle->play_timer = 0;
  }
}

static void
handle_recording_play_button_click(UNUSED GtkButton *button,
                                   gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  if ((!(handle->recorded)) || (!(handle->play_pipeline)))
    return;

  if (handle->playing)
    _stop_playing_recording(handle, TRUE);
  else if (handle->recording_filename[0])
  {
    GString* uri = g_string_new("file://");
    g_string_append(uri, handle->recording_filename);

    g_object_set(
      G_OBJECT(handle->play_pipeline),
      "uri",
      uri->str,
      NULL
    );

    g_string_free(uri, TRUE);

    gst_element_set_state(handle->play_pipeline, GST_STATE_PLAYING);
    handle->playing = TRUE;

    gtk_image_set_from_icon_name(
	    handle->play_pause_symbol,
    	"media-playback-stop-symbolic",
    	GTK_ICON_SIZE_BUTTON
    );
  }
}

static void
handle_search_entry_search_changed(UNUSED GtkSearchEntry* search_entry,
                                   gpointer user_data)
{
  g_assert(user_data);

  GtkListBox *listbox = GTK_LIST_BOX(user_data);

  gtk_list_box_invalidate_filter(listbox);
}

static void
handle_picker_button_click(UNUSED GtkButton *button,
                           gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  gboolean reveal = !gtk_revealer_get_child_revealed(handle->picker_revealer);

  gtk_revealer_set_reveal_child(handle->picker_revealer, reveal);

  _update_send_record_symbol(
    gtk_text_view_get_buffer(handle->send_text_view),
    handle->send_record_symbol,
    reveal
  );
}

static gboolean
_record_timer_func(gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;

  GString *time_string = g_string_new(NULL);

  g_string_printf(
    time_string,
    "%02u:%02u:%02u",
    (handle->record_time / 3600),
    (handle->record_time / 60) % 60,
    (handle->record_time % 60)
  );

  gtk_label_set_text(handle->recording_label, time_string->str);
  g_string_free(time_string, TRUE);

  if (!(handle->recorded))
  {
    handle->record_time++;
    handle->record_timer = util_timeout_add_seconds(
      1,
      _record_timer_func,
      handle
    );
  }
  else
    handle->record_timer = 0;

  return FALSE;
}

static gboolean
_play_timer_func(gpointer user_data)
{
  g_assert(user_data);

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) user_data;
  gint64 pos, len;

  handle->play_timer = 0;

  if (!(handle->play_pipeline))
    return FALSE;

  if (!gst_element_query_position(handle->play_pipeline, GST_FORMAT_TIME, &pos))
    return FALSE;

  if (!gst_element_query_duration(handle->play_pipeline, GST_FORMAT_TIME, &len))
    return FALSE;

  if (pos < len)
    gtk_progress_bar_set_fraction(
      handle->recording_progress_bar,
      1.0 * pos / len
    );
  else
    gtk_progress_bar_set_fraction(
      handle->recording_progress_bar,
      1.0
    );

  if (handle->playing)
    handle->play_timer = util_timeout_add(
      10,
      _play_timer_func,
      handle
    );

  return FALSE;
}

static gboolean
handle_record_bus_watch(UNUSED GstBus *bus,
                        GstMessage *msg,
                        gpointer data)
{
  g_assert((msg) && (data));

  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) data;
  GstMessageType type = GST_MESSAGE_TYPE(msg);

  switch (type)
  {
    case GST_MESSAGE_STREAM_START:
      handle->record_time = 0;
      handle->record_timer = util_idle_add(
        _record_timer_func,
        handle
      );

      break;
    default:
      break;
  }

  return TRUE;
}

static gboolean
handle_play_bus_watch(UNUSED GstBus *bus,
                      GstMessage *msg,
                      gpointer data)
{
  UI_CHAT_Handle *handle = (UI_CHAT_Handle*) data;
  GstMessageType type = GST_MESSAGE_TYPE(msg);

  switch (type)
  {
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

      if (GST_STATE_PLAYING == new_state)
        handle->play_timer = util_idle_add(
          _play_timer_func,
          handle
        );
      else if (GST_STATE_PLAYING == old_state)
        _stop_playing_recording(handle, FALSE);
      break;
    }
    case GST_MESSAGE_EOS:
      if (handle->playing)
	      _stop_playing_recording(handle, FALSE);
      break;
    default:
      break;
  }

  return TRUE;
}

static void
_setup_gst_pipelines(UI_CHAT_Handle *handle)
{
  g_assert(handle);

  handle->record_pipeline = gst_parse_launch(
    "autoaudiosrc ! audioconvert ! vorbisenc ! oggmux ! filesink name=sink",
    NULL
  );

  handle->record_sink = gst_bin_get_by_name(
    GST_BIN(handle->record_pipeline), "sink"
  );

  {
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(handle->record_pipeline));

    handle->record_watch = gst_bus_add_watch(
      bus,
      handle_record_bus_watch,
      handle
    );

    gst_object_unref(bus);
  }

  handle->play_pipeline = gst_element_factory_make("playbin", NULL);
  handle->play_sink = gst_element_factory_make("autoaudiosink", "asink");

  if ((!(handle->play_pipeline)) || (!(handle->play_sink)))
    return;

  g_object_set(
      G_OBJECT(handle->play_pipeline),
      "audio-sink",
      handle->play_sink,
      NULL
  );

  {
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(handle->play_pipeline));

    handle->play_watch = gst_bus_add_watch(
      bus,
      handle_play_bus_watch,
      handle
    );

    gst_object_unref(bus);
  }
}

UI_CHAT_Handle*
ui_chat_new(MESSENGER_Application *app,
            struct GNUNET_CHAT_Context *context)
{
  g_assert((app) && (context));

  UI_CHAT_Handle *handle = g_malloc(sizeof(UI_CHAT_Handle));

  memset(handle, 0, sizeof(*handle));

  _setup_gst_pipelines(handle);

  handle->app = app;
  handle->context = context;

  handle->title = ui_chat_title_new(handle->app, handle);

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/chat.ui")
  );

  handle->chat_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "chat_box")
  );

  handle->flap_chat_details = HDY_FLAP(
    gtk_builder_get_object(handle->builder, "flap_chat_details")
  );

  g_signal_connect(
    handle->flap_chat_details,
    "notify::reveal-flap",
    G_CALLBACK(handle_chat_details_folded),
    handle
  );

  handle->chat_search_bar = HDY_SEARCH_BAR(
    gtk_builder_get_object(handle->builder, "chat_search_bar")
  );

  handle->chat_search_entry = GTK_SEARCH_ENTRY(
    gtk_builder_get_object(handle->builder, "chat_search_entry")
  );

  handle->chat_details_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "chat_details_label")
  );

  handle->hide_chat_details_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "hide_chat_details_button")
  );

  g_signal_connect(
    handle->hide_chat_details_button,
    "clicked",
    G_CALLBACK(handle_chat_details_via_button_click),
    handle
  );

  handle->chat_details_contacts_box = GTK_BOX(
    gtk_builder_get_object(handle->builder, "chat_details_contacts_box")
  );

  handle->chat_details_files_box = GTK_BOX(
    gtk_builder_get_object(handle->builder, "chat_details_files_box")
  );

  handle->chat_details_media_box = GTK_BOX(
    gtk_builder_get_object(handle->builder, "chat_details_media_box")
  );

  handle->chat_details_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder, "chat_details_avatar")
  );

  handle->reveal_identity_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "reveal_identity_button")
  );

  g_signal_connect(
    handle->reveal_identity_button,
    "clicked",
    G_CALLBACK(handle_reveal_identity_button_click),
    handle
  );

  handle->discourse_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "discourse_button")
  );

  g_signal_connect(
    handle->discourse_button,
    "clicked",
    G_CALLBACK(handle_discourse_button_click),
    handle
  );

  handle->block_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "block_stack")
  );

  handle->block_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "block_button")
  );

  g_signal_connect(
    handle->block_button,
    "clicked",
    G_CALLBACK(handle_block_button_click),
    handle
  );

  handle->unblock_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "unblock_button")
  );

  g_signal_connect(
    handle->unblock_button,
    "clicked",
    G_CALLBACK(handle_unblock_button_click),
    handle
  );

  handle->leave_chat_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "leave_chat_button")
  );

  g_signal_connect(
    handle->leave_chat_button,
    "clicked",
    G_CALLBACK(handle_leave_chat_button_click),
    handle
  );

  handle->chat_notifications_switch = GTK_SWITCH(
    gtk_builder_get_object(handle->builder, "chat_notifications_switch")
  );

  handle->chat_scrolled_window = GTK_SCROLLED_WINDOW(
    gtk_builder_get_object(handle->builder, "chat_scrolled_window")
  );

  handle->chat_contacts_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "chat_contacts_listbox")
  );

  g_signal_connect(
    handle->chat_contacts_listbox,
    "row-activated",
    G_CALLBACK(handle_chat_contacts_listbox_row_activated),
    handle
  );

  handle->chat_files_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "chat_files_listbox")
  );

  handle->chat_media_flowbox = GTK_FLOW_BOX(
    gtk_builder_get_object(handle->builder, "chat_media_flowbox")
  );

  handle->messages_listbox = GTK_LIST_BOX(
    gtk_builder_get_object(handle->builder, "messages_listbox")
  );

  gtk_list_box_set_sort_func(
    handle->messages_listbox,
    handle_chat_messages_sort,
    app,
    NULL
  );

  gtk_list_box_set_filter_func(
    handle->messages_listbox,
    handle_chat_messages_filter,
    app,
    NULL
  );

  g_signal_connect(
    handle->chat_search_entry,
    "search-changed",
    G_CALLBACK(handle_search_entry_search_changed),
    handle->messages_listbox
  );

  g_signal_connect(
    handle->messages_listbox,
    "selected-rows-changed",
    G_CALLBACK(handle_chat_messages_selected_rows_changed),
    handle->title
  );

  g_signal_connect(
    handle->messages_listbox,
    "size-allocate",
    G_CALLBACK(handle_chat_messages_listbox_size_allocate),
    handle
  );

  handle->send_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "send_stack")
  );

  handle->send_text_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "send_text_box")
  );

  handle->send_recording_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "send_recording_box")
  );

  handle->attach_file_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "attach_file_button")
  );

  g_signal_connect(
    handle->attach_file_button,
    "clicked",
    G_CALLBACK(handle_attach_file_button_click),
    app
  );

  handle->send_text_view = GTK_TEXT_VIEW(
    gtk_builder_get_object(handle->builder, "send_text_view")
  );

  handle->emoji_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "emoji_button")
  );

  handle->send_record_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "send_record_button")
  );

  handle->send_record_symbol = GTK_IMAGE(
    gtk_builder_get_object(handle->builder, "send_record_symbol")
  );

  handle->send_popover = GTK_POPOVER(
    gtk_builder_get_object(handle->builder, "send_popover")
  );

  handle->send_now_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "send_now_button")
  );

  handle->send_later_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "send_later_button")
  );

  GtkTextBuffer *send_text_buffer = gtk_text_view_get_buffer(
    handle->send_text_view
  );

  g_signal_connect(
    send_text_buffer,
    "changed",
    G_CALLBACK(handle_send_text_buffer_changed),
    handle
  );

  g_signal_connect(
    handle->send_record_button,
    "clicked",
    G_CALLBACK(handle_send_record_button_click),
    app
  );

  g_signal_connect(
    handle->send_later_button,
    "clicked",
    G_CALLBACK(handle_send_later_button_click),
    app
  );

  g_signal_connect(
    handle->send_now_button,
    "clicked",
    G_CALLBACK(handle_send_now_button_click),
    app
  );

  g_signal_connect(
    handle->send_record_button,
    "button-press-event",
    G_CALLBACK(handle_send_record_button_pressed),
    app
  );

  g_signal_connect(
    handle->send_record_button,
    "button-release-event",
    G_CALLBACK(handle_send_record_button_released),
    app
  );

  g_signal_connect(
    handle->send_text_view,
    "key-press-event",
    G_CALLBACK(handle_send_text_key_press),
    app
  );

  g_object_set_qdata(
    G_OBJECT(handle->chat_contacts_listbox),
    app->quarks.widget,
    handle->send_text_view
  );

  g_object_set_qdata(
    G_OBJECT(handle->attach_file_button),
    app->quarks.widget,
    handle->send_text_view
  );

  g_object_set_qdata(
    G_OBJECT(handle->send_record_button),
    app->quarks.widget,
    handle->send_text_view
  );

  g_object_set_qdata(
    G_OBJECT(handle->send_text_view),
    app->quarks.data,
    context
  );

  g_object_set_qdata(
    G_OBJECT(handle->send_text_view),
    app->quarks.ui,
    handle
  );

  g_object_set_qdata(
    G_OBJECT(handle->send_record_button),
    app->quarks.ui,
    handle
  );

  g_object_set_qdata(
    G_OBJECT(handle->send_later_button),
    app->quarks.ui,
    handle
  );

  g_object_set_qdata(
    G_OBJECT(handle->send_now_button),
    app->quarks.ui,
    handle
  );

  g_object_set_qdata(
    G_OBJECT(handle->messages_listbox),
    app->quarks.ui,
    handle
  );

  handle->recording_close_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "recording_close_button")
  );

  g_signal_connect(
    handle->recording_close_button,
    "clicked",
    G_CALLBACK(handle_recording_close_button_click),
    handle
  );

  handle->recording_play_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "recording_play_button")
  );

  g_signal_connect(
    handle->recording_play_button,
    "clicked",
    G_CALLBACK(handle_recording_play_button_click),
    handle
  );

  handle->play_pause_symbol = GTK_IMAGE(
    gtk_builder_get_object(handle->builder, "play_pause_symbol")
  );

  handle->recording_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "recording_label")
  );

  handle->recording_progress_bar = GTK_PROGRESS_BAR(
    gtk_builder_get_object(handle->builder, "recording_progress_bar")
  );

  handle->picker_revealer = GTK_REVEALER(
    gtk_builder_get_object(handle->builder, "picker_revealer")
  );

  handle->picker = ui_picker_new(app, handle);

  gtk_container_add(
    GTK_CONTAINER(handle->picker_revealer),
    handle->picker->picker_box
  );

  g_signal_connect(
    handle->emoji_button,
    "clicked",
    G_CALLBACK(handle_picker_button_click),
    handle
  );

  return handle;
}

struct IterateChatClosure {
  MESSENGER_Application *app;
  GtkContainer *container;
};

static enum GNUNET_GenericReturnValue
iterate_ui_chat_update_group_contacts(void *cls,
                                      UNUSED struct GNUNET_CHAT_Group *group,
                                      struct GNUNET_CHAT_Contact *contact)
{
  struct IterateChatClosure *closure = (
    (struct IterateChatClosure*) cls
  );

  GtkListBox *listbox = GTK_LIST_BOX(closure->container);
  UI_ACCOUNT_ENTRY_Handle* entry = ui_account_entry_new(closure->app);

  ui_account_entry_set_contact(entry, contact);

  gtk_list_box_prepend(listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), closure->app->quarks.data, contact);
  g_object_set_qdata_full(
    G_OBJECT(row),
    closure->app->quarks.ui,
    entry,
    (GDestroyNotify) ui_account_entry_delete
  );

  return GNUNET_YES;
}

static void
_chat_update_contacts(UI_CHAT_Handle *handle,
                      MESSENGER_Application *app,
                      struct GNUNET_CHAT_Group* group)
{
  g_assert((handle) && (app));

  GList* children = gtk_container_get_children(
    GTK_CONTAINER(handle->chat_contacts_listbox)
  );

  GList *item = children;
  while ((item) && (item->next)) {
    GtkWidget *widget = GTK_WIDGET(item->data);
    item = item->next;

    gtk_container_remove(
      GTK_CONTAINER(handle->chat_contacts_listbox),
      widget
    );
  }

  if (children)
    g_list_free(children);

  if (group)
  {
    struct IterateChatClosure closure;
    closure.app = app;
    closure.container = GTK_CONTAINER(handle->chat_contacts_listbox);

    GNUNET_CHAT_group_iterate_contacts(
	    group,
      iterate_ui_chat_update_group_contacts,
      &closure
    );
  }

  gtk_widget_set_visible(
    GTK_WIDGET(handle->chat_details_contacts_box),
    group? TRUE : FALSE
  );
}

static enum GNUNET_GenericReturnValue
iterate_ui_chat_update_context_files(void *cls,
                                     struct GNUNET_CHAT_Context *context,
                                     struct GNUNET_CHAT_File *file)
{
  struct IterateChatClosure *closure = (
    (struct IterateChatClosure*) cls
  );

  GtkListBox *listbox = GTK_LIST_BOX(closure->container);
  UI_FILE_ENTRY_Handle* entry = ui_file_entry_new(closure->app);
  ui_file_entry_update(entry, file);

  gtk_list_box_prepend(listbox, entry->entry_box);

  GtkListBoxRow *row = GTK_LIST_BOX_ROW(
    gtk_widget_get_parent(entry->entry_box)
  );

  g_object_set_qdata(G_OBJECT(row), closure->app->quarks.data, file);
  g_object_set_qdata_full(
    G_OBJECT(row),
    closure->app->quarks.ui,
    entry,
    (GDestroyNotify) ui_file_entry_delete
  );

  return GNUNET_YES;
}

static void
_chat_update_files(UI_CHAT_Handle *handle,
                   MESSENGER_Application *app,
                   struct GNUNET_CHAT_Context *context)
{
  g_assert((handle) && (app));

  GList* children = gtk_container_get_children(
    GTK_CONTAINER(handle->chat_files_listbox)
  );

  GList *item = children;
  while (item) {
    GtkWidget *widget = GTK_WIDGET(item->data);
    item = item->next;

    gtk_container_remove(
      GTK_CONTAINER(handle->chat_files_listbox),
      widget
    );
  }

  if (children)
    g_list_free(children);

  struct IterateChatClosure closure;
  closure.app = app;
  closure.container = GTK_CONTAINER(handle->chat_files_listbox);

  const int count = context? GNUNET_CHAT_context_iterate_files(
    context,
    iterate_ui_chat_update_context_files,
    &closure
  ) : 0;

  gtk_widget_set_visible(
    GTK_WIDGET(handle->chat_details_files_box),
    count? TRUE : FALSE
  );
}

static enum GNUNET_GenericReturnValue
iterate_ui_chat_update_context_media(void *cls,
                                     struct GNUNET_CHAT_Context *context,
                                     struct GNUNET_CHAT_File *file)
{
  struct IterateChatClosure *closure = (
    (struct IterateChatClosure*) cls
  );

  GtkFlowBox *flowbox = GTK_FLOW_BOX(closure->container);
  UI_MEDIA_PREVIEW_Handle* handle = ui_media_preview_new(closure->app);
  ui_media_preview_update(handle, file);

  GdkPixbuf *image = file_get_current_preview_image(file);

  if (!image)
  {
    ui_media_preview_delete(handle);
    return GNUNET_YES;
  }

  gtk_flow_box_insert(flowbox, handle->media_box, 0);

  GtkFlowBoxChild *child = GTK_FLOW_BOX_CHILD(
    gtk_widget_get_parent(handle->media_box)
  );

  g_object_set_qdata(G_OBJECT(child), closure->app->quarks.data, file);
  g_object_set_qdata(
    G_OBJECT(child),
    closure->app->quarks.ui,
    handle
  );

  gtk_widget_set_size_request(GTK_WIDGET(child), 80, 80);

  gtk_widget_show_all(GTK_WIDGET(child));
  return GNUNET_YES;
}

static void
_chat_update_media(UI_CHAT_Handle *handle,
                   MESSENGER_Application *app,
                   struct GNUNET_CHAT_Context *context)
{
  g_assert((handle) && (app));

  GList* children = gtk_container_get_children(
    GTK_CONTAINER(handle->chat_media_flowbox)
  );

  GList *item = children;
  while (item) {
    GtkWidget *widget = GTK_WIDGET(item->data);
    item = item->next;

    UI_MEDIA_PREVIEW_Handle *media = g_object_get_qdata(
      G_OBJECT(widget),
      app->quarks.ui
    );

    ui_media_preview_delete(media);

    gtk_container_remove(
      GTK_CONTAINER(handle->chat_media_flowbox),
      widget
    );
  }

  if (children)
    g_list_free(children);

  struct IterateChatClosure closure;
  closure.app = app;
  closure.container = GTK_CONTAINER(handle->chat_media_flowbox);

  const int count = context? GNUNET_CHAT_context_iterate_files(
    context,
    iterate_ui_chat_update_context_media,
    &closure
  ) : 0;

  gtk_widget_set_visible(
    GTK_WIDGET(handle->chat_details_media_box),
    count? TRUE : FALSE
  );
}

static const gchar*
_chat_get_default_subtitle(UI_CHAT_Handle *handle,
                           MESSENGER_Application *app,
                           struct GNUNET_CHAT_Group* group)
{
  g_assert((handle) && (app));

  GList *rows = gtk_container_get_children(
    GTK_CONTAINER(handle->messages_listbox)
  );

  if (!rows)
    return NULL;

  UI_MESSAGE_Handle *last_message = NULL;
  for (GList *row = rows; row; row = row->next)
  {
    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row->data), app->quarks.ui
    );

    if (!message)
      continue;

    ui_message_refresh(message);
    last_message = message;
  }

  g_list_free(rows);

  if ((!last_message) || (!(last_message->timestamp_label)))
    return NULL;

  return group? NULL : gtk_label_get_text(last_message->timestamp_label);
}

void
ui_chat_update(UI_CHAT_Handle *handle,
               MESSENGER_Application *app)
{
  g_assert((handle) && (app));

  struct GNUNET_CHAT_Contact* contact;
  struct GNUNET_CHAT_Group* group;

  contact = GNUNET_CHAT_context_get_contact(handle->context);
  group = GNUNET_CHAT_context_get_group(handle->context);

  _chat_update_contacts(handle, app, group);
  _chat_update_files(handle, app, handle->context);
  _chat_update_media(handle, app, handle->context);

  const gchar *subtitle = _chat_get_default_subtitle(handle, app, group);

  ui_chat_title_update(handle->title, app, subtitle);

  g_object_set_qdata(
    G_OBJECT(handle->reveal_identity_button),
    app->quarks.data,
    contact
  );

  gtk_widget_set_visible(
    GTK_WIDGET(handle->reveal_identity_button),
    contact? TRUE : FALSE
  );

  g_object_set_qdata(
    G_OBJECT(handle->block_stack),
    app->quarks.data,
    contact
  );

  if (GNUNET_YES == GNUNET_CHAT_contact_is_blocked(contact))
    gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->unblock_button));
  else
    gtk_stack_set_visible_child(handle->block_stack, GTK_WIDGET(handle->block_button));

  gtk_widget_set_visible(
    GTK_WIDGET(handle->block_stack),
    contact? TRUE : FALSE
  );

  gtk_widget_set_sensitive(
    GTK_WIDGET(handle->leave_chat_button),
    (contact) || (group)? TRUE : FALSE
  );

  const int status = GNUNET_CHAT_context_get_status(handle->context);
  const gboolean activated = (GNUNET_OK == status? TRUE : FALSE);

  gtk_text_view_set_editable(handle->send_text_view, activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->send_text_view), activated);

  gtk_widget_set_sensitive(GTK_WIDGET(handle->attach_file_button), activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->emoji_button), activated);
  gtk_widget_set_sensitive(GTK_WIDGET(handle->send_record_button), activated);
}

void
ui_chat_delete(UI_CHAT_Handle *handle)
{
  g_assert(handle);

  GList *message_rows = gtk_container_get_children(GTK_CONTAINER(handle->messages_listbox));
  GList *row_element = message_rows;

  while (row_element)
  {
    GtkWidget *row = GTK_WIDGET(row_element->data);

    if (!row)
      goto skip_row;

    UI_MESSAGE_Handle *message = (UI_MESSAGE_Handle*) g_object_get_qdata(
      G_OBJECT(row), handle->app->quarks.ui
    );

    if (message)
      ui_chat_remove_message(handle, handle->app, message);

  skip_row:
    row_element = row_element->next;
  }

  if (message_rows)
    g_list_free(message_rows);

  ui_picker_delete(handle->picker);

  _chat_update_contacts(handle, handle->app, NULL);
  _chat_update_media(handle, handle->app, NULL);
  _chat_update_files(handle, handle->app, NULL);

  ui_chat_title_delete(handle->title);

  g_object_unref(handle->builder);

  if (handle->record_pipeline)
  {
    gst_element_set_state(handle->record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->record_pipeline));
  }

  if (handle->play_pipeline)
  {
    gst_element_set_state(handle->play_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->play_pipeline));
  }

  if (handle->recording_filename[0])
    remove(handle->recording_filename);

  if (handle->record_timer)
    util_source_remove(handle->record_timer);

  if (handle->play_timer)
    util_source_remove(handle->play_timer);

  g_free(handle);
}

void
ui_chat_add_message(UI_CHAT_Handle *handle,
                    MESSENGER_Application *app,
                    UI_MESSAGE_Handle *message)
{
  g_assert((handle) && (message) && (message->message_box));

  gtk_container_add(
    GTK_CONTAINER(handle->messages_listbox),
    message->message_box
  );

  GtkWidget *row = gtk_widget_get_parent(message->message_box);
  g_object_set_qdata(G_OBJECT(row), app->quarks.ui, message);

  gtk_list_box_invalidate_sort(handle->messages_listbox);
}

void
ui_chat_remove_message(UI_CHAT_Handle *handle,
                       MESSENGER_Application *app,
                       UI_MESSAGE_Handle *message)
{
  g_assert((handle) && (message) && (message->message_box));

  GtkWidget *row = gtk_widget_get_parent(message->message_box);
  g_object_set_qdata(G_OBJECT(row), app->quarks.ui, NULL);

  GtkWidget *parent = gtk_widget_get_parent(row);

  if (parent == GTK_WIDGET(handle->messages_listbox))
    gtk_container_remove(GTK_CONTAINER(handle->messages_listbox), row);

  ui_message_delete(message, app);
}
