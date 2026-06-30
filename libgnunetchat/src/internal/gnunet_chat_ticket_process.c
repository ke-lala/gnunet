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
 * @file gnunet_chat_ticket_process.c
 */

#include "gnunet_chat_ticket_process.h"

#include "../gnunet_chat_handle.h"

#include <gnunet/gnunet_common.h>

struct GNUNET_CHAT_TicketProcess*
internal_tickets_create(struct GNUNET_CHAT_Handle *handle,
                        struct GNUNET_CHAT_Contact *contact,
                        const char *name)
{
  GNUNET_assert((handle) && (contact));

  struct GNUNET_CHAT_TicketProcess *tickets = GNUNET_new(
    struct GNUNET_CHAT_TicketProcess
  );

  if (!tickets)
    return NULL;

  memset(tickets, 0, sizeof(*tickets));

  tickets->handle = handle;
  tickets->contact = contact;

  if (!name)
    goto skip_name;

  tickets->name = GNUNET_strdup(name);

  if (!(tickets->name))
  {
    GNUNET_free(tickets);
    return NULL;
  }

skip_name:
  GNUNET_CONTAINER_DLL_insert_tail(
    tickets->handle->tickets_head,
    tickets->handle->tickets_tail,
    tickets
  );

  return tickets;
}

struct GNUNET_CHAT_TicketProcess*
internal_tickets_copy(const struct GNUNET_CHAT_TicketProcess* tickets,
                      const struct GNUNET_RECLAIM_Ticket *ticket)
{
  GNUNET_assert(tickets);

  struct GNUNET_CHAT_TicketProcess* new_tickets;
  new_tickets = internal_tickets_create(
    tickets->handle,
    tickets->contact,
    tickets->name
  );

  if (!new_tickets)
    return NULL;

  if (!ticket)
    goto skip_ticket;

  new_tickets->ticket = GNUNET_new(struct GNUNET_RECLAIM_Ticket);

  if (!(new_tickets->ticket))
  {
    internal_tickets_destroy(new_tickets);
    return NULL;
  }

  GNUNET_memcpy(
    new_tickets->ticket,
    ticket,
    sizeof(*ticket)
  );

skip_ticket:
  new_tickets->callback = tickets->callback;
  new_tickets->closure = tickets->closure;

  return new_tickets;
}

void
internal_tickets_destroy(struct GNUNET_CHAT_TicketProcess *tickets)
{
  GNUNET_assert((tickets) && (tickets->handle));

  GNUNET_CONTAINER_DLL_remove(
    tickets->handle->tickets_head,
    tickets->handle->tickets_tail,
    tickets
  );

  if (tickets->ticket)
    GNUNET_free(tickets->ticket);
  if (tickets->name)
    GNUNET_free(tickets->name);

  if (tickets->iter)
    GNUNET_RECLAIM_ticket_iteration_stop(tickets->iter);
  if (tickets->op)
    GNUNET_RECLAIM_cancel(tickets->op);

  GNUNET_free(tickets);
}

void
internal_tickets_next_iter(struct GNUNET_CHAT_TicketProcess *tickets)
{
  GNUNET_assert((tickets) && (tickets->iter));

  GNUNET_RECLAIM_ticket_iteration_next(tickets->iter);
}

void
internal_tickets_stop_iter(struct GNUNET_CHAT_TicketProcess *tickets)
{
  GNUNET_assert((tickets) && (tickets->iter));

  GNUNET_RECLAIM_ticket_iteration_stop(tickets->iter);
  tickets->iter = NULL;
}
