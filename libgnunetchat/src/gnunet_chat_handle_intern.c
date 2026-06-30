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
 * @file gnunet_chat_handle_intern.c
 */

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

#include "internal/gnunet_chat_accounts.h"
#include "internal/gnunet_chat_tagging.h"

#include <gnunet/gnunet_arm_service.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_hello_uri_lib.h>
#include <gnunet/gnunet_identity_service.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_time_lib.h>
#include <gnunet/gnunet_util_lib.h>

#include <stdio.h>
#include <string.h>

#define GNUNET_UNUSED __attribute__ ((unused))

static const char gnunet_service_name_arm [] = "arm";
static const char gnunet_service_name_fs [] = "fs";
static const char gnunet_service_name_gns [] = "gns";
static const char gnunet_service_name_identity [] = "identity";
static const char gnunet_service_name_messenger [] = "messenger";
static const char gnunet_service_name_namestore [] = "namestore";
static const char gnunet_service_name_pils [] = "pils";
static const char gnunet_service_name_reclaim [] = "reclaim";

void
on_handle_shutdown(void *cls)
{
  struct GNUNET_CHAT_Handle *chat = cls;

  GNUNET_assert((chat) && (chat->shutdown_hook));
  chat->shutdown_hook = NULL;

  handle_destroy(chat);
}

void
on_handle_service_request(void *cls, 
                          enum GNUNET_ARM_RequestStatus status, 
                          enum GNUNET_ARM_Result result)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalServices *services = cls;
  services->op = NULL;

  if (status != GNUNET_ARM_REQUEST_SENT_OK)
    return;

  struct GNUNET_CHAT_Handle *chat = services->chat;

  GNUNET_CONTAINER_DLL_remove(
    chat->services_head,
    chat->services_tail,
    services
  );

  GNUNET_free(services);
}

static void
_request_service_via_arm(struct GNUNET_CHAT_Handle *chat,
                         const char *service_name)
{
  GNUNET_assert((chat) && (chat->arm) && (service_name));

  struct GNUNET_CHAT_InternalServices *services = GNUNET_new(
    struct GNUNET_CHAT_InternalServices
  );

  if (! services)
    return;

  services->chat = chat;
  services->op = GNUNET_ARM_request_service_start(
    chat->arm,
    service_name,
    GNUNET_OS_INHERIT_STD_NONE,
    on_handle_service_request,
    services
  );

  GNUNET_CONTAINER_DLL_insert(
    chat->services_head,
    chat->services_tail,
    services
  );
}

void
on_handle_arm_connection(void *cls,
			                   int connected)
{
  struct GNUNET_CHAT_Handle *chat = cls;

  GNUNET_assert((chat) && (chat->arm));

  if (GNUNET_YES == connected)
  {
    _request_service_via_arm(chat, gnunet_service_name_identity);
    _request_service_via_arm(chat, gnunet_service_name_messenger);
    _request_service_via_arm(chat, gnunet_service_name_fs);
    _request_service_via_arm(chat, gnunet_service_name_gns);
    _request_service_via_arm(chat, gnunet_service_name_namestore);
    _request_service_via_arm(chat, gnunet_service_name_pils);
    _request_service_via_arm(chat, gnunet_service_name_reclaim);
  }
  else
  {
    _request_service_via_arm(chat, gnunet_service_name_arm);
  }
}

