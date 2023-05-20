/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-parallel.c
 * Copyright (C) 2018 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

extern "C"
{

#include "core-types.h"

#include "config/gimpgeglconfig.h"

#include "gimp.h"
#include "gimp-parallel.h"
#include "gimpasync.h"
#include "gimpcancelable.h"


#define GIMP_PARALLEL_MAX_THREADS           64
#define GIMP_PARALLEL_RUN_ASYNC_MAX_THREADS  1


typedef struct
{
  GimpAsync        *async;
  gint              priority;
  GimpRunAsyncFunc  func;
  gpointer          user_data;
  GDestroyNotify    user_data_destroy_func;
} GimpParallelRunAsyncTask;

typedef struct
{
  GThread   *thread;

  gboolean   quit;

  GimpAsync *current_async;
} GimpParallelRunAsyncThread;


/*  local function prototypes  */

static void                       gimp_parallel_notify_num_processors   (GimpGeglConfig             *config);

static void                       gimp_parallel_set_n_threads           (gint                        n_threads,
                                                                         gboolean                    finish_tasks);

static void                       gimp_parallel_run_async_set_n_threads (gint                        n_threads,
                                                                         gboolean                    finish_tasks);
static gpointer                   gimp_parallel_run_async_thread_func   (GimpParallelRunAsyncThread *thread);
static void                       gimp_parallel_run_async_enqueue_task  (GimpParallelRunAsyncTask   *task);
static GimpParallelRunAsyncTask * gimp_parallel_run_async_dequeue_task  (void);
static gboolean                   gimp_parallel_run_async_execute_task  (GimpParallelRunAsyncTask   *task);
static void                       gimp_parallel_run_async_abort_task    (GimpParallelRunAsyncTask   *task);
static void                       gimp_parallel_run_async_cancel        (GimpAsync                  *async);
static void                       gimp_parallel_run_async_waiting       (GimpAsync                  *async);


/*  local variables  */

static gint                       gimp_parallel_run_async_n_threads = 0;
static GimpParallelRunAsyncThread gimp_parallel_run_async_threads[GIMP_PARALLEL_RUN_ASYNC_MAX_THREADS];

static GMutex                     gimp_parallel_run_async_mutex;
static GCond                      gimp_parallel_run_async_cond;
static GQueue                     gimp_parallel_run_async_queue = G_QUEUE_INIT;


/*  public functions  */


void
gimp_parallel_init (Gimp *gimp)
{
  GimpGeglConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GEGL_CONFIG (gimp->config);

  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (gimp_parallel_notify_num_processors),
                    NULL);

  gimp_parallel_notify_num_processors (config);
}

void
gimp_parallel_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        (gpointer) gimp_parallel_notify_num_processors,
                                        NULL);

  /* stop all threads */
  gimp_parallel_set_n_threads (0, /* finish_tasks = */ FALSE);
}

GimpAsync *
gimp_parallel_run_async (GimpRunAsyncFunc func,
                         gpointer         user_data)
{
  return gimp_parallel_run_async_full (0, func, user_data, NULL);
}

GimpAsync *
gimp_parallel_run_async_full (gint             priority,
                              GimpRunAsyncFunc func,
                              gpointer         user_data,
                              GDestroyNotify   user_data_destroy_func)
{
  GimpAsync                *async;
  GimpParallelRunAsyncTask *task;

  g_return_val_if_fail (func != NULL, NULL);

  async = gimp_async_new ();

  task = g_slice_new (GimpParallelRunAsyncTask);

  task->async                  = GIMP_ASYNC (g_object_ref (async));
  task->priority               = priority;
  task->func                   = func;
  task->user_data              = user_data;
  task->user_data_destroy_func = user_data_destroy_func;

  if (gimp_parallel_run_async_n_threads > 0)
    {
      g_signal_connect_after (async, "cancel",
                              G_CALLBACK (gimp_parallel_run_async_cancel),
                              NULL);
      g_signal_connect_after (async, "waiting",
                              G_CALLBACK (gimp_parallel_run_async_waiting),
                              NULL);

      g_mutex_lock (&gimp_parallel_run_async_mutex);

      gimp_parallel_run_async_enqueue_task (task);

      g_cond_signal (&gimp_parallel_run_async_cond);

      g_mutex_unlock (&gimp_parallel_run_async_mutex);
    }
  else
    {
      while (gimp_parallel_run_async_execute_task (task));
    }

  return async;
}

GimpAsync *
gimp_parallel_run_async_independent (GimpRunAsyncFunc func,
                                     gpointer         user_data)
{
  return gimp_parallel_run_async_independent_full (0, func, user_data);
}

