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
   The development of this code was supported by the NLnet foundation as part
   of the NGI Assure initative to have a more free and secure internet.
 */
/*
 * @author Tobias Frisch
 * @file gnunet_chat_lib.h
 */

#ifndef GNUNET_CHAT_LIB_H_
#define GNUNET_CHAT_LIB_H_

/**
 * @defgroup gnunet_chat GNUnet Chat library
 */
/**@{*/

#include <gnunet/gnunet_util_lib.h>


#include <stdint.h>
#include <time.h>

/**
 * @def GNUNET_CHAT_VERSION The major and minor version should be identical to
 *                          the #GNUNET_MESSENGER_VERSION of the GNUnet Messenger
 *                          service while the patch version is independent.
 */
#define GNUNET_CHAT_VERSION 0x000000060001L

#define GNUNET_CHAT_VERSION_MAJOR ((GNUNET_CHAT_VERSION >> 32L) & 0xFFFFL)
#define GNUNET_CHAT_VERSION_MINOR ((GNUNET_CHAT_VERSION >> 16L) & 0xFFFFL)
#define GNUNET_CHAT_VERSION_PATCH ((GNUNET_CHAT_VERSION >>  0L) & 0xFFFFL)

#define GNUNET_CHAT_ATTRIBUTE_AVATAR   "chat_avatar"
#define GNUNET_CHAT_ATTRIBUTE_BIRTHDAY "chat_birthday"
#define GNUNET_CHAT_ATTRIBUTE_EMAIL    "chat_email"
#define GNUNET_CHAT_ATTRIBUTE_NAME     "chat_name"
#define GNUNET_CHAT_ATTRIBUTE_PHONE    "chat_phone"
#define GNUNET_CHAT_ATTRIBUTE_POSTAL   "chat_postal"

#define GNUNET_CHAT_URI_PREFIX "gnunet://chat/"

/**
 * Enum for the different types of supported URIs.
 */
enum GNUNET_CHAT_UriType
{
  /**
   * The type to share and join a lobby or chat.
   */
  GNUNET_CHAT_URI_TYPE_CHAT = 1,   /**< GNUNET_CHAT_URI_TYPE_CHAT */

  /**
   * The type to download and query files vis FS service from GNUnet.
   */
  GNUNET_CHAT_URI_TYPE_FS = 2,     /**< GNUNET_CHAT_URI_TYPE_FS */

  /**
   * An unknown type of uri.
   */
  GNUNET_CHAT_URI_TYPE_UNKNOWN = 0 /**< GNUNET_CHAT_URI_TYPE_UNKNOWN */
};

/**
 * Enum for the different kinds of messages.
 */
enum GNUNET_CHAT_MessageKind
{
  /**
   * The kind to inform that something went wrong.
   */
  GNUNET_CHAT_KIND_WARNING = 1,            /**< GNUNET_CHAT_KIND_WARNING */

  /**
   * The kind to inform that the list of accounts was refreshed.
   */
  GNUNET_CHAT_KIND_REFRESH = 2,            /**< GNUNET_CHAT_KIND_REFRESH */

  /**
   * The kind to inform that the application can be used.
   */
  GNUNET_CHAT_KIND_LOGIN = 3,              /**< GNUNET_CHAT_KIND_LOGIN */

  /**
   * The kind to inform that the application needs to cleanup 
   * resources related to the currently connected account.
   */
  GNUNET_CHAT_KIND_LOGOUT = 4,             /**< GNUNET_CHAT_KIND_LOGOUT */

  /**
   * The kind to inform that an account was successfully created.
   */
  GNUNET_CHAT_KIND_CREATED_ACCOUNT = 5,    /**< GNUNET_CHAT_KIND_CREATED_ACCOUNT */

  /**
   * The kind to inform that an account was successfully deleted.
   */
  GNUNET_CHAT_KIND_DELETED_ACCOUNT = 6,    /**< GNUNET_CHAT_KIND_DELETED_ACCOUNT */

  /**
   * The kind to inform that an account was updated.
   */
  GNUNET_CHAT_KIND_UPDATE_ACCOUNT = 7,     /**< GNUNET_CHAT_KIND_UPDATE_ACCOUNT */

  /**
   * The kind to inform that a context was updated.
   */
  GNUNET_CHAT_KIND_UPDATE_CONTEXT = 8,     /**< GNUNET_CHAT_KIND_UPDATE_CONTEXT */

  /**
   * The kind to inform that a contact has joined a chat.
   */
  GNUNET_CHAT_KIND_JOIN = 9,               /**< GNUNET_CHAT_KIND_JOIN */

  /**
   * The kind to inform that a contact has left a chat.
   */
  GNUNET_CHAT_KIND_LEAVE = 10,             /**< GNUNET_CHAT_KIND_LEAVE */

  /**
   * The kind to inform that a contact has changed.
   */
  GNUNET_CHAT_KIND_CONTACT = 11,           /**< GNUNET_CHAT_KIND_CONTACT */

  /**
   * The kind to describe an invitation to a different chat.
   */
  GNUNET_CHAT_KIND_INVITATION = 12,        /**< GNUNET_CHAT_KIND_INVITATION */

  /**
   * The kind to describe a text message.
   */
  GNUNET_CHAT_KIND_TEXT = 13,              /**< GNUNET_CHAT_KIND_TEXT */

  /**
   * The kind to describe a shared file.
   */
  GNUNET_CHAT_KIND_FILE = 14,              /**< GNUNET_CHAT_KIND_FILE */

  /**
   * The kind to inform about a deletion of a previous message.
   */
  GNUNET_CHAT_KIND_DELETION = 15,          /**< GNUNET_CHAT_KIND_DELETION */

  /**
   * The kind to tag a previous message.
   */
  GNUNET_CHAT_KIND_TAG = 16,               /**< GNUNET_CHAT_KIND_TAG */

  /**
   * The kind to inform that attributes were updated.
   */
  GNUNET_CHAT_KIND_ATTRIBUTES = 17,        /**< GNUNET_CHAT_KIND_ATTRIBUTES */

  /**
   * The kind to inform that attributes were shared.
   */
  GNUNET_CHAT_KIND_SHARED_ATTRIBUTES = 18, /**< GNUNET_CHAT_KIND_SHARED_ATTRIBUTES */

  /**
   * The kind to inform that a discourse was updated.
   */
  GNUNET_CHAT_KIND_DISCOURSE = 19,         /**< GNUNET_CHAT_KIND_DISCOURSE */

  /**
   * The kind to describe a data message from a discourse.
   */
  GNUNET_CHAT_KIND_DATA = 20,              /**< GNUNET_CHAT_KIND_DATA */

  /**
   * An unknown kind of message.
   */
  GNUNET_CHAT_KIND_UNKNOWN = 0             /**< GNUNET_CHAT_KIND_UNKNOWN */
};

/**
 * Struct to identify discourses
 */
struct GNUNET_CHAT_DiscourseId
{
  char identifier [32];
};

/**
 * Struct of a chat handle.
 */
struct GNUNET_CHAT_Handle;

/**
 * Struct of a chat account.
 */
struct GNUNET_CHAT_Account;

/**
 * Struct of a chat URI.
 */
struct GNUNET_CHAT_Uri;

/**
 * Struct of a chat lobby.
 */
struct GNUNET_CHAT_Lobby;

/**
 * Struct of a chat contact.
 */
struct GNUNET_CHAT_Contact;

