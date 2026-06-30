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
 * @file gnunet_chat_lib.c
 */

#include "gnunet_chat_lib.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_fs_service.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_reclaim_lib.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_time_lib.h>
#include <gnunet/gnunet_util_lib.h>

#include <libgen.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define _(String) ((const char*) String)

#include "gnunet_chat_contact.h"
#include "gnunet_chat_context.h"
#include "gnunet_chat_discourse.h"
#include "gnunet_chat_file.h"
#include "gnunet_chat_group.h"
#include "gnunet_chat_handle.h"
#include "gnunet_chat_invitation.h"
#include "gnunet_chat_lobby.h"
#include "gnunet_chat_message.h"
#include "gnunet_chat_ticket.h"
#include "gnunet_chat_util.h"

#include "internal/gnunet_chat_tagging.h"

#include "gnunet_chat_lib_intern.c"

#define GNUNET_CHAT_VERSION_ASSERT() {\
  GNUNET_assert(\
    (GNUNET_MESSENGER_VERSION == ((GNUNET_CHAT_VERSION >> 16L) & 0xFFFFFFFFL))\
  );\
}

static const uint32_t block_anonymity_level = 1;
static const uint32_t block_content_priority = 100;
static const uint32_t block_replication_level = 1;

struct GNUNET_CHAT_Handle*
GNUNET_CHAT_start (const struct GNUNET_CONFIGURATION_Handle *cfg,
		               GNUNET_CHAT_ContextMessageCallback msg_cb, void *msg_cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!cfg)
    return NULL;

  return handle_create_from_config(
    cfg,
    msg_cb,
    msg_cls
  );
}


void
GNUNET_CHAT_stop (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  handle->destruction = GNUNET_SCHEDULER_add_with_priority(
    GNUNET_SCHEDULER_PRIORITY_URGENT,
    task_handle_destruction,
    handle
  );
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_account_create (struct GNUNET_CHAT_Handle *handle,
			                      const char* name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!name))
    return GNUNET_SYSERR;

  char *low = util_get_lower(name);

  enum GNUNET_GenericReturnValue result;
  result = handle_create_account(handle, low);

  GNUNET_free(low);
  return result;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_account_delete(struct GNUNET_CHAT_Handle *handle,
			                     const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!name))
    return GNUNET_SYSERR;

  const struct GNUNET_CHAT_Account *account;
  account = handle_get_account_by_name(handle, name, GNUNET_NO);

  if (!account)
    return GNUNET_SYSERR;

  return handle_delete_account(handle, account);
}


int
GNUNET_CHAT_iterate_accounts (struct GNUNET_CHAT_Handle *handle,
                              GNUNET_CHAT_AccountCallback callback,
                              void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return GNUNET_SYSERR;

  int iterations = 0;

  struct GNUNET_CHAT_InternalAccounts *accounts = handle->accounts_head;
  while (accounts)
  {
    if ((!(accounts->account)) || (accounts->op))
      goto skip_account;

    iterations++;

    if ((callback) && (GNUNET_YES != callback(cls, handle, accounts->account)))
      break;

  skip_account:
    accounts = accounts->next;
  }

  return iterations;
}


struct GNUNET_CHAT_Account*
GNUNET_CHAT_find_account (const struct GNUNET_CHAT_Handle *handle,
                          const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return NULL;

  return handle_get_account_by_name(handle, name, GNUNET_YES);
}


void
GNUNET_CHAT_connect (struct GNUNET_CHAT_Handle *handle,
                     struct GNUNET_CHAT_Account *account,
                     const char *secret,
                     uint32_t secret_len)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  if (handle->connection)
    GNUNET_SCHEDULER_cancel(handle->connection);

  if (handle->current == account)
  {
    handle->next = NULL;
    handle->connection = NULL;
    return;
  }

  handle->next = account;

  if (handle->next_secret)
  {
    GNUNET_CRYPTO_zero_keys(
      handle->next_secret,
      sizeof(*(handle->next_secret))
    );

    GNUNET_free(handle->next_secret);
  }

  if ((secret) && (secret_len > 0))
  {
    handle->next_secret = GNUNET_new(struct GNUNET_HashCode);

    if (handle->next_secret)
      GNUNET_CRYPTO_hash(secret, secret_len, handle->next_secret);
  }
  else
    handle->next_secret = NULL;

  if (handle->current)
  {
    handle->connection = NULL;
    GNUNET_CHAT_disconnect(handle);
    return;
  }

  handle->connection = GNUNET_SCHEDULER_add_now(
    task_handle_connection,
    handle
  );
}


void
GNUNET_CHAT_disconnect (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  if (handle->connection)
    GNUNET_SCHEDULER_cancel(handle->connection);

  if (!(handle->current))
  {
    handle->next = NULL;
    handle->connection = NULL;
    return;
  }

  handle->connection = GNUNET_SCHEDULER_add_now(
    task_handle_disconnection,
    handle
  );
}


struct GNUNET_CHAT_Account*
GNUNET_CHAT_get_connected (const struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return NULL;

  return handle->current;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_update (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return GNUNET_SYSERR;

  return handle_update(handle);
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_set_name (struct GNUNET_CHAT_Handle *handle,
		                  const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return GNUNET_SYSERR;

  if (!name)
    return GNUNET_NO;

  char *low = util_get_lower(name);
  enum GNUNET_GenericReturnValue result;

  if (handle->current)
    result = handle_rename_account(handle, handle->current, low);
  else
    result = GNUNET_OK;

  if (GNUNET_OK != result)
    return result;

  result = GNUNET_MESSENGER_set_name(handle->messenger, low);

  GNUNET_free(low);
  return result;
}


const char*
GNUNET_CHAT_get_name (const struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return NULL;

  return GNUNET_MESSENGER_get_name(handle->messenger);
}


const char*
GNUNET_CHAT_get_key (const struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return NULL;

  return handle->public_key;
}


void
GNUNET_CHAT_set_attribute (struct GNUNET_CHAT_Handle *handle,
                           const char *name,
                           const char *value)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if ((!key) || (!name))
    return;

  struct GNUNET_TIME_Relative rel;
  rel = GNUNET_TIME_relative_get_forever_();

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create_store(handle, name, rel);

  if (!attributes)
    return;

  if (value)
  {
    enum GNUNET_GenericReturnValue result;
    result = GNUNET_RECLAIM_attribute_string_to_value(
      GNUNET_RECLAIM_ATTRIBUTE_TYPE_STRING,
      value,
      &(attributes->data),
      &(attributes->attribute->data_size)
    );

    if (GNUNET_OK != result)
    {
      internal_attributes_destroy(attributes);
      return;
    }

    attributes->attribute->type = GNUNET_RECLAIM_ATTRIBUTE_TYPE_STRING;
    attributes->attribute->data = attributes->data;
  }

  attributes->iter = GNUNET_RECLAIM_get_attributes_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_attribute,
    attributes,
    cb_store_attribute,
    attributes,
    cb_task_finish_iterate_attribute,
    attributes
  );
}


void
GNUNET_CHAT_delete_attribute (struct GNUNET_CHAT_Handle *handle,
                              const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if ((!key) || (!name))
    return;

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create(handle, name);

  if (!attributes)
    return;

  attributes->iter = GNUNET_RECLAIM_get_attributes_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_attribute,
    attributes,
    cb_delete_attribute,
    attributes,
    cb_task_finish_iterate_attribute,
    attributes
  );
}


void
GNUNET_CHAT_get_attributes (struct GNUNET_CHAT_Handle *handle,
                            GNUNET_CHAT_AttributeCallback callback,
                            void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (!key)
    return;

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create(handle, NULL);

  if (!attributes)
    return;

  attributes->callback = callback;
  attributes->closure = cls;

  attributes->iter = GNUNET_RECLAIM_get_attributes_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_attribute,
    attributes,
    cb_iterate_attribute,
    attributes,
    cb_task_finish_iterate_attribute,
    attributes
  );
}


