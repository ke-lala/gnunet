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
 * @file gnunet_chat_handle.c
 */

#include "gnunet_chat_handle.h"

#include "gnunet_chat_account.h"
#include "gnunet_chat_handle_intern.c"
#include "gnunet_chat_message.h"
#include <gnunet/gnunet_arm_service.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_pils_service.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_util_lib.h>

static const unsigned int initial_map_size_of_handle = 8;
static const unsigned int minimum_amount_of_other_members_in_group = 2;

struct GNUNET_CHAT_Handle*
handle_create_from_config (const struct GNUNET_CONFIGURATION_Handle* cfg,
                           GNUNET_CHAT_ContextMessageCallback msg_cb,
                           void *msg_cls)
{
  GNUNET_assert(cfg);

  struct GNUNET_CHAT_Handle* handle = GNUNET_new(struct GNUNET_CHAT_Handle);

  handle->cfg = cfg;
  handle->shutdown_hook = GNUNET_SCHEDULER_add_shutdown(
    on_handle_shutdown, handle
  );

  handle->destruction = NULL;

  handle->services_head = NULL;
  handle->services_tail = NULL;

  handle->internal_head = NULL;
  handle->internal_tail = NULL;

  handle->directory = NULL;

  char *dir_path = NULL;
  if (GNUNET_OK != GNUNET_CONFIGURATION_get_value_filename(cfg,
		     GNUNET_MESSENGER_SERVICE_NAME,
		     "MESSENGER_DIR",
		     &dir_path))
  {
    if (dir_path)
      GNUNET_free(dir_path);
  }
  else if ((GNUNET_YES != GNUNET_DISK_directory_test(dir_path, GNUNET_YES)) &&
	         (GNUNET_OK != GNUNET_DISK_directory_create(dir_path)))
  {
    GNUNET_free(dir_path);
  }
  else
  {
    char *chat_directory = NULL;
    util_get_dirname(dir_path, "chat", &chat_directory);
    GNUNET_free(dir_path);

    if ((GNUNET_YES != GNUNET_DISK_directory_test(chat_directory, GNUNET_YES)) &&
    	  (GNUNET_OK != GNUNET_DISK_directory_create(chat_directory)))
      GNUNET_free(chat_directory);
    else
      handle->directory = chat_directory;
  }

  handle->msg_cb = msg_cb;
  handle->msg_cls = msg_cls;

  handle->accounts_head = NULL;
  handle->accounts_tail = NULL;

  handle->refreshing = GNUNET_NO;
  handle->own_contact = NULL;

  handle->next = NULL;
  handle->next_secret = NULL;
  handle->current = NULL;
  handle->monitor = NULL;

  handle->lobbies_head = NULL;
  handle->lobbies_tail = NULL;

  handle->lookups_head = NULL;
  handle->lookups_tail = NULL;

  handle->attributes_head = NULL;
  handle->attributes_tail = NULL;

  handle->tickets_head = NULL;
  handle->tickets_tail = NULL;

  handle->files = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size_of_handle, GNUNET_NO);
  
  handle->contexts = NULL;
  handle->contacts = NULL;
  handle->groups = NULL;
  handle->invitations = NULL;

  handle->arm = GNUNET_ARM_connect(
    handle->cfg,
    on_handle_arm_connection, 
    handle
  );

  if (handle->arm)
    on_handle_arm_connection(handle, GNUNET_NO);

  handle->identity = GNUNET_IDENTITY_connect(
    handle->cfg,
    on_handle_gnunet_identity,
    handle
  );

  handle->fs = GNUNET_FS_start(
    handle->cfg, "libgnunetchat",
    notify_handle_fs_progress, handle,
    GNUNET_FS_FLAGS_NONE,
    GNUNET_FS_OPTIONS_END
  );

  handle->gns = NULL;
  handle->messenger = NULL;

  handle->namestore = GNUNET_NAMESTORE_connect(
    handle->cfg
  );

  handle->pils = GNUNET_PILS_connect(
    handle->cfg,
    on_pils_identity_changed,
    handle
  );

  handle->reclaim = GNUNET_RECLAIM_connect(
    handle->cfg
  );

  handle->pid = NULL;
  handle->public_key = NULL;
  handle->user_pointer = NULL;
  return handle;
}

