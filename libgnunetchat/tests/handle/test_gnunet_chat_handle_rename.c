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
 * @file test_gnunet_chat_handle_rename.c
 */

#include "test_gnunet_chat.h"

#define TEST_RENAME_ID_A   "gnunet_chat_handle_rename_a"
#define TEST_RENAME_ID_B   "gnunet_chat_handle_rename_b"
#define TEST_RENAME_SECRET "test_secret_rename"

enum GNUNET_GenericReturnValue
on_gnunet_chat_handle_rename_msg(void *cls,
                                 struct GNUNET_CHAT_Context *context,
                                 struct GNUNET_CHAT_Message *message)
{
  static unsigned int rename_stage = 0;

  struct GNUNET_CHAT_Handle *handle = *(
      (struct GNUNET_CHAT_Handle**) cls
  );

  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_null(context);
  ck_assert_ptr_nonnull(message);

  struct GNUNET_CHAT_Account *account;
  account = GNUNET_CHAT_message_get_account(message);

  const char *name = GNUNET_CHAT_get_name(handle);
  char *dup = (char*) GNUNET_CHAT_get_user_pointer(handle);

  switch (GNUNET_CHAT_message_get_kind(message))
  {
    case GNUNET_CHAT_KIND_WARNING:
      ck_abort_msg("%s\n", GNUNET_CHAT_message_get_text(message));
      break;
    case GNUNET_CHAT_KIND_REFRESH:
      ck_assert_ptr_null(account);
      break;
    case GNUNET_CHAT_KIND_LOGIN:
      ck_assert_ptr_nonnull(account);
      ck_assert_ptr_nonnull(name);
      ck_assert_ptr_null(dup);
      ck_assert_str_eq(name, TEST_RENAME_ID_A);
      ck_assert_uint_eq(rename_stage, 1);

      dup = GNUNET_strdup(name);

      ck_assert_ptr_nonnull(dup);
      ck_assert_str_eq(name, dup);

      GNUNET_CHAT_set_user_pointer(handle, (void*) dup);

      ck_assert_int_eq(GNUNET_CHAT_set_name(
        handle,
        TEST_RENAME_ID_B
      ), GNUNET_YES);

      rename_stage = 2;
      break;
    case GNUNET_CHAT_KIND_LOGOUT:
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(rename_stage, 3);

      ck_assert_int_eq(GNUNET_CHAT_account_delete(
        handle,
        TEST_RENAME_ID_B
      ), GNUNET_OK);

      rename_stage = 4;
      break;
    case GNUNET_CHAT_KIND_CREATED_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(rename_stage, 0);

      GNUNET_CHAT_connect(
        handle,
        account,
        TEST_RENAME_SECRET,
        strlen(TEST_RENAME_SECRET)
      );

      rename_stage = 1;
      break;
    case GNUNET_CHAT_KIND_DELETED_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(rename_stage, 4);

      GNUNET_CHAT_stop(handle);
      break;
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      ck_assert_ptr_nonnull(name);
      ck_assert_ptr_nonnull(dup);
      ck_assert_str_ne(name, dup);
      ck_assert_str_eq(name, TEST_RENAME_ID_B);
      ck_assert_str_eq(dup, TEST_RENAME_ID_A);
      ck_assert_uint_eq(rename_stage, 2);

      GNUNET_free(dup);

      GNUNET_CHAT_disconnect(handle);
      rename_stage = 3;
      break;
    default:
      ck_abort();
      break;
  }

  return GNUNET_YES;
}

void
setup_gnunet_chat_handle_rename(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
}

void
call_gnunet_chat_handle_rename(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  static struct GNUNET_CHAT_Handle *handle = NULL;
  handle = GNUNET_CHAT_start(cfg, on_gnunet_chat_handle_rename_msg, &handle);

  ck_assert_ptr_nonnull(handle);
  ck_assert_int_eq(GNUNET_CHAT_account_create(
    handle, TEST_RENAME_ID_A
  ), GNUNET_OK);
}

void
cleanup_gnunet_chat_handle_rename(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
}

CREATE_GNUNET_TEST(test_gnunet_chat_handle_rename, gnunet_chat_handle_rename)

START_SUITE(handle_suite, "Handle")
ADD_TEST_TO_SUITE(test_gnunet_chat_handle_rename, "Rename")
END_SUITE

MAIN_SUITE(handle_suite, CK_NORMAL)
