/* examples/init-example.c */

#include <microhttpd2.h>
#include <assert.h>

static struct MHD_Action *
handle_request (void *cls,
                struct MHD_Request *request,
                const struct MHD_String *path,
                enum MHD_HTTP_Method method,
                uint_fast64_t upload_size)
{
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

  /* Create an HTTP server and use "handle_request()" to
     handle all requests. */
  d = MHD_daemon_create (&handle_request,
                         NULL);
  /* We run with everything on default, so port 80, no TLS */
  MHD_daemon_start (d);
  /* Wait for input on stdin */
  (void) getchar ();
  /* Then just shut everything down */
  MHD_daemon_stop (d);
  return 0;
}
