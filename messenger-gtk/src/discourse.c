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
 * @file discourse.c
 */

#include "discourse.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/rtp/rtp.h>
#include <pthread.h>
#include <stdlib.h>

const struct GNUNET_CHAT_DiscourseId*
get_voice_discourse_id()
{
  static enum GNUNET_GenericReturnValue init = GNUNET_NO;
  static struct GNUNET_CHAT_DiscourseId id;

  if (GNUNET_YES != init)
  {
    memset(&id, 0, sizeof(id));
    init = GNUNET_YES;
  }

  return &id;
}

const struct GNUNET_CHAT_DiscourseId*
get_video_discourse_id()
{
  static enum GNUNET_GenericReturnValue init = GNUNET_NO;
  static struct GNUNET_CHAT_DiscourseId id;

  if (GNUNET_YES != init)
  {
    memset(&id, 1, sizeof(id));
    init = GNUNET_YES;
  }

  return &id;
}

static void
error_cb(GstBus *bus,
         GstMessage *msg,
         gpointer data)
{
  GError *err;
  gchar *debug_info;

  gst_message_parse_error (msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);
}

static void
_setup_audio_gst_pipelines_of_subscription(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  info->audio_stream_source = gst_element_factory_make("appsrc", NULL);
  info->audio_jitter_buffer = gst_element_factory_make("rtpjitterbuffer", NULL);
  info->audio_depay = gst_element_factory_make("rtpL16depay", NULL);
  info->audio_converter = gst_element_factory_make("audioconvert", NULL);

  gst_element_set_state(info->discourse->audio_mix_pipeline, GST_STATE_NULL);

  gst_bin_add_many(
    GST_BIN(info->discourse->audio_mix_pipeline),
    info->audio_stream_source,
    info->audio_jitter_buffer,
    info->audio_depay,
    info->audio_converter,
    NULL
  );

  gst_element_link_many(
    info->audio_stream_source,
    info->audio_jitter_buffer,
    info->audio_depay,
    info->audio_converter,
    NULL
  );

  {
    GstCaps *caps = gst_caps_new_simple (
      "application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "encoding-name", G_TYPE_STRING, "L16",
      "payload", G_TYPE_INT, 11,
      "clock-rate", G_TYPE_INT, 44100,
      NULL
    );

    g_object_set(
      info->audio_stream_source,
      "format", GST_FORMAT_TIME,
      "caps", caps,
      "is-live", TRUE,
      NULL
    );

    gst_caps_unref(caps);
  }

  info->audio_mix_pad = gst_element_request_pad_simple(
    info->discourse->audio_mix_element, "sink_%u"
  );

  {
    GstPad *pad = gst_element_get_static_pad(
      info->audio_converter, "src"
    );

    g_object_set(info->audio_mix_pad, "mute", FALSE, "volume", 1.0, NULL);
    gst_pad_link(pad, info->audio_mix_pad);
  }

  gst_element_set_state(info->discourse->audio_mix_pipeline, GST_STATE_PLAYING);
}

static void
_setup_video_gst_pipelines_of_subscription(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  info->video_stream_pipeline = gst_parse_launch(
    "appsrc name=source ! rtpjitterbuffer ! rtph264depay ! avdec_h264 ! videoconvert ! "
    "gtksink name=sink sync=false",
    NULL
  );

  info->video_stream_source = gst_bin_get_by_name(
    GST_BIN(info->video_stream_pipeline), "source"
  );

  info->video_stream_sink = gst_bin_get_by_name(
    GST_BIN(info->video_stream_pipeline), "sink"
  );

  {
    GstBus *bus = gst_element_get_bus(info->video_stream_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "application/x-rtp",
      "media", G_TYPE_STRING, "video",
      "payload", G_TYPE_INT, 96,
      "clock-rate", G_TYPE_INT, 90000,
      "encoding-name", G_TYPE_STRING, "H264",
      NULL
    );

    g_object_set(
      info->video_stream_source,
      "format", GST_FORMAT_TIME,
      "caps", caps,
      "is-live", TRUE,
      NULL
    );

    gst_caps_unref(caps);

    gst_element_set_state(info->video_stream_pipeline, GST_STATE_NULL);
  }
}

