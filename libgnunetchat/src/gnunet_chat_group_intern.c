/*
   This file is part of GNUnet.
   Copyright (C) 2021--2026 GNUnet e.V.

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
 * @file gnunet_chat_group_intern.c
 */

#include "gnunet_chat_context.h"
#include "gnunet_chat_handle.h"

#include <gnunet/gnunet_messenger_service.h>

#define GNUNET_UNUSED __attribute__ ((unused))

void
search_group_by_topic(void *cls,
                      const struct GNUNET_PeerIdentity *door,
                      GNUNET_UNUSED const struct GNUNET_PeerIdentity *get_path,
                      GNUNET_UNUSED unsigned int get_path_length,
                      GNUNET_UNUSED const struct GNUNET_PeerIdentity *put_path,
                      GNUNET_UNUSED unsigned int put_path_length)
{
  struct GNUNET_CHAT_Group *group = cls;

  GNUNET_assert(
    (group) &&
		(group->handle) &&
		(group->handle->cfg) &&
		(group->handle->messenger) &&
		(group->context) &&
		(group->context->room)
  );

  const struct GNUNET_PeerIdentity *pid = group->handle->pid;

  if ((pid != NULL) && (0 == GNUNET_memcmp(pid, door)))
    return;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_memcpy(
    &(key.hash),
    GNUNET_MESSENGER_room_get_key(group->context->room),
    sizeof(key.hash)
  );

  if ((GNUNET_YES == GNUNET_CONTAINER_multipeermap_contains(
      group->registry, door)) ||
      (GNUNET_OK != GNUNET_CONTAINER_multipeermap_put(
      group->registry, door, NULL,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST)))
    return;

  GNUNET_MESSENGER_enter_room(
    group->handle->messenger,
    door,
    &key
  );
}
