/*
   This file is part of GNUnet.
   Copyright (C) 2024--2026 GNUnet e.V.

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
 * @file test_gnunet_chat_attribute_check.c
 */

#include "test_gnunet_chat.h"

#define TEST_CHECK_ID     "gnunet_chat_attribute_check"
#define TEST_CHECK_NAME   "test_attribute_check_name"
#define TEST_CHECK_VALUE  "test_attribute_check_value"
#define TEST_CHECK_SECRET "test_secret_attribute_check"

enum GNUNET_GenericReturnValue
on_gnunet_chat_attribute_check_attr(void *cls,
                                    struct GNUNET_CHAT_Handle *handle,
                                    const char *name,
                                    const char *value)
{
  ck_assert_ptr_null(cls);
  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_nonnull(name);

  if (0 == strcmp(name, TEST_CHECK_NAME))
  {
    ck_assert_ptr_nonnull(value);
    ck_assert_str_eq(value, TEST_CHECK_VALUE);

    GNUNET_CHAT_delete_attribute(handle, TEST_CHECK_NAME);
    return GNUNET_NO;
  }

  return GNUNET_YES;
}

enum GNUNET_GenericReturnValue
on_gnunet_chat_attribute_check_msg(void *cls,
                                   struct GNUNET_CHAT_Context *context,
                                   struct GNUNET_CHAT_Message *message)
{
  static unsigned int attribute_stage = 0;

  struct GNUNET_CHAT_Handle *handle = *(
    (struct GNUNET_CHAT_Handle**) cls
  );

  struct GNUNET_CHAT_Account *account;
  const char *text;

  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_nonnull(message);

  account = GNUNET_CHAT_message_get_account(message);

  switch (GNUNET_CHAT_message_get_kind(message))
  {
    case GNUNET_CHAT_KIND_WARNING:
      ck_abort_msg("%s\n", GNUNET_CHAT_message_get_text(message));
      break;
    case GNUNET_CHAT_KIND_REFRESH:
      ck_assert_ptr_null(context);
      ck_assert_ptr_null(account);

      if (attribute_stage == 0)
      {
        account = GNUNET_CHAT_find_account(handle, TEST_CHECK_ID);

        ck_assert_ptr_nonnull(account);

        GNUNET_CHAT_connect(
          handle,
          account,
          TEST_CHECK_SECRET,
          strlen(TEST_CHECK_SECRET)
        );

        attribute_stage = 1;
      }

      break;
    case GNUNET_CHAT_KIND_LOGIN:
      ck_assert_ptr_null(context);
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(attribute_stage, 1);

      GNUNET_CHAT_set_attribute(
        handle,
        TEST_CHECK_NAME,
        TEST_CHECK_VALUE
      );

      attribute_stage = 2;
      break;
    case GNUNET_CHAT_KIND_LOGOUT:
      ck_assert_ptr_null(context);
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(attribute_stage, 4);

      GNUNET_CHAT_stop(handle);
      break;
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      break;
    case GNUNET_CHAT_KIND_ATTRIBUTES:
      ck_assert_ptr_null(context);

      text = GNUNET_CHAT_message_get_text(message);

      if (text)
      {
        ck_assert_str_eq(text, TEST_CHECK_NAME);
        ck_assert_uint_eq(attribute_stage, 2);

        GNUNET_CHAT_get_attributes(
          handle,
          on_gnunet_chat_attribute_check_attr,
          NULL
        );

        attribute_stage = 3;
      }
      else
      {
        ck_assert_uint_eq(attribute_stage, 3);

        GNUNET_CHAT_disconnect(handle);

        attribute_stage = 4;
      }

      break;
    default:
      ck_abort_msg("%d\n", GNUNET_CHAT_message_get_kind(message));
      break;
  }

  return GNUNET_YES;
}

REQUIRE_GNUNET_CHAT_ACCOUNT(gnunet_chat_attribute_check, TEST_CHECK_ID)

void
call_gnunet_chat_attribute_check(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  static struct GNUNET_CHAT_Handle *handle = NULL;
  handle = GNUNET_CHAT_start(cfg, on_gnunet_chat_attribute_check_msg, &handle);

  ck_assert_ptr_nonnull(handle);
}

CREATE_GNUNET_TEST(test_gnunet_chat_attribute_check, gnunet_chat_attribute_check)

START_SUITE(handle_suite, "Attribute")
ADD_TEST_TO_SUITE(test_gnunet_chat_attribute_check, "Check")
END_SUITE

MAIN_SUITE(handle_suite, CK_NORMAL)
