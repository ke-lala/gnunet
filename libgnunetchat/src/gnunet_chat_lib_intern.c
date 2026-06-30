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
 * @file gnunet_chat_lib_intern.c
 */

#include "gnunet_chat_account.h"
#include "gnunet_chat_contact.h"
#include "gnunet_chat_handle.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_reclaim_lib.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <gnunet/gnunet_time_lib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GNUNET_UNUSED __attribute__ ((unused))

void
task_handle_destruction (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Handle *handle = (struct GNUNET_CHAT_Handle*) cls;

  struct GNUNET_CHAT_InternalAccounts *accounts = handle->accounts_head;
  while (accounts)
  {
    if ((accounts->op) && (GNUNET_CHAT_ACCOUNT_NONE != accounts->method))
      break;

    accounts = accounts->next;
  }

  if (accounts)
  {
    handle->destruction = GNUNET_SCHEDULER_add_delayed_with_priority(
      GNUNET_TIME_relative_get_millisecond_(),
      GNUNET_SCHEDULER_PRIORITY_BACKGROUND,
      task_handle_destruction,
      handle
    );

    return;
  }

  handle->destruction = NULL;
  handle_destroy(handle);
}

void
task_handle_connection (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Handle *handle = (struct GNUNET_CHAT_Handle*) cls;

  handle->connection = NULL;

  if (! handle->next)
    return;

  struct GNUNET_CHAT_Account *account = handle->next;
  struct GNUNET_HashCode local_secret;
  const struct GNUNET_HashCode *secret;

  if (handle->next_secret)
  {
    GNUNET_memcpy(
      &local_secret,
      handle->next_secret,
      sizeof(local_secret)
    );

    secret = &local_secret;

    GNUNET_CRYPTO_zero_keys(
      handle->next_secret,
      sizeof(*(handle->next_secret))
    );

    GNUNET_free(handle->next_secret);
    handle->next_secret = NULL;
  }
  else
    secret = NULL;

  handle->next = NULL;
  handle_connect(handle, account, secret);

  GNUNET_CRYPTO_zero_keys(
    &local_secret,
    sizeof(local_secret)
  );
}

void
task_handle_disconnection (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Handle *handle = (struct GNUNET_CHAT_Handle*) cls;

  handle->connection = NULL;
  handle_disconnect(handle);

  if (! handle->next)
    return;

  task_handle_connection(cls);
}

void
cb_lobby_lookup (void *cls,
                 uint32_t count,
                 const struct GNUNET_GNSRECORD_Data *data)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_UriLookups *lookups = (struct GNUNET_CHAT_UriLookups*) cls;

  if ((!(lookups->handle)) || (!(lookups->uri)) ||
      (GNUNET_CHAT_URI_TYPE_CHAT != lookups->uri->type))
    goto drop_lookup;

  struct GNUNET_CHAT_Context *context = handle_process_records(
    lookups->handle,
    lookups->uri->chat.label,
    count,
    data
  );

  if (context)
    context_write_records(context);

drop_lookup:
  if (lookups->uri)
    uri_destroy(lookups->uri);

  if (lookups->handle)
    GNUNET_CONTAINER_DLL_remove(
      lookups->handle->lookups_head,
      lookups->handle->lookups_tail,
      lookups
    );

  GNUNET_free(lookups);
}