void*
notify_handle_fs_progress(void* cls,
			                    const struct GNUNET_FS_ProgressInfo* info)
{
  struct GNUNET_CHAT_Handle *chat = cls;

  GNUNET_assert(info);

  if (!chat)
    return NULL;

  switch (info->status) {
    case GNUNET_FS_STATUS_PUBLISH_START: {
      struct GNUNET_CHAT_File *file = info->value.publish.cctx;

      file_update_upload(
        file,
        0,
        info->value.publish.size
      );

      return file;
    } case GNUNET_FS_STATUS_PUBLISH_PROGRESS: {
      struct GNUNET_CHAT_File *file = info->value.publish.cctx;

      file_update_upload(
        file,
        info->value.publish.completed,
        info->value.publish.size
      );

      return file;
    } case GNUNET_FS_STATUS_PUBLISH_COMPLETED: {
      struct GNUNET_CHAT_File *file = info->value.publish.cctx;

      file->uri = GNUNET_FS_uri_dup(
	      info->value.publish.specifics.completed.chk_uri
      );

      file_update_upload(
        file,
        info->value.publish.size,
        info->value.publish.size
      );

      file->publish = NULL;
      break;
    } case GNUNET_FS_STATUS_PUBLISH_ERROR: {
      break;
    } case GNUNET_FS_STATUS_DOWNLOAD_START: {
      struct GNUNET_CHAT_File *file = info->value.download.cctx;

      file_update_download(
	      file,
        0,
        info->value.download.size
      );

      return file;
    } case GNUNET_FS_STATUS_DOWNLOAD_ACTIVE: {
      return info->value.download.cctx;
    } case GNUNET_FS_STATUS_DOWNLOAD_INACTIVE: {
      return info->value.download.cctx;
    } case GNUNET_FS_STATUS_DOWNLOAD_PROGRESS: {
      struct GNUNET_CHAT_File *file = info->value.download.cctx;

      file_update_download(
        file,
        info->value.download.completed,
        info->value.download.size
      );

      return file;
    } case GNUNET_FS_STATUS_DOWNLOAD_COMPLETED: {
      struct GNUNET_CHAT_File *file = info->value.download.cctx;

      file_update_download(
        file,
        info->value.download.size,
        info->value.download.size
      );

      file->download = NULL;
      break;
    } case GNUNET_FS_STATUS_DOWNLOAD_ERROR: {
      break;
    } case GNUNET_FS_STATUS_UNINDEX_START: {
      struct GNUNET_CHAT_File *file = info->value.unindex.cctx;

      file_update_unindex(
	      file,
        0,
        info->value.unindex.size
      );

      return file;
    } case GNUNET_FS_STATUS_UNINDEX_PROGRESS: {
      struct GNUNET_CHAT_File *file = info->value.unindex.cctx;

      file_update_unindex(
        file,
        info->value.unindex.completed,
        info->value.unindex.size
      );

      return file;
    } case GNUNET_FS_STATUS_UNINDEX_COMPLETED: {
      struct GNUNET_CHAT_File *file = info->value.unindex.cctx;

      file_update_unindex(
	      file,
        info->value.unindex.size,
        info->value.unindex.size
      );

      file->unindex = NULL;
      char *filename = handle_create_file_path(
        chat, &(file->hash)
      );

      if (!filename)
        break;

      if (GNUNET_YES == GNUNET_DISK_file_test_read(filename))
        remove(filename);

      GNUNET_free(filename);
      break;
    } default: {
      break;
    }
  }

  return NULL;
}

static void
on_handle_refresh (void *cls)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Handle* handle = cls;

  handle->refresh = NULL;

  handle_send_internal_message(
    handle,
    NULL,
    NULL,
    GNUNET_CHAT_FLAG_REFRESH,
    NULL,
    GNUNET_YES
  );
}

