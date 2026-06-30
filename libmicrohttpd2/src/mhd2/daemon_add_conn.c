/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2014-2026 Evgeny Grin (Karlson2k)
  Copyright (C) 2007-2018 Daniel Pittman and Christian Grothoff

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
 * @file src/mhd2/daemon_add_conn.c
 * @brief  The implementations of MHD functions for adding new connections
 * @author Karlson2k (Evgeny Grin)
 * @author Daniel Pittman
 * @author Christian Grothoff
 *
 * @warning Imported from MHD1 with minimal changes
 * TODO:
 * + Rewrite,
 * + add per IP limit,
 * + add app policy for new conn,
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"
#include "mhd_assume.h"

#include "sys_offsetof.h"

#include "mhd_locks.h"
#include "mhd_atomic_counter.h"

#include "sys_sockets_types.h"
#include "sys_sockets_headers.h"
#include "sys_ip_headers.h"

#ifdef MHD_USE_TRACE_CONN_ADD_CLOSE
#  include <stdio.h>
#endif /* MHD_USE_TRACE_CONN_ADD_CLOSE */
#include <string.h>
#ifdef MHD_SUPPORT_EPOLL
#  include <sys/epoll.h>
#endif

#include "compat_calloc.h"

#include "mhd_sockets_macros.h"
#include "mhd_sockets_funcs.h"

#include "mhd_panic.h"
#include "mhd_dbg_print.h"

#include "mhd_daemon.h"
#include "mhd_connection.h"

#include "daemon_logger.h"
#include "mhd_mono_clock.h"
#include "mempool_funcs.h"
#include "events_process.h"

#include "daemon_funcs.h"
#include "response_from.h"
#include "response_destroy.h"
#include "conn_timeout.h"
#include "conn_mark_ready.h"

#ifdef MHD_SUPPORT_HTTP2
#  include "h2/h2_comm.h"
#endif /* MHD_SUPPORT_HTTP2 */
#ifdef MHD_SUPPORT_HTTPS
#  include "mhd_tls_funcs.h"
#endif

#include "mhd_public_api.h"

#include "daemon_add_conn.h"

#ifdef MHD_SUPPORT_HTTP2
static void
connection_set_http_layer_init_state (struct MHD_Connection *restrict c)
{
  c->h_layer.state = mhd_HTTP_LAYER_PREFACE;
  c->h_layer.fam = mhd_HTTP_VER_FAM_NOT_SET;
}


#else  /* ! MHD_SUPPORT_HTTP2 */
#  define connection_set_http_layer_init_state(c)       ((void) 0)
#endif /* ! MHD_SUPPORT_HTTP2 */

/**
 * Set initial internal states for the connection to start reading and
 * processing incoming data.
 * This sets:
 * + data processing stage
 * + stream request and reply initial data
 * + connection read and write buffers
 *
 * @param c the connection to process
 */
static void
connection_set_initial_state (struct MHD_Connection *restrict c)
{
  size_t read_buf_size;

  mhd_assert (mhd_HTTP_STAGE_INIT == c->stage);

  c->conn_reuse = mhd_CONN_KEEPALIVE_POSSIBLE;
  c->event_loop_info = MHD_EVENT_LOOP_INFO_RECV;

  // TODO: move request reset to special function
  memset (&(c->rq), 0, sizeof(c->rq));
  // TODO: move reply reset to special function
  memset (&(c->rp), 0, sizeof(c->rp));

#ifndef HAVE_NULL_PTR_ALL_ZEROS
  // TODO: move request reset to special function
  mhd_DLINKEDL_INIT_LIST (&(c->rq), fields);
#ifdef MHD_SUPPORT_POST_PARSER
  mhd_DLINKEDL_INIT_LIST (&(c->rq), post_fields);
#endif /* MHD_SUPPORT_POST_PARSER */
  c->rq.version = NULL;
  c->rq.url = NULL;
  c->rq.field_lines.start = NULL;
  c->rq.app_context = NULL;
  c->rq.hdrs.rq_line.rq_tgt = NULL;
  c->rq.hdrs.rq_line.rq_tgt_qmark = NULL;

  // TODO: move reply reset to special function
  c->rp.app_act_ctx.connection = NULL;
  c->rp.response = NULL;
  c->rp.resp_iov.iov = NULL;
#endif /* ! HAVE_NULL_PTR_ALL_ZEROS */

  c->write_buffer = NULL;
  c->write_buffer_size = 0;
  c->write_buffer_send_offset = 0;
  c->write_buffer_append_offset = 0;

  c->continue_message_write_offset = 0;

  c->read_buffer_offset = 0;
  read_buf_size = c->daemon->conns.cfg.mem_pool_size / 2;
  c->read_buffer
    = (char *) mhd_pool_allocate (c->pool,
                                  read_buf_size,
                                  false);
  c->read_buffer_size = read_buf_size;
}


static void
notify_app_conn (struct MHD_Daemon *restrict daemon,
                 struct MHD_Connection *restrict connection,
                 bool closed)
{
  (void) daemon, (void) connection, (void) closed;
  // TODO: implement
}


/**
 * Do basic preparation work on the new incoming connection.
 *
 * This function do all preparation that is possible outside main daemon
 * thread.
 * @remark Could be called from any thread.
 *
 * @param daemon daemon that manages the connection
 * @param client_socket socket to manage (MHD will expect
 *        to receive an HTTP request from this socket next).
 * @param addrlen number of bytes in @a addr
 * @param addr IP address of the client,
 *                will be deallocated by free() if @a external_add is 'true'
 * @param external_add indicate that socket has been added externally
 * @param non_blck indicate that socket in non-blocking mode
 * @param sk_spipe_supprs indicate that the @a client_socket has
 *                         set SIGPIPE suppression
 * @param sk_is_nonip _MHD_YES if this is not a TCP/IP socket
 * @param[out] conn_out the pointer to variable to be set to the address
 *                      of newly allocated connection structure
 * @return #MHD_SC_OK on success,
 *         error on failure (the @a client_socket is closed)
 */