struct GNUNET_CHAT_IterateFiles
{
  struct GNUNET_CHAT_Handle *handle;
  GNUNET_CHAT_FileCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_iterate_files (void *cls,
                  GNUNET_UNUSED const struct GNUNET_HashCode *key,
                  void *value)
{
  GNUNET_assert((cls) && (key));

  struct GNUNET_CHAT_IterateFiles *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  struct GNUNET_CHAT_File *file = (struct GNUNET_CHAT_File*) value;

  if (!file)
    return GNUNET_YES;

  return it->cb(it->cls, it->handle, file);
}

struct GNUNET_CHAT_HandleIterateContacts
{
  struct GNUNET_CHAT_Handle *handle;
  GNUNET_CHAT_ContactCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_handle_iterate_contacts (void *cls,
                            GNUNET_UNUSED const struct GNUNET_ShortHashCode *key,
                            void *value)
{
  GNUNET_assert((cls) && (value));

  struct GNUNET_CHAT_HandleIterateContacts *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  struct GNUNET_CHAT_Contact *contact = value;

  return it->cb(it->cls, it->handle, contact);
}

enum GNUNET_GenericReturnValue
it_handle_find_own_contact (GNUNET_UNUSED void *cls,
                            struct GNUNET_CHAT_Handle *handle,
                            struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_assert((handle) && (contact));

  if (GNUNET_YES != GNUNET_CHAT_contact_is_owned(contact))
    return GNUNET_YES;

  const char *contact_key = GNUNET_CHAT_contact_get_key(contact);
  const char *handle_key = GNUNET_CHAT_get_key(handle);

  if ((!contact_key) || (!handle_key) ||
      (0 != strcmp(contact_key, handle_key)))
    return GNUNET_YES;

  handle->own_contact = contact;
  return GNUNET_NO;
}

struct GNUNET_CHAT_HandleIterateGroups
{
  struct GNUNET_CHAT_Handle *handle;
  GNUNET_CHAT_GroupCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_handle_iterate_groups (void *cls,
                          GNUNET_UNUSED const struct GNUNET_HashCode *key,
                          void *value)
{
  GNUNET_assert((cls) && (value));

  struct GNUNET_CHAT_HandleIterateGroups *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  struct GNUNET_CHAT_Group *group = value;

  return it->cb(it->cls, it->handle, group);
}

typedef void
(*GNUNET_CHAT_ContactIterateContextCallback) (struct GNUNET_CHAT_Contact *contact,
                                              struct GNUNET_CHAT_Context *context,
                                              const char *tag);

struct GNUNET_CHAT_ContactIterateContexts
{
  struct GNUNET_CHAT_Contact *contact;
  const char *tag;

