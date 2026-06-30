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
 * @file ui/message.c
 */

#include "message.h"

#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_common.h>

#include "tag.h"

#include "../application.h"
#include "../contact.h"
#include "../file.h"
#include "../ui.h"

static void
handle_downloading_file(void *cls,
                        struct GNUNET_CHAT_File *file,
                        uint64_t completed,
                        uint64_t size)
{
  g_assert((cls) && (file));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (!app)
    return;

  file_update_download_info(file, app, completed, size);
}

static void
handle_file_button_click(GtkButton *button,
                         gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle* handle = (UI_MESSAGE_Handle*) (
    g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if (!handle)
    return;

  struct GNUNET_CHAT_File *file = (struct GNUNET_CHAT_File*) (
    g_object_get_qdata(G_OBJECT(handle->file_progress_bar), app->quarks.data)
  );

  if (!file)
    return;

  application_chat_lock(app);
  uint64_t size = GNUNET_CHAT_file_get_size(file);
  application_chat_unlock(app);

  if (size <= 0)
    return;

  application_chat_lock(app);

  uint64_t local_size = GNUNET_CHAT_file_get_local_size(file);
  const gboolean downloading = (GNUNET_YES == GNUNET_CHAT_file_is_downloading(file));

  application_chat_unlock(app);

  if (downloading)
  {
    application_chat_lock(app);
    GNUNET_CHAT_file_stop_download(file);
    application_chat_unlock(app);

    gtk_image_set_from_icon_name(
      handle->file_status_image,
      "folder-download-symbolic",
      GTK_ICON_SIZE_BUTTON
    );
  }
  else if (local_size < size)
  {
    application_chat_lock(app);
    GNUNET_CHAT_file_start_download(
      file,
      handle_downloading_file,
      app
    );
    application_chat_unlock(app);

    gtk_image_set_from_icon_name(
    	handle->file_status_image,
    	"process-stop-symbolic",
    	GTK_ICON_SIZE_BUTTON
    );
  }
  else if (size > 0)
  {
    application_chat_lock(app);
    const gchar *preview = GNUNET_CHAT_file_open_preview(file);
    application_chat_unlock(app);

    if (!preview)
      return;

    GString* uri = g_string_new("file://");
    g_string_append(uri, preview);

    if (!g_app_info_launch_default_for_uri(uri->str, NULL, NULL))
    {
      application_chat_lock(app);
      GNUNET_CHAT_file_close_preview(file);
      application_chat_unlock(app);
    }

    g_string_free(uri, TRUE);
  }
}

static void
handle_accept_button_click(GtkButton *button,
                           gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle* handle = (UI_MESSAGE_Handle*) (
      g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if ((!handle) || (!(handle->status_cb)))
    return;

  handle->status_cb(app, true, handle->status_cls);
}

static void
handle_deny_button_click(GtkButton *button,
                         gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle* handle = (UI_MESSAGE_Handle*) (
      g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if ((!handle) || (!(handle->status_cb)))
    return;

  handle->status_cb(app, false, handle->status_cls);
}

static void
handle_media_button_click(GtkButton *button,
                          gpointer user_data)
{
  g_assert((button) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  UI_MESSAGE_Handle* handle = (UI_MESSAGE_Handle*) (
      g_object_get_qdata(G_OBJECT(button), app->quarks.ui)
  );

  if (!handle)
    return;

  struct GNUNET_CHAT_File *file = (struct GNUNET_CHAT_File*) (
      g_object_get_qdata(G_OBJECT(handle->media_progress_bar), app->quarks.data)
  );

  if (!file)
    return;

  application_chat_lock(app);
  const gchar *preview = GNUNET_CHAT_file_open_preview(file);
  application_chat_unlock(app);

  if (!preview)
    return;

  ui_play_media_window_init(app, &(app->ui.play_media));

  GString* uri = g_string_new("file://");
  g_string_append(uri, preview);

  ui_play_media_window_update(
      &(app->ui.play_media),
      uri->str,
      file
  );

  gtk_widget_show(GTK_WIDGET(app->ui.play_media.window));
  g_string_free(uri, TRUE);
}

static gboolean
handle_preview_drawing_area_draw(GtkWidget* drawing_area,
                                 cairo_t* cairo,
                                 gpointer user_data)
{
  g_assert((drawing_area) && (cairo) && (user_data));

  UI_MESSAGE_Handle *handle = (UI_MESSAGE_Handle*) user_data;

  GtkStyleContext* context = gtk_widget_get_style_context(drawing_area);

  const guint width = gtk_widget_get_allocated_width(drawing_area);
  const guint height = gtk_widget_get_allocated_height(drawing_area);

  gtk_render_background(context, cairo, 0, 0, width, height);

  struct GNUNET_CHAT_File *file = (struct GNUNET_CHAT_File *) g_object_get_qdata(
    G_OBJECT(handle->message_box),
    handle->app->quarks.data
  );

  if (!file)
    return FALSE;

  GdkPixbuf *image = file_get_current_preview_image(file);

  if (!image)
    return FALSE;

  int dwidth = gdk_pixbuf_get_width(image);
  int dheight = gdk_pixbuf_get_height(image);

  gint optimal_height = width * dheight / dwidth;

  gtk_widget_set_size_request(
    GTK_WIDGET(drawing_area),
    width,
    optimal_height
  );

  double ratio_width = 1.0 * width / dwidth;
  double ratio_height = 1.0 * height / dheight;

  const double ratio = ratio_width < ratio_height? ratio_width : ratio_height;

  dwidth = (int) (dwidth * ratio);
  dheight = (int) (dheight * ratio);

  double dx = (width - dwidth) * 0.5;
  double dy = (height - dheight) * 0.5;

  const int interp_type = (ratio >= 1.0?
    GDK_INTERP_NEAREST :
    GDK_INTERP_BILINEAR
  );

  GdkPixbuf* scaled = gdk_pixbuf_scale_simple(
    image,
    dwidth,
    dheight,
    interp_type
  );

  gtk_render_icon(context, cairo, scaled, dx, dy);

  cairo_fill(cairo);

  g_object_unref(scaled);
  return FALSE;
}

UI_MESSAGE_Handle*
ui_message_new(MESSENGER_Application *app,
               UI_MESSAGE_Type type)
{
  g_assert(app);

  UI_MESSAGE_Handle* handle = g_malloc(sizeof(UI_MESSAGE_Handle));

  handle->type = type;

  handle->timestamp = ((time_t) -1);
  handle->msg = NULL;
  handle->contact = NULL;

  handle->status_cb = NULL;
  handle->status_cls = NULL;

  const char *ui_builder_file;

  switch (handle->type)
  {
    case UI_MESSAGE_SENT:
      ui_builder_file = "ui/message-sent.ui";
      break;
    case UI_MESSAGE_STATUS:
      ui_builder_file = "ui/message-status.ui";
      break;
    default:
      ui_builder_file = "ui/message.ui";
      break;
  }

  handle->builder[0] = ui_builder_from_resource(
    application_get_resource_path(app, ui_builder_file)
  );

  handle->message_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder[0], "message_box")
  );

  handle->sender_avatar = HDY_AVATAR(
    gtk_builder_get_object(handle->builder[0], "sender_avatar")
  );

  handle->sender_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder[0], "sender_label")
  );

  handle->private_image = GTK_IMAGE(
    gtk_builder_get_object(handle->builder[0], "private_image")
  );

  if (UI_MESSAGE_STATUS == handle->type)
  {
    handle->deny_revealer = GTK_REVEALER(
	    gtk_builder_get_object(handle->builder[0], "deny_revealer")
    );

    handle->accept_revealer = GTK_REVEALER(
    	gtk_builder_get_object(handle->builder[0], "accept_revealer")
    );

    handle->deny_button = GTK_BUTTON(
	    gtk_builder_get_object(handle->builder[0], "deny_button")
    );

    handle->accept_button = GTK_BUTTON(
	    gtk_builder_get_object(handle->builder[0], "accept_button")
    );

    g_object_set_qdata(G_OBJECT(handle->accept_button), app->quarks.ui, handle);
    g_object_set_qdata(G_OBJECT(handle->deny_button), app->quarks.ui, handle);

    g_signal_connect(
      handle->accept_button,
      "clicked",
      G_CALLBACK(handle_accept_button_click),
      app
    );

    g_signal_connect(
      handle->deny_button,
      "clicked",
      G_CALLBACK(handle_deny_button_click),
      app
    );
  }
  else
  {
    handle->deny_revealer = NULL;
    handle->accept_revealer = NULL;

    handle->deny_button = NULL;
    handle->accept_button = NULL;
  }

  GtkContainer *content_box = GTK_CONTAINER(
    gtk_builder_get_object(handle->builder[0], "content_box")
  );

  handle->tag_flow_box = GTK_FLOW_BOX(
    gtk_builder_get_object(handle->builder[0], "tag_flow_box")
  );

  handle->builder[1] = ui_builder_from_resource(
    application_get_resource_path(app, "ui/message_content.ui")
  );

  handle->timestamp_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder[1], "timestamp_label")
  );

  handle->read_receipt_image = GTK_IMAGE(
    gtk_builder_get_object(handle->builder[1], "read_receipt_image")
  );

  handle->content_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder[1], "content_stack")
  );

  handle->text_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder[1], "text_label")
  );

  handle->file_revealer = GTK_REVEALER(
    gtk_builder_get_object(handle->builder[1], "file_revealer")
  );

  handle->filename_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder[1], "filename_label")
  );

  handle->file_progress_bar = GTK_PROGRESS_BAR(
    gtk_builder_get_object(handle->builder[1], "file_progress_bar")
  );

  handle->file_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder[1], "file_button")
  );

  g_signal_connect(
    handle->file_button,
    "clicked",
    G_CALLBACK(handle_file_button_click),
    app
  );

  handle->file_status_image = GTK_IMAGE(
    gtk_builder_get_object(handle->builder[1], "file_status_image")
  );

  g_object_set_qdata(G_OBJECT(handle->file_button), app->quarks.ui, handle);

  handle->preview_drawing_area = GTK_DRAWING_AREA(
    gtk_builder_get_object(handle->builder[1], "preview_drawing_area")
  );

  g_signal_connect(
    handle->preview_drawing_area,
    "draw",
    G_CALLBACK(handle_preview_drawing_area_draw),
    handle
  );

  handle->media_revealer = GTK_REVEALER(
    gtk_builder_get_object(handle->builder[1], "media_revealer")
  );

  handle->media_type_image = GTK_IMAGE(
    gtk_builder_get_object(handle->builder[1], "media_type_image")
  );

  handle->media_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder[1], "media_label")
  );

  handle->media_progress_bar = GTK_PROGRESS_BAR(
    gtk_builder_get_object(handle->builder[1], "media_progress_bar")
  );

  handle->media_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder[1], "media_button")
  );

  handle->app = app;

  g_object_set_qdata(G_OBJECT(handle->media_button), app->quarks.ui, handle);

  g_signal_connect(
    handle->media_button,
    "clicked",
    G_CALLBACK(handle_media_button_click),
    app
  );

  switch (handle->type)
  {
    case UI_MESSAGE_STATUS:
      gtk_widget_set_visible(GTK_WIDGET(handle->timestamp_label), FALSE);
      break;
    default:
      break;
  }

  gtk_container_add(content_box, GTK_WIDGET(
    gtk_builder_get_object(handle->builder[1], "message_content_box")
  ));

  return handle;
}