void
handle_update_key (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(handle);

  if (handle->public_key)
    GNUNET_free(handle->public_key);

  handle->public_key = NULL;
  handle->own_contact = NULL;

  if (!(handle->messenger))
    return;

  const struct GNUNET_CRYPTO_BlindablePublicKey *pubkey;
  pubkey = GNUNET_MESSENGER_get_key(handle->messenger);

  if (pubkey)
    handle->public_key = GNUNET_CRYPTO_blindable_public_key_to_string(pubkey);
}

void
handle_destroy (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(handle);

  if (handle->shutdown_hook)
    GNUNET_SCHEDULER_cancel(handle->shutdown_hook);
  if (handle->destruction)
    GNUNET_SCHEDULER_cancel(handle->destruction);
  if (handle->connection)
    GNUNET_SCHEDULER_cancel(handle->connection);
  if (handle->refresh)
    GNUNET_SCHEDULER_cancel(handle->refresh);

  if (handle->monitor)
    GNUNET_NAMESTORE_zone_monitor_stop(handle->monitor);

  handle->connection = NULL;

  if (handle->current)
    handle_disconnect(handle);

  if (handle->next_secret)
  {
    GNUNET_CRYPTO_zero_keys(
      handle->next_secret,
      sizeof(*(handle->next_secret))
    );

    GNUNET_free(handle->next_secret);
    handle->next_secret = NULL;
  }

  GNUNET_CONTAINER_multihashmap_iterate(
    handle->files, it_destroy_handle_files, NULL
  );

  while (handle->attributes_head)
    internal_attributes_destroy(handle->attributes_head);

  if (handle->reclaim)
    GNUNET_RECLAIM_disconnect(handle->reclaim);

  if (handle->pils)
    GNUNET_PILS_disconnect(handle->pils);

  if (handle->pid)
  {
    GNUNET_free(handle->pid);
    handle->pid = NULL;
  }

  if (handle->namestore)
    GNUNET_NAMESTORE_disconnect(handle->namestore);

  struct GNUNET_CHAT_InternalAccounts *accounts;
  while (handle->accounts_head)
  {
    accounts = handle->accounts_head;

    internal_accounts_stop_method(accounts);

    if (accounts->account)
      account_destroy(accounts->account);

    internal_accounts_destroy(accounts);
  }

  if (handle->fs)
    GNUNET_FS_stop(handle->fs);

  if (handle->identity)
    GNUNET_IDENTITY_disconnect(handle->identity);

  struct GNUNET_CHAT_InternalServices *services;
  while (handle->services_head)
  {
    services = handle->services_head;

    if (services->op)
      GNUNET_ARM_operation_cancel(services->op);

    GNUNET_CONTAINER_DLL_remove(
      handle->services_head,
      handle->services_tail,
      services
    );

    GNUNET_free(services);
  }

  if (handle->arm)
    GNUNET_ARM_disconnect(handle->arm);

  GNUNET_CONTAINER_multihashmap_destroy(handle->files);

  if (handle->directory)
    GNUNET_free(handle->directory);

  struct GNUNET_CHAT_InternalMessages *internal;
  while (handle->internal_head)
  {
    internal = handle->internal_head;

    if (internal->msg)
      message_destroy(internal->msg);

    if (internal->task)
      GNUNET_SCHEDULER_cancel(internal->task);

    GNUNET_CONTAINER_DLL_remove(
      handle->internal_head,
      handle->internal_tail,
      internal
    );

    GNUNET_free(internal);
  }

  GNUNET_free(handle);
}

static void
handle_update_identity(struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(
    (handle) &&
    (handle->current) &&
		(handle->contexts) &&
		(handle->groups) &&
		(handle->contacts)
  );

  handle_update_key(handle);

  if ((0 < GNUNET_CONTAINER_multihashmap_size(handle->contexts)) ||
      (0 < GNUNET_CONTAINER_multihashmap_size(handle->groups)) ||
      (0 < GNUNET_CONTAINER_multishortmap_size(handle->contacts)))
    return;

  GNUNET_assert(handle->messenger);

  handle_send_internal_message(
    handle,
    handle->current,
    NULL,
    GNUNET_CHAT_FLAG_LOGIN,
    NULL,
    GNUNET_NO
  );

  const struct GNUNET_CRYPTO_BlindablePrivateKey *zone = handle_get_key(handle);

  if ((!zone) || (handle->monitor))
    return;

  handle->monitor = GNUNET_NAMESTORE_zone_monitor_start(
    handle->cfg,
    zone,
    GNUNET_YES,
    NULL,
    NULL,
    on_monitor_namestore_record,
    handle,
    NULL,
    NULL
  );
}

