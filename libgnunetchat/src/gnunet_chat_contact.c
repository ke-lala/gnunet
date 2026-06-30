/*
   This file is part of GNUnet.
   Copyright (C) 2021--2025 GNUnet e.V.

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
 * @file gnunet_chat_contact.c
 */

#include "gnunet_chat_contact.h"
#include "gnunet_chat_context.h"
#include "gnunet_chat_handle.h"
#include "gnunet_chat_ticket.h"

#include "internal/gnunet_chat_tagging.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_time_lib.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_contact_intern.c"

static const unsigned int initial_map_size_of_contact = 8;

struct GNUNET_CHAT_Contact*
contact_create_from_member (struct GNUNET_CHAT_Handle *handle,
			                      const struct GNUNET_MESSENGER_Contact *member)
{
  GNUNET_assert((handle) && (member));

  struct GNUNET_CHAT_Contact* contact = GNUNET_new(struct GNUNET_CHAT_Contact);

  contact->handle = handle;
  contact->context = NULL;

  contact->destruction = NULL;

  contact->member = member;
  contact->joined = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size_of_contact, GNUNET_NO);

  contact->tickets_head = NULL;
  contact->tickets_tail = NULL;

  contact->public_key = NULL;
  contact->user_pointer = NULL;

  contact->owned = GNUNET_NO;

  contact_update_key (contact);
  return contact;
}

void
contact_update_join (struct GNUNET_CHAT_Contact *contact,
                     struct GNUNET_CHAT_Context *context,
                     const struct GNUNET_HashCode *hash,
                     enum GNUNET_MESSENGER_MessageFlags flags)
{
  GNUNET_assert(
    (contact) &&
    (contact->joined) &&
    (context) &&
    (hash)
  );

  if (!(context->room))
    return;

  const enum GNUNET_GenericReturnValue blocked = contact_is_tagged(
    contact, context, NULL
  );

  const struct GNUNET_HashCode *key = GNUNET_MESSENGER_room_get_key(
    context->room
  );

  struct GNUNET_HashCode *current = GNUNET_CONTAINER_multihashmap_get(
    contact->joined,
    key
  );

  if (! current)
  {
    current = GNUNET_new(struct GNUNET_HashCode);

    if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      contact->joined, key, current, GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    {
      GNUNET_free(current);
      return;
    }

    GNUNET_memcpy(current, hash, 
      sizeof(struct GNUNET_HashCode));
    return;
  }
  else if (0 == (flags & GNUNET_MESSENGER_FLAG_RECENT))
    return;

  if (GNUNET_YES == blocked)
    contact_untag(contact, context, NULL);

  GNUNET_memcpy(current, hash, 
    sizeof(struct GNUNET_HashCode));
  
  if (GNUNET_YES == blocked)
    contact_tag(contact, context, NULL);
}

void
contact_leave (struct GNUNET_CHAT_Contact *contact,
               struct GNUNET_CHAT_Context *context)
{
  GNUNET_assert(
    (contact) &&
    (contact->joined) &&
    (context)
  );

  if (!(context->room))
    return;

  const struct GNUNET_HashCode *key = GNUNET_MESSENGER_room_get_key(
    context->room
  );

  struct GNUNET_HashCode *current = GNUNET_CONTAINER_multihashmap_get(
    contact->joined,
    key
  );

  if ((! current) ||
      (GNUNET_YES != GNUNET_CONTAINER_multihashmap_remove(contact->joined, key, current)))
    return;

  GNUNET_free(current);
}

void
contact_update_key (struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_assert(contact);

  if (contact->public_key)
    GNUNET_free(contact->public_key);

  const struct GNUNET_CRYPTO_BlindablePublicKey *pubkey;
  pubkey = contact_get_key(contact);

  if (pubkey)
    contact->public_key = GNUNET_CRYPTO_blindable_public_key_to_string(pubkey);
  else
    contact->public_key = NULL;
}

const struct GNUNET_CRYPTO_BlindablePublicKey*
contact_get_key (const struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_assert(contact);

  if (!(contact->member))
    return NULL;

  return GNUNET_MESSENGER_contact_get_key(contact->member);
}

