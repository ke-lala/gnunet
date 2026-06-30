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
 * @file gnunet_chat_tagging.c
 */

#include "gnunet_chat_tagging.h"
#include "gnunet_chat_message.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>
#include <string.h>

static const unsigned int initial_map_size_of_tagging = 4;

struct GNUNET_CHAT_InternalTagging*
internal_tagging_create ()
{
  struct GNUNET_CHAT_InternalTagging* tagging = GNUNET_new(struct GNUNET_CHAT_InternalTagging);

  tagging->tags = GNUNET_CONTAINER_multihashmap_create(
    initial_map_size_of_tagging, GNUNET_NO);

  return tagging;
}

void
internal_tagging_destroy (struct GNUNET_CHAT_InternalTagging *tagging)
{
  GNUNET_assert(
    (tagging) &&
    (tagging->tags)
  );

  GNUNET_CONTAINER_multihashmap_destroy(tagging->tags);

  GNUNET_free(tagging);
}

static void
convert_tag_to_hash (const char *tag, struct GNUNET_HashCode *hash)
{
  GNUNET_assert(hash);

  if (tag)
    GNUNET_CRYPTO_hash(tag, strlen(tag), hash);
  else
    memset(hash, 0, sizeof(*hash));
}

enum GNUNET_GenericReturnValue
internal_tagging_add (struct GNUNET_CHAT_InternalTagging *tagging,
                      struct GNUNET_CHAT_Message *message)
{
  GNUNET_assert((tagging) && (message));

  if ((GNUNET_YES != message_has_msg(message)) ||
      (GNUNET_MESSENGER_KIND_TAG != message->msg->header.kind))
    return GNUNET_SYSERR;
  
  const char *tag = message->msg->body.tag.tag;

  struct GNUNET_HashCode hash;
  convert_tag_to_hash(tag, &hash);

  return GNUNET_CONTAINER_multihashmap_put(
    tagging->tags,
    &hash,
    message,
    GNUNET_CONTAINER_MULTIHASHMAPOPTION_MULTIPLE
  );
}

enum GNUNET_GenericReturnValue
internal_tagging_remove (struct GNUNET_CHAT_InternalTagging *tagging,
                         const struct GNUNET_CHAT_Message *message)
{
  GNUNET_assert((tagging) && (message));

  if ((GNUNET_YES != message_has_msg(message)) ||
      (GNUNET_MESSENGER_KIND_TAG != message->msg->header.kind))
    return GNUNET_SYSERR;
  
  const char *tag = message->msg->body.tag.tag;

  struct GNUNET_HashCode hash;
  convert_tag_to_hash(tag, &hash);

  return GNUNET_CONTAINER_multihashmap_remove(
    tagging->tags,
    &hash,
    message
  );
}

struct GNUNET_CHAT_InternalTaggingIterator
{
  GNUNET_CHAT_TaggingCallback cb;
  void *cls;
};

static enum GNUNET_GenericReturnValue
internal_tagging_iterate_message (void *cls,
                                  const struct GNUNET_HashCode *key,
                                  void *value)
{
  struct GNUNET_CHAT_InternalTaggingIterator *it = cls;
  struct GNUNET_CHAT_Message *message = value;

  if (!(it->cb))
    return GNUNET_YES;
  
  return it->cb(it->cls, message);
}

int
internal_tagging_iterate (const struct GNUNET_CHAT_InternalTagging *tagging,
                          enum GNUNET_GenericReturnValue ignore_tag,
                          const char *tag,
                          GNUNET_CHAT_TaggingCallback cb,
                          void *cls)
{
  GNUNET_assert(tagging);

  struct GNUNET_CHAT_InternalTaggingIterator it;
  it.cb = cb;
  it.cls = cls;

  if (GNUNET_YES == ignore_tag)
    return GNUNET_CONTAINER_multihashmap_iterate(
      tagging->tags,
      internal_tagging_iterate_message,
      &it
    );

  struct GNUNET_HashCode hash;
  convert_tag_to_hash(tag, &hash);

  return GNUNET_CONTAINER_multihashmap_get_multiple(
    tagging->tags,
    &hash,
    internal_tagging_iterate_message,
    &it
  );
}
