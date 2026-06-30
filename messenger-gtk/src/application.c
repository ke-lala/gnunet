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
 * @file application.c
 */

#include "application.h"

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_chat_lib.h>
#include <gnunet/gnunet_configuration_lib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gtk-3.0/gtk/gtk.h>
#include <libhandy-1/handy.h>
#include <libnotify/notify.h>
#include <pipewire/impl.h>

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal-gtk3/portal-gtk3.h>
#endif

#include "request.h"
#include "resources.h"
#include "secret.h"

static void
_load_ui_stylesheets(MESSENGER_Application *app)
{
  g_assert(app);

  GdkScreen* screen = gdk_screen_get_default();
  GtkCssProvider* provider = gtk_css_provider_new();

  gtk_css_provider_load_from_resource(
    provider,
    application_get_resource_path(app, "css/style.css")
  );

  gtk_style_context_add_provider_for_screen(
    screen,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
  );
}

static gboolean
_application_main_window(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  app->init = 0;
  app->ui.state = MESSENGER_STATE_MAIN_WINDOW;

  application_show_window(app);
  return FALSE;
}

static gboolean
_application_accounts(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  app->init = 0;
  app->ui.state = MESSENGER_STATE_ACCOUNTS;

  ui_accounts_dialog_init(app, &(app->ui.accounts));
  ui_accounts_dialog_refresh(app, &(app->ui.accounts));

  gtk_widget_show(GTK_WIDGET(app->ui.accounts.dialog));
  return FALSE;
}

static void
_identity_secret_lookup(MESSENGER_Application *app,
                        const char *secret,
                        uint32_t secret_len,
                        gboolean success,
                        gboolean error,
                        gpointer user_data)
{
  g_assert((app) && (user_data));

  char *identity = user_data;

  if (error)
  {
    fprintf(stderr, "ERROR: Looking up secret failed\n");
  }
  else if ((success) && (secret) && (secret_len > 0))
  {
    struct GNUNET_CHAT_Account *account;

    application_chat_lock(app);
    account = GNUNET_CHAT_find_account(app->chat.messenger.handle, identity);
    GNUNET_CHAT_connect(app->chat.messenger.handle, account, secret, secret_len);
    application_chat_unlock(app);

    app->init = util_idle_add(G_SOURCE_FUNC(_application_main_window), app);
    return;
  }
  else
  {
    struct GNUNET_CHAT_Account *account;

    application_chat_lock(app);
    account = GNUNET_CHAT_find_account(app->chat.messenger.handle, identity);
    application_chat_unlock(app);

    if (account)
    {
      secret_operation_generate(app, identity, &_identity_secret_lookup, identity);
      return;
    }
  }

  app->init = util_idle_add(G_SOURCE_FUNC(_application_accounts), app);
}

static void
_application_init(MESSENGER_Application *app)
{
  g_assert(app);

  schedule_load_glib(&(app->ui.schedule));

  ui_messenger_init(app, &(app->ui.messenger));

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  if (app->portal)
    app->parent = xdp_parent_new_gtk(GTK_WINDOW(app->ui.messenger.main_window));
#endif

  if (app->chat.identity)
    secret_operation_lookup(app, app->chat.identity, &_identity_secret_lookup, app->chat.identity);
  else
    app->init = util_idle_add(G_SOURCE_FUNC(_application_accounts), app);
}