  GNUNET_CHAT_ContactIterateContextCallback cb;
};

enum GNUNET_GenericReturnValue
it_contact_iterate_contexts (void *cls,
                             const struct GNUNET_HashCode *key,
                             GNUNET_UNUSED void *value)
{
  GNUNET_assert((cls) && (key));

  struct GNUNET_CHAT_ContactIterateContexts *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  struct GNUNET_CHAT_Handle *handle = it->contact->handle;
  struct GNUNET_CHAT_Context *context = GNUNET_CONTAINER_multihashmap_get(
    handle->contexts, key);
  
  if (! context)
    return GNUNET_YES;

  it->cb(it->contact, context, it->tag);
  return GNUNET_YES;
}

struct GNUNET_CHAT_RoomFindContact
{
  const struct GNUNET_CRYPTO_BlindablePublicKey *ignore_key;
  const struct GNUNET_MESSENGER_Contact *contact;
};

enum GNUNET_GenericReturnValue
it_room_find_contact (void *cls,
                      GNUNET_UNUSED struct GNUNET_MESSENGER_Room *room,
                      const struct GNUNET_MESSENGER_Contact *member)
{
  GNUNET_assert((cls) && (member));

  const struct GNUNET_CRYPTO_BlindablePublicKey *key = GNUNET_MESSENGER_contact_get_key(
      member
  );

  struct GNUNET_CHAT_RoomFindContact *find = cls;

  if ((find->ignore_key) && (key) &&
      (0 == GNUNET_memcmp(find->ignore_key, key)))
    return GNUNET_YES;

  find->contact = member;
  return GNUNET_NO;
}

void
task_lobby_destruction (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Lobby *lobby = (struct GNUNET_CHAT_Lobby*) cls;
  struct GNUNET_CHAT_InternalLobbies *lobbies = lobby->handle->lobbies_head;

  while (lobbies)
  {
    if (lobbies->lobby == lobby)
    {
      GNUNET_CONTAINER_DLL_remove(
        lobby->handle->lobbies_head,
        lobby->handle->lobbies_tail,
        lobbies
      );

      GNUNET_free(lobbies);
      break;
    }

    lobbies = lobbies->next;
  }

  lobby->destruction = NULL;

  lobby_destroy(lobby);
}

void
task_contact_destruction (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Contact *contact = (struct GNUNET_CHAT_Contact*) cls;
  struct GNUNET_ShortHashCode shorthash;

  util_shorthash_from_member(contact->member, &shorthash);

  contact_leave (contact, contact->context);

  const uint32_t other_contexts = GNUNET_CONTAINER_multihashmap_size(
    contact->joined
  );

  if (0 >= other_contexts)
    GNUNET_CONTAINER_multishortmap_remove(
      contact->handle->contacts, &shorthash, contact
    );

  context_delete(contact->context, GNUNET_YES);

  contact->destruction = NULL;

  if (0 >= other_contexts)
    contact_destroy(contact);
}

void
task_group_destruction (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Group *group = (struct GNUNET_CHAT_Group*) cls;
  struct GNUNET_HashCode key;

  GNUNET_memcpy(&key, GNUNET_MESSENGER_room_get_key(
    group->context->room
  ), sizeof(key));

  GNUNET_CONTAINER_multihashmap_remove(
    group->handle->groups, &key, group
  );

  context_delete(group->context, GNUNET_YES);

  group->destruction = NULL;

  group_destroy(group);
}

struct GNUNET_CHAT_GroupIterateContacts
{
  struct GNUNET_CHAT_Group *group;
  GNUNET_CHAT_GroupContactCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_group_iterate_contacts (void* cls,
			                     GNUNET_UNUSED struct GNUNET_MESSENGER_Room *room,
                           const struct GNUNET_MESSENGER_Contact *member)
{
  GNUNET_assert((cls) && (member));

  struct GNUNET_CHAT_GroupIterateContacts *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  return it->cb(it->cls, it->group, handle_get_contact_from_messenger(
    it->group->handle, member
  ));
}

struct GNUNET_CHAT_ContextIterateMessages
{
  struct GNUNET_CHAT_Context *context;
  GNUNET_CHAT_ContextMessageCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_context_iterate_messages (void *cls,
                             GNUNET_UNUSED const struct GNUNET_HashCode *key,
                             void *value)
{
  GNUNET_assert((cls) && (value));

  struct GNUNET_CHAT_ContextIterateMessages *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  struct GNUNET_CHAT_Message *message = value;

  return it->cb(it->cls, it->context, message);
}

struct GNUNET_CHAT_ContextIterateFiles
{
  struct GNUNET_CHAT_Context *context;
  GNUNET_CHAT_ContextFileCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_context_iterate_files (void *cls,
                          const struct GNUNET_HashCode *key,
                          GNUNET_UNUSED void *value)
{
  GNUNET_assert((cls) && (key));

  struct GNUNET_CHAT_ContextIterateFiles *it = cls;

  if (!(it->cb))
    return GNUNET_YES;

  struct GNUNET_CHAT_Message *message = GNUNET_CONTAINER_multihashmap_get(
    it->context->messages, key
  );

  if ((!message) || (! message->msg))
    return GNUNET_YES;

  struct GNUNET_CHAT_File *file = GNUNET_CONTAINER_multihashmap_get(
    it->context->handle->files, &(message->msg->body.file.hash)
  );

  if (!file)
    return GNUNET_YES;

  return it->cb(it->cls, it->context, file);
}

struct GNUNET_CHAT_ContextIterateDiscourses
{
  struct GNUNET_CHAT_Context *context;
  GNUNET_CHAT_DiscourseCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_context_iterate_discourses (void *cls,
                               GNUNET_UNUSED const struct GNUNET_ShortHashCode *key,
                               void *value)
{
  GNUNET_assert((cls) && (value));

  struct GNUNET_CHAT_ContextIterateDiscourses *it = cls;
  struct GNUNET_CHAT_Discourse *discourse = value;

  if (!(it->cb))
    return GNUNET_YES;

  return it->cb(it->cls, it->context, discourse);
}

struct GNUNET_CHAT_MessageIterateReadReceipts
{
  struct GNUNET_CHAT_Message *message;
  GNUNET_CHAT_MessageReadReceiptCallback cb;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_message_iterate_read_receipts (void *cls,
                                  GNUNET_UNUSED struct GNUNET_MESSENGER_Room *room,
                                  const struct GNUNET_MESSENGER_Contact *member)
{
  GNUNET_assert((cls) && (member));

  struct GNUNET_CHAT_MessageIterateReadReceipts *it = cls;
  struct GNUNET_CHAT_Handle *handle = it->message->context->handle;

  if (!handle)
    return GNUNET_NO;

  struct GNUNET_ShortHashCode shorthash;
  util_shorthash_from_member(member, &shorthash);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CONTAINER_multishortmap_get(
      handle->contacts, &shorthash
  );

  if (!contact)
    return GNUNET_YES;

  struct GNUNET_TIME_Absolute *timestamp = GNUNET_CONTAINER_multishortmap_get(
    it->message->context->timestamps, &shorthash
  );

  if (!timestamp)
    return GNUNET_YES;

  struct GNUNET_TIME_Absolute abs = GNUNET_TIME_absolute_ntoh(
    it->message->msg->header.timestamp
  );

  struct GNUNET_TIME_Relative delta = GNUNET_TIME_absolute_get_difference(
    *timestamp, abs
  );

  int read_receipt;
  if (GNUNET_TIME_relative_get_zero_().rel_value_us == delta.rel_value_us)
    read_receipt = GNUNET_YES;
  else
    read_receipt = GNUNET_NO;

  if (it->cb)
    it->cb(it->cls, it->message, contact, read_receipt);

  return GNUNET_YES;
}

void
cont_update_attribute_with_status (void *cls,
                                   int32_t success,
                                   const char *emsg)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  attributes->op = NULL;

