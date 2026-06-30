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
 * @file gnunet_chat_handle.h
 */

#ifndef GNUNET_CHAT_HANDLE_H_
#define GNUNET_CHAT_HANDLE_H_

#include "gnunet_chat_lib.h"
#include "gnunet_chat_account.h"
#include "gnunet_chat_contact.h"
#include "gnunet_chat_lobby.h"
#include "gnunet_chat_message.h"
#include "gnunet_chat_uri.h"

#include "internal/gnunet_chat_accounts.h"
#include "internal/gnunet_chat_attribute_process.h"
#include "internal/gnunet_chat_ticket_process.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_arm_service.h>
#include <gnunet/gnunet_fs_service.h>
#include <gnunet/gnunet_gns_service.h>
#include <gnunet/gnunet_identity_service.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_namestore_service.h>
#include <gnunet/gnunet_reclaim_lib.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <gnunet/gnunet_time_lib.h>
#include <gnunet/gnunet_util_lib.h>

struct GNUNET_CHAT_Handle;

struct GNUNET_CHAT_InternalServices
{
  struct GNUNET_CHAT_Handle *chat;
  struct GNUNET_ARM_Operation *op;
  struct GNUNET_CHAT_InternalServices *next;
  struct GNUNET_CHAT_InternalServices *prev;
};

struct GNUNET_CHAT_InternalMessages
{
  struct GNUNET_CHAT_Handle *chat;
  struct GNUNET_CHAT_Message *msg;
  struct GNUNET_SCHEDULER_Task *task;
  struct GNUNET_CHAT_InternalMessages *next;
  struct GNUNET_CHAT_InternalMessages *prev;
};

struct GNUNET_CHAT_InternalLobbies
{
  struct GNUNET_CHAT_Lobby *lobby;
  struct GNUNET_CHAT_InternalLobbies *next;
  struct GNUNET_CHAT_InternalLobbies *prev;
};

struct GNUNET_CHAT_UriLookups
{
  struct GNUNET_CHAT_Handle *handle;

  struct GNUNET_GNS_LookupRequest *request;
  struct GNUNET_CHAT_Uri *uri;

  struct GNUNET_CHAT_UriLookups *next;
  struct GNUNET_CHAT_UriLookups *prev;
};

struct GNUNET_CHAT_Handle
{
  const struct GNUNET_CONFIGURATION_Handle* cfg;
  struct GNUNET_SCHEDULER_Task *shutdown_hook;
  struct GNUNET_SCHEDULER_Task *destruction;
  struct GNUNET_SCHEDULER_Task *connection;
  struct GNUNET_SCHEDULER_Task *refresh;

  struct GNUNET_CHAT_InternalServices *services_head;
  struct GNUNET_CHAT_InternalServices *services_tail;

  struct GNUNET_CHAT_InternalMessages *internal_head;
  struct GNUNET_CHAT_InternalMessages *internal_tail;

  char *directory;

  GNUNET_CHAT_ContextMessageCallback msg_cb;
  void *msg_cls;

  struct GNUNET_CHAT_InternalAccounts *accounts_head;
  struct GNUNET_CHAT_InternalAccounts *accounts_tail;

  enum GNUNET_GenericReturnValue refreshing;
  struct GNUNET_CHAT_Contact *own_contact;

  struct GNUNET_CHAT_Account *next;
  struct GNUNET_CHAT_Account *current;
  struct GNUNET_NAMESTORE_ZoneMonitor *monitor;

  struct GNUNET_CHAT_InternalLobbies *lobbies_head;
  struct GNUNET_CHAT_InternalLobbies *lobbies_tail;

  struct GNUNET_CHAT_UriLookups *lookups_head;
  struct GNUNET_CHAT_UriLookups *lookups_tail;

  struct GNUNET_CHAT_AttributeProcess *attributes_head;
  struct GNUNET_CHAT_AttributeProcess *attributes_tail;

  struct GNUNET_CHAT_TicketProcess *tickets_head;
  struct GNUNET_CHAT_TicketProcess *tickets_tail;

