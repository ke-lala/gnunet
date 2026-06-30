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
 * @file util.c
 */

#include "util.h"

#include <pthread.h>
#include <stdio.h>

static GList *tasks = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct UTIL_CompleteTask
{
  GSourceFunc function;
  gpointer data;
  guint id;
};

static void
util_kill_task(gpointer task_data)
{
  g_assert(task_data);

  struct UTIL_CompleteTask *task = (struct UTIL_CompleteTask*) task_data;

  if (task->id)
    g_source_remove(task->id);

  g_free(task);
}

void
util_scheduler_cleanup()
{
  if (tasks)
    g_list_free_full(tasks, (GDestroyNotify) util_kill_task);

  tasks = NULL;
}

static gboolean
util_complete_task(gpointer task_data)
{
  g_assert(task_data);

  struct UTIL_CompleteTask *task = (struct UTIL_CompleteTask*) task_data;
  gboolean result = FALSE;

  pthread_mutex_lock(&mutex);
  if (!tasks)
    goto unlock_mutex;

  const GSourceFunc function = task->function;
  gpointer data = task->data;

  if (tasks)
    tasks = g_list_remove(tasks, task);

  g_free(task);

  result = function(data);

unlock_mutex:
  pthread_mutex_unlock(&mutex);
  return result;
}

guint
util_idle_add(GSourceFunc function,
              gpointer data)
{
  struct UTIL_CompleteTask *task = g_malloc(sizeof(struct UTIL_CompleteTask));

  task->function = function;
  task->data = data;
  task->id = g_idle_add(
    G_SOURCE_FUNC(util_complete_task),
    task
  );

  tasks = g_list_append(
    tasks,
    task
  );

  return task->id;
}

guint
util_immediate_add(GSourceFunc function,
                   gpointer data)
{
  struct UTIL_CompleteTask *task = g_malloc(sizeof(struct UTIL_CompleteTask));

  task->function = function;
  task->data = data;
  task->id = g_timeout_add(
    0,
    G_SOURCE_FUNC(util_complete_task),
    task
  );

  tasks = g_list_append(
    tasks,
    task
  );

  return task->id;
}

guint
util_timeout_add(guint interval,
                 GSourceFunc function,
                 gpointer data)
{
  struct UTIL_CompleteTask *task = g_malloc(sizeof(struct UTIL_CompleteTask));

  task->function = function;
  task->data = data;
  task->id = g_timeout_add(
    interval,
    G_SOURCE_FUNC(util_complete_task),
    task
  );

  tasks = g_list_append(
    tasks,
    task
  );

  return task->id;
}

guint
util_timeout_add_seconds(guint interval,
                         GSourceFunc function,
                         gpointer data)
{
  struct UTIL_CompleteTask *task = g_malloc(sizeof(struct UTIL_CompleteTask));

  task->function = function;
  task->data = data;
  task->id = g_timeout_add_seconds(
    interval,
    G_SOURCE_FUNC(util_complete_task),
    task
  );

  tasks = g_list_append(
    tasks,
    task
  );

  return task->id;
}

gboolean
util_source_remove(guint tag)
{
  struct UTIL_CompleteTask *task = NULL;
  GList *current = tasks;

  while (current)
  {
    task = (struct UTIL_CompleteTask*) current->data;

    if (task->id == tag)
      break;

    current = g_list_next(current);
    task = NULL;
  }

  if (!task)
    return FALSE;

  const gboolean result = g_source_remove(task->id);

  tasks = g_list_remove(tasks, task);
  g_free(task);

  return result;
}

gboolean
util_source_remove_by_data(gpointer data)
{
  struct UTIL_CompleteTask *task = NULL;
  GList *current = tasks;
  GList *matches = NULL;

  while (current)
  {
    task = (struct UTIL_CompleteTask*) current->data;

    if (task->data == data)
      matches = g_list_append(matches, task);

    current = g_list_next(current);
    task = NULL;
  }

  if (!matches)
    return FALSE;

  gboolean result = TRUE;

  current = matches;
  while (current)
  {
    task = (struct UTIL_CompleteTask*) current->data;

    result &= g_source_remove(task->id);
    tasks = g_list_remove(tasks, task);
    g_free(task);

    current = g_list_next(current);
  }

  g_list_free(matches);
  return result;
}
