/*
   This file is part of GNUnet.
   Copyright (C) 2024--2025 GNUnet e.V.

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
 * @file gnunet_chat_ticket.c
 */

#include "gnunet_chat_ticket.h"

#include "gnunet_chat_contact.h"
#include "gnunet_chat_ticket_intern.c"
#include "gnunet_chat_handle.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_reclaim_service.h>
#include <string.h>

struct GNUNET_CHAT_Ticket*
ticket_create_from_message (struct GNUNET_CHAT_Handle *handle,
                            struct GNUNET_CHAT_Contact *issuer,
                            const struct GNUNET_MESSENGER_MessageTicket *message)
{
  GNUNET_assert((handle) && (issuer) && (message));

  const struct GNUNET_CRYPTO_BlindablePublicKey *identity;
  const struct GNUNET_CRYPTO_BlindablePublicKey *audience;

  identity = contact_get_key(issuer);
  audience = GNUNET_MESSENGER_get_key(handle->messenger);

  if ((!identity) || (!audience))
    return NULL;

  struct GNUNET_CHAT_Ticket *ticket = GNUNET_new(struct GNUNET_CHAT_Ticket);

  ticket->handle = handle;
  ticket->issuer = issuer;

  ticket->callback = NULL;
  ticket->closure = NULL;

  ticket->op = NULL;

  strncpy(ticket->ticket.gns_name, message->identifier, sizeof(ticket->ticket.gns_name));
  ticket->ticket.gns_name[sizeof(ticket->ticket.gns_name) - 1] = '\0';

  return ticket;
}

void
ticket_consume(struct GNUNET_CHAT_Ticket *ticket,
               GNUNET_CHAT_ContactAttributeCallback callback,
               void *cls)
{
  GNUNET_assert(ticket);

  const struct GNUNET_CRYPTO_BlindablePrivateKey *key = handle_get_key(
    ticket->handle
  );

  if (!key)
    return;

  struct GNUNET_CRYPTO_BlindablePublicKey pubkey;
  GNUNET_CRYPTO_blindable_key_get_public(key, &pubkey);

  char *rp_uri = GNUNET_CRYPTO_blindable_public_key_to_string(&pubkey);

  ticket->callback = callback;
  ticket->closure = cls;

  if (ticket->op)
    GNUNET_RECLAIM_cancel(ticket->op);

  ticket->op = GNUNET_RECLAIM_ticket_consume(
    ticket->handle->reclaim,
    &(ticket->ticket),
    rp_uri,
    cb_ticket_consume_attribute,
    ticket
  );

  GNUNET_free(rp_uri);
}

void
ticket_destroy (struct GNUNET_CHAT_Ticket *ticket)
{
  GNUNET_assert(ticket);

  if (ticket->op)
    GNUNET_RECLAIM_cancel(ticket->op);

  GNUNET_free(ticket);
}
