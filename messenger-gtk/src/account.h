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
 * @file account.h
 */

#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include "application.h"

typedef struct MESSENGER_AccountInfo
{
  struct GNUNET_CHAT_Account *account;

  GFile *icon_file;
  GIcon *icon;
  guint task;

  GList *name_avatars;
} MESSENGER_AccountInfo;

/**
 * Creates an account information struct to potentially
 * update all GUI appearances of a specific account at
 * once.
 *
 * @param account Chat account
 * @return #GNUNET_YES on info creation, otherwise #GNUNET_NO
 */
enum GNUNET_GenericReturnValue
account_create_info(struct GNUNET_CHAT_Account *account);

/**
 * Destroys and frees resources allocated for a given
 * account information struct.
 *
 * @param account Chat account
 */
void
account_destroy_info(struct GNUNET_CHAT_Account *account);

/**
 * Destroys and frees all resources allocated for
 * account information structs to cleanup everything
 * on an accounts refresh.
 */
void
account_cleanup_infos();

/**
 * Adds a HdyAvatar to the list of avatars
 * which get updated by state changes.
 *
 * @param account Chat account
 * @param avatar Avatar
 */
void
account_add_name_avatar_to_info(const struct GNUNET_CHAT_Account *account,
			                          HdyAvatar *avatar);

/**
 * Switches a HdyAvatar to the list of avatars
 * in case it's in another list. Otherwise it
 * gets added as usual.
 *
 * @param account Chat account
 * @param avatar Avatar
 */
void
account_switch_name_avatar_to_info(const struct GNUNET_CHAT_Account *account,
			                             HdyAvatar *avatar);

/**
 * Removes a HdyAvatar from the list of avatars
 * which get updated by state changes.
 *
 * @param account Chat account
 * @param avatar Avatar
 */
void
account_remove_name_avatar_from_info(const struct GNUNET_CHAT_Account *account,
			                               HdyAvatar *avatar);

/**
 * Updates the connected UI elements for a given
 * account based on current attributes changes.
 *
 * @param account Chat account
 * @param app Messenger application
 */
void
account_update_attributes(const struct GNUNET_CHAT_Account *account,
                          MESSENGER_Application *app);

#endif /* ACCOUNT_H_ */