void
on_handle_gnunet_identity (void *cls,
                           struct GNUNET_IDENTITY_Ego *ego,
                           void **ctx,
                           const char *name)
{
  GNUNET_assert(cls);

  if ((name) && (GNUNET_YES == util_is_lobby_name(name)))
    return;

  struct GNUNET_CHAT_Handle* handle = cls;

  if ((!ctx) || (!ego))
  {
    handle->refreshing = GNUNET_YES;
    goto send_refresh;
  }

  struct GNUNET_CHAT_InternalAccounts *accounts = handle->accounts_head;

  while (accounts)
  {
    if (!(accounts->account))
      goto skip_account;

    if (ego != accounts->account->ego)
      goto check_matching_name;

    if ((name) && ((!(accounts->account->name)) ||
        (0 != strcmp(accounts->account->name, name))))
    {
      const char *old_name = account_get_name(accounts->account);
      char *name_buffer = old_name? GNUNET_strdup(old_name) : NULL;
      
      util_set_name_field(name, &(accounts->account->name));

      handle_send_internal_message(
        handle,
        accounts->account,
        NULL,
        GNUNET_CHAT_FLAG_UPDATE_ACCOUNT,
        name_buffer,
        GNUNET_YES
      );

      if (name_buffer)
        GNUNET_free(name_buffer);
    }
    else if ((!name) && (!(accounts->op)))
    {
      if (handle->current == accounts->account)
	      handle_disconnect(handle);

      account_destroy(accounts->account);
      internal_accounts_destroy(accounts);
    }
    else if (!name)
      account_update_ego(accounts->account, handle, NULL);

    goto send_refresh;

check_matching_name:
    if ((name) && (accounts->account->name) && (ego) &&
	      (0 == strcmp(accounts->account->name, name)))
    {
      account_update_ego(accounts->account, handle, ego);
      goto send_refresh;
    }

skip_account:
    accounts = accounts->next;
  }

  if (!name)
    return;

  accounts = internal_accounts_create(
    handle,
    account_create_from_ego(handle, ego, name)
  );

send_refresh:
  if ((GNUNET_YES != handle->refreshing) ||
      (handle->refresh))
    return;
  
  handle->refresh = GNUNET_SCHEDULER_add_with_priority(
    GNUNET_SCHEDULER_PRIORITY_IDLE,
    on_handle_refresh,
    handle
  );
}

void
cb_account_creation (void *cls,
                     const struct GNUNET_CRYPTO_BlindablePrivateKey *key,
                     enum GNUNET_ErrorCode ec)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalAccounts *accounts = (
    (struct GNUNET_CHAT_InternalAccounts*) cls
  );

  accounts->op = NULL;

  if ((!(accounts->account)) && (accounts->identifier))
    accounts->account = account_create(
      accounts->handle, accounts->identifier
    );
  
  internal_accounts_stop_method(accounts);
  
  if (GNUNET_EC_NONE == ec)
    return;

  handle_send_internal_message(
    accounts->handle,
    accounts->account,
    NULL,
    GNUNET_CHAT_FLAG_WARNING,
    GNUNET_ErrorCode_get_hint(ec),
    GNUNET_YES
  );
}

void
cb_account_deletion (void *cls,
		                 enum GNUNET_ErrorCode ec)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalAccounts *accounts = (
    (struct GNUNET_CHAT_InternalAccounts*) cls
  );

  accounts->op = NULL;

  internal_accounts_stop_method(accounts);

  if (accounts->handle->current == accounts->account)
	  handle_disconnect(accounts->handle);

  if (GNUNET_EC_NONE != ec)
    handle_send_internal_message(
      accounts->handle,
      accounts->account,
      NULL,
      GNUNET_CHAT_FLAG_WARNING,
      GNUNET_ErrorCode_get_hint(ec),
      GNUNET_YES
    );
  else
  {
    handle_send_internal_message(
      accounts->handle,
      accounts->account,
      NULL,
      GNUNET_CHAT_FLAG_DELETE_ACCOUNT,
      NULL,
      GNUNET_YES
    );

    account_delete(accounts->account);
  }

  account_destroy(accounts->account);
  internal_accounts_destroy(accounts);
}

void
cb_account_rename (void *cls,
		               enum GNUNET_ErrorCode ec)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalAccounts *accounts = (
    (struct GNUNET_CHAT_InternalAccounts*) cls
  );

  accounts->op = NULL;

  internal_accounts_stop_method(accounts);

  if (GNUNET_EC_NONE == ec)
    return;

  handle_send_internal_message(
    accounts->handle,
    accounts->account,
    NULL,
    GNUNET_CHAT_FLAG_WARNING,
    GNUNET_ErrorCode_get_hint(ec),
    GNUNET_YES
  );
}

