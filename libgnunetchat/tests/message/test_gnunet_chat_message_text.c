/*
   This file is part of GNUnet.
   Copyright (C) 2023--2026 GNUnet e.V.

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
 * @file test_gnunet_chat_message_text.c
 */

#include "test_gnunet_chat.h"

#define TEST_TEXT_ID     "gnunet_chat_message_text"
#define TEST_TEXT_GROUP  "gnunet_chat_message_text_group"
#define TEST_TEXT_MSG    "test_text_message"
#define TEST_TEXT_SECRET "test_secret_text"

enum GNUNET_GenericReturnValue
on_gnunet_chat_message_text_msg(void *cls,
                                struct GNUNET_CHAT_Context *context,
                                struct GNUNET_CHAT_Message *message)
{
  static unsigned int text_stage = 0;

  struct GNUNET_CHAT_Handle *handle = *(
    (struct GNUNET_CHAT_Handle**) cls
  );

  struct GNUNET_CHAT_Account *account;
  struct GNUNET_CHAT_Group *group;
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

      if (text_stage == 0)
      {
        account = GNUNET_CHAT_find_account(handle, TEST_TEXT_ID);

        ck_assert_ptr_nonnull(account);

        GNUNET_CHAT_connect(
          handle,
          account,
          TEST_TEXT_SECRET,
          strlen(TEST_TEXT_SECRET)
        );

        text_stage = 1;
      }

      break;
    case GNUNET_CHAT_KIND_LOGIN:
      ck_assert_ptr_null(context);
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(text_stage, 1);

      group = GNUNET_CHAT_group_create(handle, TEST_TEXT_GROUP);

      ck_assert_ptr_nonnull(group);

      text_stage = 2;
      break;
    case GNUNET_CHAT_KIND_LOGOUT:
      ck_assert_ptr_null(context);
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_eq(text_stage, 5);

      GNUNET_CHAT_stop(handle);
      break;
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      break;
    case GNUNET_CHAT_KIND_UPDATE_CONTEXT:
      ck_assert_ptr_nonnull(context);
      break;
    case GNUNET_CHAT_KIND_JOIN:
      ck_assert_ptr_nonnull(context);
      ck_assert_uint_eq(text_stage, 2);

      ck_assert_int_eq(GNUNET_CHAT_context_send_text(
	      context, TEST_TEXT_MSG
      ), GNUNET_OK);

      text_stage = 3;
      break;
    case GNUNET_CHAT_KIND_LEAVE:
      ck_assert_ptr_nonnull(context);
      ck_assert_uint_eq(text_stage, 4);
      
      GNUNET_CHAT_disconnect(handle);
      text_stage = 5;
      break;
    case GNUNET_CHAT_KIND_CONTACT:
      ck_assert_ptr_nonnull(context);
      break;
    case GNUNET_CHAT_KIND_TEXT:
      ck_assert_ptr_nonnull(context);
      ck_assert_uint_eq(text_stage, 3);

      group = GNUNET_CHAT_context_get_group(context);

      ck_assert_ptr_nonnull(group);

      text = GNUNET_CHAT_message_get_text(message);

      ck_assert_str_eq(text, TEST_TEXT_MSG);
      ck_assert_int_eq(GNUNET_CHAT_group_leave(group), GNUNET_OK);

      text_stage = 4;
      break;
    default:
      ck_abort_msg("%d\n", GNUNET_CHAT_message_get_kind(message));
      break;
  }

  return GNUNET_YES;
}

REQUIRE_GNUNET_CHAT_ACCOUNT(gnunet_chat_message_text, TEST_TEXT_ID)

void
call_gnunet_chat_message_text(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  static struct GNUNET_CHAT_Handle *handle = NULL;
  handle = GNUNET_CHAT_start(cfg, on_gnunet_chat_message_text_msg, &handle);

  ck_assert_ptr_nonnull(handle);
}

CREATE_GNUNET_TEST(test_gnunet_chat_message_text, gnunet_chat_message_text)

START_SUITE(handle_suite, "Message")
ADD_TEST_TO_SUITE(test_gnunet_chat_message_text, "Text")
END_SUITE

MAIN_SUITE(handle_suite, CK_NORMAL)