void
handle_connect (struct GNUNET_CHAT_Handle *handle,
		            struct GNUNET_CHAT_Account *account,
                const struct GNUNET_HashCode *secret)
{
  GNUNET_assert(
    (handle) && (account) &&
		(!(handle->current)) &&
		(!(handle->contexts)) &&
    (!(handle->contacts)) &&
    (!(handle->groups)) &&
    (!(handle->invitations)) &&
		(handle->files)
  );

  if (handle->monitor)
  {
    GNUNET_NAMESTORE_zone_monitor_stop(handle->monitor);
    handle->monitor = NULL;
  }

  handle->contexts = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size_of_handle, GNUNET_NO);
  handle->contacts = GNUNET_CONTAINER_multishortmap_create(
    initial_map_size_of_handle, GNUNET_NO);
  handle->groups = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size_of_handle, GNUNET_NO);
  handle->invitations = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size_of_handle, GNUNET_NO);

  handle->gns = GNUNET_GNS_connect(handle->cfg);

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key;
  key = account_get_key(account);

  const char *name = account_get_name(account);

  handle->messenger = GNUNET_MESSENGER_connect(
    handle->cfg, name, key, secret,
    on_handle_message,
    handle
  );

  handle->next = NULL;
  handle->current = account;
  handle_update_identity(handle);
}

void
handle_disconnect (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(
    (handle) &&
		(handle->current) &&
		(handle->groups) &&
		(handle->contacts) &&
		(handle->contexts) &&
		(handle->files)
  );

  handle_send_internal_message(
    handle,
    handle->current,
    NULL,
    GNUNET_CHAT_FLAG_LOGOUT,
    NULL,
    GNUNET_YES
  );

  handle->own_contact = NULL;

  while (handle->attributes_head)
    internal_attributes_destroy(handle->attributes_head);

  while (handle->tickets_head)
    internal_tickets_destroy(handle->tickets_head);

  GNUNET_CONTAINER_multihashmap_iterate(
    handle->groups, it_destroy_handle_groups, NULL
  );

  GNUNET_CONTAINER_multishortmap_iterate(
    handle->contacts, it_destroy_handle_contacts, NULL
  );

  GNUNET_CONTAINER_multihashmap_iterate(
    handle->contexts, it_destroy_handle_contexts, NULL
  );

  struct GNUNET_CHAT_InternalMessages *internal;
  while (handle->internal_head)
  {
    internal = handle->internal_head;

    if (!(internal->msg->context))
      break;

    if (internal->msg)
      message_destroy(internal->msg);

    if (internal->task)
      GNUNET_SCHEDULER_cancel(internal->task);

    GNUNET_CONTAINER_DLL_remove(
      handle->internal_head,
      handle->internal_tail,
      internal
    );

    GNUNET_free(internal);
  }

  if (handle->messenger)
    GNUNET_MESSENGER_disconnect(handle->messenger);

  struct GNUNET_CHAT_UriLookups *lookups;
  while (handle->lookups_head)
  {
    lookups = handle->lookups_head;

    if (lookups->request)
      GNUNET_GNS_lookup_cancel(lookups->request);

    if (lookups->uri)
      uri_destroy(lookups->uri);

    GNUNET_CONTAINER_DLL_remove(
      handle->lookups_head,
      handle->lookups_tail,
      lookups
    );

    GNUNET_free(lookups);
  }

  if (handle->gns)
    GNUNET_GNS_disconnect(handle->gns);

  GNUNET_CONTAINER_multihashmap_iterate(
    handle->files, it_destroy_handle_files, NULL
  );

  handle->gns = NULL;
  handle->messenger = NULL;

  struct GNUNET_CHAT_InternalLobbies *lobbies;
  while (handle->lobbies_head)
  {
    lobbies = handle->lobbies_head;

    if (lobbies->lobby)
      lobby_destroy(lobbies->lobby);

    GNUNET_CONTAINER_DLL_remove(
      handle->lobbies_head,
      handle->lobbies_tail,
      lobbies
    );

    GNUNET_free(lobbies);
  }

  handle->own_contact = NULL;

  GNUNET_CONTAINER_multihashmap_destroy(handle->invitations);
  GNUNET_CONTAINER_multihashmap_destroy(handle->groups);
  GNUNET_CONTAINER_multishortmap_destroy(handle->contacts);
  GNUNET_CONTAINER_multihashmap_destroy(handle->contexts);
  GNUNET_CONTAINER_multihashmap_clear(handle->files);

  handle->contexts = NULL;
  handle->contacts = NULL;
  handle->groups = NULL;
  handle->invitations = NULL;

  if (handle->connection)
    GNUNET_SCHEDULER_cancel(handle->connection);

  handle->current = NULL;
  handle->connection = NULL;

  handle_update_key(handle);
}