static int
_iterate_read_receipts(void *cls,
                       UNUSED struct GNUNET_CHAT_Message *message,
                       struct GNUNET_CHAT_Contact *contact,
                       int read_receipt)
{
  g_assert((cls) && (message) && (contact));

  int *count_read_receipts = (int*) cls;

  if ((GNUNET_YES == read_receipt) &&
      (GNUNET_NO == GNUNET_CHAT_contact_is_owned(contact)))
    (*count_read_receipts)++;

  return GNUNET_YES;
}

void
ui_message_refresh(UI_MESSAGE_Handle *handle)
{
  g_assert(handle);

  if ((!(handle->msg)) ||
      (GNUNET_YES != GNUNET_CHAT_message_is_sent(handle->msg)))
    return;

  if (!(handle->read_receipt_image))
    return;

  int count = 0;
  if ((0 < GNUNET_CHAT_message_get_read_receipt(handle->msg, _iterate_read_receipts, &count)) &&
      (0 < count))
    gtk_widget_show(GTK_WIDGET(handle->read_receipt_image));
  else
    gtk_widget_hide(GTK_WIDGET(handle->read_receipt_image));
}

gboolean
_message_media_supports_file_extension(const gchar *filename)
{
  if (!filename)
    return FALSE;

  const char* extension = strrchr(filename, '.');

  if (!extension)
    return FALSE;

  if (0 == g_strcmp0(extension, ".ogg"))
    return TRUE;
  if (0 == g_strcmp0(extension, ".mp3"))
    return TRUE;
  if (0 == g_strcmp0(extension, ".wav"))
    return TRUE;

  return FALSE;
}