static MHD_FN_MUST_CHECK_RESULT_
MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_INOUT_SIZE_ (4,3)
MHD_FN_PAR_NONNULL_ (9) MHD_FN_PAR_OUT_ (9) enum MHD_StatusCode
new_connection_prepare_ (struct MHD_Daemon *restrict daemon,
                         MHD_Socket client_socket,
                         size_t addrlen,
                         struct sockaddr_storage *restrict addr,
                         bool external_add,
                         bool non_blck,
                         bool sk_spipe_supprs,
                         enum mhd_Tristate sk_is_nonip,
                         struct MHD_Connection **restrict conn_out)
{
  struct MHD_Connection *c;
  enum MHD_StatusCode ret;
  size_t tls_data_size;

  mhd_assert ((0 == addrlen) || (NULL != addr));

  ret = MHD_SC_OK;
  *conn_out = NULL;

  tls_data_size = 0;
#ifdef MHD_SUPPORT_HTTPS
  if (mhd_D_HAS_TLS (daemon))
    tls_data_size = mhd_tls_conn_get_tls_size (daemon->tls);
#endif

  c = (struct MHD_Connection *)
      mhd_calloc (1,
                  sizeof (struct MHD_Connection) + tls_data_size);
  if (NULL == c)
  {
    mhd_LOG_MSG (daemon, \
                 MHD_SC_CONNECTION_MEM_ALLOC_FAILURE, \
                 "Failed to allocate memory for the new connection");
    ret = MHD_SC_CONNECTION_MEM_ALLOC_FAILURE;
  }
  else
  {
#ifndef HAVE_NULL_PTR_ALL_ZEROS
    mhd_DLINKEDL_INIT_LINKS (c, all_conn);
    c->extr_event.app_cntx = NULL;
    mhd_DLINKEDL_INIT_LINKS (c, proc_ready);
    mhd_DLINKEDL_INIT_LINKS (c, by_timeout);
#  ifdef MHD_SUPPORT_UPGRADE
    c->upgr.c = NULL;
    mhd_DLINKEDL_INIT_LINKS (c, upgr_cleanup);
#  endif /* MHD_SUPPORT_UPGRADE */
    c->socket_context = NULL;
#endif /* ! HAVE_NULL_PTR_ALL_ZEROS */
#ifdef MHD_SUPPORT_HTTPS
    if (0 != tls_data_size)
      c->tls = (struct mhd_TlsConnData *) (c + 1);
#  ifndef HAVE_NULL_PTR_ALL_ZEROS
    else
      c->tls = NULL;
#  endif
#endif

#ifdef MHD_SUPPORT_HTTP2
    mhd_h2_blank_init (c);
#endif /* MHD_SUPPORT_HTTP2 */

    if (! external_add)
    {
      c->sk.state.corked = mhd_T_NO;
      c->sk.state.nodelay = mhd_T_NO;
    }
    else
    {
      c->sk.state.corked = mhd_T_MAYBE;
      c->sk.state.nodelay = mhd_T_MAYBE;
    }

    if ((0 < addrlen))
    {
      if (! external_add)
      {
        c->sk.addr.data = (struct sockaddr_storage *) malloc (addrlen);
        if (NULL == c->sk.addr.data)
        {
          mhd_LOG_MSG (daemon, \
                       MHD_SC_CONNECTION_MEM_ALLOC_FAILURE, \
                       "Failed to allocate memory for the new connection");
          ret = MHD_SC_CONNECTION_MEM_ALLOC_FAILURE;
        }
        else
        {
          memcpy (c->sk.addr.data,
                  addr,
                  addrlen);
          c->sk.addr.size = addrlen;
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN
          c->sk.addr.data->ss_len = addrlen;
#endif /* HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN */
        }
      }
      else
      {
        c->sk.addr.data = addr;
        c->sk.addr.size = addrlen;
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN
        c->sk.addr.data->ss_len = (uint8_t) addrlen;
#endif /* HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN */
        addr = NULL;
      }
    }
    else
    {
      c->sk.addr.data = NULL;
      c->sk.addr.size = 0u;
    }

    if (MHD_SC_OK == ret)
    {
      c->sk.fd = client_socket;
      c->sk.props.is_nonblck = non_blck;
      c->sk.props.is_nonip = sk_is_nonip;
      c->sk.props.has_spipe_supp = sk_spipe_supprs;
#ifdef MHD_SUPPORT_THREADS
      mhd_thread_handle_ID_set_invalid (&c->tid);
#endif /* MHD_SUPPORT_THREADS */
      c->daemon = daemon;
      c->event_loop_info = MHD_EVENT_LOOP_INFO_RECV;

#ifdef MHD_SUPPORT_HTTPS
      if (0 != tls_data_size)
      {
        if (! mhd_tls_conn_init (daemon->tls,
                                 &(c->sk),
                                 c->tls))
        {
          mhd_LOG_MSG (daemon, \
                       MHD_SC_TLS_CONNECTION_INIT_FAILED, \
                       "Failed to initialise TLS context for " \
                       "the new connection");
          ret = MHD_SC_TLS_CONNECTION_INIT_FAILED;
        }
        else
        {
          c->conn_state = mhd_CONN_STATE_TLS_HANDSHAKE_RECV;
#ifndef NDEBUG
          c->dbg.tls_inited = true;
#endif
        }
      }
#endif /* MHD_SUPPORT_HTTPS */

      if (MHD_SC_OK == ret)
      {
        *conn_out = c;

        return MHD_SC_OK; /* Success exit point */
      }

      /* Below is a cleanup path */
      if (NULL != c->sk.addr.data)
        free (c->sk.addr.data);
    }
    free (c);
  }

  if ((NULL != addr) && external_add)
    free (addr);

  mhd_assert (MHD_SC_OK != ret);
  return ret; /* Failure exit point */
}


