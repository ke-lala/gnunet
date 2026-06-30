/* examples/basic-authentication.c */

#include <microhttpd2.h>
#include <assert.h>


static struct MHD_Action *
handle_request (void *cls,
                struct MHD_Request *request,
                const struct MHD_String *path,
                enum MHD_HTTP_Method method,
                uint_fast64_t upload_size)
{
  union MHD_RequestInfoDynamicData dd;

  if (MHD_SC_OK !=
      MHD_request_get_info_dynamic (request,
                                    MHD_REQUEST_INFO_DYNAMIC_AUTH_BASIC_CREDS,
                                    &dd))
  {
    struct MHD_Response *r;

    r = MHD_response_from_empty (MHD_HTTP_STATUS_UNAUTHORIZED);
    if (MHD_SC_OK !=
        MHD_response_add_auth_basic_challenge (r,
                                               "test",
                                               MHD_YES))
    {
      MHD_response_destroy (r);
      return MHD_action_abort_request (request);
    }
    return MHD_action_from_response (request,
                                     r);
  }
  fprintf (stderr,
           "User `%s' has password `%s' (do not log this in real code)\n",
           dd.v_auth_basic_creds.username.cstr,
           dd.v_auth_basic_creds.password.cstr);
  /* You may want to check that username/password are
     actually OK here, and if not fail similar to
     the MHD_HTTP_STATUS_UNAUTHORIZED returned above. */

  /* Success! */
  return MHD_action_from_response (
    request,
    MHD_response_from_empty (MHD_HTTP_NO_CONTENT));
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