/**
 * Struct of a chat group.
 */
struct GNUNET_CHAT_Group;

/**
 * Struct of a chat context.
 */
struct GNUNET_CHAT_Context;

/**
 * Struct of a chat message.
 */
struct GNUNET_CHAT_Message;

/**
 * Struct of a chat file.
 */
struct GNUNET_CHAT_File;

/**
 * Struct of a chat invitation.
 */
struct GNUNET_CHAT_Invitation;

/**
 * Struct of a chat discourse.
 */
struct GNUNET_CHAT_Discourse;

/**
 * Iterator over chat accounts of a specific chat handle.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_iterate_accounts
 * @param[in,out] handle Chat handle
 * @param[in,out] account Chat account
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_AccountCallback) (void *cls,
                                struct GNUNET_CHAT_Handle *handle,
                                struct GNUNET_CHAT_Account *account);

/**
 * Iterator over attributes of a specific chat account.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_get_attributes
 * @param[in,out] account Chat account
 * @param[in] name Attribute name
 * @param[in] value Attribute value
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_AccountAttributeCallback) (void *cls,
                                         struct GNUNET_CHAT_Account *account,
                                         const char *name,
                                         const char *value);

/**
 * Iterator over attributes of a specific chat handle.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_get_attributes
 * @param[in,out] handle Chat handle
 * @param[in] name Attribute name
 * @param[in] value Attribute value
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_AttributeCallback) (void *cls,
                                  struct GNUNET_CHAT_Handle *handle,
                                  const char *name,
                                  const char *value);

/**
 * Method called when a lobby is opened to share with others via a chat URI.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_lobby_open
 * @param[in] uri Chat URI of the lobby or NULL on error
 */
typedef void
(*GNUNET_CHAT_LobbyCallback) (void *cls, 
                              const struct GNUNET_CHAT_Uri *uri);

/**
 * Iterator over chat files of a specific chat handle.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_iterate_files
 * @param[in,out] handle Chat handle
 * @param[in,out] file Chat file
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_FileCallback) (void *cls,
                             struct GNUNET_CHAT_Handle *handle,
                             struct GNUNET_CHAT_File *file);

/**
 * Iterator over chat contacts of a specific chat handle.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_iterate_contacts
 * @param[in,out] handle Chat handle
 * @param[in,out] contact Chat contact
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_ContactCallback) (void *cls,
                                struct GNUNET_CHAT_Handle *handle,
                                struct GNUNET_CHAT_Contact *contact);

/**
 * Iterator over tags of a specific chat contact.
 *
 * @param[in,out] cls Closure
 * @param[in,out] contact Chat contact
 * @param[in] tag Tag
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_ContactTagCallback) (void *cls,
                                   struct GNUNET_CHAT_Contact *contact,
                                   const char *tag);

/**
 * Iterator over accessible attributes of a specific chat contact.
 *
 * @param[in,out] cls Closure
 * @param[in,out] contact Chat contact
 * @param[in] name Attribute name
 * @param[in] value Attribute value
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_ContactAttributeCallback) (void *cls,
                                         struct GNUNET_CHAT_Contact *contact,
                                         const char *name,
                                         const char *value);

/**
 * Iterator over chat groups of a specific chat handle.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_iterate_groups
 * @param[in,out] handle Chat handle
 * @param[in,out] group Chat group
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_GroupCallback) (void *cls,
                              struct GNUNET_CHAT_Handle *handle,
                              struct GNUNET_CHAT_Group *group);

/**
 * Iterator over chat contacts in a specific chat group.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_group_iterate_contacts
 * @param[in,out] group Chat group
 * @param[in,out] contact Chat contact
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_GroupContactCallback) (void *cls,
                                     struct GNUNET_CHAT_Group *group,
                                     struct GNUNET_CHAT_Contact *contact);

/**
 * Iterator over chat messages in a specific chat context.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_context_iterate_messages
 * @param[in,out] context Chat context or NULL
 * @param[in,out] message Chat message
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_ContextMessageCallback) (void *cls,
                                       struct GNUNET_CHAT_Context *context,
                                       struct GNUNET_CHAT_Message *message);

/**
 * Iterator over chat files in a specific chat context.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_context_iterate_files
 * @param[in,out] context Chat context
 * @param[in,out] file Chat file
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_ContextFileCallback) (void *cls,
                                    struct GNUNET_CHAT_Context *context,
                                    struct GNUNET_CHAT_File *file);

/**
 * Iterator over chat contacts in a chat to check whether they received a
 * specific message or not.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_message_get_read_receipt
 * @param[in,out] message Chat message
 * @param[in,out] contact Chat contact
 * @param[in] read_receipt #GNUNET_YES if the message was received by the contact,
 * 			   #GNUNET_NO otherwise
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_MessageReadReceiptCallback) (void *cls,
                                           struct GNUNET_CHAT_Message *message,
                                           struct GNUNET_CHAT_Contact *contact,
                                           int read_receipt);

/**
 * Iterator over chat tag messages with a specific target message.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_message_iterate_tags
 * @param[in,out] message Chat message
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_MessageCallback) (void *cls,
                                struct GNUNET_CHAT_Message *message);

/**
 * Method called during an upload of a specific file in a chat to share it.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_context_send_file
 * @param[in,out] file Chat file
 * @param[in] completed Amount of the file being uploaded (in bytes)
 * @param[in] size Full size of the uploading file (in bytes)
 */
typedef void
(*GNUNET_CHAT_FileUploadCallback) (void *cls,
                                   struct GNUNET_CHAT_File *file,
                                   uint64_t completed,
                                   uint64_t size);

/**
 * Method called during a download of a specific file in a chat which was shared.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_file_start_download
 * @param[in,out] file Chat file
 * @param[in] completed Amount of the file being downloaded (in bytes)
 * @param[in] size Full size of the downloading file (in bytes)
 */
typedef void
(*GNUNET_CHAT_FileDownloadCallback) (void *cls,
                                     struct GNUNET_CHAT_File *file,
                                     uint64_t completed,
                                     uint64_t size);

/**
 * Method called during an unindexing of a specific file in a chat which was
 * uploaded before.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_file_unindex
 * @param[in,out] file Chat file
 * @param[in] completed Amount of the file being unindexed (in bytes)
 * @param[in] size Full size of the unindexing file (in bytes)
 */
typedef void
(*GNUNET_CHAT_FileUnindexCallback) (void *cls,
                                    struct GNUNET_CHAT_File *file,
                                    uint64_t completed,
                                    uint64_t size);

/**
 * Iterator over chat discourses in a specific chat context.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_context_iterate_discourses
 * @param[in,out] context Chat context
 * @param[in,out] discourse Chat discourse
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_DiscourseCallback) (void *cls,
                                  struct GNUNET_CHAT_Context *context,
                                  struct GNUNET_CHAT_Discourse *discourse);

/**
 * Iterator over chat contacts in a specific chat discourse.
 *
 * @param[in,out] cls Closure from #GNUNET_CHAT_discourse_iterate_contacts
 * @param[in,out] discourse Chat discourse
 * @param[in,out] contact Chat contact
 * @return #GNUNET_YES if we should continue to iterate, #GNUNET_NO otherwise.
 */
typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_DiscourseContactCallback) (void *cls,
                                         struct GNUNET_CHAT_Discourse *discourse,
                                         struct GNUNET_CHAT_Contact *contact);

