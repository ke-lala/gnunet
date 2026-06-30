/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2024 Evgeny Grin (Karlson2k) & Christian Grothoff

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
 * @file src/mhd2/upgraded_net.c
 * @brief  The implementation of functions for network data exchange
 *         for HTTP Upgraded connections
 * @author Karlson2k (Evgeny Grin)
 * @author Christian Grothoff
 */

#include "mhd_sys_options.h"

#include "sys_bool_type.h"
#include "sys_base_types.h"

#include "mhd_assert.h"

#include "sys_poll.h"
#ifndef MHD_SUPPORT_POLL
#  include "sys_select.h"
#endif
#include "mhd_limits.h"

#include "mhd_sockets_macros.h"

#include "mhd_upgrade.h"
#include "mhd_connection.h"
#include "mhd_locks.h"

#include "mhd_recv.h"
#include "mhd_send.h"
#include "mhd_mono_clock.h"

#include <string.h>

#include "mhd_public_api.h"


#if ! defined (MHD_SUPPORT_POLL) && \
  (defined(MHD_SOCKETS_KIND_POSIX) || ! defined(MHD_SUPPORT_SELECT))
#  if defined(_WIN32) || defined(HAVE_NANOSLEEP) || defined(HAVE_USLEEP)
#    define mhd_HAVE_MHD_SLEEP 1

/**
 * Pause execution for specified number of milliseconds.
 *
 * @param millisec the number of milliseconds to sleep
 */
static void
mhd_sleep (uint_fast32_t millisec)
{
#if defined(_WIN32)
  Sleep (millisec);
#elif defined(HAVE_NANOSLEEP)
  struct timespec slp = { (time_t) (millisec / 1000),
                          (long) ((millisec % 1000) * 1000000l)};
  struct timespec rmn;
  int num_retries = 0;
  while (0 != nanosleep (&slp, &rmn))
  {
    if (EINTR != errno)
      break;
    if (num_retries++ > 8)
      break;
    slp = rmn;
  }
#elif defined(HAVE_USLEEP)
  uint64_t us = millisec * 1000;
  do
  {
    uint64_t this_sleep;
    if (999999 < us)
      this_sleep = 999999;
    else
      this_sleep = us;
    /* Ignore return value as it could be void */
    usleep (this_sleep);
    us -= this_sleep;
  } while (us > 0);
#endif
}


#endif /* _WIN32 || HAVE_NANOSLEEP || HAVE_USLEEP */
#endif /* ! MHD_SUPPORT_POLL &&
          (MHD_SOCKETS_KIND_POSIX || ! MHD_SUPPORT_SELECT) */