/**
 * Internal (inner) function.
 * Finally insert the new connection to the list of connections
 * served by the daemon and start processing.
 * @remark To be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param daemon daemon that manages the connection
 * @param connection the newly created connection
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
static enum MHD_StatusCode
new_connection_process_inner (struct MHD_Daemon *restrict daemon,
                              struct MHD_Connection *restrict connection)
{
  enum MHD_StatusCode res;
  mhd_assert (connection->daemon == daemon);

  res = MHD_SC_OK;               /* Mute compiler warning */
  mhd_assert (MHD_SC_OK == res); /* Mute analyser warning */
  /* Allocate memory pool in the processing thread so
   * intensively used memory area is allocated in "good"
   * (for the thread) memory region. It is important with
   * NUMA and/or complex cache hierarchy. */
  connection->pool = mhd_pool_create (daemon->conns.cfg.mem_pool_size,
                                      daemon->conns.cfg.mem_pool_zeroing);
  if (NULL == connection->pool)
  { /* 'pool' creation failed */
    mhd_LOG_MSG (daemon, MHD_SC_POOL_MEM_ALLOC_FAILURE, \
                 "Failed to allocate memory for the connection memory pool.");
    res = MHD_SC_POOL_MEM_ALLOC_FAILURE;
  }
  else
  {
    /* 'pool' creation succeed */

    mhd_assert (! daemon->conns.block_new);
    mhd_assert (daemon->conns.count < daemon->conns.cfg.count_limit);

    daemon->conns.count++;
    daemon->conns.block_new =
      (daemon->conns.count >= daemon->conns.cfg.count_limit);
    mhd_DLINKEDL_INS_FIRST (&(daemon->conns), connection, all_conn);

    mhd_conn_init_activity_timeout (connection,
                                    daemon->conns.cfg.timeout_milsec);

    connection_set_http_layer_init_state (connection);
    connection_set_initial_state (connection);

    notify_app_conn (daemon, connection, false);

#ifdef MHD_SUPPORT_THREADS
    if (mhd_DAEMON_TYPE_LISTEN_ONLY == daemon->threading.d_type)
    {
      mhd_assert ((mhd_POLL_TYPE_SELECT == daemon->events.poll_type) || \
                  (mhd_POLL_TYPE_POLL == daemon->events.poll_type));
      if (! mhd_create_named_thread (&connection->tid,
                                     "MHD-connection",
                                     daemon->threading.cfg.stack_size,
                                     &mhd_worker_connection,
                                     connection))
      {
#ifdef EAGAIN
        if (EAGAIN == errno)
        {
          mhd_LOG_MSG (daemon, MHD_SC_CONNECTION_THREAD_SYS_LIMITS_REACHED,
                       "Failed to create a new thread because it would "
                       "have exceeded the system limit on the number of "
                       "threads or no system resources available.");
          res = MHD_SC_CONNECTION_THREAD_SYS_LIMITS_REACHED;
        }
        else
#endif /* EAGAIN */
        if (1)
        {
          mhd_LOG_MSG (daemon, MHD_SC_CONNECTION_THREAD_LAUNCH_FAILURE,
                       "Failed to create a thread.");
          res = MHD_SC_CONNECTION_THREAD_LAUNCH_FAILURE;
        }
      }
      else               /* New thread has been created successfully */
        return MHD_SC_OK;  /* *** Function success exit point *** */
    }
    else
#else  /* ! MHD_SUPPORT_THREADS */
    if (1)
#endif /* ! MHD_SUPPORT_THREADS */
    { /* No 'thread-per-connection' */
#ifdef MHD_SUPPORT_THREADS
      connection->tid = daemon->threading.tid;
#endif /* MHD_SUPPORT_THREADS */
#ifdef MHD_SUPPORT_EPOLL
      if (mhd_POLL_TYPE_EPOLL == daemon->events.poll_type)
      {
        struct epoll_event event;

        event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        event.data.ptr = connection;
        if (0 != epoll_ctl (daemon->events.data.epoll.e_fd,
                            EPOLL_CTL_ADD,
                            connection->sk.fd,
                            &event))
        {
          mhd_LOG_MSG (daemon, MHD_SC_EPOLL_CTL_ADD_FAILED,
                       "Failed to add connection socket to epoll.");
          res = MHD_SC_EPOLL_CTL_ADD_FAILED;
        }
        else
        {
          mhd_dbg_print_fd_mon_req ("conn", \
                                    connection->sk.fd, \
                                    true, \
                                    true, \
                                    false);
          if (0) // TODO: implement turbo
          {
            connection->sk.ready =
              (enum mhd_SocketNetState) (mhd_SOCKET_NET_STATE_RECV_READY
                                         | mhd_SOCKET_NET_STATE_SEND_READY);
            mhd_conn_mark_ready (connection, daemon);
          }
          return MHD_SC_OK;  /* *** Function success exit point *** */
        }
      }
      else /* No 'epoll' */
#endif /* MHD_SUPPORT_EPOLL */
      return MHD_SC_OK;    /* *** Function success exit point *** */
    }

    /* ** Below is a cleanup path ** */
    mhd_assert (MHD_SC_OK != res);
    notify_app_conn (daemon, connection, true);

    mhd_conn_deinit_activity_timeout (connection);

    mhd_DLINKEDL_DEL (&(daemon->conns), connection, all_conn);
    daemon->conns.count--;
    daemon->conns.block_new = false;

    mhd_pool_destroy (connection->pool);
  }
  /* Free resources allocated before the call of this functions */

#ifdef MHD_SUPPORT_HTTPS
  if (mhd_C_HAS_TLS (connection))
    mhd_tls_conn_deinit (connection->tls);
#endif

  // TODO: per IP limit

  if (NULL != connection->sk.addr.data)
    free (connection->sk.addr.data);
  (void) mhd_socket_close (connection->sk.fd);
  free (connection);
  mhd_assert (MHD_SC_OK != res);
  return res;  /* *** Function failure exit point *** */
}


