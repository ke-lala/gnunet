/*
   This file is part of GNUnet.
   Copyright (C) 2021--2022 GNUnet e.V.

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
 * @file chat/messenger.h
 */

#ifndef CHAT_MESSENGER_H_
#define CHAT_MESSENGER_H_

#include <gnunet/gnunet_chat_lib.h>

typedef struct MESSENGER_Application MESSENGER_Application;

typedef struct CHAT_MESSENGER_Handle
{
  struct GNUNET_CHAT_Handle *handle;
} CHAT_MESSENGER_Handle;

/**
 * Startup event of the GNUnet scheduler thread to
 * handle the messenger application startup.
 *
 * @param cls Closure
 * @param args Arguments
 * @param cfgfile Configuration file path
 * @param cfg Configuration
 */
void
chat_messenger_run(void *cls,
		   char *const *args,
		   const char *cfgfile,
		   const struct GNUNET_CONFIGURATION_Handle *cfg);

#endif /* CHAT_MESSENGER_H_ */