MHD_EXTERN_
MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_OUT_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (4) enum MHD_StatusCode
MHD_upgraded_recv (struct MHD_UpgradedHandle *MHD_RESTRICT urh,
                   size_t recv_buf_size,
                   void *MHD_RESTRICT recv_buf,
                   size_t *MHD_RESTRICT received_size,
                   uint_fast64_t max_wait_millisec)
{
  struct MHD_Connection *restrict c = urh->c;
#if defined(MHD_SUPPORT_POLL) || defined(MHD_SUPPORT_SELECT)
  const MHD_Socket socket_fd = c->sk.fd;
#endif /* MHD_SUPPORT_POLL || MHD_SUPPORT_SELECT */
  char *restrict buf_char = (char *) recv_buf;
  size_t last_block_size;
  enum mhd_SocketError res;

  *received_size = 0;

  if (&(c->upgr) != urh)
    return MHD_SC_UPGRADED_HANDLE_INVALID;
  if (mhd_HTTP_STAGE_UPGRADED != c->stage)
    return MHD_SC_UPGRADED_HANDLE_INVALID;

  if (0 == recv_buf_size)
    return MHD_SC_OK;

  if (NULL != c->read_buffer)
  {
    mhd_mutex_lock_chk (&(urh->lock));
    if (0 != c->read_buffer_offset) /* Re-check under the lock */
    {
      if (recv_buf_size < c->read_buffer_offset)
      {
        memcpy (buf_char, c->read_buffer, recv_buf_size);
        last_block_size = recv_buf_size;
        c->read_buffer += recv_buf_size;
        c->read_buffer_offset -= recv_buf_size;
        c->read_buffer_size -= recv_buf_size;
      }
      else
      {
        /* recv_buf_size >= c->read_buffer_offset */
        memcpy (buf_char, c->read_buffer, c->read_buffer_offset);
        last_block_size = c->read_buffer_offset;
        c->read_buffer_offset = 0;
        c->read_buffer_size = 0;
        /* Do not deallocate the read buffer to save the time under the lock.
           The connection memory pool will not be used anyway. */
        c->read_buffer = NULL;
      }
    }
    else
      last_block_size = 0;
    mhd_mutex_unlock_chk (&(urh->lock));
    *received_size = last_block_size;
    if (recv_buf_size == last_block_size)
      return MHD_SC_OK;
  }

  last_block_size = 0;
  res = mhd_recv (c,
                  recv_buf_size - *received_size,
                  buf_char + *received_size,
                  &last_block_size);
  if (mhd_SOCKET_ERR_NO_ERROR == res)
  {
    if (0 == last_block_size)
      c->sk.state.rmt_shut_wr = true;
    *received_size += last_block_size;
    return MHD_SC_OK;
  }
  else if (0 != *received_size)
    return MHD_SC_OK;

  if (! mhd_SOCKET_ERR_IS_HARD (res))
  {
    while (0 != max_wait_millisec)
    {
#if defined(MHD_SUPPORT_POLL)
      if (1)
      {
        struct pollfd fds[1];
        int poll_wait;
        int poll_res;
        int wait_err;

        if (MHD_WAIT_INDEFINITELY <= max_wait_millisec)
          poll_wait = -1;
        else
        {
          poll_wait = (int) max_wait_millisec;
          if ((max_wait_millisec != (uint_fast64_t) poll_wait) ||
              (0 > poll_wait))
            poll_wait = INT_MAX;
        }
        fds[0].fd = socket_fd;
        fds[0].events = POLLIN;

        poll_res = mhd_poll (fds,
                             1,
                             poll_wait);
        if ((0 >= poll_res) &&
            (0 != *received_size))
          return MHD_SC_OK;
        else if (0 == poll_res)
          return MHD_SC_UPGRADED_NET_TIMEOUT;
        else if (0 > poll_res)
        {
          wait_err = mhd_SCKT_GET_LERR ();
          if (! mhd_SCKT_ERR_IS_EAGAIN (wait_err) &&
              ! mhd_SCKT_ERR_IS_EINTR (wait_err) &&
              ! mhd_SCKT_ERR_IS_LOW_RESOURCES (wait_err))
            return MHD_SC_UPGRADED_NET_HARD_ERROR;
        }
        max_wait_millisec = 0; /* Re-try only one time */
      }
#else /* ! MHD_SUPPORT_POLL */
#  if defined(MHD_SUPPORT_SELECT)
      bool use_select;
#    ifdef MHD_SOCKETS_KIND_POSIX
      use_select = (socket_fd < FD_SETSIZE);
#    else  /* MHD_SOCKETS_KIND_WINSOCK */
      use_select = true;
#    endif /* MHD_SOCKETS_KIND_WINSOCK */
      if (use_select)
      {
        fd_set rfds;
        int sel_res;
        int wait_err;
        struct timeval tmvl;

#    ifdef HAVE_TIME_T
        tmvl.tv_sec = (time_t) (max_wait_millisec / 1000);
#    else  /* ! HAVE_TIME_T */
        tmvl.tv_sec = (long) (max_wait_millisec / 1000);
#    endif /* ! HAVE_TIME_T */
        if ((max_wait_millisec / 1000 != (uint_fast64_t) tmvl.tv_sec) ||
            ((0 >= tmvl.tv_sec) && (0 != tmvl.tv_sec))) /* Avoid signed/unsigned warnings */
        {
          /* Do not bother figuring out the real maximum 'time_t' value.
             '0x7FFFFFFF' is large enough to be already unrealistic and should
             fit most of signed or unsigned time_t types. */
          tmvl.tv_sec = 0x7FFFFFFF;
          tmvl.tv_usec = 0;
        }
        else
        {
#    ifdef HAVE_SUSECONDS_T
          tmvl.tv_usec = (suseconds_t) ((max_wait_millisec % 1000) * 1000);
#    else  /* ! HAVE_SUSECONDS_T */
          tmvl.tv_usec = (long) ((max_wait_millisec % 1000) * 1000);
#    endif /* ! HAVE_SUSECONDS_T */
        }
        FD_ZERO (&rfds);
        FD_SET (socket_fd, &rfds);

        sel_res = select ((int) (c->sk.fd + 1),
                          &rfds,
                          NULL,
                          NULL,
                          (MHD_WAIT_INDEFINITELY <= max_wait_millisec) ?
                          NULL : &tmvl);

        if ((0 >= sel_res) &&
            (0 != *received_size))
          return MHD_SC_OK;
        else if (0 == sel_res)
          return MHD_SC_UPGRADED_NET_TIMEOUT;
        else if (0 > sel_res)
        {
          wait_err = mhd_SCKT_GET_LERR ();
          if (! mhd_SCKT_ERR_IS_EAGAIN (wait_err) &&
              ! mhd_SCKT_ERR_IS_EINTR (wait_err) &&
              ! mhd_SCKT_ERR_IS_LOW_RESOURCES (wait_err))
            return MHD_SC_UPGRADED_NET_HARD_ERROR;
        }
        max_wait_millisec = 0; /* Re-try only one time */
      }
      else /* combined with the next 'if()' */
#  endif /* MHD_SUPPORT_SELECT */
      if (1)
      {
#  ifndef mhd_HAVE_MHD_SLEEP
        return MHD_SC_UPGRADED_WAITING_NOT_SUPPORTED;
#  else  /* mhd_HAVE_MHD_SLEEP */
        uint_fast32_t wait_millisec = (uint_fast32_t) max_wait_millisec;

        if ((wait_millisec != max_wait_millisec) ||
            (wait_millisec > 100))
          wait_millisec = 100;
        mhd_sleep (wait_millisec);
        if (MHD_WAIT_INDEFINITELY > max_wait_millisec)
          max_wait_millisec -= wait_millisec;
#  endif /* mhd_HAVE_MHD_SLEEP */
      }
#endif /* ! MHD_SUPPORT_POLL */
      last_block_size = 0;
      res = mhd_recv (c,
                      recv_buf_size - *received_size,
                      buf_char + *received_size,
                      &last_block_size);
      if (mhd_SOCKET_ERR_NO_ERROR == res)
      {
        if (0 == last_block_size)
          c->sk.state.rmt_shut_wr = true;
        *received_size += last_block_size;
        return MHD_SC_OK;
      }
    }
  }
  if (! mhd_SOCKET_ERR_IS_HARD (res))
    return MHD_SC_UPGRADED_NET_TIMEOUT;
  if (mhd_SOCKET_ERR_REMT_DISCONN == res)
    return MHD_SC_UPGRADED_NET_CONN_CLOSED;
  if (mhd_SOCKET_ERR_TLS == res)
    return MHD_SC_UPGRADED_TLS_ERROR;
  if (! mhd_SOCKET_ERR_IS_BAD (res))
    return MHD_SC_UPGRADED_NET_CONN_BROKEN;

  return MHD_SC_UPGRADED_NET_HARD_ERROR;
}


