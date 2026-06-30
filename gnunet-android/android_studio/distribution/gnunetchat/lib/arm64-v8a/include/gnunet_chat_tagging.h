/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file gnunet_chat_tagging.h
 */

#ifndef GNUNET_CHAT_INTERNAL_TAGGING_H_
#define GNUNET_CHAT_INTERNAL_TAGGING_H_

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_util_lib.h>

struct GNUNET_CHAT_Message;

struct GNUNET_CHAT_InternalTagging
{
  struct GNUNET_CONTAINER_MultiHashMap *tags;
};

typedef enum GNUNET_GenericReturnValue
(*GNUNET_CHAT_TaggingCallback) (void *cls,
                                struct GNUNET_CHAT_Message *message);

/**
 * Creates a tagging structure to manage different tag messages
 * mapped by its custom tag value.
 *
 * @return New chat tagging
 */
struct GNUNET_CHAT_InternalTagging*
internal_tagging_create ();

/**
 * Destroys a <i>tagging</i> structure to manage different tag 
 * messages mapped by its custom tag value.
 *
 * @param[out] tagging Chat tagging
 */
void
internal_tagging_destroy (struct GNUNET_CHAT_InternalTagging *tagging);

/**
 * Adds a tag <i>message</i> to a selected <i>tagging</i>
 * structure for later iterations.
 *
 * @param[in,out] tagging Chat tagging
 * @param[in,out] message Tag message
 * @return #GNUNET_OK on success, otherwise #GNUNET_SYSERR
 */
enum GNUNET_GenericReturnValue
internal_tagging_add (struct GNUNET_CHAT_InternalTagging *tagging,
                      struct GNUNET_CHAT_Message *message);

/**
 * Removes a tag <i>message</i> from a selected <i>tagging</i>
 * structure.
 *
 * @param[in,out] tagging Chat tagging
 * @param[in] message Tag message
 * @return #GNUNET_YES on success, #GNUNET_SYSERR on failure and
 *   otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
internal_tagging_remove (struct GNUNET_CHAT_InternalTagging *tagging,
                         const struct GNUNET_CHAT_Message *message);

/**
 * Iterates through a selected <i>tagging</i> structure forwarding
 * tag messages with a specific <i>tag</i> to a custom callback with
 * its closure.
 *
 * If <i>ignore_tag</i> is set to #GNUNET_YES all tag messages of the
 * <i>tagging</i> structure will be iterated, otherwise only with matching
 * tag value.
 *
 * @param[in] tagging Chat tagging
 * @param[in] ignore_tag Flag to set tag filtering of the iteration
 * @param[in] tag Tag value for filtering the iteration
 * @param[in] cb Callback for iteration
 * @param[in,out] cls Closure for iteration
 * @return Amount of tags iterated or #GNUNET_SYSERR on error
 */
int
internal_tagging_iterate (const struct GNUNET_CHAT_InternalTagging *tagging,
                         enum GNUNET_GenericReturnValue ignore_tag,
                         const char *tag,
                         GNUNET_CHAT_TaggingCallback cb,
                         void *cls);

#endif /* GNUNET_CHAT_INTERNAL_TAGGING_H_ */
