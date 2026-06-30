/*
     This file is part of GNUnet.
     Copyright (C) 20xx GNUnet e.V.

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
 * @file ext/gnunet-ext.c
 * @brief ext tool
 * @author
 */
#include "gnunet_ext_config.h"
#include <stddef.h>

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <gnunet/gettext.h>
#include <gnunet/gnunet_util_lib.h>
#include "gnunet_ext_service.h"

static int ret;

/**
 * This structure holds informations about the project.
 */
static const struct GNUNET_OS_ProjectData gnunetext_pd =
  {
   .libname = "libgnunetext",
   .project_dirname = "gnunet-ext",
   .binary_name = "gnunet-ext",
   .env_varname = "GNUNET_EXT_PREFIX",
   .base_config_varname = "GNUNET_EXT_BASE_CONFIG",
   .bug_email = "gnunet-developers@gnu.org",
   .homepage = "http://www.gnu.org/s/gnunet/",
   .config_file = "gnunet-ext.conf",
   .user_config_file = "~/.config/gnunet-ext.conf",
   .version = "1.0",
   .is_gnu = 1,
   .gettext_domain = PACKAGE,
   .gettext_path = NULL,
   .agpl_url = "https://gnunet.org/git/gnunet-ext.git",
  };

/**
 * Main function that will be run by the scheduler.
 *
 * @param cls closure
 * @param args remaining command-line arguments
 * @param cfgfile name of the configuration file used (for saving, can be NULL!)
 * @param cfg configuration
 */
static void
run (void *cls,
     char *const *args,
     const char *cfgfile,
     const struct GNUNET_CONFIGURATION_Handle *cfg)
{
  ret = 0;
}


/**
 * The main function to ext.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int
main (int argc, char *const *argv)
{
  static const struct GNUNET_GETOPT_CommandLineOption options[] = {
    GNUNET_GETOPT_OPTION_END
  };

  GNUNET_OS_init(&gnunetext_pd);
  
  return (GNUNET_OK ==
          GNUNET_PROGRAM_run (argc,
                              argv,
                              "gnunet-ext [options [value]]",
                              gettext_noop
                              ("ext"),
                              options, &run, NULL)) ? ret : 1;
}

/* end of gnunet-ext.c */
