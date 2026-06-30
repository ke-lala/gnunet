/*
   This file is part of GNUnet.
   Copyright (C) 2025 GNUnet e.V.

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
 * @file gnunet_chat_invitation_intern.c
 */

#include "gnunet_chat_context.h"
#include "gnunet_chat_invitation.h"

void
cb_invitation_update (void *cls)
{
  struct GNUNET_CHAT_Invitation *invitation = cls;

  invitation->task = NULL;

  context_update_message (invitation->context, &(invitation->hash));
}
