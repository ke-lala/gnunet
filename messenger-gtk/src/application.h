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
 * @file application.h
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal/portal.h>
#endif

#include <pipewire/pipewire.h>
#include <pthread.h>

#include <gnunet/gnunet_chat_lib.h>

#include "chat/messenger.h"

#include "ui/about.h"
#include "ui/accounts.h"
#include "ui/contact_info.h"
#include "ui/contacts.h"
#include "ui/delete_messages.h"
#include "ui/discourse.h"
#include "ui/files.h"
#include "ui/invite_contact.h"
#include "ui/messenger.h"
#include "ui/new_account.h"
#include "ui/new_contact.h"
#include "ui/new_group.h"
#include "ui/new_lobby.h"
#include "ui/new_platform.h"
#include "ui/new_tag.h"
#include "ui/play_media.h"
#include "ui/send_file.h"
#include "ui/settings.h"

#include "media.h"
#include "schedule.h"
#include "util.h"

#define MESSENGER_APPLICATION_APPNAME "GNUnet Messenger"
#define MESSENGER_APPLICATION_NAME "Messenger-GTK"
#define MESSENGER_APPLICATION_DESCRIPTION \
  "A GTK based GUI for the Messenger service of GNUnet."
#define MESSENGER_APPLICATION_TITLE "Messenger"
#define MESSENGER_APPLICATION_SUBTITLE "GNUnet"

typedef enum MESSENGER_ApplicationSignal
{
  MESSENGER_NONE = 0,
  MESSENGER_QUIT = 1,
  MESSENGER_FAIL = 2
} MESSENGER_ApplicationSignal;

typedef enum MESSENGER_ApplicationState
{
  MESSENGER_STATE_UNKNOWN = 0,
  MESSENGER_STATE_ACCOUNTS = 1,
  MESSENGER_STATE_NEW_ACCOUNT = 2,
  MESSENGER_STATE_MAIN_WINDOW = 3
} MESSENGER_ApplicationState;

typedef struct MESSENGER_Application
{
  char **argv;
  int argc;

  GtkApplication *application;
  GList *notifications;
  GList *requests;
  GList *secrets;
  guint init;

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  XdpPortal *portal;
  XdpParent *parent;
  XdpSession *session;
#endif

  struct {
    GQuark widget;
    GQuark data;
    GQuark ui;
  } quarks;

  struct {
    struct pw_main_loop *main_loop;
    struct pw_loop *loop;
    struct pw_context *context;
  } pw;

  struct {
    MESSENGER_MediaInfo camera;
    MESSENGER_MediaInfo screen;
  } media;

  struct {
    int status;
    pthread_t tid;
    char *identity;

    CHAT_MESSENGER_Handle messenger;

    MESSENGER_Schedule schedule;
  } chat;

  struct {
    MESSENGER_ApplicationState state;
    int status;

    UI_MESSENGER_Handle messenger;
    UI_ABOUT_Handle about;

    UI_CONTACT_INFO_Handle contact_info;
    UI_DELETE_MESSAGES_Handle delete_messages;
    UI_DISCOURSE_Handle discourse;
    UI_INVITE_CONTACT_Handle invite_contact;
    UI_SEND_FILE_Handle send_file;
    UI_PLAY_MEDIA_Handle play_media;

    UI_NEW_ACCOUNT_Handle new_account;
    UI_NEW_CONTACT_Handle new_contact;
    UI_NEW_GROUP_Handle new_group;
    UI_NEW_LOBBY_Handle new_lobby;
    UI_NEW_PLATFORM_Handle new_platform;
    UI_NEW_TAG_Handle new_tag;

    UI_ACCOUNTS_Handle accounts;
    UI_FILES_Handle files;
    UI_CONTACTS_Handle contacts;
    UI_SETTINGS_Handle settings;

    MESSENGER_Schedule schedule;
  } ui;

  struct {
    gboolean hide_delete_dialog;

    gboolean autostart;
    gboolean background_task;

    gboolean disable_notifications;
    gboolean play_notification_sounds;

    gboolean send_read_receipts;

    gulong auto_delete_delay;

    gboolean accept_all_invitations;
    gulong delete_invitations_delay;

    gboolean accept_all_files;
    gchar *download_folder_path;
    gulong delete_files_delay;

    gulong leave_chats_delay;
  } settings;
} MESSENGER_Application;

