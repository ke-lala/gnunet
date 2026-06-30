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
 * @file test_gnunet_chat_attribute_share.c
 */

#include "test_gnunet_chat.h"

#define TEST_SHARE_ID_A     "gnunet_chat_attribute_share_a"
#define TEST_SHARE_ID_B     "gnunet_chat_attribute_share_b"
#define TEST_SHARE_GROUP    "test_attribute_share_group"
#define TEST_SHARE_NAME     "test_attribute_share_name"
#define TEST_SHARE_VALUE    "test_attribute_share_value"
#define TEST_SHARE_SECRET_A "test_secret_attribute_share_a"
#define TEST_SHARE_SECRET_B "test_secret_attribute_share_b"

enum GNUNET_GenericReturnValue
on_gnunet_chat_attribute_share_attr(void *cls,
                                    struct GNUNET_CHAT_Contact *contact,
                                    const char *name,
                                    const char *value)
{
  ck_assert_ptr_nonnull(cls);
  ck_assert_ptr_nonnull(contact);
  ck_assert_ptr_nonnull(name);
  ck_assert_ptr_nonnull(value);

  struct GNUNET_CHAT_Handle *handle = (
    (struct GNUNET_CHAT_Handle*) cls
  );

  const struct GNUNET_CHAT_Account *account;
  account = GNUNET_CHAT_get_connected(handle);

  const char *account_name = GNUNET_CHAT_account_get_name(account);
  const char *contact_name = GNUNET_CHAT_contact_get_name(contact);

  ck_assert_ptr_nonnull(account);
  ck_assert_str_eq(account_name, TEST_SHARE_ID_B);

  if (contact_name)
    ck_assert_str_eq(contact_name, TEST_SHARE_ID_A);

  ck_assert_str_eq(name, TEST_SHARE_NAME);
  ck_assert_str_eq(value, TEST_SHARE_VALUE);

  GNUNET_CHAT_unshare_attribute_from(handle, contact, name);
  return GNUNET_NO;
}

enum GNUNET_GenericReturnValue
on_gnunet_chat_attribute_share_msg(void *cls,
                                   struct GNUNET_CHAT_Context *context,
                                   struct GNUNET_CHAT_Message *message)
{
  static unsigned int share_stage = 0;

  struct GNUNET_CHAT_Handle *handle = *(
    (struct GNUNET_CHAT_Handle**) cls
  );

  struct GNUNET_CHAT_Account *account;
  struct GNUNET_CHAT_Contact *recipient;
  struct GNUNET_CHAT_Contact *sender;
  const char *name;

  ck_assert_ptr_nonnull(handle);
  ck_assert_ptr_nonnull(message);

  account = GNUNET_CHAT_message_get_account(message);
  recipient = GNUNET_CHAT_message_get_recipient(message);
  sender = GNUNET_CHAT_message_get_sender(message);

