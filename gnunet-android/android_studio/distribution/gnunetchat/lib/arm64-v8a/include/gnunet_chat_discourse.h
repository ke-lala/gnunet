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
 * @file gnunet_chat_discourse.h
 */

#ifndef GNUNET_CHAT_DISCOURSE_H_
#define GNUNET_CHAT_DISCOURSE_H_

#include "gnunet_chat_util.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_time_lib.h>
#include <gnunet/gnunet_util_lib.h>

struct GNUNET_CHAT_Contact;
struct GNUNET_CHAT_Context;

struct GNUNET_CHAT_DiscourseSubscription
{
  struct GNUNET_CHAT_DiscourseSubscription *prev;
  struct GNUNET_CHAT_DiscourseSubscription *next;

  struct GNUNET_CHAT_Discourse *discourse;

  struct GNUNET_TIME_Absolute start;
  struct GNUNET_TIME_Absolute end;

  struct GNUNET_CHAT_Contact *contact;
  struct GNUNET_SCHEDULER_Task *task;
};

struct GNUNET_CHAT_Discourse
{
  struct GNUNET_CHAT_Context *context;

  struct GNUNET_CHAT_DiscourseId id;
  int pipe [2];

  struct GNUNET_CHAT_DiscourseSubscription *head;
  struct GNUNET_CHAT_DiscourseSubscription *tail;

  struct GNUNET_SCHEDULER_Task *pipe_task;

  void *user_pointer;
};

/**
 * Creates a chat discourse within a chat <i>context</i>
 * with a selected discourse <i>id</i>.
 *
 * @param[in,out] context Chat context
 * @param[in] id Discourse id
 * @return New chat discourse
 */
struct GNUNET_CHAT_Discourse*
discourse_create (struct GNUNET_CHAT_Context *context,
                  const struct GNUNET_CHAT_DiscourseId *id);

/**
 * Destroys a chat <i>discourse</i> and frees its memory.
 *
 * @param[in,out] discourse Chat discourse
 */
void
discourse_destroy (struct GNUNET_CHAT_Discourse *discourse);

/**
 * Updates the subscription of a specific chat <i>contact</i>
 * to a given chat <i>discourse</i> with a selected
 * <i>timestamp</i> and relative <i>time</i> window.
 *
 * @param[in,out] discourse Chat discourse
 * @param[in,out] contact Chat contact
 * @param[in] timestamp Timestamp
 * @param[in] time Time window
 * @return #GNUNET_YES on updating an existing subscription,
 *         #GNUNET_SYSERR on failure, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
discourse_subscribe (struct GNUNET_CHAT_Discourse *discourse,
                     struct GNUNET_CHAT_Contact *contact,
                     const struct GNUNET_TIME_Absolute timestamp,
                     const struct GNUNET_TIME_Relative time);

/**
 * Ends the subscription of a specific chat <i>contact</i>
 * to a given chat <i>discourse</i> at a selected 
 * <i>timestamp</i> with a potential <i>delay</i>.
 *
 * @param[in,out] discourse Chat discourse
 * @param[in,out] contact Chat contact
 * @param[in] timestamp Timestamp
 * @param[in] delay Delay
 */
void
discourse_unsubscribe (struct GNUNET_CHAT_Discourse *discourse,
                       struct GNUNET_CHAT_Contact *contact,
                       const struct GNUNET_TIME_Absolute timestamp,
                       const struct GNUNET_TIME_Relative delay);

#endif /* GNUNET_CHAT_DISCOURSE_H_ */
