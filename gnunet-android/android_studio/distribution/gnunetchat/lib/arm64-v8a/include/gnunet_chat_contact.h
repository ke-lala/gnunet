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
 * @file gnunet_chat_contact.h
 */

#ifndef GNUNET_CHAT_CONTACT_H_
#define GNUNET_CHAT_CONTACT_H_

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_lib.h"

struct GNUNET_CHAT_Handle;
struct GNUNET_CHAT_Contact;
struct GNUNET_CHAT_Context;
struct GNUNET_CHAT_Ticket;

struct GNUNET_CHAT_InternalTickets
{
  struct GNUNET_CHAT_Ticket *ticket;

  struct GNUNET_CHAT_InternalTickets *next;
  struct GNUNET_CHAT_InternalTickets *prev;
};

struct GNUNET_CHAT_Contact
{
  struct GNUNET_CHAT_Handle *handle;
  struct GNUNET_CHAT_Context *context;

  struct GNUNET_SCHEDULER_Task *destruction;

  const struct GNUNET_MESSENGER_Contact *member;
  struct GNUNET_CONTAINER_MultiHashMap *joined;

  struct GNUNET_CHAT_InternalTickets *tickets_head;
  struct GNUNET_CHAT_InternalTickets *tickets_tail;

  char *public_key;
  void *user_pointer;

  enum GNUNET_GenericReturnValue owned;
};

/**
 * Creates a chat contact using a given messenger <i>contact</i>
 * with a selected chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in] member Messenger contact
 * @return New chat contact
 */
struct GNUNET_CHAT_Contact*
contact_create_from_member (struct GNUNET_CHAT_Handle *handle,
			                      const struct GNUNET_MESSENGER_Contact *member);

/**
 * Updates the latest <i>hash</i> of a join message from a given
 * chat <i>contact</i> in a current chat <i>context</i>.
 *
 * @param[in,out] contact Chat contact
 * @param[in,out] context Chat context
 * @param[in] hash Join message hash
 * @param[in] flags Join message flags
 */
void
contact_update_join (struct GNUNET_CHAT_Contact *contact,
                     struct GNUNET_CHAT_Context *context,
                     const struct GNUNET_HashCode *hash,
                     enum GNUNET_MESSENGER_MessageFlags flags);

/**
 * Removes local chat <i>contact</i> from a given chat
 * <i>context</i>.
 *
 * @param[in,out] contact Chat contact
 * @param[in,out] context Chat context
 */
void
contact_leave (struct GNUNET_CHAT_Contact *contact,
               struct GNUNET_CHAT_Context *context);

/**
 * Updates the string representation of the public key from
 * a given chat <i>contact</i>.
 *
 * @param[in,out] contact Chat contact
 */
void
contact_update_key (struct GNUNET_CHAT_Contact *contact);

/**
 * Returns the public key from a given chat <i>contact</i>.
 *
 * @param[in] contact Chat contact
 */
const struct GNUNET_CRYPTO_BlindablePublicKey*
contact_get_key (const struct GNUNET_CHAT_Contact *contact);

/**
 * Searches for a chat context containing a given chat
 * <i>contact</i> and the least amount of other members.
 *
 * @param[in] contact Chat contact
 * @param[in] room_required Flag to suggest a room is required
 * @return Chat context or NULL
 */
struct GNUNET_CHAT_Context*
contact_find_context (const struct GNUNET_CHAT_Contact *contact,
                      enum GNUNET_GenericReturnValue room_required);

/**
 * Returns whether a chat <i>contact</i> is tagged in
 * a given chat <i>context</i> with a specific <i>tag</i>.
 *
 * @param[in] contact Chat contact
 * @param[in] context Chat context or NULL (optional)
 * @param[in] tag Tag or NULL
 * @return #GNUNET_YES if tagged, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
contact_is_tagged (const struct GNUNET_CHAT_Contact *contact,
                   const struct GNUNET_CHAT_Context *context,
                   const char *tag);

/**
 * Untags a given chat <i>contact</i> in
 * a given chat <i>context</i> from a specific <i>tag</i>.
 *
 * @param[in,out] contact Chat contact
 * @param[in,out] context Chat context
 */
void
contact_untag (struct GNUNET_CHAT_Contact *contact,
               struct GNUNET_CHAT_Context *context,
               const char *tag);

/**
 * Tags a given chat <i>contact</i> in
 * a given chat <i>context</i> with a specific <i>tag</i>.
 *
 * @param[in,out] contact Chat contact
 * @param[in,out] context Chat context
 * @param[in] tag Tag or NULL
 */
void
contact_tag (struct GNUNET_CHAT_Contact *contact,
             struct GNUNET_CHAT_Context *context,
             const char *tag);

/**
 * Iterate through all tags of a given chat <i>contact</i>
 * in a specific chat <i>context</i> (or all of them) using
 * an optional <i>callback</i> with its closure.
 *
 * @param[in,out] contact Chat contact
 * @param[in,out] context Chat context or NULL
 * @param[in] callback Callback for tag iteration or NULL
 * @param[in,out] cls Closure for tag iteration or NULL
 * @return Amount of tags iterated or #GNUNET_SYSERR on error
 */
int
contact_iterate_tags (struct GNUNET_CHAT_Contact *contact,
                      struct GNUNET_CHAT_Context *context,
                      GNUNET_CHAT_ContactTagCallback callback,
                      void *cls);

/**
 * Destroys a chat <i>contact</i> and frees its memory.
 *
 * @param[in,out] contact Chat contact
 */
void
contact_destroy (struct GNUNET_CHAT_Contact* contact);

#endif /* GNUNET_CHAT_CONTACT_H_ */
