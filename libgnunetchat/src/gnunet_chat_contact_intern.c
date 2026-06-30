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
 * @file gnunet_chat_contact_intern.c
 */

#include "gnunet_chat_context.h"
#include "gnunet_chat_message.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>
#include <stdlib.h>

#define GNUNET_UNUSED __attribute__ ((unused))

struct GNUNET_CHAT_ContactFindRoom
{
  int member_count;
  struct GNUNET_MESSENGER_Room *room;
};

enum GNUNET_GenericReturnValue
it_contact_find_room (void *cls,
                      struct GNUNET_MESSENGER_Room *room,
                      GNUNET_UNUSED const struct GNUNET_MESSENGER_Contact *member)
{
  GNUNET_assert((cls) && (room));

  const int member_count = GNUNET_MESSENGER_iterate_members(room, NULL, NULL);

  struct GNUNET_CHAT_ContactFindRoom *find = cls;

  if ((find->member_count <= 0) ||
      ((member_count >= 1) && (member_count < find->member_count)))
  {
    find->member_count = member_count;
    find->room = room;
  }

  return GNUNET_YES;
}

struct GNUNET_CHAT_ContactFindTag
{
  const struct GNUNET_HashCode *hash;
};

enum GNUNET_GenericReturnValue
it_contact_find_tag (void *cls,
                     struct GNUNET_CHAT_Message *message)
{
  GNUNET_assert((cls) && (message));

  struct GNUNET_CHAT_ContactFindTag *find = cls;

  if ((GNUNET_YES != message_has_msg(message)) ||
      (message->flags & GNUNET_MESSENGER_FLAG_DELETE))
    return GNUNET_YES;

  if (message->flags & GNUNET_MESSENGER_FLAG_SENT)
  {
    find->hash = &(message->hash);
    return GNUNET_NO;
  }

  return GNUNET_YES;
}

struct GNUNET_CHAT_ContactIterateUniqueTag
{
  struct GNUNET_CONTAINER_MultiHashMap *tags;
  GNUNET_CHAT_ContactTagCallback callback;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_contact_iterate_unique_tag (void *cls,
                               struct GNUNET_CHAT_Contact *contact,
                               const char *tag)
{
  GNUNET_assert((cls) && (contact) && (tag));

  struct GNUNET_CHAT_ContactIterateUniqueTag *it = cls;

  struct GNUNET_HashCode hash;
  GNUNET_CRYPTO_hash(tag, strlen(tag), &hash);

  if (GNUNET_YES == GNUNET_CONTAINER_multihashmap_contains(it->tags, &hash))
    return GNUNET_YES;

  if (GNUNET_OK != GNUNET_CONTAINER_multihashmap_put(it->tags, 
      &hash, NULL, GNUNET_CONTAINER_MULTIHASHMAPOPTION_UNIQUE_FAST))
    return GNUNET_YES;
  
  if (it->callback)
    return it->callback(it->cls, contact, tag);
  else
    return GNUNET_YES;
}

struct GNUNET_CHAT_ContactIterateTag
{
  struct GNUNET_CHAT_Contact *contact;
  GNUNET_CHAT_ContactTagCallback callback;
  void *cls;
};

enum GNUNET_GenericReturnValue
it_contact_iterate_tag (void *cls,
                        struct GNUNET_CHAT_Message *message)
{
  GNUNET_assert((cls) && (message));

  struct GNUNET_CHAT_ContactIterateTag *it = cls;

  if ((GNUNET_YES != message_has_msg(message)) ||
      (message->flags & GNUNET_MESSENGER_FLAG_DELETE))
    return GNUNET_YES;
  
  if ((message->flags & GNUNET_MESSENGER_FLAG_SENT) &&
      (it->callback) && (message->msg->body.tag.tag))
    return it->callback(
      it->cls,
      it->contact,
      message->msg->body.tag.tag
    );
  else
    return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
it_free_join_hashes (void *cls,
                     const struct GNUNET_HashCode *key,
                     void *value)
{
  GNUNET_assert((key) && (value));

  struct GNUNET_HashCode *hash = value;
  GNUNET_free(hash);
  return GNUNET_YES;
}
