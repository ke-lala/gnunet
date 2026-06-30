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
 * @file gnunet_chat_accounts.h
 */

#ifndef GNUNET_CHAT_INTERNAL_ACCOUNTS_H_
#define GNUNET_CHAT_INTERNAL_ACCOUNTS_H_

#include <gnunet/gnunet_identity_service.h>

enum GNUNET_CHAT_AccountMethod
{
  GNUNET_CHAT_ACCOUNT_CREATION = 1,
  GNUNET_CHAT_ACCOUNT_DELETION = 2,
  GNUNET_CHAT_ACCOUNT_RENAMING = 3,
  GNUNET_CHAT_ACCOUNT_UPDATING = 4,

  GNUNET_CHAT_ACCOUNT_NONE = 0
};

struct GNUNET_CHAT_Handle;
struct GNUNET_CHAT_Account;

struct GNUNET_CHAT_InternalAccounts
{
  struct GNUNET_CHAT_Handle *handle;
  struct GNUNET_CHAT_Account *account;

  char *identifier;
  struct GNUNET_IDENTITY_Operation *op;
  enum GNUNET_CHAT_AccountMethod method;

  struct GNUNET_CHAT_InternalAccounts *next;
  struct GNUNET_CHAT_InternalAccounts *prev;
};

/**
 * Creates a new internal account resource to
 * manage accounts for a given chat <i>handle</i>.
 *
 * The internal account gets appended to the
 * list of accounts from the handle implicitly.
 *
 * An internal account might represent only an
 * operation regarding a certain account while not
 * representing the actual account. In most cases
 * however this will be equivalent 
 * (for example iteration).
 *
 * @param[in,out] handle Chat handle
 * @param[in,out] account Chat account or NULL
 * @return New internal account resource
 */
struct GNUNET_CHAT_InternalAccounts*
internal_accounts_create(struct GNUNET_CHAT_Handle *handle,
                         struct GNUNET_CHAT_Account *account);

/**
 * Destroys and frees an internal <i>accounts</i> 
 * resource while implicitly removing it from its 
 * chat handles list of accounts.
 *
 * @param[out] accounts Internal account resource
 */
void
internal_accounts_destroy(struct GNUNET_CHAT_InternalAccounts *accounts);

/**
 * Initializes a given internal <i>accounts</i>
 * resource with a selected account <i>method</i>
 * to start further internal operations.
 *
 * @param[in,out] accounts Internal account resource
 * @param[in] method Account method
 * @param[in] identifier Account identifier or NULL
 */
void
internal_accounts_start_method(struct GNUNET_CHAT_InternalAccounts *accounts,
                               enum GNUNET_CHAT_AccountMethod method,
                               const char *identifier);

/**
 * Resets a given internal <i>accounts</i>
 * resource to a neutral method state and stops
 * its current operation.
 *
 * @param[in,out] accounts Internal account resource
 */
void
internal_accounts_stop_method(struct GNUNET_CHAT_InternalAccounts *accounts);

#endif /* GNUNET_CHAT_ACCOUNTS_H_ */
