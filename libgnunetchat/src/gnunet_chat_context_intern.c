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
 * @file gnunet_chat_context_intern.c
 */

#include "gnunet_chat_discourse.h"
#include "gnunet_chat_invitation.h"
#include "gnunet_chat_message.h"

#include "internal/gnunet_chat_tagging.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_error_codes.h>
#include <gnunet/gnunet_messenger_service.h>

#define GNUNET_UNUSED __attribute__ ((unused))

enum GNUNET_GenericReturnValue
it_destroy_context_timestamps (GNUNET_UNUSED void *cls,
                               GNUNET_UNUSED const struct GNUNET_ShortHashCode *key,
                               void *value)
{
  GNUNET_assert(value);

  struct GNUNET_TIME_Absolute *time = value;
  GNUNET_free(time);
  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
it_destroy_context_messages (GNUNET_UNUSED void *cls,
                             GNUNET_UNUSED const struct GNUNET_HashCode *key,
                             void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_Message *message = value;
  message_destroy(message);
  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
it_destroy_context_taggings (GNUNET_UNUSED void *cls,
                             GNUNET_UNUSED const struct GNUNET_HashCode *key,
                             void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_InternalTagging *tagging = value;
  internal_tagging_destroy(tagging);
  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
it_destroy_context_invites (void *cls,
                            GNUNET_UNUSED const struct GNUNET_HashCode *key,
                            void *value)
{
  GNUNET_assert((cls) && (value));

  struct GNUNET_CHAT_Context *context = cls;
  struct GNUNET_CHAT_Invitation *invitation = value;
  
  struct GNUNET_CHAT_Handle *handle = context->handle;

  GNUNET_CONTAINER_multihashmap_remove(
    handle->invitations, &(invitation->key.hash), invitation);

  invitation_destroy(invitation);
  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
it_destroy_context_discourses (GNUNET_UNUSED void *cls,
                               GNUNET_UNUSED const struct GNUNET_ShortHashCode *key,
                               void *value)
{
  GNUNET_assert(value);

  struct GNUNET_CHAT_Discourse *discourse = value;
  discourse_destroy(discourse);
  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
it_iterate_context_requests (void *cls,
                             const struct GNUNET_HashCode *key,
                             GNUNET_UNUSED void *value)
{
  struct GNUNET_CHAT_Context *context = cls;

  GNUNET_assert((context) && (context->room) && (key));

  GNUNET_MESSENGER_get_message(context->room, key);

  return GNUNET_YES;
}

void
cb_context_request_messages (void *cls)
{
  struct GNUNET_CHAT_Context *context = cls;

  GNUNET_assert(context);

  context->request_task = NULL;

  if ((!(context->room)) || (GNUNET_YES == context->deleted))
    return;

  GNUNET_CONTAINER_multihashmap_iterate(
    context->requests,
    it_iterate_context_requests,
    context
  );

  GNUNET_CONTAINER_multihashmap_clear(context->requests);
}

void
cont_context_write_records (void *cls,
			                      enum GNUNET_ErrorCode ec)
{
  struct GNUNET_CHAT_Context *context = cls;

  GNUNET_assert(context);

  context->query = NULL;

  if (GNUNET_EC_NONE != ec)
    handle_send_internal_message(
      context->handle,
      NULL,
      context,
      GNUNET_CHAT_FLAG_WARNING,
      GNUNET_ErrorCode_get_hint(ec),
      GNUNET_YES
    );
}
