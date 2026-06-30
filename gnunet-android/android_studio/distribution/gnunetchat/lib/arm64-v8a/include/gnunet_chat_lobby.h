/*
   This file is part of GNUnet.
   Copyright (C) 2022--2024 GNUnet e.V.

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
 * @file gnunet_chat_lobby.h
 */

#ifndef GNUNET_CHAT_LOBBY_H_
#define GNUNET_CHAT_LOBBY_H_

#include <gnunet/gnunet_identity_service.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_chat_lib.h"

struct GNUNET_CHAT_Handle;
struct GNUNET_CHAT_Context;
struct GNUNET_CHAT_Uri;

struct GNUNET_CHAT_Lobby
{
  struct GNUNET_CHAT_Handle *handle;

  struct GNUNET_SCHEDULER_Task *destruction;

  struct GNUNET_CHAT_Context *context;
  struct GNUNET_CHAT_Uri *uri;

  struct GNUNET_IDENTITY_Operation *op;
  struct GNUNET_NAMESTORE_QueueEntry *query;

  struct GNUNET_TIME_Absolute expiration;
  GNUNET_CHAT_LobbyCallback callback;
  void *cls;
};

/**
 * Creates a new chat lobby using a given chat <i>handle</i>.
 *
 * @param[in,out] handle Chat handle
 * @return New chat lobby
 */
struct GNUNET_CHAT_Lobby*
lobby_create (struct GNUNET_CHAT_Handle *handle);

/**
 * Destroys a chat <i>lobby</i> and frees its memory.
 *
 * @param[in,out] lobby Chat lobby
 */
void
lobby_destroy (struct GNUNET_CHAT_Lobby *lobby);

/**
 * Opens a chat <i>lobby</i> and closes it automatically
 * after a selected <i>delay</i>. Once the lobby is open
 * a given <i>callback</i> will be called with a custom
 * closure.
 *
 * @param[in,out] lobby Chat lobby
 * @param[in] delay Delay to close down the lobby again
 * @param[in] callback Lobby opening callback
 * @param[in,out] cls Closure
 */
void
lobby_open (struct GNUNET_CHAT_Lobby *lobby,
            struct GNUNET_TIME_Relative delay,
            GNUNET_CHAT_LobbyCallback callback,
            void *cls);

#endif /* GNUNET_CHAT_LOBBY_H_ */
