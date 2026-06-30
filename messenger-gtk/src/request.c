/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

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
 * @file request.c
 */

#include "request.h"

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal/portal.h>
#endif

#ifdef MESSENGER_APPLICATION_NO_PORTAL

static gboolean
_request_timeout_call(gpointer user_data)
{
  g_assert(user_data);

  MESSENGER_Request* request = (MESSENGER_Request*) user_data;

  MESSENGER_Application *app = request->application;
  MESSENGER_RequestCallback callback = request->callback;
  gpointer data = request->user_data;

  request_cleanup(request);
  request_drop(request);

  if (callback)
    callback(app, TRUE, FALSE, data);

  return FALSE;
}

#endif

MESSENGER_Request*
request_new(MESSENGER_Application *application,
            MESSENGER_RequestCallback callback,
            GCancellable *cancellable,
            gpointer user_data)
{
  g_assert((application) && (cancellable));

  MESSENGER_Request* request = g_malloc(sizeof(MESSENGER_Request));

  request->application = application;
  request->callback = callback;
  request->cancellable = cancellable;
  request->user_data = user_data;

#ifdef MESSENGER_APPLICATION_NO_PORTAL
  request->timeout = util_immediate_add(
    G_SOURCE_FUNC(_request_timeout_call),
    request
  );
#endif

  application->requests = g_list_append(
    application->requests, 
    request
  );

  return request;
}

#ifndef MESSENGER_APPLICATION_NO_PORTAL
static void
_request_background_callback(GObject *source_object,
                             GAsyncResult *result,
                             gpointer user_data)
{
  g_assert((source_object) && (result) && (user_data));

  XdpPortal *portal = XDP_PORTAL(source_object);
  MESSENGER_Request *request = (MESSENGER_Request*) user_data;

  request_cleanup(request);

  MESSENGER_Application *app = request->application;
  MESSENGER_RequestCallback callback = request->callback;
  gpointer data = request->user_data;
  
  GError *error = NULL;
  gboolean success = xdp_portal_request_background_finish(
    portal, result, &error
  );

  request_drop(request);

  gboolean error_value = false;
  if (error)
  {
    g_printerr("ERROR: %s\n", error->message);
    g_error_free(error);

    error_value = true;
  }

  if (callback)
    callback(app, success, error_value, data);
}
#endif

MESSENGER_Request*
request_new_background(MESSENGER_Application *application,
                       XdpBackgroundFlags flags,
                       MESSENGER_RequestCallback callback,
                       gpointer user_data)
{
  g_assert((application) && (callback));

  GCancellable* cancellable = g_cancellable_new();

  if (!cancellable)
    return NULL;

  MESSENGER_Request* request = request_new(
    application,
    callback,
    cancellable,
    user_data
  );

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  xdp_portal_request_background(
    application->portal,
    application->parent,
    NULL,
    NULL,
    flags,
    cancellable,
    _request_background_callback,
    request
  );
#endif

  return request;
}

#ifndef MESSENGER_APPLICATION_NO_PORTAL
static void
_request_camera_callback(GObject *source_object,
                         GAsyncResult *result,
                         gpointer user_data)
{
  g_assert((source_object) && (result) && (user_data));

  XdpPortal *portal = (XdpPortal*) source_object;
  MESSENGER_Request *request = (MESSENGER_Request*) user_data;

  request_cleanup(request);

  MESSENGER_Application *app = request->application;
  MESSENGER_RequestCallback callback = request->callback;
  gpointer data = request->user_data;

  GError *error = NULL;
  gboolean success = xdp_portal_access_camera_finish(
    portal, result, &error
  );

  request_drop(request);

  gboolean error_value = false;
  if (error)
  {
    g_printerr("ERROR: %s\n", error->message);
    g_error_free(error);

    error_value = true;
  }

  if (callback)
    callback(app, success, error_value, data);
}
#endif

MESSENGER_Request*
request_new_camera(MESSENGER_Application *application,
                   XdpCameraFlags flags,
                   MESSENGER_RequestCallback callback,
                   gpointer user_data)
{
  g_assert((application) && (callback));

  GCancellable* cancellable = g_cancellable_new();

  if (!cancellable)
    return NULL;

  MESSENGER_Request* request = request_new(
    application,
    callback,
    cancellable,
    user_data
  );

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  xdp_portal_access_camera(
    application->portal,
    application->parent,
    flags,
    cancellable,
    _request_camera_callback,
    request
  );
#endif

  return request;
}