static struct GNUNET_CHAT_InternalAccounts*
find_accounts_by_name (const struct GNUNET_CHAT_Handle *handle,
		                   const char *name,
                       enum GNUNET_GenericReturnValue skip_op)
{
  GNUNET_assert((handle) && (name));

  struct GNUNET_CHAT_InternalAccounts *accounts = handle->accounts_head;
  const char *account_name;

  while (accounts)
  {
    if ((!(accounts->account)) || ((GNUNET_YES == skip_op) &&
        (accounts->op)))
      goto skip_account;

    account_name = account_get_name(
      accounts->account
    );

    if ((account_name) && (0 == strcmp(account_name, name)))
      break;

  skip_account:
    accounts = accounts->next;
  }

  return accounts;
}

struct GNUNET_CHAT_Account*
handle_get_account_by_name (const struct GNUNET_CHAT_Handle *handle,
		                        const char *name,
                            enum GNUNET_GenericReturnValue skip_op)
{
  GNUNET_assert((handle) && (name));

  struct GNUNET_CHAT_InternalAccounts *accounts;
  accounts = find_accounts_by_name(handle, name, skip_op);

  if (!accounts)
    return NULL;

  return accounts->account;
}

static enum GNUNET_GenericReturnValue
update_accounts_operation (struct GNUNET_CHAT_InternalAccounts **out_accounts,
                           struct GNUNET_CHAT_Handle *handle,
                           const char *name,
                           enum GNUNET_CHAT_AccountMethod method)
{
  GNUNET_assert(handle);

  struct GNUNET_CHAT_InternalAccounts *accounts = *out_accounts;

  if (accounts)
    internal_accounts_stop_method(accounts);
  else
  {
    accounts = internal_accounts_create(handle, NULL);

    if (!accounts)
      return GNUNET_SYSERR;
  }

  internal_accounts_start_method(accounts, method, name);

  *out_accounts = accounts;

  return GNUNET_OK;
}

enum GNUNET_GenericReturnValue
handle_create_account (struct GNUNET_CHAT_Handle *handle,
		                   const char *name)
{
  GNUNET_assert((handle) && (name));

  struct GNUNET_CHAT_InternalAccounts *accounts;
  accounts = find_accounts_by_name(handle, name, GNUNET_NO);

  if (accounts)
    return GNUNET_SYSERR;

  enum GNUNET_GenericReturnValue result;
  result = update_accounts_operation(
    &accounts, 
    handle, 
    name, 
    GNUNET_CHAT_ACCOUNT_CREATION
  );

  if (GNUNET_OK != result)
    return result;

  accounts->op = GNUNET_IDENTITY_create(
    handle->identity,
    name,
    NULL,
    GNUNET_PUBLIC_KEY_TYPE_ECDSA,
    cb_account_creation,
    accounts
  );

  if (!accounts->op)
    return GNUNET_SYSERR;

  return result;
}

enum GNUNET_GenericReturnValue
handle_delete_account (struct GNUNET_CHAT_Handle *handle,
		                   const struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert((handle) && (account));

  struct GNUNET_CHAT_InternalAccounts *accounts;
  accounts = handle->accounts_head;

  while (accounts)
    if (account == accounts->account)
      break;
    else
      accounts = accounts->next;

  if (!accounts)
    return GNUNET_SYSERR;

  enum GNUNET_GenericReturnValue result;
  result = update_accounts_operation(
    &accounts, 
    handle, 
    NULL, 
    GNUNET_CHAT_ACCOUNT_DELETION
  );

  if (GNUNET_OK != result)
    return result;

  const char *name = account_get_name(account);

  accounts->op = GNUNET_IDENTITY_delete(
    handle->identity,
    name,
    cb_account_deletion,
    accounts
  );

  if (!accounts->op)
    return GNUNET_SYSERR;

  return result;
}