  struct GNUNET_CHAT_Account *account = attributes->account;
  struct GNUNET_CHAT_Handle *handle = attributes->handle;

  const char *attribute_name = NULL;

  if (attributes->attribute)
    attribute_name = attributes->attribute->name;

  if (GNUNET_SYSERR == success)
    handle_send_internal_message(
      handle,
      account,
      NULL,
      GNUNET_CHAT_KIND_WARNING,
      emsg,
      GNUNET_YES
    );
  else
    handle_send_internal_message(
      handle,
      account,
      NULL,
      GNUNET_CHAT_FLAG_ATTRIBUTES,
      attribute_name,
      GNUNET_YES
    );
  
  internal_attributes_destroy(attributes);
}

void
cb_task_finish_iterate_attribute (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  attributes->iter = NULL;

  struct GNUNET_CHAT_Handle *handle = attributes->handle;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key;

  if (attributes->account)
    key = account_get_key(attributes->account);
  else
    key = handle_get_key(handle);

  if (attributes->name)
    GNUNET_free(attributes->name);

  attributes->name = NULL;

  if ((! attributes->op) && (key) &&
      (attributes->attribute))
    attributes->op = GNUNET_RECLAIM_attribute_store(
      handle->reclaim,
      key,
      attributes->attribute,
      &(attributes->expires),
      cont_update_attribute_with_status,
      attributes
    );
  
  if (attributes->data)
    GNUNET_free(attributes->data);

  attributes->data = NULL;

  if (attributes->op)
    return;

  internal_attributes_destroy(attributes);
}

void
cb_task_error_iterate_attribute (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  handle_send_internal_message(
    attributes->handle,
    attributes->account,
    NULL,
    GNUNET_CHAT_FLAG_WARNING,
    "Attribute iteration failed!",
    GNUNET_YES
  );

  cb_task_finish_iterate_attribute(cls);
}

void
cb_store_attribute (void *cls,
                    const struct GNUNET_CRYPTO_BlindablePublicKey *identity,
                    const struct GNUNET_RECLAIM_Attribute *attribute)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  struct GNUNET_CHAT_Handle *handle = attributes->handle;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (! attributes->name)
  {
    internal_attributes_stop_iter(attributes);
    return;
  }