struct GNUNET_CHAT_Context*
contact_find_context (const struct GNUNET_CHAT_Contact *contact,
                      enum GNUNET_GenericReturnValue room_required)
{
  GNUNET_assert(contact);

  if ((contact->context) &&
      ((GNUNET_YES != room_required) || (contact->context->room)))
    return contact->context;

  struct GNUNET_CHAT_ContactFindRoom find;
  find.member_count = 0;
  find.room = NULL;

  GNUNET_MESSENGER_find_rooms(
    contact->handle->messenger,
    contact->member,
    it_contact_find_room,
    &find
  );

  if (!(find.room))
    return NULL;

  struct GNUNET_CHAT_Context *context = GNUNET_CONTAINER_multihashmap_get(
    contact->handle->contexts,
    GNUNET_MESSENGER_room_get_key(find.room)
  );

  if ((GNUNET_YES == room_required) && (!(context->room)))
    return NULL;

  return context;
}

const struct GNUNET_HashCode*
get_contact_join_hash (const struct GNUNET_CHAT_Contact *contact,
                       const struct GNUNET_CHAT_Context *context)
{
  GNUNET_assert((contact) && (context));

  if (!(context->room))
    return NULL;

  return GNUNET_CONTAINER_multihashmap_get(
    contact->joined,
    GNUNET_MESSENGER_room_get_key(context->room)
  );
}

enum GNUNET_GenericReturnValue
contact_is_tagged (const struct GNUNET_CHAT_Contact *contact,
                   const struct GNUNET_CHAT_Context *context,
                   const char *tag)
{
  GNUNET_assert(
    (contact) &&
    (contact->joined)
  );

  const enum GNUNET_GenericReturnValue general = (
    context ? GNUNET_NO : GNUNET_YES
  );

  if (context)
    goto skip_context_search;

  struct GNUNET_CONTAINER_MultiHashMapIterator *iter;
  iter = GNUNET_CONTAINER_multihashmap_iterator_create(
    contact->joined
  );

  if (iter)
  {
    struct GNUNET_HashCode key;
    const void *value;

    while (! context)
    {
      if (GNUNET_YES != GNUNET_CONTAINER_multihashmap_iterator_next(
          iter, &key, &value))
        break;

      context = GNUNET_CONTAINER_multihashmap_get(
        contact->handle->contexts, &key);
    }

    GNUNET_CONTAINER_multihashmap_iterator_destroy(iter);
  }

skip_context_search:
  if (! context)
    return GNUNET_NO;

  const struct GNUNET_HashCode *hash = get_contact_join_hash(
    contact, context);

  if (! hash)
    return (general == GNUNET_YES? 
      GNUNET_NO : 
      contact_is_tagged(contact, NULL, tag)
    );
  
  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    context->taggings,
    hash
  );

  if (! tagging)
    return GNUNET_NO;

  struct GNUNET_CHAT_ContactFindTag find;
  find.hash = NULL;

  internal_tagging_iterate(
    tagging,
    GNUNET_NO,
    tag,
    it_contact_find_tag,
    &find
  );

  if (find.hash)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}

void
contact_untag (struct GNUNET_CHAT_Contact *contact,
               struct GNUNET_CHAT_Context *context,
               const char *tag)
{
  GNUNET_assert(
    (contact) &&
    (contact->joined) &&
    (context)
  );

  const struct GNUNET_HashCode *hash = get_contact_join_hash(
    contact, context);

  if (! hash)
    return;

  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    context->taggings,
    hash
  );

  if (! tagging)
    return;

  struct GNUNET_CHAT_ContactFindTag find;
  find.hash = NULL;

  internal_tagging_iterate(
    tagging,
    GNUNET_NO,
    tag,
    it_contact_find_tag,
    &find
  );

  if ((! find.hash) || (! context->room))
    return;

  GNUNET_MESSENGER_delete_message(
    context->room,
    find.hash,
    GNUNET_TIME_relative_get_zero_()
  );
}