GimpAsync *
gimp_parallel_run_async_independent_full (gint             priority,
                                          GimpRunAsyncFunc func,
                                          gpointer         user_data)
{
  GimpAsync                *async;
  GimpParallelRunAsyncTask *task;
  GThread                  *thread;

  g_return_val_if_fail (func != NULL, NULL);

  async = gimp_async_new ();

  task = g_slice_new0 (GimpParallelRunAsyncTask);

  task->async     = GIMP_ASYNC (g_object_ref (async));
  task->priority  = priority;
  task->func      = func;
  task->user_data = user_data;

  thread = g_thread_new (
    "async-ind",
    [] (gpointer data) -> gpointer
    {
      GimpParallelRunAsyncTask *task = (GimpParallelRunAsyncTask *) data;

      /* adjust the thread's priority */
#if defined (G_OS_WIN32)
      if (task->priority < 0)
        {
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL);
        }
      else if (task->priority > 0)
        {
          SetThreadPriority (GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN);
        }
#elif defined (HAVE_UNISTD_H) && defined (__gnu_linux__)
      if (task->priority)
        {
          nice (task->priority);
        }
#endif

      while (gimp_parallel_run_async_execute_task (task));

      return NULL;
    },
    task);

  gimp_async_add_callback (async,
                           [] (GimpAsync *async,
                               gpointer   thread)
                           {
                             g_thread_join ((GThread *) thread);
                           },
                           thread);

  return async;
}


/*  private functions  */


static void
gimp_parallel_notify_num_processors (GimpGeglConfig *config)
{
  gimp_parallel_set_n_threads (config->num_processors,
                               /* finish_tasks = */ TRUE);
}

static void
gimp_parallel_set_n_threads (gint     n_threads,
                             gboolean finish_tasks)
{
  gimp_parallel_run_async_set_n_threads (n_threads, finish_tasks);
}

static void
gimp_parallel_run_async_set_n_threads (gint     n_threads,
                                       gboolean finish_tasks)
{
  gint i;

  n_threads = CLAMP (n_threads, 0, GIMP_PARALLEL_RUN_ASYNC_MAX_THREADS);

  if (n_threads > gimp_parallel_run_async_n_threads) /* need more threads */
    {
      for (i = gimp_parallel_run_async_n_threads; i < n_threads; i++)
        {
          GimpParallelRunAsyncThread *thread =
            &gimp_parallel_run_async_threads[i];

          thread->quit = FALSE;

          thread->thread = g_thread_new (
            "async",
            (GThreadFunc) gimp_parallel_run_async_thread_func,
            thread);
        }
    }
  else if (n_threads < gimp_parallel_run_async_n_threads) /* need less threads */
    {
      g_mutex_lock (&gimp_parallel_run_async_mutex);

      for (i = n_threads; i < gimp_parallel_run_async_n_threads; i++)
        {
          GimpParallelRunAsyncThread *thread =
            &gimp_parallel_run_async_threads[i];

          thread->quit = TRUE;

          if (thread->current_async && ! finish_tasks)
            gimp_cancelable_cancel (GIMP_CANCELABLE (thread->current_async));
        }

      g_cond_broadcast (&gimp_parallel_run_async_cond);

      g_mutex_unlock (&gimp_parallel_run_async_mutex);

      for (i = n_threads; i < gimp_parallel_run_async_n_threads; i++)
        {
          GimpParallelRunAsyncThread *thread =
            &gimp_parallel_run_async_threads[i];

          g_thread_join (thread->thread);
        }
    }

  gimp_parallel_run_async_n_threads = n_threads;

  if (n_threads == 0)
    {
      GimpParallelRunAsyncTask *task;

      /* finish remaining tasks */
      while ((task = gimp_parallel_run_async_dequeue_task ()))
        {
          if (finish_tasks)
            while (gimp_parallel_run_async_execute_task (task));
          else
            gimp_parallel_run_async_abort_task (task);
        }
    }
}