enum GNUNET_GenericReturnValue
handle_rename_account (struct GNUNET_CHAT_Handle *handle,
                       const struct GNUNET_CHAT_Account *account,
                       const char *new_name)
{
  GNUNET_assert((handle) && (account) && (new_name));

  struct GNUNET_CHAT_InternalAccounts *accounts;
  accounts = handle->accounts_head;

  while (accounts)
    if (account == accounts->account)
      break;
    else
      accounts = accounts->next;

  if (!accounts)
    return GNUNET_SYSERR;

  if (find_accounts_by_name(handle, new_name, GNUNET_NO))
    return GNUNET_SYSERR;

  const char *old_name = account_get_name(account);

  if (0 == strcmp(old_name, new_name))
    return GNUNET_OK;

  enum GNUNET_GenericReturnValue result;
  result = update_accounts_operation(
    &accounts, 
    handle, 
    NULL, 
    GNUNET_CHAT_ACCOUNT_RENAMING
  );

  if (GNUNET_OK != result)
    return result;

  accounts->op = GNUNET_IDENTITY_rename(
    handle->identity,
    old_name,
    new_name,
    cb_account_rename,
    accounts
  );

  if (!accounts->op)
    return GNUNET_SYSERR;

  return result;
}

enum GNUNET_GenericReturnValue
handle_delete_lobby (struct GNUNET_CHAT_Handle *handle,
                     const struct GNUNET_CHAT_Lobby *lobby)
{
  GNUNET_assert((handle) && (lobby));

  if (!(lobby->context))
    return GNUNET_SYSERR;

  const struct GNUNET_HashCode *key = GNUNET_MESSENGER_room_get_key(
    lobby->context->room
  );

  if (!key)
    return GNUNET_SYSERR;

  struct GNUNET_CHAT_InternalAccounts *accounts = NULL;
  enum GNUNET_GenericReturnValue result;
  result = update_accounts_operation(
    &accounts, 
    handle, 
    NULL, 
    GNUNET_CHAT_ACCOUNT_DELETION
  );

  if (GNUNET_OK != result)
    return result;

  char *name;
  util_lobby_name(key, &name);

  accounts->op = GNUNET_IDENTITY_delete(
    handle->identity,
    name,
    cb_lobby_deletion,
    accounts
  );

  GNUNET_free(name);

  if (!accounts->op)
    return GNUNET_SYSERR;

  return result;
}

const char*
handle_get_directory (const struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(handle);

  return handle->directory;
}

char*
handle_create_file_path (const struct GNUNET_CHAT_Handle *handle,
                         const struct GNUNET_HashCode *hash)
{
  GNUNET_assert((handle) && (hash));

  const char *directory = handle_get_directory(handle);

  if (!directory)
    return NULL;

  char *filename;
  util_get_filename (
    directory, "files", hash, &filename
  );

  return filename;
}

enum GNUNET_GenericReturnValue
handle_update (struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert((handle) && (handle->current));

  struct GNUNET_CHAT_InternalAccounts *accounts;
  accounts = handle->accounts_head;

  while (accounts)
    if (handle->current == accounts->account)
      break;
    else
      accounts = accounts->next;

  if (!accounts)
    return GNUNET_SYSERR;

  const char *name = account_get_name(handle->current);

  enum GNUNET_GenericReturnValue result;
  result = update_accounts_operation(
    &accounts, 
    handle, 
    name, 
    GNUNET_CHAT_ACCOUNT_UPDATING
  );

  if (GNUNET_OK != result)
    return result;

  accounts->op = GNUNET_IDENTITY_delete(
    handle->identity,
    name,
    cb_account_update,
    accounts
  );

  if (!accounts->op)
    return GNUNET_SYSERR;

  return result;
}

