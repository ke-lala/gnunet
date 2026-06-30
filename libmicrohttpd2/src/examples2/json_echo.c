/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff, Evgeny Grin (and other
  contributing authors)

  GNU libmicrohttpd is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  GNU libmicrohttpd is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  Alternatively, you can redistribute GNU libmicrohttpd and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License, or (at your option) any later version, together
  with the eCos exception, as follows:

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, or you compile this
    file and link it with other works to produce a work based on this
    file, this file does not by itself cause the resulting work to be
    covered by the GNU General Public License. However the source code
    for this file must still be made available in accordance with
    section (3) of the GNU General Public License v2.

    This exception does not invalidate any other reasons why a work
    based on this file might be covered by the GNU General Public
    License.

  You should have received copies of the GNU Lesser General Public
  License and the GNU General Public License along with this library;
  if not, see <https://www.gnu.org/licenses/>.
*/
/**
 * @file json_echo.c
 * @brief example for processing POST requests with JSON uploads, echos the JSON back to the client
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#if ! defined(_WIN32) || defined (__CYGWIN__)
#  include <sys/select.h>
#  include <unistd.h>
#else
#  include <conio.h>
#endif
#include <assert.h>
struct AppSockContext; /* Forward declaration */
#define MHD_APP_SOCKET_CNTX_TYPE struct AppSockContext
#include <microhttpd2.h>
#include <jansson.h>

/**
 * Bad request page.
 */
#define BAD_REQUEST_PAGE \
        "<html><head><title>Illegal request</title></head><body>Go away.</body></html>"

/**
 * Invalid JSON page.
 */
#define FILE_NOT_FOUND_PAGE \
        "<html><head><title>Not found</title></head><body>Go away.</body></html>"

/**
 * We keep the sockets we are waiting on in a DLL.
 */
struct AppSockContext
{
  struct AppSockContext *next;
  struct AppSockContext *prev;
  struct MHD_EventUpdateContext *ecb_cntx;
  MHD_Socket fd;
};


/**
 * Current read set.
 */
static fd_set rs;

/**
 * Current write set.
 */
static fd_set ws;

/**
 * Current error set.
 */
static fd_set es;

/**
 * Maximum FD in any set.
 */
static MHD_Socket max_fd = 0;

/**
 * Head of our internal list of sockets to select() on.
 */
static struct AppSockContext *head;

/**
 * Generates 404.
 */
static struct MHD_Response *file_not_found_response;

/**
 * Generates 400.
 */
static struct MHD_Response *bad_request_response;


static const struct MHD_UploadAction *
handle_upload (void *upload_cls,
               struct MHD_Request *request,
               size_t content_data_size,
               void *content_data)
{
  json_t *j;
  json_error_t err;
  char *s;
  struct MHD_Response *response;
  (void) upload_cls; /* Unused. Mute compiler warning. */

  j = json_loadb ((char *) content_data,
                  content_data_size,
                  0,
                  &err);
  if (NULL == j)
    return MHD_upload_action_from_response (request,
                                            bad_request_response);
  s = json_dumps (j,
                  JSON_INDENT (2));
  json_decref (j);
  response = MHD_response_from_buffer (MHD_HTTP_STATUS_OK,
                                       strlen (s),
                                       s,
                                       &free,
                                       s);
  return MHD_upload_action_from_response (request,
                                          response);
}


static const struct MHD_Action *
handle_request (void *cls,
                struct MHD_Request *request,
                const struct MHD_String *path,
                enum MHD_HTTP_Method method,
                uint_fast64_t upload_size)
{
  (void) cls;  /* Unused. Mute compiler warning. */
  (void) path; /* Unused. Mute compiler warning. */

  if (method != MHD_HTTP_METHOD_POST)
    return MHD_action_from_response (request,
                                     file_not_found_response);
  if (upload_size > 16 * 1024 * 1024)
    return MHD_action_abort_request (request);
  return MHD_action_process_upload_full (request,
                                         upload_size,
                                         &handle_upload,
                                         NULL);
}


