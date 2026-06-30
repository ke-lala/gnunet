/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Christian Grothoff

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
 * @file test_oom.c
 * @brief tests handling of memory pool exhaustion
 * @author Christian Grothoff
 */
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include "microhttpd2.h"


/**
 * How big do we make the MHD buffer? Use a small value so we
 * can trigger OOM in a reasonable amount of time.
 */
#define BUFFER_SIZE 2048

/**
 * What is the step size. Should eventually use 1, but
 * as long as we get tons of failures, a larger step size
 * is probably nicer.
 */
#define STEP 71

/**
 * Our port.
 */
static uint16_t port;

/**
 * Set to true once we hit the out-of-memory condition.
 */
static bool out_of_memory;

/**
 * Callback used by libmicrohttpd in order to obtain content.  The
 * callback is to copy at most @a max bytes of content into @a buf or
 * provide zero-copy data for #MHD_DCC_action_continue_zc().
 *
 * @param dyn_cont_cls closure argument to the callback
 * @param ctx the context to produce the action to return,
 *            the pointer is only valid until the callback returns
 * @param pos position in the datastream to access;
 *        note that if a `struct MHD_Response` object is re-used,
 *        it is possible for the same content reader to
 *        be queried multiple times for the same data;
 *        however, if a `struct MHD_Response` is not re-used,
 *        libmicrohttpd guarantees that "pos" will be
 *        the sum of all data sizes provided by this callback
 * @param[out] buf where to copy the data
 * @param max maximum number of bytes to copy to @a buf (size of @a buf),
              if the size of the content of the response is known then size
              of the buffer is never larger than amount of the content left
 * @return action to use,
 *         NULL in case of any error (the response will be aborted)
 */
static const struct MHD_DynamicContentCreatorAction *
dyn_cc (void *dyn_cont_cls,
        struct MHD_DynamicContentCreatorContext *ctx,
        uint_fast64_t pos,
        void *buf,
        size_t max)
{
  int *flag = dyn_cont_cls;
  struct MHD_NameValueCStr footer = {
    .name = "Footer",
    .value = "Value"
  };

  if (0 == *flag)
    return MHD_DCC_action_finish_with_footer (ctx,
                                              0,
                                              &footer);
  (*flag) = 0;
  memset (buf,
          'a',
          max);
  return MHD_DCC_action_continue (ctx,
                                  max);
}


/**
 * This method is called by libmicrohttpd when response with dynamic content
 * is being destroyed.  It should be used to free resources associated
 * with the dynamic content.
 *
 * @param[in] free_cls closure
 * @ingroup response
 */
static void
dyn_cc_free (void *free_cls)
{
  free (free_cls);
}


/**
 * Function to process data uploaded by a client.
 *
 * @param upload_cls the argument given together with the function
 *                   pointer when the handler was registered with MHD
 * @param request the request is being processed
 * @param content_data_size the size of the @a content_data,
 *                          zero when all data have been processed
 * @param[in] content_data the uploaded content data,
 *                         may be modified in the callback,
 *                         valid only until return from the callback,
 *                         NULL when all data have been processed
 * @return action specifying how to proceed:
 *         #MHD_upload_action_continue() to continue upload (for incremental
 *         upload processing only),
 *         #MHD_upload_action_suspend() to stop reading the upload until
 *         the request is resumed,
 *         #MHD_upload_action_abort_request() to close the socket,
 *         or a response to discard the rest of the upload and transmit
 *         the response
 * @ingroup action
 */
static const struct MHD_UploadAction *
upload_cb (void *upload_cls,
           struct MHD_Request *request,
           size_t content_data_size,
           void *content_data)
{
  int *flag;

  (void) upload_cls;
  (void) content_data_size;
  (void) content_data;
  flag = malloc (sizeof (int));
  *flag = 1;

  return MHD_upload_action_from_response (
    request,
    MHD_response_from_callback (MHD_HTTP_STATUS_OK,
                                MHD_SIZE_UNKNOWN,
                                &dyn_cc,
                                flag,
                                &dyn_cc_free));
}


