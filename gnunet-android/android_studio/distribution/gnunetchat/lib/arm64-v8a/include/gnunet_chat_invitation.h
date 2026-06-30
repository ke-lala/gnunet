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
 * @file gnunet_chat_invitation.h
 */

#ifndef GNUNET_CHAT_INVITATION_H_
#define GNUNET_CHAT_INVITATION_H_

#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_util_lib.h>

struct GNUNET_CHAT_Context;

struct GNUNET_CHAT_Invitation
{
  struct GNUNET_CHAT_Context *context;
  struct GNUNET_SCHEDULER_Task *task;

  struct GNUNET_HashCode hash;

  union GNUNET_MESSENGER_RoomKey key;
  GNUNET_PEER_Id door;
};

/**
 * Creates a chat invitation from an invite body in a
 * <i>message</i> with a selected chat <i>context</i>.
 *
 * @param[in,out] context Chat context
 * @param[in] message Invite message body
 * @return New chat invitation
 */
struct GNUNET_CHAT_Invitation*
invitation_create_from_message (struct GNUNET_CHAT_Context *context,
                                const struct GNUNET_HashCode *hash,
				                        const struct GNUNET_MESSENGER_MessageInvite *message);

/**
 * Destroys a chat <i>invitation</i> and frees its memory.
 *
 * @param[in,out] invitation Chat invitation
 */
void
invitation_destroy (struct GNUNET_CHAT_Invitation *invitation);

/**
 * Updates a chat <i>invitation</i> for applications to
 * notice any status changes.
 *
 * @param[in,out] invitation Chat invitation
 */
void
invitation_update (struct GNUNET_CHAT_Invitation *invitation);

#endif /* GNUNET_CHAT_INVITATION_H_ */
