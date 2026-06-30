/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2022-2026 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/stream_funcs.c
 * @brief  The definition of the stream internal functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "stream_funcs.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#ifdef MHD_USE_TRACE_CONN_ADD_CLOSE
#  include <stdio.h>
#endif /* MHD_USE_TRACE_CONN_ADD_CLOSE */
#include "mhd_dbg_print.h"
#include <string.h>
#include "extr_events_funcs.h"
#ifdef MHD_SUPPORT_EPOLL
#  include <sys/epoll.h>
#endif
#include "sys_kqueue.h"
#include "sys_malloc.h"

#include "mhd_daemon.h"
#include "mhd_connection.h"
#include "mhd_response.h"
#include "mempool_funcs.h"
#include "mhd_str.h"
#include "mhd_str_macros.h"

#include "mhd_sockets_funcs.h"

#include "request_get_value.h"
#include "response_destroy.h"
#include "mhd_mono_clock.h"
#include "daemon_logger.h"
#include "daemon_funcs.h"
#include "conn_timeout.h"
#include "conn_mark_ready.h"
#include "stream_process_reply.h"
#include "extr_events_funcs.h"

#ifdef MHD_SUPPORT_HTTPS
#  include "mhd_tls_funcs.h"
#endif

#include "mhd_public_api.h"


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void *
mhd_stream_alloc_memory (struct MHD_Connection *restrict c,
                         size_t size)
{
  struct mhd_MemoryPool *const restrict pool = c->pool;     /* a short alias */
  size_t need_to_be_freed = 0; /**< The required amount of additional free memory */
  void *res;

  res = mhd_pool_try_alloc (pool,
                            size,
                            &need_to_be_freed);
  if (NULL != res)
    return res;

  if (mhd_pool_is_resizable_inplace (pool,
                                     c->write_buffer,
                                     c->write_buffer_size))
  {
    if (c->write_buffer_size - c->write_buffer_append_offset >=
        need_to_be_freed)
    {
      char *buf;
      const size_t new_buf_size = c->write_buffer_size - need_to_be_freed;
      buf = (char *) mhd_pool_reallocate (pool,
                                          c->write_buffer,
                                          c->write_buffer_size,
                                          new_buf_size);
      mhd_assert (c->write_buffer == buf);
      mhd_assert (c->write_buffer_append_offset <= new_buf_size);
      mhd_assert (c->write_buffer_send_offset <= new_buf_size);
      c->write_buffer_size = new_buf_size;
      c->write_buffer = buf;
    }
    else
      return NULL;
  }
  else if (mhd_pool_is_resizable_inplace (pool,
                                          c->read_buffer,
                                          c->read_buffer_size))
  {
    if (c->read_buffer_size - c->read_buffer_offset >= need_to_be_freed)
    {
      char *buf;
      const size_t new_buf_size = c->read_buffer_size - need_to_be_freed;
      buf = (char *) mhd_pool_reallocate (pool,
                                          c->read_buffer,
                                          c->read_buffer_size,
                                          new_buf_size);
      mhd_assert (c->read_buffer == buf);
      mhd_assert (c->read_buffer_offset <= new_buf_size);
      c->read_buffer_size = new_buf_size;
      c->read_buffer = buf;
    }
    else
      return NULL;
  }
  else
    return NULL;
  res = mhd_pool_allocate (pool, size, true);
  mhd_assert (NULL != res); /* It has been checked that pool has enough space */
  return res;
}