/**
 * Finally insert the new connection to the list of connections
 * served by the daemon and start processing.
 * @remark To be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param daemon daemon that manages the connection
 * @param connection the newly created connection
 * @return #MHD_SC_OK on success,
 *         error code otherwise
 */
static enum MHD_StatusCode
new_connection_process_ (struct MHD_Daemon *restrict daemon,
                         struct MHD_Connection *restrict connection)
{
  enum MHD_StatusCode res;

  res = new_connection_process_inner (daemon,
                                      connection);
#ifdef MHD_USE_TRACE_CONN_ADD_CLOSE
  if (MHD_SC_OK == res)
    fprintf (stderr,
             "&&&  Added new connection, FD: %2llu\n",
             (unsigned long long) connection->sk.fd);
  else
    fprintf (stderr,
             "&&& Failed add connection, FD: %2llu -> %u\n",
             (unsigned long long) connection->sk.fd,
             (unsigned int) res);
#endif /* MHD_USE_TRACE_CONN_ADD_CLOSE */

  return res;
}


/**
 * The given client socket will be managed (and closed!) by MHD after
 * this call and must no longer be used directly by the application
 * afterwards.
 *
 * @param daemon daemon that manages the connection
 * @param client_socket socket to manage (MHD will expect
 *        to receive an HTTP request from this socket next).
 * @param addrlen number of bytes in @a addr
 * @param addr IP address of the client,
 *                will be deallocated by free() if @a external_add is 'true'
 * @param external_add perform additional operations needed due
 *        to the application calling us directly
 * @param non_blck indicate that socket in non-blocking mode
 * @param sk_spipe_supprs indicate that the @a client_socket has
 *                         set SIGPIPE suppression
 * @param sk_is_nonip _MHD_YES if this is not a TCP/IP socket
 * @return #MHD_SC_OK on success,
 *         error on failure (the @a client_socket is closed)
 */
static MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_INOUT_SIZE_ (4,3) enum MHD_StatusCode
internal_add_connection (struct MHD_Daemon *daemon,
                         MHD_Socket client_socket,
                         size_t addrlen,
                         struct sockaddr_storage *addr,
                         bool external_add,
                         bool non_blck,
                         bool sk_spipe_supprs,
                         enum mhd_Tristate sk_is_nonip)
{
  struct MHD_Connection *connection;
  enum MHD_StatusCode res;

  /* Direct add to master daemon could never happen. */
  mhd_assert (! mhd_D_HAS_WORKERS (daemon));
  mhd_assert (mhd_FD_FITS_DAEMON (daemon, client_socket));

  res = MHD_SC_OK;

  if ((! non_blck) &&
      (mhd_POLL_TYPE_INT_IS_EPOLL (daemon->events.poll_type) ||
       (mhd_WM_INT_EXTERNAL_EVENTS_EDGE == daemon->wmode_int)))
  {
    mhd_LOG_MSG (daemon, MHD_SC_NONBLOCKING_REQUIRED, \
                 "The daemon configuration requires non-blocking sockets, "
                 "the new socket has not been added.");
    res = MHD_SC_NONBLOCKING_REQUIRED;
  }

  if (MHD_SC_OK == res)
  {
    if (daemon->conns.block_new)
    { /* Connections limit */
      mhd_LOG_MSG (daemon, MHD_SC_LIMIT_CONNECTIONS_REACHED, \
                   "Server reached connection limit. " \
                   "Closing inbound connection.");
      res = MHD_SC_LIMIT_CONNECTIONS_REACHED;
    }

    if (MHD_SC_OK == res)
    {
      res = new_connection_prepare_ (daemon,
                                     client_socket,
                                     addrlen, addr,
                                     external_add,
                                     non_blck,
                                     sk_spipe_supprs,
                                     sk_is_nonip,
                                     &connection);
      addr = NULL; /* Cleaned up with 'connection' if needed */

      if (MHD_SC_OK == res)
      {
        mhd_ASSUME (NULL != connection);

        if (external_add)
          res = MHD_SC_FEATURE_DISABLED;
        else
          return new_connection_process_ (daemon, connection);
      }
    }
  }

  if ((NULL != addr) && external_add)
    free (addr);
  mhd_socket_close (client_socket);

  mhd_assert (MHD_SC_OK != res);

  return res;
}


MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_IN_ (4) MHD_EXTERN_ enum MHD_StatusCode
MHD_daemon_add_connection (struct MHD_Daemon *MHD_RESTRICT daemon,
                           MHD_Socket new_socket,
                           size_t addr_size,
                           const struct sockaddr *MHD_RESTRICT addr,
                           void *connection_cntx)
{
  enum MHD_StatusCode ret;
  bool sk_nonbl;
  bool sk_spipe_supprs;

  sk_nonbl = false;

  // TODO: global daemon lock for external events
  (void) connection_cntx; // TODO: add support for connection's context

  ret = MHD_SC_OK;

  if (! mhd_D_TYPE_HAS_WORKERS (daemon->threading.d_type)
      && daemon->conns.block_new)
    ret = MHD_SC_LIMIT_CONNECTIONS_REACHED;

  if (NULL != addr)
  {
    bool log_bad_addlen;

    log_bad_addlen = false;
    if (0u == addr_size)
      log_bad_addlen = true;
    else if (addr_size < (sizeof(addr->sa_family)
                          + offsetof (struct sockaddr, sa_family)))
      log_bad_addlen = true;
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    else if (addr_size < (sizeof(addr->sa_len)
                          + offsetof (struct sockaddr, sa_len)))
      log_bad_addlen = true;
#endif /* HAVE_STRUCT_SOCKADDR_SA_LEN */
#ifdef SOCK_MAXADDRLEN
    else if (SOCK_MAXADDRLEN < addr_size)
      log_bad_addlen = true;
#endif
    else if (AF_INET == addr->sa_family)
    {
      if (sizeof(struct sockaddr_in) > addr_size)
        log_bad_addlen = true;
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
      else if ((0 != addr->sa_len) &&
               (sizeof(struct sockaddr_in) > (size_t) addr->sa_len) )
      {
        mhd_LOG_MSG (daemon, MHD_SC_CONFIGURATION_WRONG_SA_SIZE, \
                     "MHD_add_connection() has been called with " \
                     "non-zero value of 'sa_len' member of " \
                     "'struct sockaddr' which does not match 'sa_family'.");
        ret = MHD_SC_CONFIGURATION_WRONG_SA_SIZE;
      }
#endif /* HAVE_STRUCT_SOCKADDR_SA_LEN */
    }
#ifdef HAVE_INET6
    else if (AF_INET6 == addr->sa_family)
    {
      if (sizeof(struct sockaddr_in6) != addr_size)
        log_bad_addlen = true;
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
      else if ((0 != addr->sa_len) &&
               (sizeof(struct sockaddr_in6) > (size_t) addr->sa_len) )
      {
        mhd_LOG_MSG (daemon, MHD_SC_CONFIGURATION_WRONG_SA_SIZE, \
                     "MHD_add_connection() has been called with " \
                     "non-zero value of 'sa_len' member of " \
                     "'struct sockaddr' which does not match 'sa_family'.");
        ret = MHD_SC_CONFIGURATION_WRONG_SA_SIZE;
      }
#endif /* HAVE_STRUCT_SOCKADDR_SA_LEN */
    }
#endif /* HAVE_INET6 */
    mhd_ASSUME ((log_bad_addlen && (MHD_SC_OK == ret)) ||
                ((MHD_SC_OK != ret) && ! log_bad_addlen) || (MHD_SC_OK == ret));
    if (log_bad_addlen)
    {
      mhd_LOG_MSG (daemon, MHD_SC_CONFIGURATION_WRONG_SA_SIZE, \
                   "MHD_add_connection() has been called with " \
                   "incorrect 'addr_size' value.");
      ret = MHD_SC_CONFIGURATION_WRONG_SA_SIZE;
    }
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    if ((0 != addr->sa_len) &&
        (addr_size > (size_t) addr->sa_len))
      addr_size = (size_t) addr->sa_len;   /* Use safest value */
#endif /* HAVE_STRUCT_SOCKADDR_SA_LEN */
  }
  else
    addr_size = 0u;

  if (MHD_SC_OK == ret)
  {
    if (! mhd_FD_FITS_DAEMON (daemon,
                              new_socket))
    {
      mhd_LOG_MSG (daemon, MHD_SC_NEW_CONN_FD_OUTSIDE_OF_SET_RANGE, \
                   "The new connection FD value is higher than allowed");
      ret = MHD_SC_NEW_CONN_FD_OUTSIDE_OF_SET_RANGE;
    }
  }

  if (MHD_SC_OK == ret)
  {
    sk_nonbl = mhd_socket_nonblocking (new_socket);
    if (! sk_nonbl)
      mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_CONFIGURE_NONBLOCKING_FAILED, \
                   "Failed to set nonblocking mode on the new client socket.");

    if (1) // TODO: implement turbo
    {
      if (! mhd_socket_noninheritable (new_socket))
        mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_CONFIGURE_NOINHERIT_FAILED, \
                     "Failed to set noninheritable mode on new client socket.");
    }
  }

#ifndef MHD_SOCKETS_KIND_WINSOCK
  sk_spipe_supprs = false;
#else  /* MHD_SOCKETS_KIND_WINSOCK */
  sk_spipe_supprs = true; /* Nothing to suppress on W32 */
#endif /* MHD_SOCKETS_KIND_WINSOCK */
#if defined(mhd_socket_nosignal)
  if (MHD_SC_OK == ret)
  {
    if (! sk_spipe_supprs)
      sk_spipe_supprs = mhd_socket_nosignal (new_socket);
    if (! sk_spipe_supprs)
    {
      mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_CONFIGURE_NOSIGPIPE_FAILED, \
                   "Failed to suppress SIGPIPE on the new client socket.");
#  ifndef HAVE_DCLR_MSG_NOSIGNAL
      /* Application expects that SIGPIPE will be suppressed,
       * but suppression failed and SIGPIPE cannot be suppressed with send(). */
      if (! daemon->sigpipe_blocked)
        ret = MHD_SC_ACCEPT_CONFIGURE_NOSIGPIPE_FAILED;
#  endif /* HAVE_DCLR_MSG_NOSIGNAL */
    }
  }
