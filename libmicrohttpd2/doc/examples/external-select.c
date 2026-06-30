/* examples/external-select.c */

/* Must be defined before including "microhttpd2.h" */
#define MHD_APP_SOCKET_CNTX_TYPE struct AppSockContext
#include <microhttpd2.h>
#include <assert.h>
#include <sys/select.h>


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
static int max_fd;

/**
 * Head of our internal list of sockets to select() on.
 */
static struct AppSockContext *head;


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
  assert (NULL == cls);
  /* Note: This code only works on UNIX where MHD_Socket is an "int". */
  if (fd >= FD_SETSIZE)
    return NULL; /* not allowed by select() */
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
    app_cntx = malloc (sizeof (MHD_APP_SOCKET_CNTX_TYPE));
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
  /* Since we are level-triggered and thus called by MHD in every
     iteration, we simply build the event sets for select()
     here directly. */
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
 * We "handle" request by just closing the connection.
 */
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
  if (MHD_SC_OK !=
      MHD_daemon_set_options (daemon,
                              MHD_D_OPTION_REREGISTER_ALL (MHD_YES),
                              MHD_D_OPTION_WM_EXTERNAL_EVENT_LOOP_LEVEL (
                                &sock_reg_update_cb,
                                NULL)))
  {
    /* Maybe external event loop type not supported by build? */
    MHD_daemon_destroy (d);
    return 1;
  }
  /* We run with everything on default, so port 80, no TLS */
  MHD_daemon_start (d);
  while (1)
  {
    struct timeval ts;
    struct AppSockContext *pos;
    uint_fast64_t next_wait;

    FD_ZERO (&rs);
    FD_ZERO (&ws);
    FD_ZERO (&es);
    FD_SET (STDIN_FILENO,
            &rs);
    max_fd = STDIN_FILENO;
    /* This will cause MHD to call the #sock_reg_update_cb() */
    MHD_daemon_process_reg_events (daemon,
                                   &next_wait);
    ts.tv_sec = (time_t) (next_wait / 1000u);
    ts.tv_usec = (suseconds_t) ((next_wait % 1000u) * 1000u);
    /* Real applications may do nicer error handling here */
    (void) select (max_fd + 1,
                   &rs,
                   &ws,
                   &es,
                   &ts);
    if (FD_ISSET (STDIN_FILENO,
                  &rs))
      break; /* exit on input on stdin */

    /* Now we need to tell MHD which events were triggered */
    for (pos = head; NULL != pos; pos = pos->next)
    {
      enum MHD_FdState current_state = MHD_FD_STATE_NONE;

      if (FD_ISSET (pos->fd,
                    &rs))
        current_state |= MHD_FD_STATE_READ;
      if (FD_ISSET (pos->fd,
                    &ws))
        current_state |= MHD_FD_STATE_SEND;
      if (FD_ISSET (pos->fd,
                    &es))
        current_state |= MHD_FD_STATE_EXCEPT;
      if (MHD_FD_STATE_NONE == current_state)
        continue; /* not triggered */
      MHD_daemon_event_update (daemon,
                               pos->ecb_cntx,
                               current_state);
    }
  }
  /* Finally, just shut everything down */
  MHD_daemon_stop (d);
  return 0;
}
