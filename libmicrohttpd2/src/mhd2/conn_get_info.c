/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/conn_get_info.c
 * @brief  The implementation of MHD_connection_get_info_*() functions
 * @author Karlson2k (Evgeny Grin)
 */

#include "mhd_sys_options.h"

#include "mhd_assert.h"
#include "mhd_unreachable.h"

#include "mhd_connection.h"

#include "daemon_funcs.h"
#ifdef MHD_SUPPORT_HTTPS
#  include "mhd_tls_funcs.h"
#  include "mhd_tls_ver_stct.h"
#endif

#include "mhd_public_api.h"

MHD_EXTERN_ MHD_FN_MUST_CHECK_RESULT_
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_OUT_ (3) enum MHD_StatusCode
MHD_connection_get_info_fixed_sz (
  struct MHD_Connection *MHD_RESTRICT connection,
  enum MHD_ConnectionInfoFixedType info_type,
  union MHD_ConnectionInfoFixedData *MHD_RESTRICT output_buf,
  size_t output_buf_size)
{
  switch (info_type)
  {
  case MHD_CONNECTION_INFO_FIXED_CLIENT_ADDRESS:
    if (NULL == connection->sk.addr.data)
    {
      if (mhd_T_IS_NOT_YES (connection->sk.props.is_nonip))
        return MHD_SC_INFO_GET_TYPE_UNOBTAINABLE;
      else
        return MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE;
    }
    mhd_assert (0 != connection->sk.addr.size);
    if (sizeof(output_buf->v_client_address_sa_info) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_client_address_sa_info.sa_size = connection->sk.addr.size;
    output_buf->v_client_address_sa_info.sa =
      (const struct sockaddr*) connection->sk.addr.data;
    return MHD_SC_OK;
  case MHD_CONNECTION_INFO_FIXED_CONNECTION_SOCKET:
    if (sizeof(output_buf->v_connection_socket) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    mhd_assert (MHD_INVALID_SOCKET != connection->sk.fd);
    output_buf->v_connection_socket = connection->sk.fd;
    return MHD_SC_OK;
  case MHD_CONNECTION_INFO_FIXED_DAEMON:
    if (sizeof(output_buf->v_daemon) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_daemon = mhd_daemon_get_master_daemon (connection->daemon);
    return MHD_SC_OK;
  case MHD_CONNECTION_INFO_FIXED_APP_CONTEXT:
    if (sizeof(output_buf->v_app_context_ppvoid) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_app_context_ppvoid = &(connection->socket_context);
    return MHD_SC_OK;

  case MHD_CONNECTION_INFO_FIXED_SENTINEL:
  default:
    break;
  }
  return MHD_SC_INFO_GET_TYPE_UNKNOWN;
}


MHD_EXTERN_ MHD_FN_MUST_CHECK_RESULT_
MHD_FN_PAR_NONNULL_ (1)
MHD_FN_PAR_NONNULL_ (3) MHD_FN_PAR_OUT_ (3) enum MHD_StatusCode
MHD_connection_get_info_dynamic_sz (
  struct MHD_Connection *MHD_RESTRICT connection,
  enum MHD_ConnectionInfoDynamicType info_type,
  union MHD_ConnectionInfoDynamicData *MHD_RESTRICT output_buf,
  size_t output_buf_size)
{
  switch (info_type)
  {
  case MHD_CONNECTION_INFO_DYNAMIC_HTTP_VER:
    if (mhd_HTTP_STAGE_REQ_LINE_RECEIVED > connection->stage)
      return MHD_SC_TOO_EARLY;
    if (sizeof(output_buf->v_http_ver) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_http_ver = connection->rq.http_ver;
    return MHD_SC_OK;
  case MHD_CONNECTION_INFO_DYNAMIC_CONNECTION_TIMEOUT_MILSEC:
    if (sizeof(output_buf->v_connection_timeout_uint32) <= output_buf_size)
    {
      output_buf->v_connection_timeout_uint32 =
        connection->timeout.milsec;
      return MHD_SC_OK;
    }
    return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
  case MHD_CONNECTION_INFO_DYNAMIC_CONNECTION_SUSPENDED:
    if (sizeof(output_buf->v_connection_suspended_bool) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    output_buf->v_connection_suspended_bool =
      connection->suspended ? MHD_YES : MHD_NO;
    return MHD_SC_OK;
  case MHD_CONNECTION_INFO_DYNAMIC_TLS_VER:
#ifdef MHD_SUPPORT_HTTPS
    if ((mhd_CONN_STATE_TCP_CONNECTED != connection->conn_state) &&
        (mhd_CONN_STATE_TLS_CONNECTED != connection->conn_state))
    {
      if (mhd_CONN_FLAG_CLOSING > connection->conn_state)
        return MHD_SC_TOO_EARLY;
      else
        return MHD_SC_TOO_LATE;
    }
#endif
    if (sizeof(output_buf->v_tls_ver) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
    if (! mhd_C_HAS_TLS (connection))
      output_buf->v_tls_ver = MHD_TLS_VERSION_NO_TLS;
    else
    {
#ifdef MHD_SUPPORT_HTTPS
      struct mhd_StctTlsVersion stct_tls_ver;
      if (! mhd_tls_conn_get_tls_ver (connection->tls, \
                                      &stct_tls_ver))
        return MHD_SC_INFO_GET_TYPE_UNOBTAINABLE;
      output_buf->v_tls_ver = stct_tls_ver.tls_ver;
#else  /* ! MHD_SUPPORT_HTTPS */
      mhd_UNREACHABLE ();
#endif /* ! MHD_SUPPORT_HTTPS */
    }
    return MHD_SC_OK;
  case MHD_CONNECTION_INFO_DYNAMIC_TLS_SESSION:
    if (! mhd_C_HAS_TLS (connection))
      return MHD_SC_INFO_GET_TYPE_NOT_APPLICABLE;
    if (sizeof(output_buf->v_tls_session) > output_buf_size)
      return MHD_SC_INFO_GET_BUFF_TOO_SMALL;
#ifndef MHD_SUPPORT_HTTPS
    mhd_UNREACHABLE ();
    return MHD_SC_INTERNAL_ERROR;
#else  /* MHD_SUPPORT_HTTPS */
    mhd_tls_conn_get_tls_sess (connection->tls, &(output_buf->v_tls_session));
#endif /* MHD_SUPPORT_HTTPS */
    break;
  case MHD_CONNECTION_INFO_DYNAMIC_SENTINEL:
  default:
    break;
  }
  return MHD_SC_INFO_GET_TYPE_UNKNOWN;
}
