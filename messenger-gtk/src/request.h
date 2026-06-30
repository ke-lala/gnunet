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
 * @file request.h
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <gio/gio.h>

#ifndef MESSENGER_APPLICATION_NO_PORTAL
#include <libportal/portal.h>
#else
typedef enum XdpBackgroundFlags {
  XDP_BACKGROUND_FLAG_ACTIVATABLE = 1,
  XDP_BACKGROUND_FLAG_AUTOSTART = 2,
  XDP_BACKGROUND_FLAG_NONE = 0,
} XdpBackgroundFlags;

typedef enum XdpCameraFlags {
  XDP_CAMERA_FLAG_NONE = 0,
} XdpCameraFlags;

typedef enum XdpOutputType {
  XDP_OUTPUT_NONE = 0,
  XDP_OUTPUT_MONITOR = 1,
  XDP_OUTPUT_WINDOW = 2,
  XDP_OUTPUT_VIRTUAL = 4,
} XdpOutputType;

typedef enum XdpScreencastFlags {
  XDP_SCREENCAST_FLAG_NONE = 0,
  XDP_SCREENCAST_FLAG_MULTIPLE = 1,
} XdpScreencastFlags;

typedef enum XdpCursorMode {
  XDP_CURSOR_MODE_HIDDEN = 1,
  XDP_CURSOR_MODE_EMBEDDED = 2,
  XDP_CURSOR_MODE_METADATA = 4,
} XdpCursorMode;

typedef enum XdpPersistMode {
  XDP_PERSIST_MODE_NONE = 0,
  XDP_PERSIST_MODE_TRANSIENT = 1,
  XDP_PERSIST_MODE_PERSISTENT = 2,
} XdpPersistMode;
#endif

#include "application.h"

typedef void (*MESSENGER_RequestCallback)(
  MESSENGER_Application *application,
  gboolean success,
  gboolean error,
  gpointer user_data
);

typedef struct MESSENGER_Request {
  MESSENGER_Application *application;
  MESSENGER_RequestCallback callback;
  GCancellable *cancellable;
  gpointer user_data;

#ifdef MESSENGER_APPLICATION_NO_PORTAL
  guint timeout;
#endif
} MESSENGER_Request;

/**
 * Creates a new request for the messsenger
 * application for a certain permission.
 *
 * The request object will automatically be
 * added to the list of the messenger application.
 *
 * @param application Messenger application
 * @param cancellable Cancellable object (optional)
 * @param user_data User data (optional)
 * @return New request object
 */
MESSENGER_Request*
request_new(MESSENGER_Application *application,
            MESSENGER_RequestCallback callback,
            GCancellable *cancellable,
            gpointer user_data);

/**
 * Creates a new request for the messsenger
 * application for a background permission.
 *
 * @param application Messenger application
 * @param flags Background flags
 * @param callback Callback
 * @param user_data User data
 * @return New background request object
 */
MESSENGER_Request*
request_new_background(MESSENGER_Application *application,
                       XdpBackgroundFlags flags,
                       MESSENGER_RequestCallback callback,
                       gpointer user_data);

/**
 * Creates a new request for the messsenger
 * application for a camera permission.
 *
 * @param application Messenger application
 * @param flags Camera flags
 * @param callback Callback
 * @param user_data User data
 * @return New camera request object
 */
MESSENGER_Request*
request_new_camera(MESSENGER_Application *application,
                   XdpCameraFlags flags,
                   MESSENGER_RequestCallback callback,
                   gpointer user_data);

/**
 * Creates a new request for the messsenger
 * application for a screencast permission.
 *
 * @param application Messenger application
 * @param outputs Output types
 * @param flags Screencast flags
 * @param cursor_mode Cursor mode
 * @param persist_mode Persist mode
 * @param callback Callback
 * @param user_data User data
 * @return New camera request object
 */
MESSENGER_Request*
request_new_screencast(MESSENGER_Application *application,
                       XdpOutputType outputs,
                       XdpScreencastFlags flags,
                       XdpCursorMode cursor_mode,
                       XdpPersistMode persist_mode,
                       MESSENGER_RequestCallback callback,
                       gpointer user_data);

/**
 * Cancel a request object if possible.
 *
 * @param request Request object
 */
void
request_cancel(MESSENGER_Request *request);

/**
 * Cleanup a request object and its owned
 * resources.
 *
 * @param request Request object
 */
void
request_cleanup(MESSENGER_Request *request);

/**
 * Drop a request object from the messenger
 * application list of requests.
 *
 * @param request Request object
 */
void
request_drop(MESSENGER_Request *request);

/**
 * Delete a request object and its owned 
 * resources.
 *
 * @param request Request object
 */
void
request_delete(MESSENGER_Request *request);

#endif /* REQUEST_H_ */
