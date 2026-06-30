/*
   This file is part of GNUnet.
   Copyright (C) 2022--2026 GNUnet e.V.

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
 * @file gnunet_chat_account.c
 */

#include "gnunet_chat_account.h"
#include "gnunet_chat_handle.h"
#include "gnunet_chat_util.h"

#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_identity_service.h>
#include <gnunet/gnunet_messenger_service.h>
#include <gnunet/gnunet_util_lib.h>

struct GNUNET_CHAT_Account*
account_create (struct GNUNET_CHAT_Handle *handle,
                const char *name)
{
  GNUNET_assert(name);

  struct GNUNET_CHAT_Account *account = GNUNET_new(struct GNUNET_CHAT_Account);

  account->handle = handle;

  account->ego = NULL;
  account->created = GNUNET_NO;

  account->name = NULL;

  util_set_name_field(name, &(account->name));

  account->user_pointer = NULL;

  return account;
}

struct GNUNET_CHAT_Account*
account_create_from_ego (struct GNUNET_CHAT_Handle *handle,
                         struct GNUNET_IDENTITY_Ego *ego,
			                   const char *name)
{
  GNUNET_assert((ego) && (name));

  struct GNUNET_CHAT_Account *account = account_create(handle, name);
  
  account->ego = ego;
  account->created = GNUNET_YES;

  return account;
}

const struct GNUNET_CRYPTO_BlindablePrivateKey*
account_get_key (const struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert(account);

  if (!(account->ego))
    return NULL;

  return GNUNET_IDENTITY_ego_get_private_key(
    account->ego
  );
}

const char*
account_get_name (const struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert(account);

  return account->name;
}

void
account_update_ego (struct GNUNET_CHAT_Account *account,
                    struct GNUNET_CHAT_Handle *handle,
                    struct GNUNET_IDENTITY_Ego *ego)
{
  GNUNET_assert((account) && (handle));

  enum GNUNET_CHAT_MessageFlag flag;
  if (GNUNET_NO == account->created)
    flag = GNUNET_CHAT_FLAG_CREATE_ACCOUNT;
  else
    flag = GNUNET_CHAT_FLAG_UPDATE_ACCOUNT;

  account->ego = ego;

  if (!(account->ego))
    return;

  if ((handle->current == account) && (handle->messenger))
  {
    GNUNET_MESSENGER_set_key(
      handle->messenger,
      GNUNET_IDENTITY_ego_get_private_key(account->ego)
    );

    handle_update_key(handle);
  }

  handle_send_internal_message(
    handle,
    account,
    NULL,
    flag,
    NULL,
    GNUNET_YES
  );
}

void
account_delete (struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert(account);

  // TODO: clear namestore entries
}

void
account_destroy (struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert(account);

  if (account->name)
    GNUNET_free(account->name);

  GNUNET_free(account);
}
