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
 * @file gnunet_chat_lib_uml.c
 */

#include "gnunet/gnunet_chat_lib.h"
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_scheduler_lib.h>
#include <gnunet/gnunet_time_lib.h>
#include <string.h>

struct GNUNET_CHAT_Tool
{
  struct GNUNET_CHAT_Handle *handle;
  struct GNUNET_SCHEDULER_Task *task;
  char *account_name;
  char *group_name;
  char *contact_name;
  char *secret;
  bool quit;
};

static enum GNUNET_GenericReturnValue
accounts_iterate (void *cls,
                  struct GNUNET_CHAT_Handle *handle,
                  struct GNUNET_CHAT_Account *account)
{
  struct GNUNET_CHAT_Tool *tool = cls;

  const char *account_name = GNUNET_CHAT_account_get_name(account);

  if (0 == strcmp(tool->account_name, account_name))
  {
    GNUNET_CHAT_connect(
      tool->handle,
      account,
      tool->secret,
      tool->secret? strlen(tool->secret) : 0
    );

    return GNUNET_NO;
  }

  return GNUNET_YES;
}

static void
idle (void *cls)
{
  struct GNUNET_CHAT_Tool *tool = cls;

  tool->task = NULL;
  tool->quit = true;

  GNUNET_CHAT_stop(tool->handle);
}

static enum GNUNET_GenericReturnValue
chat_message (void *cls,
              struct GNUNET_CHAT_Context *context,
              struct GNUNET_CHAT_Message *message)
{
  struct GNUNET_CHAT_Tool *tool = cls;

  if (tool->task)
  {
    GNUNET_SCHEDULER_cancel(tool->task);
    tool->task = NULL;
  }

  const char *kind_name = "UNKNOWN";
  enum GNUNET_CHAT_MessageKind kind = GNUNET_CHAT_message_get_kind(
    message
  );

  switch (kind)
  {
    case GNUNET_CHAT_KIND_WARNING:
      kind_name = "WARNING";
      break;
    case GNUNET_CHAT_KIND_REFRESH:
      kind_name = "REFRESH";
      break;
    case GNUNET_CHAT_KIND_LOGIN:
      kind_name = "LOGIN";
      break;
    case GNUNET_CHAT_KIND_LOGOUT:
      kind_name = "LOGOUT";
      break;
    case GNUNET_CHAT_KIND_CREATED_ACCOUNT:
      kind_name = "CREATED_ACCOUNT";
      break;
    case GNUNET_CHAT_KIND_DELETED_ACCOUNT:
      kind_name = "DELETED_ACCOUNT";
      break;
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
      kind_name = "UPDATE_ACCOUNT";
      break;
    case GNUNET_CHAT_KIND_UPDATE_CONTEXT:
      kind_name = "UPDATE_CONTEXT";
      break;
    case GNUNET_CHAT_KIND_JOIN:
      kind_name = "JOIN";
      break;
    case GNUNET_CHAT_KIND_LEAVE:
      kind_name = "LEAVE";
      break;
    case GNUNET_CHAT_KIND_CONTACT:
      kind_name = "CONTACT";
      break;
    case GNUNET_CHAT_KIND_INVITATION:
      kind_name = "INVITATION";
      break;
    case GNUNET_CHAT_KIND_TEXT:
      kind_name = "TEXT";
      break;
    case GNUNET_CHAT_KIND_FILE:
      kind_name = "FILE";
      break;
    case GNUNET_CHAT_KIND_DELETION:
      kind_name = "DELETION";
      break;
    case GNUNET_CHAT_KIND_TAG:
      kind_name = "TAG";
      break;
    case GNUNET_CHAT_KIND_ATTRIBUTES:
      kind_name = "ATTRIBUTES";
      break;
    case GNUNET_CHAT_KIND_SHARED_ATTRIBUTES:
      kind_name = "SHARED_ATTRIBUTES";
      break;
    default:
      break;
  }

  const struct GNUNET_CHAT_Group *group = GNUNET_CHAT_context_get_group(context);
  const struct GNUNET_CHAT_Contact *contact = GNUNET_CHAT_context_get_contact(context);

  bool ignore = true;

  if (group)
  {
    const char *group_name = GNUNET_CHAT_group_get_name(group);

    if ((group_name) && (tool->group_name) && (0 == strcmp(tool->group_name, group_name)))
      ignore = false;
  }
  
  if (contact)
  {
    const char *contact_name = GNUNET_CHAT_contact_get_name(contact);

    if ((contact_name) && (tool->contact_name) && (0 == strcmp(tool->contact_name, contact_name)))
      ignore = false;
  }

  if (!ignore)
  {
    const struct GNUNET_CHAT_Contact *sender = GNUNET_CHAT_message_get_sender(message);
    const struct GNUNET_CHAT_Contact *recipient = GNUNET_CHAT_message_get_recipient(message);

    const char *sender_name = GNUNET_CHAT_contact_get_name(sender);
    const char *text = GNUNET_CHAT_message_get_text(message);

    printf(
      "%llx -> %llx: %s",
      (unsigned long long) sender,
      (unsigned long long) recipient,
      kind_name
    );

    if (sender_name)
      printf("\\n%s", sender_name);

    if (text)
      printf("\\n%s", text);

    printf("\n");
  }

  if (GNUNET_CHAT_KIND_REFRESH == kind)
    GNUNET_CHAT_iterate_accounts(
      tool->handle,
      accounts_iterate,
      tool
    );

  if ((!(tool->quit)) && (!(tool->task)))
    tool->task = GNUNET_SCHEDULER_add_delayed_with_priority(
      GNUNET_TIME_relative_get_second_(),
      GNUNET_SCHEDULER_PRIORITY_IDLE,
      idle,
      tool
    );

  return GNUNET_YES;
}

static void
run (void *cls,
     char* const* args,
     const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  struct GNUNET_CHAT_Tool *tool = cls;

  if (!(tool->account_name))
    return;

  tool->handle = GNUNET_CHAT_start(
    cfg,
    chat_message,
    tool
  );
}

int
main (int argc,
      char* const* argv)
{
  struct GNUNET_CHAT_Tool tool;
  memset(&tool, 0, sizeof(tool));

  const struct GNUNET_OS_ProjectData *data;
  data = GNUNET_OS_project_data_gnunet ();

  struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_option_string(
      'a',
      "account",
      "ACCOUNT_NAME",
      "name of account to read messages from",
      &(tool.account_name)
    ),
    GNUNET_GETOPT_option_string(
      'c',
      "contact",
      "CONTACT_NAME",
      "name of contact chat to read messages from",
      &(tool.contact_name)
    ),
    GNUNET_GETOPT_option_string(
      'g',
      "group",
      "GROUP_NAME",
      "name of group chat to read messages from",
      &(tool.group_name)
    ),
    GNUNET_GETOPT_option_string(
      'S',
      "secret",
      "SECRET",
      "storage secret for local keys",
      &(tool.secret)
    ),
    GNUNET_GETOPT_OPTION_END
  };

  printf("@startuml\n");

  enum GNUNET_GenericReturnValue result = GNUNET_PROGRAM_run(
    data,
    argc,
    argv,
    "libgnunetchat_uml",
    gettext_noop("A tool to debug the Messenger service of GNUnet."),
    options,
    &run,
    &tool
  );

  printf("@enduml\n");

  return GNUNET_OK == result? 0 : 1;
}