static void
_update_invitation_message(UI_MESSAGE_Handle *handle,
                           MESSENGER_Application *app,
                           struct GNUNET_CHAT_Invitation *invitation)
{
  g_assert((handle) && (app) && (invitation));

  enum GNUNET_GenericReturnValue accepted, rejected;
  accepted = GNUNET_CHAT_invitation_is_accepted(invitation);
  rejected = GNUNET_CHAT_invitation_is_rejected(invitation);

  if (handle->deny_button)
    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->deny_button), 
      GNUNET_YES == accepted || GNUNET_YES == rejected? FALSE : TRUE
    );

  if (handle->accept_button)
    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->accept_button), 
      GNUNET_YES == accepted || GNUNET_YES == rejected? FALSE : TRUE
    );

  if (handle->deny_revealer)
    gtk_revealer_set_reveal_child(
      GTK_REVEALER(handle->deny_revealer),
      GNUNET_YES == accepted || GNUNET_YES == rejected? FALSE : TRUE
    );

  if (handle->accept_revealer)
    gtk_revealer_set_reveal_child(
      GTK_REVEALER(handle->accept_revealer),
      GNUNET_YES == accepted || GNUNET_YES == rejected? FALSE : TRUE
    );

  if ((app->settings.accept_all_invitations) &&
      (GNUNET_NO == accepted) && (handle->accept_button))
    gtk_button_clicked(handle->accept_button);
}