void
GNUNET_CHAT_share_attribute_with (struct GNUNET_CHAT_Handle *handle,
                                  struct GNUNET_CHAT_Contact *contact,
                                  const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!contact))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  const struct GNUNET_CRYPTO_BlindablePublicKey *pubkey = contact_get_key(
    contact
  );

  if ((!key) || (!pubkey) || (!name))
    return;

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create_share(handle, contact, name);

  if (!attributes)
    return;

  attributes->iter = GNUNET_RECLAIM_get_attributes_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_attribute,
    attributes,
    cb_share_attribute,
    attributes,
    cb_task_finish_iterate_attribute,
    attributes
  );
}


void
GNUNET_CHAT_unshare_attribute_from (struct GNUNET_CHAT_Handle *handle,
                                    struct GNUNET_CHAT_Contact *contact,
                                    const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!contact))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if ((!key) || (!name))
    return;

  struct GNUNET_CHAT_TicketProcess *tickets;
  tickets = internal_tickets_create(handle, contact, name);

  if (!tickets)
    return;

  tickets->iter = GNUNET_RECLAIM_ticket_iteration_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_ticket,
    tickets,
    cb_iterate_ticket_check,
    tickets,
    cb_task_finish_iterate_ticket,
    tickets
  );
}


void
GNUNET_CHAT_get_shared_attributes (struct GNUNET_CHAT_Handle *handle,
                                   struct GNUNET_CHAT_Contact *contact,
                                   GNUNET_CHAT_ContactAttributeCallback callback,
                                   void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!contact))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    handle
  );

  if (!key)
    return;

  struct GNUNET_CHAT_TicketProcess *tickets;
  tickets = internal_tickets_create(handle, contact, NULL);

  if (!tickets)
    return;

  tickets->callback = callback;
  tickets->closure = cls;

  tickets->iter = GNUNET_RECLAIM_ticket_iteration_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_ticket,
    tickets,
    cb_iterate_ticket,
    tickets,
    cb_task_finish_iterate_ticket,
    tickets
  );
}


struct GNUNET_CHAT_Uri*
GNUNET_CHAT_uri_parse (const char *uri,
		                   char **emsg)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!uri)
    return NULL;

  return uri_parse_from_string(uri, emsg);
}


char*
GNUNET_CHAT_uri_to_string (const struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!uri)
    return NULL;

  return uri_to_string(uri);
}


enum GNUNET_CHAT_UriType
GNUNET_CHAT_uri_get_type (const struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!uri)
    return GNUNET_CHAT_URI_TYPE_UNKNOWN;

  return uri->type;
}


void
GNUNET_CHAT_uri_destroy (struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!uri)
    return;

  uri_destroy(uri);
}


struct GNUNET_CHAT_Lobby*
GNUNET_CHAT_lobby_open (struct GNUNET_CHAT_Handle *handle,
                        unsigned int delay,
                        GNUNET_CHAT_LobbyCallback callback,
                        void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return NULL;

  struct GNUNET_TIME_Relative rel = GNUNET_TIME_relative_multiply(
    GNUNET_TIME_relative_get_second_(), delay
  );

  struct GNUNET_CHAT_InternalLobbies *lobbies = GNUNET_new(
    struct GNUNET_CHAT_InternalLobbies
  );

  lobbies->lobby = lobby_create(handle);

  GNUNET_CONTAINER_DLL_insert(
    handle->lobbies_head,
    handle->lobbies_tail,
    lobbies
  );

  lobby_open(lobbies->lobby, rel, callback, cls);

  return lobbies->lobby;
}


void
GNUNET_CHAT_lobby_close (struct GNUNET_CHAT_Lobby *lobby)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!lobby) || (lobby->destruction))
    return;

  lobby->destruction = GNUNET_SCHEDULER_add_now(
    task_lobby_destruction,
    lobby
  );
}


void
GNUNET_CHAT_lobby_join (struct GNUNET_CHAT_Handle *handle,
			                  const struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!(handle->gns)) ||
      (!uri) || (GNUNET_CHAT_URI_TYPE_CHAT != uri->type))
    return;

  struct GNUNET_CHAT_UriLookups *lookups = GNUNET_new(
    struct GNUNET_CHAT_UriLookups
  );

  lookups->handle = handle;
  lookups->uri = uri_create_chat(
    &(uri->chat.zone),
    uri->chat.label
  );

  lookups->request = GNUNET_GNS_lookup(
    handle->gns,
    lookups->uri->chat.label,
    &(uri->chat.zone),
    GNUNET_GNSRECORD_TYPE_MESSENGER_ROOM_ENTRY,
    GNUNET_GNS_LO_DEFAULT,
    cb_lobby_lookup,
    lookups
  );

  GNUNET_CONTAINER_DLL_insert(
    handle->lookups_head,
    handle->lookups_tail,
    lookups
  );
}


struct GNUNET_CHAT_File*
GNUNET_CHAT_request_file (struct GNUNET_CHAT_Handle *handle,
                          const struct GNUNET_CHAT_Uri *uri)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) ||
      (!uri) || (GNUNET_CHAT_URI_TYPE_FS != uri->type))
    return NULL;

  if (!GNUNET_FS_uri_test_chk(uri->fs.uri))
    return NULL;

  const struct GNUNET_HashCode *hash = GNUNET_FS_uri_chk_get_file_hash(
    uri->fs.uri
  );

  if (!hash)
    return NULL;

  struct GNUNET_CHAT_File *file = GNUNET_CONTAINER_multihashmap_get(
    handle->files,
    hash
  );

  if (file)
    return file;

  file = file_create_from_chk_uri(handle, uri->fs.uri);

  if (!file)
    return NULL;

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(handle->files, hash, file,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    file_destroy(file);
    file = NULL;
  }

  return file;
}


struct GNUNET_CHAT_File*
GNUNET_CHAT_upload_file (struct GNUNET_CHAT_Handle *handle,
                         const char *path,
                         GNUNET_CHAT_FileUploadCallback callback,
                         void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) ||
      (!path))
    return NULL;

  struct GNUNET_HashCode hash;
  if (GNUNET_OK != util_hash_file(path, &hash))
    return NULL;

  char *filename = handle_create_file_path(
    handle, &hash
  );

  if (!filename)
    return NULL;

  struct GNUNET_CHAT_File *file = GNUNET_CONTAINER_multihashmap_get(
    handle->files,
    &hash
  );

  if (file)
    goto file_binding;

  if ((GNUNET_YES == GNUNET_DISK_file_test(filename)) ||
      (GNUNET_OK != GNUNET_DISK_directory_create_for_file(filename)) ||
      (GNUNET_OK != GNUNET_DISK_file_copy(path, filename)))
  {
    GNUNET_free(filename);
    return NULL;
  }

  char* p = GNUNET_strdup(path);

  file = file_create_from_disk(
    handle,
    basename(p),
    &hash
  );

  GNUNET_free(p);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->files, &hash, file,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    file_destroy(file);
    GNUNET_free(filename);
    return NULL;
  }

  struct GNUNET_FS_BlockOptions bo;

  bo.anonymity_level = block_anonymity_level;
  bo.content_priority = block_content_priority;
  bo.replication_level = block_replication_level;
  bo.expiration_time = GNUNET_TIME_absolute_get_forever_();

  struct GNUNET_FS_FileInformation* fi = GNUNET_FS_file_information_create_from_file(
    handle->fs,
    file,
    filename,
    NULL,
    file->meta,
    GNUNET_YES,
    &bo
  );

  file->publish = GNUNET_FS_publish_start(
    handle->fs, fi,
    NULL, NULL, NULL,
    GNUNET_FS_PUBLISH_OPTION_NONE
  );

  if (file->publish)
    file->status |= GNUNET_CHAT_FILE_STATUS_PUBLISH;

  GNUNET_free(filename);

file_binding:
  file_bind_upload(file, NULL, callback, cls);
  return file;
}


int
GNUNET_CHAT_iterate_files (struct GNUNET_CHAT_Handle *handle,
                           GNUNET_CHAT_FileCallback callback,
                           void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_IterateFiles it;
  it.handle = handle;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_CONTAINER_multihashmap_iterate(
    handle->files,
    it_iterate_files,
    &it
  );
}


