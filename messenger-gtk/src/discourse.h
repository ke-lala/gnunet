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
 * @file discourse.h
 */

#ifndef DISCOURSE_H_
#define DISCOURSE_H_

#include "application.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gtk-3.0/gtk/gtk.h>
#include <pthread.h>

/**
 * Returns the discourse id for a typical voice chat.
 *
 * @return Voice chat discourse id
 */
const struct GNUNET_CHAT_DiscourseId*
get_voice_discourse_id();

/**
 * Returns the discourse id for a typical video chat.
 *
 * @return Video chat discourse id
 */
const struct GNUNET_CHAT_DiscourseId*
get_video_discourse_id();

typedef enum MESSENGER_DiscourseControl {
  MESSENGER_DISCOURSE_CTRL_MICROPHONE = 1,
  MESSENGER_DISCOURSE_CTRL_SPEAKERS = 2,
  MESSENGER_DISCOURSE_CTRL_WEBCAM = 3,
  MESSENGER_DISCOURSE_CTRL_SCREEN_CAPTURE = 4,

  MESSENGER_DISCOURSE_CTRL_UNKNOWN = 0
} MESSENGER_DiscourseControl;

typedef struct MESSENGER_DiscourseInfo
{
  struct GNUNET_CHAT_Discourse *discourse;

  GstElement *audio_record_pipeline;
  GstElement *audio_record_sink;

  GstElement *video_record_pipeline;
  GstElement *video_record_source;
  GstElement *video_record_sink;
  GstElement *video_heartbeat_source;

  GstElement *audio_mix_pipeline;
  GstElement *audio_mix_element;
  GstElement *audio_volume_element;

  pthread_mutex_t mutex;
  guint heartbeat;
  
  GList *subscriptions;
} MESSENGER_DiscourseInfo;

typedef struct MESSENGER_DiscourseSubscriptionInfo
{
  MESSENGER_DiscourseInfo *discourse;
  struct GNUNET_CHAT_Contact *contact;

  GstElement *audio_stream_source;
  GstElement *audio_jitter_buffer;
  GstElement *audio_depay;
  GstElement *audio_converter;

  GstElement *video_stream_pipeline;
  GstElement *video_stream_source;
  GstElement *video_stream_sink;

  GstPad *audio_mix_pad;
  GList *buffers; 

  uint64_t position;
  uint64_t last_timestamp;

  pthread_mutex_t mutex;

  GDateTime *end_datetime;
} MESSENGER_DiscourseSubscriptionInfo;

/**
 * Creates a discourse information struct to potentially
 * update all GUI appearances of a specific discourse at
 * once.
 *
 * @param discourse Chat discourse
 * @return #GNUNET_YES on info creation, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
discourse_create_info(struct GNUNET_CHAT_Discourse *discourse);

/**
 * Destroys and frees resources allocated for a given
 * discourse information struct.
 *
 * @param discourse Chat discourse
 */
void
discourse_destroy_info(struct GNUNET_CHAT_Discourse *discourse);

/**
 * Updates the connected UI elements for a given
 * discourse and its subscriptions depending on the 
 * current state.
 *
 * @param discourse Chat discourse
 */
void
discourse_update_subscriptions(struct GNUNET_CHAT_Discourse *discourse);

/**
 * Pushes a data message of a given discourse to
 * update UI elements or output regarding its content.
 *
 * @param discourse Chat discourse
 * @param message Chat message
 */
void
discourse_stream_message(struct GNUNET_CHAT_Discourse *discourse,
                         const struct GNUNET_CHAT_Message *message);

/**
 * Returns whether a given discourse should have controls
 * regarding a given type of input/output.
 *
 * @param discourse Chat discourse
 * @return #TRUE if there should be controls, otherwise #FALSE
 */
bool
discourse_has_controls(struct GNUNET_CHAT_Discourse *discourse,
                       MESSENGER_DiscourseControl control);

/**
 * Sets the volume for speakers of a given discourse.
 *
 * @param discourse Chat discourse
 * @param volume Speakers volume
 */
void
discourse_set_volume(struct GNUNET_CHAT_Discourse *discourse,
                     double volume);

/**
 * Returns the volume for speakers of a given discourse.
 *
 * @param discourse Chat discourse
 * @return Speakers volume
 */
double
discourse_get_volume(struct GNUNET_CHAT_Discourse *discourse);

/**
 * Mutes/Unmutes the microphone of a given discourse.
 *
 * @param discourse Chat discourse
 * @param mute Mute flag
 */
void
discourse_set_mute(struct GNUNET_CHAT_Discourse *discourse,
                   bool mute);

/**
 * Returns whether the microphone of a given discourse
 * is muted or not.
 *
 * @param discourse Chat discourse
 * @return #TRUE if muted, #FALSE otherwise
 */
bool
discourse_is_mute(const struct GNUNET_CHAT_Discourse *discourse);

/**
 * Sets the capture target of a given discourse by name.
 *
 * @param discourse Chat discourse
 * @param name Target name
 */
void
discourse_set_target(struct GNUNET_CHAT_Discourse *discourse,
                     const char *name);

/**
 * Links/Unlinks a widget from the video pipeline of a discourse
 * for a given chat contact to a selected container as
 * child.
 *
 * @param discourse Chat discourse
 * @param contact Chat contact
 * @param container Container
 * @param linked Linking flag
 * @return #TRUE if successful, #FALSE otherwise
 */
gboolean
discourse_link_widget(const struct GNUNET_CHAT_Discourse *discourse,
                      const struct GNUNET_CHAT_Contact *contact,
                      GtkContainer *container);

/**
 * Returns whether the data stream of a selected chat contact in 
 * a given discourse is active or not.
 *
 * @param discourse Chat discourse
 * @param contact Chat contact
 * @return #TRUE if active, #FALSE otherwise
 */
gboolean
discourse_is_active(const struct GNUNET_CHAT_Discourse *discourse,
                    const struct GNUNET_CHAT_Contact *contact);

#endif /* DISCOURSE_H_ */