const struct GNUNET_CRYPTO_BlindablePrivateKey*
handle_get_key (const struct GNUNET_CHAT_Handle *handle)
{
  GNUNET_assert(handle);

  if (!(handle->current))
    return NULL;

  return account_get_key(handle->current);
}

void
handle_send_internal_message (struct GNUNET_CHAT_Handle *handle,
                              struct GNUNET_CHAT_Account *account,
                              struct GNUNET_CHAT_Context *context,
                              enum GNUNET_CHAT_MessageFlag flag,
                              const char *warning,
                              enum GNUNET_GenericReturnValue feedback)
{
  GNUNET_assert((handle) && (GNUNET_CHAT_FLAG_NONE != flag));

  if ((handle->destruction) || (!(handle->msg_cb)))
    return;

  struct GNUNET_CHAT_InternalMessages *internal = GNUNET_new(
    struct GNUNET_CHAT_InternalMessages
  );

  internal->chat = handle;
  internal->msg = message_create_internally(
    account, context, flag, warning,
    GNUNET_YES != feedback? GNUNET_YES : GNUNET_NO
  );

  if (!(internal->msg))
  {
    GNUNET_free(internal);
    return;
  }

  if (GNUNET_YES != feedback)
    internal->task = GNUNET_SCHEDULER_add_now(
      on_handle_internal_message_callback,
      internal
    );
  else
  {
    internal->task = NULL;
    if (handle->msg_cb)
      handle->msg_cb(handle->msg_cls, context, internal->msg);
  }

  if (context)
    GNUNET_CONTAINER_DLL_insert(
      handle->internal_head,
      handle->internal_tail,
      internal
    );
  else
    GNUNET_CONTAINER_DLL_insert_tail(
    	handle->internal_head,
    	handle->internal_tail,
    	internal
    );
}

void
handle_send_room_name (struct GNUNET_CHAT_Handle *handle,
		                   struct GNUNET_MESSENGER_Room *room)
{
  GNUNET_assert((handle) && (handle->messenger) && (room));

  if (handle->destruction)
    return;

  const char *name = GNUNET_MESSENGER_get_name(handle->messenger);

  if (!name)
    return;

  struct GNUNET_MESSENGER_Message msg;
  memset(&msg, 0, sizeof(msg));

  msg.header.kind = GNUNET_MESSENGER_KIND_NAME;
  msg.body.name.name = GNUNET_strdup(name);

  GNUNET_MESSENGER_send_message(room, &msg, NULL);

  GNUNET_free(msg.body.name.name);
}

enum GNUNET_GenericReturnValue
handle_request_context_by_room (struct GNUNET_CHAT_Handle *handle,
				                        struct GNUNET_MESSENGER_Room *room)
{
  GNUNET_assert(
    (handle) &&
		(handle->contexts) &&
		(room)
  );

  const struct GNUNET_HashCode *key = GNUNET_MESSENGER_room_get_key(room);

  struct GNUNET_CHAT_Context *context = GNUNET_CONTAINER_multihashmap_get(
    handle->contexts, key
  );

  struct GNUNET_CHAT_CheckHandleRoomMembers check;

  if (!context)
    goto new_context;

  if ((GNUNET_CHAT_CONTEXT_TYPE_UNKNOWN == context->type) &&
      (GNUNET_YES != context->deleted))
    goto check_type;
  
  return GNUNET_OK;

new_context:
  context = context_create_from_room(handle, room);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->contexts, key, context,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    context_destroy(context);
    return GNUNET_SYSERR;
  }

  if (GNUNET_CHAT_CONTEXT_TYPE_GROUP == context->type)
    goto setup_group;

check_type:
  check.ignore_key = GNUNET_MESSENGER_get_key(handle->messenger);
  check.contact = NULL;

  const int checks = GNUNET_MESSENGER_iterate_members(
    room, check_handle_room_members, &check
  );

  if ((check.contact) &&
      (GNUNET_OK == intern_provide_contact_for_member(handle,
						      check.contact,
						      context)))
  {
    context_delete(context, GNUNET_NO);

    context->type = GNUNET_CHAT_CONTEXT_TYPE_CONTACT;
    context->deleted = GNUNET_NO;

    context_write_records(context);
  }
  else if (checks >= minimum_amount_of_other_members_in_group)
  {
    context_delete(context, GNUNET_NO);

    context->type = GNUNET_CHAT_CONTEXT_TYPE_GROUP;
    context->deleted = GNUNET_NO;

    if (context->contact)
    {
      struct GNUNET_CHAT_Contact *contact = handle_get_contact_from_messenger(
	      handle, check.contact
      );

      if ((contact) && (contact->context == context))
	      contact->context = NULL;

      context->contact = NULL;
    }

    goto setup_group;
  }

  return GNUNET_OK;