int
GNUNET_CHAT_context_iterate_discourses (struct GNUNET_CHAT_Context *context,
                                        GNUNET_CHAT_DiscourseCallback callback,
                                        void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!(context->discourses)))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_ContextIterateDiscourses it;
  it.context = context;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_CONTAINER_multishortmap_iterate(
    context->discourses,
    it_context_iterate_discourses,
    &it
  );
}


void
GNUNET_CHAT_set_user_pointer (struct GNUNET_CHAT_Handle *handle,
			                        void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return;

  handle->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_get_user_pointer (const struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction))
    return NULL;

  return handle->user_pointer;
}


int
GNUNET_CHAT_iterate_contacts (struct GNUNET_CHAT_Handle *handle,
                              GNUNET_CHAT_ContactCallback callback,
                              void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!(handle->contacts)))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_HandleIterateContacts it;
  it.handle = handle;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_CONTAINER_multishortmap_iterate(
    handle->contacts, it_handle_iterate_contacts, &it
  );
}


struct GNUNET_CHAT_Contact*
GNUNET_CHAT_get_own_contact (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!(handle->own_contact))
    GNUNET_CHAT_iterate_contacts (handle, it_handle_find_own_contact, NULL);

  return handle->own_contact;
}


const char*
GNUNET_CHAT_account_get_name (const struct GNUNET_CHAT_Account *account)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!account)
    return NULL;

  return account_get_name(account);
}


void
GNUNET_CHAT_account_get_attributes (struct GNUNET_CHAT_Handle *handle,
                                    struct GNUNET_CHAT_Account *account,
                                    GNUNET_CHAT_AccountAttributeCallback callback,
                                    void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!account))
    return;

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = account_get_key(
    account
  );

  if (!key)
    return;

  struct GNUNET_CHAT_AttributeProcess *attributes;
  attributes = internal_attributes_create_request(handle, account);

  if (!attributes)
    return;

  attributes->account_callback = callback;
  attributes->closure = cls;

  attributes->iter = GNUNET_RECLAIM_get_attributes_start(
    handle->reclaim,
    key,
    cb_task_error_iterate_attribute,
    attributes,
    cb_iterate_attribute,
    attributes,
    cb_task_finish_iterate_attribute,
    attributes
  );
}


void
GNUNET_CHAT_account_set_user_pointer (struct GNUNET_CHAT_Account *account,
				                              void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!account)
    return;

  account->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_account_get_user_pointer (const struct GNUNET_CHAT_Account *account)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!account)
    return NULL;

  return account->user_pointer;
}


struct GNUNET_CHAT_Group *
GNUNET_CHAT_group_create (struct GNUNET_CHAT_Handle *handle,
                          const char* topic)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) ||
      (!(handle->groups)) || (!(handle->contexts)))
    return NULL;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_MESSENGER_create_room_key(
    &key,
    topic,
    topic? GNUNET_YES : GNUNET_NO,
    GNUNET_YES,
    GNUNET_NO
  );

  if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(handle->contexts, &(key.hash)))
    return NULL;

  struct GNUNET_MESSENGER_Room *room = GNUNET_MESSENGER_open_room(
    handle->messenger, &key
  );

  if (!room)
    return NULL;

  struct GNUNET_CHAT_Context *context = context_create_from_room(handle, room);
  context->type = GNUNET_CHAT_CONTEXT_TYPE_GROUP;

  util_set_name_field(topic, &(context->topic));

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->contexts, &(key.hash), context,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    goto destroy_context;

  struct GNUNET_CHAT_Group *group = group_create_from_context(handle, context);

  if (context->topic)
    group_publish(group);

  if (GNUNET_OK == GNUNET_CONTAINER_multihashmap_put(
      handle->groups, &(key.hash), group,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    context_write_records(context);
    return group;
  }

  group_destroy(group);

  GNUNET_CONTAINER_multihashmap_remove(handle->contexts, &(key.hash), context);

destroy_context:
  context_destroy(context);
  return NULL;
}


int
GNUNET_CHAT_iterate_groups (struct GNUNET_CHAT_Handle *handle,
                            GNUNET_CHAT_GroupCallback callback,
                            void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!handle) || (handle->destruction) || (!(handle->groups)))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_HandleIterateGroups it;
  it.handle = handle;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_CONTAINER_multihashmap_iterate(
    handle->groups, it_handle_iterate_groups, &it
  );
}


void
GNUNET_CHAT_contact_delete (struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!contact) || (contact->destruction))
    return;

  if (contact->context)
    contact->context->deleted = GNUNET_YES;

  contact->destruction = GNUNET_SCHEDULER_add_now(
    task_contact_destruction,
    contact
  );
}


void
GNUNET_CHAT_contact_set_name (struct GNUNET_CHAT_Contact *contact,
			                        const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!contact) || (!(contact->context)) ||
      (contact->context->topic))
    return;

  context_update_nick(contact->context, name);

  if (contact->context->room)
    context_write_records(contact->context);
}


const char*
GNUNET_CHAT_contact_get_name (const struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return NULL;

  if ((contact->context) && (! contact->context->topic) &&
      (contact->context->nick))
    return contact->context->nick;

  return GNUNET_MESSENGER_contact_get_name(contact->member);
}


const char*
GNUNET_CHAT_contact_get_key (const struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return NULL;

  return contact->public_key;
}


struct GNUNET_CHAT_Context*
GNUNET_CHAT_contact_get_context (struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return NULL;

  if (contact->context)
    return contact->context;

  struct GNUNET_CHAT_Context *context = contact_find_context(
    contact,
    GNUNET_NO
  );

  if ((context) && (GNUNET_CHAT_CONTEXT_TYPE_CONTACT == context->type))
    goto attach_return;

  context = context_create_from_contact(contact->handle, contact->member);

attach_return:
  if (context)
    contact->context = context;

  return context;
}


void
GNUNET_CHAT_contact_set_user_pointer (struct GNUNET_CHAT_Contact *contact,
				                              void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return;

  contact->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_contact_get_user_pointer (const struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return NULL;

  return contact->user_pointer;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_contact_is_owned (const struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return GNUNET_SYSERR;

  return contact->owned;
}


void
GNUNET_CHAT_contact_set_blocked (struct GNUNET_CHAT_Contact *contact,
                                 enum GNUNET_GenericReturnValue blocked)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return;

  struct GNUNET_CHAT_ContactIterateContexts it;
  it.contact = contact;
  it.tag = NULL;

  if (GNUNET_NO == blocked)
    it.cb = contact_untag;
  else if (GNUNET_YES == blocked)
    it.cb = contact_tag;
  else
    return;

  GNUNET_CONTAINER_multihashmap_iterate(
    contact->joined,
    it_contact_iterate_contexts,
    &it
  );
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_contact_is_blocked (const struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return GNUNET_SYSERR;

  return contact_is_tagged(contact, NULL, NULL);
}


void
GNUNET_CHAT_contact_tag (struct GNUNET_CHAT_Contact *contact,
                         const char *tag)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!contact) || (!tag) || (!tag[0]))
    return;

  struct GNUNET_CHAT_ContactIterateContexts it;
  it.contact = contact;
  it.tag = tag;
  it.cb = contact_tag;

  GNUNET_CONTAINER_multihashmap_iterate(
    contact->joined,
    it_contact_iterate_contexts,
    &it
  );
}


void
GNUNET_CHAT_contact_untag (struct GNUNET_CHAT_Contact *contact,
                           const char *tag)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!contact) || (!tag) || (!tag[0]))
    return;

  struct GNUNET_CHAT_ContactIterateContexts it;
  it.contact = contact;
  it.tag = tag;
  it.cb = contact_untag;

  GNUNET_CONTAINER_multihashmap_iterate(
    contact->joined,
    it_contact_iterate_contexts,
    &it
  );
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_contact_is_tagged (const struct GNUNET_CHAT_Contact *contact,
                               const char *tag)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!contact) || (!tag) || (!tag[0]))
    return GNUNET_SYSERR;

  return contact_is_tagged(contact, NULL, tag);
}


int
GNUNET_CHAT_contact_iterate_tags (struct GNUNET_CHAT_Contact *contact,
                                  GNUNET_CHAT_ContactTagCallback callback,
                                  void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return GNUNET_SYSERR;

  return contact_iterate_tags(
    contact,
    NULL,
    callback,
    cls
  );
}