  switch (GNUNET_CHAT_message_get_kind(message))
  {
    case GNUNET_CHAT_KIND_WARNING:
      ck_abort_msg("%s\n", GNUNET_CHAT_message_get_text(message));
      break;
    case GNUNET_CHAT_KIND_REFRESH:
      ck_assert_ptr_null(context);
      ck_assert_ptr_null(account);

      if (share_stage == 0)
      {
        account = GNUNET_CHAT_find_account(handle, TEST_SHARE_ID_A);

        ck_assert_ptr_nonnull(account);

        GNUNET_CHAT_connect(
          handle,
          account,
          TEST_SHARE_SECRET_A,
          strlen(TEST_SHARE_SECRET_A)
        );

        share_stage = 1;
      }

      break;
    case GNUNET_CHAT_KIND_LOGIN:
      ck_assert_ptr_null(context);
      ck_assert_ptr_nonnull(account);

      name = GNUNET_CHAT_account_get_name(account);

      ck_assert_ptr_nonnull(name);
      ck_assert_uint_ge(share_stage, 1);

      if (0 == strcmp(name, TEST_SHARE_ID_B))
      {
        ck_assert_uint_eq(share_stage, 4);

        GNUNET_CHAT_set_attribute(
          handle,
          TEST_SHARE_NAME,
          TEST_SHARE_VALUE
        );

        share_stage = 5;
      }
      else
      {
        ck_assert_uint_eq(share_stage, 1);

        share_stage = 2;
      }

      ck_assert_ptr_nonnull(GNUNET_CHAT_group_create(
        handle, TEST_SHARE_GROUP
      ));
      break;
    case GNUNET_CHAT_KIND_LOGOUT:
      ck_assert_ptr_null(context);
      ck_assert_ptr_nonnull(account);
      ck_assert_uint_ge(share_stage, 3);

      name = GNUNET_CHAT_account_get_name(account);

      ck_assert_ptr_nonnull(name);

      if (0 == strcmp(name, TEST_SHARE_ID_A))
      {
        ck_assert_uint_eq(share_stage, 3);

        share_stage = 4;
      }
      else if (0 == strcmp(name, TEST_SHARE_ID_B))
      {
        ck_assert_uint_eq(share_stage, 7);

        GNUNET_CHAT_stop(handle);
      }

      break;
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
      ck_assert_ptr_nonnull(account);
      break;
    case GNUNET_CHAT_KIND_UPDATE_CONTEXT:
      ck_assert_ptr_nonnull(context);
      break;
    case GNUNET_CHAT_KIND_JOIN:
      ck_assert_ptr_nonnull(context);
      ck_assert_ptr_nonnull(account);
      ck_assert_ptr_nonnull(sender);

      name = GNUNET_CHAT_account_get_name(account);

      ck_assert_ptr_nonnull(name);

      if ((0 == strcmp(name, TEST_SHARE_ID_A)) &&
          (GNUNET_YES == GNUNET_CHAT_message_is_sent(message)))
      {
        ck_assert_uint_eq(share_stage, 2);

        GNUNET_CHAT_connect(
          handle, 
          GNUNET_CHAT_find_account(handle, TEST_SHARE_ID_B),
          TEST_SHARE_SECRET_B,
          strlen(TEST_SHARE_SECRET_B)
        );

        share_stage = 3;
      }
      else if ((GNUNET_YES != GNUNET_CHAT_message_is_sent(message)) &&
               (share_stage == 5))
      {
        GNUNET_CHAT_share_attribute_with(handle, sender, TEST_SHARE_NAME);
        share_stage = 6;
      }
      
      break;
    case GNUNET_CHAT_KIND_CONTACT:
      ck_assert_ptr_nonnull(context);
      ck_assert_ptr_nonnull(sender);
      break;
    case GNUNET_CHAT_KIND_ATTRIBUTES:
      ck_assert_ptr_null(context);
      ck_assert_uint_ge(share_stage, 5);
      break;
    case GNUNET_CHAT_KIND_SHARED_ATTRIBUTES:
      ck_assert_uint_ge(share_stage, 6);

      if (!context)
      {
        ck_assert_ptr_null(account);
        ck_assert_ptr_null(sender);
        ck_assert_ptr_null(recipient);

        GNUNET_CHAT_disconnect(handle);
        share_stage = 7;
      }
      else if (GNUNET_YES == GNUNET_CHAT_message_is_sent(message))
      {
        ck_assert_ptr_nonnull(account);
        ck_assert_ptr_nonnull(sender);
        ck_assert_ptr_nonnull(recipient);
        ck_assert_ptr_ne(sender, recipient);

        GNUNET_CHAT_get_shared_attributes(
          handle,
          recipient,
          on_gnunet_chat_attribute_share_attr,
          handle
        );
      }
      break;
    default:
      ck_abort_msg("%d\n", GNUNET_CHAT_message_get_kind(message));
      break;
  }

  return GNUNET_YES;
}

const char *TEST_SHARE_IDS [] = {
  TEST_SHARE_ID_A,
  TEST_SHARE_ID_B,
  NULL
};

SETUP_GNUNET_CHAT_ACCOUNTS(gnunet_chat_attribute_share, TEST_SHARE_IDS)
CLEANUP_GNUNET_CHAT_ACCOUNTS(gnunet_chat_attribute_share, TEST_SHARE_IDS)

void
call_gnunet_chat_attribute_share(const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  static struct GNUNET_CHAT_Handle *handle = NULL;
  handle = GNUNET_CHAT_start(cfg, on_gnunet_chat_attribute_share_msg, &handle);

  ck_assert_ptr_nonnull(handle);
}

CREATE_GNUNET_TEST(test_gnunet_chat_attribute_share, gnunet_chat_attribute_share)

START_SUITE(handle_suite, "Attribute")
ADD_TEST_TO_SUITE(test_gnunet_chat_attribute_share, "Share")
END_SUITE

MAIN_SUITE(handle_suite, CK_NORMAL)