static MESSENGER_DiscourseSubscriptionInfo*
discourse_subscription_create_info(MESSENGER_DiscourseInfo *discourse,
                                   struct GNUNET_CHAT_Contact *contact)
{
  g_assert((discourse) && (contact));

  MESSENGER_DiscourseSubscriptionInfo* info = g_malloc(
    sizeof(MESSENGER_DiscourseSubscriptionInfo)
  );

  if (!info)
    return NULL;

  info->discourse = discourse;
  info->contact = contact;

  info->audio_stream_source = NULL;
  info->audio_jitter_buffer = NULL;
  info->audio_depay = NULL;
  info->audio_converter = NULL;

  info->video_stream_pipeline = NULL;
  info->video_stream_source = NULL;
  info->video_stream_sink = NULL;

  info->audio_mix_pad = NULL;
  info->buffers = NULL;

  info->position = 0;
  info->last_timestamp = 0;

  pthread_mutex_init(&(info->mutex), NULL);

  info->end_datetime = NULL;

  const struct GNUNET_CHAT_DiscourseId *id = GNUNET_CHAT_discourse_get_id(
    info->discourse->discourse
  );

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
    _setup_audio_gst_pipelines_of_subscription(info);
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    _setup_video_gst_pipelines_of_subscription(info);

  return info;
}

static void
discourse_subscription_destroy_info(MESSENGER_DiscourseSubscriptionInfo *info)
{
  g_assert(info);

  GList *buf = info->buffers;
  while (buf)
  {
    GstBuffer *buffer = (GstBuffer*) buf->data;

    if (buffer)
      gst_buffer_unref(buffer);

    buf = g_list_next(buf);
  }

  if (info->buffers)
    g_list_free(info->buffers);

  if ((info->audio_stream_source) || (info->audio_jitter_buffer) || 
      (info->audio_depay) || (info->audio_converter))
    gst_element_set_state(info->discourse->audio_mix_pipeline, GST_STATE_NULL);

  if (info->video_stream_pipeline)
  {
    gst_element_set_state(info->video_stream_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->video_stream_pipeline));
  }

  if (info->audio_mix_pad)
  {
    GstPad *pad = gst_element_get_static_pad(
      info->audio_converter, "src"
    );

    gst_pad_unlink(pad, info->audio_mix_pad);

    gst_element_release_request_pad(info->discourse->audio_mix_element, info->audio_mix_pad);
    gst_object_unref(GST_OBJECT(info->audio_mix_pad));
  }

  if ((info->audio_stream_source) || (info->audio_jitter_buffer) || 
      (info->audio_depay) || (info->audio_converter))
  {
    gst_element_unlink_many(
      info->audio_stream_source,
      info->audio_jitter_buffer,
      info->audio_depay,
      info->audio_converter,
      NULL
    );

    gst_bin_remove_many(
      GST_BIN(info->discourse->audio_mix_pipeline),
      info->audio_stream_source,
      info->audio_jitter_buffer,
      info->audio_depay,
      info->audio_converter,
      NULL
    );

    gst_element_set_state(info->discourse->audio_mix_pipeline, GST_STATE_PLAYING);
  }

  pthread_mutex_lock(&(info->mutex));

  if (info->end_datetime)
  {
    g_date_time_unref(info->end_datetime);
    info->end_datetime = NULL;
  }

  pthread_mutex_unlock(&(info->mutex));
  pthread_mutex_destroy(&(info->mutex));

  g_free(info);
}

