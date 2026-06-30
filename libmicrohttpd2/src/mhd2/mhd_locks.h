/* SPDX-License-Identifier: LGPL-2.1-or-later OR (GPL-2.0-or-later WITH eCos-exception-2.0) */
/*
  This file is part of GNU libmicrohttpd.
  Copyright (C) 2016-2025 Evgeny Grin (Karlson2k)

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
 * @file src/mhd2/mhd_locks.h
 * @brief  Header for platform-independent locks abstraction
 * @author Karlson2k (Evgeny Grin)
 * @author Christian Grothoff
 *
 * Provides basic abstraction for locks/mutex.
 * Any functions can be implemented as macro on some platforms
 * unless explicitly marked otherwise.
 * Any function argument can be skipped in macro, so avoid
 * variable modification in function parameters.
 *
 * @warning Unlike pthread functions, most of functions return
 *          nonzero on success.
 */

#ifndef MHD_LOCKS_H
#define MHD_LOCKS_H 1

#include "mhd_sys_options.h"

#ifdef MHD_SUPPORT_THREADS

#if defined(HAVE_PTHREAD_H) && defined(mhd_THREADS_KIND_POSIX)
/**
 * The mutex is POSIX Threads' mutex
 */
#  define mhd_MUTEX_KIND_PTHREAD 1
#  include <pthread.h>
#  include "sys_null_macro.h"
#elif defined(mhd_THREADS_KIND_W32)
#  include "sys_w32_ver.h"
#  if _WIN32_WINNT >= 0x0600 /* Vista or later */ && \
  ! defined (MHD_NO_W32_SRWLOCKS)
/**
 * The mutex is W32 SRW lock
 */
#    define mhd_MUTEX_KIND_W32_SRW 1
#  else
/**
 * The mutex is W32 Critical Section
 */
#    define mhd_MUTEX_KIND_W32_CS 1
#  endif
#  if 0 /* _WIN32_WINNT >= 0x0602 */ /* Win8 or later */
/* This include does not work as _ARM_ or _AMD64_ macros
   are missing */
#    include <synchapi.h>
#  else
#    include <windows.h>
#  endif
#else
#error No base mutex API is available.
#endif

#include "mhd_panic.h"

#if defined(mhd_MUTEX_KIND_PTHREAD)
typedef pthread_mutex_t mhd_mutex;
#elif defined(mhd_MUTEX_KIND_W32_SRW)
typedef SRWLOCK mhd_mutex;
#elif defined(mhd_MUTEX_KIND_W32_CS)
typedef CRITICAL_SECTION mhd_mutex;
#endif

#if defined(mhd_MUTEX_KIND_PTHREAD)
/**
 * Initialise a new mutex.
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#  define mhd_mutex_init(pmutex) (0 == pthread_mutex_init ((pmutex), NULL))
#elif defined(mhd_MUTEX_KIND_W32_SRW)
/**
 * Initialise a new mutex.
 * @param pmutex the pointer to the mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_init(pmutex) (InitializeSRWLock ((pmutex)), ! 0)
#elif defined(mhd_MUTEX_KIND_W32_CS)
#  if _WIN32_WINNT < 0x0600
/* Before Vista */
/**
 * Initialise a new mutex.
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#    define mhd_mutex_init(pmutex) \
        (InitializeCriticalSectionAndSpinCount ((pmutex), 0))
#  else
/* The function always succeed starting from Vista */
/**
 * Initialise a new mutex.
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#    define mhd_mutex_init(pmutex) \
        (((void) InitializeCriticalSection (pmutex)), ! 0)
#  endif
#endif

#ifdef mhd_MUTEX_KIND_W32_CS
#  if _WIN32_WINNT < 0x0600
/* Before Vista */
/**
 * Initialise a new mutex for short locks.
 *
 * Initialised mutex is optimised for locks held only for very short period of
 * time. It should be used when only a single or just a few variables are
 * modified under the lock.
 *
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#    define mhd_mutex_init_short(pmutex) \
        (InitializeCriticalSectionAndSpinCount ((pmutex), 128))
#  else
/* The function always succeed starting from Vista */
/**
 * Initialise a new mutex for short locks.
 *
 * Initialised mutex is optimised for locks held only for very short period of
 * time. It should be used when only a single or just a few variables are
 * modified under the lock.
 *
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#    define mhd_mutex_init_short(pmutex) \
        ((void) InitializeCriticalSectionAndSpinCount ((pmutex), 128), ! 0)
#  endif
#endif

#ifndef mhd_mutex_init_short
#  define mhd_mutex_init_short(pmutex) mhd_mutex_init ((pmutex))
#endif

#if defined(mhd_MUTEX_KIND_PTHREAD)
#  if defined(PTHREAD_MUTEX_INITIALIZER)
/**
 * The value to statically initialise mutex
 */
#    define mhd_MUTEX_INITIALISER_STAT       PTHREAD_MUTEX_INITIALIZER
#  endif /* PTHREAD_MUTEX_INITIALIZER */
#elif defined(mhd_MUTEX_KIND_W32_SRW)
#  if defined(SRWLOCK_INIT)
/**
 * The value to statically initialise mutex
 */
