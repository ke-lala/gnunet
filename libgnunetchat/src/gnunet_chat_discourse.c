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
 * @file gnunet_chat_discourse.c
 */

#include "gnunet_chat_discourse.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_time_lib.h>

#include <unistd.h>

#include "gnunet_chat_discourse_intern.c"

struct GNUNET_CHAT_Discourse*
discourse_create (struct GNUNET_CHAT_Context *context,
                  const struct GNUNET_CHAT_DiscourseId *id)
{
  GNUNET_assert((context) && (id));

  struct GNUNET_CHAT_Discourse *discourse = GNUNET_new(struct GNUNET_CHAT_Discourse);

  discourse->context = context;

  GNUNET_memcpy(&(discourse->id), id, sizeof(struct GNUNET_CHAT_DiscourseId));

  if (0 != pipe(discourse->pipe))
  {
    discourse->pipe[0] = -1;
    discourse->pipe[1] = -1;
  }

  discourse->head = NULL;
  discourse->tail = NULL;

  discourse->pipe_task = GNUNET_SCHEDULER_add_now(
    cb_reinit_discourse_pipe, discourse
  );

  discourse->user_pointer = NULL;

  return discourse;
}

static void
discourse_remove_subscription (void *cls)
{
  struct GNUNET_CHAT_DiscourseSubscription *sub = cls;
  struct GNUNET_CHAT_Discourse *discourse = sub->discourse;

  GNUNET_CONTAINER_DLL_remove(
    discourse->head,
    discourse->tail,
    sub
  );

  GNUNET_free(sub);
}

void
discourse_destroy (struct GNUNET_CHAT_Discourse *discourse)
{
  GNUNET_assert(discourse);

  while (discourse->head)
  {
    struct GNUNET_CHAT_DiscourseSubscription *sub = discourse->head;

    if (sub->task)
      GNUNET_SCHEDULER_cancel(sub->task);

    discourse_remove_subscription(sub);
  }

  if (discourse->pipe_task)
    GNUNET_SCHEDULER_cancel(discourse->pipe_task);

  if (-1 != discourse->pipe[0])
    close(discourse->pipe[0]);
  if (-1 != discourse->pipe[1])
    close(discourse->pipe[1]);

  GNUNET_free(discourse);
}

enum GNUNET_GenericReturnValue
discourse_subscribe (struct GNUNET_CHAT_Discourse *discourse,
                     struct GNUNET_CHAT_Contact *contact,
                     const struct GNUNET_TIME_Absolute timestamp,
                     const struct GNUNET_TIME_Relative time)
{
  GNUNET_assert((discourse) && (contact));

  const struct GNUNET_TIME_Absolute end = GNUNET_TIME_absolute_add(
    timestamp,
    time
  );

  if (GNUNET_TIME_absolute_cmp(end, <, GNUNET_TIME_absolute_get()))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_DiscourseSubscription *sub;
  for (sub = discourse->head; sub; sub = sub->next)
    if (sub->contact == contact)
      break;
  
  const enum GNUNET_GenericReturnValue update = (
    sub? GNUNET_YES : GNUNET_NO
  );

  if (!sub)
  {
    sub = GNUNET_new(struct GNUNET_CHAT_DiscourseSubscription);

    sub->prev = NULL;
    sub->next = NULL;

    sub->discourse = discourse;
    sub->contact = contact;

    GNUNET_CONTAINER_DLL_insert(
      discourse->head,
      discourse->tail,
      sub
    );
  }
  else if (sub->task)
    GNUNET_SCHEDULER_cancel(sub->task);

  sub->start = timestamp;
  sub->end = end;

  sub->task = GNUNET_SCHEDULER_add_at(
    end,
    discourse_remove_subscription,
    sub
  );

  return update;
}

void
discourse_unsubscribe (struct GNUNET_CHAT_Discourse *discourse,
                       struct GNUNET_CHAT_Contact *contact,
                       const struct GNUNET_TIME_Absolute timestamp,
                       const struct GNUNET_TIME_Relative delay)
{
  GNUNET_assert((discourse) && (contact));

  struct GNUNET_CHAT_DiscourseSubscription *sub;
  for (sub = discourse->head; sub; sub = sub->next)
    if (sub->contact == contact)
      break;

  if ((!sub) || (GNUNET_TIME_absolute_cmp(sub->start, >, timestamp)))
    return;

  const struct GNUNET_TIME_Absolute exit = GNUNET_TIME_absolute_add(
    timestamp, delay
  );

  if (GNUNET_TIME_absolute_cmp(exit, <, sub->end))
    sub->end = exit;

  if (sub->task)
    GNUNET_SCHEDULER_cancel(sub->task);

  if (GNUNET_TIME_absolute_cmp(sub->end, <, GNUNET_TIME_absolute_get()))
    discourse_remove_subscription(sub);
  else
    sub->task = GNUNET_SCHEDULER_add_at(
      sub->end,
      discourse_remove_subscription,
      sub
    );
}