void
GNUNET_CHAT_contact_get_attributes (struct GNUNET_CHAT_Contact *contact,
                                    GNUNET_CHAT_ContactAttributeCallback callback,
                                    void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!contact)
    return;

  struct GNUNET_CHAT_InternalTickets *tickets;
  tickets = contact->tickets_head;

  while (tickets)
  {
    ticket_consume(
      tickets->ticket,
      callback,
      cls
    );

    tickets = tickets->next;
  }
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_group_leave (struct GNUNET_CHAT_Group *group)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!group) || (group->destruction))
    return GNUNET_SYSERR;

  group->context->deleted = GNUNET_YES;
  group->destruction = GNUNET_SCHEDULER_add_now(
    task_group_destruction,
    group
  );

  return GNUNET_OK;
}


void
GNUNET_CHAT_group_set_name (struct GNUNET_CHAT_Group *group,
			                      const char *name)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!group) || (!(group->context)))
    return;

  context_update_nick(group->context, name);

  if (group->context->room)
    context_write_records(group->context);
}


const char*
GNUNET_CHAT_group_get_name (const struct GNUNET_CHAT_Group *group)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!group) || (!(group->context)))
    return NULL;

  if (group->context->nick)
    return group->context->nick;

  return group->context->topic;
}


void
GNUNET_CHAT_group_set_user_pointer (struct GNUNET_CHAT_Group *group,
				                            void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!group)
    return;

  group->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_group_get_user_pointer (const struct GNUNET_CHAT_Group *group)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!group)
    return NULL;

  return group->user_pointer;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_group_invite_contact (struct GNUNET_CHAT_Group *group,
				                          struct GNUNET_CHAT_Contact *contact)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!group) || (!contact) || (!contact->member))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_Context *context = contact_find_context(
    contact,
    GNUNET_YES
  );

  if (!context)
    return GNUNET_SYSERR;

  const struct GNUNET_PeerIdentity *pid = context->handle->pid;

  if (!pid)
    return GNUNET_SYSERR;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_memcpy(
    &(key.hash),
    GNUNET_MESSENGER_room_get_key(group->context->room),
    sizeof(key.hash)
  );

  handle_send_room_name(group->handle, GNUNET_MESSENGER_open_room(
    group->handle->messenger, &key
  ));

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_INVITE;
  GNUNET_memcpy(&(msg.body.invite.door), pid, sizeof(msg.body.invite.door));
  GNUNET_memcpy(&(msg.body.invite.key), &key, sizeof(msg.body.invite.key));

  GNUNET_MESSENGER_send_message(context->room, &msg, contact->member);
  return GNUNET_OK;
}


int
GNUNET_CHAT_group_iterate_contacts (struct GNUNET_CHAT_Group *group,
                                    GNUNET_CHAT_GroupContactCallback callback,
                                    void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!group)
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_GroupIterateContacts it;
  it.group = group;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_MESSENGER_iterate_members(
    group->context->room, it_group_iterate_contacts, &it
  );
}


void
GNUNET_CHAT_member_set_user_pointer (struct GNUNET_CHAT_Group *group,
                                     const struct GNUNET_CHAT_Contact *member,
                                     void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!group) || (!(group->context)) || (!member))
    return;

  struct GNUNET_ShortHashCode hash;
  util_shorthash_from_member(member->member, &hash);

  GNUNET_CONTAINER_multishortmap_put(
    group->context->member_pointers,
    &hash,
    user_pointer,
    GNUNET_CONTAINER_MULTIHASHMAPOPTION_REPLACE
  );
}


void*
GNUNET_CHAT_member_get_user_pointer (const struct GNUNET_CHAT_Group *group,
				                             const struct GNUNET_CHAT_Contact *member)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!group) || (!(group->context)) || (!member))
    return NULL;

  struct GNUNET_ShortHashCode hash;
  util_shorthash_from_member(member->member, &hash);

  return GNUNET_CONTAINER_multishortmap_get(
    group->context->member_pointers,
    &hash
  );
}


struct GNUNET_CHAT_Context*
GNUNET_CHAT_group_get_context (struct GNUNET_CHAT_Group *group)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!group)
    return NULL;

  return group->context;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_get_status (struct GNUNET_CHAT_Context *context)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!(context->room)))
    return GNUNET_SYSERR;

  switch (context->type) {
    case GNUNET_CHAT_CONTEXT_TYPE_CONTACT:
    {
      const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_context_get_contact(
        context
      );

      return contact? GNUNET_OK : GNUNET_NO;
    }
    case GNUNET_CHAT_CONTEXT_TYPE_GROUP:
      return GNUNET_OK;
    default:
      return GNUNET_NO;
  }
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_request (struct GNUNET_CHAT_Context *context)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!context)
    return GNUNET_SYSERR;
  else if (context->room)
    return GNUNET_OK;

  struct GNUNET_CHAT_Handle *handle = context->handle;

  if ((!handle) || (!(context->contact)))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_Contact *contact = handle_get_contact_from_messenger(
    handle, context->contact
  );

  if (!contact)
    return GNUNET_SYSERR;

  enum GNUNET_GenericReturnValue owned = GNUNET_CHAT_contact_is_owned(
    contact
  );

  context->type = GNUNET_CHAT_CONTEXT_TYPE_CONTACT;

  struct GNUNET_CHAT_Context *other = contact_find_context(
    contact, GNUNET_YES
  );

  if (!other)
    return GNUNET_SYSERR;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_MESSENGER_create_room_key(
    &key,
    NULL,
    GNUNET_YES == owned? GNUNET_YES : GNUNET_NO,
    GNUNET_NO,
    GNUNET_YES == owned? GNUNET_YES : GNUNET_NO
  );

  if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(
      handle->contexts, &(key.hash)))
    return GNUNET_SYSERR;

  const struct GNUNET_PeerIdentity *pid = handle->pid;

  if (!pid)
    return GNUNET_SYSERR;

  struct GNUNET_MESSENGER_Room *room;
  if (GNUNET_YES == owned)
  {
    room = GNUNET_MESSENGER_enter_room(
      handle->messenger,
      pid,
      &key
    );
  }
  else
    room = GNUNET_MESSENGER_open_room(
      handle->messenger, &key
    );

  if (!room)
    return GNUNET_SYSERR;

  context_update_room(context, room, GNUNET_YES);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->contexts, &(key.hash), context,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    context_update_room(context, NULL, GNUNET_YES);
    return GNUNET_SYSERR;
  }

  if (GNUNET_YES != owned)
  {
    struct GNUNET_MESSENGER_Message msg;
    memset(&msg, 0, sizeof(msg));

    msg.header.kind = GNUNET_MESSENGER_KIND_INVITE;
    GNUNET_memcpy(&(msg.body.invite.door), pid, sizeof(msg.body.invite.door));
    GNUNET_memcpy(&(msg.body.invite.key), &key, sizeof(msg.body.invite.key));

    GNUNET_MESSENGER_send_message(other->room, &msg, context->contact);
  }

  return GNUNET_OK;
}


struct GNUNET_CHAT_Contact*
GNUNET_CHAT_context_get_contact (struct GNUNET_CHAT_Context *context)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (GNUNET_CHAT_CONTEXT_TYPE_CONTACT != context->type))
    return NULL;

  if (context->contact)
    return handle_get_contact_from_messenger(context->handle, context->contact);

  struct GNUNET_MESSENGER_Room *room = context->room;
  struct GNUNET_CHAT_RoomFindContact find;
  union GNUNET_MESSENGER_RoomKey key;

  GNUNET_memcpy(&(key.hash), GNUNET_MESSENGER_room_get_key(room), sizeof(key.hash));

  if (key.code.group_bit)
    return NULL;

  if (! key.code.feed_bit)
    find.ignore_key = GNUNET_MESSENGER_get_key(context->handle->messenger);
  else
    find.ignore_key = NULL;

  find.contact = NULL;

  int member_count = GNUNET_MESSENGER_iterate_members(
    room,
    it_room_find_contact,
    &find
  );

  if ((!find.contact) || (member_count > 2))
    return NULL;

  return handle_get_contact_from_messenger(context->handle, find.contact);
}