setup_group:
  GNUNET_MESSENGER_iterate_members(room, scan_handle_room_members, handle);

  struct GNUNET_CHAT_Group *group = group_create_from_context(
    handle, context
  );

  if (context->topic)
    group_publish(group);

  if (GNUNET_OK == GNUNET_CONTAINER_multihashmap_put(
      handle->groups, key, group,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    handle_send_internal_message(
      handle,
      NULL,
      context,
      GNUNET_CHAT_FLAG_UPDATE_CONTEXT,
      NULL,
      GNUNET_NO
    );

    context_write_records(context);
    return GNUNET_OK;
  }

  group_destroy(group);

  GNUNET_CONTAINER_multihashmap_remove(handle->contexts, key, context);
  context_destroy(context);
  return GNUNET_SYSERR;
}

struct GNUNET_CHAT_Contact*
handle_get_contact_from_messenger (const struct GNUNET_CHAT_Handle *handle,
				                           const struct GNUNET_MESSENGER_Contact *contact)
{
  GNUNET_assert((handle) && (handle->contacts) && (contact));

  struct GNUNET_ShortHashCode shorthash;
  util_shorthash_from_member(contact, &shorthash);

  return GNUNET_CONTAINER_multishortmap_get(
    handle->contacts, &shorthash
  );
}

struct GNUNET_CHAT_Group*
handle_get_group_from_messenger (const struct GNUNET_CHAT_Handle *handle,
				                         const struct GNUNET_MESSENGER_Room *room)
{
  GNUNET_assert((handle) && (handle->groups) && (room));

  const struct GNUNET_HashCode *key = GNUNET_MESSENGER_room_get_key(room);

  if (!key)
    return NULL;

  return GNUNET_CONTAINER_multihashmap_get(
    handle->groups, key
  );
}

struct GNUNET_CHAT_Context*
handle_process_records (struct GNUNET_CHAT_Handle *handle,
                        const char *label,
                        unsigned int count,
                        const struct GNUNET_GNSRECORD_Data *data)
{
  GNUNET_assert((handle) && (data));

  if (!count)
    return NULL;

  const struct GNUNET_MESSENGER_RoomEntryRecord *record = NULL;

  for (unsigned int i = 0; i < count; i++)
  {
    if (GNUNET_YES == GNUNET_GNSRECORD_is_expired(data + i))
      continue;

    if ((GNUNET_GNSRECORD_TYPE_MESSENGER_ROOM_ENTRY == data[i].record_type) &&
      (!(GNUNET_GNSRECORD_RF_SUPPLEMENTAL & data[i].flags)))
    {
      record = data[i].data;
      break;
    }
  }

  if (!record)
    return NULL;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_memcpy (&(key.hash), &(record->key), sizeof(key));

  struct GNUNET_CHAT_Context *context = GNUNET_CONTAINER_multihashmap_get(
    handle->contexts,
    &(key.hash)
  );

  if ((context) && (context->room))
  {
    context_read_records(context, label, count, data);
    return NULL;
  }

  struct GNUNET_MESSENGER_Room *room = GNUNET_MESSENGER_enter_room(
    handle->messenger,
    &(record->door),
    &key
  );

  if (!room)
    return NULL;
  else if (context)
  {
    context_update_room(context, room, GNUNET_NO);
    context_read_records(context, label, count, data);
    return NULL;
  }

  context = context_create_from_room(handle, room);
  context_read_records(context, label, count, data);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->contexts, &(key.hash), context,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    context_destroy(context);
    GNUNET_MESSENGER_close_room(room);
    return NULL;
  }

  if (GNUNET_CHAT_CONTEXT_TYPE_GROUP != context->type)
    return context;

  struct GNUNET_CHAT_Group *group = group_create_from_context(handle, context);

  if (context->topic)
    group_publish(group);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      handle->groups, &(key.hash), group,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    group_destroy(group);

  return context;
}