/**
 * Start a chat handle with a certain configuration.
 *
 * A custom callback for warnings and message events can be provided optionally
 * together with their respective closures.
 *
 * @param[in] cfg Configuration
 * @param[in] msg_cb Callback for message events (optional)
 * @param[in,out] msg_cls Closure for message events (optional)
 * @return Chat handle
 */
struct GNUNET_CHAT_Handle*
GNUNET_CHAT_start (const struct GNUNET_CONFIGURATION_Handle *cfg,
                   GNUNET_CHAT_ContextMessageCallback msg_cb,
                   void *msg_cls);

/**
 * Stops a chat handle closing all its remaining resources and frees the
 * regarding memory.
 *
 * @param[in,out] handle Chat handle
 */
void
GNUNET_CHAT_stop (struct GNUNET_CHAT_Handle *handle);

/**
 * Creates a new chat account to use with a given chat <i>handle</i> under a
 * unique <i>name</i>.
 *
 * If a specific name is already in use of another chat accounts, the function
 * will fail and return #GNUNET_NO.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name Account name
 * @return #GNUNET_OK on success, #GNUNET_NO on failure and otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_account_create (struct GNUNET_CHAT_Handle *handle,
                            const char *name);

/**
 * Deletes an existing chat account of a given chat <i>handle</i> under a
 * unique <i>name</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name Account name
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_account_delete(struct GNUNET_CHAT_Handle *handle,
                           const char *name);

/**
 * Iterates through the accounts of a given chat <i>handle</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] handle Chat handle
 * @param[in] callback Callback for account iteration (optional)
 * @param[in,out] cls Closure for account iteration (optional)
 * @return Amount of accounts iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_iterate_accounts (struct GNUNET_CHAT_Handle *handle,
                              GNUNET_CHAT_AccountCallback callback,
                              void *cls);

/**
 * Searches for an existing chat account of a given chat <i>handle</i> with
 * a unique <i>name</i>.
 *
 * @param[in] handle Chat handle
 * @param[in] name Account name
 * @return Found account by the handle or NULL
 */
struct GNUNET_CHAT_Account*
GNUNET_CHAT_find_account (const struct GNUNET_CHAT_Handle *handle,
                          const char *name);

/**
 * Connects a chat <i>handle</i> to a selected chat <i>account</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] account Chat account
 */
void
GNUNET_CHAT_connect (struct GNUNET_CHAT_Handle *handle,
                     struct GNUNET_CHAT_Account *account);

/**
 * Disconnects a chat <i>handle</i> from the current chat account.
 *
 * @param[in,out] handle Chat handle
 */
void
GNUNET_CHAT_disconnect (struct GNUNET_CHAT_Handle *handle);

/**
 * Returns the connected account of a chat <i>handle</i> for related
 * communication or NULL if no account is set yet.
 *
 * @param[in] handle Chat handle
 * @return Account used by the handle or NULL
 */
struct GNUNET_CHAT_Account*
GNUNET_CHAT_get_connected (const struct GNUNET_CHAT_Handle *handle);

/**
 * Updates a chat handle to renew the used ego to sign sent messages in active
 * chats.
 *
 * Updating the chat handle should only be used if necessary because the usage
 * can require renewed exchanging of GNS entries.
 *
 * @param[in,out] handle Chat handle
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_update (struct GNUNET_CHAT_Handle *handle);

/**
 * Updates the name of a chat handle for related communication.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name New name or NULL
 * @return #GNUNET_YES on success, #GNUNET_NO on failure and #GNUNET_SYSERR if <i>handle</i> is NULL
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_set_name (struct GNUNET_CHAT_Handle *handle,
                      const char *name);

/**
 * Returns the name of a chat handle for related communication or NULL if no
 * name is set.
 *
 * @param[in] handle Chat handle
 * @return Name used by the handle or NULL
 */
const char*
GNUNET_CHAT_get_name (const struct GNUNET_CHAT_Handle *handle);

/**
 * Returns the public key of the used ego to verify signatures of sent messages.
 *
 * @param[in] handle Chat handle
 * @return Public key of the handles ego or NULL
 */
const char*
GNUNET_CHAT_get_key (const struct GNUNET_CHAT_Handle *handle);

/**
 * Updates an attribute of a chat handle for related communication under a given 
 * <i>name</i> and a custom <i>value</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name Attribute name
 * @param[in] value Attribute value
 */
void
GNUNET_CHAT_set_attribute (struct GNUNET_CHAT_Handle *handle,
                           const char *name,
                           const char *value);

/**
 * Deletes an attribute of a chat <i>handle</i> for related communication 
 * under a given <i>name</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] name Attribute name
 */
void
GNUNET_CHAT_delete_attribute (struct GNUNET_CHAT_Handle *handle,
                              const char *name);

/**
 * Calls an optional <i>callback</i> for each attribute of a given chat 
 * <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] callback Callback for attribute iteration (optional)
 * @param[in,out] cls Closure for attribute iteration (optional)
 */
void
GNUNET_CHAT_get_attributes (struct GNUNET_CHAT_Handle *handle,
                            GNUNET_CHAT_AttributeCallback callback,
                            void *cls);

/**
 * Share an attribute of a chat <i>handle</i> under a given <i>name</i>
 * with a specific chat <i>contact</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] contact Chat contact
 * @param[in] name Attribute name
 */
void
GNUNET_CHAT_share_attribute_with (struct GNUNET_CHAT_Handle *handle,
                                  struct GNUNET_CHAT_Contact *contact,
                                  const char *name);

/**
 * Unshare an attribute of a chat <i>handle</i> under a given <i>name</i>
 * from a specific chat <i>contact</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] contact Chat contact
 * @param[in] name Attribute name
 */
void
GNUNET_CHAT_unshare_attribute_from (struct GNUNET_CHAT_Handle *handle,
                                    struct GNUNET_CHAT_Contact *contact,
                                    const char *name);

/**
 * Calls an optional <i>callback</i> for each attribute of a given chat 
 * <i>handle</i> shared with a specific chat <i>contact</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] contact Chat contact
 * @param[in] callback Callback for attribute iteration (optional)
 * @param[in,out] cls Closure for attribute iteration (optional)
 */
void
GNUNET_CHAT_get_shared_attributes (struct GNUNET_CHAT_Handle *handle,
                                   struct GNUNET_CHAT_Contact *contact,
                                   GNUNET_CHAT_ContactAttributeCallback callback,
                                   void *cls);

/**
 * Convert an UTF-8 String to a chat URI which will be newly allocated.
 *
 * @param[in] uri UTF-8 string to parse
 * @param[out] emsg Where to store the parser error message (if any)
 * @return URI on success, NULL on error
 */
struct GNUNET_CHAT_Uri*
GNUNET_CHAT_uri_parse (const char *uri,
                       char **emsg);

/**
 * Convert a chat URI to a UTF-8 String.
 *
 * @param[in] uri Chat URI
 * @return The UTF-8 string representing the URI
 */
char*
GNUNET_CHAT_uri_to_string (const struct GNUNET_CHAT_Uri *uri);

/**
 * Returns the type of a given chat URI.
 *
 * @param[in] uri Chat URI
 * @return The type of URI
 */
enum GNUNET_CHAT_UriType
GNUNET_CHAT_uri_get_type (const struct GNUNET_CHAT_Uri *uri);

/**
 * Free an allocated chat URI.
 *
 * @param[in,out] uri Chat URI
 */