struct GNUNET_CHAT_Group*
GNUNET_CHAT_context_get_group (struct GNUNET_CHAT_Context *context)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (GNUNET_CHAT_CONTEXT_TYPE_GROUP != context->type))
    return NULL;

  if (!(context->room))
    return NULL;

  return handle_get_group_from_messenger(context->handle, context->room);
}


void
GNUNET_CHAT_context_set_user_pointer (struct GNUNET_CHAT_Context *context,
				                              void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!context)
    return;

  context->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_context_get_user_pointer (const struct GNUNET_CHAT_Context *context)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!context)
    return NULL;

  return context->user_pointer;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_send_text (struct GNUNET_CHAT_Context *context,
			                         const char *text)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!text) || (!(context->room)))
    return GNUNET_SYSERR;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TEXT;
  msg.body.text.text = GNUNET_strdup(text);

  GNUNET_MESSENGER_send_message(context->room, &msg, NULL);

  GNUNET_free(msg.body.text.text);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_send_read_receipt (struct GNUNET_CHAT_Context *context,
				                               struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!(context->room)))
    return GNUNET_SYSERR;

  char zero = '\0';
  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TEXT;
  msg.body.text.text = &zero;

  const struct GNUNET_MESSENGER_Contact *receiver = NULL;

  if (!message)
    goto skip_filter;

  if (GNUNET_CHAT_FLAG_NONE != message->flag)
    return GNUNET_SYSERR;

  if (message->flags & GNUNET_MESSENGER_FLAG_SENT)
    return GNUNET_OK;

  if (message->flags & GNUNET_MESSENGER_FLAG_PRIVATE)
  {
    receiver = GNUNET_MESSENGER_get_sender(context->room, &(message->hash));

    if (!receiver)
      return GNUNET_SYSERR;
  }

  if ((GNUNET_YES != message_has_msg(message)) ||
      (GNUNET_MESSENGER_KIND_TEXT != message->msg->header.kind))
    goto skip_filter;

  if ((!(message->msg->body.text.text)) ||
      (!(message->msg->body.text.text[0])))
    return GNUNET_SYSERR;

skip_filter:
  GNUNET_MESSENGER_send_message(context->room, &msg, receiver);
  return GNUNET_OK;
}


struct GNUNET_CHAT_File*
GNUNET_CHAT_context_send_file (struct GNUNET_CHAT_Context *context,
                               const char *path,
                               GNUNET_CHAT_FileUploadCallback callback,
                               void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!path) || (!(context->room)))
    return NULL;

  struct GNUNET_HashCode hash;
  if (GNUNET_OK != util_hash_file(path, &hash))
    return NULL;

  char *filename = handle_create_file_path(
    context->handle, &hash
  );

  if (!filename)
    return NULL;

  struct GNUNET_CHAT_File *file = GNUNET_CONTAINER_multihashmap_get(
    context->handle->files,
    &hash
  );

  if (file)
    goto file_binding;

  if ((GNUNET_YES == GNUNET_DISK_file_test(filename)) ||
      (GNUNET_OK != GNUNET_DISK_directory_create_for_file(filename)) ||
      (GNUNET_OK != GNUNET_DISK_file_copy(path, filename)))
  {
    GNUNET_free(filename);
    return NULL;
  }

  char* p = GNUNET_strdup(path);

  file = file_create_from_disk(
    context->handle,
    basename(p),
    &hash
  );

  GNUNET_free(p);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      context->handle->files, &hash, file,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    file_destroy(file);
    GNUNET_free(filename);
    return NULL;
  }

  struct GNUNET_FS_BlockOptions bo;

  bo.anonymity_level = block_anonymity_level;
  bo.content_priority = block_content_priority;
  bo.replication_level = block_replication_level;
  bo.expiration_time = GNUNET_TIME_absolute_get_forever_();

  struct GNUNET_FS_FileInformation* fi = GNUNET_FS_file_information_create_from_file(
    context->handle->fs,
    file,
    filename,
    NULL,
    file->meta,
    GNUNET_YES,
    &bo
  );

  file->publish = GNUNET_FS_publish_start(
    context->handle->fs, fi,
    NULL, NULL, NULL,
    GNUNET_FS_PUBLISH_OPTION_NONE
  );

  if (file->publish)
    file->status |= GNUNET_CHAT_FILE_STATUS_PUBLISH;

  GNUNET_free(filename);

file_binding:
  file_bind_upload(file, context, callback, cls);
  return file;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_share_file (struct GNUNET_CHAT_Context *context,
				                        struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!file) ||
      (!(file->name)) || (strlen(file->name) > NAME_MAX) ||
      (!(file->uri)) || (!(context->room)))
    return GNUNET_SYSERR;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_FILE;

  GNUNET_memcpy(&(msg.body.file.hash), &(file->hash), sizeof(file->hash));
  GNUNET_strlcpy(msg.body.file.name, file->name, NAME_MAX);
  msg.body.file.uri = GNUNET_FS_uri_to_string(file->uri);

  GNUNET_MESSENGER_send_message(context->room, &msg, NULL);

  GNUNET_free(msg.body.file.uri);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_send_tag (struct GNUNET_CHAT_Context *context,
                              struct GNUNET_CHAT_Message *message,
                              const char *tag)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!message) || (!tag) || (!(context->room)))
    return GNUNET_SYSERR;

  char *tag_value = GNUNET_strdup(tag);

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TAG;
  GNUNET_memcpy(&(msg.body.tag.hash), &(message->hash),
    sizeof(struct GNUNET_HashCode));
  msg.body.tag.tag = tag_value;

  GNUNET_MESSENGER_send_message(
    context->room,
    &msg,
    NULL
  );

  GNUNET_free(tag_value);
  return GNUNET_OK;
}


struct GNUNET_CHAT_Discourse*
GNUNET_CHAT_context_open_discourse (struct GNUNET_CHAT_Context *context,
                                    const struct GNUNET_CHAT_DiscourseId *id)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!context) || (!(context->discourses)) || (!(context->room)) || (!id))
    return NULL;

  struct GNUNET_ShortHashCode sid;
  util_shorthash_from_discourse_id(id, &sid);

  struct GNUNET_CHAT_Discourse *discourse = GNUNET_CONTAINER_multishortmap_get(
    context->discourses, &sid
  );

  if (!discourse)
  {
    discourse = discourse_create(context, id);

    if (GNUNET_OK != GNUNET_CONTAINER_multishortmap_put(context->discourses,
        &sid, discourse, GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    {
      discourse_destroy(discourse);
      return NULL;
    }
  }

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_SUBSCRIBTION;
  GNUNET_memcpy(
    &(msg.body.subscription.discourse),
    &sid,
    sizeof(struct GNUNET_ShortHashCode)
  );

  const struct GNUNET_TIME_Relative subscription_time = GNUNET_TIME_relative_multiply(
    GNUNET_TIME_relative_get_second_(), 10
  );

  msg.body.subscription.time = GNUNET_TIME_relative_hton(subscription_time);
  msg.body.subscription.flags = GNUNET_MESSENGER_FLAG_SUBSCRIPTION_KEEP_ALIVE;

  GNUNET_MESSENGER_send_message(
    context->room,
    &msg,
    NULL
  );

  return discourse;
}


int
GNUNET_CHAT_context_iterate_messages (struct GNUNET_CHAT_Context *context,
                                      GNUNET_CHAT_ContextMessageCallback callback,
                                      void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!context)
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_ContextIterateMessages it;
  it.context = context;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_CONTAINER_multihashmap_iterate(
    context->messages, it_context_iterate_messages, &it
  );
}


int
GNUNET_CHAT_context_iterate_files (struct GNUNET_CHAT_Context *context,
                                   GNUNET_CHAT_ContextFileCallback callback,
                                   void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!context)
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_ContextIterateFiles it;
  it.context = context;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_CONTAINER_multihashmap_iterate(
    context->files, it_context_iterate_files, &it
  );
}