/**
 * Shrink stream read buffer to the zero size of free space in the buffer
 * @param c the connection whose read buffer is being manipulated
 */
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_stream_shrink_read_buffer (struct MHD_Connection *restrict c)
{
  void *new_buf;

  if ((NULL == c->read_buffer) || (0 == c->read_buffer_size))
  {
    mhd_assert (0 == c->read_buffer_size);
    mhd_assert (0 == c->read_buffer_offset);
    return;
  }

  mhd_assert (c->read_buffer_offset <= c->read_buffer_size);
  if (0 == c->read_buffer_offset)
  {
    mhd_pool_deallocate (c->pool, c->read_buffer, c->read_buffer_size);
    c->read_buffer = NULL;
    c->read_buffer_size = 0;
  }
  else
  {
    mhd_assert (mhd_pool_is_resizable_inplace (c->pool, c->read_buffer, \
                                               c->read_buffer_size));
    new_buf = mhd_pool_reallocate (c->pool, c->read_buffer, c->read_buffer_size,
                                   c->read_buffer_offset);
    mhd_assert (c->read_buffer == new_buf);
    c->read_buffer = (char *) new_buf;
    c->read_buffer_size = c->read_buffer_offset;
  }
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ size_t
mhd_stream_maximize_write_buffer (struct MHD_Connection *restrict c)
{
  struct mhd_MemoryPool *const restrict pool = c->pool;
  void *new_buf;
  size_t new_size;
  size_t free_size;

  mhd_assert ((NULL != c->write_buffer) || (0 == c->write_buffer_size));
  mhd_assert (c->write_buffer_append_offset >= c->write_buffer_send_offset);
  mhd_assert (c->write_buffer_size >= c->write_buffer_append_offset);

  free_size = mhd_pool_get_free (pool);
  if (0 != free_size)
  {
    new_size = c->write_buffer_size + free_size;
    /* This function must not move the buffer position.
     * mhd_pool_reallocate () may return the new position only if buffer was
     * allocated 'from_end' or is not the last allocation,
     * which should not happen. */
    mhd_assert ((NULL == c->write_buffer) || \
                mhd_pool_is_resizable_inplace (pool, c->write_buffer, \
                                               c->write_buffer_size));
    new_buf = mhd_pool_reallocate (pool,
                                   c->write_buffer,
                                   c->write_buffer_size,
                                   new_size);
    mhd_assert ((c->write_buffer == new_buf) || (NULL == c->write_buffer));
    c->write_buffer = (char *) new_buf;
    c->write_buffer_size = new_size;
    if (c->write_buffer_send_offset == c->write_buffer_append_offset)
    {
      /* All data have been sent, reset offsets to zero. */
      c->write_buffer_send_offset = 0;
      c->write_buffer_append_offset = 0;
    }
  }

  return c->write_buffer_size - c->write_buffer_append_offset;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_stream_release_write_buffer (struct MHD_Connection *restrict c)
{
  struct mhd_MemoryPool *const restrict pool = c->pool;

  mhd_assert ((NULL != c->write_buffer) || (0 == c->write_buffer_size));
  mhd_assert (c->write_buffer_append_offset == c->write_buffer_send_offset);
  mhd_assert (c->write_buffer_size >= c->write_buffer_append_offset);

  mhd_pool_deallocate (pool, c->write_buffer, c->write_buffer_size);
  c->write_buffer_send_offset = 0;
  c->write_buffer_append_offset = 0;
  c->write_buffer_size = 0;
  c->write_buffer = NULL;

}


#ifndef MHD_MAX_REASONABLE_HEADERS_SIZE_
/**
 * A reasonable headers size (excluding request line) that should be sufficient
 * for most requests.
 * If incoming data buffer free space is not enough to process the complete
 * header (the request line and all headers) and the headers size is larger than
 * this size then the status code 431 "Request Header Fields Too Large" is
 * returned to the client.
 * The larger headers are processed by MHD if enough space is available.
 */
#  define MHD_MAX_REASONABLE_HEADERS_SIZE_ (6 * 1024)
#endif /* ! MHD_MAX_REASONABLE_HEADERS_SIZE_ */

#ifndef MHD_MAX_REASONABLE_REQ_TARGET_SIZE_
/**
 * A reasonable request target (the request URI) size that should be sufficient
 * for most requests.
 * If incoming data buffer free space is not enough to process the complete
 * header (the request line and all headers) and the request target size is
 * larger than this size then the status code 414 "URI Too Long" is
 * returned to the client.
 * The larger request targets are processed by MHD if enough space is available.
 * The value chosen according to RFC 9112 Section 3, paragraph 5
 */
#  define MHD_MAX_REASONABLE_REQ_TARGET_SIZE_ 8000
#endif /* ! MHD_MAX_REASONABLE_REQ_TARGET_SIZE_ */

#ifndef MHD_MIN_REASONABLE_HEADERS_SIZE_
/**
 * A reasonable headers size (excluding request line) that should be sufficient
 * for basic simple requests.
 * When no space left in the receiving buffer try to avoid replying with
 * the status code 431 "Request Header Fields Too Large" if headers size
 * is smaller then this value.
 */
#  define MHD_MIN_REASONABLE_HEADERS_SIZE_ 26
#endif /* ! MHD_MIN_REASONABLE_HEADERS_SIZE_ */

#ifndef MHD_MIN_REASONABLE_REQ_TARGET_SIZE_
/**
 * A reasonable request target (the request URI) size that should be sufficient
 * for basic simple requests.
 * When no space left in the receiving buffer try to avoid replying with
 * the status code 414 "URI Too Long" if the request target size is smaller then
 * this value.
 */
#  define MHD_MIN_REASONABLE_REQ_TARGET_SIZE_ 40
#endif /* ! MHD_MIN_REASONABLE_REQ_TARGET_SIZE_ */

#ifndef MHD_MIN_REASONABLE_REQ_METHOD_SIZE_
/**
 * A reasonable request method string size that should be sufficient
 * for basic simple requests.
 * When no space left in the receiving buffer try to avoid replying with
 * the status code 501 "Not Implemented" if the request method size is
 * smaller then this value.
 */
#  define MHD_MIN_REASONABLE_REQ_METHOD_SIZE_ 16
#endif /* ! MHD_MIN_REASONABLE_REQ_METHOD_SIZE_ */

#ifndef MHD_MIN_REASONABLE_REQ_CHUNK_LINE_LENGTH_
/**
 * A reasonable minimal chunk line length.
 * When no space left in the receiving buffer reply with 413 "Content Too Large"
 * if the chunk line length is larger than this value.
 */
#  define MHD_MIN_REASONABLE_REQ_CHUNK_LINE_LENGTH_ 4
#endif /* ! MHD_MIN_REASONABLE_REQ_CHUNK_LINE_LENGTH_ */


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_IN_SIZE_ (4,3) unsigned int
mhd_stream_get_no_space_err_status_code (struct MHD_Connection *restrict c,
                                         enum MHD_ProcRecvDataStage stage,
                                         size_t add_element_size,
                                         const char *restrict add_element)
{
  size_t method_size;
  size_t uri_size;
  size_t opt_headers_size;
  size_t host_field_line_size;

  mhd_assert ((0 == add_element_size) || (NULL != add_element));

  c->rq.too_large = true;

  if (mhd_HTTP_STAGE_REQ_LINE_RECEIVED < c->stage)
  {
    if (mhd_HTTP_STAGE_HEADERS_RECEIVED > c->stage)
    {
      mhd_assert (NULL != c->rq.field_lines.start);
      opt_headers_size =
        (size_t) ((c->read_buffer + c->read_buffer_offset)
                  - c->rq.field_lines.start);
    }
    else
      opt_headers_size = c->rq.field_lines.size;
  }
  else
    opt_headers_size = 0u;

  /* The read buffer is fully used by the request line, the field lines
     (headers) and internal information.
     The return status code works as a suggestion for the client to reduce
     one of the request elements. */

  if ((MHD_PROC_RECV_BODY_CHUNKED == stage) &&
      (MHD_MIN_REASONABLE_REQ_CHUNK_LINE_LENGTH_ < add_element_size))
  {
    /* Request could be re-tried easily with smaller chunk sizes */
    return MHD_HTTP_STATUS_CONTENT_TOO_LARGE;
  }

  host_field_line_size = 0;
  /* The "Host:" field line is mandatory.
     The total size of the field lines (headers) cannot be smaller than
     the size of the "Host:" field line. */
  if ((MHD_PROC_RECV_HEADERS == stage)
      && (0 != add_element_size))
  {
    static const size_t header_host_key_len =
      mhd_SSTR_LEN (MHD_HTTP_HEADER_HOST);
    const bool is_host_header =
      (header_host_key_len + 1 <= add_element_size)
      && ( (0 == add_element[header_host_key_len])
           || (':' == add_element[header_host_key_len]) )
      && mhd_str_equal_caseless_bin_n (MHD_HTTP_HEADER_HOST,
                                       add_element,
                                       header_host_key_len);
    if (is_host_header)
    {
      const bool is_parsed = ! (
        (mhd_HTTP_STAGE_HEADERS_RECEIVED > c->stage) &&
        (add_element_size == c->read_buffer_offset) &&
        (c->read_buffer == add_element) );
      size_t actual_element_size;

      mhd_assert (! is_parsed || (0 == add_element[header_host_key_len]));
      /* The actual size should be larger due to CRLF or LF chars,
         however the exact termination sequence is not known here and
         as perfect precision is not required, to simplify the code
         assume the minimal length. */
      if (is_parsed)
        actual_element_size = add_element_size + 1;  /* "1" for LF */
      else
        actual_element_size = add_element_size;

      host_field_line_size = actual_element_size;
      mhd_assert (opt_headers_size >= actual_element_size);
      opt_headers_size -= actual_element_size;
    }
  }
  if (0 == host_field_line_size)
  {
    static const size_t host_field_name_len =
      mhd_SSTR_LEN (MHD_HTTP_HEADER_HOST);
    struct MHD_StringNullable host_value;

    if (mhd_request_get_value_n (&(c->rq),
                                 MHD_VK_HEADER,
                                 host_field_name_len,
                                 MHD_HTTP_HEADER_HOST,
                                 &host_value))
    {
      /* Calculate the minimal size of the field line: no space between
         colon and the field value, line terminated by LR */
      host_field_line_size =
        host_field_name_len + host_value.len + 2; /* "2" for ':' and LF */

      /* The "Host:" field could be added by application */
      if (opt_headers_size >= host_field_line_size)
      {
        opt_headers_size -= host_field_line_size;
        /* Take into account typical space after colon and CR at the end of the line */
        if (opt_headers_size >= 2)
          opt_headers_size -= 2;
      }
      else
        host_field_line_size = 0; /* No "Host:" field line set by the client */
    }
  }

  uri_size = c->rq.req_target_len;
  if (mhd_HTTP_METHOD_OTHER != c->rq.http_mthd)
    method_size = 0; /* Do not recommend shorter request method */
  else
  {
    mhd_assert (NULL != c->rq.method.cstr);
    method_size = c->rq.method.len;
    mhd_assert (method_size == strlen (c->rq.method.cstr));
  }

  if ((size_t) MHD_MAX_REASONABLE_HEADERS_SIZE_ < opt_headers_size)
  {
    /* Typically the easiest way to reduce request header size is
       a removal of some optional headers. */
    if (opt_headers_size > (uri_size / 8))
    {
      if ((opt_headers_size / 2) > method_size)
        return MHD_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;
      else
        return MHD_HTTP_STATUS_NOT_IMPLEMENTED; /* The length of the HTTP request method is unreasonably large */
    }
    else
    { /* Request target is MUCH larger than headers */
      if ((uri_size / 16) > method_size)
        return MHD_HTTP_STATUS_URI_TOO_LONG;
      else
        return MHD_HTTP_STATUS_NOT_IMPLEMENTED; /* The length of the HTTP request method is unreasonably large */
    }
  }
  if ((size_t) MHD_MAX_REASONABLE_REQ_TARGET_SIZE_ < uri_size)
  {
    /* If request target size if larger than maximum reasonable size
       recommend client to reduce the request target size (length). */
    if ((uri_size / 16) > method_size)
      return MHD_HTTP_STATUS_URI_TOO_LONG;     /* Request target is MUCH larger than headers */
    else
      return MHD_HTTP_STATUS_NOT_IMPLEMENTED;  /* The length of the HTTP request method is unreasonably large */
  }

  /* The read buffer is too small to handle reasonably large requests */

  if ((size_t) MHD_MIN_REASONABLE_HEADERS_SIZE_ < opt_headers_size)
  {
    /* Recommend application to retry with minimal headers */
    if ((opt_headers_size * 4) > uri_size)
    {
      if (opt_headers_size > method_size)
        return MHD_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;
      else
        return MHD_HTTP_STATUS_NOT_IMPLEMENTED; /* The length of the HTTP request method is unreasonably large */
    }
    else
    { /* Request target is significantly larger than headers */
      if (uri_size > method_size * 4)
        return MHD_HTTP_STATUS_URI_TOO_LONG;
      else
        return MHD_HTTP_STATUS_NOT_IMPLEMENTED; /* The length of the HTTP request method is unreasonably large */
    }
  }
  if ((size_t) MHD_MIN_REASONABLE_REQ_TARGET_SIZE_ < uri_size)
  {
    /* Recommend application to retry with a shorter request target */
    if (uri_size > method_size * 4)
      return MHD_HTTP_STATUS_URI_TOO_LONG;
    else
      return MHD_HTTP_STATUS_NOT_IMPLEMENTED; /* The length of the HTTP request method is unreasonably large */
  }

  if ((size_t) MHD_MIN_REASONABLE_REQ_METHOD_SIZE_ < method_size)
  {
    /* The request target (URI) and headers are (reasonably) very small.
       Some non-standard long request method is used. */
    /* The last resort response as it means "the method is not supported
       by the server for any URI". */
    return MHD_HTTP_STATUS_NOT_IMPLEMENTED;
  }

  /* The almost impossible situation: all elements are small, but cannot
     fit the buffer. The application set the buffer size to
     critically low value? */

  if ((1 < opt_headers_size) || (1 < uri_size))
  {
    if (opt_headers_size >= uri_size)
      return MHD_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;
    else
      return MHD_HTTP_STATUS_URI_TOO_LONG;
  }

  /* Nothing to reduce in the request.
     Reply with some status. */
  if (0 != host_field_line_size)
    return MHD_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;

  return MHD_HTTP_STATUS_URI_TOO_LONG;
}


MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_stream_switch_from_recv_to_send (struct MHD_Connection *c)
{
  /* Read buffer is not needed for this request, shrink it.*/
  mhd_stream_shrink_read_buffer (c);
}


/**
 * Finish request serving.
 * The stream will be re-used or closed.
 *
 * @param c the connection to use.
 */
MHD_INTERNAL MHD_FN_PAR_NONNULL_ALL_ void
mhd_stream_finish_req_serving (struct MHD_Connection *restrict c,
                               bool reuse)
{
  struct MHD_Daemon *const restrict d = c->daemon;

  if (! reuse)
  {
    mhd_assert (! c->stop_with_error || (NULL == c->rp.response) || \
                (c->rp.response->cfg.int_err_resp));

    /* Next function will notify client and set connection
     * state to "PRE-CLOSING" */
    /* Later response and memory pool will be destroyed */
    mhd_conn_start_closing (c,
                            c->stop_with_error ?
                            mhd_CONN_CLOSE_ERR_REPLY_SENT :
                            mhd_CONN_CLOSE_HTTP_COMPLETED,
                            NULL);
  }
  else
  {
    /* Reset connection to process the next request */
    size_t new_read_buf_size;
    mhd_assert (! c->stop_with_error);
    mhd_assert (! c->discard_request);
    mhd_assert (NULL == c->rq.cntn.lbuf.data);

#if 0 // TODO: notification callback
    if ( (NULL != d->notify_completed) &&
         (c->rq.app_aware) )
      d->notify_completed (d->notify_completed_cls,
                           c,
                           &c->rq.app_context,
                           MHD_REQUEST_ENDED_COMPLETED_OK);
    c->rq.app_aware = false;
#endif

    mhd_stream_call_dcc_cleanup_if_needed (c);
    if (NULL != c->rp.resp_iov.iov)
    {
      free (c->rp.resp_iov.iov);
      c->rp.resp_iov.iov = NULL;
    }

    if (NULL != c->rp.response)
      mhd_response_dec_use_count (c->rp.response);
    c->rp.response = NULL;

    c->conn_reuse = mhd_CONN_KEEPALIVE_POSSIBLE;
    c->stage = mhd_HTTP_STAGE_INIT;
    c->event_loop_info = MHD_EVENT_LOOP_INFO_RECV; /* Dummy state, real state set later */

    // TODO: move request reset to special function
    memset (&c->rq, 0, sizeof(c->rq));

    // TODO: move reply reset to special function
    /* iov (if any) will be deallocated by mhd_pool_reset */
    memset (&c->rp, 0, sizeof(c->rp));

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

    /* Reset the read buffer to the starting size,
       preserving the bytes we have already read. */
    new_read_buf_size = d->conns.cfg.mem_pool_size / 2;
    if (c->read_buffer_offset > new_read_buf_size)
      new_read_buf_size = c->read_buffer_offset;

    c->read_buffer
      = (char *) mhd_pool_reset (c->pool,
                                 c->read_buffer,
                                 c->read_buffer_offset,
                                 new_read_buf_size);
    c->read_buffer_size = new_read_buf_size;
  }
  c->rq.app_context = NULL;
}


/* return 'true' is lingering needed, 'false' is lingering is not needed */
static MHD_FN_PAR_NONNULL_ALL_ bool
conn_start_socket_closing (struct MHD_Connection *restrict c,
                           bool close_hard)
{
  bool need_lingering;
  /* Make changes on the socket early to let the kernel and the remote
   * to process the changes in parallel. */
  if (close_hard)
  {
    /* Use abortive closing, send RST to remote to indicate a problem */
    (void) mhd_socket_set_hard_close (c->sk.fd);
    c->stage = mhd_HTTP_STAGE_PRE_CLOSING;
    c->event_loop_info = MHD_EVENT_LOOP_INFO_CLEANUP;

    return false;
  }

  mhd_assert (c->sk.state.rmt_shut_wr || \
              ! mhd_SOCKET_ERR_IS_HARD (c->sk.state.discnt_err));

  need_lingering = ! c->sk.state.rmt_shut_wr;
  if (need_lingering)
  {
#ifdef MHD_SUPPORT_HTTPS
    if (mhd_C_HAS_TLS (c))
    {
      if ((0 != (((unsigned int) c->sk.ready)
                 & mhd_SOCKET_NET_STATE_SEND_READY))
          || c->sk.props.is_nonblck)
        need_lingering =
          (mhd_TLS_PROCED_FAILED != mhd_tls_conn_shutdown (c->tls));
    }
    else
#endif /* MHD_SUPPORT_HTTPS */
    if (1)
    {
      need_lingering = mhd_socket_shut_wr (c->sk.fd);
      if (need_lingering)
        need_lingering = (! c->sk.state.rmt_shut_wr); /* Skip as already closed */
    }
  }

  return need_lingering;
}


#ifdef MHD_SUPPORT_HTTP2

static MHD_FN_PAR_NONNULL_ALL_ void
conn_h2_start_closing (struct MHD_Connection *restrict c,
                       bool close_hard)
{
  mhd_assert (mhd_C_IS_HTTP2 (c));
  mhd_assert (c->h2.dbg.h2_deinited);
  mhd_assert (! c->rq.app_aware);

  conn_start_socket_closing (c,
                             close_hard);

  mhd_conn_deinit_activity_timeout (c);

#ifndef NDEBUG
  c->dbg.closing_started = true;
#endif
}


#endif /* MHD_SUPPORT_HTTP2 */


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) MHD_FN_PAR_CSTR_ (3) void
mhd_conn_start_closing (struct MHD_Connection *restrict c,
                        enum mhd_ConnCloseReason reason,
                        const char *log_msg)
{
  bool close_hard;
  enum MHD_RequestEndedCode end_code;
  enum MHD_StatusCode sc;
  bool reply_sending_aborted;

#ifdef MHD_USE_TRACE_CONN_ADD_CLOSE
  fprintf (stderr,
           "&&& mhd_conn_start_closing([FD: %2llu], %u, %s%s%s)...\n",
           (unsigned long long) c->sk.fd,
           (unsigned int) reason,
           log_msg ? "\"" : "",
           log_msg ? log_msg : "[NULL]",
           log_msg ? "\"" : "");
#endif /* MHD_USE_TRACE_CONN_ADD_CLOSE */

#ifdef MHD_SUPPORT_HTTP2
  if (mhd_C_IS_HTTP2 (c))
  {
    mhd_assert ((mhd_CONN_CLOSE_TIMEDOUT == reason) ||
                (mhd_CONN_CLOSE_DAEMON_SHUTDOWN == reason) ||
                (mhd_CONN_CLOSE_H2_CLOSE_SOFT == reason) ||
                (mhd_CONN_CLOSE_H2_CLOSE_HARD == reason));
    mhd_assert (NULL == log_msg);
    conn_h2_start_closing (c,
                           reason != mhd_CONN_CLOSE_H2_CLOSE_SOFT);
    return;
  }
#endif /* MHD_SUPPORT_HTTP2 */

  reply_sending_aborted =
    ((mhd_HTTP_STAGE_HEADERS_SENDING <= c->stage)
     && (mhd_HTTP_STAGE_FULL_REPLY_SENT > c->stage));
  sc = MHD_SC_INTERNAL_ERROR;
  switch (reason)
  {
  case mhd_CONN_CLOSE_CLIENT_HTTP_ERR_ABORT_CONN:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_HTTP_PROTOCOL_ERROR;
    sc = MHD_SC_REQ_MALFORMED;
    mhd_assert (! reply_sending_aborted);
    break;
  case mhd_CONN_CLOSE_NO_POOL_MEM_FOR_REQUEST:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_NO_RESOURCES;
    mhd_assert (! reply_sending_aborted);
    break;
  case mhd_CONN_CLOSE_CLIENT_SHUTDOWN_EARLY:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_CLIENT_ABORT;
    sc = MHD_SC_CLIENT_SHUTDOWN_EARLY;
    mhd_assert (! reply_sending_aborted);
    break;
  case mhd_CONN_CLOSE_H2_PREFACE_MISSING:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_HTTP_PROTOCOL_ERROR;
    sc = MHD_SC_ALPN_H2_NO_PREFACE;
    break;
  case mhd_CONN_CLOSE_NO_POOL_MEM_FOR_REPLY:
    close_hard = true;
    end_code = (! c->stop_with_error || c->rq.too_large) ?
               MHD_REQUEST_ENDED_NO_RESOURCES :
               MHD_REQUEST_ENDED_HTTP_PROTOCOL_ERROR;
    sc = MHD_SC_REPLY_POOL_ALLOCATION_FAILURE;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted due to insufficient memory " \
                             "in the connection pool");
    break;
  case mhd_CONN_CLOSE_NO_MEM_FOR_ERR_RESPONSE:
    close_hard = true;
    end_code = c->rq.too_large ?
               MHD_REQUEST_ENDED_NO_RESOURCES :
               MHD_REQUEST_ENDED_HTTP_PROTOCOL_ERROR;
    sc = MHD_SC_ERR_RESPONSE_ALLOCATION_FAILURE;
    break;
  case mhd_CONN_CLOSE_APP_ERROR:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_BY_APP_ERROR;
    sc = MHD_SC_APPLICATION_DATA_GENERATION_FAILURE_CLOSED;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted due to application reply " \
                             "generation failure");
    break;
  case mhd_CONN_CLOSE_APP_ABORTED:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_BY_APP_ABORT;
    sc = MHD_SC_APPLICATION_CALLBACK_ABORT_ACTION;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Application aborted reply sending");
    break;
  case mhd_CONN_CLOSE_FILE_OFFSET_TOO_LARGE:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_FILE_ERROR;
    sc = MHD_SC_REPLY_FILE_OFFSET_TOO_LARGE;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted because OS failed " \
                             "to read too large response file");
    break;
  case mhd_CONN_CLOSE_FILE_READ_ERROR:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_FILE_ERROR;
    sc = MHD_SC_REPLY_FILE_READ_ERROR;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted because OS failed " \
                             "to read response file");
    break;
  case mhd_CONN_CLOSE_FILE_TOO_SHORT:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_BY_APP_ERROR;
    sc = MHD_SC_REPLY_FILE_TOO_SHORT;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted because response file is "
                             "shorter that expected");
    break;
#ifdef MHD_SUPPORT_AUTH_DIGEST
  case mhd_CONN_CLOSE_NONCE_ERROR:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_NONCE_ERROR;
    sc = MHD_SC_REPLY_NONCE_ERROR;
    mhd_assert (! reply_sending_aborted);
    break;
#endif /* MHD_SUPPORT_AUTH_DIGEST */

  case mhd_CONN_CLOSE_INT_ERROR:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_NO_RESOURCES;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted due to MHD internal error");
    break;
  case mhd_CONN_CLOSE_EXTR_EVENT_REG_FAILED:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_BY_EXT_EVENT_ERROR;
    sc = MHD_SC_EXTR_EVENT_REG_FAILED;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted due to external event " \
                             "registration failure");
    break;
  case mhd_CONN_CLOSE_NO_SYS_RESOURCES:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_NO_RESOURCES;
    sc = MHD_SC_NO_SYS_RESOURCES;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted due to lack of " \
                             "system resources");
    break;
  case mhd_CONN_CLOSE_SOCKET_ERR:
    close_hard = true;
    switch (c->sk.state.discnt_err)
    {
    case mhd_SOCKET_ERR_NOMEM:
      end_code = MHD_REQUEST_ENDED_NO_RESOURCES;
      sc = MHD_SC_NO_SYS_RESOURCES;
      if (reply_sending_aborted && (NULL == log_msg))
        log_msg = mhd_MSG4LOG ("Response aborted because system closed " \
                               "socket due to lack of system resources");
      break;
    case mhd_SOCKET_ERR_REMT_DISCONN:
      close_hard = false;
      end_code = (mhd_HTTP_STAGE_INIT == c->stage) ?
                 MHD_REQUEST_ENDED_COMPLETED_OK /* Not used */
                 : MHD_REQUEST_ENDED_CLIENT_ABORT;
      if (reply_sending_aborted)
      {
        sc = MHD_SC_CLIENT_CLOSED_CONN_EARLY;
        if (NULL == log_msg)
          log_msg = mhd_MSG4LOG ("Response aborted because remote client " \
                                 "closed connection early");
      }
      break;
    case mhd_SOCKET_ERR_CONNRESET:
      end_code = MHD_REQUEST_ENDED_CLIENT_ABORT;
      sc = MHD_SC_CONNECTION_RESET;
      if (reply_sending_aborted && (NULL == log_msg))
        log_msg = mhd_MSG4LOG ("Response aborted due to aborted connection");
      break;
    case mhd_SOCKET_ERR_CONN_BROKEN:
    case mhd_SOCKET_ERR_NOTCONN:
    case mhd_SOCKET_ERR_TLS:
    case mhd_SOCKET_ERR_PIPE:
    case mhd_SOCKET_ERR_NOT_CHECKED:
    case mhd_SOCKET_ERR_BADF:
    case mhd_SOCKET_ERR_INVAL:
    case mhd_SOCKET_ERR_OPNOTSUPP:
    case mhd_SOCKET_ERR_NOTSOCK:
    case mhd_SOCKET_ERR_OTHER:
    case mhd_SOCKET_ERR_INTERNAL:
    case mhd_SOCKET_ERR_NO_ERROR:
      end_code = MHD_REQUEST_ENDED_CONNECTION_ERROR;
      sc = MHD_SC_CONNECTION_BROKEN;
      if (reply_sending_aborted && (NULL == log_msg))
        log_msg = mhd_MSG4LOG ("Response aborted due to broken connection");
      break;
    case mhd_SOCKET_ERR_AGAIN:
    case mhd_SOCKET_ERR_INTR:
    default:
      mhd_UNREACHABLE ();
      break;
    }
    break;
  case mhd_CONN_CLOSE_DAEMON_SHUTDOWN:
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_DAEMON_SHUTDOWN;
    break;

  case mhd_CONN_CLOSE_TIMEDOUT:
    if (mhd_HTTP_STAGE_INIT == c->stage)
    {
      close_hard = false;
      end_code = MHD_REQUEST_ENDED_COMPLETED_OK; /* Not used */
      break;
    }
    close_hard = true;
    end_code = MHD_REQUEST_ENDED_TIMEOUT_REACHED;
    sc = MHD_SC_CONNECTION_TIMEOUT;
    if (reply_sending_aborted && (NULL == log_msg))
      log_msg = mhd_MSG4LOG ("Response aborted due to sending timeout");
    break;

  case mhd_CONN_CLOSE_ERR_REPLY_SENT:
    close_hard = false;
    end_code = c->rq.too_large ?
               MHD_REQUEST_ENDED_NO_RESOURCES :
               MHD_REQUEST_ENDED_HTTP_PROTOCOL_ERROR;
    break;
#ifdef MHD_SUPPORT_UPGRADE
  case mhd_CONN_CLOSE_UPGRADE:
    close_hard = false;
    end_code = MHD_REQUEST_ENDED_COMPLETED_OK_UPGRADE;
    break;
#endif /* MHD_SUPPORT_UPGRADE */
  case mhd_CONN_CLOSE_HTTP_COMPLETED:
    close_hard = false;
    end_code = MHD_REQUEST_ENDED_COMPLETED_OK;
    break;

#ifdef MHD_SUPPORT_HTTP2
  case mhd_CONN_CLOSE_H2_CLOSE_SOFT:
  case mhd_CONN_CLOSE_H2_CLOSE_HARD:
#endif /* MHD_SUPPORT_HTTP2 */
  default:
    mhd_assert (0 && "Unreachable code");
    mhd_UNREACHABLE ();
    end_code = MHD_REQUEST_ENDED_COMPLETED_OK;
    close_hard = false;
    break;
  }

  mhd_assert ((NULL == log_msg) || (MHD_SC_INTERNAL_ERROR != sc));

#ifdef MHD_SUPPORT_UPGRADE
  if (mhd_CONN_CLOSE_UPGRADE == reason)
  {
    mhd_assert (mhd_HTTP_STAGE_UPGRADING == c->stage);
    c->event_loop_info = MHD_EVENT_LOOP_INFO_UPGRADED;
  }
  else
#endif /* MHD_SUPPORT_UPGRADE */
  if (1)
  {
    if (conn_start_socket_closing (c,
                                   close_hard))
    {
      (void) 0; // TODO: start local lingering phase
      c->stage = mhd_HTTP_STAGE_PRE_CLOSING; // TODO: start local lingering phase
      c->event_loop_info = MHD_EVENT_LOOP_INFO_CLEANUP; // TODO: start local lingering phase
    }
    else
    {  /* No need / not possible to linger */
      c->stage = mhd_HTTP_STAGE_PRE_CLOSING;
      c->event_loop_info = MHD_EVENT_LOOP_INFO_CLEANUP;
    }
  }

#ifdef MHD_SUPPORT_LOG_FUNCTIONALITY
  if (NULL != log_msg)
  {
    mhd_LOG_MSG (c->daemon, sc, log_msg);
  }
#else  /* ! MHD_SUPPORT_LOG_FUNCTIONALITY */
  (void) log_msg; /* Mute compiler warning */
  (void) sc;      /* Mute compiler warning */
#endif /* ! MHD_SUPPORT_LOG_FUNCTIONALITY */

#if 0 // TODO: notification callback
  mhd_assert ((mhd_HTTP_STAGE_INIT != c->stage) || (! c->rq.app_aware));
  if ( (NULL != d->notify_completed) &&
       (c->rq.app_aware) )
    d->notify_completed (d->notify_completed_cls,
                         c,
                         &c->rq.app_context,
                         MHD_REQUEST_ENDED_COMPLETED_OK);
#else
  (void) end_code;
#endif
  c->rq.app_aware = false;

  if (! c->suspended)
  {
    mhd_assert (! c->resuming);
    mhd_conn_deinit_activity_timeout (c);
  }

#ifndef NDEBUG
  c->dbg.closing_started = true;
#endif
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) void
mhd_conn_pre_clean_part1 (struct MHD_Connection *restrict c)
{
  // TODO: support suspended connections
  mhd_conn_mark_unready (c, c->daemon);

  mhd_stream_call_dcc_cleanup_if_needed (c);
  if (NULL != c->rq.cntn.lbuf.data)
    mhd_daemon_free_lbuf (c->daemon, &(c->rq.cntn.lbuf));

  if (mhd_WM_INT_HAS_EXT_EVENTS (c->daemon->wmode_int))
  {
    struct MHD_Daemon *const d = c->daemon;
    if (NULL != c->events.extrn.app_cntx)
    {
      c->events.extrn.app_cntx =
        mhd_daemon_extr_event_reg (d,
                                   c->sk.fd,
                                   MHD_FD_STATE_NONE,
                                   c->events.extrn.app_cntx,
                                   (struct MHD_EventUpdateContext *) c);
      if (NULL != c->events.extrn.app_cntx)
        mhd_log_extr_event_dereg_failed (d);
    }
  }
#ifdef MHD_SUPPORT_EPOLL
  else if (mhd_POLL_TYPE_EPOLL == c->daemon->events.poll_type)
  {
    struct epoll_event event;

    event.events = 0;
    event.data.ptr = NULL;
    if (0 != epoll_ctl (c->daemon->events.data.epoll.e_fd,
                        EPOLL_CTL_DEL,
                        c->sk.fd,
                        &event))
    {
      mhd_LOG_MSG (c->daemon, MHD_SC_EVENTS_CONN_REMOVE_FAILED,
                   "Failed to remove connection socket from epoll.");
    }
  }
#endif /* MHD_SUPPORT_EPOLL */
#ifdef MHD_SUPPORT_KQUEUE
  else if (mhd_D_IS_USING_KQUEUE (c->daemon))
  {
#  ifdef MHD_SUPPORT_UPGRADE
    /* Remove socket from kqueue monitoring only if upgrading.
       If connection is being closed, the socket is removed automatically
       when the socket is closed. */
    if (mhd_HTTP_STAGE_UPGRADING == c->stage)
    {
      static const struct timespec zero_timeout = {0, 0};
      struct kevent events[2];
      int res;

      mhd_KE_SET (events + 0u,
                  c->sk.fd,
                  EVFILT_WRITE,
                  EV_DELETE,
                  c);
      mhd_KE_SET (events + 1u,
                  c->sk.fd,
                  EVFILT_READ,
                  EV_DELETE,
                  c);

#    ifdef MHD_USE_TRACE_POLLING_FDS
      fprintf (stderr,
               "### (Starting) kevent(%d, changes, 2, [NULL], "
               "0, [0, 0])...\n",
               c->daemon->events.data.kq.kq_fd);
#    endif /* MHD_USE_TRACE_POLLING_FDS */
      res = mhd_kevent (c->daemon->events.data.kq.kq_fd,
                        events,
                        2,
                        NULL,
                        0,
                        &zero_timeout);
#    ifdef MHD_USE_TRACE_POLLING_FDS
      fprintf (stderr,
               "### (Finished) kevent(%d, changes, 2, [NULL], "
               "0, [0, 0]) -> %d\n",
               c->daemon->events.data.kq.kq_fd,
               res);
#    endif /* MHD_USE_TRACE_POLLING_FDS */
      if (0 > res)
      {
        mhd_LOG_MSG (c->daemon, MHD_SC_EVENTS_CONN_REMOVE_FAILED,
                     "Failed to remove upgraded connection socket "
                     "from kqueue monitoring.");
        /* Continue with monitored socket which may wake-up
           daemon's monitoring */
      }
    }
#  endif /* MHD_SUPPORT_UPGRADE */
    (void) 0;
  }
#endif /* MHD_SUPPORT_KQUEUE */
}


MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1) void
mhd_conn_pre_clean (struct MHD_Connection *restrict c)
{
#ifdef MHD_USE_TRACE_CONN_ADD_CLOSE
  fprintf (stderr,
           "&&&    Closing connection, FD: %2llu\n",
           (unsigned long long) c->sk.fd);
#endif /* MHD_USE_TRACE_CONN_ADD_CLOSE */

  mhd_assert (c->dbg.closing_started);
  mhd_assert (! c->dbg.pre_cleaned);

#ifdef MHD_SUPPORT_UPGRADE
  if (NULL == c->upgr.c)
#endif
  mhd_conn_pre_clean_part1 (c);

  if (NULL != c->rp.resp_iov.iov)
  {
    free (c->rp.resp_iov.iov);
    c->rp.resp_iov.iov = NULL;
  }
  if (NULL != c->rp.response)
    mhd_response_dec_use_count (c->rp.response);
  c->rp.response = NULL;

  mhd_assert (NULL != c->pool);
  c->read_buffer_offset = 0;
  c->read_buffer_size = 0;
  c->read_buffer = NULL;
  c->write_buffer_send_offset = 0;
  c->write_buffer_append_offset = 0;
  c->write_buffer_size = 0;
  c->write_buffer = NULL;
  // TODO: call in the thread where it was allocated for thread-per-connection
  mhd_pool_destroy (c->pool);
  c->pool = NULL;

  c->stage = mhd_HTTP_STAGE_CLOSED;
#ifndef NDEBUG
  c->dbg.pre_cleaned = true;
#endif
}