static void
discourse_subscription_stream_message(MESSENGER_DiscourseSubscriptionInfo *info,
                                      const struct GNUNET_CHAT_Message *message)
{
  g_assert((info) && (message));

  const uint64_t available = GNUNET_CHAT_message_available(message);

  if (!available)
    return;

  const struct GNUNET_CHAT_DiscourseId *id = GNUNET_CHAT_discourse_get_id(
    info->discourse->discourse
  );

  uint64_t clockrate = 0;
  GstElement *appsrc = NULL;

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
  {
    if (GNUNET_YES == GNUNET_CHAT_message_is_sent(message))
      return;

    clockrate = 44100;
    appsrc = info->audio_stream_source;
  }
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
  {
    clockrate = 90000;
    appsrc = info->video_stream_source;
  }
  else
    return;

  GstBuffer *buffer = gst_buffer_new_and_alloc(available);
  GstFlowReturn ret = GST_FLOW_ERROR;

  if (!buffer)
    return;

  GstMapInfo mapping;
  if (gst_buffer_map(buffer, &mapping, GST_MAP_WRITE))
  {
    if (mapping.size)
    {
      if (GNUNET_OK != GNUNET_CHAT_message_read(message, (char*) mapping.data, mapping.size))
        memset(mapping.data, 0, mapping.size);
    }

    gst_buffer_unmap(buffer, &mapping);
  }
  else
    goto skip_buffer;

  uint64_t timestamp = info->last_timestamp;
  uint32_t payload_len = 0;

  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  if (gst_rtp_buffer_map(buffer, GST_MAP_READ, &rtp))
  {
    const uint32_t rtp_timestamp = gst_rtp_buffer_get_timestamp(&rtp);
    payload_len = gst_rtp_buffer_get_payload_len(&rtp);

    timestamp = gst_rtp_buffer_ext_timestamp(&timestamp, rtp_timestamp);
    if (!timestamp)
      timestamp = rtp_timestamp;

    gst_rtp_buffer_unmap(&rtp);
  }

  if (payload_len)
    info->buffers = g_list_append(info->buffers, buffer);
  
  buffer = NULL;

  GDateTime *dt = g_date_time_new_now_local();

  if ((!payload_len) || (info->last_timestamp == timestamp) ||
      ((!(info->last_timestamp)) && (!(info->position))))
    goto skip_buffer;

  if (info->buffers)
    buffer = gst_buffer_new();
  
  GList *buf = info->buffers;
  while (buf)
  {
    GstBuffer *sub_buffer = (GstBuffer*) buf->data;

    if (sub_buffer)
    {
      gst_buffer_append_memory(
        buffer,
        gst_buffer_get_memory(sub_buffer, 0)
      );

      gst_buffer_unref(sub_buffer);
    }

    buf = g_list_next(buf);
  }

  if (info->buffers)
  {
    g_list_free(info->buffers);
    info->buffers = NULL;
  }

  const uint64_t duration = timestamp - info->last_timestamp;

  if (buffer)
  {
    GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(info->position, GST_SECOND, clockrate);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(duration, GST_SECOND, clockrate);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
  }

  if ((appsrc) && (!(info->position)))
    gst_element_set_state(appsrc, GST_STATE_PLAYING);

  info->position += duration;

  pthread_mutex_lock(&(info->mutex));

  if (info->end_datetime)
  {
    g_date_time_unref(info->end_datetime);
    info->end_datetime = NULL;
  }

  if (dt)
    info->end_datetime = g_date_time_add_seconds(dt, 0.1 + (gdouble) duration / clockrate);

  pthread_mutex_unlock(&(info->mutex));

skip_buffer:
  if (payload_len)
    info->last_timestamp = timestamp;
  else if (dt)
  {
    pthread_mutex_lock(&(info->mutex));

    if (info->end_datetime)
      g_date_time_unref(info->end_datetime);

    info->end_datetime = g_date_time_add_seconds(dt, 0.1);

    pthread_mutex_unlock(&(info->mutex));
  }

  if (dt)
    g_date_time_unref(dt);

  if (buffer)
    gst_buffer_unref(buffer);

  if (GST_FLOW_OK != ret)
    return;
}

static gboolean
_discourse_video_heartbeat(gpointer user_data)
{
  MESSENGER_DiscourseInfo *info = (MESSENGER_DiscourseInfo*) user_data;

  info->heartbeat = 0;

  GstBuffer *buffer = gst_buffer_new();

  if (!buffer)
    return FALSE;

  GstFlowReturn ret = GST_FLOW_ERROR;
  
  gst_rtp_buffer_allocate_data(
    buffer,
    0,
    0,
    0
  );

  g_signal_emit_by_name(
    info->video_heartbeat_source,
    "push-buffer",
    buffer,
    &ret
  );

  if (buffer)
    gst_buffer_unref(buffer);

  info->heartbeat = util_timeout_add(
    100,
    G_SOURCE_FUNC(_discourse_video_heartbeat),
    info
  );

  return FALSE;
}