  if (0 == strcmp(attribute->name, attributes->name))
  {
    internal_attributes_stop_iter(attributes);

    if (attributes->attribute)
    {
      attributes->attribute->credential = attribute->credential;
      attributes->attribute->flag = attribute->flag;
      attributes->attribute->id = attribute->id;
    }

    attributes->op = GNUNET_RECLAIM_attribute_store(
      handle->reclaim,
      key,
      attributes->attribute,
      &(attributes->expires),
      cont_update_attribute_with_status,
      attributes
    );

    if (attributes->data)
      GNUNET_free(attributes->data);

    attributes->data = NULL;

    GNUNET_free(attributes->name);
    attributes->name = NULL;
    return;
  }

  internal_attributes_next_iter(attributes);
}

void
cb_delete_attribute (void *cls,
                     const struct GNUNET_CRYPTO_BlindablePublicKey *identity,
                     const struct GNUNET_RECLAIM_Attribute *attribute)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  if (! attributes->name)
  {
    internal_attributes_stop_iter(attributes);
    return;
  }

  struct GNUNET_CHAT_Handle *handle = attributes->handle;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (0 == strcmp(attribute->name, attributes->name))
  {
    internal_attributes_stop_iter(attributes);

    attributes->op = GNUNET_RECLAIM_attribute_delete(
      handle->reclaim,
      key,
      attribute,
      cont_update_attribute_with_status,
      attributes
    );

    GNUNET_free(attributes->name);
    attributes->name = NULL;
    return;
  }

  internal_attributes_next_iter(attributes);
}

void
cb_iterate_attribute (void *cls,
                      const struct GNUNET_CRYPTO_BlindablePublicKey *identity,
                      const struct GNUNET_RECLAIM_Attribute *attribute)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  struct GNUNET_CHAT_Handle *handle = attributes->handle;
  enum GNUNET_GenericReturnValue result = GNUNET_YES;

  char *value = GNUNET_RECLAIM_attribute_value_to_string(
    attribute->type,
    attribute->data,
    attribute->data_size
  );

  if (attributes->callback)
    result = attributes->callback(attributes->closure, handle, attribute->name, value);
  else if (attributes->account_callback)
    result = attributes->account_callback(
      attributes->closure,
      attributes->account,
      attribute->name,
      value
    );

  if (value)
    GNUNET_free (value);
  
  if (GNUNET_YES != result)
    internal_attributes_stop_iter(attributes);
  else
    internal_attributes_next_iter(attributes);
}

void
cb_issue_ticket (void *cls,
                 const struct GNUNET_RECLAIM_Ticket *ticket,
                 const struct GNUNET_RECLAIM_PresentationList *presentations)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  attributes->op = NULL;

  if ((!(attributes->contact)) || (!(attributes->contact->member)))
    goto skip_sending;

  struct GNUNET_CHAT_Context *context = contact_find_context(
    attributes->contact,
    GNUNET_YES
  );

  if ((!context) || (!ticket))
    goto skip_sending;

  char *identifier = GNUNET_strdup(ticket->gns_name);

  if (!identifier)
    goto skip_sending;

  struct GNUNET_MESSENGER_Message message;
  memset(&message, 0, sizeof(message));

  message.header.kind = GNUNET_MESSENGER_KIND_TICKET;
  message.body.ticket.identifier = identifier;

  GNUNET_MESSENGER_send_message(
    context->room,
    &message,
    attributes->contact->member
  );

