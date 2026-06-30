/*
   This file is part of GNUnet.
   Copyright (C) 2022--2025 GNUnet e.V.

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
 * @file ui/play_media.c
 */

#include "play_media.h"

#include "../application.h"
#include "../ui.h"
#include "../util.h"
#include <glib-2.0/glib.h>

gboolean
ui_play_media_window_supports_file_extension(const gchar *filename)
{
  if (!filename)
    return FALSE;

  const char* extension = strrchr(filename, '.');

  if (!extension)
    return FALSE;

  if (0 == g_strcmp0(extension, ".mkv"))
    return TRUE;
  if (0 == g_strcmp0(extension, ".mp4"))
    return TRUE;
  if (0 == g_strcmp0(extension, ".webm"))
    return TRUE;

  return FALSE;
}

static void
handle_back_button_click(UNUSED GtkButton *button,
			                   gpointer user_data)
{
  g_assert(user_data);

  GtkWindow *window = GTK_WINDOW(user_data);
  gtk_window_close(window);
}

static void
_set_media_controls_sensivity(UI_PLAY_MEDIA_Handle *handle,
			                        gboolean sensitive)
{
  g_assert(handle);

  if (handle->play_pause_button)
    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->play_pause_button),
      sensitive
    );

  if (handle->volume_button)
    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->volume_button),
      sensitive
    );

  if (handle->timeline_scale)
    gtk_widget_set_sensitive(
      GTK_WIDGET(handle->timeline_scale),
      sensitive
    );
}

static void
handle_timeline_scale_value_changed(GtkRange *range,
				                            gpointer user_data);

static void
_set_signal_connection_of_timeline(UI_PLAY_MEDIA_Handle *handle,
				                           gboolean connected)
{
  g_assert(handle);

  if (!(handle->timeline_scale))
    return;

  if (connected == (handle->timeline_signal != 0))
    return;

  if (connected)
    handle->timeline_signal = g_signal_connect(
	    handle->timeline_scale,
    	"value-changed",
    	G_CALLBACK(handle_timeline_scale_value_changed),
    	handle
    );
  else
  {
    g_signal_handler_disconnect(
      handle->timeline_scale,
      handle->timeline_signal
    );

    handle->timeline_signal = 0;
  }
}

static void
_set_media_position(UI_PLAY_MEDIA_Handle *handle,
                    gint64 pos,
                    gint64 len,
                    gboolean include_scale)
{
  g_assert(handle);

  const gdouble position = (
      len > 0? 1.0 * pos / len : 0.0
  );

  if (handle->timeline_label)
  {
    GString *str = g_string_new(NULL);

    guint pos_seconds = GST_TIME_AS_SECONDS(pos);
    guint len_seconds = GST_TIME_AS_SECONDS(len);

    g_string_append_printf(
      str,
      "%u:%02u / %u:%02u",
      pos_seconds / 60,
      pos_seconds % 60,
      len_seconds / 60,
      len_seconds % 60
    );

    ui_label_set_text(handle->timeline_label, str->str);
    g_string_free(str, TRUE);
  }

  if (handle->timeline_progress_bar)
    gtk_progress_bar_set_fraction(
      handle->timeline_progress_bar,
      1.0 * position
    );

  if ((!(handle->timeline_scale)) || (!include_scale))
    return;

  _set_signal_connection_of_timeline(handle, FALSE);

  gtk_range_set_value(
    GTK_RANGE(handle->timeline_scale),
    100.0 * position
  );

  _set_signal_connection_of_timeline(handle, TRUE);
}

static gboolean
_adjust_playing_media_position(UI_PLAY_MEDIA_Handle *handle);

static void
_set_next_timeout_callback_of_timeline(UI_PLAY_MEDIA_Handle *handle)
{
  g_assert(handle);

  handle->timeline = util_timeout_add(
    200,
    G_SOURCE_FUNC(_adjust_playing_media_position),
    handle
  );
}

static gboolean
_adjust_playing_media_position(UI_PLAY_MEDIA_Handle *handle)
{
  g_assert(handle);

  gint64 pos, len;

  handle->timeline = 0;

  if (!(handle->pipeline))
    return G_SOURCE_REMOVE;

  if (!gst_element_query_position(handle->pipeline, GST_FORMAT_TIME, &pos))
    return G_SOURCE_REMOVE;

  if (!gst_element_query_duration(handle->pipeline, GST_FORMAT_TIME, &len))
    return G_SOURCE_REMOVE;

  _set_media_position(handle, pos, len, TRUE);
  _set_next_timeout_callback_of_timeline(handle);
  return G_SOURCE_REMOVE;
}