void
cb_lobby_deletion (void *cls,
		               enum GNUNET_ErrorCode ec)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalAccounts *accounts = (
    (struct GNUNET_CHAT_InternalAccounts*) cls
  );

  accounts->op = NULL;

  internal_accounts_stop_method(accounts);

  if (GNUNET_EC_NONE != ec)
    handle_send_internal_message(
      accounts->handle,
      accounts->account,
      NULL,
      GNUNET_CHAT_FLAG_WARNING,
      GNUNET_ErrorCode_get_hint(ec),
      GNUNET_YES
    );

  internal_accounts_destroy(accounts);
}

static void
cb_account_update_completion (void *cls,
                              const struct GNUNET_CRYPTO_BlindablePrivateKey *key,
                              enum GNUNET_ErrorCode ec)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalAccounts *accounts = (
    (struct GNUNET_CHAT_InternalAccounts*) cls
  );

  accounts->op = NULL;

  cb_account_creation(cls, key, ec);
}

void
cb_account_update (void *cls,
		               enum GNUNET_ErrorCode ec)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_InternalAccounts *accounts = (
    (struct GNUNET_CHAT_InternalAccounts*) cls
  );

  if ((GNUNET_EC_NONE != ec) || (!(accounts->identifier)))
  {
    cb_account_deletion(cls, ec);
    return;
  }
  
  accounts->op = GNUNET_IDENTITY_create(
    accounts->handle->identity,
    accounts->identifier,
    NULL,
    GNUNET_PUBLIC_KEY_TYPE_ECDSA,
    cb_account_update_completion,
    accounts
  );
}

int
intern_provide_contact_for_member(struct GNUNET_CHAT_Handle *handle,
                                  const struct GNUNET_MESSENGER_Contact *member,
                                  struct GNUNET_CHAT_Context *context)
{
  GNUNET_assert((handle) && (handle->contacts));

  if (!member)
    return GNUNET_SYSERR;

  struct GNUNET_ShortHashCode shorthash;
  util_shorthash_from_member(member, &shorthash);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CONTAINER_multishortmap_get(
    handle->contacts, &shorthash
  );

  if (contact)
  {
    if ((context) && (NULL == contact->context))
    {
      contact->context = context;
      context->contact = member;
    }

    return GNUNET_OK;
  }

  contact = contact_create_from_member(
    handle, member
  );

  if (context)
  {
    contact->context = context;
    context->contact = member;
  }