  GNUNET_free(identifier);

skip_sending:
  internal_attributes_destroy(attributes);
}

static struct GNUNET_RECLAIM_AttributeList*
attribute_list_from_attribute (const struct GNUNET_RECLAIM_Attribute *attribute)
{
  struct GNUNET_RECLAIM_AttributeList *attrs;
  struct GNUNET_RECLAIM_AttributeListEntry *le;

  attrs = GNUNET_new (struct GNUNET_RECLAIM_AttributeList);

  if (!attrs)
    return NULL;
  
  le = GNUNET_new (struct GNUNET_RECLAIM_AttributeListEntry);

  if (!le)
  {
    GNUNET_free (attrs);
    return NULL;
  }

  le->attribute = GNUNET_RECLAIM_attribute_new (
    attribute->name,
    &(attribute->credential),
    attribute->type,
    attribute->data,
    attribute->data_size
  );

  le->attribute->flag = attribute->flag;
  le->attribute->id = attribute->id;

  GNUNET_CONTAINER_DLL_insert (
    attrs->list_head,
    attrs->list_tail,
    le
  );

  return attrs;
}

void
cb_share_attribute (void *cls,
                    const struct GNUNET_CRYPTO_BlindablePublicKey *identity,
                    const struct GNUNET_RECLAIM_Attribute *attribute)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_AttributeProcess *attributes = (
    (struct GNUNET_CHAT_AttributeProcess*) cls
  );

  if (! attributes->name)
  {
    internal_attributes_stop_iter(attributes);
    return;
  }

  struct GNUNET_CHAT_Handle *handle = attributes->handle;

  if (0 != strcmp(attribute->name, attributes->name))
  {
    internal_attributes_next_iter(attributes);
    return;
  }
  
  internal_attributes_stop_iter(attributes);

  GNUNET_free(attributes->name);
  attributes->name = NULL;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (!key)
    return;

  const struct GNUNET_CRYPTO_BlindablePublicKey *pubkey = contact_get_key(
    attributes->contact
  );

  if (!pubkey)
    return;

  char *rp_uri = GNUNET_CRYPTO_blindable_public_key_to_string(pubkey);

  struct GNUNET_RECLAIM_AttributeList *attrs;
  attrs = attribute_list_from_attribute(attribute);

  if (!attrs)
    goto cleanup;

  attributes->op = GNUNET_RECLAIM_ticket_issue(
    handle->reclaim,
    key,
    rp_uri,
    attrs,
    cb_issue_ticket,
    attributes
  );
  
  GNUNET_RECLAIM_attribute_list_destroy(attrs);

cleanup:
  GNUNET_free(rp_uri);
}

void
cb_task_finish_iterate_ticket (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  tickets->iter = NULL;

  internal_tickets_destroy(tickets);
}

void
cb_task_error_iterate_ticket (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  handle_send_internal_message(
    tickets->handle,
    NULL,
    NULL,
    GNUNET_CHAT_FLAG_WARNING,
    "Ticket iteration failed!",
    GNUNET_YES
  );

  cb_task_finish_iterate_ticket(cls);
}

void
cont_revoke_ticket (void *cls,
                    int32_t success,
                    const char *emsg)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  tickets->op = NULL;

  struct GNUNET_CHAT_Handle *handle = tickets->handle;

  if (success == GNUNET_SYSERR)
    handle_send_internal_message(
      handle,
      NULL,
      NULL,
      GNUNET_CHAT_FLAG_WARNING,
      emsg,
      GNUNET_YES
    );
  else
    handle_send_internal_message(
      handle,
      NULL,
      NULL,
      GNUNET_CHAT_FLAG_SHARE_ATTRIBUTES,
      NULL,
      GNUNET_NO
    );

  internal_tickets_destroy(tickets);
}

void
cb_consume_ticket_check (void *cls,
                         const struct GNUNET_CRYPTO_BlindablePublicKey *identity,
                         const struct GNUNET_RECLAIM_Attribute *attribute,
                         const struct GNUNET_RECLAIM_Presentation *presentation)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  if ((!identity) && (!attribute) && (!presentation))
  {
    tickets->op = NULL;

    struct GNUNET_CHAT_Handle *handle = tickets->handle;

    const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
      handle
    );

    if (tickets->name)
    {
      GNUNET_free(tickets->name);
      tickets->name = NULL;
    }
    else if (key)
      tickets->op = GNUNET_RECLAIM_ticket_revoke(
        handle->reclaim,
        key,
        tickets->ticket,
        cont_revoke_ticket,
        tickets
      );
    
    if (tickets->ticket)
    {
      GNUNET_free(tickets->ticket);
      tickets->ticket = NULL;
    }

