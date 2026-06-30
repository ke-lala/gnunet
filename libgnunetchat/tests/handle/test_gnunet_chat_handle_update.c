/*
   This file is part of GNUnet.
   Copyright (C) 2021--2026 GNUnet e.V.

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
 * @file test_gnunet_chat_handle_update.c
 */

#include "test_gnunet_chat.h"

#define TEST_UPDATE_ID     "gnunet_chat_handle_update"
#define TEST_UPDATE_SECRET "test_secret_handle_update"

enum GNUNET_GenericReturnValue
on_gnunet_chat_handle_update_msg(void *cls,
                                 struct GNUNET_CHAT_Context *context,
                                 struct GNUNET_CHAT_Message *message)
{
  static unsigned int update_stage = 0;

  struct GNUNET_CHAT_Handle *handle = *(
    (struct GNUNET_CHAT_Handle**) cls
  );

  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_null(context);
  ck_assert_ptr_nonnull(message);

  struct GNUNET_CHAT_Account *account;
  account = GNUNET_CHAT_message_get_account(message);

  const char *key;
  char *dup;

  switch (GNUNET_CHAT_message_get_kind(message))
  {
    case GNUNET_CHAT_KIND_WARNING:
      ck_abort_msg("%s\n", GNUNET_CHAT_message_get_text(message));
      break;
    case GNUNET_CHAT_KIND_REFRESH:
      ck_assert_ptr_null(context);
      ck_assert_ptr_null(account);

      if (update_stage == 0)
      {
        account = GNUNET_CHAT_find_account(handle, TEST_UPDATE_ID);

        ck_assert_ptr_nonnull(account);

        GNUNET_CHAT_connect(
          handle,
          account,
          TEST_UPDATE_SECRET,
          strlen(TEST_UPDATE_SECRET)
        );

        update_stage = 1;
      }
      
      break;
    case GNUNET_CHAT_KIND_LOGIN:
      account = GNUNET_CHAT_get_connected(handle);

      ck_assert_ptr_nonnull(account);
      ck_assert_str_eq(
        GNUNET_CHAT_account_get_name(account),
        TEST_UPDATE_ID
      );

      key = GNUNET_CHAT_get_key(handle);
      ck_assert_ptr_nonnull(key);

      dup = (char*) GNUNET_CHAT_get_user_pointer(handle);

      ck_assert_ptr_null(dup);
      ck_assert_int_eq(update_stage, 1);

      dup = GNUNET_strdup(key);

      ck_assert_ptr_nonnull(dup);
      ck_assert_str_eq(key, dup);

      GNUNET_CHAT_set_user_pointer(handle, (void*) dup);
      GNUNET_CHAT_update(handle);

      update_stage = 2;
      break;
    case GNUNET_CHAT_KIND_LOGOUT:
      account = GNUNET_CHAT_get_connected(handle);

      ck_assert_int_ge(update_stage, 2);
      ck_assert_int_le(update_stage, 3);

      ck_assert_ptr_nonnull(account);
      ck_assert_str_eq(
        GNUNET_CHAT_account_get_name(account),
        TEST_UPDATE_ID
      );

      if (update_stage == 3)
      {
        update_stage = 4;
        GNUNET_CHAT_stop(handle);
      }

      break;
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
      ck_assert_ptr_nonnull(account);

      if ((0 != strcmp(GNUNET_CHAT_account_get_name(account),
                       TEST_UPDATE_ID)))
        break;
      
      key = GNUNET_CHAT_get_key(handle);
      ck_assert_ptr_nonnull(key);

      dup = (char*) GNUNET_CHAT_get_user_pointer(handle);

      ck_assert_int_eq(update_stage, 2);
      ck_assert_ptr_nonnull(dup);

      if (0 == strcmp(key, dup))
        break;

      ck_assert_str_ne(key, dup);

      GNUNET_free(dup);

      GNUNET_CHAT_disconnect(handle);

      update_stage = 3;
      break;
    default:
      ck_abort();
      break;
  }

  return GNUNET_YES;
}

REQUIRE_GNUNET_CHAT_ACCOUNT(gnunet_chat_handle_update, TEST_UPDATE_ID)

void
call_gnunet_chat_handle_update(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  static struct GNUNET_CHAT_Handle *handle = NULL;
  handle = GNUNET_CHAT_start(cfg, on_gnunet_chat_handle_update_msg, &handle);

  ck_assert_ptr_nonnull(handle);
}

CREATE_GNUNET_TEST(test_gnunet_chat_handle_update, gnunet_chat_handle_update)

START_SUITE(handle_suite, "Handle")
ADD_TEST_TO_SUITE(test_gnunet_chat_handle_update, "Update")
END_SUITE

MAIN_SUITE(handle_suite, CK_NORMAL)
