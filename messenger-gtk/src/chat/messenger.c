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
 * @file chat/messenger.c
 */

#include "messenger.h"

#include "../event.h"
#include <gnunet/gnunet_chat_lib.h>

static int
_chat_messenger_message(void *cls,
                        struct GNUNET_CHAT_Context *context,
                        struct GNUNET_CHAT_Message *message)
{
  g_assert((cls) && (message));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  if (GNUNET_YES == GNUNET_CHAT_message_is_deleted(message))
  {
    application_call_message_event(
        app,
        event_delete_message,
        context,
        message
    );

    goto skip_message_handling;
  }

  // Handle each kind of message as proper event regarding context
  switch (GNUNET_CHAT_message_get_kind(message))
  {
    case GNUNET_CHAT_KIND_WARNING:
      application_call_message_event(
      	  app,
      	  event_handle_warning,
      	  context,
      	  message
      );
      break;
    case GNUNET_CHAT_KIND_REFRESH:
    {
      application_call_event(app, event_refresh_accounts);
      break;
    }
    case GNUNET_CHAT_KIND_LOGIN:
    {
      application_call_event(app, event_update_profile);
      break;
    }
    case GNUNET_CHAT_KIND_LOGOUT:
    {
      application_call_sync_event(app, event_cleanup_profile);
      break;
    }
    case GNUNET_CHAT_KIND_CREATED_ACCOUNT:
    case GNUNET_CHAT_KIND_UPDATE_ACCOUNT:
    {
      application_call_message_event(
          app,
          event_select_profile,
          context,
          message
      );
      break;
    }
    case GNUNET_CHAT_KIND_UPDATE_CONTEXT:
    {
      application_call_message_event(
          app,
          event_update_chats,
          context,
          message
      );
      break;
    }
    case GNUNET_CHAT_KIND_JOIN:
    case GNUNET_CHAT_KIND_LEAVE:
    {
      application_call_message_event(
      	  app,
      	  (GNUNET_YES == GNUNET_CHAT_message_is_sent(message)?
      	      event_update_chats :
      	      event_presence_contact
      	  ),
      	  context,
      	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_CONTACT:
    case GNUNET_CHAT_KIND_SHARED_ATTRIBUTES:
    {
      application_call_message_event(
      	  app,
      	  event_update_contacts,
      	  context,
      	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_INVITATION:
    {
      application_call_message_event(
          app,
          event_invitation,
          context,
          message
      );
      break;
    }
    case GNUNET_CHAT_KIND_TEXT:
    case GNUNET_CHAT_KIND_FILE:
    {
      application_call_message_event(
          app,
          event_receive_message,
          context,
          message
      );
      break;
    }
    case GNUNET_CHAT_KIND_DELETION:
    {
      struct GNUNET_CHAT_Message *target;
      target = GNUNET_CHAT_message_get_target(message);

      if (target) 
        application_call_message_event(
            app,
            event_delete_message,
            context,
            target
        );
      break;
    }
    case GNUNET_CHAT_KIND_TAG:
    {
      application_call_message_event(
      	  app,
      	  event_tag_message,
      	  context,
      	  message
      );
      break;
    }
    case GNUNET_CHAT_KIND_ATTRIBUTES:
    {
      application_call_event(app, event_update_attributes);
      break;
    }
    case GNUNET_CHAT_KIND_DISCOURSE:
    {
      application_call_message_event(
          app,
          event_discourse,
          context,
          message
      );
      break;
    }
    case GNUNET_CHAT_KIND_DATA:
    {
      application_call_message_event(
          app,
          event_discourse_data,
          context,
          message
      );
      break;
    }
    default:
      break;
  }

skip_message_handling:
  return GNUNET_YES;
}

void
chat_messenger_run(void *cls,
                   UNUSED char *const *args,
                   UNUSED const char *cfgfile,
                   const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  g_assert((cls) && (cfg));

  MESSENGER_Application *app = (MESSENGER_Application*) cls;

  schedule_load_gnunet(&(app->chat.schedule));

  app->chat.messenger.handle = GNUNET_CHAT_start(
      cfg,
      &_chat_messenger_message,
      app
  );
}