void
contact_tag (struct GNUNET_CHAT_Contact *contact,
             struct GNUNET_CHAT_Context *context,
             const char *tag)
{
  GNUNET_assert(
    (contact) &&
    (contact->joined) &&
    (context)
  );

  const struct GNUNET_HashCode *hash = get_contact_join_hash(
    contact, context);

  if (! hash)
    return;

  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    context->taggings,
    hash
  );

  if (! tagging)
    goto skip_tag_search;

  struct GNUNET_CHAT_ContactFindTag find;
  find.hash = NULL;

  internal_tagging_iterate(
    tagging,
    GNUNET_NO,
    tag,
    it_contact_find_tag,
    &find
  );

  if (find.hash)
    return;

skip_tag_search:
  if (! context->room)
    return;

  char *tag_value = tag? GNUNET_strdup(tag) : NULL;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TAG;
  GNUNET_memcpy(&(msg.body.tag.hash), hash,
    sizeof(struct GNUNET_HashCode));
  msg.body.tag.tag = tag_value;

  GNUNET_MESSENGER_send_message(
    context->room,
    &msg,
    contact->member
  );

  if (tag_value)
    GNUNET_free(tag_value);
}

int
contact_iterate_tags (struct GNUNET_CHAT_Contact *contact,
                      struct GNUNET_CHAT_Context *context,
                      GNUNET_CHAT_ContactTagCallback callback,
                      void *cls)
{
  GNUNET_assert((contact) && (contact->joined));

  if (! context)
  {
    struct GNUNET_CHAT_ContactIterateUniqueTag it;
    it.tags = GNUNET_CONTAINER_multihashmap_create(
      initial_map_size_of_contact, GNUNET_NO);
    it.callback = callback;
    it.cls = cls;

    if (! (it.tags))
      return GNUNET_SYSERR;

    int result = GNUNET_SYSERR;

    struct GNUNET_CONTAINER_MultiHashMapIterator *iter;
    iter = GNUNET_CONTAINER_multihashmap_iterator_create(
      contact->joined
    );

    if (! iter)
      goto free_tags_iteration;

    struct GNUNET_HashCode key;
    const void *value;

    while (! context)
    {
      if (GNUNET_YES != GNUNET_CONTAINER_multihashmap_iterator_next(
          iter, &key, &value))
        break;

      context = GNUNET_CONTAINER_multihashmap_get(
        contact->handle->contexts, &key);
      
      if (context)
        result = contact_iterate_tags(
          contact,
          context,
          it_contact_iterate_unique_tag,
          &it
        );
    }

    GNUNET_CONTAINER_multihashmap_iterator_destroy(iter);

free_tags_iteration:
    GNUNET_CONTAINER_multihashmap_destroy(it.tags);
    return result;
  }

  const struct GNUNET_HashCode *hash = get_contact_join_hash(
    contact, context);

  if (! hash)
    return GNUNET_SYSERR;

  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    context->taggings,
    hash
  );

  if (! tagging)
    return 0;

  struct GNUNET_CHAT_ContactIterateTag it;
  it.contact = contact;
  it.callback = callback;
  it.cls = cls;

  return internal_tagging_iterate(
    tagging,
    GNUNET_YES,
    NULL,
    it_contact_iterate_tag,
    &it
  );
}

void
contact_destroy (struct GNUNET_CHAT_Contact* contact)
{
  GNUNET_assert(contact);

  if (contact->destruction)
    GNUNET_SCHEDULER_cancel(contact->destruction);

  struct GNUNET_CHAT_InternalTickets *tickets;
  while (contact->tickets_head)
  {
    tickets = contact->tickets_head;

    GNUNET_CONTAINER_DLL_remove(
      contact->tickets_head,
      contact->tickets_tail,
      tickets
    );

    ticket_destroy(tickets->ticket);

    GNUNET_free(tickets);
  }

  if (contact->public_key)
    GNUNET_free(contact->public_key);

  if (contact->joined)
  {
    GNUNET_CONTAINER_multihashmap_iterate(
      contact->joined, it_free_join_hashes, NULL
    );

    GNUNET_CONTAINER_multihashmap_destroy(contact->joined);
  }

  if ((contact->context) && (!(contact->context->room)))
    context_destroy(contact->context);

  GNUNET_free(contact);
}
