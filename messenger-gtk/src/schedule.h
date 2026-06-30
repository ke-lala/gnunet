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
 * @file schedule.h
 */

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#include <glib-2.0/glib.h>
#include <gnunet/gnunet_util_lib.h>
#include <pthread.h>

typedef struct MESSENGER_Semaphore {
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  unsigned int counter;
} MESSENGER_Semaphore;

typedef struct MESSENGER_SignalHandle {
#ifdef MESSENGER_APPLICATION_NO_EVENT_FD
  int pipe_fds [2];
#else
  int event_fd;
#endif
} MESSENGER_SignalHandle;

typedef enum MESSENGER_ScheduleSignal : unsigned char {
  MESSENGER_SCHEDULE_SIGNAL_RUN = 1,
  MESSENGER_SCHEDULE_SIGNAL_LOCK = 2,
} MESSENGER_ScheduleSignal;

typedef struct MESSENGER_Schedule {
  MESSENGER_SignalHandle push_signal;

  MESSENGER_Semaphore push_sem;
  MESSENGER_Semaphore sync_sem;
  gboolean locked;

  GSourceFunc function;
  gpointer data;

  struct GNUNET_SCHEDULER_Task *task;
  guint poll;
} MESSENGER_Schedule;

/**
 * Initializes a schedule to synchronize a current thread
 * with another certain thread via pipes and mutexes.
 */
void
schedule_init(MESSENGER_Schedule *schedule);

/**
 * Completes initialization of a schedule to synchronize
 * another thread with the GNUnet scheduler.
 */
void
schedule_load_gnunet(MESSENGER_Schedule *schedule);

/**
 * Completes initialization of a schedule to synchronize
 * another thread with the GLib/GTK scheduler.
 */
void
schedule_load_glib(MESSENGER_Schedule *schedule);

/**
 * Cleanup a schedule and all of its resources for
 * its synchronization.
 */
void
schedule_cleanup(MESSENGER_Schedule *schedule);

/**
 * Calls a given function from the thread of a given
 * schedule and waits for its completion until then
 * in the current thread.
 */
void
schedule_sync_run(MESSENGER_Schedule *schedule,
                  GSourceFunc function,
                  gpointer data);

/**
 * Locks the thread of a given schedule to wait
 * until the schedule gets unlocked again from
 * the current thread. Execution only continues
 * in the current thread afterwards.
 */
void
schedule_sync_lock(MESSENGER_Schedule *schedule);

/**
 * Unlocks the thread of a given schedule from
 * the current thread. Execution continues in 
 * parallel again afterwards.
 */
void
schedule_sync_unlock(MESSENGER_Schedule *schedule);

#endif /* SCHEDULE_H_ */
