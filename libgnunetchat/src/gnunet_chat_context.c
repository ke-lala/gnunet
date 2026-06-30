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
 * @file gnunet_chat_context.c
 */

#include "gnunet_chat_context.h"
#include "gnunet_chat_file.h"
#include "gnunet_chat_handle.h"
#include "gnunet_chat_message.h"
#include "gnunet_chat_util.h"

#include "gnunet_chat_context_intern.c"
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_namestore_service.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_util_lib.h>
#include <string.h>

static const unsigned int initial_map_size_of_room = 8;
static const unsigned int initial_map_size_of_contact = 4;

static void
init_new_context (struct GNUNET_CHAT_Context *context,
                  unsigned int initial_map_size)
{
  GNUNET_assert(context);

  context->flags = 0;
  context->nick = NULL;
  context->topic = NULL;
  context->deleted = GNUNET_NO;

  context->request_task = NULL;

  context->timestamps = GNUNET_CONTAINER_multishortmap_create(
    initial_map_size, GNUNET_NO);
  context->dependencies = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size, GNUNET_NO);
  context->messages = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size, GNUNET_NO);
  context->requests = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size, GNUNET_NO);
  context->taggings = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size, GNUNET_NO);
  context->invites = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size, GNUNET_NO);
  context->files = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size, GNUNET_NO);
  context->discourses = GNUNET_CONTAINER_multishortmap_create(
    initial_map_size, GNUNET_NO);
  
  context->user_pointer = NULL;

  context->member_pointers = GNUNET_CONTAINER_multishortmap_create(
    initial_map_size, GNUNET_NO);

  context->query = NULL;
}

struct GNUNET_CHAT_Context*
context_create_from_room (struct GNUNET_CHAT_Handle *handle,
			                    struct GNUNET_MESSENGER_Room *room)
{
  GNUNET_assert((handle) && (room));

  struct GNUNET_CHAT_Context* context = GNUNET_new(struct GNUNET_CHAT_Context);

  context->handle = handle;
  context->type = GNUNET_CHAT_CONTEXT_TYPE_UNKNOWN;
  
  init_new_context(context, initial_map_size_of_room);

  context->room = room;
  context->contact = NULL;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_memcpy(
    &(key.hash),
    GNUNET_MESSENGER_room_get_key(room),
    sizeof (key.hash)
  );

  if (key.code.group_bit)
    context->type = GNUNET_CHAT_CONTEXT_TYPE_GROUP;
  else
    context->type = GNUNET_CHAT_CONTEXT_TYPE_CONTACT;

  return context;
}

struct GNUNET_CHAT_Context*
context_create_from_contact (struct GNUNET_CHAT_Handle *handle,
			                       const struct GNUNET_MESSENGER_Contact *contact)
{
  GNUNET_assert((handle) && (contact));

  struct GNUNET_CHAT_Context* context = GNUNET_new(struct GNUNET_CHAT_Context);

  context->handle = handle;
  context->type = GNUNET_CHAT_CONTEXT_TYPE_CONTACT;

  init_new_context(context, initial_map_size_of_contact);

  context->room = NULL;
  context->contact = contact;

  return context;
}