MHD_EXTERN_
MHD_FN_PAR_NONNULL_ALL_
MHD_FN_PAR_IN_SIZE_ (3,2)
MHD_FN_PAR_OUT_ (4) enum MHD_StatusCode
MHD_upgraded_send (struct MHD_UpgradedHandle *MHD_RESTRICT urh,
                   size_t send_buf_size,
                   const void *MHD_RESTRICT send_buf,
                   size_t *MHD_RESTRICT sent_size,
                   uint_fast64_t max_wait_millisec,
                   enum MHD_Bool more_data_to_come)
{
  struct MHD_Connection *restrict c = urh->c;
#if defined(MHD_SUPPORT_POLL) || defined(MHD_SUPPORT_SELECT)
  const MHD_Socket socket_fd = c->sk.fd;
#endif /* MHD_SUPPORT_POLL || MHD_SUPPORT_SELECT */
  const char *restrict buf_char = (const char *) send_buf;
  const bool push_data = (MHD_NO == more_data_to_come);
  bool finish_time_set;
  bool wait_indefinitely;
  uint_fast64_t finish_time = 0;

  *sent_size = 0;

  if (&(c->upgr) != urh)
    return MHD_SC_UPGRADED_HANDLE_INVALID;
  if (mhd_HTTP_STAGE_UPGRADED != c->stage)
    return MHD_SC_UPGRADED_HANDLE_INVALID;