  if (GNUNET_OK == GNUNET_CONTAINER_multishortmap_put(
      handle->contacts, &shorthash, contact,
    GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    return GNUNET_OK;

  if (context)
    context->contact = NULL;

  contact_destroy(contact);
  return GNUNET_SYSERR;
}

struct GNUNET_CHAT_CheckHandleRoomMembers
{
  const struct GNUNET_CRYPTO_BlindablePublicKey *ignore_key;
  const struct GNUNET_MESSENGER_Contact *contact;
};

int
check_handle_room_members (void* cls,
			                     GNUNET_UNUSED struct GNUNET_MESSENGER_Room *room,
                           const struct GNUNET_MESSENGER_Contact *member)
{
  struct GNUNET_CHAT_CheckHandleRoomMembers *check = cls;

  GNUNET_assert((check) && (member));

  const struct GNUNET_CRYPTO_BlindablePublicKey *member_key = (
    GNUNET_MESSENGER_contact_get_key(member)
  );

  if ((member_key) && (check->ignore_key) &&
      (0 == GNUNET_memcmp(member_key, check->ignore_key)))
    return GNUNET_YES;

  if (check->contact)
  {
    check->contact = NULL;
    return GNUNET_NO;
  }

  check->contact = member;
  return GNUNET_YES;
}

int
scan_handle_room_members (void* cls,
			                    GNUNET_UNUSED struct GNUNET_MESSENGER_Room *room,
                          const struct GNUNET_MESSENGER_Contact *member)
{
  struct GNUNET_CHAT_Handle *handle = cls;

  GNUNET_assert((handle) && (member));

  if (GNUNET_OK == intern_provide_contact_for_member(handle, member, NULL))
    return GNUNET_YES;
  else
    return GNUNET_NO;
}

void
on_monitor_namestore_record(void *cls,
                            GNUNET_UNUSED const
                            struct GNUNET_CRYPTO_BlindablePrivateKey *zone,
                            const char *label,
                            unsigned int count,
                            const struct GNUNET_GNSRECORD_Data *data)
{
  struct GNUNET_CHAT_Handle *handle = cls;

  GNUNET_assert((handle) && (label) && (data));

  if (handle->destruction)
  {
    GNUNET_NAMESTORE_zone_monitor_stop(handle->monitor);
    handle->monitor = NULL;
    return;
  }

  handle_process_records(handle, label, count, data);

  if (handle->monitor)
    GNUNET_NAMESTORE_zone_monitor_next(handle->monitor, 1);
}

void
on_pils_identity_changed(void *cls,
                         const struct GNUNET_HELLO_Parser *parser,
                         GNUNET_UNUSED const struct GNUNET_HashCode *hash)
{
  struct GNUNET_CHAT_Handle *handle = cls;

  GNUNET_assert((handle) && (parser));

  const struct GNUNET_PeerIdentity *id = GNUNET_HELLO_parser_get_id(parser);

  if (!id)
    return;

  if (!handle->pid)
    handle->pid = GNUNET_new(struct GNUNET_PeerIdentity);

  GNUNET_memcpy(handle->pid, id, sizeof(struct GNUNET_PeerIdentity));
}

void
on_handle_message_callback(void *cls);

static enum GNUNET_GenericReturnValue
it_context_iterate_dependencies(void *cls,
                                const struct GNUNET_HashCode *key,
                                void *value)
{
  struct GNUNET_CHAT_Message *message = (struct GNUNET_CHAT_Message*) value;

  if ((message) && (!message->task))
    message->task = GNUNET_SCHEDULER_add_now(
      on_handle_message_callback, message
    );

  return GNUNET_YES;
}

void
on_handle_internal_message_callback(void *cls)
{
  struct GNUNET_CHAT_InternalMessages *internal = cls;

  GNUNET_assert(
    (internal) &&
    (internal->chat) &&
    (internal->msg) &&
    (internal->task)
  );

  internal->task = NULL;

  struct GNUNET_CHAT_Handle *handle = internal->chat;
  struct GNUNET_CHAT_Context *context = internal->msg->context;

  if (!(handle->msg_cb))
    return;

  handle->msg_cb(handle->msg_cls, context, internal->msg);
}

static enum GNUNET_GenericReturnValue
it_invitation_update (GNUNET_UNUSED void *cls,
                      GNUNET_UNUSED const struct GNUNET_HashCode *key,
                      void *value)
{
  struct GNUNET_CHAT_Invitation *invitation = (struct GNUNET_CHAT_Invitation*) value;

  if (invitation)
    invitation_update(invitation);

  return GNUNET_YES;
}

void
on_handle_message_callback(void *cls)
{
  struct GNUNET_CHAT_Message *message = (struct GNUNET_CHAT_Message*) cls;

  GNUNET_assert(
    (message) &&
		(message->context) &&
		(message->context->handle)
  );

  message->task = NULL;

  if (GNUNET_YES != message_has_msg(message))
    return;

  const struct GNUNET_TIME_Absolute timestamp = GNUNET_TIME_absolute_ntoh(
    message->msg->header.timestamp
  );

  struct GNUNET_TIME_Relative task_delay;
  switch (message->msg->header.kind)
  {
    case GNUNET_MESSENGER_KIND_DELETION:
    {
      const struct GNUNET_TIME_Relative delay = GNUNET_TIME_relative_ntoh(
	      message->msg->body.deletion.delay
      );

      task_delay = GNUNET_TIME_absolute_get_difference(
        GNUNET_TIME_absolute_get(),
        GNUNET_TIME_absolute_add(timestamp, delay)
      );

      break;
    }
    default:
    {
      task_delay = GNUNET_TIME_relative_get_zero_();
      break;
    }
  }

  if (! GNUNET_TIME_relative_is_zero(task_delay))
  {
    message->task = GNUNET_SCHEDULER_add_delayed(
      task_delay,
      on_handle_message_callback,
      message
    );

    return;
  }

  struct GNUNET_CHAT_Context *context = message->context;
  struct GNUNET_CHAT_Handle *handle = context->handle;
  const struct GNUNET_MESSENGER_Contact *sender;

  if (GNUNET_MESSENGER_FLAG_DELETE & message->flags)
    goto skip_msg_handing;

  switch (message->msg->header.kind)
  {
    case GNUNET_MESSENGER_KIND_INVITE:
    {
      if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(context->invites, 
                                                               &(message->hash)))
        break;
      
      struct GNUNET_CHAT_Invitation *invitation = invitation_create_from_message(
	      context, &(message->hash), &(message->msg->body.invite)
      );

      if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
        context->invites, &(message->hash), invitation,
        GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
	      invitation_destroy(invitation);
      else
        GNUNET_CONTAINER_multihashmap_put(
          handle->invitations, &(invitation->key.hash), invitation,
          GNUNET_CONTAINER_MULTIHASHMAPOPTION_MULTIPLE);
      break;
    }
    case GNUNET_MESSENGER_KIND_FILE:
    {
      if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(context->files, 
                                                               &(message->hash)))
        break;

      GNUNET_CONTAINER_multihashmap_put(
        context->files, &(message->hash), NULL,
        GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST
      );

      struct GNUNET_CHAT_File *file = GNUNET_CONTAINER_multihashmap_get(
        context->handle->files, &(message->msg->body.file.hash)
      );

      if (file)
	      break;

      file = file_create_from_message(
	      context->handle, &(message->msg->body.file)
      );

	  if (!file)
		  break;

      if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
          context->handle->files, &(file->hash), file,
          GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
	      file_destroy(file);
      break;
    }
    case GNUNET_MESSENGER_KIND_TAG:
    {
      struct GNUNET_CHAT_InternalTagging *tagging = GNUNET_CONTAINER_multihashmap_get(
        context->taggings, &(message->msg->body.tag.hash));
      
      if (!tagging)
      {
        tagging = internal_tagging_create();

        if (!tagging)
          break;

        if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
            context->taggings, &(message->msg->body.tag.hash), tagging,
            GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
        {
          internal_tagging_destroy(tagging);
          break;
        }
      }

      internal_tagging_add(tagging, message);
      break;
    }
    default:
      break;
  }

skip_msg_handing:
  sender = GNUNET_MESSENGER_get_sender(context->room, &(message->hash));

