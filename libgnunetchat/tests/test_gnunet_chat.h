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
 * @file test_gnunet_chat.h
 */

#ifndef TEST_GNUNET_CHAT_H_
#define TEST_GNUNET_CHAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#define CK_DEFAULT_TIMEOUT 5

#include <gnunet/gnunet_chat_lib.h>

#define SETUP_GNUNET_CHAT_ACCOUNTS(test_call, test_accounts)      \
enum GNUNET_GenericReturnValue                                    \
on_setup_##test_call (void *cls,                                  \
                      struct GNUNET_CHAT_Context *context,        \
                      struct GNUNET_CHAT_Message *message)        \
{                                                                 \
  static enum GNUNET_GenericReturnValue done = GNUNET_NO;         \
  static size_t counter = 0;                                      \
                                                                  \
  struct GNUNET_CHAT_Handle *handle = *(                          \
    (struct GNUNET_CHAT_Handle**) cls                             \
  );                                                              \
                                                                  \
  enum GNUNET_CHAT_MessageKind kind;                              \
  kind = GNUNET_CHAT_message_get_kind(message);                   \
                                                                  \
  if (GNUNET_CHAT_KIND_CREATED_ACCOUNT == kind)                   \
  {                                                               \
    const struct GNUNET_CHAT_Account *account;                    \
    account = GNUNET_CHAT_message_get_account(message);           \
    ck_assert_ptr_nonnull(account);                               \
                                                                  \
    fprintf(stdout, " - Setup account: %s\n",                     \
      GNUNET_CHAT_account_get_name(account));                     \
                                                                  \
    const char **accounts;                                        \
    for (accounts = test_accounts; *accounts; accounts++)         \
      if (0 == strcmp(GNUNET_CHAT_account_get_name(account),      \
                      *accounts))                                 \
        break;                                                    \
                                                                  \
    if (*accounts)                                                \
      counter = (counter? counter - 1 : counter);                 \
  }                                                               \
                                                                  \
                                                                  \
  if ((GNUNET_YES == done) && (0 == counter))                     \
    GNUNET_CHAT_stop(handle);                                     \
                                                                  \
  if ((GNUNET_CHAT_KIND_REFRESH != kind) || (GNUNET_YES == done)) \
    return GNUNET_YES;                                            \
                                                                  \
  for (; test_accounts[counter]; counter++)                       \
    ck_assert(GNUNET_OK == GNUNET_CHAT_account_create(            \
      handle, test_accounts[counter]                              \
    ));                                                           \
                                                                  \
  done = GNUNET_YES;                                              \
  return GNUNET_YES;                                              \
}                                                                 \
                                                                  \
void                                                              \
setup_##test_call (const struct GNUNET_CONFIGURATION_Handle *cfg) \
{                                                                 \
  static struct GNUNET_CHAT_Handle *handle = NULL;                \
  handle = GNUNET_CHAT_start(                                     \
    cfg,                                                          \
    on_setup_##test_call,                                         \
    &handle                                                       \
  );                                                              \
                                                                  \
  ck_assert_ptr_nonnull(handle);                                  \
}

#define CLEANUP_GNUNET_CHAT_ACCOUNTS(test_call, test_accounts)      \
enum GNUNET_GenericReturnValue                                      \
on_cleanup_##test_call (void *cls,                                  \
                        struct GNUNET_CHAT_Context *context,        \
                        struct GNUNET_CHAT_Message *message)        \
{                                                                   \
  static enum GNUNET_GenericReturnValue done = GNUNET_NO;           \
  static size_t counter = 0;                                        \
                                                                    \
  struct GNUNET_CHAT_Handle *handle = *(                            \
    (struct GNUNET_CHAT_Handle**) cls                               \
  );                                                                \
                                                                    \
  enum GNUNET_CHAT_MessageKind kind;                                \
  kind = GNUNET_CHAT_message_get_kind(message);                     \
                                                                    \
  if (GNUNET_CHAT_KIND_DELETED_ACCOUNT == kind)                     \
  {                                                                 \
    const struct GNUNET_CHAT_Account *account;                      \
    account = GNUNET_CHAT_message_get_account(message);             \
    ck_assert_ptr_nonnull(account);                                 \
                                                                    \
    fprintf(stdout, " - Cleanup account: %s\n",                     \
      GNUNET_CHAT_account_get_name(account));                       \
                                                                    \
    const char **accounts;                                          \
    for (accounts = test_accounts; *accounts; accounts++)           \
      if (0 == strcmp(GNUNET_CHAT_account_get_name(account),        \
                      *accounts))                                   \
        break;                                                      \
                                                                    \
    if (*accounts)                                                  \
      counter = (counter? counter - 1 : counter);                   \
  }                                                                 \
                                                                    \
  if ((GNUNET_YES == done) && (0 == counter))                       \
    GNUNET_CHAT_stop(handle);                                       \
                                                                    \
  if ((GNUNET_CHAT_KIND_REFRESH != kind) || (GNUNET_YES == done))   \
    return GNUNET_YES;                                              \
                                                                    \
  for (; test_accounts[counter]; counter++)                         \
    ck_assert(GNUNET_OK == GNUNET_CHAT_account_delete(              \
      handle, test_accounts[counter]                                \
    ));                                                             \
                                                                    \
  done = GNUNET_YES;                                                \
  return GNUNET_YES;                                                \
}                                                                   \
                                                                    \
