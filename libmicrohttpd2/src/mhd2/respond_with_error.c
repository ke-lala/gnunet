/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/respond_with_error.c
 * @brief  The implementation of error response functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"
#include "respond_with_error.h"

#include "sys_base_types.h"
#include "sys_null_macro.h"
#include "mhd_str_macros.h"

#include "mhd_assert.h"

#include "sys_malloc.h"

#include "mhd_connection.h"

#include "mempool_funcs.h"

#include "response_from.h"
#include "daemon_logger.h"
#include "response_destroy.h"
#include "stream_funcs.h"
#include "daemon_funcs.h"

#include "mhd_public_api.h"

MHD_INTERNAL
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_CSTR_ (4) MHD_FN_PAR_CSTR_ (6) void
respond_with_error_len (struct MHD_Connection *c,
                        unsigned int http_code,
                        size_t msg_len,
                        const char *msg,
                        size_t add_hdr_line_len,
                        char *add_hdr_line)
{
  struct MHD_Response *err_res;

  mhd_assert (! c->stop_with_error); /* Do not send error twice */
  mhd_assert (mhd_HTTP_STAGE_REQ_RECV_FINISHED >= c->stage);

  /* Discard most of the request data */

  if (NULL != c->rq.cntn.lbuf.data)
    mhd_daemon_free_lbuf (c->daemon, &(c->rq.cntn.lbuf));
  c->rq.cntn.lbuf.data = NULL;

  c->write_buffer = NULL;
  c->write_buffer_size = 0;
  c->write_buffer_send_offset = 0;
  c->write_buffer_append_offset = 0;

  mhd_DLINKEDL_INIT_LIST (&(c->rq), fields);
  c->rq.version = NULL;
  c->rq.method.len = 0;
  c->rq.method.cstr = NULL;
  c->rq.url = NULL;
  c->continue_message_write_offset = 0;
  if (0 != c->read_buffer_size)
  {
    mhd_pool_deallocate (c->pool,
                         c->read_buffer,
                         c->read_buffer_size);
    c->read_buffer = NULL;
    c->read_buffer_size = 0;
    c->read_buffer_offset = 0;
  }

  c->stop_with_error = true;
  c->discard_request = true;
  if ((MHD_HTTP_STATUS_CONTENT_TOO_LARGE == http_code) ||
      (MHD_HTTP_STATUS_URI_TOO_LONG == http_code) ||
      (MHD_HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE == http_code))
    c->rq.too_large = true;

  mhd_LOG_PRINT (c->daemon,
                 MHD_SC_REQ_PROCCESSING_ERR_REPLY,
                 mhd_LOG_FMT ("Error processing request. Sending %u " \
                              "error reply: %s"),
                 (unsigned int) http_code,
                 (NULL != msg) ? msg : "[EMPTY BODY]");

  if (NULL != c->rp.response)
  {
    mhd_response_dec_use_count (c->rp.response);
    c->rp.response = NULL;
  }
  err_res = mhd_response_special_for_error (http_code,
                                            msg_len,
                                            msg,
                                            add_hdr_line_len,
                                            add_hdr_line);
  if (NULL == err_res)
  {
    if (NULL != add_hdr_line)
      free (add_hdr_line);
    mhd_STREAM_ABORT (c, \
                      mhd_CONN_CLOSE_NO_MEM_FOR_ERR_RESPONSE, \
                      "No memory to create error response.");
    return;
  }
  c->rp.response = err_res;
  c->stage = mhd_HTTP_STAGE_START_REPLY;
}
