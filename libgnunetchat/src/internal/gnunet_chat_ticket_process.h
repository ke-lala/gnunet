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
 * @file gnunet_chat_ticket_process.h
 */

#ifndef GNUNET_CHAT_INTERNAL_TICKET_PROCESS_H_
#define GNUNET_CHAT_INTERNAL_TICKET_PROCESS_H_

#include <gnunet/gnunet_reclaim_service.h>

#include "gnunet_chat_lib.h"

struct GNUNET_CHAT_Handle;
struct GNUNET_CHAT_Contact;

struct GNUNET_CHAT_TicketProcess
{
  struct GNUNET_CHAT_Handle *handle;
  struct GNUNET_CHAT_Contact *contact;

  struct GNUNET_RECLAIM_Ticket *ticket;
  char *name;

  GNUNET_CHAT_ContactAttributeCallback callback;
  void *closure;

  struct GNUNET_RECLAIM_TicketIterator *iter;
  struct GNUNET_RECLAIM_Operation *op;

  struct GNUNET_CHAT_TicketProcess *next;
  struct GNUNET_CHAT_TicketProcess *prev;
};

/**
 * Creates a new ticket process resource to
 * handle general ticket management from
 * a specific char <i>contact</i> using
 * a given chat <i>handle</i>.
 *
 * The ticket process gets appended to the
 * list of processes from the handle implicitly.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] contact Chat contact
 * @param[in] name Attribute name or NULL
 * @return New ticket process
 */
struct GNUNET_CHAT_TicketProcess*
internal_tickets_create(struct GNUNET_CHAT_Handle *handle,
                        struct GNUNET_CHAT_Contact *contact,
                        const char *name);

/**
 * Creates a new copy of a given <i>tickets</i> 
 * process with additional information about a 
 * valid reclaim <i>ticket</i>.
 *
 * @see internal_tickets_create()
 *
 * @param[in] tickets Original ticket process
 * @param[in] ticket Reclaim ticket or NULL
 * @return New ticket process
 */
struct GNUNET_CHAT_TicketProcess*
internal_tickets_copy(const struct GNUNET_CHAT_TicketProcess* tickets,
                      const struct GNUNET_RECLAIM_Ticket *ticket);

/**
 * Destroys and frees a given <i>tickets</i> 
 * process resource. This will implicitly remove
 * it from its chat handles list of processes.
 *
 * @param[out] tickets Ticket process
 */
void
internal_tickets_destroy(struct GNUNET_CHAT_TicketProcess *tickets);

/**
 * Continues the iteration of a given <i>tickets</i> 
 * process resource to its next step.
 *
 * @param[in,out] tickets Ticket process
 */
void
internal_tickets_next_iter(struct GNUNET_CHAT_TicketProcess *tickets);

/**
 * Stops the iteration of a given <i>tickets</i> 
 * process resource.
 *
 * @param[in,out] tickets Ticket process
 */
void
internal_tickets_stop_iter(struct GNUNET_CHAT_TicketProcess *tickets);

#endif /* GNUNET_CHAT_INTERNAL_TICKET_PROCESS_H_ */