void
GNUNET_CHAT_uri_destroy (struct GNUNET_CHAT_Uri *uri);

/**
 * Opens an empty chat lobby which will expire after a custom <i>delay</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] delay Expiration delay
 * @param[in] callback Callback for the lobby opening
 * @param[in,out] cls Closure for the lobby opening (optional)
 * @return Chat lobby
 */
struct GNUNET_CHAT_Lobby*
GNUNET_CHAT_lobby_open (struct GNUNET_CHAT_Handle *handle,
                        unsigned int delay,
                        GNUNET_CHAT_LobbyCallback callback,
                        void *cls);

/**
 * Closes a chat <i>lobby</i> overriding the expiration to be now.
 *
 * @param[in,out] lobby Chat lobby
 */
void
GNUNET_CHAT_lobby_close (struct GNUNET_CHAT_Lobby *lobby);

/**
 * Joins an open lobby via URI with a given chat <i>handle</i> if it can still
 * be resolved (depends on connection and expiration of the lobby).
 *
 * @param[in,out] handle Chat handle
 * @param[in] uri Chat URI
 */
void
GNUNET_CHAT_lobby_join (struct GNUNET_CHAT_Handle *handle,
                        const struct GNUNET_CHAT_Uri *uri);

/**
 * Requests a file with a given chat <i>handle</i> from a selected chat 
 * <i>uri</i> to potentially download it.
 *
 * @param[in,out] handle Chat handle
 * @param[in] uri Chat URI
 * @return File handle on success, NULL on failure
 */
struct GNUNET_CHAT_File*
GNUNET_CHAT_request_file (struct GNUNET_CHAT_Handle *handle,
                          const struct GNUNET_CHAT_Uri *uri);

/**
 * Uploads a local file specified via its <i>path</i> to share its uri
 * with others afterwards.
 *
 * @param[in,out] handle Chat handle
 * @param[in] path Local file path
 * @param[in] callback Callback for file uploading (optional)
 * @param[in,out] cls Closure for file uploading (optional)
 * @return The file handle on success, NULL on failure
 */
struct GNUNET_CHAT_File*
GNUNET_CHAT_upload_file (struct GNUNET_CHAT_Handle *handle,
                         const char *path,
                         GNUNET_CHAT_FileUploadCallback callback,
                         void *cls);

/**
 * Iterates through the files of a given chat <i>handle</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] handle Chat handle
 * @param[in] callback Callback for file iteration (optional)
 * @param[in,out] cls Closure for file iteration (optional)
 * @return Amount of files iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_iterate_files (struct GNUNET_CHAT_Handle *handle,
                           GNUNET_CHAT_FileCallback callback,
                           void *cls);

/**
 * Sets a custom <i>user pointer</i> to a given chat <i>handle</i> so it can
 * be accessed in all handle related callbacks.
 *
 * @param[in,out] handle Chat handle
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_set_user_pointer (struct GNUNET_CHAT_Handle *handle,
                              void *user_pointer);

/**
 * Returns the custom user pointer of a given chat <i>handle</i> or NULL if it
 * was not set any.
 *
 * @param[in] handle Chat handle
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_get_user_pointer (const struct GNUNET_CHAT_Handle *handle);

/**
 * Iterates through the contacts of a given chat <i>handle</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] handle Chat handle
 * @param[in] callback Callback for contact iteration (optional)
 * @param[in,out] cls Closure for contact iteration (optional)
 * @return Amount of contacts iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_iterate_contacts (struct GNUNET_CHAT_Handle *handle,
                              GNUNET_CHAT_ContactCallback callback,
                              void *cls);

/**
 * Returns the chat contact matching a given chat <i>handle</i>'s current 
 * account.
 *
 * @param[in,out] handle Chat handle
 * @return Chat contact or NULL
 */
struct GNUNET_CHAT_Contact*
GNUNET_CHAT_get_own_contact (struct GNUNET_CHAT_Handle *handle);

/**
 * Returns the provided name of a given <i>account</i> or NULL on failure.
 *
 * @param[in] account Chat account
 * @return Name or NULL
 */
const char*
GNUNET_CHAT_account_get_name (const struct GNUNET_CHAT_Account *account);

/**
 * Calls an optional <i>callback</i> for each attribute of a given chat 
 * <i>account</i> using a chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] account Chat account
 * @param[in] callback Callback for attribute iteration (optional)
 * @param[in,out] cls Closure for attribute iteration (optional)
 */
void
GNUNET_CHAT_account_get_attributes (struct GNUNET_CHAT_Handle *handle,
                                    struct GNUNET_CHAT_Account *account,
                                    GNUNET_CHAT_AccountAttributeCallback callback,
                                    void *cls);

/**
 * Sets a custom <i>user pointer</i> to a given chat <i>account</i> so it can
 * be accessed in account related callbacks.
 *
 * @param[in,out] account Chat account
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_account_set_user_pointer (struct GNUNET_CHAT_Account *account,
                                      void *user_pointer);

/**
 * Returns the custom user pointer of a given <i>contact</i> or NULL if it was
 * not set any.
 *
 * @param[in] account Chat account
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_account_get_user_pointer (const struct GNUNET_CHAT_Account *account);

/**
 * Creates a new group chat to use with a given chat <i>handle</i> with an
 * optional public <i>topic</i>.
 *
 * If a specific <i>topic</i> is used, the created group will be publically
 * available to join for others searching for the used topic. Otherwise the
 * group will be private using a randomly generated key and others can only
 * join the chat via a private invitation.
 *
 * @param[in,out] handle Chat handle
 * @param[in] topic Public topic (optional)
 * @return New group chat
 */
struct GNUNET_CHAT_Group*
GNUNET_CHAT_group_create (struct GNUNET_CHAT_Handle *handle,
                          const char* topic);

/**
 * Iterates through the groups of a given chat <i>handle</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] handle Chat handle
 * @param[in] callback Callback for group iteration (optional)
 * @param[in,out] cls Closure for group iteration (optional)
 * @return Amount of groups iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_iterate_groups (struct GNUNET_CHAT_Handle *handle,
                            GNUNET_CHAT_GroupCallback callback,
                            void *cls);

/**
 * Leaves the private chat with a specific <i>contact</i> and frees the
 * regarding memory of the contact if there remains no common chat with it.
 *
 * @param[in,out] contact Contact
 */
void
GNUNET_CHAT_contact_delete (struct GNUNET_CHAT_Contact *contact);

/**
 * Overrides the name of a given <i>contact</i> with a custom nick <i>name</i>
 * which will be only used locally.
 *
 * @param[in,out] contact Contact
 * @param[in] name Custom nick name
 */
void
GNUNET_CHAT_contact_set_name (struct GNUNET_CHAT_Contact *contact,
                              const char *name);

/**
 * Returns the provided name of a given <i>contact</i> or its custom nick name
 * if it was overriden.
 *
 * @param[in] contact Contact
 * @return Name or custom nick name
 */
const char*
GNUNET_CHAT_contact_get_name (const struct GNUNET_CHAT_Contact *contact);

/**
 * Returns the public key of the used ego by a specific <i>contact</i> to
 * verify signatures of sent messages.
 *
 * @param[in] contact Contact
 * @return Public key of the contacts ego or NULL
 */
const char*
GNUNET_CHAT_contact_get_key (const struct GNUNET_CHAT_Contact *contact);