static gboolean
discourse_subscription_link_widget(MESSENGER_DiscourseSubscriptionInfo *info,
                                   GtkContainer *container)
{
  g_assert(info);

  GtkWidget *widget;
  if (info->video_stream_sink)
    g_object_get(info->video_stream_sink, "widget", &widget, NULL);
  else
    widget = NULL;

  if (!widget)
    return FALSE;

  GtkWidget *parent = gtk_widget_get_parent(widget);
  if (parent)
  {
    GtkContainer *current = GTK_CONTAINER(parent);

    if (current == container)
    {
      g_object_unref(widget);
      return TRUE;
    }

    gst_element_set_state(info->video_stream_pipeline, GST_STATE_NULL);

    gtk_widget_hide(widget);
    gtk_widget_unrealize(widget);

    gtk_container_remove(
      current,
      widget
    );
  }

  if (container)
  {
    gtk_box_pack_start(
      GTK_BOX(container),
      widget,
      true,
      true,
      0
    );
  }

  g_object_unref(widget);
  
  if (container)
  {
    gtk_widget_realize(widget);
    gtk_widget_show_all(GTK_WIDGET(container));

    gst_element_set_state(info->video_stream_pipeline, GST_STATE_PLAYING);
  }

  return TRUE;
}

static void
_setup_audio_gst_pipelines(MESSENGER_DiscourseInfo *info)
{
  g_assert(info);

  info->audio_record_pipeline = gst_parse_launch(
    "autoaudiosrc ! audioconvert ! audio/x-raw,format=S16BE,layout=interleaved,rate=44100,channels=1 ! "
    "rtpL16pay ! capsfilter name=filter ! fdsink name=sink",
    NULL
  );

  info->audio_record_sink = gst_bin_get_by_name(
    GST_BIN(info->audio_record_pipeline), "sink"
  );

  GstElement *filter = gst_bin_get_by_name(
    GST_BIN(info->audio_record_pipeline), "filter"
  );

  {
    GstBus *bus = gst_element_get_bus(info->audio_record_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "encoding-name", G_TYPE_STRING, "L16",
      "payload", G_TYPE_INT, 11,
      "clock-rate", G_TYPE_INT, 44100,
      NULL
    );

    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    const int fd = GNUNET_CHAT_discourse_get_fd(info->discourse);
    if (-1 != fd)
      g_object_set(info->audio_record_sink, "fd", fd, NULL);

    gst_element_set_state(info->audio_record_pipeline, GST_STATE_PLAYING);
  }

  info->audio_mix_pipeline = gst_parse_launch(
    "audiomixer name=mixer ! volume name=control ! autoaudiosink",
    NULL
  );

  info->audio_mix_element = gst_bin_get_by_name(
    GST_BIN(info->audio_mix_pipeline), "mixer"
  );

  info->audio_volume_element = gst_bin_get_by_name(
    GST_BIN(info->audio_mix_pipeline), "control"
  );

  {
    GstBus *bus = gst_element_get_bus(info->audio_mix_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    gst_element_set_state(info->audio_mix_pipeline, GST_STATE_PLAYING);
  }
}

static void
_setup_video_gst_pipelines(MESSENGER_DiscourseInfo *info)
{
  g_assert(info);

  info->video_record_pipeline = gst_parse_launch(
    "pipewiresrc name=source ! "
    "video/x-raw,framerate={ [ 0/1, 30/1 ] } ! "
    "videoscale ! video/x-raw,height=[1,1280],width=[1,1280] ! "
    "videoconvert ! video/x-raw,format=I420 ! "
    "x264enc bitrate=1000 speed-preset=fast bframes=0 key-int-max=30 tune=zerolatency byte-stream=true ! "
    "video/x-h264,profile=baseline ! rtph264pay aggregate-mode=zero-latency mtu=45000 ! "
    "tee ! queue ! rtpmux name=mux ! capsfilter name=filter ! fdsink name=sink",
    NULL
  );

  info->video_record_source = gst_bin_get_by_name(
    GST_BIN(info->video_record_pipeline), "source"
  );

  info->video_record_sink = gst_bin_get_by_name(
    GST_BIN(info->video_record_pipeline), "sink"
  );

  GstElement *mux = gst_bin_get_by_name(
    GST_BIN(info->video_record_pipeline),
    "mux"
  );

  info->video_heartbeat_source = gst_element_factory_make("appsrc", NULL);

  {
    gst_bin_add(
      GST_BIN(info->video_record_pipeline),
      info->video_heartbeat_source
    );

    GstPad *mux_pad = gst_element_request_pad_simple(mux, "sink_%u");

    GstPad *pad = gst_element_get_static_pad(
      info->video_heartbeat_source, "src"
    );

    gst_pad_link(pad, mux_pad);
  }

  GstElement *filter = gst_bin_get_by_name(
    GST_BIN(info->video_record_pipeline), "filter"
  );

  {
    GstBus *bus = gst_element_get_bus(info->video_record_pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, info);
    gst_object_unref(bus);

    GstCaps *caps = gst_caps_new_simple (
      "application/x-rtp",
      "media", G_TYPE_STRING, "video",
      "payload", G_TYPE_INT, 96,
      "clock-rate", G_TYPE_INT, 90000,
      "encoding-name", G_TYPE_STRING, "H264",
      NULL
    );

    g_object_set(info->video_heartbeat_source, "caps", caps, NULL);
    g_object_set(filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    const int fd = GNUNET_CHAT_discourse_get_fd(info->discourse);
    if (-1 != fd)
      g_object_set(info->video_record_sink, "fd", fd, NULL);

    gst_element_set_state(info->video_record_pipeline, GST_STATE_NULL);
  }
}

enum GNUNET_GenericReturnValue
discourse_create_info(struct GNUNET_CHAT_Discourse *discourse)
{
  if ((!discourse) || (GNUNET_CHAT_discourse_get_user_pointer(discourse)))
    return GNUNET_NO;

  MESSENGER_DiscourseInfo* info = g_malloc(sizeof(MESSENGER_DiscourseInfo));

  if (!info)
    return GNUNET_NO;

  info->discourse = discourse;

  info->audio_record_pipeline = NULL;
  info->audio_record_sink = NULL;

  info->video_record_pipeline = NULL;
  info->video_record_source = NULL;
  info->video_record_sink = NULL;
  info->video_heartbeat_source = NULL;

  info->audio_mix_pipeline = NULL;
  info->audio_mix_element = NULL;
  info->audio_volume_element = NULL;

  pthread_mutex_init(&(info->mutex), NULL);

  info->heartbeat = 0;
  info->subscriptions = NULL;

  const struct GNUNET_CHAT_DiscourseId *id = GNUNET_CHAT_discourse_get_id(
    info->discourse
  );

  if (0 == GNUNET_memcmp(id, get_voice_discourse_id()))
     _setup_audio_gst_pipelines(info);
  else if (0 == GNUNET_memcmp(id, get_video_discourse_id()))
    _setup_video_gst_pipelines(info);

  GNUNET_CHAT_discourse_set_user_pointer(discourse, info);
  return GNUNET_YES;
}

void
discourse_destroy_info(struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert(discourse);

  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  pthread_mutex_lock(&(info->mutex));

  if (info->subscriptions)
  {
    MESSENGER_DiscourseSubscriptionInfo *sub_info;
    GList *sub = info->subscriptions;
    while (sub)
    {
      sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
      discourse_subscription_destroy_info(sub_info);
      sub = g_list_next(sub);
    }

    g_list_free(info->subscriptions);
  }

  pthread_mutex_unlock(&(info->mutex));

  if (info->heartbeat)
    util_source_remove(info->heartbeat);

  if (info->video_record_pipeline)
  {
    gst_element_set_state(info->video_record_pipeline, GST_STATE_NULL);

    GstElement *mux = gst_bin_get_by_name(
      GST_BIN(info->video_record_pipeline),
      "mux"
    );

    GstPad *mux_pad = gst_element_request_pad_simple(mux, "sink_%u");
    GstPad *pad = gst_element_get_static_pad(
      info->video_heartbeat_source, "src"
    );

    gst_pad_unlink(pad, mux_pad);

    gst_bin_remove(
      GST_BIN(info->video_record_pipeline),
      info->video_heartbeat_source
    );

    gst_object_unref(GST_OBJECT(info->video_record_pipeline));
  }

  if (info->audio_mix_pipeline)
  {
    gst_element_set_state(info->audio_mix_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->audio_mix_pipeline));
  }

  if (info->audio_record_pipeline)
  {
    gst_element_set_state(info->audio_record_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(info->audio_record_pipeline));
  }

  pthread_mutex_destroy(&(info->mutex));

  g_free(info);

  GNUNET_CHAT_discourse_set_user_pointer(discourse, NULL);
}

static enum GNUNET_GenericReturnValue
_append_contact_to_subscription_list(void *cls,
                                     struct GNUNET_CHAT_Discourse *discourse,
                                     struct GNUNET_CHAT_Contact *contact)
{
  g_assert((cls) && (discourse) && (contact));

  GList **list = cls;
  *list = g_list_append(*list, contact);
  return GNUNET_YES;
}

void
discourse_update_subscriptions(struct GNUNET_CHAT_Discourse *discourse)
{
  g_assert(discourse);

  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  GList *list = NULL;
  GNUNET_CHAT_discourse_iterate_contacts(
    info->discourse,
    _append_contact_to_subscription_list,
    &list
  );

  pthread_mutex_lock(&(info->mutex));

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info;

  GList *drop = NULL;

  while (sub)
  {
    GList *link = sub;

    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (link->data);
    sub = g_list_next(sub);

    if (g_list_find(list, sub_info->contact))
    {
      list = g_list_remove(list, sub_info->contact);
      continue;
    }

    GList *rest = g_list_remove_link(info->subscriptions, link);

    if (!drop)
      drop = link;
    else
      drop = g_list_concat(drop, link);

    if (!rest)
      break;
  }

  sub = drop;
  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);

    discourse_subscription_destroy_info(sub_info);
    sub = g_list_next(sub);
  }

  if (drop)
    g_list_free(drop);

  sub = list;
  while (sub)
  {
    sub_info = discourse_subscription_create_info(
      info, (struct GNUNET_CHAT_Contact*) sub->data
    );

    if (sub_info)
      info->subscriptions = g_list_append(
        info->subscriptions, sub_info
      );
    
    sub = g_list_next(sub);
  }

  pthread_mutex_unlock(&(info->mutex));

  if (list)
    g_list_free(list);
}

