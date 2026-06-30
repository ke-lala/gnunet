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
 * @file gnunet_chat_lobby.c
 */

#include "gnunet_chat_lobby.h"

#include "gnunet_chat_handle.h"
#include "gnunet_chat_lobby_intern.c"
#include <gnunet/gnunet_messenger_service.h>

struct GNUNET_CHAT_Lobby*
lobby_create (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(handle);

  struct GNUNET_CHAT_Lobby *lobby = GNUNET_new(struct GNUNET_CHAT_Lobby);

  lobby->handle = handle;
  lobby->destruction = NULL;
  lobby->context = NULL;
  lobby->uri = NULL;

  lobby->op = NULL;
  lobby->query = NULL;

  lobby->expiration = GNUNET_TIME_absolute_get_forever_();
  lobby->callback = NULL;
  lobby->cls = NULL;

  return lobby;
}

void
lobby_destroy (struct GNUNET_CHAT_Lobby *lobby)
{
  GNUNET_assert(lobby);

  if (lobby->destruction)
    GNUNET_SCHEDULER_cancel(lobby->destruction);

  if ((!(lobby->op)) && (!(lobby->query)))
    goto skip_deletion;

  handle_delete_lobby(lobby->handle, lobby);

skip_deletion:
  if (lobby->op)
    GNUNET_IDENTITY_cancel(lobby->op);

  if (lobby->query)
    GNUNET_NAMESTORE_cancel(lobby->query);

  if (lobby->uri)
    uri_destroy(lobby->uri);

  GNUNET_free(lobby);
}

void
lobby_open (struct GNUNET_CHAT_Lobby *lobby,
            struct GNUNET_TIME_Relative delay,
            GNUNET_CHAT_LobbyCallback callback,
            void *cls)
{
  GNUNET_assert(lobby);

  char *name;

  lobby->expiration = GNUNET_TIME_relative_to_absolute(delay);
  lobby->callback = callback;
  lobby->cls = cls;

  if (lobby->op)
  {
    GNUNET_IDENTITY_cancel(lobby->op);
    goto open_zone;
  }

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_MESSENGER_create_room_key(
    &key,
    NULL,
    GNUNET_NO,
    GNUNET_NO,
    GNUNET_NO
  );

  struct GNUNET_MESSENGER_Room *room = GNUNET_MESSENGER_open_room(
    lobby->handle->messenger,
    &key
  );

  if (!room)
    return;

  lobby->context = context_create_from_room(lobby->handle, room);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      lobby->handle->contexts, &(key.hash), lobby->context,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    context_destroy(lobby->context);
    lobby->context = NULL;

    GNUNET_MESSENGER_close_room(room);
    return;
  }

open_zone:
  util_lobby_name(&(key.hash), &name);

  lobby->op = GNUNET_IDENTITY_create(
    lobby->handle->identity,
    name,
    NULL,
    GNUNET_PUBLIC_KEY_TYPE_EDDSA,
    cont_lobby_identity_create,
    lobby
  );

  GNUNET_free(name);
}
