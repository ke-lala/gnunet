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
 * @file gnunet_chat_group.h
 */

#ifndef GNUNET_CHAT_GROUP_H_
#define GNUNET_CHAT_GROUP_H_

#include <gnunet/gnunet_regex_service.h>
#include <gnunet/gnunet_util_lib.h>

struct GNUNET_CHAT_Handle;
struct GNUNET_CHAT_Context;

struct GNUNET_CHAT_Group
{
  struct GNUNET_CHAT_Handle *handle;
  struct GNUNET_CHAT_Context *context;

  struct GNUNET_SCHEDULER_Task *destruction;

  struct GNUNET_REGEX_Announcement *announcement;
  struct GNUNET_REGEX_Search *search;

  struct GNUNET_CONTAINER_MultiPeerMap *registry;

  void *user_pointer;
};

/**
 * Creates a chat group from a chat <i>context</i>
 * with a selected chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] context Chat context
 * @return New chat group
 */
struct GNUNET_CHAT_Group*
group_create_from_context (struct GNUNET_CHAT_Handle *handle,
			                     struct GNUNET_CHAT_Context *context);

/**
 * Destroys a chat <i>group</i> and frees its memory.
 *
 * @param[in,out] group Chat group
 */
void
group_destroy (struct GNUNET_CHAT_Group* group);

/**
 * Publishes a selected chat <i>group</i> under the
 * topic of its context.
 *
 * @param[in,out] group Chat group
 */
void
group_publish (struct GNUNET_CHAT_Group* group);

#endif /* GNUNET_CHAT_GROUP_H_ */