static void
_application_activate(GApplication* application,
                      gpointer user_data)
{
  g_assert((application) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  g_application_hold(application);

  _application_init(app);

  g_application_release(application);
}

static void
_application_open(GApplication* application,
                  GFile **files,
                  gint n_files,
                  UNUSED gchar* hint,
                  gpointer user_data)
{
  g_assert((application) && (files) && (user_data));

  MESSENGER_Application *app = (MESSENGER_Application*) user_data;

  g_application_hold(application);

  _application_init(app);

  for (gint i = 0; i < n_files; i++) {
    if (!g_file_has_uri_scheme(files[i], "gnunet"))
      continue;

    gchar *uri_string = g_file_get_uri(files[i]);

    if (!uri_string)
      continue;

    char *emsg = NULL;
    struct GNUNET_CHAT_Uri *uri = GNUNET_CHAT_uri_parse(uri_string, &emsg);

    if (emsg)
    {
      g_printerr("ERROR: %s\n", emsg);
      GNUNET_free(emsg);
    }

    if (!uri)
      goto free_string;

    GNUNET_CHAT_lobby_join(app->chat.messenger.handle, uri);
    GNUNET_CHAT_uri_destroy(uri);

  free_string:
    g_free(uri_string);
  }

  g_application_release(application);
}

void
application_init(MESSENGER_Application *app,
                 int argc,
                 char **argv)
{
  g_assert((app) && (argv));

  memset(app, 0, sizeof(*app));

  app->argc = argc;
  app->argv = argv;

  setlocale(LC_ALL, "");
  bindtextdomain(MESSENGER_APPLICATION_DOMAIN, MESSENGER_APPLICATION_LOCALEDIR);
  bind_textdomain_codeset(MESSENGER_APPLICATION_DOMAIN, "UTF-8");
  textdomain(MESSENGER_APPLICATION_DOMAIN);

  pw_init(&argc, &argv);
  gst_init(&argc, &argv);
  gtk_init(&argc, &argv);
  hdy_init();

  app->application = gtk_application_new(
    MESSENGER_APPLICATION_ID,
    G_APPLICATION_HANDLES_OPEN |
    G_APPLICATION_NON_UNIQUE
  );

  resources_register();

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  GError *error = NULL;
  app->portal = xdp_portal_initable_new(&error);
  app->session = NULL;

  if (error)
  {
    g_printerr("ERROR: %s\n", error->message);
    g_error_free(error);
  }
#endif

  notify_init(MESSENGER_APPLICATION_NAME);
  app->notifications = NULL;
  app->requests = NULL;
  app->secrets = NULL;

  _load_ui_stylesheets(app);

  schedule_init(&(app->chat.schedule));
  schedule_init(&(app->ui.schedule));

  app->chat.status = EXIT_FAILURE;
  app->chat.tid = 0;

  app->quarks.widget = g_quark_from_string("messenger_widget");
  app->quarks.data = g_quark_from_string("messenger_data");
  app->quarks.ui = g_quark_from_string("messenger_ui");

  app->pw.main_loop = pw_main_loop_new(NULL);
  app->pw.loop = app->pw.main_loop? pw_main_loop_get_loop(app->pw.main_loop) : NULL;

  if (app->pw.loop)
    app->pw.context = pw_context_new(
      app->pw.loop,
      pw_properties_new(
        PW_KEY_CORE_DAEMON,
        NULL,
        NULL
      ),
      0
    );

  if (app->pw.context)
    pw_context_load_module(app->pw.context, "libpipewire-module-link-factory", NULL, NULL);

#ifdef MESSENGER_APPLICATION_NO_PORTAL
  application_pw_core_init(app);
  application_pw_main_loop_run(app);
#endif

  g_application_add_main_option(
    G_APPLICATION(app->application),
    "ego",
    'e',
    G_OPTION_FLAG_NONE,
    G_OPTION_ARG_STRING,
    "Identity to select for messaging",
    "IDENTITY"
  );

  g_signal_connect(
    app->application,
    "activate",
    G_CALLBACK(_application_activate),
    app
  );

  g_signal_connect(
    app->application,
    "open",
    G_CALLBACK(_application_open),
    app
  );
}

const gchar*
application_get_resource_path(MESSENGER_Application *app,
                              const char *path)
{
  g_assert((app) && (path));

  static gchar resource_path [PATH_MAX];

  const gchar *base_path = g_application_get_resource_base_path(
    G_APPLICATION(app->application)
  );

  snprintf(resource_path, PATH_MAX, "%s/%s", base_path, path);
  return resource_path;
}

static void*
_application_chat_thread(void *args)
{
  g_assert(args);

  MESSENGER_Application *app = (MESSENGER_Application*) args;

  const struct GNUNET_OS_ProjectData *data =
    GNUNET_OS_project_data_gnunet();

  struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_option_string (
      'e',
      "ego",
      "IDENTITY",
      "Identity to select for messaging",
      &(app->chat.identity)
    ),
    GNUNET_GETOPT_OPTION_END
  };

  app->chat.status = (GNUNET_PROGRAM_run(
    data,
    app->argc,
    app->argv,
    MESSENGER_APPLICATION_BINARY,
    gettext_noop(MESSENGER_APPLICATION_DESCRIPTION),
    options,
    &chat_messenger_run,
    app
  ) == GNUNET_OK? EXIT_SUCCESS : EXIT_FAILURE);

  return NULL;
}