void
discourse_stream_message(struct GNUNET_CHAT_Discourse *discourse,
                         const struct GNUNET_CHAT_Message *message)
{
  g_assert((discourse) && (message));

  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  pthread_mutex_lock(&(info->mutex));

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info = NULL;

  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    if (GNUNET_CHAT_message_get_sender(message) == sub_info->contact)
      break;

    sub = g_list_next(sub);
  }

  if (sub_info)
    discourse_subscription_stream_message(sub_info, message);

  pthread_mutex_unlock(&(info->mutex));
}

bool
discourse_has_controls(struct GNUNET_CHAT_Discourse *discourse,
                       MESSENGER_DiscourseControl control)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  switch (control)
  {
    case MESSENGER_DISCOURSE_CTRL_MICROPHONE:
      return (info->audio_record_pipeline? TRUE : FALSE);
    case MESSENGER_DISCOURSE_CTRL_SPEAKERS:
      return (info->audio_mix_pipeline? TRUE : FALSE);
    case MESSENGER_DISCOURSE_CTRL_WEBCAM:
      return (info->video_record_pipeline? TRUE : FALSE);
    case MESSENGER_DISCOURSE_CTRL_SCREEN_CAPTURE:
      return (info->video_record_pipeline? TRUE : FALSE);
    default:
      return FALSE;
  }
}