static void
_set_timeout_callback_of_timeline(UI_PLAY_MEDIA_Handle *handle,
				                          gboolean connected)
{
  g_assert(handle);
  
  if (handle->timeline)
    util_source_remove(handle->timeline);

  if (connected)
    _set_next_timeout_callback_of_timeline(handle);
  else
    handle->timeline = 0;
}

static void
_set_media_state(UI_PLAY_MEDIA_Handle *handle,
		             gboolean playing)
{
  g_assert(handle);
  
  if (handle->play_symbol_stack)
    gtk_stack_set_visible_child_name(
        handle->play_symbol_stack,
	playing? "pause_page" : "play_page"
    );

  _set_timeout_callback_of_timeline(handle, playing);
}

static void
_disable_video_processing(UI_PLAY_MEDIA_Handle *handle,
			                    gboolean drop_pipeline)
{
  g_assert(handle);

  if (handle->preview_stack)
    gtk_stack_set_visible_child(handle->preview_stack, handle->fail_box);

  _set_media_controls_sensivity(handle, FALSE);
  _set_media_position(handle, 0, 0, TRUE);
  _set_media_state(handle, FALSE);

  if ((!(handle->pipeline)) || (!drop_pipeline))
    return;

  gst_element_set_state(handle->pipeline, GST_STATE_NULL);
}

static void
_adjust_playing_media_state(UI_PLAY_MEDIA_Handle *handle, gboolean playing)
{
  _set_media_state(handle, playing);

  if (handle->preview_stack)
    gtk_stack_set_visible_child(
      handle->preview_stack,
      handle->video_box
    );
}

static void
_pause_playing_media(UI_PLAY_MEDIA_Handle *handle)
{
  if (!(handle->pipeline))
    return;

  GstStateChangeReturn ret = gst_element_set_state(
      handle->pipeline,
      GST_STATE_PAUSED
  );

  if (GST_STATE_CHANGE_FAILURE == ret)
  {
    _disable_video_processing(handle, TRUE);
    return;
  }
}

static void
_continue_playing_media(UI_PLAY_MEDIA_Handle *handle)
{
  if (!(handle->pipeline))
    return;

  GstStateChangeReturn ret = gst_element_set_state(
      handle->pipeline,
      GST_STATE_PLAYING
  );

  if (GST_STATE_CHANGE_FAILURE == ret)
  {
    _disable_video_processing(handle, TRUE);
    return;
  }
}

static void
handle_play_pause_button_click(GtkButton *button,
			       gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  if (!(handle->play_symbol_stack))
    return;

  const gchar *page = gtk_stack_get_visible_child_name(
      handle->play_symbol_stack
  );

  if (0 == g_strcmp0(page, "pause_page"))
    _pause_playing_media(handle);
  else
    _continue_playing_media(handle);
}

static void
handle_volume_button_value_changed(GtkScaleButton *button,
				   double value,
				   gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  if (!(handle->pipeline))
    return;

  g_object_set(
      G_OBJECT(handle->pipeline),
      "volume",
      value,
      NULL
  );

  g_object_set(
      G_OBJECT(handle->pipeline),
      "mute",
      (value <= 0.0),
      NULL
  );
}

static void
handle_timeline_scale_value_changed(GtkRange *range,
				    gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;
  gint64 pos, len;

  if (!(handle->pipeline))
    return;

  if (!gst_element_query_duration(handle->pipeline, GST_FORMAT_TIME, &len))
    return;

  pos = (gint64) (gtk_range_get_value(range) * len / 100);

  if (gst_element_seek_simple(handle->pipeline,
			      GST_FORMAT_TIME,
			      GST_SEEK_FLAG_FLUSH,
			      pos))
    _set_media_position(handle, pos, len, FALSE);
}

static void
handle_fullscreen_button_click(GtkButton *button,
			       gpointer user_data)
{
  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  gtk_revealer_set_reveal_child(handle->header_revealer, handle->fullscreen);
  hdy_flap_set_reveal_flap(handle->controls_flap, handle->fullscreen);

  handle->fullscreen = !(handle->fullscreen);

  if (!(handle->fullscreen))
    gtk_window_unfullscreen(GTK_WINDOW(handle->window));

  gtk_widget_hide(GTK_WIDGET(handle->window));

  gtk_window_set_type_hint(
      GTK_WINDOW(handle->window),
      handle->fullscreen?
	  GDK_WINDOW_TYPE_HINT_NORMAL :
	  GDK_WINDOW_TYPE_HINT_DIALOG
  );

  gtk_window_set_modal(GTK_WINDOW(handle->window), !(handle->fullscreen));

  gtk_window_set_position(
      GTK_WINDOW(handle->window),
      handle->fullscreen? GTK_WIN_POS_NONE : GTK_WIN_POS_CENTER_ON_PARENT
  );

  gtk_window_set_transient_for(
      GTK_WINDOW(handle->window),
      handle->fullscreen? NULL : handle->parent
  );

  gtk_widget_show_all(GTK_WIDGET(handle->window));
  hdy_flap_set_reveal_flap(handle->controls_flap, !(handle->fullscreen));

  if (handle->fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(handle->window));
  else
  {
    if (handle->motion_lost)
      util_source_remove(handle->motion_lost);

    handle->motion_lost = 0;
  }

  gtk_stack_set_visible_child_name(
      handle->fullscreen_symbol_stack,
      handle->fullscreen? "scale_down_page" : "scale_up_page"
  );
}