void
context_destroy (struct GNUNET_CHAT_Context *context)
{
  GNUNET_assert(
    (context) &&
    (context->timestamps) &&
    (context->dependencies) &&
    (context->messages) &&
    (context->taggings) &&
    (context->invites) &&
    (context->files) &&
    (context->discourses)
  );

  if (context->request_task)
    GNUNET_SCHEDULER_cancel(context->request_task);

  if (context->query)
    GNUNET_NAMESTORE_cancel(context->query);

  GNUNET_CONTAINER_multishortmap_iterate(
    context->timestamps, it_destroy_context_timestamps, NULL
  );

  GNUNET_CONTAINER_multihashmap_clear(context->dependencies);
  GNUNET_CONTAINER_multihashmap_iterate(
    context->messages, it_destroy_context_messages, NULL
  );

  GNUNET_CONTAINER_multihashmap_iterate(
    context->taggings, it_destroy_context_taggings, NULL
  );

  GNUNET_CONTAINER_multihashmap_iterate(
    context->invites, it_destroy_context_invites, context
  );

  GNUNET_CONTAINER_multishortmap_iterate(
    context->discourses, it_destroy_context_discourses, NULL
  );

  GNUNET_CONTAINER_multishortmap_destroy(context->member_pointers);

  GNUNET_CONTAINER_multishortmap_destroy(context->timestamps);
  GNUNET_CONTAINER_multihashmap_destroy(context->dependencies);
  GNUNET_CONTAINER_multihashmap_destroy(context->messages);
  GNUNET_CONTAINER_multihashmap_destroy(context->requests);
  GNUNET_CONTAINER_multihashmap_destroy(context->taggings);
  GNUNET_CONTAINER_multihashmap_destroy(context->invites);
  GNUNET_CONTAINER_multihashmap_destroy(context->files);
  GNUNET_CONTAINER_multishortmap_destroy(context->discourses);

  if (context->topic)
    GNUNET_free(context->topic);

  if (context->nick)
    GNUNET_free(context->nick);

  GNUNET_free(context);
}

void
context_request_message (struct GNUNET_CHAT_Context* context,
                         const struct GNUNET_HashCode *hash)
{
  GNUNET_assert((context) && (hash));

  if ((!(context->room)) || (GNUNET_YES == context->deleted))
    return;

  if ((GNUNET_is_zero(hash)) || 
      (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(context->messages, hash)))
    return;

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(context->requests,
      hash, NULL, GNUNET_CONTAINER_MULTIHASHMAPOPTION_REPLACE))
    return;
  
  if (context->request_task)
    return;

  context->request_task = GNUNET_SCHEDULER_add_with_priority(
    GNUNET_SCHEDULER_PRIORITY_BACKGROUND,
    cb_context_request_messages,
    context
  );
}

void
context_update_message (struct GNUNET_CHAT_Context* context,
                        const struct GNUNET_HashCode *hash)
{
  GNUNET_assert((context) && (hash));

  struct GNUNET_CHAT_Message *message = GNUNET_CONTAINER_multihashmap_get(
    context->messages, hash);
  
  if (!message)
    return;

  message->flags |= GNUNET_MESSENGER_FLAG_UPDATE;

  struct GNUNET_CHAT_Handle *handle = context->handle;

  if (!(handle->msg_cb))
    return;

  handle->msg_cb(handle->msg_cls, context, message);
}

void
context_update_room (struct GNUNET_CHAT_Context *context,
                     struct GNUNET_MESSENGER_Room *room,
                     enum GNUNET_GenericReturnValue record)
{
  GNUNET_assert(context);

  if (room == context->room)
    return;

  GNUNET_assert(
    (context->timestamps) &&
    (context->messages) &&
    (context->requests) &&
    (context->invites) &&
    (context->discourses)
  );

  GNUNET_CONTAINER_multishortmap_iterate(
    context->timestamps, it_destroy_context_timestamps, NULL
  );

  GNUNET_CONTAINER_multihashmap_iterate(
    context->messages, it_destroy_context_messages, NULL
  );

  GNUNET_CONTAINER_multihashmap_iterate(
    context->invites, it_destroy_context_invites, context
  );

  GNUNET_CONTAINER_multishortmap_iterate(
    context->discourses, it_destroy_context_discourses, NULL
  );

  GNUNET_CONTAINER_multishortmap_destroy(context->timestamps);
  context->timestamps = GNUNET_CONTAINER_multishortmap_create(
    initial_map_size_of_room, GNUNET_NO);

  GNUNET_CONTAINER_multihashmap_clear(context->messages);
  GNUNET_CONTAINER_multihashmap_clear(context->requests);
  GNUNET_CONTAINER_multihashmap_clear(context->invites);
  GNUNET_CONTAINER_multihashmap_clear(context->files);

  GNUNET_CONTAINER_multishortmap_destroy(context->discourses);
  context->discourses = GNUNET_CONTAINER_multishortmap_create(
    initial_map_size_of_room, GNUNET_NO);