/**
 * Returns the chat context for a private chat with a given <i>contact</i>.
 *
 * @param[in,out] contact Contact
 * @return Chat context
 */
struct GNUNET_CHAT_Context*
GNUNET_CHAT_contact_get_context (struct GNUNET_CHAT_Contact *contact);

/**
 * Sets a custom <i>user pointer</i> to a given <i>contact</i> so it can be
 * accessed in contact related callbacks.
 *
 * @param[in,out] contact Contact
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_contact_set_user_pointer (struct GNUNET_CHAT_Contact *contact,
                                      void *user_pointer);

/**
 * Returns the custom user pointer of a given <i>contact</i> or NULL if it was
 * not set any.
 *
 * @param[in] contact Contact
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_contact_get_user_pointer (const struct GNUNET_CHAT_Contact *contact);

/**
 * Returns if a given <i>contact</i> is owned by the current account and whether
 * it has sent messages with.
 *
 * @param[in] contact Contact
 * @return #GNUNET_YES if the contact is owned, otherwise# GNUNET_NO
 *         and #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_contact_is_owned (const struct GNUNET_CHAT_Contact *contact);

/**
 * Blocks or unblocks a given <i>contact</i> by the current account.
 *
 * @param[in,out] contact Contact
 * @param[in] blocked #GNUNET_YES to block, #GNUNET_NO to unblock
 */
void
GNUNET_CHAT_contact_set_blocked (struct GNUNET_CHAT_Contact *contact,
                                 enum GNUNET_GenericReturnValue blocked);

/**
 * Returns if a given <i>contact</i> is blocked by the current account and
 * whether messages of it should be filtered.
 *
 * @param[in] contact Contact
 * @return #GNUNET_YES if the contact is blocked, #GNUNET_SYSERR on failure and 
 *         #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_contact_is_blocked (const struct GNUNET_CHAT_Contact *contact);

/**
 * Tags a given <i>contact</i> by the current account with a specific
 * <i>tag</i>.
 *
 * @param[in,out] contact Contact
 * @param[in] tag Tag
 */
void
GNUNET_CHAT_contact_tag (struct GNUNET_CHAT_Contact *contact,
                         const char *tag);

/**
 * Untags a given <i>contact</i> by the current account with a specific
 * <i>tag</i>.
 *
 * @param[in,out] contact Contact
 * @param[in] tag Tag
 */
void
GNUNET_CHAT_contact_untag (struct GNUNET_CHAT_Contact *contact,
                           const char *tag);

/**
 * Returns if a given <i>contact</i> is tagged by the current account with
 * a specific <i>tag</i>.
 *
 * @param[in] contact Contact
 * @param[in] tag Tag
 * @return #GNUNET_YES if the contact is tagged, #GNUNET_SYSERR on failure and 
 *         #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_contact_is_tagged (const struct GNUNET_CHAT_Contact *contact,
                               const char *tag);

/**
 * Calls an optional <i>callback</i> for each tag of a given chat 
 * <i>contact</i>.
 *
 * @param[in] contact Chat contact
 * @param[in] callback Callback for tag iteration (optional)
 * @param[in,out] cls Closure for tag iteration (optional)
 * @return Amount of tags iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_contact_iterate_tags (struct GNUNET_CHAT_Contact *contact,
                                  GNUNET_CHAT_ContactTagCallback callback,
                                  void *cls);

/**
 * Calls an optional <i>callback</i> for each attribute of a given chat 
 * <i>contact</i>.
 *
 * @param[in,out] contact Chat contact
 * @param[in] callback Callback for attribute iteration (optional)
 * @param[in,out] cls Closure for attribute iteration (optional)
 */
void
GNUNET_CHAT_contact_get_attributes (struct GNUNET_CHAT_Contact *contact,
                                    GNUNET_CHAT_ContactAttributeCallback callback,
                                    void *cls);

/**
 * Leaves a specific <i>group</i> chat and frees its memory if it is not shared
 * with other groups or contacts.
 *
 * @param[in,out] group Group
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_group_leave (struct GNUNET_CHAT_Group *group);

/**
 * Sets the name of a given <i>group</i> to a custom nick <i>name</i>
 * which will be only used locally.
 *
 * @param[in,out] group Group
 * @param[in] name Custom nick name
 */
void
GNUNET_CHAT_group_set_name (struct GNUNET_CHAT_Group *group,
                            const char *name);

/**
 * Returns the custom nick name of a given <i>group</i> if it was overriden.
 *
 * @param[in] group Group
 * @return Custom nick name or NULL
 */
const char*
GNUNET_CHAT_group_get_name (const struct GNUNET_CHAT_Group *group);

/**
 * Sets a custom <i>user pointer</i> to a given <i>group</i> so it can be
 * accessed in group related callbacks.
 *
 * @param[in,out] group Group
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_group_set_user_pointer (struct GNUNET_CHAT_Group *group,
                                    void *user_pointer);

/**
 * Returns the custom user pointer of a given <i>group</i> or NULL if it was
 * not set any.
 *
 * @param[in] group Group
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_group_get_user_pointer (const struct GNUNET_CHAT_Group *group);

/**
 * Invites a specific <i>contact</i> to a given <i>group</i> via a privately
 * sent invitation.
 *
 * @param[in,out] group Group
 * @param[in,out] contact Contact
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_group_invite_contact (struct GNUNET_CHAT_Group *group,
                                  struct GNUNET_CHAT_Contact *contact);

/**
 * Iterates through the contacts of a given <i>group</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] group Group
 * @param[in] callback Callback for contact iteration (optional)
 * @param[in,out] cls Closure for contact iteration (optional)
 * @return Amount of contacts iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_group_iterate_contacts (struct GNUNET_CHAT_Group *group,
                                    GNUNET_CHAT_GroupContactCallback callback,
                                    void *cls);

/**
 * Sets a custom <i>user pointer</i> to a given <i>member</i> relative to a
 * <i>group</i> so it can be accessed in member related callbacks.
 *
 * @param[in,out] group Chat group
 * @param[in] member Contact
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_member_set_user_pointer (struct GNUNET_CHAT_Group *group,
                                     const struct GNUNET_CHAT_Contact *member,
                                     void *user_pointer);

/**
 * Returns the custom user pointer of a given <i>member</i> relative to a
 * <i>group</i> or NULL if it was not set any.
 *
 * @param[in] group Chat group
 * @param[in] member Contact
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_member_get_user_pointer (const struct GNUNET_CHAT_Group *group,
                                     const struct GNUNET_CHAT_Contact *member);

/**
 * Returns the chat context for a chat with a given <i>group</i>.
 *
 * @param[in,out] group Group
 * @return Chat context
 */
struct GNUNET_CHAT_Context*
GNUNET_CHAT_group_get_context (struct GNUNET_CHAT_Group *group);

/**
 * Returns the current status of a given <i>context</i> whether it is usable
 * or not to communicate with other contacts.
 *
 * @param[in] context Context
 * @return #GNUNET_OK if usable, #GNUNET_NO if the context has been requested,
 *         #GNUNET_SYSERR otherwise.
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_get_status (struct GNUNET_CHAT_Context *context);

/**
 * Requests a <i>context</i> to get established between all required contacts.
 * The current status of this request can be tracked via
 * #GNUNET_CHAT_context_get_status() and will only change through the
 * receivement of new messages.
 *
 * @param[in,out] context Context
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_request (struct GNUNET_CHAT_Context *context);

/**
 * Returns the chat contact which uses a given <i>context</i>.
 *
 * @param[in] context Context
 * @return Chat contact
 */