    if (tickets->op)
      return;

    internal_tickets_destroy(tickets);
    return;
  }

  if ((!attribute) || (! tickets->name) || 
      (0 != strcmp(tickets->name, attribute->name)))
    return;

  if (tickets->name)
  {
    GNUNET_free(tickets->name);
    tickets->name = NULL;
  }
}

static enum GNUNET_GenericReturnValue
is_contact_ticket_audience (const struct GNUNET_CHAT_Contact *contact,
                            const char *rp_uri)
{
  GNUNET_assert((contact) && (rp_uri));

  const struct GNUNET_CRYPTO_BlindablePublicKey *pubkey;
  pubkey = contact_get_key(contact);

  if (!pubkey)
    return GNUNET_NO;

  struct GNUNET_CRYPTO_BlindablePublicKey audience;
  enum GNUNET_GenericReturnValue parsing;

  parsing = GNUNET_CRYPTO_blindable_public_key_from_string(rp_uri, &audience);

  if ((GNUNET_OK != parsing) || (0 != GNUNET_memcmp(pubkey, &audience)))
    return GNUNET_NO;

  return GNUNET_YES;
}

void
cb_iterate_ticket_check (void *cls,
                         const struct GNUNET_RECLAIM_Ticket *ticket,
                         const char *rp_uri)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  struct GNUNET_CHAT_Handle *handle = tickets->handle;

  if ((!rp_uri) || (!(tickets->contact)) || 
      (GNUNET_YES != is_contact_ticket_audience(tickets->contact, rp_uri)))
  {
    internal_tickets_next_iter(tickets);
    return;
  }

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (!key)
  {
    internal_tickets_stop_iter(tickets);
    return;
  }

  struct GNUNET_CHAT_TicketProcess *new_tickets;
  new_tickets = internal_tickets_copy(tickets, ticket);

  if (!new_tickets)
  {
    internal_tickets_stop_iter(tickets);
    return;
  }

  new_tickets->op = GNUNET_RECLAIM_ticket_consume(
    handle->reclaim,
    ticket,
    rp_uri,
    cb_consume_ticket_check,
    new_tickets
  );

  internal_tickets_next_iter(tickets);
}

void
cb_consume_ticket (void *cls,
                   const struct GNUNET_CRYPTO_BlindablePublicKey *identity,
                   const struct GNUNET_RECLAIM_Attribute *attribute,
                   const struct GNUNET_RECLAIM_Presentation *presentation)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  if ((!identity) && (!attribute) && (!presentation))
  {
    tickets->op = NULL;

    internal_tickets_destroy(tickets);
    return;
  }

  if (!attribute)
    return;

  char *value = GNUNET_RECLAIM_attribute_value_to_string(
    attribute->type,
    attribute->data,
    attribute->data_size
  );

  if (tickets->callback)
    tickets->callback(tickets->closure, tickets->contact, attribute->name, value);

  if (value)
    GNUNET_free (value);
}

void
cb_iterate_ticket (void *cls,
                   const struct GNUNET_RECLAIM_Ticket *ticket,
                   const char *rp_uri)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_TicketProcess *tickets = (
    (struct GNUNET_CHAT_TicketProcess*) cls
  );

  struct GNUNET_CHAT_Handle *handle = tickets->handle;

  if ((!rp_uri) || (!(tickets->contact)) || 
      (GNUNET_YES != is_contact_ticket_audience(tickets->contact, rp_uri)))
  {
    internal_tickets_next_iter(tickets);
    return;
  }

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (!key)
  {
    internal_tickets_stop_iter(tickets);
    return;
  }

  struct GNUNET_CHAT_TicketProcess *new_tickets;
  new_tickets = internal_tickets_copy(tickets, NULL);

  if (!new_tickets)
  {
    internal_tickets_stop_iter(tickets);
    return;
  }

  new_tickets->op = GNUNET_RECLAIM_ticket_consume(
    handle->reclaim,
    ticket,
    rp_uri,
    cb_consume_ticket,
    new_tickets
  );

  internal_tickets_next_iter(tickets);
}
