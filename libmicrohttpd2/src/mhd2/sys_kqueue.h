/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2026 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/sys_kqueue.h
 * @brief  The header for the system kqueue functions and related data types
 * @author Karlson2k (Evgeny Grin)
 *
 * This header includes system macros for kqueue and defines related
 * MHD macros.
 */

#ifndef MHD_SYS_KQUEUE_H
#define MHD_SYS_KQUEUE_H 1

#include "mhd_sys_options.h"

#ifdef MHD_SUPPORT_KQUEUE
#  include "mhd_socket_type.h"
#  ifndef MHD_SOCKETS_KIND_POSIX
#error Only POSIX type sockets are supported
#  endif
#  ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif /* HAVE_SYS_TYPES_H */

#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  endif

#  include <sys/event.h>

#  ifdef HAVE_KQUEUEX
#    ifdef KQUEUE_CLOEXEC
#      define mhd_kqueue()                  kqueuex (KQUEUE_CLOEXEC)
#      define mhd_KQUEUE_HAS_CLOEXEC_SET()  (! 0)
#    else
#      undef HAVE_KQUEUEX   /* No use for kqueuex() */
#    endif
#  endif

#  ifdef HAVE_KQUEUE1
#    ifdef mhd_kqueue
#      undef HAVE_KQUEUE1   /* No use for kqueue1() */
#    else
#      include <fcntl.h>
#      ifdef O_CLOEXEC
#        define mhd_kqueue()                kqueue1 (O_CLOEXEC)
#        define mhd_KQUEUE_HAS_CLOEXEC_SET()    (! 0)
#      else
#        undef HAVE_KQUEUE1
#      endif
#    endif
#  endif

#  ifndef mhd_kqueue
#    define mhd_kqueue()                    kqueue ()
#    define mhd_KQUEUE_HAS_CLOEXEC_SET()    (0)
#  endif

#  ifdef __NetBSD__
#    include <sys/param.h>
#    if __NetBSD_Version__ + 0 < 1000000000
#      define mhd_KE_UDATA_IS_INTPTR    1
#    endif
#  endif

#  ifndef mhd_KE_UDATA_IS_INTPTR
typedef void *mhd_KE_UDATA_TYPE;
#    define mhd_PTR_TO_KE_UDATA(ptr)    (ptr)
#    define mhd_KE_UDATA_TO_PTR(ud)     (ud)
#  else
typedef intptr_t mhd_KE_UDATA_TYPE;
#    define mhd_PTR_TO_KE_UDATA(ptr)    ((mhd_KE_UDATA_TYPE) (ptr))
#    define mhd_KE_UDATA_TO_PTR(ud)     ((void*) (ud))
#  endif

#  define mhd_KE_GET_UDATA(ev_ptr)  mhd_KE_UDATA_TO_PTR ((ev_ptr)->udata)

#  define mhd_KE_SET(ev_ptr,fd,evfltr,evflags,evudata_ptr) do { \
          struct kevent mhd__ke_tmp = {0u};                      \
          mhd__ke_tmp.ident = (unsigned int) (fd);                \
          mhd__ke_tmp.filter = (evfltr);                           \
          mhd__ke_tmp.flags = (evflags);                           \
          mhd__ke_tmp.udata = mhd_PTR_TO_KE_UDATA ((evudata_ptr)); \
          (*(ev_ptr)) = mhd__ke_tmp; } while (0)

#  ifdef EV_KEEPUDATA
#    define mhd_EV_KEEPUDATA_OR_ZERO    EV_KEEPUDATA
#  else
#    define mhd_EV_KEEPUDATA_OR_ZERO    (0)
#  endif

#  ifndef __NetBSD__
#    define mhd_kevent(kqfd,chlist,nchs,evlist,nevs,tmout) \
        kevent ((kqfd),(chlist),(nchs),(evlist),(nevs),(tmout))
#  else  /* ! __NetBSD__ */
#    define mhd_kevent(kqfd,chlist,nchs,evlist,nevs,tmout) \
        kevent ((kqfd),(chlist),(size_t) (nchs),(evlist),(size_t) (nevs), \
                (tmout))
#  endif

#endif /* MHD_SUPPORT_KQUEUE */

#endif /* ! MHD_SYS_KQUEUE_H */
