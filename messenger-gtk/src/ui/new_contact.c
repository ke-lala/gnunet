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
 * @file ui/new_platform.h
 */

#include "new_contact.h"

#include "../application.h"
#include "../request.h"
#include "../ui.h"

#include <pipewire/pipewire.h>

static void
handle_cancel_button_click(UNUSED GtkButton *button,
			                     gpointer user_data)
{
  g_assert(user_data);
  
  GtkDialog *dialog = GTK_DIALOG(user_data);
  gtk_window_close(GTK_WINDOW(dialog));
}

static void
handle_confirm_button_click(UNUSED GtkButton *button,
			                      gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  const gint id_length = gtk_entry_get_text_length(app->ui.new_contact.id_entry);
  const gchar *id_text = gtk_entry_get_text(app->ui.new_contact.id_entry);

  if (id_length <= 0)
    goto close_dialog;

  gchar *emsg = NULL;
  struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_uri_parse(id_text, &emsg);

  if (emsg)
  {
    g_printerr("ERROR: %s\n", emsg);
    GNUNET_free(emsg);
  }

  if (!uri)
    goto close_dialog;

  application_chat_lock(app);
  GNUNET_CHAT_lobby_join(app->chat.messenger.handle, uri);
  application_chat_unlock(app);

  GNUNET_CHAT_uri_destroy(uri);


close_dialog:
  gtk_window_close(GTK_WINDOW(app->ui.new_contact.dialog));
}

static void
handle_dialog_destroy(UNUSED GtkWidget *window,
		                  gpointer user_data)
{
  g_assert(user_data);

  ui_new_contact_dialog_cleanup((UI_NEW_CONTACT_Handle*) user_data);
}

static void
handle_camera_combo_box_change(GtkComboBox *widget,
			                         gpointer user_data)
{
  g_assert((widget) && (user_data));

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) user_data;
  gchar *name = NULL;

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter(widget, &iter))
    gtk_tree_model_get(
      GTK_TREE_MODEL(handle->camera_list_store),
      &iter,
      0, &name,
      -1
    );
  
  if (!name)
    return;

  g_object_set(
    G_OBJECT(handle->source),
    "target-object",
    name,
    NULL
  );

  g_free(name);

  if (!handle->pipeline)
    return;

  gtk_stack_set_visible_child(handle->preview_stack, handle->loading_box);

  gst_element_set_state(handle->pipeline, GST_STATE_NULL);
  gst_element_set_state(handle->pipeline, GST_STATE_PLAYING);
}

static void
_disable_video_processing(UI_NEW_CONTACT_Handle *handle,
                          gboolean drop_pipeline)
{
  g_assert(handle);

  if (!(handle->preview_stack))
    goto skip_stack;

  if (handle->camera_count)
    gtk_stack_set_visible_child(handle->preview_stack, handle->fail_box);
  else if (drop_pipeline)
    gtk_stack_set_visible_child(handle->preview_stack, handle->no_camera_box);
  else
    gtk_stack_set_visible_child(handle->preview_stack, handle->loading_box);

skip_stack:
  if ((!(handle->pipeline)) || (!drop_pipeline))
    return;

  gst_element_set_state(handle->pipeline, GST_STATE_NULL);
}

static void
msg_error_cb(UNUSED GstBus *bus,
             GstMessage *msg,
             gpointer data)
{
  g_assert((msg) && (data));

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;

  GError* error;
  gst_message_parse_error(msg, &error, NULL);

  if (!error)
    fprintf(stderr, "ERROR: Unknown error\n");
  else if (error->message)
    fprintf(stderr, "ERROR: %s (%d)\n", error->message, error->code);
  else
    fprintf(stderr, "ERROR: Unknown error (%d)\n", error->code);

  _disable_video_processing(handle, TRUE);
}

static void
msg_eos_cb(UNUSED GstBus *bus,
           UNUSED GstMessage *msg,
           gpointer data)
{
  g_assert(data);

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;

  if (GST_MESSAGE_SRC(msg) == GST_OBJECT(handle->pipeline))
    _disable_video_processing(handle, TRUE);
}