static gboolean
handle_media_motion_lost(gpointer user_data)
{
  g_assert(user_data);

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  handle->motion_lost = 0;

  if (!(hdy_flap_get_reveal_flap(handle->controls_flap)))
    return G_SOURCE_REMOVE;

  hdy_flap_set_reveal_flap(handle->controls_flap, FALSE);
  return G_SOURCE_REMOVE;
}

static gboolean
handle_media_motion_notify(GtkWidget *widget,
			   GdkEvent *event,
			   gpointer user_data)
{
  g_assert(user_data);

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) user_data;

  if (hdy_flap_get_reveal_flap(handle->controls_flap))
    return G_SOURCE_REMOVE;

  if (handle->motion_lost)
    util_source_remove(handle->motion_lost);

  hdy_flap_set_reveal_flap(handle->controls_flap, TRUE);

  if (!(handle->fullscreen))
    return G_SOURCE_REMOVE;

  handle->motion_lost = util_timeout_add_seconds(
      3,
      G_SOURCE_FUNC(handle_media_motion_lost),
      handle
  );

  return G_SOURCE_REMOVE;
}

static void
handle_window_destroy(UNUSED GtkWidget *window,
		                  gpointer user_data)
{
  g_assert(user_data);

  ui_play_media_window_cleanup((UI_PLAY_MEDIA_Handle*) user_data);
}

static void
msg_error_cb(UNUSED GstBus *bus,
             GstMessage *msg,
             gpointer data)
{
  g_assert((msg) && (data));

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) data;

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

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) data;

  if (GST_MESSAGE_SRC(msg) != GST_OBJECT(handle->pipeline))
    return;

  if (handle->timeline_scale)
    gtk_range_set_value(GTK_RANGE(handle->timeline_scale), 0.0);

  _adjust_playing_media_state(handle, FALSE);
}

static void
msg_state_changed_cb(UNUSED GstBus *bus,
                     GstMessage *msg,
                     gpointer data)
{
  g_assert((msg) && (data));

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) data;

  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

  if (GST_MESSAGE_SRC(msg) != GST_OBJECT(handle->pipeline))
    return;

  if (!(handle->sink))
  {
    _disable_video_processing(handle, FALSE);
    return;
  }

  if (GST_STATE_READY == new_state)
    _set_media_controls_sensivity(handle, TRUE);

  if ((GST_STATE_PLAYING != new_state) && (GST_STATE_PAUSED != new_state))
    return;

  _adjust_playing_media_state(handle, GST_STATE_PLAYING == new_state);
}

static void
msg_buffering_cb(UNUSED GstBus *bus,
                 GstMessage *msg,
                 gpointer data)
{
  g_assert((msg) && (data));

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) data;

  gint percent = 0;
  gst_message_parse_buffering(msg, &percent);

  GstStateChangeReturn ret = gst_element_set_state(
      handle->pipeline,
      (percent < 100? GST_STATE_PAUSED : GST_STATE_PLAYING)
  );

  if (ret == GST_STATE_CHANGE_FAILURE)
    _disable_video_processing(handle, TRUE);
}

static void
_setup_gst_pipeline(UI_PLAY_MEDIA_Handle *handle)
{
  g_assert(handle);

  handle->pipeline = gst_element_factory_make("playbin", NULL);

  if (!(handle->pipeline))
    return;

  handle->sink = gst_element_factory_make("gtksink", "vsink");

  if (!(handle->sink))
    return;

  g_object_set(
      G_OBJECT(handle->pipeline),
      "video-sink",
      handle->sink,
      NULL
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
      "message::buffering",
      (GCallback) msg_buffering_cb,
      handle
  );

  gst_object_unref(bus);
}

static void*
_ui_play_media_video_thread(void *args)
{
  g_assert(args);

  UI_PLAY_MEDIA_Handle *handle = (UI_PLAY_MEDIA_Handle*) args;
  _continue_playing_media(handle);
  return NULL;
}

