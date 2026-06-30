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
 * @file gnunet_chat_ticket.h
 */

#ifndef GNUNET_CHAT_TICKET_H_
#define GNUNET_CHAT_TICKET_H_

#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_lib.h"

struct GNUNET_CHAT_Ticket
{
  struct GNUNET_CHAT_Handle *handle;
  struct GNUNET_CHAT_Contact *issuer;

  GNUNET_CHAT_ContactAttributeCallback callback;
  void *closure;

  struct GNUNET_RECLAIM_Operation *op;

  struct GNUNET_RECLAIM_Ticket ticket;
};

/**
 * Creates a chat ticket from a ticket body in a
 * <i>message</i> received by a chat <i>handle</i>
 * from a given <i>issuer</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] issuer Messenger contact
 * @param[in] message Ticket message body
 * @return New chat ticket
 */
struct GNUNET_CHAT_Ticket*
ticket_create_from_message (struct GNUNET_CHAT_Handle *handle,
                            struct GNUNET_CHAT_Contact *issuer,
                            const struct GNUNET_MESSENGER_MessageTicket *message);

/**
 * Consumes a chat <i>ticket</i> and calls a selected
 * <i>callback</i> for each of its attributes using
 * a custom closure.
 *
 * @param[in,out] ticket Chat ticket
 * @param[in] callback Attribute callback
 * @param[in,out] cls Closure
 */
void
ticket_consume(struct GNUNET_CHAT_Ticket *ticket,
               GNUNET_CHAT_ContactAttributeCallback callback,
               void *cls);

/**
 * Destroys a chat <i>ticket</i> and frees its memory.
 *
 * @param[in,out] ticket Chat ticket
 */
void
ticket_destroy (struct GNUNET_CHAT_Ticket *ticket);

#endif /* GNUNET_CHAT_TICKET_H_ */
