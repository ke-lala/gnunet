/*
   This file is part of GNUnet.
   Copyright (C) 2024 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author Tobias Frisch
 * @file schedule.c
 */

#include "schedule.h"

#include <glib-2.0/glib-unix.h>
#include <glib-2.0/glib.h>
#include <pthread.h>

#ifndef MESSENGER_APPLICATION_NO_EVENT_FD
#include <sys/eventfd.h>
#endif

static void
semaphore_init(MESSENGER_Semaphore *semaphore,
               unsigned int val)
{
  g_assert(semaphore);

  g_assert(0 == pthread_mutex_init(&(semaphore->mutex), NULL));
  g_assert(0 == pthread_cond_init(&(semaphore->condition), NULL));

  semaphore->counter = val;
}

static void
semaphore_destroy(MESSENGER_Semaphore *semaphore)
{
  g_assert(0 == pthread_cond_destroy(&(semaphore->condition)));
  g_assert(0 == pthread_mutex_destroy(&(semaphore->mutex)));
}

static void
semaphore_down(MESSENGER_Semaphore *semaphore)
{
  g_assert(semaphore);

  g_assert(0 == pthread_mutex_lock(&(semaphore->mutex)));
  while (0 == semaphore->counter)
    pthread_cond_wait(&(semaphore->condition), &(semaphore->mutex));
  semaphore->counter--;
  g_assert(0 == pthread_mutex_unlock(&(semaphore->mutex)));
}

static void
semaphore_up(MESSENGER_Semaphore *semaphore)
{
  g_assert(semaphore);

  g_assert(0 == pthread_mutex_lock(&(semaphore->mutex)));
  semaphore->counter++;
  g_assert(0 == pthread_mutex_unlock(&(semaphore->mutex)));

  pthread_cond_signal(&(semaphore->condition));
}

static void
signal_init(MESSENGER_SignalHandle *handle)
{
  g_assert(handle);

#ifdef MESSENGER_APPLICATION_NO_EVENT_FD
  g_assert(0 == pipe(handle->pipe_fds));
#else
  handle->event_fd = eventfd(0, 0);
  g_assert(-1 != handle->event_fd);
#endif
}

static void
signal_destroy(MESSENGER_SignalHandle *handle)
{
  g_assert(handle);

#ifdef MESSENGER_APPLICATION_NO_EVENT_FD
  close(handle->pipe_fds[0]);
  close(handle->pipe_fds[1]);
#else
  close(handle->event_fd);
#endif
}

static int
signal_fd(MESSENGER_SignalHandle *handle,
          unsigned char index)
{
  g_assert(handle);

  int fd;

#ifdef MESSENGER_APPLICATION_NO_EVENT_FD
  fd = handle->pipe_fds[index];
#else
  fd = handle->event_fd;
#endif
  
  g_assert(-1 != fd);
  return fd;
}

static MESSENGER_ScheduleSignal
signal_read(MESSENGER_SignalHandle *handle)
{
  const int fd = signal_fd(handle, 0);

#ifdef MESSENGER_APPLICATION_NO_EVENT_FD
  unsigned char data;
#else
  unsigned long data;
#endif
  
  g_assert(sizeof(data) == read(fd, &data, sizeof(data)));
  return (MESSENGER_ScheduleSignal) data;
}

static void
signal_write(MESSENGER_SignalHandle *handle,
             MESSENGER_ScheduleSignal val)
{
  const int fd = signal_fd(handle, 1);

#ifdef MESSENGER_APPLICATION_NO_EVENT_FD
  const unsigned char data = (unsigned char) val;
#else
  const unsigned long data = (unsigned long) val;
#endif

  g_assert(sizeof(data) == write(fd, &data, sizeof(data)));
}

void
schedule_init(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);
  memset(schedule, 0, sizeof(MESSENGER_Schedule));

  signal_init(&(schedule->push_signal));

  semaphore_init(&(schedule->push_sem), 0);
  semaphore_init(&(schedule->sync_sem), 0);
}

static gboolean
__schedule_pushed_handling(MESSENGER_Schedule *schedule,
                           MESSENGER_ScheduleSignal val)
{
  g_assert(schedule);

  gboolean keep;

  switch (val)
  {
    case MESSENGER_SCHEDULE_SIGNAL_RUN:
      g_assert(schedule->function);

      keep = schedule->function(schedule->data);

      schedule->function = NULL;
      schedule->data = NULL;
      break;
    case MESSENGER_SCHEDULE_SIGNAL_LOCK:
      g_assert(!(schedule->function));

      keep = TRUE;

      semaphore_up(&(schedule->sync_sem));
      semaphore_down(&(schedule->push_sem));
      break;
    default:
      return FALSE;
  }

  return keep;
}