  if (context->room)
    context_delete(context, GNUNET_YES);

  context->room = room;

  if ((!(context->room)) || (GNUNET_YES != record))
    return;

  context_write_records(context);
}

void
context_update_nick (struct GNUNET_CHAT_Context *context,
                     const char *nick)
{
  GNUNET_assert(context);

  if (context->nick)
    GNUNET_free(context->nick);

  if (nick)
    context->nick = GNUNET_strdup(nick);
  else
    context->nick = NULL;

  if ((!(context->handle)) ||
      (GNUNET_YES == context->deleted))
    return;

  handle_send_internal_message(
    context->handle,
    NULL,
    context,
    GNUNET_CHAT_FLAG_UPDATE_CONTEXT,
    NULL,
    GNUNET_NO
  );
}

void
context_read_records (struct GNUNET_CHAT_Context *context,
                      const char *label,
                      unsigned int count,
                      const struct GNUNET_GNSRECORD_Data *data)
{
  GNUNET_assert((context) && (context->room));

  char *nick = NULL;
  char *topic = NULL;
  uint32_t flags = 0;

  for (unsigned int i = 0; i < count; i++)
  {
    if (!(GNUNET_GNSRECORD_RF_SUPPLEMENTAL & data[i].flags))
      continue;

    if (GNUNET_GNSRECORD_TYPE_MESSENGER_ROOM_DETAILS == data[i].record_type)
    {
      if (nick)
	      continue;

      const struct GNUNET_MESSENGER_RoomDetailsRecord *record = data[i].data;

      nick = GNUNET_strndup(record->name, sizeof(record->name));
      flags = record->flags;
    }

    if (GNUNET_DNSPARSER_TYPE_TXT == data[i].record_type)
    {
      if (topic)
	      continue;

      topic = GNUNET_strndup(data[i].data, data[i].data_size);
    }
  }

  context->flags = flags;
  context_update_nick(context, nick);

  if (nick)
    GNUNET_free(nick);

  const struct GNUNET_HashCode *hash = GNUNET_MESSENGER_room_get_key(
    context->room
  );

  if (topic)
  {
    struct GNUNET_HashCode topic_hash;
    GNUNET_CRYPTO_hash(topic, strlen(topic), &topic_hash);

    if (0 != GNUNET_CRYPTO_hash_cmp(&topic_hash, hash))
    {
      GNUNET_free(topic);
      topic = NULL;
    }
  }

  util_set_name_field(topic, &(context->topic));

  if (topic)
    GNUNET_free(topic);

  context->type = util_get_context_label_type(label, hash);
}

void
context_delete_message (struct GNUNET_CHAT_Context *context,
                        const struct GNUNET_CHAT_Message *message)
{
  GNUNET_assert((context) && (message));

  if (GNUNET_YES != message_has_msg(message))
    return;

  struct GNUNET_CHAT_Handle *handle = context->handle;

  switch (message->msg->header.kind)
  {
    case GNUNET_MESSENGER_KIND_INVITE:
    {
      struct GNUNET_CHAT_Invitation *invite = GNUNET_CONTAINER_multihashmap_get(
        context->invites, &(message->hash)
      );

      if (! invite)
        break;

      GNUNET_CONTAINER_multihashmap_remove(
        handle->invitations, &(invite->key.hash), invite);

      if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_remove(
        context->invites, &(message->hash), invite))
        invitation_destroy(invite);
      
      break;
    }
    case GNUNET_MESSENGER_KIND_FILE:
    {
      if (GNUNET_YES != GNUNET_CONTAINER_multihashmap_contains(context->files, &(message->hash)))
        break;

      GNUNET_CONTAINER_multihashmap_remove_all(context->files, &(message->hash));
      break;
    }
    case GNUNET_MESSENGER_KIND_TAG:
    {
      struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
        context->taggings,
        &(message->msg->body.tag.hash)
      );

      if (!tagging)
        break;

      internal_tagging_remove(tagging, message);
      break;
    }
    default:
      break;
  }
}