void                                                                \
cleanup_##test_call (const struct GNUNET_CONFIGURATION_Handle *cfg) \
{                                                                   \
  static struct GNUNET_CHAT_Handle *handle = NULL;                  \
  handle = GNUNET_CHAT_start(                                       \
    cfg,                                                            \
    on_cleanup_##test_call,                                         \
    &handle                                                         \
  );                                                                \
                                                                    \
  ck_assert_ptr_nonnull(handle);                                    \
}

#define REQUIRE_GNUNET_CHAT_ACCOUNT(test_call, test_account)  \
const char *accounts_##test_call [] = {                       \
  test_account, NULL                                          \
};                                                            \
                                                              \
SETUP_GNUNET_CHAT_ACCOUNTS(test_call, accounts_##test_call)   \
CLEANUP_GNUNET_CHAT_ACCOUNTS(test_call, accounts_##test_call)

#define __CREATE_GNUNET_TEST_TASK(test_call)                     \
void                                                             \
task_##test_call (void *cls,                                     \
                  __attribute__ ((unused)) char *const *args,    \
                  __attribute__ ((unused)) const char *cfgfile,  \
                  const struct GNUNET_CONFIGURATION_Handle *cfg) \
{                                                                \
  ck_assert_ptr_nonnull(cls);                                    \
  ck_assert_ptr_nonnull(cfg);                                    \
  fprintf(                                                       \
    stdout,                                                      \
    "Stage: %s\n",                                               \
    (const char*) cls                                            \
  );                                                             \
  test_call (cfg);                                               \
}

#define __CALL_GNUNET_TEST_TASK(test_call)             \
{                                                      \
  enum GNUNET_GenericReturnValue result;               \
  const struct GNUNET_OS_ProjectData *data;            \
  struct GNUNET_GETOPT_CommandLineOption options[] = { \
    GNUNET_GETOPT_OPTION_END                           \
  };                                                   \
                                                       \
  data = GNUNET_OS_project_data_gnunet ();             \
                                                       \
  char *binary = #test_call;                           \
  char *const args [] = { binary };                \
                                                       \
  fprintf(stdout, "Running: %s\n", binary);            \
  result = GNUNET_PROGRAM_run(                         \
    data,                                              \
    1,                                                 \
    args,                                              \
    binary,                                            \
    "",                                                \
    options,                                           \
    task_##test_call,                                  \
    binary                                             \
  );                                                   \
                                                       \
  ck_assert(result == GNUNET_OK);                      \
}

#define CREATE_GNUNET_TEST(test_name, test_call) \
__CREATE_GNUNET_TEST_TASK(call_##test_call)      \
                                                 \
START_TEST(test_name)                            \
__CALL_GNUNET_TEST_TASK(call_##test_call)        \
END_TEST                                         \
                                                 \
__CREATE_GNUNET_TEST_TASK(setup_##test_call)     \
__CREATE_GNUNET_TEST_TASK(cleanup_##test_call)   \
                                                 \
void                                             \
setup_##test_name ()                             \
__CALL_GNUNET_TEST_TASK(setup_##test_call)       \
                                                 \
void                                             \
cleanup_##test_name ()                           \
__CALL_GNUNET_TEST_TASK(cleanup_##test_call)

#define START_SUITE(suite_name, suite_title) \
Suite* suite_name (void)                     \
{                                            \
  Suite *suite;                              \
  TCase *tcase;                              \
                                             \
  suite = suite_create(suite_title);

#define ADD_TEST_TO_SUITE(test_name, test_title) \
  tcase = tcase_create(test_title);              \
  tcase_add_test(tcase, test_name);              \
                                                 \
  tcase_add_checked_fixture(                     \
    tcase,                                       \
    setup_##test_name,                           \
    cleanup_##test_name                          \
  );                                             \
                                                 \
  suite_add_tcase(suite, tcase);

#define END_SUITE \
  return suite;   \
}

#define MAIN_SUITE(suite_name, suite_check)                \
int main (void)                                            \
{                                                          \
  int tests_failed;                                        \
  SRunner *runner;                                         \
                                                           \
  runner = srunner_create(suite_name ());                  \
  srunner_set_fork_status(runner, CK_NOFORK);              \
  srunner_run_all(runner, suite_check);                    \
                                                           \
  tests_failed = srunner_ntests_failed(runner);            \
  srunner_free(runner);                                    \
                                                           \
  return (tests_failed == 0? EXIT_SUCCESS : EXIT_FAILURE); \
}

#endif /* TEST_GNUNET_CHAT_H_ */
