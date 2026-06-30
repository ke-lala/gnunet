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
 * @file test_gnunet_chat_handle_accounts.c
 */

#include "test_gnunet_chat.h"

#define TEST_ACCOUNTS_ID   "gnunet_chat_handle_accounts"

enum GNUNET_GenericReturnValue
on_gnunet_chat_handle_accounts_it(void *cls,
                                  struct GNUNET_CHAT_Handle *handle,
                                  struct GNUNET_CHAT_Account *account)
{
  unsigned int *accounts_stage = cls;

  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_nonnull(account);
  ck_assert_int_eq(*accounts_stage, 2);

  const char *name = GNUNET_CHAT_account_get_name(account);

  ck_assert_ptr_nonnull(name);

  if (0 == strcmp(name, TEST_ACCOUNTS_ID))
  {
    *accounts_stage = 3;
    return GNUNET_NO;
  }

  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
on_gnunet_chat_handle_accounts_msg(void *cls,
                                   struct GNUNET_CHAT_Context *context,
                                   struct GNUNET_CHAT_Message *message)
{
  static unsigned int accounts_stage = 0;

  struct GNUNET_CHAT_Handle *handle = *(
      (struct GNUNET_CHAT_Handle**) cls
  );

  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_null(context);

  const struct GNUNET_CHAT_Account *account;
  account = GNUNET_CHAT_message_get_account(message);

  const char *name;
  name = GNUNET_CHAT_account_get_name(account);

  switch (GNUNET_CHAT_message_get_kind(message))
  {
    case GNUNET_CHAT_KIND_WARNING:
      ck_abort_msg("%s\n", GNUNET_CHAT_message_get_text(message));
      break;
    case GNUNET_CHAT_KIND_REFRESH:
      if (0 == accounts_stage)
      {
        ck_assert_int_eq(GNUNET_CHAT_account_create(
          handle,
          TEST_ACCOUNTS_ID
        ), GNUNET_OK);

        accounts_stage = 1;
      } else
      if (2 == accounts_stage)
      {
        ck_assert_int_ge(GNUNET_CHAT_iterate_accounts(
          handle,
          on_gnunet_chat_handle_accounts_it,
          &accounts_stage
        ), 1);
      }

      if (3 == accounts_stage)
      {
        ck_assert_int_eq(GNUNET_CHAT_account_delete(
          handle,
          TEST_ACCOUNTS_ID
        ), GNUNET_OK);

        accounts_stage = 4;
      }

      break;
    case GNUNET_CHAT_KIND_CREATED_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      ck_assert_ptr_nonnull(name);

      if (0 == strcmp(name, TEST_ACCOUNTS_ID))
        accounts_stage = 2;
      
      break;
    case GNUNET_CHAT_KIND_DELETED_ACCOUNT:
      ck_assert_int_eq(accounts_stage, 4);
      ck_assert_ptr_nonnull(name);

      if (0 == strcmp(name, TEST_ACCOUNTS_ID))
        GNUNET_CHAT_stop(handle);
      
      break;
    default:
      ck_abort();
      break;
  }

  return GNUNET_YES;
}

void
setup_gnunet_chat_handle_accounts(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
}

void
call_gnunet_chat_handle_accounts(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  static struct GNUNET_CHAT_Handle *handle = NULL;
  handle = GNUNET_CHAT_start(cfg, on_gnunet_chat_handle_accounts_msg, &handle);

  ck_assert_ptr_nonnull(handle);
}

void
cleanup_gnunet_chat_handle_accounts(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
}

CREATE_GNUNET_TEST(test_gnunet_chat_handle_accounts, gnunet_chat_handle_accounts)

START_SUITE(handle_suite, "Handle")
ADD_TEST_TO_SUITE(test_gnunet_chat_handle_accounts, "Accounts")
END_SUITE

MAIN_SUITE(handle_suite, CK_NORMAL)