void
application_run(MESSENGER_Application *app)
{
  g_assert(app);

  // Start thread to run GNUnet scheduler
  pthread_create(
    &(app->chat.tid),
    NULL,
    _application_chat_thread,
    app
  );

  app->ui.status = g_application_run(
    G_APPLICATION(app->application),
    app->argc,
    app->argv
  );

  if (app->ui.status != 0)
    application_exit(app, MESSENGER_FAIL);

  // Wait for other thread to stop properly
  pthread_join(app->chat.tid, NULL);

  GList *list;

  // Get rid of open secret operations
  list = app->secrets;

  while (list)
  {
    if (list->data)
    {
      secret_operation_cancel((MESSENGER_SecretOperation*) list->data);
      secret_operation_destroy((MESSENGER_SecretOperation*) list->data);
    }

    list = list->next;
  }
  
  // Get rid of open requests
  list = app->requests;

  while (list)
  {
    if (list->data)
    {
      request_cancel((MESSENGER_Request*) list->data);
      request_delete((MESSENGER_Request*) list->data);
    }

    list = list->next;
  }
  
  // Get rid of open notifications
  list = app->notifications;

  while (list)
  {
    if (list->data)
      notify_notification_close(NOTIFY_NOTIFICATION(list->data), NULL);

    list = list->next;
  }

  if (app->secrets)
    g_list_free(app->secrets);

  if (app->requests)
    g_list_free(app->requests);

  if (app->notifications)
    g_list_free(app->notifications);

  notify_uninit();

  resources_unregister();

  g_object_unref(app->application);
}

void
application_pw_main_loop_run(MESSENGER_Application *app)
{
  g_assert(app);

  if (!(app->pw.main_loop))
    return;

  pw_main_loop_run(app->pw.main_loop);
}

#ifndef MESSENGER_APPLICATION_NO_PORTAL
void
application_set_active_session(MESSENGER_Application *app,
                               XdpSession *session)
{
  g_assert(app);

  if (app->session == session)
    return;

  if (app->session)
  {
    const XdpSessionState state = xdp_session_get_session_state(
      app->session
    );

    if (XDP_SESSION_ACTIVE == state)
      xdp_session_close(app->session);

    g_object_unref(app->session);
  }

  app->session = session;
}

int
application_get_active_session_remote(MESSENGER_Application *app)
{
  g_assert(app);

  if (!(app->session))
    return -1;

  const XdpSessionState state = xdp_session_get_session_state(
    app->session
  );

  if (XDP_SESSION_ACTIVE != state)
    return -1;
  
  return xdp_session_open_pipewire_remote(app->session);
}
#endif

static void
_request_background_callback(MESSENGER_Application *app,
                             gboolean success,
                             gboolean error,
                             gpointer user_data)
{
  g_assert((app) && (user_data));

  gboolean *setting = (gboolean*) user_data;
  *setting = success;
}

void
application_show_window(MESSENGER_Application *app)
{
  g_assert(app);
  
  if (MESSENGER_STATE_MAIN_WINDOW != app->ui.state)
    return;

  gtk_widget_show(GTK_WIDGET(app->ui.messenger.main_window));

  request_new_background(
    app,
    XDP_BACKGROUND_FLAG_AUTOSTART,
    _request_background_callback,
    &(app->settings.autostart)
  );

  request_new_background(
    app,
    XDP_BACKGROUND_FLAG_ACTIVATABLE,
    _request_background_callback,
    &(app->settings.background_task)
  );
}

typedef struct MESSENGER_ApplicationEventCall
{
  MESSENGER_Application *app;
  MESSENGER_ApplicationEvent event;
} MESSENGER_ApplicationEventCall;

static gboolean
_application_event_call(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) user_data;

  call->event(call->app);

  GNUNET_free(call);
  return TRUE;
}