static void
_update_message_with_file(UI_MESSAGE_Handle *handle,
                          MESSENGER_Application *app,
                          struct GNUNET_CHAT_File *file)
{
  g_assert(handle);

  struct GNUNET_CHAT_File *prev = g_object_get_qdata(
    G_OBJECT(handle->preview_drawing_area),
    app->quarks.data
  );

  if (prev)
    file_remove_widget_from_preview(file, GTK_WIDGET(handle->preview_drawing_area));
  if (file)
    file_add_widget_to_preview(file, GTK_WIDGET(handle->preview_drawing_area));

  g_object_set_qdata(
    G_OBJECT(handle->preview_drawing_area),
    app->quarks.data,
    file
  );
}

static void
_update_file_message(UI_MESSAGE_Handle *handle,
                     MESSENGER_Application *app,
                     struct GNUNET_CHAT_File *file)
{
  g_assert((handle) && (app) && (file));

  const char *filename = GNUNET_CHAT_file_get_name(file);

  uint64_t size = GNUNET_CHAT_file_get_size(file);
  uint64_t local_size = GNUNET_CHAT_file_get_local_size(file);

  gboolean autostart_download = FALSE;

  if ((size <= 0) || (size > local_size))
  {
    gtk_image_set_from_icon_name(
      handle->file_status_image,
      "folder-download-symbolic",
      GTK_ICON_SIZE_BUTTON
    );

    if ((app->settings.accept_all_files) &&
        (GNUNET_YES != GNUNET_CHAT_file_is_downloading(file)))
      autostart_download = TRUE;

    goto file_content;
  }

  if ((!(handle->preview_drawing_area)) ||
      (GNUNET_CHAT_file_get_size(file) != GNUNET_CHAT_file_get_local_size(file)))
    goto file_progress;

  file_load_preview_image(file);

