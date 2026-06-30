/* SPDX-License-Identifier: 0BSD */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

  Permission to use, copy, modify, and/or distribute this software for
  any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
  FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
  OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/**
 * @file minimal_example2.c
 * @brief  Minimal example for libmicrohttpd v2
 * @author Karlson2k (Evgeny Grin)
 */

#include <stdio.h>
#include <stdlib.h>
#include <microhttpd2.h>

static MHD_FN_PAR_NONNULL_ (2) MHD_FN_PAR_NONNULL_ (3)
const struct MHD_Action *
req_cb (void *cls,
        struct MHD_Request *MHD_RESTRICT request,
        const struct MHD_String *MHD_RESTRICT path,
        enum MHD_HTTP_Method method,
        uint_fast64_t upload_size)
{
  static const char res_msg[] = "Hello there!\n";

  (void) cls;
  (void) path;
  (void) method;
  (void) upload_size; /* Unused */

  return MHD_action_from_response (
    request,
    MHD_response_from_buffer_static (
      MHD_HTTP_STATUS_OK,
      sizeof(res_msg) / sizeof(char) - 1,
      res_msg));
}


int
main (int argc,
      char *const *argv)
{
  struct MHD_Daemon *d;
  int port;

  if (argc != 2)
  {
    fprintf (stderr,
             "Usage:\n%s [PORT]\n",
             argv[0]);
    port = 8080;
  }
  else
    port = atoi (argv[1]);
  if ((1 > port) || (65535 < port))
  {
    fprintf (stderr,
             "The PORT must be a numeric value between 1 and 65535.\n");
    return 2;
  }
  d = MHD_daemon_create (&req_cb,
                         NULL);
  if (NULL == d)
  {
    fprintf (stderr,
             "Failed to create MHD daemon.\n");
    return 3;
  }
  if (MHD_SC_OK !=
      MHD_DAEMON_SET_OPTIONS (
        d,
        MHD_D_OPTION_WM_WORKER_THREADS (1),
        MHD_D_OPTION_BIND_PORT (MHD_AF_AUTO,
                                (uint_least16_t) port)))
  {
    fprintf (stderr,
             "Failed to set MHD daemon run parameters.\n");
  }
  else
  {
    if (MHD_SC_OK !=
        MHD_daemon_start (d))
    {
      fprintf (stderr,
               "Failed to start MHD daemon.\n");
    }
    else
    {
      printf ("The MHD daemon is listening on port %d\n"
              "Press ENTER to stop.\n", port);
      (void) fgetc (stdin);
    }
  }
  printf ("Stopping... ");
  fflush (stdout);
  MHD_daemon_destroy (d);
  printf ("OK\n");
  return 0;
}