#    define mhd_MUTEX_INITIALISER_STAT       SRWLOCK_INIT
#  endif
#endif

#ifdef mhd_MUTEX_INITIALISER_STAT
/**
 *  Define static mutex and statically initialise it.
 */
#  define mhd_MUTEX_STATIC_DEFN_INIT(m) \
        static mhd_mutex m = mhd_MUTEX_INITIALISER_STAT
#endif

#if defined(mhd_MUTEX_KIND_PTHREAD)
/**
 * Destroy previously initialised mutex.
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#  define mhd_mutex_destroy(pmutex) (0 == pthread_mutex_destroy ((pmutex)))
#elif defined(mhd_MUTEX_KIND_W32_SRW)
/**
 * Destroy (no-op) previously initialised mutex.
 * @param pmutex the pointer to the mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_destroy(pmutex) ((void) (pmutex), ! 0)
#elif defined(mhd_MUTEX_KIND_W32_CS)
/**
 * Destroy previously initialised mutex.
 * @param pmutex the pointer to the mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_destroy(pmutex) (DeleteCriticalSection ((pmutex)), ! 0)
#endif


#if defined(mhd_MUTEX_KIND_PTHREAD)
/**
 * Acquire a lock on previously initialised mutex.
 * If the mutex was already locked by other thread, function blocks until
 * the mutex becomes available.
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#  define mhd_mutex_lock(pmutex) (0 == pthread_mutex_lock ((pmutex)))
#elif defined(mhd_MUTEX_KIND_W32_SRW)
/**
 * Acquire a lock on previously initialised mutex.
 * If the mutex was already locked by other thread, function blocks until
 * the mutex becomes available.
 * @param pmutex the pointer to the mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_lock(pmutex) (AcquireSRWLockExclusive ((pmutex)), ! 0)
#elif defined(mhd_MUTEX_KIND_W32_CS)
/**
 * Acquire a lock on previously initialised mutex.
 * If the mutex was already locked by other thread, function blocks until
 * the mutex becomes available.
 * @param pmutex the pointer to the mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_lock(pmutex) (EnterCriticalSection ((pmutex)), ! 0)
#endif

#if defined(mhd_MUTEX_KIND_PTHREAD)
/**
 * Unlock previously locked mutex.
 * @param pmutex the pointer to the mutex
 * @return nonzero on success, zero otherwise
 */
#  define mhd_mutex_unlock(pmutex) (0 == pthread_mutex_unlock ((pmutex)))
#elif defined(mhd_MUTEX_KIND_W32_SRW)
/**
 * Acquire a lock on previously initialised mutex.
 * If the mutex was already locked by other thread, function blocks until
 * the mutex becomes available.
 * @param pmutex the pointer to the mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_unlock(pmutex) (ReleaseSRWLockExclusive ((pmutex)), ! 0)
#elif defined(mhd_MUTEX_KIND_W32_CS)
/**
 * Unlock previously initialised and locked mutex.
 * @param pmutex pointer to mutex
 * @return always nonzero (success)
 */
#  define mhd_mutex_unlock(pmutex) (LeaveCriticalSection ((pmutex)), ! 0)
#endif

/**
 * Destroy previously initialised mutex and abort execution if error is
 * detected.
 * @param pmutex the pointer to the mutex
 */
#define mhd_mutex_destroy_chk(pmutex) do {        \
          if (! mhd_mutex_destroy (pmutex))        \
          MHD_PANIC ("Failed to destroy mutex.\n"); \
} while (0)

/**
 * Acquire a lock on previously initialised mutex.
 * If the mutex was already locked by other thread, function blocks until
 * the mutex becomes available.
 * If error is detected, execution is aborted.
 * @param pmutex the pointer to the mutex
 */
#define mhd_mutex_lock_chk(pmutex) do {        \
          if (! mhd_mutex_lock (pmutex))        \
          MHD_PANIC ("Failed to lock mutex.\n"); \
} while (0)

/**
 * Unlock previously locked mutex.
 * If error is detected, execution is aborted.
 * @param pmutex the pointer to the mutex
 */
#define mhd_mutex_unlock_chk(pmutex) do {        \
          if (! mhd_mutex_unlock (pmutex))        \
          MHD_PANIC ("Failed to unlock mutex.\n"); \
} while (0)

#else  /* ! MHD_SUPPORT_THREADS */

#  define mhd_mutex_init(ignored) (! 0)
#  define mhd_mutex_init_short(ignored) (! 0)
#  define mhd_MUTEX_INITIALISER_STAT /* empty */
#  define mhd_MUTEX_STATIC_DEFN_INIT(ignored) /* nothing */
#  define mhd_mutex_destroy(ignored) (! 0)
#  define mhd_mutex_destroy_chk(ignored) ((void) 0)
#  define mhd_mutex_lock(ignored) (! 0)
#  define mhd_mutex_lock_chk(ignored) ((void) 0)
#  define mhd_mutex_unlock(ignored) (! 0)
#  define mhd_mutex_unlock_chk(ignored) ((void) 0)

#endif /* ! MHD_SUPPORT_THREADS */

#endif /* ! MHD_LOCKS_H */