#endif /* mhd_socket_nosignal */

  if (MHD_SC_OK == ret)
  {
    struct mhd_DaemonExtAddedConn *new_conn;

    new_conn =
      (struct mhd_DaemonExtAddedConn*)
      malloc (sizeof(struct mhd_DaemonExtAddedConn));

    if (NULL == new_conn)
      ret = MHD_SC_CONNECTION_MEM_ALLOC_FAILURE;
    else
    {
      mhd_DLINKEDL_INIT_LINKS (new_conn, queue);
      new_conn->skt = new_socket;
      new_conn->is_nonblock = sk_nonbl;
      new_conn->has_spipe_suppr = sk_spipe_supprs;
      new_conn->addr_size = addr_size;

      if (0 != addr_size)
      {
        new_conn->addr = (struct sockaddr_storage *) malloc (addr_size);
        if (NULL == new_conn->addr)
          ret = MHD_SC_CONNECTION_MEM_ALLOC_FAILURE;
        else
          memcpy (new_conn->addr,
                  addr,
                  addr_size);
      }
      else
        new_conn->addr = NULL;

      if (MHD_SC_OK == ret)
      {
        struct MHD_Daemon *d_to_add;
        if (! mhd_D_TYPE_HAS_WORKERS (daemon->threading.d_type))
          d_to_add = daemon;
        else
        {
#if defined(MHD_SUPPORT_THREADS)
          size_t d_offset;

          d_offset =
            mhd_atomic_counter_get_inc_wrap (
              &(daemon->events.act_req.ext_added.master.next_d_idx));

          d_to_add = (daemon->threading.hier.pool.workers
                      + (d_offset % daemon->threading.hier.pool.num));
          mhd_ASSUME (NULL != d_to_add);

          if (d_to_add->conns.block_new)
          {
            /* Try to find daemon with available connection slots */
            size_t i;

            /* Start from the other side of the workers pool to avoid
               conflict with the next called "add external connection". */
            d_offset += daemon->threading.hier.pool.num / 2;

            for (i = 0u; i < daemon->threading.hier.pool.num; ++i)
            {
              d_to_add = (daemon->threading.hier.pool.workers
                          + ((d_offset + i) % daemon->threading.hier.pool.num));

              if (d_to_add->conns.block_new)
                d_to_add = NULL;
              else
                break;
            }
          }
#else  /* ! MHD_SUPPORT_THREADS */
          mhd_UNREACHABLE ();
          d_to_add = NULL;
#endif  /* ! MHD_SUPPORT_THREADS */
        }

        if (NULL == d_to_add)
          ret = MHD_SC_LIMIT_CONNECTIONS_REACHED;
        else
        {
          mhd_mutex_lock_chk (
            &(d_to_add->events.act_req.ext_added.worker.q_lock));
          mhd_DLINKEDL_INS_LAST (&(d_to_add->events.act_req.ext_added.worker),
                                 new_conn,
                                 queue);
          mhd_mutex_unlock_chk (
            &(d_to_add->events.act_req.ext_added.worker.q_lock));

          mhd_daemon_trigger_itc (d_to_add);

          return MHD_SC_OK; /* Success exit point */
        }

        /* Below is a clean-up path */

        if (NULL != new_conn->addr)
          free (new_conn->addr);
      }
      free (new_conn);
    }
  }

  (void) mhd_socket_close (new_socket);

  mhd_assert (MHD_SC_OK != ret);

  return ret;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ enum mhd_DaemonAcceptResult