  GdkPixbuf *image = file_get_current_preview_image(file);

  if (image)
  {
    gtk_widget_set_size_request(
      GTK_WIDGET(handle->preview_drawing_area),
      250,
      -1
    );

    gtk_stack_set_visible_child(
      handle->content_stack,
      GTK_WIDGET(handle->preview_drawing_area)
    );

    _update_message_with_file(handle, app, file);
    return;
  }

  if (_message_media_supports_file_extension(filename))
  {
    gtk_image_set_from_icon_name(
      handle->media_type_image,
      "audio-x-generic-symbolic",
      GTK_ICON_SIZE_DND
    );

    goto media_content;
  }

  if (!ui_play_media_window_supports_file_extension(filename))
    goto file_progress;

media_content:
  ui_label_set_text(handle->media_label, filename);

  gtk_stack_set_visible_child(
    handle->content_stack,
    GTK_WIDGET(handle->media_revealer)
  );

  gtk_revealer_set_reveal_child(handle->media_revealer, TRUE);

  g_object_set_qdata(
    G_OBJECT(handle->media_progress_bar),
    app->quarks.data,
    file
  );

  return;

file_progress:
  gtk_progress_bar_set_fraction(handle->file_progress_bar, 1.0);

  gtk_image_set_from_icon_name(
    handle->file_status_image,
    "document-open-symbolic",
    GTK_ICON_SIZE_BUTTON
  );

file_content:
  ui_label_set_text(handle->filename_label, filename);

  gtk_stack_set_visible_child(
    handle->content_stack,
    GTK_WIDGET(handle->file_revealer)
  );

  gtk_revealer_set_reveal_child(handle->file_revealer, TRUE);

  g_object_set_qdata(
    G_OBJECT(handle->file_progress_bar),
    app->quarks.data,
    file
  );

  if (autostart_download)
    gtk_button_clicked(handle->file_button);
}

void
ui_message_update(UI_MESSAGE_Handle *handle,
                  MESSENGER_Application *app,
                  struct GNUNET_CHAT_Message *msg)
{
  g_assert((handle) && (app));

  struct GNUNET_CHAT_File *file = NULL;
  struct GNUNET_CHAT_Invitation *invitation = NULL;

  if (handle->msg)
    GNUNET_CHAT_message_set_user_pointer(handle->msg, NULL);

  handle->msg = msg;

  if (msg)
    GNUNET_CHAT_message_set_user_pointer(msg, handle);

  ui_message_refresh(handle);

  if (msg)
  {
    if (GNUNET_YES == GNUNET_CHAT_message_is_private(msg))
      gtk_widget_show(GTK_WIDGET(handle->private_image));
    
    invitation = GNUNET_CHAT_message_get_invitation(msg);
    file = GNUNET_CHAT_message_get_file(msg);

    handle->timestamp = GNUNET_CHAT_message_get_timestamp(msg);

    if (handle->accept_button)
      g_object_set_qdata(G_OBJECT(handle->accept_button), app->quarks.data, invitation);

    g_object_set_qdata(G_OBJECT(handle->message_box), app->quarks.data, file);
  }
  else
  {
    if (handle->accept_button)
      invitation = (struct GNUNET_CHAT_Invitation*) (
        g_object_get_qdata(G_OBJECT(handle->accept_button), app->quarks.data)
      );

    file = (struct GNUNET_CHAT_File*) (
	    g_object_get_qdata(G_OBJECT(handle->message_box), app->quarks.data)
    );
  }

  if (invitation)
    _update_invitation_message(handle, app, invitation);

  if (file)
    _update_file_message(handle, app, file);
}

void
ui_message_set_contact(UI_MESSAGE_Handle *handle,
                       struct GNUNET_CHAT_Contact *contact)
{
  g_assert(handle);

  if (handle->contact)
  {
    contact_remove_name_avatar_from_info(handle->contact, handle->sender_avatar);
    contact_remove_name_label_from_info(handle->contact, handle->sender_label);
    contact_remove_visible_widget_to_info(handle->contact, handle->message_box);
  }

