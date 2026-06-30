/*
   This file is part of GNUnet.
   Copyright (C) 2021--2024 GNUnet e.V.

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
 * @file gnunet_chat_util.h
 */

#ifndef GNUNET_CHAT_UTIL_H_
#define GNUNET_CHAT_UTIL_H_

#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_lib.h"

/**
 * Enum for the types of chat contexts.
 */
enum GNUNET_CHAT_ContextType
{
  /**
   * Contact context type
   */
  GNUNET_CHAT_CONTEXT_TYPE_CONTACT = 1,/**< GNUNET_CHAT_CONTEXT_TYPE_CONTACT */

  /**
   * Group context type
   */
  GNUNET_CHAT_CONTEXT_TYPE_GROUP   = 2,/**< GNUNET_CHAT_CONTEXT_TYPE_GROUP */

  /**
   * Unknown context type
   */
  GNUNET_CHAT_CONTEXT_TYPE_UNKNOWN = 0 /**< GNUNET_CHAT_CONTEXT_TYPE_UNKNOWN */
};

/**
 * Converts a unique messenger contact, being consistent <i>member</i>
 * of multiple messenger rooms via memory consistency, into a short
 * hash variant for map access as key.
 *
 * @param[in] member Messenger contact
 * @param[out] shorthash Short hash
 */
void
util_shorthash_from_member (const struct GNUNET_MESSENGER_Contact *member,
                            struct GNUNET_ShortHashCode *shorthash);

/**
 * Converts a discourse id into a short hash variant for map access 
 * as key.
 *
 * @param[in] id Discourse id
 * @param[out] shorthash Short hash
 */
void
util_shorthash_from_discourse_id (const struct GNUNET_CHAT_DiscourseId *id,
                                  struct GNUNET_ShortHashCode *shorthash);

/**
 * Converts a short hash variant into a discourse id.
 *
 * @param[in] shorthash Short hash
 * @param[out] id Discourse id
 */
void
util_discourse_id_from_shorthash (const struct GNUNET_ShortHashCode *shorthash,
                                  struct GNUNET_CHAT_DiscourseId *id);

/**
 * Updates the stored content of a <i>field</i> with
 * a given <i>name</i>.
 *
 * @param[in] name Name
 * @param[out] field String field
 */
void
util_set_name_field (const char *name,
                     char **field);

/**
 * Generates the <i>hash</i> of a file under a given
 * <i>filename</i>.
 *
 * @param[in] filename File name
 * @param[out] hash Hash of file
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
util_hash_file (const char *filename,
                struct GNUNET_HashCode *hash);

/**
 * Encrypts a file inplace under a given <i>filename</i>
 * with a selected symmetric <i>key</i> and its <i>hash</i>
 * as initialization vector.
 *
 * @param[in] filename File name
 * @param[in] hash Hash of file
 * @param[in] key Symmetric key
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
util_encrypt_file (const char *filename,
                   const struct GNUNET_HashCode *hash,
                   const struct GNUNET_CRYPTO_SymmetricSessionKey *key);

/**
 * Decrypts a file inplace under a given <i>filename</i>
 * with a selected symmetric <i>key</i> and its <i>hash</i>
 * as parameter for the initialization vector and comparison
 * to verify success.
 *
 * @param[in] filename File name
 * @param[in] hash Hash of file
 * @param[in] key Symmetric key
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
util_decrypt_file (const char *filename,
                   const struct GNUNET_HashCode *hash,
                   const struct GNUNET_CRYPTO_SymmetricSessionKey *key);

/**
 * Append the path of a <i>directory</i> and a custom
 * subdirectory name to a composed <i>filename</i>.
 *
 * @param[in] directory Directory path
 * @param[in] subdir Subdirectory name
 * @param[out] filename Filename
 * @return Number of bytes in filename excluding 0-termination
 */
int
util_get_dirname (const char *directory,
                  const char *subdir,
                  char **filename);

/**
 * Append the path of a <i>directory</i>, a custom
 * subdirectory name and a <i>hash</i> to a composed
 * <i>filename</i>.
 *
 * @param[in] directory Directory path
 * @param[in] subdir Subdirectory name
 * @param[in] hash Hash
 * @param[out] filename Filename
 * @return Number of bytes in filename excluding 0-termination
 */
int
util_get_filename (const char *directory,
                   const char *subdir,
                   const struct GNUNET_HashCode *hash,
                   char **filename);

/**
 * Allocates a new string representing the lower case versionn
 * of a given <i>name</i> to work properly with the EGO naming
 * scheme for example.
 *
 * @param[in] name Name
 * @return Lower case name
 */
char*
util_get_lower(const char *name);

/**
 * Construct a composed <i>label</i> from a given context
 * <i>type</i> and the <i>hash</i> of the contexts room.
 *
 * @param[in] type Chat context type
 * @param[in] hash Hash of room
 * @param[out] label Namestore label
 * @return Number of bytes in label excluding 0-termination
 */
int
util_get_context_label (enum GNUNET_CHAT_ContextType type,
                        const struct GNUNET_HashCode *hash,
                        char **label);

/**
 * Extract the chat context type from a used <i>label</i> by
 * a given context with a certain <i>hash</i> of its room.
 *
 * @param[in] label Namestore label
 * @param[in] hash Hash of room
 * @return Chat context type or #GNUNET_CHAT_CONTEXT_TYPE_UNKNOWN
 */
enum GNUNET_CHAT_ContextType
util_get_context_label_type (const char *label,
			                       const struct GNUNET_HashCode *hash);

/**
 * Provide a standardized <i>name</i> for a lobby using
 * a given <i>hash</i> of its internal room.
 *
 * @param[in] hash Hash of room
 * @param[out] name Name of lobby
 * @return Number of bytes in name excluding 0-termination
 */
int
util_lobby_name (const struct GNUNET_HashCode *hash,
		             char **name);

/**
 * Check whether an identity <i>name</i> could be a 
 * standardized name for a lobby.
 * @see util_lobby_name()
 *
 * @param[in] name Identity name
 * @return #GNUNET_YES if it might be a lobby name,
 * otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
util_is_lobby_name(const char *name);

/**
 * Return the chat related kind for a messages original kind
 * from the service. It will return #GNUNET_CHAT_KIND_UNKNOWN
 * in case the message kind is not supported.
 *
 * @param[in] kind Messenger service message kind
 * @return Chat message kind or #GNUNET_CHAT_KIND_UNKNOWN_
 */
enum GNUNET_CHAT_MessageKind
util_message_kind_from_kind (enum GNUNET_MESSENGER_MessageKind kind);

#endif /* GNUNET_CHAT_UTIL_H_ */