void
context_write_records (struct GNUNET_CHAT_Context *context)
{
  GNUNET_assert((context) && (context->handle) && (context->room));

  const struct GNUNET_CRYPTO_BlindablePrivateKey *zone = handle_get_key(
    context->handle
  );

  const struct GNUNET_PeerIdentity *pid = context->handle->pid;

  if ((!zone) || (!pid))
    return;

  const struct GNUNET_HashCode *hash = GNUNET_MESSENGER_room_get_key(
    context->room
  );

  struct GNUNET_TIME_Absolute expiration = GNUNET_TIME_absolute_get_forever_();

  struct GNUNET_MESSENGER_RoomEntryRecord room_entry;
  GNUNET_memcpy(&(room_entry.door), pid, sizeof(*pid));

  GNUNET_memcpy(
    &(room_entry.key),
    hash,
    sizeof(room_entry.key)
  );

  struct GNUNET_MESSENGER_RoomDetailsRecord room_details;
  memset(room_details.name, 0, sizeof(room_details.name));
  
  const char *topic = context->topic;

  if (topic)
  {
    struct GNUNET_HashCode topic_hash;
    GNUNET_CRYPTO_hash(topic, strlen(topic), &topic_hash);

    if (0 != GNUNET_CRYPTO_hash_cmp(&topic_hash, hash))
      topic = NULL;
  }

  char *label;
  util_get_context_label(context->type, hash, &label);

  unsigned int count = 0;
  struct GNUNET_GNSRECORD_Data data [3];

  if (GNUNET_YES == context->deleted)
    goto skip_record_data;

  data[count].record_type = GNUNET_GNSRECORD_TYPE_MESSENGER_ROOM_ENTRY;
  data[count].data = &room_entry;
  data[count].data_size = sizeof(room_entry);
  data[count].expiration_time = expiration.abs_value_us;
  data[count].flags = GNUNET_GNSRECORD_RF_PRIVATE;
  count++;

  if (context->nick)
  {
    size_t name_len = strlen(context->nick);
    if (name_len >= sizeof(room_details.name))
      name_len = sizeof(room_details.name) - 1;

    GNUNET_memcpy(room_details.name, context->nick, name_len);
    room_details.name[name_len] = '\0';
  }

  if ((context->nick) || (context->flags != 0))
  {
    room_details.flags = context->flags;

    data[count].record_type = GNUNET_GNSRECORD_TYPE_MESSENGER_ROOM_DETAILS;
    data[count].data = &room_details;
    data[count].data_size = sizeof(room_details);
    data[count].expiration_time = expiration.abs_value_us;
    data[count].flags = (
      GNUNET_GNSRECORD_RF_PRIVATE |
      GNUNET_GNSRECORD_RF_SUPPLEMENTAL
    );

    count++;
  }

  if (topic)
  {
    data[count].record_type = GNUNET_DNSPARSER_TYPE_TXT;
    data[count].data = topic;
    data[count].data_size = strlen(topic);
    data[count].expiration_time = expiration.abs_value_us;
    data[count].flags = (
      GNUNET_GNSRECORD_RF_PRIVATE |
      GNUNET_GNSRECORD_RF_SUPPLEMENTAL
    );

    count++;
  }

skip_record_data:
  if (context->query)
    GNUNET_NAMESTORE_cancel(context->query);

  context->query = GNUNET_NAMESTORE_record_set_store(
    context->handle->namestore,
    zone,
    label,
    count,
    data,
    cont_context_write_records,
    context
  );

  GNUNET_free(label);
}

void
context_delete (struct GNUNET_CHAT_Context *context,
                enum GNUNET_GenericReturnValue exit)
{
  GNUNET_assert((context) && (context->room));

  context->deleted = GNUNET_YES;
  context_write_records(context);

  if (GNUNET_YES != exit)
    return;

  if (context->request_task)
  {
    GNUNET_SCHEDULER_cancel(context->request_task);
    context->request_task = NULL;
  }

  GNUNET_MESSENGER_close_room(context->room);
}