enum GNUNET_CHAT_MessageKind
GNUNET_CHAT_message_get_kind (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return GNUNET_CHAT_KIND_UNKNOWN;

  switch (message->flag)
  {
    case GNUNET_CHAT_FLAG_WARNING:
      return GNUNET_CHAT_KIND_WARNING;
    case GNUNET_CHAT_FLAG_REFRESH:
      return GNUNET_CHAT_KIND_REFRESH;
    case GNUNET_CHAT_FLAG_LOGIN:
      return GNUNET_CHAT_KIND_LOGIN;
    case GNUNET_CHAT_FLAG_LOGOUT:
      return GNUNET_CHAT_KIND_LOGOUT;
    case GNUNET_CHAT_FLAG_CREATE_ACCOUNT:
      return GNUNET_CHAT_KIND_CREATED_ACCOUNT;
    case GNUNET_CHAT_FLAG_DELETE_ACCOUNT:
      return GNUNET_CHAT_KIND_DELETED_ACCOUNT;
    case GNUNET_CHAT_FLAG_UPDATE_ACCOUNT:
      return GNUNET_CHAT_KIND_UPDATE_ACCOUNT;
    case GNUNET_CHAT_FLAG_UPDATE_CONTEXT:
      return GNUNET_CHAT_KIND_UPDATE_CONTEXT;
    case GNUNET_CHAT_FLAG_ATTRIBUTES:
      return GNUNET_CHAT_KIND_ATTRIBUTES;
    case GNUNET_CHAT_FLAG_SHARE_ATTRIBUTES:
      return GNUNET_CHAT_KIND_SHARED_ATTRIBUTES;
    default:
      break;
  }

  if (GNUNET_YES != message_has_msg(message))
    return GNUNET_CHAT_KIND_UNKNOWN;

  return util_message_kind_from_kind(message->msg->header.kind);
}


time_t
GNUNET_CHAT_message_get_timestamp (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)))
    return ((time_t) -1);

  struct GNUNET_TIME_Absolute abs = GNUNET_TIME_absolute_ntoh(
    message->msg->header.timestamp
  );

  struct GNUNET_TIME_Timestamp ts = GNUNET_TIME_absolute_to_timestamp(
    abs
  );

  return (time_t) GNUNET_TIME_timestamp_to_s(ts);
}


struct GNUNET_CHAT_Contact*
GNUNET_CHAT_message_get_sender (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)) || (!(message->context->room)))
    return NULL;

  const struct GNUNET_MESSENGER_Contact *sender = GNUNET_MESSENGER_get_sender(
    message->context->room, &(message->hash)
  );

  if (!sender)
    return NULL;

  return handle_get_contact_from_messenger(message->context->handle, sender);
}


struct GNUNET_CHAT_Contact*
GNUNET_CHAT_message_get_recipient (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)) || (!(message->context->room)))
    return NULL;

  const struct GNUNET_MESSENGER_Contact *recipient = GNUNET_MESSENGER_get_recipient(
    message->context->room, &(message->hash)
  );

  if (!recipient)
    return NULL;

  return handle_get_contact_from_messenger(message->context->handle, recipient);
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_sent (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return GNUNET_SYSERR;

  if (message->flags & GNUNET_MESSENGER_FLAG_SENT)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_private (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return GNUNET_SYSERR;

  if (message->flags & GNUNET_MESSENGER_FLAG_PRIVATE)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_recent (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return GNUNET_SYSERR;

  if (message->flags & GNUNET_MESSENGER_FLAG_RECENT)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_update (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return GNUNET_SYSERR;

  if (message->flags & GNUNET_MESSENGER_FLAG_UPDATE)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_deleted (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return GNUNET_SYSERR;

  if ((GNUNET_CHAT_FLAG_NONE == message->flag) &&
      ((message->flags & GNUNET_MESSENGER_FLAG_DELETE) ||
       (!message->msg)))
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_tagged (const struct GNUNET_CHAT_Message *message,
                               const char *tag)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (!(message->context)))
    return GNUNET_SYSERR;

  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    message->context->taggings, &(message->hash));

  if (!tagging)
    return GNUNET_NO;

  if (internal_tagging_iterate(tagging, GNUNET_NO, tag, NULL, NULL) > 0)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


int
GNUNET_CHAT_message_get_read_receipt (struct GNUNET_CHAT_Message *message,
                                      GNUNET_CHAT_MessageReadReceiptCallback callback,
                                      void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)))
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_MessageIterateReadReceipts it;
  it.message = message;
  it.cb = callback;
  it.cls = cls;

  return GNUNET_MESSENGER_iterate_members(
    message->context->room, it_message_iterate_read_receipts, &it
  );
}


const char*
GNUNET_CHAT_message_get_text (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return NULL;

  if (GNUNET_CHAT_FLAG_WARNING == message->flag)
    return message->warning;
  else if (GNUNET_CHAT_FLAG_UPDATE_ACCOUNT == message->flag)
    return message->warning;
  else if (GNUNET_CHAT_FLAG_ATTRIBUTES == message->flag)
    return message->attr;

  if (GNUNET_YES != message_has_msg(message))
    return NULL;

  if (GNUNET_MESSENGER_KIND_TEXT == message->msg->header.kind)
    return message->msg->body.text.text;
  else if (GNUNET_MESSENGER_KIND_FILE == message->msg->header.kind)
    return message->msg->body.file.name;
  else if (GNUNET_MESSENGER_KIND_TAG == message->msg->header.kind)
    return message->msg->body.tag.tag;
  else
    return NULL;
}


void
GNUNET_CHAT_message_set_user_pointer (struct GNUNET_CHAT_Message *message,
                                      void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return;

  message->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_message_get_user_pointer (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return NULL;

  return message->user_pointer;
}


struct GNUNET_CHAT_Account*
GNUNET_CHAT_message_get_account (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!message)
    return NULL;

  if ((message->context) && (message->context->handle))
    return message->context->handle->current;
  else
    return message->account;
}


struct GNUNET_CHAT_File*
GNUNET_CHAT_message_get_file (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)))
    return NULL;

  if (GNUNET_MESSENGER_KIND_FILE != message->msg->header.kind)
    return NULL;

  return GNUNET_CONTAINER_multihashmap_get(
    message->context->handle->files,
    &(message->msg->body.file.hash)
  );
}


struct GNUNET_CHAT_Invitation*
GNUNET_CHAT_message_get_invitation (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)))
    return NULL;

  if (GNUNET_MESSENGER_KIND_INVITE != message->msg->header.kind)
    return NULL;

  return GNUNET_CONTAINER_multihashmap_get(
    message->context->invites,
    &(message->hash)
  );
}


struct GNUNET_CHAT_Discourse*
GNUNET_CHAT_message_get_discourse (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)) || (!(message->context->discourses)))
    return NULL;

  struct GNUNET_CHAT_Discourse *discourse;

  if (GNUNET_MESSENGER_KIND_SUBSCRIBTION == message->msg->header.kind)
    discourse = GNUNET_CONTAINER_multishortmap_get(
      message->context->discourses,
      &(message->msg->body.subscription.discourse));
  else if (GNUNET_MESSENGER_KIND_TALK == message->msg->header.kind)
    discourse = GNUNET_CONTAINER_multishortmap_get(
      message->context->discourses,
      &(message->msg->body.talk.discourse));
  else
    discourse = NULL;

  return discourse;
}


struct GNUNET_CHAT_Message*
GNUNET_CHAT_message_get_target (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)))
    return NULL;

  struct GNUNET_CHAT_Message *target;

  if (GNUNET_MESSENGER_KIND_DELETION == message->msg->header.kind)
    target = GNUNET_CONTAINER_multihashmap_get(
	message->context->messages, &(message->msg->body.deletion.hash));
  else if (GNUNET_MESSENGER_KIND_TAG == message->msg->header.kind)
    target = GNUNET_CONTAINER_multihashmap_get(
      message->context->messages, &(message->msg->body.tag.hash));
  else
    target = NULL;

  return target;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_delete (struct GNUNET_CHAT_Message *message,
			                      unsigned int delay)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (!(message->context)))
    return GNUNET_SYSERR;

  struct GNUNET_TIME_Relative rel = GNUNET_TIME_relative_multiply(
    GNUNET_TIME_relative_get_second_(), delay
  );

  GNUNET_MESSENGER_delete_message(
    message->context->room,
    &(message->hash),
    rel
  );

  return GNUNET_OK;
}


