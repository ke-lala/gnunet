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
 * @file gnunet_chat_accounts.c
 */

#include "gnunet_chat_accounts.h"

#include "../gnunet_chat_handle.h"

#include <gnunet/gnunet_common.h>

struct GNUNET_CHAT_InternalAccounts*
internal_accounts_create(struct GNUNET_CHAT_Handle *handle,
                         struct GNUNET_CHAT_Account *account)
{
  GNUNET_assert(handle);

  struct GNUNET_CHAT_InternalAccounts *accounts = GNUNET_new(
    struct GNUNET_CHAT_InternalAccounts
  );

  if (!accounts)
    return NULL;

  accounts->handle = handle;
  accounts->account = account;

  accounts->identifier = NULL;
  accounts->op = NULL;
  accounts->method = GNUNET_CHAT_ACCOUNT_NONE;

  GNUNET_CONTAINER_DLL_insert(
    accounts->handle->accounts_head,
    accounts->handle->accounts_tail,
    accounts
  );

  return accounts;
}

void
internal_accounts_destroy(struct GNUNET_CHAT_InternalAccounts *accounts)
{
  GNUNET_assert((accounts) && (accounts->handle));

  GNUNET_CONTAINER_DLL_remove(
    accounts->handle->accounts_head,
    accounts->handle->accounts_tail,
    accounts
  );

  if (accounts->identifier)
    GNUNET_free(accounts->identifier);

  if (accounts->op)
    GNUNET_IDENTITY_cancel(accounts->op);

  GNUNET_free(accounts);
}

void
internal_accounts_start_method(struct GNUNET_CHAT_InternalAccounts *accounts,
                               enum GNUNET_CHAT_AccountMethod method,
                               const char *identifier)
{
  GNUNET_assert(
    (accounts) && 
    (GNUNET_CHAT_ACCOUNT_NONE == accounts->method) &&
    (!(accounts->identifier)) &&
    (!(accounts->op))
  );

  accounts->identifier = identifier ? GNUNET_strdup(identifier) : NULL;
  accounts->method = method;
}

void
internal_accounts_stop_method(struct GNUNET_CHAT_InternalAccounts *accounts)
{
  GNUNET_assert(accounts);

  if (accounts->identifier)
  {
    GNUNET_free(accounts->identifier);
    accounts->identifier = NULL;
  }

  if (accounts->op)
  {
    GNUNET_IDENTITY_cancel(accounts->op);
    accounts->op = NULL;
  }

  accounts->method = GNUNET_CHAT_ACCOUNT_NONE;
}