  struct GNUNET_CONTAINER_MultiHashMap *files;
  struct GNUNET_CONTAINER_MultiHashMap *contexts;
  struct GNUNET_CONTAINER_MultiShortmap *contacts;
  struct GNUNET_CONTAINER_MultiHashMap *groups;
  struct GNUNET_CONTAINER_MultiHashMap *invitations;

  struct GNUNET_ARM_Handle *arm;
  struct GNUNET_FS_Handle *fs;
  struct GNUNET_GNS_Handle *gns;
  struct GNUNET_IDENTITY_Handle *identity;
  struct GNUNET_MESSENGER_Handle *messenger;
  struct GNUNET_NAMESTORE_Handle *namestore;
  struct GNUNET_RECLAIM_Handle *reclaim;

  char *public_key;
  void *user_pointer;
};

/**
 * Creates a chat handle with a selected configuration,
 * a custom message callback and a custom closure for
 * the callback.
 *
 * @param[in] cfg Configuration
 * @param[in] msg_cb Message callback
 * @param[in,out] msg_cls Closure
 * @return New chat handle
 */
struct GNUNET_CHAT_Handle*
handle_create_from_config (const struct GNUNET_CONFIGURATION_Handle* cfg,
                           GNUNET_CHAT_ContextMessageCallback msg_cb,
                           void *msg_cls);

/**
 * Updates the string representation of the public key from
 * a given chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 */
void
handle_update_key (struct GNUNET_CHAT_Handle *handle);

/**
 * Destroys a chat <i>handle</i> and frees its memory.
 *
 * @param[in,out] handle Chat handle
 */
void
handle_destroy (struct GNUNET_CHAT_Handle *handle);

/**
 * Connects a given chat <i>handle</i> to a selected
 * chat <i>account</i> using it for further operations.
 *
 * @param[in,out] handle Chat handle
 * @param[in] account Chat account
 */
void
handle_connect (struct GNUNET_CHAT_Handle *handle,
		            struct GNUNET_CHAT_Account *account);

/**
 * Disconnects a given chat <i>handle</i> from its current
 * connected chat account.
 *
 * @param[in,out] handle Chat handle
 */
void
handle_disconnect (struct GNUNET_CHAT_Handle *handle);

/**
 * Searches for an existing chat account by <i>name</i> as
 * identifier for a given chat <i>handle</i>.
 *
 * @param[in] handle Chat handle
 * @param[in] name Chat account name
 * @param[in] skip_op Whether to skip accounts with active operation
 * @return Chat account
 */
struct GNUNET_CHAT_Account*
handle_get_account_by_name (const struct GNUNET_CHAT_Handle *handle,
		                        const char *name,
                            enum GNUNET_GenericReturnValue skip_op);

/**
 * Enqueues a creation for a chat account with a specific
 * <i>name</i> as identifier for a given chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name Chat account name
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
handle_create_account (struct GNUNET_CHAT_Handle *handle,
		                   const char *name);

/**
 * Enqueues a deletion for a chat <i>account</i> of a 
 * given chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] account Chat account
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
handle_delete_account (struct GNUNET_CHAT_Handle *handle,
		                   const struct GNUNET_CHAT_Account *account);

/**
 * Renames a chat <i>account</i> of a given chat 
 * <i>handle</i> to another specific <i>new_name</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] account Chat account
 * @param[in] new_name New chat account name
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
handle_rename_account (struct GNUNET_CHAT_Handle *handle,
                       const struct GNUNET_CHAT_Account *account,
                       const char *new_name);

/**
 * Enqueues a deletion for a chat <i>lobby</i> for a 
 * given chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] lobby Chat lobby
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
handle_delete_lobby (struct GNUNET_CHAT_Handle *handle,
                     const struct GNUNET_CHAT_Lobby *lobby);

/**
 * Returns the main directory path to store information
 * of a given chat <i>handle</i>.
 *
 * @param[in] handle Chat handle
 * @return Directory path
 */