int
GNUNET_CHAT_message_iterate_tags (struct GNUNET_CHAT_Message *message,
                                  GNUNET_CHAT_MessageCallback callback,
                                  void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (!(message->context)))
    return GNUNET_SYSERR;

  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    message->context->taggings, &(message->hash));

  if (!tagging)
    return 0;

  return internal_tagging_iterate(tagging, GNUNET_YES, NULL, callback, cls);
}


uint64_t
GNUNET_CHAT_message_available (const struct GNUNET_CHAT_Message *message)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)))
    return 0;

  if (GNUNET_MESSENGER_KIND_TALK == message->msg->header.kind)
    return message->msg->body.talk.length;
  else
    return 0;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_read (const struct GNUNET_CHAT_Message *message,
                          char *data,
                          uint64_t size)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)))
    return GNUNET_SYSERR;

  if (GNUNET_MESSENGER_KIND_TALK != message->msg->header.kind)
    return GNUNET_SYSERR;

  const uint64_t available = message->msg->body.talk.length;

  if (available < size)
    return GNUNET_NO;

  GNUNET_memcpy(
    data,
    message->msg->body.talk.data,
    size
  );

  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_feed (const struct GNUNET_CHAT_Message *message,
                          int fd)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!message) || (GNUNET_YES != message_has_msg(message)) ||
      (fd == -1))
    return GNUNET_SYSERR;

  if (GNUNET_MESSENGER_KIND_TALK != message->msg->header.kind)
    return GNUNET_SYSERR;

  if (!(message->msg->body.talk.length))
    return GNUNET_NO;

  const ssize_t written = write(
    fd,
    message->msg->body.talk.data,
    message->msg->body.talk.length
  );

  if (-1 == written)
    return GNUNET_SYSERR;
  else if (written != message->msg->body.talk.length)
    return GNUNET_NO;
  else
    return GNUNET_OK;
}


const char*
GNUNET_CHAT_file_get_name (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return NULL;

  return file->name;
}


const char*
GNUNET_CHAT_file_get_hash (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return NULL;

  return GNUNET_h2s_full(&(file->hash));
}


uint64_t
GNUNET_CHAT_file_get_size (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (!(file->uri)))
    return 0;

  return GNUNET_FS_uri_chk_get_file_size(file->uri);
}


uint64_t
GNUNET_CHAT_file_get_local_size (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return 0;

  char *filename = handle_create_file_path(
    file->handle, &(file->hash)
  );

  if (!filename)
    return 0;

  uint64_t size;
  if (GNUNET_OK != GNUNET_DISK_file_size(filename, &size, GNUNET_NO, GNUNET_YES))
    size = 0;

  GNUNET_free(filename);
  return size;
}


struct GNUNET_CHAT_Uri*
GNUNET_CHAT_file_get_uri (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (!(file->uri)))
    return NULL;

  return uri_create_file(file->uri);
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_uploading (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (0 == (file->status & GNUNET_CHAT_FILE_STATUS_PUBLISH)))
    return GNUNET_NO;
  else
    return GNUNET_YES;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_ready (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (file->status & GNUNET_CHAT_FILE_STATUS_MASK))
    return GNUNET_NO;

  const uint64_t size = GNUNET_CHAT_file_get_size(file);
  const uint64_t local_size = GNUNET_CHAT_file_get_local_size(file);

  if (size != local_size)
    return GNUNET_NO;
  else
    return GNUNET_YES;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_valid (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (file->status & GNUNET_CHAT_FILE_STATUS_MASK))
    return GNUNET_SYSERR;

  char *filename = handle_create_file_path(
    file->handle, &(file->hash)
  );

  if (!filename)
    return GNUNET_SYSERR;

  struct GNUNET_HashCode hash;
  enum GNUNET_GenericReturnValue result;

  if (GNUNET_OK == util_hash_file(filename, &hash))
  {
    if (0 == GNUNET_CRYPTO_hash_cmp(&hash, &(file->hash)))
      result = GNUNET_YES;
    else
      result = GNUNET_NO;
  }
  else
    result = GNUNET_SYSERR;

  GNUNET_free(filename);
  return result;
}


const char*
GNUNET_CHAT_file_open_preview (struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return NULL;

  if (file->preview)
    return file->preview;

  char *filename = handle_create_file_path(
    file->handle, &(file->hash)
  );

  if (!filename)
    return NULL;

  if (GNUNET_YES != GNUNET_DISK_file_test(filename))
  {
    GNUNET_free(filename);
    filename = NULL;
  }

  file->preview = filename;
  return file->preview;
}


void
GNUNET_CHAT_file_close_preview (struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (!(file->preview)))
    return;

  GNUNET_free(file->preview);
  file->preview = NULL;
}


void
GNUNET_CHAT_file_set_user_pointer (struct GNUNET_CHAT_File *file,
				                           void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return;

  file->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_file_get_user_pointer (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return NULL;

  return file->user_pointer;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_downloading (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (0 == (file->status & GNUNET_CHAT_FILE_STATUS_DOWNLOAD)))
    return GNUNET_NO;
  else
    return GNUNET_YES;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_start_download (struct GNUNET_CHAT_File *file,
                                 GNUNET_CHAT_FileDownloadCallback callback,
                                 void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (!(file->uri)))
    return GNUNET_SYSERR;

  if (file->download)
  {
    file_bind_downlaod(file, callback, cls);

    GNUNET_FS_download_resume(file->download);
    return GNUNET_OK;
  }

  char *filename = handle_create_file_path(
    file->handle, &(file->hash)
  );

  if (!filename)
    return GNUNET_SYSERR;

  const uint64_t size = GNUNET_FS_uri_chk_get_file_size(file->uri);

  uint64_t offset;
  if (GNUNET_OK != GNUNET_DISK_file_size(filename, &offset,
      GNUNET_NO, GNUNET_YES))
    offset = 0;

  if (offset >= size)
  {
    if (callback)
      callback(cls, file, size, size);

    goto free_filename;
  }

  file_bind_downlaod(file, callback, cls);

  const uint64_t remaining = (size - offset);

  file->download = GNUNET_FS_download_start(
    file->handle->fs,
    file->uri,
    file->meta,
    filename,
    NULL,
    offset,
    remaining,
    1,
    GNUNET_FS_DOWNLOAD_OPTION_NONE,
    file,
    NULL
  );

  if (file->download)
    file->status |= GNUNET_CHAT_FILE_STATUS_DOWNLOAD;

free_filename:
  GNUNET_free(filename);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_pause_download (struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return GNUNET_SYSERR;

  GNUNET_FS_download_suspend(file->download);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_resume_download (struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return GNUNET_SYSERR;

  GNUNET_FS_download_resume(file->download);
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_stop_download (struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return GNUNET_SYSERR;

  GNUNET_FS_download_stop(file->download, GNUNET_YES);
  file->download = NULL;
  return GNUNET_OK;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_unindexing (const struct GNUNET_CHAT_File *file)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!file) || (0 == (file->status & GNUNET_CHAT_FILE_STATUS_UNINDEX)))
    return GNUNET_NO;
  else
    return GNUNET_YES;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_unindex (struct GNUNET_CHAT_File *file,
                          GNUNET_CHAT_FileUnindexCallback callback,
                          void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!file)
    return GNUNET_SYSERR;

  if (file->publish)
  {
    GNUNET_FS_publish_stop(file->publish);
    file->publish = NULL;
    return GNUNET_OK;
  }

  file_bind_unindex(file, callback, cls);

  if (file->unindex)
    return GNUNET_OK;

  char *filename = handle_create_file_path(
    file->handle, &(file->hash)
  );

  if (!filename)
    return GNUNET_SYSERR;

  file->unindex = GNUNET_FS_unindex_start(
    file->handle->fs, filename, file
  );

  if (file->unindex)
    file->status |= GNUNET_CHAT_FILE_STATUS_UNINDEX;

  GNUNET_free(filename);
  return GNUNET_OK;
}


void
GNUNET_CHAT_invitation_accept (struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!invitation)
    return;

  struct GNUNET_CHAT_Handle *handle;
  handle = invitation->context->handle;

  if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(
      handle->contexts, &(invitation->key.hash)))
    return;

  struct GNUNET_PeerIdentity door;
  GNUNET_PEER_resolve(invitation->door, &door);

  struct GNUNET_MESSENGER_Room *room;
  room = GNUNET_MESSENGER_enter_room(
    invitation->context->handle->messenger,
    &door, &(invitation->key)
  );

  if (!room)
    return;

  struct GNUNET_CHAT_Context *context;
  context = context_create_from_room(handle, room);

  if (!context)
    return;

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->contexts, &(invitation->key.hash), context,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    goto destroy_context;

  if (GNUNET_CHAT_CONTEXT_TYPE_GROUP != context->type)
  {
    context_write_records(context);
    return;
  }

  struct GNUNET_CHAT_Group *group;
  group = group_create_from_context(handle, context);

  if (GNUNET_OK == GNUNET_CONTAINER_multihashmap_put(
      handle->groups, &(invitation->key.hash), group,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    context_write_records(context);
    return;
  }

  group_destroy(group);

  GNUNET_CONTAINER_multihashmap_remove(
    handle->contexts, &(invitation->key.hash), context);

destroy_context:
  context_destroy(context);
}


void
GNUNET_CHAT_invitation_reject (struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!invitation)
    return;

  const struct GNUNET_MESSENGER_Contact *sender = GNUNET_MESSENGER_get_sender(
    invitation->context->room, &(invitation->hash)
  );

  if (!sender)
    return;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TAG;
  GNUNET_memcpy(&(msg.body.tag.hash), &(invitation->hash),
                sizeof(struct GNUNET_HashCode));
  msg.body.tag.tag = NULL;

  GNUNET_MESSENGER_send_message(invitation->context->room, &msg, sender);
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_invitation_is_accepted (const struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!invitation)
    return GNUNET_NO;

  return GNUNET_CONTAINER_multihashmap_contains(
    invitation->context->handle->contexts,
    &(invitation->key.hash)
  );
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_invitation_is_rejected (const struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!invitation)
    return GNUNET_NO;

  const struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
    invitation->context->taggings, &(invitation->hash));

  if (!tagging)
    return GNUNET_NO;

  if (internal_tagging_iterate(tagging, GNUNET_NO, NULL, NULL, NULL) > 0)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_invitation_is_direct (const struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((invitation->key.code.public_bit) ||
      (invitation->key.code.group_bit) ||
      (invitation->key.code.feed_bit))
    return GNUNET_NO;
  else
    return GNUNET_YES;
}