  finish_time_set = false;
  wait_indefinitely = (MHD_WAIT_INDEFINITELY <= max_wait_millisec);

  while (1)
  {
    enum mhd_SocketError res;
    size_t last_block_size;
    uint_fast64_t wait_left;
#if ! defined(MHD_SUPPORT_POLL) && defined(MHD_SUPPORT_SELECT)
    bool use_select;
#endif /* ! MHD_SUPPORT_POLL */

    last_block_size = 0;
    res = mhd_send_data (c,
                         send_buf_size - *sent_size,
                         buf_char + *sent_size,
                         push_data,
                         &last_block_size);
    if (mhd_SOCKET_ERR_NO_ERROR == res)
    {
      *sent_size += last_block_size;
      if (send_buf_size == *sent_size)
        break;
    }
    else if (mhd_SOCKET_ERR_IS_HARD (res))
    {
      if (0 != *sent_size)
        break;

      if (mhd_SOCKET_ERR_REMT_DISCONN == res)
        return MHD_SC_UPGRADED_NET_CONN_CLOSED;
      if (mhd_SOCKET_ERR_TLS == res)
        return MHD_SC_UPGRADED_TLS_ERROR;
      if (! mhd_SOCKET_ERR_IS_BAD (res))
        return MHD_SC_UPGRADED_NET_CONN_BROKEN;

      return MHD_SC_UPGRADED_NET_HARD_ERROR;
    }

    if (0 == max_wait_millisec)
    {
      mhd_assert (0 == *sent_size);

      return MHD_SC_UPGRADED_NET_TIMEOUT;
    }

    if (! wait_indefinitely)
    {
      uint_fast64_t cur_time;
      cur_time = mhd_monotonic_msec_counter ();

      if (! finish_time_set)
      {
        finish_time = cur_time + max_wait_millisec;
        wait_left = max_wait_millisec;
      }
      else
      {
        wait_left = finish_time - cur_time;
        if ((wait_left > cur_time - finish_time) ||
            (0 == wait_left))
          return MHD_SC_UPGRADED_NET_TIMEOUT;
      }
    }
    else
      wait_left = MHD_WAIT_INDEFINITELY; /* Mute compiler warning */

#if defined(MHD_SUPPORT_POLL)
    if (1)
    {
      struct pollfd fds[1];
      int poll_wait;
      int poll_res;
      int wait_err;

      if (wait_indefinitely)
        poll_wait = -1;
      else
      {
        poll_wait = (int) wait_left;
        if ((wait_left != (uint_fast64_t) poll_wait) ||
            (0 > poll_wait))
          poll_wait = INT_MAX;
      }
      fds[0].fd = socket_fd;
      fds[0].events = POLLOUT;

      poll_res = mhd_poll (fds,
                           1,
                           poll_wait);
      if (0 < poll_res)
        continue;
      if (0 == poll_res)
      {
        if (wait_indefinitely ||
            (INT_MAX == poll_wait))
          continue;
        if (0 != *sent_size)
          return MHD_SC_OK;
        return MHD_SC_UPGRADED_NET_TIMEOUT;
      }

      mhd_assert (0 > poll_res);
      wait_err = mhd_SCKT_GET_LERR ();
      if (! mhd_SCKT_ERR_IS_EAGAIN (wait_err) &&
          ! mhd_SCKT_ERR_IS_EINTR (wait_err) &&
          ! mhd_SCKT_ERR_IS_LOW_RESOURCES (wait_err))
        return MHD_SC_UPGRADED_NET_HARD_ERROR;
    }
#else /* ! MHD_SUPPORT_POLL */
#  if defined(MHD_SUPPORT_SELECT)
#    ifdef MHD_SOCKETS_KIND_POSIX
    use_select = (socket_fd < FD_SETSIZE);
#    else  /* MHD_SOCKETS_KIND_WINSOCK */
    use_select = true;
#    endif /* MHD_SOCKETS_KIND_WINSOCK */
    if (use_select)
    {
      fd_set wfds;
      int sel_res;
      int wait_err;
      struct timeval tmvl;
      bool max_wait;

      max_wait = false;
      if (wait_indefinitely)
      {
        tmvl.tv_sec = 0;
        tmvl.tv_usec = 0;
      }
      else
      {
#    ifdef HAVE_TIME_T
        tmvl.tv_sec = (time_t) (max_wait_millisec / 1000);
#    else  /* ! HAVE_TIME_T */
        tmvl.tv_sec = (long) (max_wait_millisec / 1000);
#    endif /* ! HAVE_TIME_T */
        if ((max_wait_millisec / 1000 != (uint_fast64_t) tmvl.tv_sec) ||
            ((0 >= tmvl.tv_sec) && (0 != tmvl.tv_sec))) /* Avoid signed/unsigned warnings */
        {
          /* Do not bother figuring out the real maximum 'time_t' value.
             '0x7FFFFFFF' is large enough to be already unrealistic and should
             fit most of signed or unsigned time_t types. */
          tmvl.tv_sec = 0x7FFFFFFF;
          tmvl.tv_usec = 0;
          max_wait = true;
        }
        else
        {
#    ifdef HAVE_SUSECONDS_T
          tmvl.tv_usec = (suseconds_t) ((max_wait_millisec % 1000) * 1000);
#    else  /* ! HAVE_SUSECONDS_T */
          tmvl.tv_usec = (long) ((max_wait_millisec % 1000) * 1000);
#    endif /* ! HAVE_SUSECONDS_T */
        }
      }
      FD_ZERO (&wfds);
      FD_SET (socket_fd, &wfds);

      sel_res = select ((int) (c->sk.fd + 1),
                        NULL,
                        &wfds,
                        NULL,
                        wait_indefinitely ? NULL : &tmvl);

      if (0 < sel_res)
        continue;
      if (0 == sel_res)
      {
        if (wait_indefinitely ||
            max_wait)
          continue;
        if (0 != *sent_size)
          return MHD_SC_OK;
        return MHD_SC_UPGRADED_NET_TIMEOUT;
      }

      mhd_assert (0 > sel_res);
      wait_err = mhd_SCKT_GET_LERR ();
      if (! mhd_SCKT_ERR_IS_EAGAIN (wait_err) &&
          ! mhd_SCKT_ERR_IS_EINTR (wait_err) &&
          ! mhd_SCKT_ERR_IS_LOW_RESOURCES (wait_err))
        return MHD_SC_UPGRADED_NET_HARD_ERROR;
    }
    else /* combined with the next 'if()' */
#  endif /* MHD_SUPPORT_SELECT */
    if (1)
    {
#  ifndef mhd_HAVE_MHD_SLEEP
      return MHD_SC_UPGRADED_WAITING_NOT_SUPPORTED;
#  else  /* mhd_HAVE_MHD_SLEEP */
      uint_fast32_t wait_millisec = (uint_fast32_t) wait_left;

      if ((wait_millisec != wait_left) ||
          (wait_millisec > 100))
        wait_millisec = 100;
      mhd_sleep (wait_millisec);
#  endif /* mhd_HAVE_MHD_SLEEP */
    }
#endif /* ! MHD_SUPPORT_POLL */
  }

  return MHD_SC_OK;
}