mhd_daemon_accept_connection (struct MHD_Daemon *restrict daemon)
{
  struct sockaddr_storage addrstorage[2]; /* Support non-standard extensions */
  socklen_t addrlen;
  MHD_Socket s;
  MHD_Socket fd;
  bool sk_nonbl;
  bool sk_spipe_supprs;
  bool sk_cloexec;
  enum mhd_Tristate sk_non_ip;
#if ! defined(NDEBUG) && defined (mhd_USE_ACCEPT4)
  const bool use_accept4 = ! daemon->dbg.avoid_accept4;
#elif defined (mhd_USE_ACCEPT4)
  static const bool use_accept4 = true;
#else  /* ! USE_ACCEPT4 && ! _DEBUG */
  static const bool use_accept4 = false;
#endif /* ! USE_ACCEPT4 && ! _DEBUG */

#ifdef MHD_SUPPORT_THREADS
  mhd_assert ((! mhd_D_HAS_THREADS (daemon)) || \
              mhd_thread_handle_ID_is_current_thread (daemon->threading.tid));
  mhd_assert (! mhd_D_TYPE_HAS_WORKERS (daemon->threading.d_type));
#endif /* MHD_SUPPORT_THREADS */

  fd = daemon->net.listen.fd;
  mhd_assert (MHD_INVALID_SOCKET != fd);
  mhd_assert (! daemon->net.listen.is_broken);

  addrlen = (socklen_t) sizeof (addrstorage);
  memset (addrstorage,
          0,
          (size_t) addrlen);
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN
  addrstorage->ss_len = (uint8_t) addrlen;
#endif /* HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN */

  /* Initialise with default values to avoid compiler warnings */
  sk_nonbl = false;
  sk_spipe_supprs = false;
  sk_cloexec = false;
  s = MHD_INVALID_SOCKET;

#ifdef mhd_USE_ACCEPT4
  if (use_accept4)
  {
    s = accept4 (fd,
                 (struct sockaddr *) addrstorage,
                 &addrlen,
                 mhd_SOCK_CLOEXEC | mhd_SOCK_NONBLOCK | mhd_SOCK_NOSIGPIPE);
    if (MHD_INVALID_SOCKET != s)
    {
      sk_nonbl = (mhd_SOCK_NONBLOCK != 0);
#ifndef MHD_SOCKETS_KIND_WINSOCK
      sk_spipe_supprs = (mhd_SOCK_NOSIGPIPE != 0);
#else  /* MHD_SOCKETS_KIND_WINSOCK */
      sk_spipe_supprs = true; /* Nothing to suppress on W32 */
#endif /* MHD_SOCKETS_KIND_WINSOCK */
      sk_cloexec = (mhd_SOCK_CLOEXEC != 0);
    }
  }
#endif /* mhd_USE_ACCEPT4 */
#if ! defined(mhd_USE_ACCEPT4) || ! defined(NDEBUG)
  if (! use_accept4)
  {
    s = accept (fd,
                (struct sockaddr *) addrstorage,
                &addrlen);
    if (MHD_INVALID_SOCKET != s)
    {
#ifdef MHD_ACCEPTED_INHERITS_NONBLOCK
      sk_nonbl = daemon->net.listen.non_block;
#else  /* ! MHD_ACCEPTED_INHERITS_NONBLOCK */
      sk_nonbl = false;
#endif /* ! MHD_ACCEPTED_INHERITS_NONBLOCK */
#ifndef MHD_SOCKETS_KIND_WINSOCK
      sk_spipe_supprs = false;
#else  /* MHD_SOCKETS_KIND_WINSOCK */
      sk_spipe_supprs = true; /* Nothing to suppress on W32 */
#endif /* MHD_SOCKETS_KIND_WINSOCK */
      sk_cloexec = false;
    }
  }
#endif /* !mhd_USE_ACCEPT4 || _DEBUG */

  if (MHD_INVALID_SOCKET == s)
  { /* This could be a common occurrence with multiple worker threads */
    const int err = mhd_SCKT_GET_LERR ();

    if (mhd_SCKT_ERR_IS_EINVAL (err))
      return mhd_DAEMON_ACCEPT_NO_MORE_PENDING; /* can happen during shutdown */   // FIXME: remove?
    if (mhd_SCKT_ERR_IS_DISCNN_BEFORE_ACCEPT (err))
      return mhd_DAEMON_ACCEPT_NO_MORE_PENDING;   /* do not print error if client just disconnects early */
    if (mhd_SCKT_ERR_IS_EINTR (err))
      return mhd_DAEMON_ACCEPT_SKIPPED;
    if (mhd_SCKT_ERR_IS_EAGAIN (err))
      return mhd_DAEMON_ACCEPT_NO_MORE_PENDING;
    if (mhd_SCKT_ERR_IS_LOW_RESOURCES (err) )
    {
      /* system/process out of resources */
      if (0 == daemon->conns.count)
      {
        /* Not setting 'block_new' flag, as there is no way it
           would ever be cleared.  Instead trying to produce
           bit fat ugly warning. */
        mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED_INSTANTLY, \
                     "Hit process or system resource limit at FIRST " \
                     "connection. This is really bad as there is no sane " \
                     "way to proceed. Will try busy waiting for system " \
                     "resources to become magically available.");
      }
      else
      {
        daemon->conns.block_new = true;
        mhd_LOG_PRINT (daemon, MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED, \
                       mhd_LOG_FMT ("Hit process or system resource limit " \
                                    "at %u connections, temporarily " \
                                    "suspending accept(). Consider setting " \
                                    "a lower MHD_OPTION_CONNECTION_LIMIT."), \
                       daemon->conns.count);
      }
      return mhd_DAEMON_ACCEPT_FAILED;
    }
    mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_FAILED_UNEXPECTEDLY,
                 "Error accepting connection.");
    return mhd_DAEMON_ACCEPT_FAILED;
  }

  if (! mhd_FD_FITS_DAEMON (daemon, s))
  {
    mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_OUTSIDE_OF_SET_RANGE, \
                 "The accepted socket has value outside of allowed range.");
    (void) mhd_socket_close (s);
    return mhd_DAEMON_ACCEPT_FAILED;
  }
  if (mhd_SOCKET_TYPE_IP == daemon->net.listen.type)
    sk_non_ip = mhd_T_NO;
  else if (mhd_SOCKET_TYPE_UNKNOWN == daemon->net.listen.type)
    sk_non_ip = mhd_T_MAYBE;
  else
    sk_non_ip = mhd_T_YES;
  if (0 >= addrlen)
  {
    if (mhd_SOCKET_TYPE_IP == daemon->net.listen.type)
      mhd_LOG_MSG (daemon, MHD_SC_ACCEPTED_UNKNOWN_TYPE, \
                   "Accepted socket has non-positive length of the address. " \
                   "Processing the new socket as a socket with " \
                   "unknown type.");
    addrlen = 0;
    sk_non_ip = mhd_T_MAYBE;
  }
  else if (((socklen_t) sizeof (addrstorage)) < addrlen)
  {
    /* Should not happen as 'sockaddr_storage' must be large enough to
     * store any address supported by the system. */
    mhd_LOG_MSG (daemon, MHD_SC_ACCEPTED_SOCKADDR_TOO_LARGE, \
                 "Accepted socket address is larger than expected by " \
                 "system headers. Processing the new socket as a socket with " \
                 "unknown type.");
    addrlen = 0;
    sk_non_ip = mhd_T_MAYBE; /* IP-type addresses must fit */
  }
  else if (mhd_T_MAYBE == sk_non_ip)
  {
    if (AF_INET == ((struct sockaddr *) addrstorage)->sa_family)
      sk_non_ip = mhd_T_NO;
#ifdef HAVE_INET6
    else if (AF_INET6 == ((struct sockaddr *) addrstorage)->sa_family)
      sk_non_ip = mhd_T_NO;
#endif /* HAVE_INET6 */
  }

  if (! sk_nonbl)
  { /* Was not set automatically */
    sk_nonbl = mhd_socket_nonblocking (s);
    if (! sk_nonbl)
      mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_CONFIGURE_NONBLOCKING_FAILED, \
                   "Failed to set nonblocking mode on "
                   "new connection socket.");
  }

  if (! sk_cloexec)
  { /* Was not set automatically */
    sk_cloexec =  mhd_socket_noninheritable (s);
    if (! sk_cloexec)
      mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_CONFIGURE_NOINHERIT_FAILED, \
                   "Failed to set non-inheritable mode on "
                   "new connection socket.");
  }

#if defined(mhd_socket_nosignal)
  if (! sk_spipe_supprs && ! mhd_socket_nosignal (s))
  {
    mhd_LOG_MSG (daemon, MHD_SC_ACCEPT_CONFIGURE_NOSIGPIPE_FAILED,
                 "Failed to suppress SIGPIPE on incoming connection " \
                 "socket.");
#ifndef HAVE_DCLR_MSG_NOSIGNAL
    /* Application expects that SIGPIPE will be suppressed,
     * but suppression failed and SIGPIPE cannot be suppressed with send(). */
    if (! daemon->sigpipe_blocked)
    {
      (void) MHD_socket_close_ (s);
      return mhd_DAEMON_ACCEPT_FAILED;
    }
#endif /* HAVE_DCLR_MSG_NOSIGNAL */
  }
  else
    sk_spipe_supprs = true;