#ifndef MESSENGER_APPLICATION_NO_PORTAL
static void
_request_session_start_callback(GObject *source_object,
                                GAsyncResult *result,
                                gpointer user_data)
{
  g_assert((source_object) && (result) && (user_data));

  XdpSession *session = (XdpSession*) source_object;
  MESSENGER_Request *request = (MESSENGER_Request*) user_data;

  request_cleanup(request);

  MESSENGER_Application *app = request->application;
  MESSENGER_RequestCallback callback = request->callback;
  gpointer data = request->user_data;

  GError *error = NULL;
  gboolean success = xdp_session_start_finish(
    session,
    result,
    &error
  );

  application_set_active_session(app, success? session : NULL);
  request_drop(request);

  gboolean error_value = false;
  if (error)
  {
    g_printerr("ERROR: %s\n", error->message);
    g_error_free(error);

    error_value = true;
  }

  if (callback)
    callback(app, success, error_value, data);
}

static void
_request_screencast_callback(GObject *source_object,
                             GAsyncResult *result,
                             gpointer user_data)
{
  g_assert((source_object) && (result) && (user_data));

  XdpPortal *portal = (XdpPortal*) source_object;
  MESSENGER_Request *request = (MESSENGER_Request*) user_data;

  request_cleanup(request);

  MESSENGER_Application *app = request->application;
  MESSENGER_RequestCallback callback = request->callback;
  gpointer data = request->user_data;

  GError *error = NULL;
  XdpSession *session = xdp_portal_create_screencast_session_finish(
    portal, result, &error
  );

  if (session)
    application_set_active_session(app, session);
  request_drop(request);

  gboolean error_value = false;
  if (error)
  {
    g_printerr("ERROR: %s\n", error->message);
    g_error_free(error);

    error_value = true;
  }

  if (!session)
    goto skip_session_start;

  GCancellable* cancellable = g_cancellable_new();

  if (!cancellable)
    goto skip_session_start;

  request = request_new(
    app,
    callback,
    cancellable,
    data
  );

  xdp_session_start(
    session,
    app->parent,
    cancellable,
    _request_session_start_callback,
    request
  );

  return;
  
skip_session_start:
  if (callback)
    callback(app, false, error_value, data);
}
#endif

MESSENGER_Request*
request_new_screencast(MESSENGER_Application *application,
                       XdpOutputType outputs,
                       XdpScreencastFlags flags,
                       XdpCursorMode cursor_mode,
                       XdpPersistMode persist_mode,
                       MESSENGER_RequestCallback callback,
                       gpointer user_data)
{
  g_assert((application) && (callback));

  GCancellable* cancellable = g_cancellable_new();

  if (!cancellable)
    return NULL;

  MESSENGER_Request* request = request_new(
    application,
    callback,
    cancellable,
    user_data
  );

#ifndef MESSENGER_APPLICATION_NO_PORTAL
  xdp_portal_create_screencast_session(
    application->portal,
    outputs,
    flags,
    cursor_mode,
    persist_mode,
    NULL,
    cancellable,
    _request_screencast_callback,
    request
  );
#endif

  return request;
}

void
request_cancel(MESSENGER_Request *request)
{
  g_assert(request);

#ifdef MESSENGER_APPLICATION_NO_PORTAL
  if (request->timeout)
    util_source_remove(request->timeout);

  request->timeout = 0;
#endif

  if (!request->cancellable)
    return;
  
  if (!g_cancellable_is_cancelled(request->cancellable))
    g_cancellable_cancel(request->cancellable);
}

void
request_cleanup(MESSENGER_Request *request)
{
  g_assert(request);

#ifdef MESSENGER_APPLICATION_NO_PORTAL
  if (request->timeout)
    util_source_remove(request->timeout);

  request->timeout = 0;
#endif

  if (!request->cancellable)
    return;

  g_object_unref(request->cancellable);
  request->cancellable = NULL;
}

void
request_drop(MESSENGER_Request *request)
{
  g_assert(request);

  if (request->application->requests)
    request->application->requests = g_list_remove(
      request->application->requests,
      request
    );

  request_delete(request);
}

void
request_delete(MESSENGER_Request *request)
{
  g_assert(request);

  request_cleanup(request);
  g_free(request);
}