void
ui_play_media_window_init(MESSENGER_Application *app,
                          UI_PLAY_MEDIA_Handle *handle)
{
  g_assert((app) && (handle));

  _setup_gst_pipeline(handle);

  handle->parent = GTK_WINDOW(app->ui.messenger.main_window);

  handle->builder = ui_builder_from_resource(
    application_get_resource_path(app, "ui/play_media.ui")
  );

  handle->window = HDY_WINDOW(
    gtk_builder_get_object(handle->builder, "play_media_window")
  );

  gtk_window_set_position(
    GTK_WINDOW(handle->window),
    GTK_WIN_POS_CENTER_ON_PARENT
  );

  gtk_window_set_transient_for(
    GTK_WINDOW(handle->window),
    handle->parent
  );

  handle->header_revealer = GTK_REVEALER(
    gtk_builder_get_object(handle->builder, "header_revealer")
  );

  handle->title_bar = HDY_HEADER_BAR(
    gtk_builder_get_object(handle->builder, "title_bar")
  );

  handle->back_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "back_button")
  );

  g_signal_connect(
    handle->back_button,
    "clicked",
    G_CALLBACK(handle_back_button_click),
    handle->window
  );

  handle->controls_flap = HDY_FLAP(
    gtk_builder_get_object(handle->builder, "controls_flap")
  );

  handle->preview_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "preview_stack")
  );

  handle->fail_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "fail_box")
  );

  handle->video_box = GTK_WIDGET(
    gtk_builder_get_object(handle->builder, "video_box")
  );

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

  handle->play_pause_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "play_pause_button")
  );

  handle->play_symbol_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "play_symbol_stack")
  );

  g_signal_connect(
    handle->play_pause_button,
    "clicked",
    G_CALLBACK(handle_play_pause_button_click),
    handle
  );

  handle->volume_button = GTK_VOLUME_BUTTON(
    gtk_builder_get_object(handle->builder, "volume_button")
  );

  g_signal_connect(
    handle->volume_button,
    "value-changed",
    G_CALLBACK(handle_volume_button_value_changed),
    handle
  );

  handle->timeline_label = GTK_LABEL(
    gtk_builder_get_object(handle->builder, "timeline_label")
  );

  handle->timeline_progress_bar = GTK_PROGRESS_BAR(
    gtk_builder_get_object(handle->builder, "timeline_progress_bar")
  );

  handle->timeline_scale = GTK_SCALE(
    gtk_builder_get_object(handle->builder, "timeline_scale")
  );

  _set_signal_connection_of_timeline(handle, handle->sink? TRUE : FALSE);

  handle->settings_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "settings_button")
  );

  handle->fullscreen_button = GTK_BUTTON(
    gtk_builder_get_object(handle->builder, "fullscreen_button")
  );

  handle->fullscreen_symbol_stack = GTK_STACK(
    gtk_builder_get_object(handle->builder, "fullscreen_symbol_stack")
  );

  g_signal_connect(
    handle->fullscreen_button,
    "clicked",
    G_CALLBACK(handle_fullscreen_button_click),
    handle
  );

  g_signal_connect(
    handle->window,
    "motion-notify-event",
    G_CALLBACK(handle_media_motion_notify),
    handle
  );

  gtk_widget_add_events(
    GTK_WIDGET(handle->window),
    GDK_POINTER_MOTION_HINT_MASK |
    GDK_POINTER_MOTION_MASK
  );

  g_signal_connect(
    handle->window,
    "destroy",
    G_CALLBACK(handle_window_destroy),
    handle
  );

  gtk_scale_button_set_value(
    GTK_SCALE_BUTTON(handle->volume_button),
    1.0
  );

  gtk_widget_show_all(GTK_WIDGET(handle->window));
}

void
ui_play_media_window_update(UI_PLAY_MEDIA_Handle *handle,
                            const gchar *uri,
                            const struct GNUNET_CHAT_File *file)
{
  g_assert((handle) && (uri));

  if (handle->video_tid)
    pthread_join(handle->video_tid, NULL);

  if (!(handle->pipeline))
    return;

  _disable_video_processing(handle, TRUE);
  g_object_set(G_OBJECT(handle->pipeline), "uri", uri, NULL);

  const gchar *filename;

  if (file)
    filename = GNUNET_CHAT_file_get_name(file);
  else
    filename = uri;

  hdy_header_bar_set_subtitle(
    handle->title_bar,
    filename? filename : ""
  );

  pthread_create(
    &(handle->video_tid),
    NULL,
    _ui_play_media_video_thread,
    handle
  );
}

void
ui_play_media_window_cleanup(UI_PLAY_MEDIA_Handle *handle)
{
  g_assert(handle);

  if (handle->video_tid)
    pthread_join(handle->video_tid, NULL);

  g_object_unref(handle->builder);

  if (handle->timeline)
    util_source_remove(handle->timeline);

  if (handle->motion_lost)
    util_source_remove(handle->motion_lost);

  if (handle->pipeline)
  {
    gst_element_set_state(handle->pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(handle->pipeline));
  }

  memset(handle, 0, sizeof(*handle));
}
