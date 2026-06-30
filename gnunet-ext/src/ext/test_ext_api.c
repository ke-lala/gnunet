/*
     This file is part of GNUnet.
     Copyright (C)

     GNUnet is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     GNUnet is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with GNUnet; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
*/
/**
 * @file ext/test_ext_api.c
 * @brief testcase for ext_api.c
 */
#include "gnunet_ext_config.h"
#include <stddef.h>

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <gnunet/gnunet_util_lib.h>
#include "gnunet_ext_service.h"


/**
 * Return value from #main().  Set to 0 to mark test as passing.
 */
static int ok = 1;


/**
 * Main function of the test.
 */
static void
run (void *cls,
     char *const *args,
     const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  ok = 0;
}


/**
 * Launches the gnunet-service-ext and then the #run() function
 * with some specified arguments, allowing it to then interact
 * with the 'ext' service.
 *
 * Note that you may want to use libgnunettesting or
 * libgnunettestbed to launch a "full" peer instead of just
 * a single service.
 *
 * @param argc ignored
 * @param argv ignored
 * @return 77 if we failed to find gnunet-service-ext
 */
int
main (int argc,
      char *argv[])
{
  char *const ext_argv[] = {
    "test-ext-api",
    NULL
  };
  struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_OPTION_END
  };
  struct GNUNET_OS_Process *proc;
  char *path;

  GNUNET_log_setup ("test_ext_api",
		    "WARNING",
		    NULL);
  path = GNUNET_OS_get_libexec_binary_path ("gnunet-service-ext");
  if (NULL == path)
  {
    fprintf (stderr,
             "Failed to determine path for `%s'\n",
             "gnunet-service-ext");
    return 77;
  }
  proc = GNUNET_OS_start_process (GNUNET_NO,
                                  GNUNET_OS_INHERIT_STD_ALL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  path,
                                  "gnunet-service-ext",
                                  NULL);
  GNUNET_free (path);
  if (NULL == proc)
  {
    fprintf (stderr,
             "Service executable not found `%s'\n",
             "gnunet-service-ext");
    return 77;
  }
  GNUNET_PROGRAM_run (1,
                      ext_argv,
                      "test-ext-api",
                      "nohelp",
                      options,
                      &run,
                      &ok);
  if (0 != GNUNET_OS_process_kill (proc,
                                   SIGTERM))
  {
    GNUNET_log_strerror (GNUNET_ERROR_TYPE_WARNING,
                         "kill");
    ok = 1;
  }
  GNUNET_OS_process_wait (proc);
  GNUNET_OS_process_destroy (proc);
  return ok;
}

/* end of test_ext_api.c */
