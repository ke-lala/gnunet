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
 * @file gnunet_chat_message.c
 */

#include "gnunet_chat_message.h"
#include "gnunet_chat_context.h"

#include <gnunet/gnunet_messenger_service.h>

struct GNUNET_CHAT_Message*
message_create_from_msg (struct GNUNET_CHAT_Context *context,
                         const struct GNUNET_HashCode *hash,
                         enum GNUNET_MESSENGER_MessageFlags flags,
                         const struct GNUNET_MESSENGER_Message *msg)
{
  GNUNET_assert((context) && (hash) && (msg));

  struct GNUNET_CHAT_Message *message = GNUNET_new(struct GNUNET_CHAT_Message);

  message->account = NULL;
  message->context = context;
  message->task = NULL;

  GNUNET_memcpy(&(message->hash), hash, sizeof(message->hash));
  message->flags = flags;
  message->flag = GNUNET_CHAT_FLAG_NONE;

  message->warning_buffer = NULL;

  message->msg = msg;
  message->user_pointer = NULL;

  return message;
}

struct GNUNET_CHAT_Message*
message_create_internally (struct GNUNET_CHAT_Account *account,
                           struct GNUNET_CHAT_Context *context,
                           enum GNUNET_CHAT_MessageFlag flag,
                           const char *warning,
                           enum GNUNET_GenericReturnValue async_warning)
{
  struct GNUNET_CHAT_Message *message = GNUNET_new(struct GNUNET_CHAT_Message);

  message->account = account;
  message->context = context;
  message->task = NULL;

  memset(&(message->hash), 0, sizeof(message->hash));
  message->flags = GNUNET_MESSENGER_FLAG_PRIVATE;
  message->flag = flag;

  if (GNUNET_YES == async_warning)
    message->warning_buffer = warning? GNUNET_strdup(warning) : NULL;
  else
    message->warning_buffer = NULL;

  message->warning = GNUNET_YES == async_warning? message->warning_buffer : warning;
  message->user_pointer = NULL;

  return message;
}

enum GNUNET_GenericReturnValue
message_has_msg (const struct GNUNET_CHAT_Message* message)
{
  GNUNET_assert(message);

  if (message->flag != GNUNET_CHAT_FLAG_NONE)
    return GNUNET_NO;

  if (message->msg)
    return GNUNET_YES;
  else
    return GNUNET_NO;
}

void
message_update_msg (struct GNUNET_CHAT_Message* message,
                    enum GNUNET_MESSENGER_MessageFlags flags,
                    const struct GNUNET_MESSENGER_Message *msg)
{
  GNUNET_assert((message) && (msg));

  if ((GNUNET_YES != message_has_msg(message)) ||
      (message->flags & GNUNET_MESSENGER_FLAG_DELETE))
    return;

  if (flags & GNUNET_MESSENGER_FLAG_UPDATE)
    message->msg = msg;
  else if (flags & GNUNET_MESSENGER_FLAG_DELETE)
    context_delete_message(message->context, message);
  else
    return;

  message->flags = flags | GNUNET_MESSENGER_FLAG_UPDATE;
}

void
message_destroy (struct GNUNET_CHAT_Message* message)
{
  GNUNET_assert(message);

  if (message->task)
    GNUNET_SCHEDULER_cancel(message->task);

  if (message->warning_buffer)
    GNUNET_free(message->warning_buffer);

  GNUNET_free(message);
}