static void
msg_state_changed_cb(UNUSED GstBus *bus,
                     GstMessage *msg,
                     gpointer data)
{
  g_assert((msg) && (data));

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;

  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

  if ((GST_MESSAGE_SRC(msg) != GST_OBJECT(handle->pipeline)) ||
      (new_state == old_state) || (!(handle->preview_stack)))
    return;

  if ((GST_STATE_PAUSED == new_state) || (!(handle->sink)))
    _disable_video_processing(handle, FALSE);
  else if (GST_STATE_PLAYING == new_state)
    gtk_stack_set_visible_child(
      handle->preview_stack,
      handle->video_box
    );
}

static void
msg_barcode_cb(UNUSED GstBus *bus,
               GstMessage *msg,
               gpointer data)
{
  g_assert((msg) && (data));

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) data;
  GstMessageType msg_type = GST_MESSAGE_TYPE(msg);

  if ((GST_MESSAGE_SRC(msg) != GST_OBJECT(handle->scanner)) ||
      (GST_MESSAGE_ELEMENT != msg_type))
    return;

  const GstStructure *s = gst_message_get_structure(msg);

  if (!s)
    return;

  const gchar *type = gst_structure_get_string(s, "type");
  const gchar *symbol = gst_structure_get_string(s, "symbol");

  if ((!type) || (!symbol) || (0 != g_strcmp0(type, "QR-Code")))
    return;

  if (handle->id_entry)
    gtk_entry_set_text(handle->id_entry, symbol);
}

static void
_setup_gst_pipeline(UI_NEW_CONTACT_Handle *handle)
{
  g_assert(handle);

  handle->pipeline = gst_parse_launch(
    "pipewiresrc name=source ! videoconvert ! zbar name=scanner"
    " ! videoconvert ! aspectratiocrop aspect-ratio=1/1"
    " ! videoconvert ! video/x-raw,format=RGB"
    " ! videoconvert ! gtksink name=sink",
    NULL
  );

  if (!(handle->pipeline))
    return;

  handle->source = gst_bin_get_by_name(
    GST_BIN(handle->pipeline), "source"
  );

  handle->scanner = gst_bin_get_by_name(
    GST_BIN(handle->pipeline), "scanner"
  );

  handle->sink = gst_bin_get_by_name(
    GST_BIN(handle->pipeline), "sink"
  );

  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(handle->pipeline));

  if (!bus)
    return;

  gst_bus_add_signal_watch(bus);

  g_signal_connect(
    G_OBJECT(bus),
    "message::error",
    (GCallback) msg_error_cb,
    handle
  );

  g_signal_connect(
    G_OBJECT(bus),
    "message::eos",
    (GCallback) msg_eos_cb,
    handle
  );

  g_signal_connect(
    G_OBJECT(bus),
    "message::state-changed",
    (GCallback) msg_state_changed_cb,
    handle
  );

  g_signal_connect(
    G_OBJECT(bus),
    "message",
    (GCallback) msg_barcode_cb,
    handle
  );

  gst_object_unref(bus);
}

static void*
_ui_new_contact_video_thread(void *args)
{
  g_assert(args);

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) args;

  if (!(handle->pipeline))
    return NULL;

  GstStateChangeReturn ret = gst_element_set_state(
    handle->pipeline,
    GST_STATE_PLAYING
  );

  if (GST_STATE_CHANGE_FAILURE == ret)
    _disable_video_processing(handle, TRUE);

  return NULL;
}

static void
iterate_cameras(void *cls,
                const char *name,
                const char *description,
                const char *media_class,
                const char *media_role)
{
  g_assert(cls);

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) cls;

  if ((!name) || (!description) || (!media_class) || (!media_role))
    return;

  if (0 != g_strcmp0(media_class, "Video/Source"))
    return;
  if (0 != g_strcmp0(media_role, "Camera"))
    return;

  GtkTreeIter iter;
  gtk_list_store_append(handle->camera_list_store, &iter);
  gtk_list_store_set(
    handle->camera_list_store,
    &iter,
    0, name,
    1, description,
    -1
  );

  handle->camera_count++;
}