void
discourse_set_volume(struct GNUNET_CHAT_Discourse *discourse,
                     double volume)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->audio_mix_pipeline)) || (!(info->audio_volume_element)))
    return;

  g_object_set(info->audio_volume_element, "volume", volume, NULL);
}

double
discourse_get_volume(struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if ((!info) || (!(info->audio_mix_pipeline)) || (!(info->audio_volume_element)))
    return 0.0;

  gdouble volume;
  g_object_get(info->audio_volume_element, "volume", &volume, NULL);

  return volume;
}

void
discourse_set_mute(struct GNUNET_CHAT_Discourse *discourse,
                   bool mute)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  if ((mute) && (info->heartbeat))
  {
    util_source_remove(info->heartbeat);
    info->heartbeat = 0;
  }

  const GstState state = mute? GST_STATE_NULL : GST_STATE_PLAYING;

  if (info->audio_record_pipeline)
    gst_element_set_state(info->audio_record_pipeline, state);

  if (info->video_record_pipeline)
  {
    gst_element_set_state(info->video_record_pipeline, state);

    if ((!mute) && (!(info->heartbeat)))
      info->heartbeat = util_idle_add(
        G_SOURCE_FUNC(_discourse_video_heartbeat),
        info
      );
  }
}

bool
discourse_is_mute(const struct GNUNET_CHAT_Discourse *discourse)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return TRUE;

  GstState state = GST_STATE_NULL;

  if (info->audio_record_pipeline)
    gst_element_get_state(
      info->audio_record_pipeline,
      &state,
      NULL,
      GST_CLOCK_TIME_NONE
    );
  
  if (info->video_record_pipeline)
    gst_element_get_state(
      info->video_record_pipeline,
      &state,
      NULL,
      GST_CLOCK_TIME_NONE
    );

  return (GST_STATE_PLAYING != state);
}

