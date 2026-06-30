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
 * @file gnunet_chat_group.c
 */

#include "gnunet_chat_group.h"
#include "gnunet_chat_util.h"

#include "gnunet_chat_group_intern.c"
#include <gnunet/gnunet_scheduler_lib.h>

static const unsigned int initial_map_size_of_context = 8;
static const uint16_t group_regex_compression = 6;

struct GNUNET_CHAT_Group*
group_create_from_context (struct GNUNET_CHAT_Handle *handle,
			                     struct GNUNET_CHAT_Context *context)
{
  GNUNET_assert((handle) && (context));

  struct GNUNET_CHAT_Group* group = GNUNET_new(struct GNUNET_CHAT_Group);

  group->handle = handle;
  group->context = context;

  group->destruction = NULL;

  group->announcement = NULL;
  group->search = NULL;

  group->registry = GNUNET_CONTAINER_multipeermap_create(
    initial_map_size_of_context, GNUNET_NO);

  group->user_pointer = NULL;

  return group;
}

void
group_destroy (struct GNUNET_CHAT_Group* group)
{
  GNUNET_assert(group);

  if (group->destruction)
    GNUNET_SCHEDULER_cancel(group->destruction);

  if (group->registry)
    GNUNET_CONTAINER_multipeermap_destroy(group->registry);

  if (group->search)
    GNUNET_REGEX_search_cancel(group->search);

  if (group->announcement)
    GNUNET_REGEX_announce_cancel(group->announcement);

  GNUNET_free(group);
}

void
group_publish (struct GNUNET_CHAT_Group* group)
{
  GNUNET_assert(
    (group) &&
		(group->context) &&
		(group->context->topic) &&
		(group->handle) &&
		(group->handle->cfg)
  );

  char* topic = NULL;
  GNUNET_asprintf (
    &topic,
    "GNUNET_CHAT_%s",
    group->context->topic
  );

  group->announcement = GNUNET_REGEX_announce(
    group->handle->cfg, topic,
    GNUNET_TIME_relative_get_minute_(),
    group_regex_compression
  );

  group->search = GNUNET_REGEX_search(
    group->handle->cfg, topic,
    search_group_by_topic, group
  );

  GNUNET_free(topic);
}