static void
_init_camera_pipeline(MESSENGER_Application *app,
                      UI_NEW_CONTACT_Handle *handle,
                      gboolean access)
{
  g_assert((app) && (handle));

  handle->camera_count = 0;

  if (access)
  {
    media_init_camera_capturing(&(app->media.camera), app);
    media_pw_main_loop_run(&(app->media.camera));

    media_pw_iterate_nodes(&(app->media.camera), iterate_cameras, handle);

    if (handle->camera_count)
      gtk_combo_box_set_active(handle->camera_combo_box, 0);
  }

  gtk_revealer_set_reveal_child(
    handle->camera_combo_box_revealer,
    handle->camera_count > 1
  );

  pthread_create(
    &(handle->video_tid),
    NULL,
    _ui_new_contact_video_thread,
    handle
  );
}

static void
_request_camera_callback(MESSENGER_Application *app,
                         gboolean success,
                         gboolean error,
                         gpointer user_data)
{
  g_assert((app) && (user_data));

  UI_NEW_CONTACT_Handle *handle = (UI_NEW_CONTACT_Handle*) user_data;

  _init_camera_pipeline(app, handle, success);
}

void
ui_new_contact_dialog_init(MESSENGER_Application *app,
			                     UI_NEW_CONTACT_Handle *handle)
{
  g_assert((app) && (handle));

  handle->camera_count = 0;

  _setup_gst_pipeline(handle);

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/new_contact.ui")
  );

  handle->dialog = GTK_DIALOG(
    gtk_builder_get_object(handle->builder, "new_contact_dialog")
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->dialog),
    GTK_WINDOW(app->ui.messenger.main_window)
  );

  handle->camera_combo_box_revealer = GTK_REVEALER(
    gtk_builder_get_object(handle->builder, "camera_combo_box_revealer")
  );

  handle->camera_combo_box = GTK_COMBO_BOX(
    gtk_builder_get_object(handle->builder, "camera_combo_box")
  );

  handle->camera_list_store = GTK_LIST_STORE(
    gtk_builder_get_object(handle->builder, "camera_list_store")
  );

  g_signal_connect(
    handle->camera_combo_box,
    "changed",
    G_CALLBACK(handle_camera_combo_box_change),
    handle
  );

  handle->preview_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "preview_stack")
  );

  handle->loading_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "loading_box")
  );

  handle->fail_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "fail_box")
  );

  handle->no_camera_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "no_camera_box")
  );

  handle->video_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "video_box")
  );

  gtk_stack_set_visible_child(handle->preview_stack, handle->loading_box);

  GtkWidget *widget;
  if (handle->sink)
    g_object_get(handle->sink, "widget", &widget, NULL);
  else
    widget = NULL;

  if (widget)
  {
    gtk_box_pack_start(
      GTK_BOX(handle->video_box),
      widget,
      true,
      true,
      0
    );

    g_object_unref(widget);
    gtk_widget_realize(widget);

    gtk_widget_show_all(handle->video_box);
  }

  handle->id_entry = GTK_ENTRY(
    gtk_builder_get_object(handle->builder, "id_entry")
  );

  handle->cancel_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "cancel_button")
  );

  g_signal_connect(
    handle->cancel_button,
    "clicked",
    G_CALLBACK(handle_cancel_button_click),
    handle->dialog
  );

  handle->confirm_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "confirm_button")
  );

  g_signal_connect(
    handle->confirm_button,
    "clicked",
    G_CALLBACK(handle_confirm_button_click),
    app
  );

  g_signal_connect(
    handle->dialog,
    "destroy",
    G_CALLBACK(handle_dialog_destroy),
    handle
  );

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  if (app->portal)
#else
  if (TRUE)
#endif
  {
    request_new_camera(
      app,
      XDP_CAMERA_FLAG_NONE,
      _request_camera_callback,
      handle
    );
  }
  else
    _init_camera_pipeline(app, handle, false);
}

void
ui_new_contact_dialog_cleanup(UI_NEW_CONTACT_Handle *handle)
{
  g_assert(handle);

  if (handle->video_tid)
    pthread_join(handle->video_tid, NULL);

  g_object_unref(handle->builder);

  if (handle->pipeline)
  {
    gst_element_set_state(handle->pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->pipeline));
  }

  memset(handle, 0, sizeof(*handle));
}