  if (!sender)
    goto clear_dependencies;

  struct GNUNET_ShortHashCode shorthash;
  util_shorthash_from_member(sender, &shorthash);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CONTAINER_multishortmap_get(
    handle->contacts, &shorthash
  );

  if (!contact)
    goto clear_dependencies;

  if (GNUNET_MESSENGER_FLAG_DELETE & message->flags)
    goto skip_sender_handing;

  switch (message->msg->header.kind)
  {
    case GNUNET_MESSENGER_KIND_JOIN:
    {
      contact_update_join(contact, context, 
        &(message->hash), message->flags);
      
      GNUNET_CONTAINER_multihashmap_get_multiple(
        handle->invitations,
        GNUNET_MESSENGER_room_get_key(context->room),
        it_invitation_update, handle);
      
      if ((GNUNET_MESSENGER_FLAG_SENT & message->flags) &&
          (GNUNET_MESSENGER_FLAG_RECENT & message->flags))
        handle_send_room_name(handle, context->room);
      
      break;
    }
    case GNUNET_MESSENGER_KIND_LEAVE:
    {
      GNUNET_CONTAINER_multihashmap_get_multiple(
        handle->invitations,
        GNUNET_MESSENGER_room_get_key(context->room),
        it_invitation_update, handle);
      
      break;
    }
    case GNUNET_MESSENGER_KIND_KEY:
    {
      contact_update_key(contact);
      break;
    }
    case GNUNET_MESSENGER_KIND_TICKET:
    {
      struct GNUNET_CHAT_InternalTickets *tickets = contact->tickets_head;
      while (tickets)
      {
        if (0 == strncmp(tickets->ticket->ticket.gns_name, 
                         message->msg->body.ticket.identifier,
                         sizeof(tickets->ticket->ticket.gns_name)))
          break;

        tickets = tickets->next;
      }

      if (tickets)
        break;
      
      tickets = GNUNET_new(
        struct GNUNET_CHAT_InternalTickets
      );

      if (!tickets)
        break;

      tickets->ticket = ticket_create_from_message(
	      handle, contact, &(message->msg->body.ticket)
      );

      if (!tickets->ticket)
      {
        GNUNET_free(tickets);
        break;
      }

      GNUNET_CONTAINER_DLL_insert_tail(
        contact->tickets_head,
        contact->tickets_tail,
        tickets
      );
      break;
    }
    case GNUNET_MESSENGER_KIND_SUBSCRIBTION:
    {
      const struct GNUNET_ShortHashCode *sid = &(message->msg->body.subscription.discourse);
      struct GNUNET_CHAT_Discourse *discourse = GNUNET_CONTAINER_multishortmap_get(
        context->discourses, sid
      );

      if (! discourse)
      {
        struct GNUNET_CHAT_DiscourseId id;
        util_discourse_id_from_shorthash(sid, &id);

        discourse = discourse_create(context, &id);

        if (GNUNET_OK != GNUNET_CONTAINER_multishortmap_put(context->discourses,
          sid, discourse, GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
        {
          discourse_destroy(discourse);
          break;
        }
      }

      enum GNUNET_GenericReturnValue subscription_update = GNUNET_NO;

      if (GNUNET_MESSENGER_FLAG_SUBSCRIPTION_UNSUBSCRIBE & message->msg->body.subscription.flags)
        discourse_unsubscribe(
          discourse,
          contact,
          GNUNET_TIME_absolute_ntoh(message->msg->header.timestamp),
          GNUNET_TIME_relative_ntoh(message->msg->body.subscription.time)
        );
      else
        subscription_update = discourse_subscribe(
          discourse,
          contact,
          GNUNET_TIME_absolute_ntoh(message->msg->header.timestamp),
          GNUNET_TIME_relative_ntoh(message->msg->body.subscription.time)
        );
      
      if (GNUNET_YES == subscription_update)
        message->flags |= GNUNET_MESSENGER_FLAG_UPDATE;

      break;
    }
    default:
      break;
  }

skip_sender_handing:
  if (!(handle->msg_cb))
    goto clear_dependencies;

  handle->msg_cb(handle->msg_cls, context, message);

clear_dependencies:
  GNUNET_CONTAINER_multihashmap_get_multiple(context->dependencies,
                                             &(message->hash),
                                             it_context_iterate_dependencies,
                                             NULL);
  GNUNET_CONTAINER_multihashmap_remove_all(context->dependencies,
                                           &(message->hash));
}

void
on_handle_message (void *cls,
                   struct GNUNET_MESSENGER_Room *room,
                   const struct GNUNET_MESSENGER_Contact *sender,
                   const struct GNUNET_MESSENGER_Contact *recipient,
                   const struct GNUNET_MESSENGER_Message *msg,
                   const struct GNUNET_HashCode *hash,
                   enum GNUNET_MESSENGER_MessageFlags flags)
{
  struct GNUNET_CHAT_Handle *handle = cls;

  GNUNET_assert(
    (handle) &&
		(room) &&
		(msg) &&
		(hash)
  );

  if ((handle->destruction) ||
      (GNUNET_OK != handle_request_context_by_room(handle, room)))
    return;
  
  struct GNUNET_CHAT_Context *context = GNUNET_CONTAINER_multihashmap_get(
    handle->contexts, GNUNET_MESSENGER_room_get_key(room)
  );

  if (GNUNET_MESSENGER_KIND_MERGE == msg->header.kind)
    context_request_message(context, &(msg->body.merge.previous));

  context_request_message(context, &(msg->header.previous));

  if ((GNUNET_CHAT_KIND_UNKNOWN == util_message_kind_from_kind(msg->header.kind)) ||
      (GNUNET_OK != intern_provide_contact_for_member(handle, sender, NULL)))
    return;

  const struct GNUNET_TIME_Absolute timestamp = GNUNET_TIME_absolute_ntoh(
    msg->header.timestamp
  );

  struct GNUNET_ShortHashCode shorthash;
  util_shorthash_from_member(sender, &shorthash);

  struct GNUNET_CHAT_Contact *contact = GNUNET_CONTAINER_multishortmap_get(
    handle->contacts, &shorthash
  );

  if (flags & GNUNET_MESSENGER_FLAG_SENT)
    contact->owned = GNUNET_YES;

  struct GNUNET_TIME_Absolute *time = GNUNET_CONTAINER_multishortmap_get(
    context->timestamps, &shorthash
  );

  if (!time)
  {
    time = GNUNET_new(struct GNUNET_TIME_Absolute);
    *time = timestamp;

    if (GNUNET_OK != GNUNET_CONTAINER_multishortmap_put(
        context->timestamps, &shorthash, time,
        GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
      GNUNET_free(time);
  }
  else
  {
    const struct GNUNET_TIME_Relative delta = GNUNET_TIME_absolute_get_difference(
	    timestamp, *time
    );

    if (GNUNET_TIME_relative_is_zero(delta))
      *time = timestamp;
  }

  const struct GNUNET_HashCode *dependency = NULL;

  struct GNUNET_CHAT_Message *message = GNUNET_CONTAINER_multihashmap_get(
    context->messages, hash
  );

  if (message)
  {
    if (message->flags & GNUNET_MESSENGER_FLAG_DELETE)
      return;

    message_update_msg (message, flags, msg);

    if (0 == (message->flags & GNUNET_MESSENGER_FLAG_UPDATE))
      return;

    goto handle_callback;
  }

  message = message_create_from_msg(context, hash, flags, msg);

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(
      context->messages, hash, message,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
  {
    message_destroy(message);
    return;
  }

handle_callback:
  switch (msg->header.kind)
  {
    case GNUNET_MESSENGER_KIND_DELETION:
    {
      dependency = &(msg->body.deletion.hash);
      break;
    }
    case GNUNET_MESSENGER_KIND_TRANSCRIPT:
    {
      dependency = &(msg->body.transcript.hash);
      break;
    }
    case GNUNET_MESSENGER_KIND_TAG:
    {
      dependency = &(msg->body.tag.hash);
      break;
    }
    default:
      break;
  }

  if ((dependency) && 
      (GNUNET_YES != GNUNET_CONTAINER_multihashmap_contains(context->messages, dependency)))
  {
    GNUNET_CONTAINER_multihashmap_put(
      context->dependencies,
      dependency,
      message,
      GNUNET_CONTAINER_MULTIHASHMAPOPTION_MULTIPLE
    );

    GNUNET_MESSENGER_get_message(room, dependency);
    return;
  }

  on_handle_message_callback(message);
}

int
it_destroy_handle_groups (GNUNET_UNUSED void *cls,
                          GNUNET_UNUSED const struct GNUNET_HashCode *key,
                          void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_Group *group = value;
  group_destroy(group);
  return GNUNET_YES;
}

int
it_destroy_handle_contacts (GNUNET_UNUSED void *cls,
                            GNUNET_UNUSED const struct GNUNET_ShortHashCode *key,
                            void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_Contact *contact = value;
  contact_destroy(contact);
  return GNUNET_YES;
}

int
it_destroy_handle_contexts (GNUNET_UNUSED void *cls,
                            GNUNET_UNUSED const struct GNUNET_HashCode *key,
                            void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_Context *context = value;
  context_destroy(context);
  return GNUNET_YES;
}

int
it_destroy_handle_files (GNUNET_UNUSED void *cls,
                         GNUNET_UNUSED const struct GNUNET_HashCode *key,
                         void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_File *file = value;
  file_destroy(file);
  return GNUNET_YES;
}
