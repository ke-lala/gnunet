/*
   This file is part of GNUnet.
   Copyright (C) 2022--2026 GNUnet e.V.

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
 * @file gnunet_chat_lobby_intern.c
 */

#include "gnunet_chat_context.h"

void
cont_lobby_write_records (void *cls,
			                    enum GNUNET_ErrorCode ec)
{
  struct GNUNET_CHAT_Lobby *lobby = cls;

  GNUNET_assert(lobby);

  lobby->query = NULL;

  handle_delete_lobby(lobby->handle, lobby);

  if (GNUNET_EC_NONE == ec)
  {
    context_write_records(lobby->context);
    goto call_cb;
  }

  handle_send_internal_message(
    lobby->handle,
    NULL,
    lobby->context,
    GNUNET_CHAT_FLAG_WARNING,
    GNUNET_ErrorCode_get_hint(ec),
    GNUNET_YES
  );

  if (lobby->uri)
    uri_destroy(lobby->uri);

  lobby->uri = NULL;

call_cb:
  if (lobby->callback)
    lobby->callback(lobby->cls, lobby->uri);
}

void
cont_lobby_identity_create (void *cls,
                            const struct GNUNET_CRYPTO_BlindablePrivateKey *zone,
                            enum GNUNET_ErrorCode ec)
{
  struct GNUNET_CHAT_Lobby *lobby = cls;

  GNUNET_assert(lobby);

  lobby->op = NULL;

  if (GNUNET_EC_NONE != ec)
  {
    handle_send_internal_message(
	    lobby->handle,
      NULL,
    	lobby->context,
    	GNUNET_CHAT_FLAG_WARNING,
	    GNUNET_ErrorCode_get_hint(ec),
      GNUNET_YES
    );

    return;
  }

  const struct GNUNET_HashCode *key = GNUNET_MESSENGER_room_get_key(
    lobby->context->room
  );

  const struct GNUNET_PeerIdentity *pid = lobby->handle->pid;

  struct GNUNET_MESSENGER_RoomEntryRecord room;
  GNUNET_memcpy(&(room.key), key, sizeof(room.key));

  if (pid)
    GNUNET_memcpy(&(room.door), pid, sizeof(room.door));
  else
    memset(&(room.door), 0, sizeof(room.door));

  struct GNUNET_GNSRECORD_Data data [1];
  data[0].record_type = GNUNET_GNSRECORD_TYPE_MESSENGER_ROOM_ENTRY;
  data[0].data = &room;
  data[0].data_size = sizeof(room);
  data[0].expiration_time = lobby->expiration.abs_value_us;
  data[0].flags = GNUNET_GNSRECORD_RF_NONE;

  if (lobby->uri)
    uri_destroy(lobby->uri);

  struct GNUNET_CRYPTO_BlindablePublicKey public_zone;
  GNUNET_CRYPTO_blindable_key_get_public(zone, &public_zone);

  char *label;
  util_get_context_label(lobby->context->type, key, &label);

  lobby->uri = uri_create_chat(&public_zone, label);
  GNUNET_free(label);

  lobby->query = GNUNET_NAMESTORE_record_set_store(
    lobby->handle->namestore,
    zone,
    lobby->uri->chat.label,
    1,
    data,
    cont_lobby_write_records,
    lobby
  );
}