const struct GNUNET_CHAT_DiscourseId*
GNUNET_CHAT_discourse_get_id (const struct GNUNET_CHAT_Discourse *discourse)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!discourse)
    return NULL;

  return &(discourse->id);
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_discourse_is_open (const struct GNUNET_CHAT_Discourse *discourse)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!discourse)
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_DiscourseSubscription *sub;
  for (sub = discourse->head; sub; sub = sub->next)
  {
    if (GNUNET_TIME_absolute_cmp(sub->end, <, GNUNET_TIME_absolute_get()))
      continue;

    if (GNUNET_YES == sub->contact->owned)
      return GNUNET_YES;
  }

  return GNUNET_NO;
}


void
GNUNET_CHAT_discourse_set_user_pointer (struct GNUNET_CHAT_Discourse *discourse,
                                        void *user_pointer)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!discourse)
    return;

  discourse->user_pointer = user_pointer;
}


void*
GNUNET_CHAT_discourse_get_user_pointer (const struct GNUNET_CHAT_Discourse *discourse)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (!discourse)
    return NULL;

  return discourse->user_pointer;
}


void
GNUNET_CHAT_discourse_close (struct GNUNET_CHAT_Discourse *discourse)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!discourse) || (!(discourse->context)) || (!(discourse->context->room)))
    return;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_SUBSCRIBTION;

  util_shorthash_from_discourse_id(
    &(discourse->id),
    &(msg.body.subscription.discourse)
  );

  msg.body.subscription.time = GNUNET_TIME_relative_hton(GNUNET_TIME_relative_get_zero_());
  msg.body.subscription.flags = GNUNET_MESSENGER_FLAG_SUBSCRIPTION_UNSUBSCRIBE;

  GNUNET_MESSENGER_send_message(
    discourse->context->room,
    &msg,
    NULL
  );
}


enum GNUNET_GenericReturnValue
GNUNET_CHAT_discourse_write (struct GNUNET_CHAT_Discourse *discourse,
                             const char *data,
                             uint64_t size)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if ((!discourse) || (!data) || (!(discourse->context)) ||
      (!(discourse->context->room)))
    return GNUNET_SYSERR;

  static const uint64_t max_size = (uint16_t) (
    GNUNET_MAX_MESSAGE_SIZE - GNUNET_MIN_MESSAGE_SIZE -
    sizeof (struct GNUNET_MESSENGER_Message)
  );

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_TALK;
  msg.body.talk.data = GNUNET_malloc(size > max_size? max_size : size);

  util_shorthash_from_discourse_id(
    &(discourse->id),
    &(msg.body.talk.discourse)
  );

  while (size > 0)
  {
    msg.body.talk.length = (uint16_t) (size > max_size? max_size : size);

    GNUNET_memcpy(
      msg.body.talk.data,
      data,
      msg.body.talk.length
    );

    size -= msg.body.talk.length;
    data += msg.body.talk.length;

    GNUNET_MESSENGER_send_message(discourse->context->room, &msg, NULL);
  }

  GNUNET_free(msg.body.talk.data);
  return GNUNET_OK;
}


int
GNUNET_CHAT_discourse_get_fd (const struct GNUNET_CHAT_Discourse *discourse)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (! discourse)
    return GNUNET_SYSERR;

  return discourse->pipe[1];
}


int
GNUNET_CHAT_discourse_iterate_contacts (struct GNUNET_CHAT_Discourse *discourse,
                                        GNUNET_CHAT_DiscourseContactCallback callback,
                                        void *cls)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (! discourse)
    return GNUNET_SYSERR;

  int iterations = 0;

  struct GNUNET_CHAT_DiscourseSubscription *sub;
  for (sub = discourse->head; sub; sub = sub->next)
  {
    if (GNUNET_TIME_absolute_cmp(sub->end, <, GNUNET_TIME_absolute_get()))
      continue;

    if (callback)
      callback(cls, discourse, sub->contact);

    iterations++;
  }

  return iterations;
}

enum GNUNET_GenericReturnValue
GNUNET_CHAT_generate_secret (char *secret,
                             uint32_t secret_len)
{
  GNUNET_CHAT_VERSION_ASSERT();

  if (secret_len <= 0)
    return GNUNET_SYSERR;

  const uint32_t requested = secret_len * 5 / 8 + 1;
  const uint32_t size = ((requested*8) + (((requested*8) % 5) > 0 ? 5 - ((requested*8) % 5) : 0)) / 5;

  char *raw_secret = GNUNET_malloc(requested);
  char *buf;

  if (!raw_secret)
    return GNUNET_SYSERR;

  if (size > secret_len)
  {
    buf = GNUNET_malloc(size);

    if (!buf)
    {
      GNUNET_free(raw_secret);
      return GNUNET_SYSERR;
    }
  }
  else
    buf = secret;

  GNUNET_CRYPTO_random_block(
    raw_secret,
    requested
  );

  enum GNUNET_GenericReturnValue result;
  result = GNUNET_STRINGS_data_to_string(
    raw_secret,
    requested,
    buf,
    size
  ) == NULL? GNUNET_SYSERR : GNUNET_OK;

  GNUNET_CRYPTO_zero_keys(raw_secret, requested);
  GNUNET_free(raw_secret);

  if (buf != secret)
  {
    GNUNET_memcpy(secret, buf, secret_len);
    GNUNET_CRYPTO_zero_keys(buf, size);
    GNUNET_free(buf);
  }

  return result;
}