/**
 * A client has requested the given url using the given method
 * (#MHD_HTTP_METHOD_GET, #MHD_HTTP_METHOD_PUT,
 * #MHD_HTTP_METHOD_DELETE, #MHD_HTTP_METHOD_POST, etc).
 * If @a upload_size is not zero and response action is provided by this
 * callback, then upload will be discarded and the stream (the connection for
 * HTTP/1.1) will be closed after sending the response.
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param request the request object
 * @param path the requested uri (without arguments after "?")
 * @param method the HTTP method used (#MHD_HTTP_METHOD_GET,
 *        #MHD_HTTP_METHOD_PUT, etc.)
 * @param upload_size the size of the message upload content payload,
 *                    #MHD_SIZE_UNKNOWN for chunked uploads (if the
 *                    final chunk has not been processed yet)
 * @return action how to proceed, NULL
 *         if the request must be aborted due to a serious
 *         error while handling the request (implies closure
 *         of underling data stream, for HTTP/1.1 it means
 *         socket closure).
 */
static const struct MHD_Action *
server_req_cb (void *cls,
               struct MHD_Request *MHD_RESTRICT request,
               const struct MHD_String *MHD_RESTRICT path,
               enum MHD_HTTP_Method method,
               uint_fast64_t upload_size)
{
  (void) cls;
  (void) path;
  (void) method;
  (void) upload_size;
  return MHD_action_process_upload_full (request,
                                         upload_size,
                                         &upload_cb,
                                         NULL);
}


/**
 * Helper function to deal with partial writes.
 * Fails hard (calls exit() on failures)!
 *
 * @param fd where to write to
 * @param buf what to write
 * @param buf_size number of bytes in @a buf
 */
static void
write_all (int fd,
           const void *buf,
           size_t buf_size)
{
  const char *cbuf = (const char *) buf;
  size_t off;

  off = 0;
  while (off < buf_size)
  {
    ssize_t ret;

    ret = write (fd,
                 &cbuf[off],
                 buf_size - off);
    if (ret <= 0)
    {
      fprintf (stderr,
               "Writing %u bytes to %d failed: %s\n",
               (unsigned int) (buf_size - off),
               fd,
               strerror (errno));
      exit (1);
    }
    off += (size_t) ret;
  }
}


static int
run_test (unsigned int url_len,
          unsigned int query_len,
          unsigned int header_len,
          unsigned int cookie_len,
          unsigned int body_len)
{
  char filler[BUFFER_SIZE + 1];
  int s;

  out_of_memory = false;
  memset (filler,
          'a',
          BUFFER_SIZE);
  filler[BUFFER_SIZE] = '\0'; /* just to be conservative */
  s = socket (AF_INET,
              SOCK_STREAM,
              0);
  if (-1 == s)
  {
    fprintf (stderr,
             "socket() failed: %s\n",
             strerror (errno));
    return -1;
  }

  {
    struct sockaddr_in sa = {
      .sin_family = AF_INET,
      .sin_port = htons (port),
    };
    inet_pton (AF_INET,
               "127.0.0.1",
               &sa.sin_addr);
    if (0 != connect (s,
                      (struct sockaddr *) &sa,
                      sizeof (sa)))
    {
      fprintf (stderr,
               "bind() failed: %s\n",
               strerror (errno));
      close (s);
      return -1;
    }
  }

  {
    char upload[BUFFER_SIZE * 2];
    int iret;

    iret = snprintf (upload,
                     sizeof (upload),
                     "PUT /%.*s?q=%.*s HTTP/1.0\r\n"
                     "Content-Length: %u\r\n"
                     "Key: %.*s\r\n"
                     "Cookie: a=%.*s\r\n\r\n"
                     "%.*s",
                     (int) url_len,
                     filler,
                     (int) query_len,
                     filler,
                     body_len,
                     (int) header_len,
                     filler,
                     (int) cookie_len,
                     filler,
                     (int) body_len,
                     filler);
    if ( (-1 == iret) ||
         ( ((size_t) iret) > sizeof (upload)) )
    {
      fprintf (stderr,
               "failed to build request buffer: %d\n",
               iret);
      close (s);
      return -1;
    }
    write_all (s,
               upload,
               strlen (upload));
  }
  /* read and discard response */
  {
    bool got_data = false;
    bool nice = false;
    char dummy[16 * 1024];
    int flags = 0;

    while (1)
    {
      ssize_t res;

      res = recv (s,
                  &dummy,
                  sizeof (dummy),
                  flags);
      flags = MSG_DONTWAIT;
      if (res > 0)
      {
        got_data = true;
        dummy[res] = '\0';
        /* FIXME: allow other "too large" responses to also count as
           'nice' here */
        if (NULL !=
            strstr (dummy,
                    "431 Request Header Fields Too Large"))
          nice = true;
      }
      if (res <= 0)
        break;
    }
    if (nice)
      out_of_memory = true;
    if (! got_data)
    {
      out_of_memory = true;
      fprintf (stderr,
               "Response was not nice (%u/%u/%u/%u/%u)\n",
               url_len,
               query_len,
               header_len,
               cookie_len,
               body_len);
    }
  }
  close (s);
  return out_of_memory ? 1 : 0;
}