/* This is the function MHD will call when the external event
   loop needs to change how it watches out for changes to
   some socket's state */
static MHD_APP_SOCKET_CNTX_TYPE *
sock_reg_update_cb (
  void *cls,
  MHD_Socket fd,
  enum MHD_FdState watch_for,
  MHD_APP_SOCKET_CNTX_TYPE *app_cntx,
  struct MHD_EventUpdateContext *ecb_cntx)
{
  (void) cls; /* Unused. Mute compiler warning. */
#ifdef MHD_SOCKETS_KIND_POSIX
  /* The value is limited by MHD_D_OPTION_FD_NUMBER_LIMIT() */
  assert (fd < FD_SETSIZE);
#endif /* MHD_SOCKETS_KIND_POSIX */
  if (MHD_FD_STATE_NONE == watch_for)
  {
    /* Remove from DLL */
    if (app_cntx == head)
      head = app_cntx->next;
    if (NULL != app_cntx->prev)
      app_cntx->prev->next = app_cntx->next;
    if (NULL != app_cntx->next)
      app_cntx->next->prev = app_cntx->prev;
    free (app_cntx);
    return NULL;
  }
  if (NULL == app_cntx)
  {
    /* First time, allocate data structure to keep
       the socket and MHD's context */
    app_cntx =
      (MHD_APP_SOCKET_CNTX_TYPE *) malloc (sizeof (MHD_APP_SOCKET_CNTX_TYPE));
    if (NULL == app_cntx)
      return NULL; /* closes connection */
    /* prepend to DLL */
    app_cntx->prev = NULL;
    app_cntx->next = head;
    if (NULL != head)
      head->prev = app_cntx;
    head = app_cntx;
    app_cntx->fd = fd;
  }
  else
  {
    /* socket must not change */
    assert (fd == app_cntx->fd);
  }
  /* MHD could change its associated context, so always update */
  app_cntx->ecb_cntx = ecb_cntx;
  /* Since we are called by MHD in every iteration, we simply build
     the event sets for select() here directly. */
  if (watch_for & MHD_FD_STATE_RECV)
    FD_SET (fd,
            &rs);
  if (watch_for & MHD_FD_STATE_SEND)
    FD_SET (fd,
            &ws);
  if (watch_for & MHD_FD_STATE_EXCEPT)
    FD_SET (fd,
            &es);
  if (fd > max_fd)
    max_fd = fd;
  return app_cntx;
}


/**
 * Mark the given response as HTML for the browser.
 *
 * @param response response to mark
 */
static void
mark_as_html (struct MHD_Response *response)
{
  if (NULL == response)
    return;
  (void) MHD_response_add_header (response,
                                  MHD_HTTP_HEADER_CONTENT_TYPE,
                                  "text/html");
}


/**
 * Call with the port number as the only argument.
 * Terminates when reading from stdin or on signals, such as CTRL-C.
 */
