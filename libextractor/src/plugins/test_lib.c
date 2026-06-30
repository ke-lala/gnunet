/*
     This file is part of libextractor.
     Copyright (C) 2012 Vidyut Samanta and Christian Grothoff

     libextractor is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     libextractor is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libextractor; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
*/
/**
 * @file plugins/test_lib.c
 * @brief helper library for writing testcases
 * @author Christian Grothoff
 */
#include "platform.h"
#include "test_lib.h"
#include <sys/types.h>
#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * Function that libextractor calls for each
 * meta data item found.
 *
 * @param cls closure the 'struct SolutionData' we are currently working on
 * @param plugin_name should be "test"
 * @param type should be "COMMENT"
 * @param format should be "UTF8"
 * @param data_mime_type should be "<no mime>"
 * @param data hello world or good bye
 * @param data_len number of bytes in data
 * @return 0 (always)
 */
static int
process_replies (void *cls,
                 const char *plugin_name,
                 enum EXTRACTOR_MetaType type,
                 enum EXTRACTOR_MetaFormat format,
                 const char *data_mime_type,
                 const char *data,
                 size_t data_len)
{
  struct SolutionData *sd = cls;
  unsigned int i;

  for (i = 0; -1 != sd[i].solved; i++)
  {
    if ( (0 != sd[i].solved) ||
         (sd[i].type != type) ||
         (sd[i].format != format) )
      continue;
    if ( (sd[i].regex) &&
         (EXTRACTOR_METAFORMAT_BINARY != format) )
    {
      regex_t re;
      regmatch_t match;

      if (0 !=
          regcomp (&re,
                   sd[i].data,
                   REG_EXTENDED))
      {
        fprintf (stderr,
                 "Not a valid regex: %s\n",
                 sd[i].data);
        abort ();
      }
      if ( ('\0' != data[data_len - 1]) ||
           (0 != regexec (&re,
                          data,
                          1,
                          &match,
                          0)) )
      {
        regfree (&re);
        continue;
      }
      regfree (&re);
    }
    else
    {
      if ( (EXTRACTOR_METAFORMAT_BINARY != format) &&
           ( (sd[i].data_len != data_len) ||
             (0 != memcmp (sd[i].data, data, data_len)) ) )
        continue;
      if ( (EXTRACTOR_METAFORMAT_BINARY == format) &&
           ( (sd[i].data_len > data_len) ||
             (0 != memcmp (sd[i].data, data, sd[i].data_len)) ) )
        continue;
    }

    if (NULL != sd[i].data_mime_type)
    {
      if (NULL == data_mime_type)
        continue;
      if (0 != strcmp (sd[i].data_mime_type, data_mime_type))
        continue;
    }
    else
    {
      if (NULL != data_mime_type)
        continue;
    }
    sd[i].solved = 1;
    return 0;
  }
  fprintf (stderr,
           "Got additional meta data of type %d and format %d with value `%.*s' from plugin `%s'\n",
           type,
           format,
           (int) data_len,
           data,
           plugin_name);
  return 0;
}


/**
 * Run a test for the given plugin, problem set and options.
 *
 * @param plugin_name name of the plugin to load
 * @param ps array of problems the plugin should solve;
 *        NULL in filename terminates the array.
 * @param opt options to use for loading the plugin
 * @return 0 on success, 1 on failure
 */
static int
run (const char *plugin_name,
     struct ProblemSet *ps,
     enum EXTRACTOR_Options opt)
{
  struct EXTRACTOR_PluginList *pl;
  unsigned int i;
  unsigned int j;
  int ret;
  bool skipped = false;

  pl = EXTRACTOR_plugin_add_config (NULL,
                                    plugin_name,
                                    opt);
  for (i = 0; NULL != ps[i].filename; i++)
  {
    if (0 != access (ps[i].filename,
                     R_OK))
    {
      fprintf (stderr,
               "Failed to access %s, skipping test\n",
               ps[i].filename);
      skipped = true;
      continue;
    }
    EXTRACTOR_extract (pl,
                       ps[i].filename,
                       NULL, 0,
                       &process_replies,
                       ps[i].solution);
  }
  EXTRACTOR_plugin_remove_all (pl);
  if (skipped)
    return 0;
  ret = 0;
  for (i = 0; NULL != ps[i].filename; i++)
    for (j = 0; -1 != ps[i].solution[j].solved; j++)
      if (0 == ps[i].solution[j].solved)
      {
        ret = 1;
        fprintf (stderr,
                 "Did not get expected meta data of type %d and format %d with value `%.*s' from plugin `%s'\n",
                 ps[i].solution[j].type,
                 ps[i].solution[j].format,
                 (int) ps[i].solution[j].data_len,
                 ps[i].solution[j].data,
                 plugin_name);
      }
      else
        ps[i].solution[j].solved = 0;
  /* reset for next round */
  return ret;
}


#ifndef WINDOWS
/**
 * Install a signal handler to ignore SIGPIPE.
 */
static void
ignore_sigpipe ()
{
  struct sigaction oldsig;
  struct sigaction sig;

  memset (&sig, 0, sizeof (struct sigaction));
  sig.sa_handler = SIG_IGN;
  sigemptyset (&sig.sa_mask);
#ifdef SA_INTERRUPT
  sig.sa_flags = SA_INTERRUPT;  /* SunOS */
#else
  sig.sa_flags = SA_RESTART;
#endif
  if (0 != sigaction (SIGPIPE, &sig, &oldsig))
    fprintf (stderr,
             "Failed to install SIGPIPE handler: %s\n", strerror (errno));
}


#endif


/**
 * Main function to be called to test a plugin.
 *
 * @param plugin_name name of the plugin to load
 * @param ps array of problems the plugin should solve;
 *        NULL in filename terminates the array.
 * @return 0 on success, 1 on failure
 */
int
ET_main (const char *plugin_name,
         struct ProblemSet *ps)
{
  int ret;

  /* change environment to find plugins which may not yet be
     not installed but should be in the current directory (or .libs)
     on 'make check' */
#ifndef WINDOWS
  ignore_sigpipe ();
#endif
  if (0 != putenv ("LIBEXTRACTOR_PREFIX=./.libs/"))
    fprintf (stderr,
             "Failed to update my environment, plugin loading may fail: %s\n",
             strerror (errno));
  ret = run (plugin_name, ps, EXTRACTOR_OPTION_DEFAULT_POLICY);
  if (0 != ret)
    return ret;
  ret = run (plugin_name, ps, EXTRACTOR_OPTION_IN_PROCESS);
  if (0 != ret)
    return ret;
  return 0;
}


/* end of test_lib.c */
