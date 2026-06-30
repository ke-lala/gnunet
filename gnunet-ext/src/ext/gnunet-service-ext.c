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
 * @file ext/gnunet-service-ext.c
 * @brief ext service implementation
 * @author Christian Grothoff
 */
#include <stddef.h>

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "gnunet_ext_config.h"
#include <gnunet/gnunet_util_lib.h>
#include "gnunet_protocols_ext.h"

/**
 * Some state we track per client.
 */
struct ClientContext
{
  /**
   * For telling service to continue processing more messages.
   */
  struct GNUNET_SERVICE_Client *c;

  /**
   * For sending messages to the client.
   */
  struct GNUNET_MQ_Handle *mq;

  /**
   * Sample state.
   */
  uint32_t state;
};


/**
 * Our configuration.
 */
static const struct GNUNET_CONFIGURATION_Handle *cfg;

/**
 * This structure holds informations about the project.
 */
static const struct GNUNET_OS_ProjectData gnunetext_pd = {
  .libname = "libgnunetext",
  .project_dirname = "gnunet-ext",
  .binary_name = "gnunet-service-ext",
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
 * Initialize the project with the data set in the
 * GNUNET_OS_ProjectData structure.  This is defined with
 * __attribute__ ((constructor)) because it has to be called before
 * the main function (implicitly defined by GNUNET_SERVICE_MAIN.)
 * Other "pre-main" initialization can be performed here too.
 */
static void __attribute__ ((constructor))
project_data_initialize (void)
{
  GNUNET_OS_init (&gnunetext_pd);
}


/**
 * Handle EXT-message.
 *
 * @param cls identification of the client
 * @param message the actual message
 */
static void
handle_ext (void *cls,
            const struct GNUNET_MessageHeader *message)
{
  struct ClientContext *cc = cls;
  struct GNUNET_MQ_Envelope *env;
  struct GNUNET_MessageHeader *response;

  /* Send same type of message back... */
  env = GNUNET_MQ_msg (response,
                       GNUNET_MESSAGE_TYPE_EXT);
  GNUNET_MQ_send (cc->mq,
                  env);

  /* Continue processing more messages from client */
  GNUNET_SERVICE_client_continue (cc->c);
}


/**
 * Task run during shutdown.
 *
 * @param cls unused
 */
static void
shutdown_task (void *cls)
{
  /* Clean up whatever #run() setup here. */
}


/**
 * Process statistics requests.
 *
 * @param cls closure
 * @param server the initialized server
 * @param c configuration to use
 */
static void
run (void *cls,
     const struct GNUNET_CONFIGURATION_Handle *c,
     struct GNUNET_SERVICE_Handle *service)
{
  cfg = c;
  GNUNET_SCHEDULER_add_shutdown (&shutdown_task,
                                 NULL);
}


/**
 * Callback called when a client connects to the service.
 *
 * @param cls closure for the service
 * @param c the new client that connected to the service
 * @param mq the message queue used to send messages to the client
 * @return @a c
 */
static void *
client_connect_cb (void *cls,
                   struct GNUNET_SERVICE_Client *c,
                   struct GNUNET_MQ_Handle *mq)
{
  struct ClientContext *cc;

  cc = GNUNET_new (struct ClientContext);
  cc->c = c;
  cc->mq = mq;
  /* setup more for new client here */
  return cc;
}


/**
 * Callback called when a client disconnected from the service
 *
 * @param cls closure for the service
 * @param c the client that disconnected
 * @param internal_cls our `struct ClientContext`
 */
static void
client_disconnect_cb (void *cls,
                      struct GNUNET_SERVICE_Client *c,
                      void *internal_cls)
{
  struct ClientContext *cc = internal_cls;

  GNUNET_assert (cc->c == c);
  /* Tear down rest of client here */
  GNUNET_free (cc);
}


/**
 * Define "main" method using service macro.
 */
GNUNET_SERVICE_MAIN
  ("ext",
  GNUNET_SERVICE_OPTION_NONE,
  &run,
  &client_connect_cb,
  &client_disconnect_cb,
  NULL,
  GNUNET_MQ_hd_fixed_size (ext,
                           GNUNET_MESSAGE_TYPE_EXT,
                           struct GNUNET_MessageHeader,
                           NULL),
  GNUNET_MQ_handler_end ());

/* end of gnunet-service-ext.c */