int
main (int argc,
      char *const *argv)
{
  struct MHD_Daemon *d;
  unsigned int port;
  char dummy;

  if ( (argc != 2) ||
       (1 != sscanf (argv[1],
                     "%u%c",
                     &port,
                     &dummy)) ||
       (UINT16_MAX < port) )
  {
    if (2 == argc)
    {
      fprintf (stderr,
               "Usage: %s PORT\n",
               argv[0]);
      return 1;
    }
    port = 8080;
  }
  file_not_found_response =
    MHD_response_from_buffer_static (
      MHD_HTTP_STATUS_NOT_FOUND,
      strlen (FILE_NOT_FOUND_PAGE),
      FILE_NOT_FOUND_PAGE);
  mark_as_html (file_not_found_response);
  if (MHD_SC_OK !=
      MHD_response_set_option (file_not_found_response,
                               &MHD_R_OPTION_REUSABLE (MHD_YES)))
    return 1;
  bad_request_response =
    MHD_response_from_buffer_static (
      MHD_HTTP_STATUS_BAD_REQUEST,
      strlen (BAD_REQUEST_PAGE),
      BAD_REQUEST_PAGE);
  mark_as_html (bad_request_response);
  if (MHD_SC_OK !=
      MHD_response_set_option (bad_request_response,
                               &MHD_R_OPTION_REUSABLE (MHD_YES)))
    return 1;

  d = MHD_daemon_create (&handle_request,
                         NULL);
  if (NULL == d)
    return 1;
  if (MHD_SC_OK !=
      MHD_DAEMON_SET_OPTIONS (
        d,
        MHD_D_OPTION_REREGISTER_ALL (MHD_YES),
        MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_CB_LEVEL (
          &sock_reg_update_cb,
          NULL),
        MHD_D_OPTION_FD_NUMBER_LIMIT (FD_SETSIZE),
        MHD_D_OPTION_DEFAULT_TIMEOUT_MILSEC (120000u),
        MHD_D_OPTION_CONN_MEMORY_LIMIT (256 * 1024),
        MHD_D_OPTION_BIND_PORT (MHD_AF_AUTO,
                                (uint_least16_t) port)))
    return 1;
  if (MHD_SC_OK !=
      MHD_daemon_start (d))
  {
    MHD_daemon_destroy (d);
    return 1;
  }
  while (1)
  {
    struct timeval ts;
    struct AppSockContext *pos;
    uint_fast64_t next_wait;

    FD_ZERO (&rs);
    FD_ZERO (&ws);
    FD_ZERO (&es);
#ifdef MHD_SOCKETS_KIND_POSIX
    FD_SET (STDIN_FILENO,
            &rs);
    max_fd = STDIN_FILENO;
#endif /* MHD_SOCKETS_KIND_POSIX */

    /* This will cause MHD to call the #sock_reg_update_cb() */
    MHD_daemon_process_reg_events (d,
                                   &next_wait);
#ifdef MHD_SOCKETS_KIND_POSIX
    ts.tv_sec = (time_t) (next_wait / 1000000);
#else  /* W32 */
    /* W32 cannot monitor "stdin" with select().
       Use poor man replacement. */
    if (300000u < next_wait)
      next_wait = 300000u;
    ts.tv_sec = (long) (next_wait / 1000000);
#endif /* W32 */
    ts.tv_usec = (long) (next_wait % 1000000);
    /* Real applications may do nicer error handling here */
    (void) select ((int) max_fd + 1,
                   &rs,
                   &ws,
                   &es,
                   &ts);
#ifdef MHD_SOCKETS_KIND_POSIX
    if (FD_ISSET (STDIN_FILENO,
                  &rs))
      break; /* exit on input on stdin */
#else  /* W32 */
    if (0 != _kbhit ())
      break; /* exit on console input */
#endif /* W32 */

    /* Now we need to tell MHD which events were triggered */
    for (pos = head; NULL != pos; pos = pos->next)
    {
      enum MHD_FdState current_state = MHD_FD_STATE_NONE;

      if (FD_ISSET (pos->fd,
                    &rs))
        current_state =
          (enum MHD_FdState) (current_state | MHD_FD_STATE_RECV);
      if (FD_ISSET (pos->fd,
                    &ws))
        current_state =
          (enum MHD_FdState) (current_state | MHD_FD_STATE_SEND);
      if (FD_ISSET (pos->fd,
                    &es))
        current_state =
          (enum MHD_FdState) (current_state | MHD_FD_STATE_EXCEPT);
      MHD_daemon_event_update (d,
                               pos->ecb_cntx,
                               current_state);
    }
  }
  MHD_daemon_destroy (d);
  MHD_response_destroy (file_not_found_response);
  MHD_response_destroy (bad_request_response);
  return 0;
}