  if (contact)
  {
    contact_add_name_avatar_to_info(contact, handle->sender_avatar);
    contact_add_name_label_to_info(contact, handle->sender_label);
    contact_add_visible_widget_to_info(contact, handle->message_box);
  }

  handle->contact = contact;
}

void
ui_message_set_status_callback(UI_MESSAGE_Handle *handle,
                               UI_MESSAGE_StatusCallback cb,
                               gpointer cls)
{
  g_assert(handle);

  handle->status_cb = cb;
  handle->status_cls = cls;
}

void
ui_message_add_tag(UI_MESSAGE_Handle *handle,
                   MESSENGER_Application *app,
                   struct GNUNET_CHAT_Message *tag_message)
{
  g_assert((handle) && (app) && (tag_message));

  if ((GNUNET_CHAT_KIND_TAG != GNUNET_CHAT_message_get_kind(tag_message)) ||
      (GNUNET_CHAT_message_get_target(tag_message) != handle->msg))
    return;

  const char *tag_value = GNUNET_CHAT_message_get_text(tag_message);

  if ((!tag_value) || (!*tag_value))
    return;

  UI_TAG_Handle *tag = ui_tag_new(app);
  ui_tag_set_message(tag, app, tag_message);

  gtk_container_add(GTK_CONTAINER(handle->tag_flow_box), GTK_WIDGET(tag->tag_label));
  gtk_widget_show_all(GTK_WIDGET(tag->tag_label));
}

static void
_remove_tag_from_message(UI_MESSAGE_Handle *handle,
                         MESSENGER_Application *app,
                         GtkWidget *child)
{
  g_assert((handle) && (app) && (child));

  GList *items = gtk_container_get_children(GTK_CONTAINER(child));
  UI_TAG_Handle *tag = NULL;

  if (items)
  {
    GtkLabel *tag_label = GTK_LABEL(items->data);

    tag = g_object_get_qdata(
      G_OBJECT(tag_label),
      app->quarks.ui
    );

    g_list_free(items);
  }

  gtk_container_remove(GTK_CONTAINER(handle->tag_flow_box), child);

  ui_tag_delete(tag);
  gtk_widget_destroy(child);
}

void
ui_message_remove_tag(UI_MESSAGE_Handle *handle,
                      MESSENGER_Application *app,
                      struct GNUNET_CHAT_Message *tag_message)
{
  g_assert((handle) && (app) && (tag_message));

  if ((GNUNET_CHAT_KIND_TAG != GNUNET_CHAT_message_get_kind(tag_message)) ||
      (GNUNET_CHAT_message_get_target(tag_message) != handle->msg))
    return;
  
  GList *children = gtk_container_get_children(GTK_CONTAINER(handle->tag_flow_box));

  if (!children)
    return;

  GtkWidget *removable = NULL;

  GList *list = children;
  while (list)
  {
    GtkFlowBoxChild *child = GTK_FLOW_BOX_CHILD(list->data);
    GList *items = gtk_container_get_children(GTK_CONTAINER(child));

    if (items)
    {
      GtkLabel *tag_label = GTK_LABEL(items->data);

      const struct GNUNET_CHAT_Message *msg = g_object_get_qdata(
        G_OBJECT(tag_label),
        app->quarks.data
      );

      if (tag_message == msg)
        removable = GTK_WIDGET(child);

      g_list_free(items);
    }

    list = list->next;

    if (removable)
      break;
  }

  if (children)
    g_list_free(children);

  if (!removable)
    return;

  _remove_tag_from_message(handle, app, removable);
}

void
ui_message_delete(UI_MESSAGE_Handle *handle,
                  MESSENGER_Application *app)
{
  g_assert((handle) && (app));

  _update_message_with_file(handle, app, NULL);
  ui_message_set_contact(handle, NULL);

  GList *children = gtk_container_get_children(GTK_CONTAINER(handle->tag_flow_box));

  GList *list = children;
  while (list)
  {
    GtkWidget *child = GTK_WIDGET(list->data);

    if (child)
      _remove_tag_from_message(handle, app, child);

    list = list->next;
  }

  if (children)
    g_list_free(children);

  g_object_unref(handle->builder[1]);
  g_object_unref(handle->builder[0]);

  g_free(handle);
}