static gpointer
gimp_parallel_run_async_thread_func (GimpParallelRunAsyncThread *thread)
{
  g_mutex_lock (&gimp_parallel_run_async_mutex);

  while (TRUE)
    {
      GimpParallelRunAsyncTask *task;

      while (! thread->quit &&
             (task = gimp_parallel_run_async_dequeue_task ()))
        {
          gboolean resume;

          thread->current_async = GIMP_ASYNC (g_object_ref (task->async));

          do
            {
              g_mutex_unlock (&gimp_parallel_run_async_mutex);

              resume = gimp_parallel_run_async_execute_task (task);

              g_mutex_lock (&gimp_parallel_run_async_mutex);
            }
          while (resume &&
                 (g_queue_is_empty (&gimp_parallel_run_async_queue) ||
                  task->priority <
                  ((GimpParallelRunAsyncTask *)
                     g_queue_peek_head (
                       &gimp_parallel_run_async_queue))->priority));

          g_clear_object (&thread->current_async);

          if (resume)
            gimp_parallel_run_async_enqueue_task (task);
        }

      if (thread->quit)
        break;

      g_cond_wait (&gimp_parallel_run_async_cond,
                   &gimp_parallel_run_async_mutex);
    }

  g_mutex_unlock (&gimp_parallel_run_async_mutex);

  return NULL;
}

static void
gimp_parallel_run_async_enqueue_task (GimpParallelRunAsyncTask *task)
{
  GList *link;
  GList *iter;

  if (gimp_async_is_canceled (task->async))
    {
      gimp_parallel_run_async_abort_task (task);

      return;
    }

  link       = g_list_alloc ();
  link->data = task;

  g_object_set_data (G_OBJECT (task->async),
                     "gimp-parallel-run-async-link", link);

  for (iter = g_queue_peek_tail_link (&gimp_parallel_run_async_queue);
       iter;
       iter = g_list_previous (iter))
    {
      GimpParallelRunAsyncTask *other_task =
        (GimpParallelRunAsyncTask *) iter->data;

      if (other_task->priority <= task->priority)
        break;
    }

  if (iter)
    {
      link->prev = iter;
      link->next = iter->next;

      iter->next = link;

      if (link->next)
        link->next->prev = link;
      else
        gimp_parallel_run_async_queue.tail = link;

      gimp_parallel_run_async_queue.length++;
    }
  else
    {
      g_queue_push_head_link (&gimp_parallel_run_async_queue, link);
    }
}

static GimpParallelRunAsyncTask *
gimp_parallel_run_async_dequeue_task (void)
{
  GimpParallelRunAsyncTask *task;

  task = (GimpParallelRunAsyncTask *) g_queue_pop_head (
                                        &gimp_parallel_run_async_queue);

  if (task)
    {
      g_object_set_data (G_OBJECT (task->async),
                         "gimp-parallel-run-async-link", NULL);
    }

  return task;
}

static gboolean
gimp_parallel_run_async_execute_task (GimpParallelRunAsyncTask *task)
{
  if (gimp_async_is_canceled (task->async))
    {
      gimp_parallel_run_async_abort_task (task);

      return FALSE;
    }

  task->func (task->async, task->user_data);

  if (gimp_async_is_stopped (task->async))
    {
      g_object_unref (task->async);

      g_slice_free (GimpParallelRunAsyncTask, task);

      return FALSE;
    }

  return TRUE;
}

static void
gimp_parallel_run_async_abort_task (GimpParallelRunAsyncTask *task)
{
  if (task->user_data && task->user_data_destroy_func)
    task->user_data_destroy_func (task->user_data);

  gimp_async_abort (task->async);

  g_object_unref (task->async);

  g_slice_free (GimpParallelRunAsyncTask, task);
}

static void
gimp_parallel_run_async_cancel (GimpAsync *async)
{
  GList                    *link;
  GimpParallelRunAsyncTask *task = NULL;

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "gimp-parallel-run-async-link");

  if (! link)
    return;

  g_mutex_lock (&gimp_parallel_run_async_mutex);

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "gimp-parallel-run-async-link");

  if (link)
    {
      g_object_set_data (G_OBJECT (async),
                         "gimp-parallel-run-async-link", NULL);

      task = (GimpParallelRunAsyncTask *) link->data;

      g_queue_delete_link (&gimp_parallel_run_async_queue, link);
    }

  g_mutex_unlock (&gimp_parallel_run_async_mutex);

  if (task)
    gimp_parallel_run_async_abort_task (task);
}

static void
gimp_parallel_run_async_waiting (GimpAsync *async)
{
  GList *link;

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "gimp-parallel-run-async-link");

  if (! link)
    return;

  g_mutex_lock (&gimp_parallel_run_async_mutex);

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "gimp-parallel-run-async-link");

  if (link)
    {
      GimpParallelRunAsyncTask *task = (GimpParallelRunAsyncTask *) link->data;

      task->priority = G_MININT;

      g_queue_unlink         (&gimp_parallel_run_async_queue, link);
      g_queue_push_head_link (&gimp_parallel_run_async_queue, link);
    }

  g_mutex_unlock (&gimp_parallel_run_async_mutex);
}

} /* extern "C" */