/**
 * Initializes the messenger application with
 * startup arguments.
 *
 * @param app Messenger application
 * @param argc Amount of arguments
 * @param argv Arguments
 */
void
application_init(MESSENGER_Application *app,
                 int argc,
                 char **argv);

/**
 * Returns the path from resources of the
 * messenger application relative to its storage.
 *
 * @param app Messenger application
 * @param path Path
 * @return Resource path
 */
const gchar*
application_get_resource_path(MESSENGER_Application *app,
                              const char *path);

/**
 * Runs the messenger application starting the
 * second thread and waiting for the application
 * to finish.
 *
 * @param app Messenger application
 */
void
application_run(MESSENGER_Application *app);

/**
 * Initialize the pipewire core of the messenger 
 * application if possible.
 *
 * @param app Messenger application
 */
void
application_pw_core_init(MESSENGER_Application *app);

/**
 * Cleanups the pipewire core of the messenger
 * application if available.
 */
void
application_pw_core_cleanup(MESSENGER_Application *app);

/**
 * Run the pipewire main loop of the messenger 
 * application if available.
 *
 * @param app Messenger application
 */
void
application_pw_main_loop_run(MESSENGER_Application *app);

#ifndef MESSENGER_APPLICATION_NO_PORTAL
/**
 * Sets the active session for the messenger 
 * application and frees the previous one
 * if any is still active.
 *
 * @param app Messenger application
 * @param session Screencast session
 */
void
application_set_active_session(MESSENGER_Application *app,
                               XdpSession *session);

/**
 * Returns the file descriptor to the pipewire 
 * remote where the screencast streams of the
 * active session from the messenger application
 * are available.
 *
 * @param app Messenger application
 * @return File descriptor
 */
int
application_get_active_session_remote(MESSENGER_Application *app);
#endif

/**
 * Shows the messenger application main window.
 *
 * @param app Messenger application
 */
void
application_show_window(MESSENGER_Application *app);

typedef void (*MESSENGER_ApplicationEvent) (
  MESSENGER_Application *app
);

typedef void (*MESSENGER_ApplicationMessageEvent) (
  MESSENGER_Application *app,
  struct GNUNET_CHAT_Context *context,
  struct GNUNET_CHAT_Message *msg
);

/**
 * Calls a given event with the messenger application
 * asyncronously but explicitly synchronized via mutex.
 *
 * @param app Messenger application
 * @param event Event
 */
void
application_call_event(MESSENGER_Application *app,
                       MESSENGER_ApplicationEvent event);

/**
 * Calls a given event with the messenger application
 * syncronously.
 *
 * @param app Messenger application
 * @param event Event
 */
void
application_call_sync_event(MESSENGER_Application *app,
                            MESSENGER_ApplicationEvent event);

/**
 * Calls a given message event with the messenger
 * application asyncronously but explicitly synchronized
 * via mutex.
 *
 * @param app Messenger application
 * @param event Message event
 * @param context Chat context
 * @param message Message
 */
void
application_call_message_event(MESSENGER_Application *app,
                               MESSENGER_ApplicationMessageEvent event,
                               struct GNUNET_CHAT_Context *context,
                               struct GNUNET_CHAT_Message *message);

/**
 * Lock the thread of the GNUnet scheduler
 * until it gets unlocked again.
 *
 * @param app Messenger application
 */
void
application_chat_lock(MESSENGER_Application *app);

/**
 * Unlock the thread of the GNUnet scheduler
 * after being locked.
 *
 * @param app Messenger application
 */
void
application_chat_unlock(MESSENGER_Application *app);

/**
 * Signals the second thread to exit the application.
 *
 * @param app Messenger application
 * @param signal Exit signal
 */
void
application_exit(MESSENGER_Application *app,
                 MESSENGER_ApplicationSignal signal);

/**
 * Returns the exit status of the messenger application.
 *
 * @param app Messenger application
 * @return Exit status
 */
int
application_status(MESSENGER_Application *app);

#endif /* APPLICATION_H_ */
