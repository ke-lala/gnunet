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
 * @file gnunet_chat_ticket_intern.c
 */

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_reclaim_lib.h>

#define GNUNET_UNUSED __attribute__ ((unused))

void
cb_ticket_consume_attribute (void *cls, 
                             const struct GNUNET_CRYPTO_BlindablePublicKey *identity, 
                             const struct GNUNET_RECLAIM_Attribute *attribute, 
                             const struct GNUNET_RECLAIM_Presentation *presentation)
{
  GNUNET_assert(cls);

  struct GNUNET_CHAT_Ticket *ticket = (
    (struct GNUNET_CHAT_Ticket*) cls
  );

  if (!attribute)
  {
    ticket->op = NULL;
    return;
  }

  char *value = GNUNET_RECLAIM_attribute_value_to_string(
    attribute->type,
    attribute->data,
    attribute->data_size
  );

  if (ticket->callback)
    ticket->callback(ticket->closure, ticket->issuer, attribute->name, value);

  if (value)
    GNUNET_free(value);
}