#endif /* mhd_socket_nosignal */
  return (MHD_SC_OK == internal_add_connection (daemon,
                                                s,
                                                (size_t) addrlen,
                                                addrstorage,
                                                false,
                                                sk_nonbl,
                                                sk_spipe_supprs,
                                                sk_non_ip)) ?
         mhd_DAEMON_ACCEPT_SUCCESS : mhd_DAEMON_ACCEPT_FAILED;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_conn_remove_from_daemon (struct MHD_Connection *restrict c)
{
  mhd_assert (c->dbg.closing_started);
  mhd_assert (c->dbg.pre_cleaned);
  mhd_assert (! c->dbg.removed_from_daemon);
  mhd_assert (NULL == c->rp.response);
  mhd_assert (! c->rq.app_aware);
  mhd_assert (! c->in_proc_ready);
  mhd_assert (NULL == c->rq.cntn.lbuf.data);
  mhd_assert (NULL == mhd_DLINKEDL_GET_NEXT (c, proc_ready));
  mhd_assert (NULL == mhd_DLINKEDL_GET_PREV (c, proc_ready));
  mhd_assert (c != mhd_DLINKEDL_GET_FIRST (&(c->daemon->events), proc_ready));
  mhd_assert (c != mhd_DLINKEDL_GET_LAST (&(c->daemon->events), proc_ready));

  if (mhd_D_HAS_THR_PER_CONN (c->daemon))
  {
    mhd_assert (0 && "Not implemented yet");
    // TODO: Support "thread per connection"
  }
  mhd_assert (NULL == mhd_DLINKEDL_GET_NEXT (&(c->timeout), tmout_list));
  mhd_assert (NULL == mhd_DLINKEDL_GET_PREV (&(c->timeout), tmout_list));
  mhd_assert (NULL == c->pool);

  mhd_DLINKEDL_DEL (&(c->daemon->conns), c, all_conn);

  // TODO: update per-IP limits

  c->daemon->conns.count--;
  c->daemon->conns.block_new = false;

#ifndef NDEBUG
  c->dbg.removed_from_daemon = true;
#endif /* NDEBUG */
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_conn_close_final (struct MHD_Connection *restrict c)
{
  mhd_assert (c->dbg.closing_started);
  mhd_assert (c->dbg.pre_cleaned);
  mhd_assert (c->dbg.removed_from_daemon);
  mhd_assert (NULL == c->rp.response);
  mhd_assert (! c->rq.app_aware);
  mhd_assert (! c->in_proc_ready);
  mhd_assert (NULL == mhd_DLINKEDL_GET_NEXT (c, proc_ready));
  mhd_assert (NULL == mhd_DLINKEDL_GET_PREV (c, proc_ready));
  mhd_assert (c != mhd_DLINKEDL_GET_FIRST (&(c->daemon->events), proc_ready));
  mhd_assert (c != mhd_DLINKEDL_GET_LAST (&(c->daemon->events), proc_ready));

  mhd_assert (NULL == mhd_DLINKEDL_GET_NEXT (&(c->timeout), tmout_list));
  mhd_assert (NULL == mhd_DLINKEDL_GET_PREV (&(c->timeout), tmout_list));
  mhd_assert (NULL == c->pool);

  mhd_assert (NULL == mhd_DLINKEDL_GET_NEXT (c, all_conn));
  mhd_assert (NULL == mhd_DLINKEDL_GET_PREV (c, all_conn));
  mhd_assert (c != mhd_DLINKEDL_GET_FIRST (&(c->daemon->conns), all_conn));
  mhd_assert (c != mhd_DLINKEDL_GET_LAST (&(c->daemon->conns), all_conn));

#ifdef MHD_SUPPORT_HTTPS
  if (mhd_C_HAS_TLS (c))
  {
    mhd_assert (mhd_D_HAS_TLS (c->daemon));
    mhd_assert (c->dbg.tls_inited);
    mhd_tls_conn_deinit (c->tls);
  }
#  ifndef NDEBUG
  else
  {
    mhd_assert (! mhd_D_HAS_TLS (c->daemon));
    mhd_assert (! c->dbg.tls_inited);
  }
#  endif
#endif

  if (NULL != c->sk.addr.data)
    free (c->sk.addr.data);
  mhd_socket_close (c->sk.fd);
#ifdef MHD_USE_TRACE_CONN_ADD_CLOSE
  fprintf (stderr,
           "&&&     Closed connection, FD: %2llu\n",
           (unsigned long long) c->sk.fd);
#endif /* MHD_USE_TRACE_CONN_ADD_CLOSE */

  free (c);
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_daemon_process_ext_added_conns (struct MHD_Daemon *restrict d)
{
  struct mhd_DaemonExtAddedConn *ext_added;
  mhd_DLNKDL_LIST (mhd_DaemonExtAddedConn, detached_q);

  mhd_assert (! mhd_D_HAS_WORKERS (d));

  if (NULL ==
      mhd_DLINKEDL_GET_FIRST (&(d->events.act_req.ext_added.worker),
                              queue))
    return; /* Shortcut: the queue is empty */

  /* Detach the queue to quickly manipulate the lock one time only */
  mhd_mutex_lock_chk (&(d->events.act_req.ext_added.worker.q_lock));
  detached_q = d->events.act_req.ext_added.worker.queue;
  mhd_DLINKEDL_INIT_LIST (&(d->events.act_req.ext_added.worker),
                          queue);
  mhd_mutex_unlock_chk (&(d->events.act_req.ext_added.worker.q_lock));

  /* Process without lock the detached queue in FIFO order */
  for (ext_added = mhd_DLINKEDL_GET_FIRST_D (&detached_q);
       NULL != ext_added;
       ext_added = mhd_DLINKEDL_GET_FIRST_D (&detached_q))
  {
    mhd_ASSUME (NULL == mhd_DLINKEDL_GET_PREV (ext_added,
                                               queue));
    mhd_DLINKEDL_DEL_D (&detached_q, ext_added,
                        queue);

    if (! d->conns.block_new)
    {
      (void) internal_add_connection (d,
                                      ext_added->skt,
                                      ext_added->addr_size,
                                      ext_added->addr,
                                      true,
                                      ext_added->is_nonblock,
                                      ext_added->has_spipe_suppr,
                                      mhd_T_MAYBE);
    }
    else
    {
      if (NULL != ext_added->addr)
        free (ext_added->addr);
    }
    free (ext_added);
  }
}