const char*
handle_get_directory (const struct GNUNET_CHAT_Handle *handle);

/**
 * Returns an allocated string providing the full path to
 * a file stored by a chat <i>handle</i> with a given 
 * <i>hash</i>.
 *
 * @param[in] handle Chat handle
 * @param[in] hash File hash
 * @return File path
 */
char*
handle_create_file_path (const struct GNUNET_CHAT_Handle *handle,
                         const struct GNUNET_HashCode *hash);

/**
 * Updates the used private key by creating a new identity
 * using the same identifier as currently in use, replacing
 * the old identity.
 *
 * @param[in,out] handle Chat handle
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
handle_update (struct GNUNET_CHAT_Handle *handle);

/**
 * Returns the private key from the current connected chat
 * account of a given chat <i>handle</i>.
 *
 * @param[in] handle Chat handle
 * @return EGOs private key or NULL
 */
const struct GNUNET_CRYPTO_BlindablePrivateKey*
handle_get_key (const struct GNUNET_CHAT_Handle *handle);

/**
 * Sends an internal chat message from a given chat
 * <i>handle</i> with an optional chat <i>account</i> or
 * <i>context</i>, a custom <i>flag</i> and an optional 
 * <i>warning</i> text.
 *
 * You can select whether the callback for the internal 
 * message should be scheduled dynamically or be called
 * as instant feedback.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] account Chat account or NULL
 * @param[in,out] context Chat context or NULL
 * @param[in] flag Chat message flag
 * @param[in] warning Warning text
 * @param[in] feedback Instant feedback
 */
void
handle_send_internal_message (struct GNUNET_CHAT_Handle *handle,
                              struct GNUNET_CHAT_Account *account,
                              struct GNUNET_CHAT_Context *context,
                              enum GNUNET_CHAT_MessageFlag flag,
                              const char *warning,
                              enum GNUNET_GenericReturnValue feedback);

/**
 * Sends a name message to a messenger <i>room</i> with
 * a selected chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] room Messenger room
 */
void
handle_send_room_name (struct GNUNET_CHAT_Handle *handle,
		                   struct GNUNET_MESSENGER_Room *room);

/**
 * Checks a given chat <i>handle</i> for any chat context
 * connected with a messenger <i>room</i>, creates it if
 * necessary and manages its context type.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] room Messenger room
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
handle_request_context_by_room (struct GNUNET_CHAT_Handle *handle,
				                        struct GNUNET_MESSENGER_Room *room);

/**
 * Returns the chat contact registered for a given messenger
 * <i>contact</i> by a selected chat <i>handle</i>.
 *
 * @param[in] handle Chat handle
 * @param[in] contact Messenger contact
 * @return Chat contact or NULL
 */
struct GNUNET_CHAT_Contact*
handle_get_contact_from_messenger (const struct GNUNET_CHAT_Handle *handle,
				                           const struct GNUNET_MESSENGER_Contact *contact);

/**
 * Returns the chat group registered for a given messenger
 * <i>room</i> by a selected chat <i>handle</i>.
 *
 * @param[in] handle Chat handle
 * @param[in] room Messenger room
 * @return Chat group or NULL
 */
struct GNUNET_CHAT_Group*
handle_get_group_from_messenger (const struct GNUNET_CHAT_Handle *handle,
				                         const struct GNUNET_MESSENGER_Room *room);

/**
 * Processes the <i>data</i> of records under a given
 * <i>label</i> and creates a matching chat <i>context</i>
 * with it if it does not exist already, registered by a chat
 * <i>handle</i>, to be updated.
 *
 * @param[in,out] handle Chat handle
 * @param[in] label Namestore label
 * @param[in] count Count of data
 * @param[in] data Records data
 * @return Chat context or NULL
 */
struct GNUNET_CHAT_Context*
handle_process_records (struct GNUNET_CHAT_Handle *handle,
                        const char *label,
                        unsigned int count,
                        const struct GNUNET_GNSRECORD_Data *data);

#endif /* GNUNET_CHAT_HANDLE_H_ */
