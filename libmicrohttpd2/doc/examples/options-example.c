/* examples/options-example.c */

#include <microhttpd2.h>
#include <assert.h>


static struct MHD_Action *
handle_request (void *cls,
                struct MHD_Request *request,
                const struct MHD_String *path,
                enum MHD_HTTP_Method method,
                uint_fast64_t upload_size)
{
  struct MHD_String *host;

  /* We passed NULL in main() for the closure */
  assert (NULL == cls);
  /* This simply closes the connection after receiving
     the HTTP header, never return actually returning
     any data. */
  return MHD_action_abort_request (request);
}


int
main ()
{
  struct MHD_Daemon *d;
  static const struct MHD_DaemonOptionAndValue bind_opt
    = MHD_D_OPTION_BIND_PORT (MHD_AF_AUTO,
                              80);

  /* Create an HTTP server and use "handle_request()" to
     handle all requests. */
  d = MHD_daemon_create (&handle_request,
                         NULL);
  if (MHD_SC_OK !=
      MHD_DAEMON_SET_OPTIONS (d,
                              MHD_D_OPTION_LISTEN_BACKLOG (5),
                              MHD_D_OPTION_SUPPRESS_DATE_HEADER ()))
    fprintf (stderr,
             "That's OK, we don't care too much about these\n");
  if (MHD_SC_OK !=
      MHD_daemon_set_option (d,
                             &bind_opt))
  {
    fprintf (stderr,
             "Failed to set port to bind to!\n");
    MHD_daemon_stop (d);
    return 1;
  }
  MHD_daemon_start (d);
  /* Wait for input on stdin */
  (void) getchar ();
  /* Then just shut everything down */
  MHD_daemon_stop (d);
  return 0;
}