void
application_call_event(MESSENGER_Application *app,
                       MESSENGER_ApplicationEvent event)
{
  g_assert((app) && (event));

  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) GNUNET_malloc(
    sizeof(MESSENGER_ApplicationEventCall)
  );

  call->app = app;
  call->event = event;

  schedule_sync_run(
    &(app->ui.schedule),
    G_SOURCE_FUNC(_application_event_call),
    call
  );
}

static gboolean
_application_sync_event_call(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) user_data;

  call->event(call->app);

  GNUNET_free(call);
  return TRUE;
}

void
application_call_sync_event(MESSENGER_Application *app,
                            MESSENGER_ApplicationEvent event)
{
  g_assert((app) && (event));

  MESSENGER_ApplicationEventCall *call;

  call = (MESSENGER_ApplicationEventCall*) GNUNET_malloc(
    sizeof(MESSENGER_ApplicationEventCall)
  );

  call->app = app;
  call->event = event;

  schedule_sync_run(
    &(app->ui.schedule),
    G_SOURCE_FUNC(_application_sync_event_call),
    call
  );
}

typedef struct MESSENGER_ApplicationMessageEventCall
{
  MESSENGER_Application *app;
  MESSENGER_ApplicationMessageEvent event;

  struct GNUNET_CHAT_Context *context;
  struct GNUNET_CHAT_Message *message;
} MESSENGER_ApplicationMessageEventCall;

static gboolean
_application_message_event_call(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_ApplicationMessageEventCall *call;

  call = (MESSENGER_ApplicationMessageEventCall*) user_data;

  call->event(call->app, call->context, call->message);

  GNUNET_free(call);
  return TRUE;
}

void
application_call_message_event(MESSENGER_Application *app,
                               MESSENGER_ApplicationMessageEvent event,
                               struct GNUNET_CHAT_Context *context,
                               struct GNUNET_CHAT_Message *message)
{
  g_assert((app) && (event) && (message));

  MESSENGER_ApplicationMessageEventCall *call;

  call = (MESSENGER_ApplicationMessageEventCall*) GNUNET_malloc(
    sizeof(MESSENGER_ApplicationMessageEventCall)
  );

  call->app = app;
  call->event = event;

  call->context = context;
  call->message = message;

  schedule_sync_run(
    &(app->ui.schedule),
    G_SOURCE_FUNC(_application_message_event_call),
    call
  );
}

void
application_chat_lock(MESSENGER_Application *app)
{
  g_assert(app);

  if (app->ui.schedule.function)
  {
    g_assert(!(app->chat.schedule.locked));
    return;
  }
  
  schedule_sync_lock(&(app->chat.schedule));
}

void
application_chat_unlock(MESSENGER_Application *app)
{
  g_assert(app);

  if (app->ui.schedule.function)
  {
    g_assert(!(app->chat.schedule.locked));
    return;
  }

  schedule_sync_unlock(&(app->chat.schedule));
}

static gboolean
_application_stop_chat(gpointer user_data)
{
  MESSENGER_Application *app = user_data;

  GNUNET_CHAT_disconnect(app->chat.messenger.handle);
  GNUNET_CHAT_stop(app->chat.messenger.handle);
  app->chat.messenger.handle = NULL;

  GNUNET_SCHEDULER_shutdown();
  return FALSE;
}

void
application_exit(MESSENGER_Application *app,
                 MESSENGER_ApplicationSignal signal)
{
  g_assert(app);

  schedule_sync_run(
    &(app->chat.schedule),
    G_SOURCE_FUNC(_application_stop_chat),
    app
  );

  schedule_cleanup(&(app->chat.schedule));
  schedule_cleanup(&(app->ui.schedule));

  util_scheduler_cleanup();

  media_pw_cleanup(&(app->media.camera));
  media_pw_cleanup(&(app->media.screen));

  if (app->pw.context)
    pw_context_destroy(app->pw.context);

	if (app->pw.main_loop)
  {
    pw_main_loop_quit(app->pw.main_loop);
    pw_main_loop_destroy(app->pw.main_loop);
  }

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  application_set_active_session(app, NULL);

  if (app->portal)
    g_object_unref(app->portal);

  app->portal = NULL;
#endif

  gst_deinit();
  pw_deinit();
}

int
application_status(MESSENGER_Application *app)
{
  g_assert(app);
  
  if (EXIT_SUCCESS != app->chat.status)
    return app->chat.status;

  return app->ui.status;
}