static int
test_url (void)
{
  bool oom_hit;

  oom_hit = false;
  for (unsigned int i = 0;
       i < BUFFER_SIZE;
       i += STEP)
  {
    int ret;

    ret = run_test (i, 0, 0, 0, 0);
    if (-1 == ret)
    {
      return 1;
    }
    if (1 == ret)
    {
      oom_hit = true;
    }
    if ( (oom_hit) && (1 != ret) )
    {
      fprintf (stderr,
               "Strange: OOM stopped at %u after being hit earlier (url)?\n",
               i);
    }
  }
  if (! oom_hit)
  {
    fprintf (stderr,
             "Failed to trigger OOM condition via URL\n");
    return 1;
  }
  return 0;
}


static int
test_query (void)
{
  bool oom_hit;

  oom_hit = false;
  for (unsigned int i = 0;
       i < BUFFER_SIZE;
       i += STEP)
  {
    int ret;

    ret = run_test (0, i, 0, 0, 0);
    if (-1 == ret)
    {
      return 1;
    }
    if (1 == ret)
    {
      oom_hit = true;
    }
    if ( (oom_hit) && (1 != ret) )
    {
      fprintf (stderr,
               "Strange: OOM stopped at %u after being hit earlier (query)?\n",
               i);
    }
  }
  if (! oom_hit)
  {
    fprintf (stderr,
             "Failed to trigger OOM condition via query\n");
    return 1;
  }
  return 0;
}


static int
test_header (void)
{
  bool oom_hit;

  oom_hit = false;
  for (unsigned int i = 0;
       i < BUFFER_SIZE;
       i += STEP)
  {
    int ret;

    ret = run_test (0, 0, i, 0, 0);
    if (-1 == ret)
    {
      return 1;
    }
    if (1 == ret)
    {
      oom_hit = true;
    }
    if ( (oom_hit) && (1 != ret) )
    {
      fprintf (stderr,
               "Strange: OOM stopped at %u after being hit earlier (header)?\n",
               i);
    }
  }
  if (! oom_hit)
  {
    fprintf (stderr,
             "Failed to trigger OOM condition via header\n");
    return 1;
  }
  return 0;
}


static int
test_cookie (void)
{
  bool oom_hit;

  oom_hit = false;
  for (unsigned int i = 0;
       i < BUFFER_SIZE;
       i += STEP)
  {
    int ret;

    ret = run_test (0, 0, 0, i, 0);
    if (-1 == ret)
    {
      return 1;
    }
    if (1 == ret)
    {
      oom_hit = true;
    }
    if ( (oom_hit) && (1 != ret) )
    {
      fprintf (stderr,
               "Strange: OOM stopped at %u after being hit earlier (cookie)?\n",
               i);
    }
  }
  if (! oom_hit)
  {
    fprintf (stderr,
             "Failed to trigger OOM condition via cookie\n");
    return 1;
  }
  return 0;
}