void
discourse_set_target(struct GNUNET_CHAT_Discourse *discourse,
                     const char *name)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return;

  if (info->video_record_source)
    g_object_set(
      G_OBJECT(info->video_record_source),
      "target-object",
      name,
      NULL
    );
}

gboolean
discourse_link_widget(const struct GNUNET_CHAT_Discourse *discourse,
                      const struct GNUNET_CHAT_Contact *contact,
                      GtkContainer *container)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  pthread_mutex_lock(&(info->mutex));

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info = NULL;

  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    if ((sub_info) && (contact == sub_info->contact))
      break;

    sub_info = NULL;
    sub = g_list_next(sub);
  }

  gboolean linked = FALSE;
  if (sub_info)
    linked = discourse_subscription_link_widget(sub_info, container);

  pthread_mutex_unlock(&(info->mutex));
  return linked;
}

gboolean
discourse_is_active(const struct GNUNET_CHAT_Discourse *discourse,
                    const struct GNUNET_CHAT_Contact *contact)
{
  MESSENGER_DiscourseInfo* info = GNUNET_CHAT_discourse_get_user_pointer(discourse);

  if (!info)
    return FALSE;

  pthread_mutex_lock(&(info->mutex));

  GList *sub = info->subscriptions;
  MESSENGER_DiscourseSubscriptionInfo *sub_info = NULL;

  while (sub)
  {
    sub_info = (MESSENGER_DiscourseSubscriptionInfo*) (sub->data);
    if ((sub_info) && (contact == sub_info->contact))
      break;

    sub_info = NULL;
    sub = g_list_next(sub);
  }

  gboolean active = FALSE;
  if (!sub_info)
    goto unlock_info_mutex;

  GstState state = GST_STATE_NULL;

  if (sub_info->audio_stream_source)
    gst_element_get_state(
      sub_info->audio_stream_source,
      &state,
      NULL,
      GST_CLOCK_TIME_NONE
    );
  
  if (sub_info->video_stream_source)
    gst_element_get_state(
      sub_info->video_stream_source,
      &state,
      NULL,
      GST_CLOCK_TIME_NONE
    );

  if (GST_STATE_PLAYING != state)
    goto unlock_info_mutex;

  pthread_mutex_lock(&(sub_info->mutex));

  if (!(sub_info->end_datetime))
    goto unlock_sub_info_mutex;

  GDateTime *dt = g_date_time_new_now_local();
  if (dt)
  {
    GTimeSpan ts = g_date_time_difference(sub_info->end_datetime, dt);
    g_date_time_unref(dt);
    active = (ts >= 0);
  }

unlock_sub_info_mutex:
  pthread_mutex_unlock(&(sub_info->mutex));

unlock_info_mutex:
  pthread_mutex_unlock(&(info->mutex));
  return active;
}
