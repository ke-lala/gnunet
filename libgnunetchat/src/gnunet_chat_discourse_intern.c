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
 * @file gnunet_chat_discourse_intern.c
 */

#include "gnunet_chat_context.h"

#define GNUNET_UNUSED __attribute__ ((unused))

#define MAX_WRITE_SIZE (    \
  GNUNET_MAX_MESSAGE_SIZE - \
  GNUNET_MIN_MESSAGE_SIZE - \
  sizeof (struct GNUNET_MESSENGER_Message))

static void
cb_read_discourse_pipe (void *cls);

void
cb_reinit_discourse_pipe (void *cls)
{
  struct GNUNET_CHAT_Discourse *discourse = cls;

  GNUNET_assert(discourse);

  discourse->pipe_task = NULL;

  if (-1 == discourse->pipe[0])
    return;

  struct GNUNET_NETWORK_FDSet *rs = GNUNET_NETWORK_fdset_create();

  GNUNET_NETWORK_fdset_set_native(rs, discourse->pipe[0]);

  discourse->pipe_task = GNUNET_SCHEDULER_add_select(
    GNUNET_SCHEDULER_PRIORITY_DEFAULT,
    GNUNET_TIME_UNIT_FOREVER_REL,
    rs,
    NULL,
    cb_read_discourse_pipe,
    discourse
  );

  GNUNET_NETWORK_fdset_destroy(rs);
}

static void
cb_read_discourse_pipe (void *cls)
{
  struct GNUNET_CHAT_Discourse *discourse = cls;

  GNUNET_assert((discourse) && (-1 != discourse->pipe[0]));

  discourse->pipe_task = NULL;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TALK;

  util_shorthash_from_discourse_id(
    &(discourse->id),
    &(msg.body.talk.discourse)
  );

  char data [MAX_WRITE_SIZE];
  ssize_t len;

  do
  {
    len = read(discourse->pipe[0], data, MAX_WRITE_SIZE);

    if (len <= 0)
      break;

    msg.body.talk.data = data;
    msg.body.talk.length = (uint16_t) len;

    GNUNET_MESSENGER_send_message(discourse->context->room, &msg, NULL);
  }
  while (MAX_WRITE_SIZE == len);

  if (len < 0)
    return;

  discourse->pipe_task = GNUNET_SCHEDULER_add_now(
    cb_reinit_discourse_pipe, discourse
  );
}