static int
test_body (void)
{
  bool oom_hit;

  oom_hit = false;
  for (unsigned int i = 0;
       i < BUFFER_SIZE;
       i += STEP)
  {
    int ret;

    ret = run_test (0, 0, 0, 0, i);
    if (-1 == ret)
    {
      return 1;
    }
    if (1 == ret)
    {
      oom_hit = true;
    }
    if ( (oom_hit) && (1 != ret) )
    {
      fprintf (stderr,
               "Strange: OOM stopped at %u after being hit earlier (body)?\n",
               i);
    }
  }
  if (! oom_hit)
  {
    fprintf (stderr,
             "Failed to trigger OOM condition via body\n");
    return 1;
  }
  return 0;
}


static int
test_mix (void)
{
  bool oom_hit;

  /* mix and match path */
  for (unsigned int i = 0;
       i < BUFFER_SIZE;
       i += STEP)
  {
    int ret;

    ret = run_test (i / 5 + 1, i / 5 + 1, i / 5 + 1, i / 5 + 1, i / 5 + 1);
    if (-1 == ret)
    {
      return 1;
    }
    if (1 == ret)
    {
      oom_hit = true;
    }
    if ( (oom_hit) && (1 != ret) )
    {
      fprintf (stderr,
               "Strange: OOM stopped at %u after being hit earlier (mix)?\n",
               i);
    }
  }
  if (! oom_hit)
  {
    fprintf (stderr,
             "Failed to trigger OOM condition in mix-and-match\n");
    return 1;
  }


  return 0;
}


static int
run_tests (void)
{
  int ret = 0;

#if 1
  ret |= test_url ();
  ret |= test_query ();
  ret |= test_header ();
  ret |= test_cookie ();
  ret |= test_body ();
  ret |= test_mix ();
#endif
  return ret;
}


static void
no_log (void *cls,
        enum MHD_StatusCode sc,
        const char *fm,
        va_list ap)
{
  (void) cls;
  (void) sc;
  (void) fm;
  (void) ap;

  /* intentionally empty */
}


int
main (void)
{
  struct MHD_Daemon *d;

  d = MHD_daemon_create (&server_req_cb,
                         NULL);
  if (MHD_SC_OK !=
      MHD_DAEMON_SET_OPTIONS (
        d,
        MHD_D_OPTION_WM_WORKER_THREADS (2),
        MHD_D_OPTION_LOG_CALLBACK (&no_log, NULL),
        MHD_D_OPTION_CONN_MEMORY_LIMIT (BUFFER_SIZE),
        MHD_D_OPTION_DEFAULT_TIMEOUT_MILSEC (1500),
        MHD_D_OPTION_BIND_PORT (MHD_AF_AUTO,
                                0)))
  {
    fprintf (stderr,
             "Failed to configure daemon!");
    return 1;
  }

  {
    enum MHD_StatusCode sc;

    sc = MHD_daemon_start (d);
    if (MHD_SC_OK != sc)
    {
#ifdef FIXME_STATUS_CODE_TO_STRING_NOT_IMPLEMENTED
      fprintf (stderr,
               "Failed to start server: %s\n",
               MHD_status_code_to_string_lazy (sc));
#else
      fprintf (stderr,
               "Failed to start server: %u\n",
               (unsigned int) sc);
#endif
      MHD_daemon_destroy (d);
      return 1;
    }
  }

  {
    union MHD_DaemonInfoFixedData info;
    enum MHD_StatusCode sc;

    sc = MHD_daemon_get_info_fixed (
      d,
      MHD_DAEMON_INFO_FIXED_BIND_PORT,
      &info);
    if (MHD_SC_OK != sc)
    {
      fprintf (stderr,
               "Failed to determine our port: %u\n",
               (unsigned int) sc);
      MHD_daemon_destroy (d);
      return 1;
    }
    port = info.v_bind_port_uint16;
  }

  {
    int result;

    result = run_tests ();
    MHD_daemon_destroy (d);
    return result;
  }
}