struct GNUNET_CHAT_Contact*
GNUNET_CHAT_context_get_contact (struct GNUNET_CHAT_Context *context);

/**
 * Returns the chat group which uses a given <i>context</i>.
 *
 * @param[in] context Context
 * @return Chat group
 */
struct GNUNET_CHAT_Group*
GNUNET_CHAT_context_get_group (struct GNUNET_CHAT_Context *context);

/**
 * Sets a custom <i>user pointer</i> to a given chat <i>context</i> so it can
 * be accessed in chat context related callbacks.
 *
 * @param[in,out] context Chat context
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_context_set_user_pointer (struct GNUNET_CHAT_Context *context,
                                      void *user_pointer);

/**
 * Returns the custom user pointer of a given chat <i>context</i> or NULL if it
 * was not set any.
 *
 * @param[in] context Chat context
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_context_get_user_pointer (const struct GNUNET_CHAT_Context *context);

/**
 * Sends a selected <i>text</i> into a given chat <i>context</i>.
 *
 * @param[in,out] context Chat context
 * @param[in] text Text
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_send_text (struct GNUNET_CHAT_Context *context,
                               const char *text);

/**
 * Sends a read receipt depending on a selected <i>message</i> into a given
 * chat <i>context</i>.
 *
 * @param[in,out] context Chat context
 * @param[in,out] message Message (optional)
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_send_read_receipt (struct GNUNET_CHAT_Context *context,
                                       struct GNUNET_CHAT_Message *message);

/**
 * Uploads a local file specified via its <i>path</i> using symmetric encryption
 * and shares the regarding information to download and decrypt it in a given
 * chat <i>context</i>.
 *
 * @param[in,out] context Chat context
 * @param[in] path Local file path
 * @param[in] callback Callback for file uploading (optional)
 * @param[in,out] cls Closure for file uploading (optional)
 * @return The file handle on success, NULL on failure
 */
struct GNUNET_CHAT_File*
GNUNET_CHAT_context_send_file (struct GNUNET_CHAT_Context *context,
                               const char *path,
                               GNUNET_CHAT_FileUploadCallback callback,
                               void *cls);

/**
 * Shares the information to download and decrypt a specific <i>file</i> from
 * another chat in a given chat <i>context</i>.
 *
 * @param[in,out] context Chat context
 * @param[in,out] file File handle
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_share_file (struct GNUNET_CHAT_Context *context,
                                struct GNUNET_CHAT_File *file);

/**
 * Sends a tag message targeting a selected <i>message</i> into a given
 * chat <i>context</i> with a given <i>tag</i> value.
 *
 * @param[in,out] context Chat context
 * @param[in,out] message Message
 * @param[in] tag Tag value
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_context_send_tag (struct GNUNET_CHAT_Context *context,
                              struct GNUNET_CHAT_Message *message,
                              const char *tag);

/**
 * Opens a chat discourse under a specific <i>id</i> in a given chat 
 * <i>context</i> to send data live to other contacts.
 *
 * @param[in,out] context Chat context
 * @param[in] id Discourse id
 * @return Chat discourse
 */
struct GNUNET_CHAT_Discourse*
GNUNET_CHAT_context_open_discourse (struct GNUNET_CHAT_Context *context,
                                    const struct GNUNET_CHAT_DiscourseId *id);

/**
 * Iterates through the contacts of a given chat <i>context</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] context Chat context
 * @param[in] callback Callback for contact iteration (optional)
 * @param[in,out] cls Closure for contact iteration (optional)
 * @return Amount of contacts iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_context_iterate_messages (struct GNUNET_CHAT_Context *context,
                                      GNUNET_CHAT_ContextMessageCallback callback,
                                      void *cls);

/**
 * Iterates through the files of a given chat <i>context</i> with a selected
 * callback and custom closure.
 *
 * @param[in,out] context Chat context
 * @param[in] callback Callback for file iteration (optional)
 * @param[in,out] cls Closure for file iteration (optional)
 * @return Amount of files iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_context_iterate_files (struct GNUNET_CHAT_Context *context,
                                   GNUNET_CHAT_ContextFileCallback callback,
                                   void *cls);

/**
 * Iterates through the discourses of a given chat <i>context</i> with a 
 * selected callback and custom closure.
 *
 * @param[in,out] context Chat context
 * @param[in] callback Callback for file iteration (optional)
 * @param[in,out] cls Closure for file iteration (optional)
 * @return Amount of discourses iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_context_iterate_discourses (struct GNUNET_CHAT_Context *context,
                                        GNUNET_CHAT_DiscourseCallback callback,
                                        void *cls);

/**
 * Returns the kind of a given <i>message</i> to determine its content and
 * related usage.
 *
 * @param[in] message Message
 * @return The kind of message
 */