static void
__schedule_exit_handling(MESSENGER_Schedule *schedule,
                         MESSENGER_ScheduleSignal val)
{
  g_assert(schedule);

  semaphore_up(&(schedule->sync_sem));
}

static void
__schedule_setup_push_task(MESSENGER_Schedule *schedule);

static void
__schedule_pushed_task(void *cls)
{
  MESSENGER_Schedule *schedule = cls;
  MESSENGER_ScheduleSignal val;

  g_assert(schedule);
  schedule->task = NULL;

  val = signal_read(&(schedule->push_signal));

  if (__schedule_pushed_handling(schedule, val))
    __schedule_setup_push_task(schedule);

  __schedule_exit_handling(schedule, val);
}

static void
__schedule_setup_push_task(MESSENGER_Schedule *schedule)
{
  struct GNUNET_NETWORK_FDSet *fd = GNUNET_NETWORK_fdset_create ();
  GNUNET_NETWORK_fdset_set_native(
    fd,
    signal_fd(&(schedule->push_signal), 0)
  );

  schedule->task = GNUNET_SCHEDULER_add_select(
    GNUNET_SCHEDULER_PRIORITY_DEFAULT,
    GNUNET_TIME_relative_get_forever_(),
    fd,
    NULL,
    __schedule_pushed_task,
    schedule
  );

  GNUNET_NETWORK_fdset_destroy(fd);
}

void
schedule_load_gnunet(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);
  __schedule_setup_push_task(schedule);
}

static gboolean
__schedule_pushed(gint fd,
                  GIOCondition condition,
                  gpointer user_data)
{
  MESSENGER_Schedule *schedule = user_data;
  MESSENGER_ScheduleSignal val;
  gboolean keep;
  guint task;

  g_assert(schedule);
  task = schedule->poll;
  schedule->poll = 0;

  val = signal_read(&(schedule->push_signal));

  keep = __schedule_pushed_handling(schedule, val);

  if (keep)
    schedule->poll = task;

  __schedule_exit_handling(schedule, val);
  return keep;
}

void
schedule_load_glib(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);
  schedule->poll = g_unix_fd_add(
    signal_fd(&(schedule->push_signal), 0),
    G_IO_IN,
    __schedule_pushed,
    schedule
  );
}

void
schedule_cleanup(MESSENGER_Schedule *schedule)
{
  g_assert(schedule);

  if (schedule->task)
    GNUNET_SCHEDULER_cancel(schedule->task);
  if (schedule->poll)
    g_source_remove(schedule->poll);

  semaphore_destroy(&(schedule->push_sem));
  semaphore_destroy(&(schedule->sync_sem));

  signal_destroy(&(schedule->push_signal));

  memset(schedule, 0, sizeof(MESSENGER_Schedule));
}

void
schedule_sync_run(MESSENGER_Schedule *schedule,
                  GSourceFunc function,
                  gpointer data)
{
  g_assert(
    (schedule) &&
    (!(schedule->locked)) &&
    (!(schedule->function)) &&
    (function)
  );

  schedule->function = function;
  schedule->data = data;

  const MESSENGER_ScheduleSignal push = MESSENGER_SCHEDULE_SIGNAL_RUN;

  signal_write(&(schedule->push_signal), push);
  semaphore_down(&(schedule->sync_sem));
}

void
schedule_sync_lock(MESSENGER_Schedule *schedule)
{
  g_assert(
    (schedule) &&
    (!(schedule->locked)) &&
    (!(schedule->function))
  );

  const MESSENGER_ScheduleSignal push = MESSENGER_SCHEDULE_SIGNAL_LOCK;

  signal_write(&(schedule->push_signal), push);
  semaphore_down(&(schedule->sync_sem));

  schedule->locked = TRUE;
}

void
schedule_sync_unlock(MESSENGER_Schedule *schedule)
{
  g_assert(
    (schedule) &&
    (schedule->locked) &&
    (!(schedule->function))
  );

  semaphore_up(&(schedule->push_sem));
  semaphore_down(&(schedule->sync_sem));

  schedule->locked = FALSE;
}
