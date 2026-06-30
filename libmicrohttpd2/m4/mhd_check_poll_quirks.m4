# SPDX-License-Identifier: FSFAP
#
# SYNOPSIS
#
#   MHD_CHECK_POLL_QUIRKS
#
# DESCRIPTION
#
#   Check whether the system poll() (or WSAPoll()) function has any known
#   quirks.
#
#   If any such quirks are detected, corresponding preprocessor macros
#   are defined.
#
#   The configure.ac script must check for the presence of all headers
#   used by this macro before "invoking" it.
#
# LICENSE
#
#   Copyright (c) 2025 Karlson2k (Evgeny Grin) <k2k@drgrin.dev>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([MHD_CHECK_POLL_QUIRKS],[dnl
  AC_PREREQ([2.64])dnl
  AC_REQUIRE([AC_CANONICAL_HOST])dnl
  AC_REQUIRE([AC_PROG_CC])dnl
  AC_LANG_ASSERT([C])dnl
  AS_IF([test -z "$use_itc"],[AC_MSG_FAILURE([\$use_itc is not set])])
  AS_VAR_IF([have_poll],["yes"],[:],[AC_MSG_FAILURE([\$have_poll is not 'yes'])])
  AC_CACHE_CHECK([[whether poll() clobbers fds.events]],
    [[mhd_cv_poll_clobbers_events]], [dnl
      AC_RUN_IFELSE([
          AC_LANG_SOURCE([[

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#ifdef HAVE_SOCKLIB_H
#  include <sockLib.h>
#endif
#ifdef HAVE_INETLIB_H
#  include <inetLib.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_SYS_UN_H
#  include <sys/un.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif
#if defined(HAVE_NETDB_H)
#  include <netdb.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#  include <netinet/tcp.h>
#endif
#if !defined(_WIN32) || defined(__CYGWIN__)
#  include <poll.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  ifndef MHD_ITC_SOCKETPAIR_
#error Only socketpair ITC supported on native W32
fail test here %%%@<:@-1@:>@
#  endif
#endif
#include <stdio.h>

#if defined(MHD_ITC_EVENTFD_)
#  include <sys/eventfd.h>
#elif defined(MHD_ITC_PIPE_)
/* No Additional includes */
#elif defined(MHD_ITC_SOCKETPAIR_)
/* No Additional includes */
#else
#error No selected ITC type
fail test here %%%@<:@-1@:>@
#endif

#if defined(PF_INET)
#  define my_PF_INET PF_INET
#else
#  define my_PF_INET AF_INET
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#  define CHECK_USE_POSIX_SOCKETS 1
#  define my_sckt_type int
#  define MY_INVALID_SOCKET (-1)
#  define my_send3(s,b,l) send((s),(b),(l),0)
#  define my_fd_close(skt) close((skt))
#else
#  define CHECK_USE_WINSOCK_SOCKETS 1
#  define my_sckt_type SOCKET
#  define MY_INVALID_SOCKET INVALID_SOCKET
#  define my_send3(s,b,l) send((s),(const char *)(b),(l),0)
#  define my_fd_close(skt) closesocket((skt))
#endif

#if defined(MHD_ITC_SOCKETPAIR_)
#  ifdef CHECK_USE_POSIX_SOCKETS
#    if defined(AF_UNIX)
#      define my_socketpair(sk_arr) socketpair(AF_UNIX, SOCK_STREAM, 0, (sk_arr))
#    elif defined(AF_LOCAL)
#      define my_socketpair(sk_arr) socketpair(AF_LOCAL, SOCK_STREAM, 0, (sk_arr))
#    else
#      define my_socketpair(sk_arr) socketpair(AF_INET, SOCK_STREAM, 0, (sk_arr))
#    endif
#  else /* WinSock sockets */
static int my_socket_nonblocking (SOCKET sckt);

static int my_socketpair (my_sckt_type sckt[2])
{
  int i;

#define PAIR_MAX_TRIES 511
  for (i = 0; i < PAIR_MAX_TRIES; i++)
  {
    struct sockaddr_in listen_addr;
    my_sckt_type listen_s;
    static const socklen_t c_addinlen = sizeof(struct sockaddr_in);   /* Try to help compiler to optimise */
    socklen_t addr_len = c_addinlen;

    listen_s = socket (AF_INET,
                       SOCK_STREAM,
                       0);
    if (INVALID_SOCKET == listen_s)
      break;   /* can't create even single socket */

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = 0;   /* same as htons(0) */
    listen_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

    if ( ((0 == bind (listen_s,
                      (struct sockaddr *) &listen_addr,
                      c_addinlen)) &&
          (0 == listen (listen_s,
                        1) ) &&
          (0 == getsockname (listen_s,
                             (struct sockaddr *) &listen_addr,
                             &addr_len))) )
    {
      my_sckt_type client_s = socket (AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in accepted_from_addr;
      struct sockaddr_in client_addr;

      if (INVALID_SOCKET != client_s)
      {
        if (my_socket_nonblocking (client_s) &&
            ( (0 == connect (client_s,
                             (struct sockaddr *) &listen_addr,
                             c_addinlen)) ||
              (WSAEWOULDBLOCK == WSAGetLastError()) ))
        {
          my_sckt_type server_s;

          addr_len = c_addinlen;
          server_s = accept (listen_s,
                             (struct sockaddr *) &accepted_from_addr,
                             &addr_len);
          if (INVALID_SOCKET != server_s)
          {
            addr_len = c_addinlen;
            if ( (0 == getsockname (client_s,
                                    (struct sockaddr *) &client_addr,
                                    &addr_len)) &&
                 (accepted_from_addr.sin_port == client_addr.sin_port) &&
                 (accepted_from_addr.sin_addr.s_addr ==
                  client_addr.sin_addr.s_addr) )
            {
              closesocket (listen_s);
              return 0;
            }
            closesocket (server_s);
          }
        }
        closesocket (client_s);
      }
    }
    closesocket (listen_s);
  }

  sckt[0] = INVALID_SOCKET;
  sckt[1] = INVALID_SOCKET;

  return -1;
}

static int my_socket_nonblocking (SOCKET sckt)
{
  unsigned long set_flag = 1;

  if (0 == ioctlsocket (sckt, (long) FIONBIO, &set_flag))
    return !0;

  return 0;
}

#  endif
#endif

static int check_poll_fn(void)
{
  int res;
  int err_res = 0;
  struct pollfd fds[2];
#ifdef MHD_ITC_EVENTFD_
  my_sckt_type itc_fd;
#else
  my_sckt_type itc_pair[2];
#endif
  my_sckt_type listen_skt;

#if defined(MHD_ITC_EVENTFD_)
  itc_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (0 > itc_fd)
    err_res = 10;
  else
  {
    if (1) /* for local scope */
    {
      unsigned char val[8] = {1u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
      if (0 > write (itc_fd, (void *) val, 8)) /* "activate" ITC */
        (void) res; /* ignore result */
    }
    fds[0].fd = itc_fd;
#elif defined(MHD_ITC_PIPE_)
#  ifdef HAVE_PIPE2_FUNC
  res = pipe2(itc_pair, 0
#    ifdef O_CLOEXEC
                        | O_CLOEXEC
#    endif
#    ifdef O_NONBLOCK
                        | O_NONBLOCK
#    endif
              );
#  else
  res = pipe(itc_pair);
#  endif
  if (0 != res)
    err_res = 11;
  else
  {
    if (1) /* for local scope */
    {
      unsigned char val = 1u;
      if (0 > write (itc_pair[1], (void *) &val, 1)) /* "activate" ITC */
        (void) res; /* ignore result */
    }
    fds[0].fd = itc_pair[0];
#elif defined(MHD_ITC_SOCKETPAIR_)
  res = my_socketpair(itc_pair);
  if (0 != res)
    err_res = 12;
  else
  {
    if (1) /* for local scope */
    {
      unsigned char val = 1u;
      if (0 > my_send3(itc_pair[1], (void *) &val, 1)) /* "activate" ITC */
        (void) 0; /* ignore result */
    }
    fds[0].fd = itc_pair[0];
#endif
    fds[0].events = POLLIN;


    listen_skt = socket(my_PF_INET, SOCK_STREAM, 0);
    if (MY_INVALID_SOCKET == listen_skt)
      err_res = 13;
    else
    {
      if (1) /* for local scope */
      {
        struct sockaddr_in sk_addr;

        sk_addr.sin_family = AF_INET;
        sk_addr.sin_addr.s_addr = INADDR_ANY;
        sk_addr.sin_port = 0;
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
        sk_addr.sin_len = sizeof(sk_addr);
#endif

        res = bind(listen_skt, (struct sockaddr *) &sk_addr, sizeof(sk_addr));
        (void) res; /* ignore result */
      }

      res = listen(listen_skt, 8);
      (void) res; /* ignore result */

      fds[1].fd = listen_skt;
      fds[1].events = POLLIN;

#ifdef CHECK_USE_POSIX_SOCKETS
      res = poll(fds, 2, 0);
#else
      res = WSAPoll(fds, 2, 0);
#endif
      if (0 > res)
        err_res = 14;

      if (POLLIN != fds[0].events)
      {
        fprintf (stderr, "fds[0].events changed from 0x%X to 0x%X\n", (unsigned int) POLLIN, (unsigned int) fds[0].events);
        err_res = 97; /* Magic number */
      }
      if (POLLIN != fds[1].events)
      {
        fprintf (stderr, "fds[1].events changed from 0x%X to 0x%X\n", (unsigned int) POLLIN, (unsigned int) fds[1].events);
        err_res = 97; /* Magic number */
      }
      res = my_fd_close(listen_skt);
      (void) res; /* Ignore result */
    }
#if defined(MHD_ITC_EVENTFD_)
    res = my_fd_close(itc_fd);
    (void) res; /* Ignore result */
#else
    res = my_fd_close(itc_pair[1]);
    (void) res; /* Ignore result */
    res = my_fd_close(itc_pair[0]);
    (void) res; /* Ignore result */
#endif
  }
  return err_res;
}

int main(void)
{
  int err_res;
#ifdef CHECK_USE_WINSOCK_SOCKETS
  if (1) /* for local scope */
  {
    WSADATA wsa_d;
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsa_d))
      return 5;
  }
#endif
  err_res = check_poll_fn();
  if ((0 != err_res) && (97 != err_res))
    fprintf (stderr, "Failed to preform poll() checking.\n");

#ifdef MHD_SOCKETS_KIND_WINSOCK
  (void) WSACleanup();
#endif /* MHD_SOCKETS_KIND_WINSOCK */

  return err_res;
}

            ]]
          )
        ],
        [mhd_cv_poll_clobbers_events="no"],
        [
          test_res="$?"
          AS_IF([test "$test_res" -eq 0],[AC_MSG_FAILURE()],
            [test "$test_res" -eq 97],[mhd_cv_poll_clobbers_events="yes"] # Test magic number
            ,[mhd_cv_poll_clobbers_events="assuming yes"]
          )
          AS_UNSET([test_res])
        ],
        [
          AS_CASE(["$host_os"],
            [haiku],[mhd_cv_poll_clobbers_events="assuming yes"],
            [mhd_cv_poll_clobbers_events="guessing no"]
          )
        ]
      )
    ]
  )
  AS_CASE([$mhd_cv_poll_clobbers_events],
    [*yes],
    [AC_DEFINE([HAVE_POLL_CLOBBERS_EVENTS],
               [1],[Define to '1' is poll() change value of 'events' member of array pointed by 'fds'])
    ]
  )
])