enum GNUNET_CHAT_MessageKind
GNUNET_CHAT_message_get_kind (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the timestamp of a given <i>message</i> in absolute measure.
 *
 * @param[in] message Message
 * @return The timestamp of message
 */
time_t
GNUNET_CHAT_message_get_timestamp (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the contact of the sender from a given <i>message</i>.
 *
 * @param[in] message Message
 * @return Contact of the messages sender
 */
struct GNUNET_CHAT_Contact*
GNUNET_CHAT_message_get_sender (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the contact of the recipient from a given <i>message</i>.
 *
 * @param[in] message Message
 * @return Contact of the messages recipient or NULL
 */
struct GNUNET_CHAT_Contact*
GNUNET_CHAT_message_get_recipient (const struct GNUNET_CHAT_Message *message);

/**
 * Returns #GNUNET_YES if the <i>message</i> was sent by the related chat 
 * handle, otherwise it returns #GNUNET_NO.
 *
 * @param[in] message Message
 * @return #GNUNET_YES if the message was sent, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_sent (const struct GNUNET_CHAT_Message *message);

/**
 * Returns #GNUNET_YES if the <i>message</i> was received privately encrypted
 * by the related chat handle, otherwise it returns #GNUNET_NO.
 *
 * @param[in] message Message
 * @return #GNUNET_YES if the message was privately received,
 * 	   otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_private (const struct GNUNET_CHAT_Message *message);

/**
 * Returns #GNUNET_YES if the <i>message</i> was received recently by related 
 * chat handle, otherwise it returns #GNUNET_NO.
 *
 * @param[in] message Message
 * @return #GNUNET_YES if the message was received recently,
 *     otherwise #GNUNET_NO 
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_recent (const struct GNUNET_CHAT_Message *message);

/**
 * Returns #GNUNET_YES if the <i>message</i> was received because of an 
 * update by related chat handle, otherwise it returns #GNUNET_NO.
 *
 * @param[in] message Message
 * @return #GNUNET_YES if the message was received to update
 *     a previous message, otherwise #GNUNET_NO 
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_update (const struct GNUNET_CHAT_Message *message);

/**
 * Returns #GNUNET_YES if the <i>message</i> was received because of a 
 * deletion by related chat handle or if it has been deleted internally, 
 * otherwise it returns #GNUNET_NO.
 *
 * @param[in] message Message
 * @return #GNUNET_YES if the message was received to delete
 *     a previous message, otherwise #GNUNET_NO 
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_deleted (const struct GNUNET_CHAT_Message *message);

/**
 * Returns #GNUNET_YES if the <i>message</i> was tagged with a custom
 * <i>tag</i> value, otherwise it returns #GNUNET_NO.
 *
 * @param[in] message Message
 * @return #GNUNET_YES if the message was tagged with
 *     the given tag value, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_is_tagged (const struct GNUNET_CHAT_Message *message,
                               const char *tag);

/**
 * Iterates through the contacts of the context related to a given chat
 * <i>message</i> to check whether it was received by each of the contacts.
 *
 * @param[in,out] message Message
 * @param[in] callback Callback for contact iteration (optional)
 * @param[in,out] cls Closure for contact iteration (optional)
 * @return Amount of contacts iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_message_get_read_receipt (struct GNUNET_CHAT_Message *message,
                                      GNUNET_CHAT_MessageReadReceiptCallback callback,
                                      void *cls);

/**
 * Returns the text of a given <i>message</i> if its kind is
 * #GNUNET_CHAT_KIND_TEXT or #GNUNET_CHAT_KIND_WARNING,
 * otherwise it returns NULL.
 *
 * @param[in] message Message
 * @return The text of message or NULL
 */
const char*
GNUNET_CHAT_message_get_text (const struct GNUNET_CHAT_Message *message);

/**
 * Sets a custom <i>user pointer</i> to a given <i>message</i> so it can
 * be accessed in message related callbacks.
 *
 * @param[in,out] message Message
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_message_set_user_pointer (struct GNUNET_CHAT_Message *message,
                                      void *user_pointer);

/**
 * Returns the custom user pointer of a given <i>message</i> or NULL if it
 * was not set any.
 *
 * @param[in] message Message
 * @return Custom user pointer
 */
void*
GNUNET_CHAT_message_get_user_pointer (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the account of a given <i>message</i> which is either 
 * its sender or target of the message depending on the kind of
 * the message, otherwise it returns NULL.
 *
 * @param[in] message Message
 * @return The account of message or NULL
 */
struct GNUNET_CHAT_Account*
GNUNET_CHAT_message_get_account (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the file handle of a given <i>message</i> if its kind is
 * #GNUNET_CHAT_KIND_FILE, otherwise it returns NULL.
 *
 * @param[in] message Message
 * @return The file handle of message or NULL
 */
struct GNUNET_CHAT_File*
GNUNET_CHAT_message_get_file (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the invitation of a given <i>message</i> if its kind is
 * #GNUNET_CHAT_KIND_INVITATION, otherwise it returns NULL.
 *
 * @param[in] message Message
 * @return The invitation of message or NULL
 */
struct GNUNET_CHAT_Invitation*
GNUNET_CHAT_message_get_invitation (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the discourse of a given <i>message</i> if its kind is
 * #GNUNET_CHAT_KIND_DISCOURSE or #GNUNET_CHAT_KIND_DATA,
 * otherwise it returns NULL.
 *
 * @param[in] message Message
 * @return The discourse of message or NULL
 */
struct GNUNET_CHAT_Discourse*
GNUNET_CHAT_message_get_discourse (const struct GNUNET_CHAT_Message *message);

/**
 * Returns the target message of an operation represented by a given
 * <i>message</i> if its kind is #GNUNET_CHAT_KIND_DELETION, otherwise it
 * returns NULL.
 *
 * @param[in] message Message
 * @return The target of message or NULL
 */
struct GNUNET_CHAT_Message*
GNUNET_CHAT_message_get_target (const struct GNUNET_CHAT_Message *message);

/**
 * Deletes a given <i>message</i> with a specific relative <i>delay</i>
 * in seconds.
 *
 * @param[in,out] message Message
 * @param[in] delay Relative delay
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_delete (struct GNUNET_CHAT_Message *message,
                            unsigned int delay);

/**
 * Iterates through the tag messages in the context of a given 
 * <i>message</i>.
 *
 * @param[in,out] message Message
 * @param[in] callback Callback for tag message iteration (optional)
 * @param[in,out] cls Closure for tag message iteration (optional)
 * @return Amount of tag messages iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_message_iterate_tags (struct GNUNET_CHAT_Message *message,
                                  GNUNET_CHAT_MessageCallback callback,
                                  void *cls);

/**
 * Returns the amount of data from a given chat <i>message</i> if its 
 * kind is #GNUNET_CHAT_KIND_DATA, otherwise it returns zero.
 *
 * @param[in] message Message
 * @return The amount of data or zero
 */
uint64_t
GNUNET_CHAT_message_available (const struct GNUNET_CHAT_Message *message);

/**
 * Reads the data from a given chat <i>message</i> if its kind is
 * #GNUNET_CHAT_KIND_DATA into a specific <i>data</i> buffer
 * up to a selected <i>size</i>.
 *
 * @param[in] message Message
 * @param[out] data Data buffer
 * @param[in] size Data size
 * @return #GNUNET_OK on success, #GNUNET_NO if there's missing data 
 *         to read, otherwise #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_read (const struct GNUNET_CHAT_Message *message,
                          char *data,
                          uint64_t size);

/**
 * Feeds the data from a given chat <i>message</i> if its kind is
 * #GNUNET_CHAT_KIND_DATA into a specific file descriptor (of pipe for 
 * example).
 *
 * @param[in] message Message
 * @param[in] fd File descriptor
 * @return #GNUNET_OK on success, #GNUNET_NO if there's not enough data 
 *         to read, otherwise #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_message_feed (const struct GNUNET_CHAT_Message *message,
                          int fd);

/**
 * Returns the name of a given <i>file</i> handle.
 *
 * @param[in] file File handle
 * @return The file name of file
 */
const char*
GNUNET_CHAT_file_get_name (const struct GNUNET_CHAT_File *file);

/**
 * Returns the hash of a given <i>file</i> handle.
 *
 * @param[in] file File handle
 * @return The hash of file
 */
const char*
GNUNET_CHAT_file_get_hash (const struct GNUNET_CHAT_File *file);

/**
 * Returns the actual file size of a given <i>file</i> handle.
 *
 * @param[in] file File handle
 * @return The file size of file
 */
uint64_t
GNUNET_CHAT_file_get_size (const struct GNUNET_CHAT_File *file);

/**
 * Returns the local file size of a given <i>file</i> handle.
 *
 * This can be less than the actual size of the file once its download
 * has been completed!
 *
 * @param[in] file File handle
 * @return The local file size of file
 */
uint64_t
GNUNET_CHAT_file_get_local_size (const struct GNUNET_CHAT_File *file);

/**
 * Returns a new allocated uri to access the content of a given 
 * <i>file</i> handle.
 *
 * @param[in] file File handle
 * @return URI on success, NULL on error
 */
struct GNUNET_CHAT_Uri*
GNUNET_CHAT_file_get_uri (const struct GNUNET_CHAT_File *file);

/**
 * Returns if a given <i>file</i> handle is currently uploading.
 *
 * @param[in] file File handle
 * @return #GNUNET_YES during active upload, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_uploading (const struct GNUNET_CHAT_File *file);

/**
 * Returns if a given <i>file</i> handle is currently ready
 * for previewing its content.
 *
 * @param[in] file File handle
 * @return #GNUNET_YES when ready, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_ready (const struct GNUNET_CHAT_File *file);

/**
 * Returns the temporary file name of the decrypted file preview
 * of a given <i>file</i> handle.
 *
 * This can only be used when the file handle is ready to preview!
 * @see GNUNET_CHAT_file_is_ready()
 *
 * @param[in,out] file File handle
 * @return The temporary file name or NULL on error
 */
const char*
GNUNET_CHAT_file_open_preview (struct GNUNET_CHAT_File *file);

/**
 * Deletes the temporary decrypted file preview of a given <i>file</i>
 * handle.
 *
 * This can only be used when the file handle is ready to preview!
 * @see GNUNET_CHAT_file_is_ready()
 *
 * @param[out] file File handle
 */
void
GNUNET_CHAT_file_close_preview (struct GNUNET_CHAT_File *file);

/**
 * Sets a custom <i>user pointer</i> to a given <i>file</i> handle so it can
 * be accessed in file related callbacks.
 *
 * @param[in,out] file File handle
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_file_set_user_pointer (struct GNUNET_CHAT_File *file,
                                   void *user_pointer);

/**
 * Returns the custom user pointer of a given <i>file</i> handle or NULL if it
 * was not set any.
 *
 * @param[in] file File handle
 * @return Custom user pointer
 */
void*
GNUNET_CHAT_file_get_user_pointer (const struct GNUNET_CHAT_File *file);

/**
 * Returns if a given <i>file</i> handle is currently downloading.
 *
 * @param[in] file File handle
 * @return #GNUNET_YES during active download, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_downloading (const struct GNUNET_CHAT_File *file);

/**
 * Starts downloading a file from a given <i>file</i> handle and sets up a
 * selected callback and custom closure for its progress.
 *
 * @param[in,out] file File handle
 * @param[in] callback Callback for file downloading (optional)
 * @param[in,out] cls Closure for file downloading (optional)
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_start_download (struct GNUNET_CHAT_File *file,
                                 GNUNET_CHAT_FileDownloadCallback callback,
                                 void *cls);

/**
 * Pause downloading a file from a given <i>file</i> handle.
 *
 * @param[in,out] file File handle
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_pause_download (struct GNUNET_CHAT_File *file);

/**
 * Resume downloading a file from a given <i>file</i> handle.
 *
 * @param[in,out] file File handle
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_resume_download (struct GNUNET_CHAT_File *file);

/**
 * Stops downloading a file from a given <i>file</i> handle.
 *
 * @param[in,out] file File handle
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_stop_download (struct GNUNET_CHAT_File *file);

/**
 * Returns if a given <i>file</i> handle is currently unindexing.
 *
 * @param[in] file File handle
 * @return #GNUNET_YES during active unindexing, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_is_unindexing (const struct GNUNET_CHAT_File *file);

/**
 * Unindexes an uploaded file from a given <i>file</i> handle with a selected
 * callback and a custom closure.
 *
 * @param[in,out] file File handle
 * @param[in] callback Callback for file unindexing (optional)
 * @param[in,out] cls Closure for file unindexing (optional)
 * @return #GNUNET_OK on success, #GNUNET_SYSERR on failure
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_file_unindex (struct GNUNET_CHAT_File *file,
                          GNUNET_CHAT_FileUnindexCallback callback,
                          void *cls);

/**
 * Accepts a given chat <i>invitation</i> to enter another chat.
 *
 * @param[in,out] invitation Chat invitation
 */
void
GNUNET_CHAT_invitation_accept (struct GNUNET_CHAT_Invitation *invitation);

/**
 * Rejects a given chat <i>invitation</i> to enter another chat.
 *
 * @param[in,out] invitation Chat invitation
 */
void
GNUNET_CHAT_invitation_reject (struct GNUNET_CHAT_Invitation *invitation);

/**
 * Returns if a given <i>invitation</i> got accepted.
 *
 * @param[in] invitation Chat invitation
 * @return #GNUNET_YES if accepted, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_invitation_is_accepted (const struct GNUNET_CHAT_Invitation *invitation);

/**
 * Returns if a given <i>invitation</i> got rejected.
 *
 * @param[in] invitation Chat invitation
 * @return #GNUNET_YES if rejected, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_invitation_is_rejected (const struct GNUNET_CHAT_Invitation *invitation);

/**
 * Returns whether the intend from a given 
 * <i>invitation</i>. is to open a direct chat or
 * a group chat.
 *
 * @param[in] invitation Chat invitation
 * @return #GNUNET_YES if it is a direct chat request, #GNUNET_NO otherwise
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_invitation_is_direct (const struct GNUNET_CHAT_Invitation *invitation);

/**
 * Returns the discourse id of a given chat <i>discourse</i>.
 *
 * @param[in] discourse Chat discourse
 * @return Discourse id
 */
const struct GNUNET_CHAT_DiscourseId*
GNUNET_CHAT_discourse_get_id (const struct GNUNET_CHAT_Discourse *discourse);

/**
 * Returns whether a chat <i>discourse</i> is currently open.
 *
 * @param[in] discourse Chat discourse
 * @return #GNUNET_YES if the discourse is open, #GNUNET_SYSERR on failure and 
 *         #GNUNET_NO otherwise.
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_discourse_is_open (const struct GNUNET_CHAT_Discourse *discourse);

/**
 * Sets a custom <i>user pointer</i> to a given chat <i>discourse</i> so it can
 * be accessed in chat discourse related callbacks.
 *
 * @param[in,out] discourse Chat discourse
 * @param[in] user_pointer Custom user pointer
 */
void
GNUNET_CHAT_discourse_set_user_pointer (struct GNUNET_CHAT_Discourse *discourse,
                                        void *user_pointer);

/**
 * Returns the custom user pointer of a given chat <i>discourse</i> or NULL if it
 * was not set any.
 *
 * @param[in] discourse Chat discourse
 * @return Custom user pointer or NULL
 */
void*
GNUNET_CHAT_discourse_get_user_pointer (const struct GNUNET_CHAT_Discourse *discourse);

/**
 * Closes a given chat <i>discourse</i> to stop receiving any data messages
 * from the specific discourse.
 *
 * @param[in,out] discourse Chat discourse
 */
void
GNUNET_CHAT_discourse_close (struct GNUNET_CHAT_Discourse *discourse);

/**
 * Sends messages to a given chat <i>discourse</i> containing the
 * specified <i>data</i> of a custom <i>size</i> in bytes.
 *
 * @param[in,out] discourse Chat discourse
 * @param[in] data Data buffer
 * @param[in] size Data size
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
GNUNET_CHAT_discourse_write (struct GNUNET_CHAT_Discourse *discourse,
                             const char *data,
                             uint64_t size);

/**
 * Returns a file descriptor of a pipe to write data via IPC
 * into the given chat <i>discourse</i>.
 *
 * @param[in] discourse Chat discourse
 * @return File descriptor of pipe end or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_discourse_get_fd (const struct GNUNET_CHAT_Discourse *discourse);

/**
 * Iterates through the subscribed chat contacts of a given chat <i>discourse</i> 
 * with a selected callback and custom closure.
 *
 * @param[in,out] discourse Chat discourse
 * @param[in] callback Callback for contact iteration (optional)
 * @param[in,out] cls Closure for contact iteration (optional)
 * @return Amount of contacts iterated or #GNUNET_SYSERR on failure
 */
int
GNUNET_CHAT_discourse_iterate_contacts (struct GNUNET_CHAT_Discourse *discourse,
                                        GNUNET_CHAT_DiscourseContactCallback callback,
                                        void *cls);

/**@}*/

#endif /* GNUNET_CHAT_LIB_H_ */
