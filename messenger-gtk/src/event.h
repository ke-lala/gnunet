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
 * @file event.h
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "application.h"

/**
 * Event for the UI to be called whenever the application
 * causes any issue in back-end throwing a warning. This
 * might be specific to a given context or none if its
 * a general warning.
 *
 * @param app Messenger application
 * @param context Chat context or NULL
 * @param msg Warning message
 */
void
event_handle_warning(MESSENGER_Application *app,
                     struct GNUNET_CHAT_Context *context,
                     struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever the accounts
 * might add or remove an account from their list.
 *
 * @param app Messenger application
 */
void
event_refresh_accounts(MESSENGER_Application *app);

/**
 * Event for the UI to be called whenever the user
 * updates their information.
 *
 * @param app Messenger application
 */
void
event_update_profile(MESSENGER_Application *app);

/**
 * Event for the UI to be called whenever the user
 * disconnects the current account.
 *
 * @param app Messenger application
 */
void
event_cleanup_profile(MESSENGER_Application *app);

/**
 * Event for the UI to be called whenever a the user
 * creates or updates an account.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Create/Update message
 */
void
event_select_profile(MESSENGER_Application *app,
                     struct GNUNET_CHAT_Context *context,
                     struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a the user
 * joins or leaves a chat (context) via message.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Join/Leave message
 */
void
event_update_chats(MESSENGER_Application *app,
                   struct GNUNET_CHAT_Context *context,
                   struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a contact
 * joins or leaves a given context via message.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Join/Leave message
 */
void
event_presence_contact(MESSENGER_Application *app,
                       struct GNUNET_CHAT_Context *context,
                       struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a contact
 * updates their information with a message in a
 * given context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Update message
 */
void
event_update_contacts(MESSENGER_Application *app,
                      struct GNUNET_CHAT_Context *context,
                      struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever an invitation
 * message gets received in a given context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Invitation message
 */
void
event_invitation(MESSENGER_Application *app,
                 struct GNUNET_CHAT_Context *context,
                 struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a content
 * message (text or file) gets received in a given
 * context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Message
 */
void
event_receive_message(MESSENGER_Application *app,
                      struct GNUNET_CHAT_Context *context,
                      struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a message
 * gets deleted in a given context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Delete message
 */
void
event_delete_message(MESSENGER_Application *app,
                     struct GNUNET_CHAT_Context *context,
                     struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a message
 * gets tagged in a given context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Tag message
 */
void
event_tag_message(MESSENGER_Application *app,
                  struct GNUNET_CHAT_Context *context,
                  struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever an attribute
 * gets changed.
 *
 * @param app Messenger application
 */
void
event_update_attributes(MESSENGER_Application *app);

/**
 * Event for the UI to be called whenever a discourse
 * message gets received in a given context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Discourse message
 */
void
event_discourse(MESSENGER_Application *app,
                struct GNUNET_CHAT_Context *context,
                struct GNUNET_CHAT_Message *msg);

/**
 * Event for the UI to be called whenever a data
 * message gets received in a given context.
 *
 * @param app Messenger application
 * @param context Chat context
 * @param msg Data message
 */
void
event_discourse_data(MESSENGER_Application *app,
                     struct GNUNET_CHAT_Context *context,
                     struct GNUNET_CHAT_Message *msg);

#endif /* EVENT_H_ */
