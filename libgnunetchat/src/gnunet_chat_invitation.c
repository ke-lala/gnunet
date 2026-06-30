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
 * @file gnunet_chat_invitation.c
 */

#include "gnunet_chat_invitation.h"

#include "gnunet_chat_context.h"
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_scheduler_lib.h>

#include "gnunet_chat_invitation_intern.c"

struct GNUNET_CHAT_Invitation*
invitation_create_from_message (struct GNUNET_CHAT_Context *context,
                                const struct GNUNET_HashCode *hash,
				                        const struct GNUNET_MESSENGER_MessageInvite *message)
{
  GNUNET_assert((context) && (hash) && (message));

  struct GNUNET_CHAT_Invitation *invitation = GNUNET_new(struct GNUNET_CHAT_Invitation);

  invitation->context = context;
  invitation->task = NULL;

  GNUNET_memcpy(&(invitation->hash), hash, sizeof(invitation->hash));

  GNUNET_memcpy(&(invitation->key), &(message->key), sizeof(invitation->key));
  invitation->door = GNUNET_PEER_intern(&(message->door));

  return invitation;
}


void
invitation_destroy (struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_assert(invitation);

  if (invitation->task)
    GNUNET_SCHEDULER_cancel(invitation->task);

  GNUNET_PEER_decrement_rcs(&(invitation->door), 1);

  GNUNET_free(invitation);
}


void
invitation_update (struct GNUNET_CHAT_Invitation *invitation)
{
  GNUNET_assert(invitation);

  if (invitation->task)
    return;

  invitation->task = GNUNET_SCHEDULER_add_with_priority(
    GNUNET_SCHEDULER_PRIORITY_BACKGROUND,
    cb_invitation_update,
    invitation
  );
}
